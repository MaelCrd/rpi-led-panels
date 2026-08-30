#include "Animations/SpotifyCurrent.h"
#include "../deps/stb_image.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace animations {

using json = nlohmann::json;

// Child process stop flag (set by signal handler in the child)
static volatile sig_atomic_t child_stop = 0;

static void child_sigterm_handler(int) { child_stop = 1; }

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
  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0)
    return false;
  ssize_t written = write(fd, token.data(), token.size());
  close(fd);
  return written == static_cast<ssize_t>(token.size());
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
  if (fetch_pid > 0) {
    // Ask the child process to terminate
    kill(fetch_pid, SIGTERM);
    int status = 0;
    waitpid(fetch_pid, &status, 0);
    fetch_pid = -1;
  }
  if (g_curl) {
    curl_easy_cleanup(g_curl);
    g_curl = nullptr;
  }
}

bool SpotifyCurrent::init_spotify() {
  if (client_id.empty() || client_secret.empty()) {
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
  if (refresh_token.empty()) {
    std::string refreshed_token;
    std::string new_refresh_token;
    if (refresh_user_access_token(this->refresh_token, this->client_id,
                                  this->client_secret, refreshed_token,
                                  new_refresh_token)) {
      this->g_access_token = refreshed_token;
      write_cached_token(token_cache_path, refreshed_token);
      if (!new_refresh_token.empty() &&
          new_refresh_token != this->refresh_token) {
      }
    }
  }

  // Start the fetch thread
  if (fetch_pid <= 0) {
    // Create an IPC pipe for child -> parent updates
    int pipefd[2];
    if (pipe(pipefd) == 0) {
      this->ipc_pipe_read_fd = pipefd[0];
      this->ipc_pipe_write_fd = pipefd[1];
    } else {
      std::cerr << "init_spotify: pipe() failed: " << errno << "\n";
      this->ipc_pipe_read_fd = -1;
      this->ipc_pipe_write_fd = -1;
    }
    pid_t pid = fork();
    if (pid < 0) {
      std::cerr << "Failed to fork spotify fetch process\n";
    } else if (pid == 0) {
      // Child process: install signal handler and run worker
      signal(SIGTERM, child_sigterm_handler);
      // Reinitialize curl in the child process to avoid using parent's CURL
      if (this->g_curl) {
        curl_easy_cleanup(this->g_curl);
        this->g_curl = nullptr;
      }
      this->g_curl = curl_easy_init();
      if (this->g_curl)
        curl_easy_setopt(this->g_curl, CURLOPT_WRITEFUNCTION, write_callback);
      else
        std::cerr << "Spotify child: curl_easy_init() failed\n";
      // Lower the child's scheduling priority to reduce CPU impact
      if (setpriority(PRIO_PROCESS, 0, 10) != 0) {
        std::cerr << "Spotify child: setpriority failed errno=" << errno
                  << "\n";
      } else {
      }

      // Try to create a cgroup v2 for this child and limit CPU to ~1%.
      // cpu.max format: "max period_us" - use period 100000 (100ms) and max
      // 1000 (1%).
      const char *cgroup_root = "/sys/fs/cgroup";
      struct stat st;
      if (stat(cgroup_root, &st) == 0 && S_ISDIR(st.st_mode)) {
        std::string cg_parent = std::string(cgroup_root) + "/rpi_led_panels";
        // Create parent dir if needed
        if (mkdir(cg_parent.c_str(), 0755) != 0 && errno != EEXIST) {
          std::cerr << "Spotify child: mkdir parent cgroup failed errno="
                    << errno << "\n";
        }
        std::string cg_dir = cg_parent + "/spotify_" + std::to_string(getpid());
        if (mkdir(cg_dir.c_str(), 0755) != 0 && errno != EEXIST) {
          std::cerr << "Spotify child: mkdir cgroup failed errno=" << errno
                    << "\n";
        } else {
          std::string cpu_max_path = cg_dir + "/cpu.max";
          int fd = open(cpu_max_path.c_str(), O_WRONLY | O_CLOEXEC);
          if (fd >= 0) {
            const char *val = "8000 100000"; // ~8% (8000/100000)
            ssize_t w = write(fd, val, strlen(val));
            if (w < 0) {
              std::cerr << "Spotify child: write cpu.max failed errno=" << errno
                        << "\n";
            } else {
              (void)val;
            }
            close(fd);
          } else {
            std::cerr << "Spotify child: open cpu.max failed errno=" << errno
                      << "\n";
            // If cpu.max doesn't exist (ENOENT), attempt to enable the cpu
            // controller in the parent cgroup by writing "+cpu\n" to
            // cgroup.subtree_control.
            if (errno == ENOENT) {
              std::string parent_subtree =
                  cg_parent + "/cgroup.subtree_control";
              int sfd = open(parent_subtree.c_str(), O_WRONLY | O_CLOEXEC);
              if (sfd >= 0) {
                const char *enable = "+cpu\n";
                if (write(sfd, enable, strlen(enable)) < 0) {
                  std::cerr << "Spotify child: write to " << parent_subtree
                            << " failed errno=" << errno << "\n";
                } else {
                  (void)parent_subtree;
                }
                close(sfd);
                // Try opening cpu.max again
                struct stat statbuf;
                if (stat(cpu_max_path.c_str(), &statbuf) == 0) {
                  if (S_ISDIR(statbuf.st_mode)) {
                    std::cerr << "Spotify child: cpu.max exists but is a "
                                 "directory (EISDIR).\n";
                    std::cerr
                        << "Please remove the mistaken directory and ensure "
                           "cgroup v2 subtree control is enabled (echo +cpu > "
                           "<parent>/cgroup.subtree_control).\n";
                  } else {
                    int fd = open(cpu_max_path.c_str(), O_WRONLY | O_CLOEXEC);
                    if (fd >= 0) {
                      const char *val = "1000 100000"; // ~1% (1000/100000)
                      ssize_t w = write(fd, val, strlen(val));
                      if (w < 0) {
                        std::cerr << "Spotify child: write cpu.max failed: "
                                  << strerror(errno) << "\n";
                      } else {
                        (void)val;
                      }
                      close(fd);
                    } else {
                      std::cerr << "Spotify child: open cpu.max failed: "
                                << strerror(errno) << "\n";
                    }
                  }
                } else {
                  // stat failed - path probably doesn't exist
                  std::cerr << "Spotify child: stat(cpu.max) failed: "
                            << strerror(errno) << "\n";
                }
              } else {
                // Try enabling at the root cgroup level as a last resort
                std::string root_subtree =
                    std::string(cgroup_root) + "/cgroup.subtree_control";
                int rsfd = open(root_subtree.c_str(), O_WRONLY | O_CLOEXEC);
                if (rsfd >= 0) {
                  const char *enable = "+cpu\n";
                  if (write(rsfd, enable, strlen(enable)) < 0) {
                    std::cerr << "Spotify child: write to " << root_subtree
                              << " failed errno=" << errno << "\n";
                  } else {
                    (void)root_subtree;
                  }
                  close(rsfd);
                  // Try opening cpu.max again
                  fd = open(cpu_max_path.c_str(), O_WRONLY | O_CLOEXEC);
                  if (fd >= 0) {
                    const char *val = "1000 100000";
                    ssize_t w = write(fd, val, strlen(val));
                    if (w < 0) {
                      std::cerr << "Spotify child: write cpu.max failed after "
                                   "enabling root controller errno="
                                << errno << "\n";
                    } else {
                      (void)val;
                    }
                    close(fd);
                  } else {
                    std::cerr
                        << "Spotify child: open cpu.max still failed errno="
                        << errno << "\n";
                  }
                } else {
                  std::cerr << "Spotify child: open parent subtree_control "
                               "failed errno="
                            << errno << "\n";
                }
              }
            }
          }

          // Add this process to the cgroup
          std::string procs_path = cg_dir + "/cgroup.procs";
          int pfd = open(procs_path.c_str(), O_WRONLY | O_CLOEXEC);
          if (pfd >= 0) {
            std::string pidstr = std::to_string(getpid());
            if (write(pfd, pidstr.c_str(), pidstr.size()) < 0) {
              std::cerr << "Spotify child: write cgroup.procs failed errno="
                        << errno << "\n";
            } else {
              (void)cg_dir;
            }
            close(pfd);
          } else {
            std::cerr << "Spotify child: open cgroup.procs failed errno="
                      << errno << "\n";
          }
        }
      } else {
        std::cerr << "Spotify child: cgroup root not present or inaccessible\n";
      }
      // In child: close read end of pipe (we only write)
      if (this->ipc_pipe_read_fd >= 0) {
        close(this->ipc_pipe_read_fd);
        this->ipc_pipe_read_fd = -1;
      }
      // Run the same worker function; when it returns, exit the child
      this->fetch_thread_worker();
      _exit(0);
    } else {
      // Parent stores child pid
      fetch_pid = pid;
      // In parent: close write end of pipe (we only read)
      if (this->ipc_pipe_write_fd >= 0) {
        close(this->ipc_pipe_write_fd);
        this->ipc_pipe_write_fd = -1;
      }
      // Set read fd non-blocking so animate() can poll
      if (this->ipc_pipe_read_fd >= 0) {
        int flags = fcntl(this->ipc_pipe_read_fd, F_GETFL, 0);
        fcntl(this->ipc_pipe_read_fd, F_SETFL, flags | O_NONBLOCK);
      }
      (void)fetch_pid;
    }
  }

  return !g_access_token.empty();
}

void SpotifyCurrent::fetch_thread_worker() {
  int loop_count = 0;
  while (!stop_thread && !child_stop) {
    ++loop_count;
    (void)loop_count;
    (void)getpid();
    TrackData new_track_data;
    if (fetch_track_info(new_track_data)) {
      (void)new_track_data;

      // If we have an IPC pipe (child), send the update to the parent instead
      // of modifying the local memory which isn't shared across fork.
      if (this->ipc_pipe_write_fd >= 0) {
        // Serialize metadata as JSON
        try {
          json meta;
          meta["id"] = new_track_data.id;
          meta["name"] = new_track_data.name;
          meta["artists"] = new_track_data.artists;
          meta["progress_ms"] = new_track_data.progress_ms;
          meta["duration_ms"] = new_track_data.duration_ms;
          meta["cover_width"] = new_track_data.cover_width;
          meta["cover_height"] = new_track_data.cover_height;
          std::string meta_str = meta.dump();

          uint32_t meta_len = htonl(static_cast<uint32_t>(meta_str.size()));
          uint32_t img_len = htonl(
              static_cast<uint32_t>(new_track_data.cover_rgb_data.size()));

          std::vector<uint8_t> buf;
          buf.resize(4 + meta_str.size() + 4 +
                     new_track_data.cover_rgb_data.size());
          uint8_t *p = buf.data();
          memcpy(p, &meta_len, 4);
          p += 4;
          memcpy(p, meta_str.data(), meta_str.size());
          p += meta_str.size();
          memcpy(p, &img_len, 4);
          p += 4;
          if (!new_track_data.cover_rgb_data.empty())
            memcpy(p, new_track_data.cover_rgb_data.data(),
                   new_track_data.cover_rgb_data.size());

          // Single atomic write (message should be < PIPE_BUF)
          ssize_t wrote =
              write(this->ipc_pipe_write_fd, buf.data(), buf.size());
          if (wrote < 0) {
            std::cerr << "fetch_thread_worker: write to ipc pipe failed: "
                      << errno << "\n";
          } else {
            // Remember the last track id in the child process so we don't
            // repeatedly re-download the same cover on subsequent polls.
            // g_last_track_id is process-local (child has its own copy after
            // fork), so setting it here prevents fetch_track_info() from
            // downloading the cover again until the track actually changes.
            this->g_last_track_id = new_track_data.id;
          }
        } catch (const std::exception &e) {
          std::cerr << "fetch_thread_worker: ipc serialization error: "
                    << e.what() << "\n";
        }
      } else {
        // No IPC pipe (running as thread or fallback) - update local pending
        // data
        std::lock_guard<std::mutex> lock(track_data_mutex);
        if (new_track_data.id != pending_track_data.id) {
          pending_track_data = std::move(new_track_data);
          new_track_available = true;
          this->g_last_track_id = pending_track_data.id;
        } else {
          pending_track_data.progress_ms = new_track_data.progress_ms;
          pending_track_data.duration_ms = new_track_data.duration_ms;
        }
      }
    } else {
      std::cerr << "fetch_thread_worker: fetch_track_info() => false (no "
                   "response or error)\n";
    }

    // Sleep between polls. Use a single longer sleep to minimize wakeups and
    // CPU. SIGTERM will interrupt nanosleep so child_stop will be observed
    // quickly.
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}

bool SpotifyCurrent::download_image(const std::string &url,
                                    std::vector<uint8_t> &out_data,
                                    int &out_width, int &out_height) {
  if (url.empty()) {
    return false;
  }

  (void)url;

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
    std::cerr << "download_image: curl failed or empty image data, res="
              << curl_easy_strerror(res) << " size=" << image_data.size()
              << "\n";
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
    std::cerr << "download_image: stbi_load_from_memory failed\n";
    return false;
  }

  // Store the RGB data
  size_t rgb_size = width * height * 3;
  out_data.assign(rgb_data, rgb_data + rgb_size);
  out_width = width;
  out_height = height;

  // Free stb_image memory
  stbi_image_free(rgb_data);

  (void)out_width;
  (void)out_height;

  return true;
}

// Try to read a single update message from the IPC pipe (non-blocking).
// Returns true if a message was read and applied to pending_track_data.
bool SpotifyCurrent::read_ipc_update() {
  if (this->ipc_pipe_read_fd < 0)
    return false;

  // Use select with zero timeout to check for data
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(this->ipc_pipe_read_fd, &rfds);
  struct timeval tv = {0, 0};
  int sel = select(this->ipc_pipe_read_fd + 1, &rfds, nullptr, nullptr, &tv);
  if (sel <= 0) // no data or error
    return false;

  // Read 4-byte meta length
  uint32_t meta_len_n = 0;
  ssize_t r = read(this->ipc_pipe_read_fd, &meta_len_n, 4);
  if (r <= 0)
    return false;
  uint32_t meta_len = ntohl(meta_len_n);

  std::string meta_str;
  meta_str.resize(meta_len);
  ssize_t got = 0;
  while (got < (ssize_t)meta_len) {
    ssize_t n = read(this->ipc_pipe_read_fd, &meta_str[got], meta_len - got);
    if (n <= 0)
      return false;
    got += n;
  }

  uint32_t img_len_n = 0;
  r = read(this->ipc_pipe_read_fd, &img_len_n, 4);
  if (r <= 0)
    return false;
  uint32_t img_len = ntohl(img_len_n);

  std::vector<uint8_t> img;
  img.resize(img_len);
  got = 0;
  while (got < (ssize_t)img_len) {
    ssize_t n = read(this->ipc_pipe_read_fd, img.data() + got, img_len - got);
    if (n <= 0)
      return false;
    got += n;
  }

  try {
    auto meta = json::parse(meta_str);
    TrackData td;
    if (meta.contains("id"))
      td.id = meta["id"].get<std::string>();
    if (meta.contains("name"))
      td.name = meta["name"].get<std::string>();
    if (meta.contains("artists"))
      td.artists = meta["artists"].get<std::string>();
    if (meta.contains("progress_ms"))
      td.progress_ms = meta["progress_ms"].get<long>();
    if (meta.contains("duration_ms"))
      td.duration_ms = meta["duration_ms"].get<long>();
    if (meta.contains("cover_width"))
      td.cover_width = meta["cover_width"].get<int>();
    if (meta.contains("cover_height"))
      td.cover_height = meta["cover_height"].get<int>();
    td.cover_rgb_data = std::move(img);

    {
      std::lock_guard<std::mutex> lock(track_data_mutex);
      // If this update contains image data (or non-zero cover size) or the
      // track id differs from our last seen id, treat it as a full update
      // and replace pending data so animate() can react (crossfade, cover
      // changes, etc.).
      if (!td.cover_rgb_data.empty() || td.cover_width > 0 ||
          td.cover_height > 0 || td.id != this->g_last_track_id) {
        pending_track_data = std::move(td);
        new_track_available = true;
        this->g_last_track_id = pending_track_data.id;
      } else {
        // This is a progress-only update for the same track. Update only
        // the progress/duration so we don't overwrite cover data or
        // trigger crossfade behavior.
        pending_track_data.progress_ms = td.progress_ms;
        pending_track_data.duration_ms = td.duration_ms;
        // Do not set new_track_available; animate() will read the updated
        // pending_track_data when drawing the progress bar.
        (void)td;
      }
    }
    return true;
  } catch (...) {
    std::cerr << "read_ipc_update: parse error\n";
    return false;
  }
}

bool SpotifyCurrent::fetch_track_info(TrackData &out_data) {
  if (!this->g_curl || this->g_access_token.empty()) {
    return false;
  }

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
  long http_code = 0;
  curl_easy_getinfo(this->g_curl, CURLINFO_RESPONSE_CODE, &http_code);
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
    if (item.contains("duration_ms") &&
        item["duration_ms"].is_number_integer()) {
      out_data.duration_ms = item["duration_ms"].get<long>();
    }

    // Check if track changed
    if (current_id == this->g_last_track_id && !current_id.empty()) {
      return true; // No need to update other fields
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

void SpotifyCurrent::display_covers(float fade_progress) {
  if (prev_track_data.cover_rgb_data.empty() ||
      pending_track_data.cover_rgb_data.empty()) {
    return;
  }

  int x_offset = 12;
  int y_offset = 32;
  fade_progress =
      (fade_progress * fade_progress * (3 - 2 * fade_progress)); // Smoothstep
  float inv_progress = 1.0f - fade_progress;
  // 1 - Normal crossfade
  if (true) {
    // float inv_progress = 1.0f - fade_progress;
    // Slow start and end to the fade
    for (int y = 0; y < prev_track_data.cover_height; ++y) {
      for (int x = 0; x < prev_track_data.cover_width; ++x) {
        int index = (y * prev_track_data.cover_width + x) * 3;
        uint8_t r = prev_track_data.cover_rgb_data[index] * inv_progress +
                    pending_track_data.cover_rgb_data[index] * fade_progress;
        uint8_t g =
            prev_track_data.cover_rgb_data[index + 1] * inv_progress +
            pending_track_data.cover_rgb_data[index + 1] * fade_progress;
        uint8_t b =
            prev_track_data.cover_rgb_data[index + 2] * inv_progress +
            pending_track_data.cover_rgb_data[index + 2] * fade_progress;
        offscreen_canvas->SetPixel(x + x_offset, y + y_offset, r, g, b);
      }
    }
  }

  // 2 - Crossfade with color overflow :
  // for a single pixel, starts from color1, then goes to color2final
  // (color2final = color1 + (255 - color1) + color2)
  else {
    // float fade_progress_r = pow(sin(fade_progress * M_PI * 3 / 2), 2);
    // float fade_progress_g = pow(sin(fade_progress * M_PI * 5 / 2), 2);
    // float fade_progress_b = pow(sin(fade_progress * M_PI * 7 / 2), 2);
    float fade_progress_r = fade_progress;
    float fade_progress_g = fade_progress;
    float fade_progress_b = fade_progress;
    for (int y = 0; y < prev_track_data.cover_height; ++y) {
      for (int x = 0; x < prev_track_data.cover_width; ++x) {
        int index = (y * prev_track_data.cover_width + x) * 3;
        uint8_t r = prev_track_data.cover_rgb_data[index] +
                    (256 - prev_track_data.cover_rgb_data[index] +
                     pending_track_data.cover_rgb_data[index]) *
                        fade_progress_r;
        uint8_t g = prev_track_data.cover_rgb_data[index + 1] +
                    (256 - prev_track_data.cover_rgb_data[index + 1] +
                     pending_track_data.cover_rgb_data[index + 1]) *
                        fade_progress_g;
        uint8_t b = prev_track_data.cover_rgb_data[index + 2] +
                    (256 - prev_track_data.cover_rgb_data[index + 2] +
                     pending_track_data.cover_rgb_data[index + 2]) *
                        fade_progress_b;
        offscreen_canvas->SetPixel(x + x_offset, y + y_offset, r, g, b);
      }
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
  const int cover_y = 32;
  const int cover_size = 64;
  const int cover_center_y = cover_y + cover_size / 2;
  const int text_x = cover_x + cover_size + 10;
  const int max_text_width = matrix->width() - 12 - text_x;

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
  int line_spacing = 0; // Reduced spacing between lines within title/artists
  int title_height =
      title_lines.size() * title_font.height() -
      (title_lines.size() > 1 ? (title_lines.size() - 1) * line_spacing : 0);
  int artists_height =
      artists_lines.size() * artists_font.height() -
      (artists_lines.size() > 1 ? (artists_lines.size() - 1) * line_spacing
                                : 0);
  int total_text_height = title_height + spacing + artists_height;

  // Calculate starting Y position to center text vertically with the cover
  int text_start_y = cover_center_y - (total_text_height / 2);

  // Draw track name
  int title_y = text_start_y + title_font.baseline();
  for (const auto &line : title_lines) {
    // Convert parameter color to rgb_matrix::Color
    rgb_matrix::Color draw_title_color(params_.title_color.value.r,
                                       params_.title_color.value.g,
                                       params_.title_color.value.b);
    rgb_matrix::DrawText(offscreen_canvas, title_font, text_x, title_y,
                         draw_title_color, nullptr, line.c_str());
    title_y += title_font.height() - line_spacing;
  }

  // Draw artists (with spacing after title)
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
  // Try to drain any IPC updates from the child process before rendering
  if (this->ipc_pipe_read_fd >= 0) {
    // Read as many updates as available
    while (read_ipc_update()) {
      // continue draining
    }
  }
  if (last_animate_call <= 0) {
    last_animate_call = time;
  }

  if (!initialized) {
    if (init_spotify()) {
      initialized = true;
    } else {
      std::cerr << "SpotifyCurrent initialization failed.\n";
    }
  }

  // Handle crossfade animation
  if (is_fading && fade_progress < 1.0) {
    // display_covers(fade_progress);
    // matrix->SwapOnVSync(offscreen_canvas);
    fade_progress +=
        (time - last_animate_time) /
        static_cast<double>(params_.cover_fade_duration
                                .value); // Adjust fade speed via parameter
    last_animate_time = time;
  } else {
    if (is_fading) {
      fade_progress = 1.0;
      // display_covers(fade_progress);
      // matrix->SwapOnVSync(offscreen_canvas);
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

      // If we don't have a previous cover (first track), display the cover
      // immediately instead of trying to crossfade from an empty image.
      if (prev_track_data.cover_rgb_data.empty()) {
        prev_track_data = new_track;
        pending_track_data = new_track;
        fade_progress = 1.0;
        is_fading = false;

        // Draw the cover immediately
        // offscreen_canvas->Clear();
        // display_covers(1.0f);

        // Render track text
        // render_track_text(pending_track_data);

        // matrix->SwapOnVSync(offscreen_canvas);
      } else {
        // Start crossfade
        pending_track_data = new_track;
        fade_progress = 0.0;
        is_fading = true;

        // Render track text
        // render_track_text(pending_track_data);
      }
    }
  }

  // Show progress bar and update it
  int bar_y = matrix->height() - 1 - 1; // 1px from bottom
  int bar_height = 1;
  int bar_width = matrix->width();
  if (pending_track_data.duration_ms > 0) {
    float progress_ratio = static_cast<float>(pending_track_data.progress_ms) /
                           static_cast<float>(pending_track_data.duration_ms);
    progress_ratio = std::clamp(progress_ratio, 0.0f, 1.0f);
    float progress_diff = progress_ratio - displayed_progress_ratio;
    displayed_progress_ratio +=
        progress_diff * 4.0f *
        (time - last_animate_call); // Smoothly interpolate displayed progress

    displayed_progress_ratio = std::clamp(displayed_progress_ratio, 0.0f, 1.0f);
    float filled_width_f = bar_width * displayed_progress_ratio;
    int filled_width = static_cast<int>(bar_width * displayed_progress_ratio);
    float remaining_width_f = filled_width_f - filled_width;

    offscreen_canvas->Clear();

    // Draw the progress bar
    for (int x = 0; x < bar_width; ++x) {
      if (x < filled_width) {
        offscreen_canvas->SetPixel(x, bar_y, params_.progress_bar_color.value.r,
                                   params_.progress_bar_color.value.g,
                                   params_.progress_bar_color.value.b);
      } else if (x == filled_width) {
        // Partial pixel for smoother progress
        uint8_t r = static_cast<uint8_t>(params_.progress_bar_color.value.r *
                                         remaining_width_f);
        uint8_t g = static_cast<uint8_t>(params_.progress_bar_color.value.g *
                                         remaining_width_f);
        uint8_t b = static_cast<uint8_t>(params_.progress_bar_color.value.b *
                                         remaining_width_f);
        offscreen_canvas->SetPixel(x, bar_y, r, g, b);
      } else {
        offscreen_canvas->SetPixel(x, bar_y, 0, 0, 0); // Clear unfilled part
      }
    }

    display_covers(is_fading ? fade_progress : 1.0f);
    render_track_text(pending_track_data);
    // offscreen_canvas->Fill(255, 0, 0);
    offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);

    pending_track_data.progress_ms +=
        static_cast<long>((time - last_animate_call) * 1000);
  }

  last_animate_call = time;
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

} // namespace animations