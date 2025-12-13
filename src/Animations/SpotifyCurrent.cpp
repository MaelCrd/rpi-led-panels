#include "Animations/SpotifyCurrent.h"
#include "../deps/stb_image.h"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>

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

    if (prev_track_data.id != new_track.id && !is_fading) {
      /*std::cout << "Now playing: " << new_track.name << " by "
                << new_track.artists << "\n";*/

      // Start crossfade
      pending_track_data = new_track;
      fade_progress = 0.0;
      is_fading = true;

      int x_offset = 12 + 64 + 10;
      int y_offset = 12 + 20;

      // Clear the text area
      offscreen_canvas->SubFill(x_offset, y_offset, matrix->width() - x_offset,
                                matrix->height() / 2, 0, 0, 0);

      // Draw track name and artists
      rgb_matrix::DrawText(offscreen_canvas, title_font, x_offset,
                           y_offset + 10, title_color, nullptr,
                           pending_track_data.name.c_str());
      rgb_matrix::DrawText(offscreen_canvas, artists_font, x_offset,
                           y_offset + 30, artists_color, nullptr,
                           pending_track_data.artists.c_str());
      // offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
    }
  }

  // std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

} // namespace animations