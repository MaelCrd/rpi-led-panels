#include "Animations/SpotifyCurrent/SpotifyFetcher.h"
#include "Animations/SpotifyCurrent/SpotifyClient.h"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace animations {

using json = nlohmann::json;

static volatile sig_atomic_t child_stop = 0;

static void child_sigterm_handler(int) { child_stop = 1; }

SpotifyFetcher::SpotifyFetcher(const std::string &client_id,
                               const std::string &client_secret,
                               const std::string &refresh_token)
    : client_id_(client_id), client_secret_(client_secret),
      refresh_token_(refresh_token) {}

SpotifyFetcher::~SpotifyFetcher() {
  if (fetch_pid_ > 0) {
    kill(fetch_pid_, SIGTERM);
    int status = 0;
    waitpid(fetch_pid_, &status, 0);
    fetch_pid_ = -1;
  }
  if (ipc_pipe_read_fd_ >= 0) {
    close(ipc_pipe_read_fd_);
    ipc_pipe_read_fd_ = -1;
  }
  if (ipc_pipe_write_fd_ >= 0) {
    close(ipc_pipe_write_fd_);
    ipc_pipe_write_fd_ = -1;
  }
}

bool SpotifyFetcher::start() {
  if (fetch_pid_ > 0)
    return true; // Already started

  int pipefd[2];
  if (pipe(pipefd) == 0) {
    ipc_pipe_read_fd_ = pipefd[0];
    ipc_pipe_write_fd_ = pipefd[1];
  } else {
    std::cerr << "SpotifyFetcher::start: pipe() failed: " << errno << "\n";
    return false;
  }

  pid_t pid = fork();
  if (pid < 0) {
    std::cerr << "Failed to fork spotify fetch process\n";
    return false;
  } else if (pid == 0) {
    // Child process
    struct sigaction sa{};
    sa.sa_handler = child_sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);

    if (setpriority(PRIO_PROCESS, 0, 10) != 0) {
      std::cerr << "Spotify child: setpriority failed errno=" << errno << "\n";
    }

    const char *cgroup_root = "/sys/fs/cgroup";
    struct stat st;
    if (stat(cgroup_root, &st) == 0 && S_ISDIR(st.st_mode)) {
      std::string cg_parent = std::string(cgroup_root) + "/rpi_led_panels";
      if (mkdir(cg_parent.c_str(), 0755) != 0 && errno != EEXIST) {
        std::cerr << "Spotify child: mkdir parent cgroup failed errno=" << errno
                  << "\n";
      }
      std::string cg_dir = cg_parent + "/spotify_" + std::to_string(getpid());
      if (mkdir(cg_dir.c_str(), 0755) != 0 && errno != EEXIST) {
        std::cerr << "Spotify child: mkdir cgroup failed errno=" << errno
                  << "\n";
      } else {
        std::string cpu_max_path = cg_dir + "/cpu.max";
        int fd = open(cpu_max_path.c_str(), O_WRONLY | O_CLOEXEC);
        if (fd >= 0) {
          const char *val = "8000 100000";
          ssize_t w = write(fd, val, strlen(val));
          if (w < 0) {
            std::cerr << "Spotify child: write cpu.max failed errno=" << errno
                      << "\n";
          }
          close(fd);
        } else {
          if (errno == ENOENT) {
            std::string parent_subtree = cg_parent + "/cgroup.subtree_control";
            int sfd = open(parent_subtree.c_str(), O_WRONLY | O_CLOEXEC);
            if (sfd >= 0) {
              const char *enable = "+cpu\n";
              if (write(sfd, enable, strlen(enable)) < 0) {
                std::cerr << "Spotify child: write to " << parent_subtree
                          << " failed errno=" << errno << "\n";
              }
              close(sfd);
              struct stat statbuf;
              if (stat(cpu_max_path.c_str(), &statbuf) == 0) {
                if (!S_ISDIR(statbuf.st_mode)) {
                  int fd = open(cpu_max_path.c_str(), O_WRONLY | O_CLOEXEC);
                  if (fd >= 0) {
                    const char *val = "1000 100000";
                    if (write(fd, val, strlen(val)) < 0) {
                      std::cerr << "Spotify child: write cpu.max failed: "
                                << strerror(errno) << "\n";
                    }
                    close(fd);
                  }
                }
              }
            } else {
              std::string root_subtree =
                  std::string(cgroup_root) + "/cgroup.subtree_control";
              int rsfd = open(root_subtree.c_str(), O_WRONLY | O_CLOEXEC);
              if (rsfd >= 0) {
                const char *enable = "+cpu\n";
                if (write(rsfd, enable, strlen(enable)) < 0) {
                  std::cerr << "Spotify child: write to " << root_subtree
                            << " failed errno=" << errno << "\n";
                }
                close(rsfd);
                fd = open(cpu_max_path.c_str(), O_WRONLY | O_CLOEXEC);
                if (fd >= 0) {
                  const char *val = "1000 100000";
                  if (write(fd, val, strlen(val)) < 0) {
                    std::cerr << "Spotify child: write cpu.max failed after "
                                 "enabling root controller errno="
                              << errno << "\n";
                  }
                  close(fd);
                }
              }
            }
          }
        }

        std::string procs_path = cg_dir + "/cgroup.procs";
        int pfd = open(procs_path.c_str(), O_WRONLY | O_CLOEXEC);
        if (pfd >= 0) {
          std::string pidstr = std::to_string(getpid());
          if (write(pfd, pidstr.c_str(), pidstr.size()) < 0) {
            std::cerr << "Spotify child: write cgroup.procs failed errno="
                      << errno << "\n";
          }
          close(pfd);
        }
      }
    }

    if (ipc_pipe_read_fd_ >= 0) {
      close(ipc_pipe_read_fd_);
      ipc_pipe_read_fd_ = -1;
    }

    fetch_thread_worker();
    _exit(0);
  } else {
    // Parent process
    fetch_pid_ = pid;
    if (ipc_pipe_write_fd_ >= 0) {
      close(ipc_pipe_write_fd_);
      ipc_pipe_write_fd_ = -1;
    }
    if (ipc_pipe_read_fd_ >= 0) {
      int flags = fcntl(ipc_pipe_read_fd_, F_GETFL, 0);
      fcntl(ipc_pipe_read_fd_, F_SETFL, flags | O_NONBLOCK);
    }
  }

  return true;
}

void SpotifyFetcher::fetch_thread_worker() {
  SpotifyClient client(client_id_, client_secret_, refresh_token_);
  if (!client.init()) {
    std::cerr << "SpotifyFetcher: Client initialization failed.\n";
    return;
  }

  while (!child_stop) {
    TrackData new_track_data;
    if (client.fetch_track_info(new_track_data, last_track_id_)) {
      if (ipc_pipe_write_fd_ >= 0) {
        try {
          json meta;
          meta["id"] = new_track_data.id;
          meta["name"] = new_track_data.name;
          meta["artists"] = new_track_data.artists;
          meta["progress_ms"] = new_track_data.progress_ms;
          meta["duration_ms"] = new_track_data.duration_ms;
          meta["is_playing"] = new_track_data.is_playing;
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

          ssize_t wrote = write(ipc_pipe_write_fd_, buf.data(), buf.size());
          if (wrote < 0) {
            std::cerr << "fetch_thread_worker: write to ipc pipe failed: "
                      << errno << "\n";
          } else {
            last_track_id_ = new_track_data.id;
          }
        } catch (const std::exception &e) {
          std::cerr << "fetch_thread_worker: ipc serialization error: "
                    << e.what() << "\n";
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}

bool SpotifyFetcher::read_update(TrackData &out_data) {
  if (ipc_pipe_read_fd_ < 0)
    return false;

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(ipc_pipe_read_fd_, &rfds);
  struct timeval tv = {0, 0};
  int sel = select(ipc_pipe_read_fd_ + 1, &rfds, nullptr, nullptr, &tv);
  if (sel <= 0)
    return false;

  uint32_t meta_len_n = 0;
  ssize_t r = read(ipc_pipe_read_fd_, &meta_len_n, 4);
  if (r <= 0)
    return false;
  uint32_t meta_len = ntohl(meta_len_n);

  std::string meta_str;
  meta_str.resize(meta_len);
  ssize_t got = 0;
  while (got < (ssize_t)meta_len) {
    ssize_t n = read(ipc_pipe_read_fd_, &meta_str[got], meta_len - got);
    if (n <= 0)
      return false;
    got += n;
  }

  uint32_t img_len_n = 0;
  r = read(ipc_pipe_read_fd_, &img_len_n, 4);
  if (r <= 0)
    return false;
  uint32_t img_len = ntohl(img_len_n);

  std::vector<uint8_t> img;
  img.resize(img_len);
  got = 0;
  while (got < (ssize_t)img_len) {
    ssize_t n = read(ipc_pipe_read_fd_, img.data() + got, img_len - got);
    if (n <= 0)
      return false;
    got += n;
  }

  try {
    auto meta = json::parse(meta_str);
    if (meta.contains("id"))
      out_data.id = meta["id"].get<std::string>();
    if (meta.contains("name"))
      out_data.name = meta["name"].get<std::string>();
    if (meta.contains("artists"))
      out_data.artists = meta["artists"].get<std::string>();
    if (meta.contains("progress_ms"))
      out_data.progress_ms = meta["progress_ms"].get<long>();
    if (meta.contains("duration_ms"))
      out_data.duration_ms = meta["duration_ms"].get<long>();
    if (meta.contains("is_playing"))
      out_data.is_playing = meta["is_playing"].get<bool>();
    if (meta.contains("cover_width"))
      out_data.cover_width = meta["cover_width"].get<int>();
    if (meta.contains("cover_height"))
      out_data.cover_height = meta["cover_height"].get<int>();
    out_data.cover_rgb_data = std::move(img);
    return true;
  } catch (...) {
    std::cerr << "read_ipc_update: parse error\n";
    return false;
  }
}

} // namespace animations
