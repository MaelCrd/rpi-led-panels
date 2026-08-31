#ifndef SPOTIFY_FETCHER_H
#define SPOTIFY_FETCHER_H

#include <atomic>
#include <mutex>
#include <string>
#include <sys/types.h>

#include "Animations/SpotifyCurrent/SpotifyTrackData.h"

namespace animations {

class SpotifyFetcher {
public:
  SpotifyFetcher(const std::string &client_id, const std::string &client_secret,
                 const std::string &refresh_token);
  ~SpotifyFetcher();

  // Forks the process and starts fetching in the background
  bool start();

  // Reads any pending update from the child process.
  // Returns true if a new update was populated into out_data.
  bool read_update(TrackData &out_data);

private:
  void fetch_thread_worker();

  std::string client_id_;
  std::string client_secret_;
  std::string refresh_token_;

  pid_t fetch_pid_ = -1;
  int ipc_pipe_read_fd_ = -1;
  int ipc_pipe_write_fd_ = -1;

  std::string last_track_id_;
};

} // namespace animations

#endif // SPOTIFY_FETCHER_H
