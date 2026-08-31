#include "Animations/SpotifyCurrent/SpotifyCurrent.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

namespace animations {

void SpotifyCurrent::display_covers(float fade_progress) {
  if (prev_track_data.cover_rgb_data.empty() ||
      pending_track_data.cover_rgb_data.empty()) {
    return;
  }

  int x_offset = 12;
  int y_offset = 32;
  fade_progress = (fade_progress * fade_progress * (3 - 2 * fade_progress));
  float inv_progress = 1.0f - fade_progress;

  for (int y = 0; y < prev_track_data.cover_height; ++y) {
    for (int x = 0; x < prev_track_data.cover_width; ++x) {
      int index = (y * prev_track_data.cover_width + x) * 3;
      uint8_t r = prev_track_data.cover_rgb_data[index] * inv_progress +
                  pending_track_data.cover_rgb_data[index] * fade_progress;
      uint8_t g = prev_track_data.cover_rgb_data[index + 1] * inv_progress +
                  pending_track_data.cover_rgb_data[index + 1] * fade_progress;
      uint8_t b = prev_track_data.cover_rgb_data[index + 2] * inv_progress +
                  pending_track_data.cover_rgb_data[index + 2] * fade_progress;
      offscreen_canvas->SetPixel(x + x_offset, y + y_offset, r, g, b);
    }
  }
}

std::vector<std::string> SpotifyCurrent::wrap_text(const std::string &text,
                                                   const rgb_matrix::Font &font,
                                                   int max_width,
                                                   int max_lines) {
  std::vector<std::string> lines;
  std::istringstream words_stream(text);
  std::string word;
  std::string current_line;
  bool text_truncated = false;

  while (words_stream >> word) {
    std::string test_line =
        current_line.empty() ? word : current_line + " " + word;

    int line_width = 0;
    for (char c : test_line) {
      line_width += font.CharacterWidth(c);
    }

    if (line_width <= max_width) {
      current_line = test_line;
    } else {
      if (!current_line.empty()) {
        lines.push_back(current_line);
        if (lines.size() >= static_cast<size_t>(max_lines)) {
          text_truncated = true;
          break;
        }
      }
      current_line = word;

      int word_width = 0;
      for (char c : word) {
        word_width += font.CharacterWidth(c);
      }
      if (word_width > max_width) {
        current_line.clear();
        int accumulated_width = 0;
        for (size_t i = 0; i < word.size(); ++i) {
          int char_width = font.CharacterWidth(word[i]);
          if (accumulated_width + char_width >
              max_width - font.CharacterWidth('.') * 3) {
            current_line += "...";
            break;
          }
          current_line += word[i];
          accumulated_width += char_width;
        }
        lines.push_back(current_line);
        if (lines.size() >= static_cast<size_t>(max_lines)) {
          text_truncated = true;
          break;
        }
        current_line.clear();
      }
    }
  }

  if (!current_line.empty() && lines.size() < static_cast<size_t>(max_lines)) {
    lines.push_back(current_line);
  } else if (!current_line.empty() &&
             lines.size() >= static_cast<size_t>(max_lines)) {
    text_truncated = true;
  }

  if (text_truncated && !lines.empty()) {
    std::string &last_line = lines.back();
    int ellipsis_width = font.CharacterWidth('.') * 3;
    int current_width = 0;
    for (char c : last_line) {
      current_width += font.CharacterWidth(c);
    }

    while (current_width + ellipsis_width > max_width && !last_line.empty()) {
      char removed = last_line.back();
      last_line.pop_back();
      current_width -= font.CharacterWidth(removed);
    }

    while (!last_line.empty() && last_line.back() == ' ') {
      last_line.pop_back();
    }

    last_line += "...";
  }

  return lines;
}

void SpotifyCurrent::render_track_text(const TrackData &track_data) {
  const int cover_x = 12;
  const int cover_y = 32;
  const int cover_size = 64;
  const int cover_center_y = cover_y + cover_size / 2;
  const int text_x = cover_x + cover_size + 10;
  const int max_text_width = matrix->width() - 12 - text_x;

  offscreen_canvas->SubFill(text_x, cover_y, matrix->width() - text_x,
                            cover_size, 0, 0, 0);

  auto title_lines = wrap_text(track_data.name, title_font, max_text_width, 3);
  auto artists_lines =
      wrap_text(track_data.artists, artists_font, max_text_width, 3);

  int spacing = 6;
  int line_spacing = 0;
  int title_height =
      title_lines.size() * title_font.height() -
      (title_lines.size() > 1 ? (title_lines.size() - 1) * line_spacing : 0);
  int artists_height =
      artists_lines.size() * artists_font.height() -
      (artists_lines.size() > 1 ? (artists_lines.size() - 1) * line_spacing
                                : 0);
  int total_text_height = title_height + spacing + artists_height;

  int text_start_y = cover_center_y - (total_text_height / 2);

  int title_y = text_start_y + title_font.baseline();
  for (const auto &line : title_lines) {
    rgb_matrix::Color draw_title_color(params_.title_color.value.r,
                                       params_.title_color.value.g,
                                       params_.title_color.value.b);
    rgb_matrix::DrawText(offscreen_canvas, title_font, text_x, title_y,
                         draw_title_color, nullptr, line.c_str());
    title_y += title_font.height() - line_spacing;
  }

  int artists_y =
      text_start_y + title_height + spacing + artists_font.baseline();
  for (const auto &line : artists_lines) {
    rgb_matrix::Color draw_artists_color(params_.artists_color.value.r,
                                         params_.artists_color.value.g,
                                         params_.artists_color.value.b);
    rgb_matrix::DrawText(offscreen_canvas, artists_font, text_x, artists_y,
                         draw_artists_color, nullptr, line.c_str());
    artists_y += artists_font.height() - line_spacing;
  }
}

void SpotifyCurrent::animate(double time) {
  if (last_animate_call <= 0) {
    last_animate_call = time;
  }

  if (!initialized && fetcher_) {
    if (fetcher_->start()) {
      initialized = true;
    } else {
      std::cerr << "SpotifyCurrent initialization failed.\n";
    }
  }

  if (fetcher_) {
    TrackData update;
    while (fetcher_->read_update(update)) {
      if (!update.cover_rgb_data.empty() || update.cover_width > 0 ||
          update.cover_height > 0 || update.id != pending_track_data.id) {
        pending_track_data = std::move(update);
        new_track_available = true;
      } else {
        pending_track_data.progress_ms = update.progress_ms;
        pending_track_data.duration_ms = update.duration_ms;
        pending_track_data.is_playing = update.is_playing;
      }
    }
  }

  if (is_fading && fade_progress < 1.0) {
    fade_progress += (time - last_animate_time) /
                     static_cast<double>(params_.cover_fade_duration.value);
    last_animate_time = time;
  } else {
    if (is_fading) {
      fade_progress = 1.0;
      is_fading = false;
      prev_track_data = pending_track_data;
    }
    last_animate_time = time;
  }

  if (new_track_available) {
    new_track_available = false;
    TrackData new_track = pending_track_data;

    if (prev_track_data.id != new_track.id && !is_fading) {
      if (prev_track_data.cover_rgb_data.empty()) {
        prev_track_data = new_track;
        pending_track_data = new_track;
        fade_progress = 1.0;
        is_fading = false;
      } else {
        pending_track_data = new_track;
        fade_progress = 0.0;
        is_fading = true;
      }
    }
  }

  int bar_y = matrix->height() - 1 - 1;
  int bar_width = matrix->width();
  if (pending_track_data.duration_ms > 0) {
    float progress_ratio = static_cast<float>(pending_track_data.progress_ms) /
                           static_cast<float>(pending_track_data.duration_ms);
    progress_ratio = std::clamp(progress_ratio, 0.0f, 1.0f);
    float progress_diff = progress_ratio - displayed_progress_ratio;
    displayed_progress_ratio +=
        progress_diff * 4.0f * (time - last_animate_call);

    displayed_progress_ratio = std::clamp(displayed_progress_ratio, 0.0f, 1.0f);
    float filled_width_f = bar_width * displayed_progress_ratio;
    int filled_width = static_cast<int>(bar_width * displayed_progress_ratio);
    float remaining_width_f = filled_width_f - filled_width;

    offscreen_canvas->Clear();

    for (int x = 0; x < bar_width; ++x) {
      if (x < filled_width) {
        offscreen_canvas->SetPixel(x, bar_y, params_.progress_bar_color.value.r,
                                   params_.progress_bar_color.value.g,
                                   params_.progress_bar_color.value.b);
      } else if (x == filled_width) {
        uint8_t r = static_cast<uint8_t>(params_.progress_bar_color.value.r *
                                         remaining_width_f);
        uint8_t g = static_cast<uint8_t>(params_.progress_bar_color.value.g *
                                         remaining_width_f);
        uint8_t b = static_cast<uint8_t>(params_.progress_bar_color.value.b *
                                         remaining_width_f);
        offscreen_canvas->SetPixel(x, bar_y, r, g, b);
      } else {
        offscreen_canvas->SetPixel(x, bar_y, 0, 0, 0);
      }
    }

    display_covers(is_fading ? fade_progress : 1.0f);
    render_track_text(pending_track_data);
    offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);

    if (pending_track_data.is_playing) {
      pending_track_data.progress_ms +=
          static_cast<long>((time - last_animate_call) * 1000);
    }
  }

  last_animate_call = time;
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

} // namespace animations