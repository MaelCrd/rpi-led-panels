#include "Animations/SpotifyCurrent.h"
#include "../deps/stb_image.h"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

namespace animations {

using json = nlohmann::json;

// Minimal Base64 encoder
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

std::string base64_encode(const std::string &in) {
  std::string out;
  int val = 0, valb = -6;
  for (unsigned char c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(base64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4)
    out.push_back('=');
  return out;
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

std::string extract_json_field(const std::string &json_str,
                               const std::string &key) {
  try {
    auto j = json::parse(json_str);
    if (j.contains(key) && j[key].is_string()) {
      return j[key].get<std::string>();
    }
  } catch (...) {
  }
  return "";
}

std::string get_token_cache_path() {
  const char *env_path = std::getenv("SPOTIFY_USER_TOKEN_CACHE_PATH");
  if (env_path != nullptr)
    return std::string(env_path);
  const char *home = std::getenv("HOME");
  if (home == nullptr)
    return ".spotify_user_access_token";
  return std::string(home) + "/.spotify_user_access_token";
}

bool read_cached_token(const std::string &path, std::string &token) {
  std::ifstream in(path);
  if (!in)
    return false;
  std::getline(in, token);
  return !token.empty();
}

bool write_cached_token(const std::string &path, const std::string &token) {
  std::ofstream out(path, std::ofstream::trunc);
  if (!out)
    return false;
  out << token;
  return true;
}

bool refresh_user_access_token(const std::string &refresh_token,
                               const std::string &client_id,
                               const std::string &client_secret,
                               std::string &out_access_token,
                               std::string &out_new_refresh_token) {
  CURL *refresh_curl = curl_easy_init();
  if (!refresh_curl)
    return false;

  std::string credentials = client_id + ":" + client_secret;
  std::string auth = base64_encode(credentials);

  struct curl_slist *headers = nullptr;
  headers =
      curl_slist_append(headers, ("Authorization: Basic " + auth).c_str());
  headers = curl_slist_append(
      headers, "Content-Type: application/x-www-form-urlencoded");

  char *escaped_refresh =
      curl_easy_escape(refresh_curl, refresh_token.c_str(), 0);
  std::string post_fields = "grant_type=refresh_token&refresh_token=";
  if (escaped_refresh != nullptr)
    post_fields += escaped_refresh;

  curl_easy_setopt(refresh_curl, CURLOPT_URL,
                   "https://accounts.spotify.com/api/token");
  curl_easy_setopt(refresh_curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(refresh_curl, CURLOPT_POSTFIELDS, post_fields.c_str());

  std::string response;
  curl_easy_setopt(refresh_curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(refresh_curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(refresh_curl);
  if (escaped_refresh != nullptr)
    curl_free(escaped_refresh);
  curl_slist_free_all(headers);

  if (res != CURLE_OK) {
    std::cerr << "Refresh request failed: " << curl_easy_strerror(res) << "\n";
    curl_easy_cleanup(refresh_curl);
    return false;
  }

  out_access_token = extract_json_field(response, "access_token");
  out_new_refresh_token = extract_json_field(response, "refresh_token");

  curl_easy_cleanup(refresh_curl);
  return !out_access_token.empty();
}

SpotifyCurrent::~SpotifyCurrent() {
  stop_thread = true;
  if (fetch_thread.joinable()) {
    fetch_thread.join();
  }
  if (g_curl) {
    curl_easy_cleanup(g_curl);
    g_curl = nullptr;
  }
}

bool SpotifyCurrent::init_spotify() {
  if (!client_id || !client_secret) {
    std::cerr << "Client ID or Secret missing.\n";
    return false;
  }

  this->g_curl = curl_easy_init();
  if (!this->g_curl) {
    std::cerr << "Failed to initialize libcurl\n";
    return false;
  }
  curl_easy_setopt(this->g_curl, CURLOPT_WRITEFUNCTION, write_callback);

  const char *user_token_env = std::getenv("SPOTIFY_USER_ACCESS_TOKEN");

  std::string token_cache_path = get_token_cache_path();
  std::string cached_user_token;
  bool has_cached_token =
      read_cached_token(token_cache_path, cached_user_token);

  if (user_token_env && *user_token_env) {
    g_access_token = user_token_env;
  } else if (has_cached_token) {
    g_access_token = cached_user_token;
  }

  // Try to refresh if we have a refresh token
  if (!g_refresh_token_str.empty()) {
    std::string refreshed_token;
    std::string new_refresh_token;
    if (refresh_user_access_token(this->g_refresh_token_str, this->client_id,
                                  this->client_secret, refreshed_token,
                                  new_refresh_token)) {
      this->g_access_token = refreshed_token;
      write_cached_token(token_cache_path, refreshed_token);
      if (!new_refresh_token.empty() &&
          new_refresh_token != this->g_refresh_token_str) {
        std::cerr << "New refresh token received.\n";
      }
    }
  }

  // Start the fetch thread
  if (!fetch_thread.joinable()) {
    fetch_thread = std::thread(&SpotifyCurrent::fetch_thread_worker, this);
  }

  return !g_access_token.empty();
}

void SpotifyCurrent::fetch_thread_worker() {
  while (!stop_thread) {
    TrackData new_track_data;
    if (fetch_track_info(new_track_data)) {
      std::lock_guard<std::mutex> lock(track_data_mutex);
      // Only update if track ID changed
      if (new_track_data.id != pending_track_data.id) {
        pending_track_data = std::move(new_track_data);
        new_track_available = true;
      }
    }

    // Sleep for 2 seconds, but check stop_thread frequently
    for (int i = 0; i < 20 && !stop_thread; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

bool SpotifyCurrent::download_image(const std::string &url,
                                    std::vector<uint8_t> &out_data,
                                    int &out_width, int &out_height) {
  if (url.empty()) {
    return false;
  }

  CURL *curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  std::string image_data;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &image_data);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK || image_data.empty()) {
    return false;
  }

  // Decode the image using stb_image
  int width, height, channels;
  unsigned char *rgb_data = stbi_load_from_memory(
      reinterpret_cast<const unsigned char *>(image_data.data()),
      static_cast<int>(image_data.size()), &width, &height, &channels,
      3 // Force 3 channels (RGB)
  );

  if (!rgb_data) {
    return false;
  }

  // Store the RGB data
  size_t rgb_size = width * height * 3;
  out_data.assign(rgb_data, rgb_data + rgb_size);
  out_width = width;
  out_height = height;

  // Free stb_image memory
  stbi_image_free(rgb_data);

  return true;
}

bool SpotifyCurrent::fetch_track_info(TrackData &out_data) {
  if (!this->g_curl || this->g_access_token.empty())
    return false;

  static const std::string playing_url =
      "https://api.spotify.com/v1/me/player/currently-playing";
  std::string bearer = "Authorization: Bearer " + this->g_access_token;

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, bearer.c_str());
  curl_easy_setopt(this->g_curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(this->g_curl, CURLOPT_URL, playing_url.c_str());
  curl_easy_setopt(this->g_curl, CURLOPT_HTTPGET, 1L);

  std::string response;
  curl_easy_setopt(this->g_curl, CURLOPT_WRITEDATA, &response);
  CURLcode res = curl_easy_perform(this->g_curl);
  curl_slist_free_all(headers);

  if (res != CURLE_OK) {
    return false;
  }

  if (response.empty())
    return false;

  try {
    auto j = json::parse(response);
    if (!j.contains("item") || !j["item"].is_object())
      return false;

    auto item = j["item"];
    std::string current_id;
    if (item.contains("id") && item["id"].is_string()) {
      current_id = item["id"].get<std::string>();
    }

    // Check if track changed
    if (current_id == this->g_last_track_id && !current_id.empty()) {
      return false; // No change
    }

    out_data.id = current_id;

    if (item.contains("name") && item["name"].is_string()) {
      out_data.name = item["name"].get<std::string>();
    }

    std::string artists_str;
    if (item.contains("artists") && item["artists"].is_array()) {
      for (const auto &artist : item["artists"]) {
        if (artist.contains("name") && artist["name"].is_string()) {
          if (!artists_str.empty())
            artists_str += ", ";
          artists_str += artist["name"].get<std::string>();
        }
      }
    }
    out_data.artists = artists_str;

    std::string cover_url;
    if (item.contains("album") && item["album"].contains("images") &&
        item["album"]["images"].is_array()) {
      // Find 64x64 image
      for (const auto &img : item["album"]["images"]) {
        if (img.contains("height") && img["height"] == 64) {
          if (img.contains("url") && img["url"].is_string()) {
            cover_url = img["url"].get<std::string>();
            break;
          }
        }
      }
      // Fallback if 64x64 not found, take the last one (usually smallest)
      if (cover_url.empty() && !item["album"]["images"].empty()) {
        auto &last = item["album"]["images"].back();
        if (last.contains("url"))
          cover_url = last["url"].get<std::string>();
      }
    }

    if (!cover_url.empty()) {
      download_image(cover_url, out_data.cover_rgb_data, out_data.cover_width,
                     out_data.cover_height);
    }

    return true;

  } catch (...) {
    return false;
  }
}

void SpotifyCurrent::display_covers(float fade_progress) {
  if (prev_track_data.cover_rgb_data.empty() ||
      pending_track_data.cover_rgb_data.empty())
    return;

  int x_offset = 12;
  int y_offset = 12;

  for (int y = 0; y < prev_track_data.cover_height; ++y) {
    for (int x = 0; x < prev_track_data.cover_width; ++x) {
      int index = (y * prev_track_data.cover_width + x) * 3;
      uint8_t r =
          prev_track_data.cover_rgb_data[index] * (1.0 - fade_progress) +
          pending_track_data.cover_rgb_data[index] * fade_progress;
      uint8_t g =
          prev_track_data.cover_rgb_data[index + 1] * (1.0 - fade_progress) +
          pending_track_data.cover_rgb_data[index + 1] * fade_progress;
      uint8_t b =
          prev_track_data.cover_rgb_data[index + 2] * (1.0 - fade_progress) +
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

    // Measure the width of the test line
    int line_width = 0;
    for (char c : test_line) {
      line_width += font.CharacterWidth(c);
    }

    if (line_width <= max_width) {
      // The word fits on the current line
      current_line = test_line;
    } else {
      // The word doesn't fit, start a new line
      if (!current_line.empty()) {
        lines.push_back(current_line);
        if (lines.size() >= max_lines) {
          text_truncated = true;
          break;
        }
      }
      current_line = word;

      // Check if even a single word is too long
      int word_width = 0;
      for (char c : word) {
        word_width += font.CharacterWidth(c);
      }
      if (word_width > max_width) {
        // Word is too long, truncate it
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
        if (lines.size() >= max_lines) {
          text_truncated = true;
          break;
        }
        current_line.clear();
      }
    }
  }

  // Add the last line if it's not empty
  if (!current_line.empty() && lines.size() < max_lines) {
    lines.push_back(current_line);
  } else if (!current_line.empty() && lines.size() >= max_lines) {
    text_truncated = true;
  }

  // If text was truncated, add ellipsis to the last line
  if (text_truncated && !lines.empty()) {
    std::string &last_line = lines.back();

    // Remove characters from the end until "..." fits
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

    // Trim any trailing spaces before adding ellipsis
    while (!last_line.empty() && last_line.back() == ' ') {
      last_line.pop_back();
    }

    last_line += "...";
  }

  return lines;
}

void SpotifyCurrent::render_track_text(const TrackData &track_data) {
  // Calculate positions
  const int cover_x = 12;
  const int cover_y = 12;
  const int cover_size = 64;
  const int cover_center_y = cover_y + cover_size / 2;
  const int text_x = cover_x + cover_size + 10;
  const int max_text_width = matrix->width() - text_x;

  // Clear the text area
  offscreen_canvas->SubFill(text_x, cover_y, matrix->width() - text_x,
                            cover_size, 0, 0, 0);

  // Wrap text to get the lines
  auto title_lines = wrap_text(track_data.name, title_font, max_text_width, 3);
  auto artists_lines =
      wrap_text(track_data.artists, artists_font, max_text_width, 3);

  // Calculate total height of text block (with 6px spacing between title and
  // artists)
  int spacing = 6;
  int title_height = title_lines.size() * title_font.height();
  int artists_height = artists_lines.size() * artists_font.height();
  int total_text_height = title_height + spacing + artists_height;

  // Calculate starting Y position to center text vertically with the cover
  int text_start_y = cover_center_y - (total_text_height / 2);

  // Draw track name
  int title_y = text_start_y + title_font.baseline();
  for (const auto &line : title_lines) {
    rgb_matrix::DrawText(offscreen_canvas, title_font, text_x, title_y,
                         title_color, nullptr, line.c_str());
    title_y += title_font.height();
  }

  // Draw artists (with spacing after title)
  int artists_y =
      text_start_y + title_height + spacing + artists_font.baseline();
  for (const auto &line : artists_lines) {
    rgb_matrix::DrawText(offscreen_canvas, artists_font, text_x, artists_y,
                         artists_color, nullptr, line.c_str());
    artists_y += artists_font.height();
  }
}

void SpotifyCurrent::animate(double time) {
  if (!initialized) {
    if (init_spotify()) {
      initialized = true;
      std::cout << "SpotifyCurrent initialized successfully.\n";
    } else {
      std::cerr << "SpotifyCurrent initialization failed.\n";
    }
  }

  // Handle crossfade animation
  if (is_fading && fade_progress < 1.0) {
    display_covers(fade_progress);
    matrix->SwapOnVSync(offscreen_canvas);
    fade_progress += (time - last_animate_time) *
                     0.5; // Adjust fade speed here (0.5 = 2 second fade)
    last_animate_time = time;
  } else {
    if (is_fading) {
      fade_progress = 1.0;
      display_covers(fade_progress);
      matrix->SwapOnVSync(offscreen_canvas);
      is_fading = false;
      prev_track_data = pending_track_data;
    }
    last_animate_time = time;
  }

  // Check if new track is available from the fetch thread
  if (new_track_available.exchange(false)) {
    TrackData new_track;
    {
      std::lock_guard<std::mutex> lock(track_data_mutex);
      new_track = pending_track_data;
    }

    // Cover is 64x64 at position (12, 12), so it spans from y=12 to y=76
    const int cover_x = 12;
    const int cover_y = 12;
    const int cover_size = 64;
    const int cover_center_y = cover_y + cover_size / 2; // y = 44

    const int text_x = cover_x + cover_size + 10; // x = 86
    const int max_text_width =
        matrix->width() - text_x - 5; // Leave 5px margin on the right

    if (prev_track_data.id != new_track.id && !is_fading) {
      /*std::cout << "Now playing: " << new_track.name << " by "
                << new_track.artists << "\n";*/

      // If we don't have a previous cover (first track), display the cover
      // immediately instead of trying to crossfade from an empty image.
      if (prev_track_data.cover_rgb_data.empty()) {
        prev_track_data = new_track;
        pending_track_data = new_track;
        fade_progress = 1.0;
        is_fading = false;

        // Draw the cover immediately
        offscreen_canvas->Clear();
        display_covers(1.0f);

        // Render track text
        render_track_text(pending_track_data);

        matrix->SwapOnVSync(offscreen_canvas);
      } else {
        // Start crossfade
        pending_track_data = new_track;
        fade_progress = 0.0;
        is_fading = true;

        // Render track text
        render_track_text(pending_track_data);
      }
    }
  }

  // std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

} // namespace animations