#ifndef SPOTIFYCURRENT_ANIMATION_H
#define SPOTIFYCURRENT_ANIMATION_H

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "Animation.h"
#include "Utils.h"
#include "parameters/param_system.hpp"

#include "Animations/SpotifyCurrent/SpotifyTrackData.h"
#include "Animations/SpotifyCurrent/SpotifyFetcher.h"

#ifndef ASSETS_DIR
#define ASSETS_DIR ".."
#endif

namespace animations {

struct SpotifyCurrentParams : ParameterSet<SpotifyCurrentParams> {
  auto tuple() {
    return std::tie(cover_fade_duration, title_color, artists_color,
                    progress_bar_color);
  }
  auto const tuple() const {
    return std::tie(cover_fade_duration, title_color, artists_color,
                    progress_bar_color);
  }

  PARAM_FLOAT(cover_fade_duration, 1.3f, 0.0f, 10.0f, "Cover fade duration")
  PARAM_COLOR(title_color, Color(255, 255, 255), "Title color")
  PARAM_COLOR(artists_color, Color(150, 150, 150), "Artists color")
  PARAM_COLOR(progress_bar_color, Color(50, 50, 50), "Progress bar color")
};

class SpotifyCurrent : public Animation<SpotifyCurrent, SpotifyCurrentParams> {
public:
  SpotifyCurrent(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    title_font.LoadFont(ASSETS_DIR "/deps/matrix/fonts/6x12.bdf");
    artists_font.LoadFont(ASSETS_DIR "/deps/matrix/fonts/6x10.bdf");

    auto env = utils::loadEnv();
    if (!std::getenv("SPOTIFY_CLIENT_ID") ||
        !std::getenv("SPOTIFY_CLIENT_SECRET") ||
        !std::getenv("SPOTIFY_REFRESH_TOKEN")) {
      std::cerr << "Warning: SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET, or "
                   "SPOTIFY_REFRESH_TOKEN not set in environment.\n";
      return;
    }
    
    fetcher_ = std::make_unique<SpotifyFetcher>(
      std::getenv("SPOTIFY_CLIENT_ID"),
      std::getenv("SPOTIFY_CLIENT_SECRET"),
      std::getenv("SPOTIFY_REFRESH_TOKEN")
    );
  }

  ~SpotifyCurrent() = default;

  void animate(double time) override;

  void display_covers(float fade_progress);

  std::vector<std::string> wrap_text(const std::string &text,
                                     const rgb_matrix::Font &font,
                                     int max_width, int max_lines);

  void render_track_text(const TrackData &track_data);

  std::string name() const override { return "SpotifyCurrent"; }

  void applyDefaultParameters() override {
    params_.cover_fade_duration.value = 1.3f;
    params_.title_color.value = Color(255, 255, 255);
    params_.artists_color.value = Color(150, 150, 150);
    params_.progress_bar_color.value = Color(50, 50, 50);
  }

  void applyPresetParameters() override {
    params_.cover_fade_duration.value = 1.3f;
    params_.title_color.value = Color(255, 255, 255);
    params_.artists_color.value = Color(150, 150, 150);
    params_.progress_bar_color.value = Color(65, 65, 65);
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
  rgb_matrix::Font title_font;
  rgb_matrix::Font artists_font;

private:
  std::unique_ptr<SpotifyFetcher> fetcher_;
  
  TrackData prev_track_data{};
  TrackData pending_track_data;

  bool initialized = false;

  double fade_progress = 0.0;
  bool is_fading = false;
  double last_animate_time = -1.0;
  double last_animate_call = 0.0;
  float displayed_progress_ratio = 0.0f;
  
  bool new_track_available = false;
};

} // namespace animations

#endif // SPOTIFYCURRENT_ANIMATION_H