#include "Animations/SpotifyCurrent/SpotifyClient.h"
#include "stb_image.h"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unistd.h>

namespace animations {

using json = nlohmann::json;

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

static std::string base64_encode(const std::string &in) {
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

static size_t write_callback(void *contents, size_t size, size_t nmemb,
                             void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

static std::string extract_json_field(const std::string &json_str,
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

SpotifyClient::SpotifyClient(const std::string &client_id,
                             const std::string &client_secret,
                             const std::string &refresh_token)
    : client_id_(client_id), client_secret_(client_secret),
      refresh_token_(refresh_token), curl_(nullptr) {}

SpotifyClient::~SpotifyClient() {
  if (curl_) {
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
  }
}

std::string SpotifyClient::get_token_cache_path() const {
  const char *env_path = std::getenv("SPOTIFY_USER_TOKEN_CACHE_PATH");
  if (env_path != nullptr)
    return std::string(env_path);
  const char *home = std::getenv("HOME");
  if (home == nullptr)
    return ".spotify_user_access_token";
  return std::string(home) + "/.spotify_user_access_token";
}

bool SpotifyClient::read_cached_token(const std::string &path,
                                      std::string &token) const {
  std::ifstream in(path);
  if (!in)
    return false;
  std::getline(in, token);
  return !token.empty();
}

bool SpotifyClient::write_cached_token(const std::string &path,
                                       const std::string &token) const {
  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0)
    return false;
  ssize_t written = write(fd, token.data(), token.size());
  close(fd);
  return written == static_cast<ssize_t>(token.size());
}

bool SpotifyClient::refresh_user_access_token(
    std::string &out_access_token, std::string &out_new_refresh_token) {
  CURL *refresh_curl = curl_easy_init();
  if (!refresh_curl)
    return false;

  std::string credentials = client_id_ + ":" + client_secret_;
  std::string auth = base64_encode(credentials);

  struct curl_slist *headers = nullptr;
  headers =
      curl_slist_append(headers, ("Authorization: Basic " + auth).c_str());
  headers = curl_slist_append(
      headers, "Content-Type: application/x-www-form-urlencoded");

  char *escaped_refresh =
      curl_easy_escape(refresh_curl, refresh_token_.c_str(), 0);
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

bool SpotifyClient::init() {
  if (client_id_.empty() || client_secret_.empty()) {
    std::cerr << "Client ID or Secret missing.\n";
    return false;
  }

  curl_ = curl_easy_init();
  if (!curl_) {
    std::cerr << "Failed to initialize libcurl\n";
    return false;
  }
  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);

  const char *user_token_env = std::getenv("SPOTIFY_USER_ACCESS_TOKEN");

  std::string token_cache_path = get_token_cache_path();
  std::string cached_user_token;
  bool has_cached_token =
      read_cached_token(token_cache_path, cached_user_token);

  if (user_token_env && *user_token_env) {
    access_token_ = user_token_env;
  } else if (has_cached_token) {
    access_token_ = cached_user_token;
  }

  if (!refresh_token_.empty()) {
    std::string refreshed_token;
    std::string new_refresh_token;
    if (refresh_user_access_token(refreshed_token, new_refresh_token)) {
      access_token_ = refreshed_token;
      write_cached_token(token_cache_path, refreshed_token);
      if (!new_refresh_token.empty() && new_refresh_token != refresh_token_) {
        refresh_token_ = new_refresh_token;
      }
    }
  }

  return !access_token_.empty();
}

bool SpotifyClient::download_image(const std::string &url,
                                   std::vector<uint8_t> &out_data,
                                   int &out_width, int &out_height) {
  if (url.empty())
    return false;

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::string image_data;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &image_data);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK || image_data.empty()) {
    std::cerr << "download_image: curl failed or empty image data, res="
              << curl_easy_strerror(res) << " size=" << image_data.size()
              << "\n";
    return false;
  }

  int width, height, channels;
  unsigned char *rgb_data = stbi_load_from_memory(
      reinterpret_cast<const unsigned char *>(image_data.data()),
      static_cast<int>(image_data.size()), &width, &height, &channels, 3);

  if (!rgb_data) {
    std::cerr << "download_image: stbi_load_from_memory failed\n";
    return false;
  }

  size_t rgb_size = width * height * 3;
  out_data.assign(rgb_data, rgb_data + rgb_size);
  out_width = width;
  out_height = height;

  stbi_image_free(rgb_data);
  return true;
}

bool SpotifyClient::fetch_track_info(TrackData &out_data,
                                     const std::string &last_track_id) {
  if (!curl_ || access_token_.empty())
    return false;

  static const std::string playing_url =
      "https://api.spotify.com/v1/me/player/currently-playing";
  std::string bearer = "Authorization: Bearer " + access_token_;

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, bearer.c_str());
  curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl_, CURLOPT_URL, playing_url.c_str());
  curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);

  std::string response;
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
  CURLcode res = curl_easy_perform(curl_);
  long http_code = 0;
  curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
  curl_slist_free_all(headers);

  if (res != CURLE_OK) {
    std::cerr << "fetch_track_info: curl perform failed: "
              << curl_easy_strerror(res) << "\n";
    return false;
  }

  if (response.empty()) {
    std::cerr << "fetch_track_info: empty response (" << http_code << ")\n";
    return false;
  }

  try {
    auto j = json::parse(response);
    if (!j.contains("item") || !j["item"].is_object())
      return false;

    auto item = j["item"];
    std::string current_id;
    if (item.contains("id") && item["id"].is_string()) {
      current_id = item["id"].get<std::string>();
    }
    out_data.id = current_id;

    if (j.contains("progress_ms") && j["progress_ms"].is_number_integer()) {
      out_data.progress_ms = j["progress_ms"].get<long>();
    }
    if (j.contains("is_playing") && j["is_playing"].is_boolean()) {
      out_data.is_playing = j["is_playing"].get<bool>();
    }
    if (item.contains("duration_ms") &&
        item["duration_ms"].is_number_integer()) {
      out_data.duration_ms = item["duration_ms"].get<long>();
    }

    if (current_id == last_track_id && !current_id.empty()) {
      return true;
    }

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
      for (const auto &img : item["album"]["images"]) {
        if (img.contains("height") && img["height"] == 64) {
          if (img.contains("url") && img["url"].is_string()) {
            cover_url = img["url"].get<std::string>();
            break;
          }
        }
      }
      if (cover_url.empty() && !item["album"]["images"].empty()) {
        auto &last = item["album"]["images"].back();
        if (last.contains("url"))
          cover_url = last["url"].get<std::string>();
      }
    }

    if (!cover_url.empty()) {
      bool img_ok = download_image(cover_url, out_data.cover_rgb_data,
                                   out_data.cover_width, out_data.cover_height);
      if (!img_ok) {
        std::cerr << "fetch_track_info: download_image failed for url="
                  << cover_url << "\n";
      }
    }

    return true;

  } catch (const std::exception &e) {
    std::cerr << "fetch_track_info: json parse/processing exception: "
              << e.what() << "\n";
    return false;
  } catch (...) {
    std::cerr << "fetch_track_info: unknown exception while parsing\n";
    return false;
  }
}

} // namespace animations
