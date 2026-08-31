#ifndef SPOTIFYCURRENT_ANIMATION_H
#define SPOTIFYCURRENT_ANIMATION_H

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sys/types.h>

#include "Animation.h"
#include "Utils.h"
#include "parameters/param_system.hpp"

#ifndef ASSETS_DIR
#define ASSETS_DIR ".."
#endif

namespace animations {

struct SpotifyCurrentParams : ParameterSet<SpotifyCurrentParams> {
  // Provide tuple of references for iteration
  auto tuple() {
    return std::tie(cover_fade_duration, title_color, artists_color,
                    progress_bar_color);
  }
  auto const tuple() const {
    return std::tie(cover_fade_duration, title_color, artists_color,
                    progress_bar_color);
  }

  // Parameters for SpotifyCurrent
  PARAM_FLOAT(cover_fade_duration, 1.3f, 0.0f, 10.0f, "Cover fade duration")
  PARAM_COLOR(title_color, Color(255, 255, 255), "Title color")
  PARAM_COLOR(artists_color, Color(150, 150, 150), "Artists color")
  PARAM_COLOR(progress_bar_color, Color(50, 50, 50), "Progress bar color")
};

class SpotifyCurrent : public Animation<SpotifyCurrent, SpotifyCurrentParams> {
public:
  struct TrackData {
    std::string id;
    std::string name;
    std::string artists;
    std::vector<uint8_t> cover_rgb_data;
    int cover_width = 0;
    int cover_height = 0;
    long progress_ms = 0;
    long duration_ms = 0;
  };

  SpotifyCurrent(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    title_font.LoadFont(ASSETS_DIR "/deps/matrix/fonts/6x12.bdf");
    artists_font.LoadFont(ASSETS_DIR "/deps/matrix/fonts/6x10.bdf");

    // Load environment variables from .env file
    auto env = utils::loadEnv();
    if (!std::getenv("SPOTIFY_CLIENT_ID") ||
        !std::getenv("SPOTIFY_CLIENT_SECRET") ||
        !std::getenv("SPOTIFY_REFRESH_TOKEN")) {
      std::cerr << "Warning: SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET, or "
                   "SPOTIFY_REFRESH_TOKEN not set in environment.\n";
      return;
    }
    client_id = std::getenv("SPOTIFY_CLIENT_ID");
    client_secret = std::getenv("SPOTIFY_CLIENT_SECRET");
    refresh_token = std::getenv("SPOTIFY_REFRESH_TOKEN");
  }

  ~SpotifyCurrent();

  void animate(double time) override;

  void display_covers(float fade_progress);

  std::vector<std::string> wrap_text(const std::string &text,
                                     const rgb_matrix::Font &font,
                                     int max_width, int max_lines);

  void render_track_text(const TrackData &track_data);

  std::string name() const override { return "SpotifyCurrent"; }

  // Override mode parameter methods - Static has no parameters so all modes are
  // the same
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
  bool init_spotify();
  // Helper functions for the worker thread
  bool fetch_track_info(TrackData &out_data);
  bool download_image(const std::string &url, std::vector<uint8_t> &out_data,
                      int &out_width, int &out_height);
  void fetch_thread_worker();
  // Read a single IPC update (if available) from child process pipe
  bool read_ipc_update();

  std::string client_id, client_secret, refresh_token;
  TrackData prev_track_data{};
  TrackData pending_track_data;

  bool initialized = false;

  CURL *g_curl = nullptr;
  std::string g_access_token;
  std::string g_last_track_id;

  // Fade tracking
  double fade_progress = 0.0; // 0.0 = show old cover, 1.0 = show new cover
  bool is_fading = false;

  double last_animate_time = -1.0;
  double last_animate_call = 0.0;

  float displayed_progress_ratio = 0.0f;

  // Worker process management (was a thread before)
  pid_t fetch_pid = -1;
  std::mutex track_data_mutex;
  std::atomic<bool> stop_thread{false};
  std::atomic<bool> new_track_available{false};
  // IPC pipe fds: parent will keep read_fd, child will use write_fd
  int ipc_pipe_read_fd = -1;
  int ipc_pipe_write_fd = -1;
};
} // namespace animations

#endif // SPOTIFYCURRENT_ANIMATION_H