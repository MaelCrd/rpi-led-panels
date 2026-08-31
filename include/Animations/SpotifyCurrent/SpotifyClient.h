#ifndef SPOTIFY_CLIENT_H
#define SPOTIFY_CLIENT_H

#include "Animations/SpotifyCurrent/SpotifyTrackData.h"
#include <curl/curl.h>
#include <string>
#include <vector>

namespace animations {

class SpotifyClient {
public:
  SpotifyClient(const std::string &client_id, const std::string &client_secret,
                const std::string &refresh_token);
  ~SpotifyClient();

  // Initializes curl and access token
  bool init();

  // Fetches the current track info from Spotify
  // Returns true if successful and out_data is populated.
  // last_track_id is used to prevent re-downloading the same cover art
  // unnecessarily.
  bool fetch_track_info(TrackData &out_data, const std::string &last_track_id);

private:
  bool download_image(const std::string &url, std::vector<uint8_t> &out_data,
                      int &out_width, int &out_height);
  bool refresh_user_access_token(std::string &out_access_token,
                                 std::string &out_new_refresh_token);
  std::string get_token_cache_path() const;
  bool read_cached_token(const std::string &path, std::string &token) const;
  bool write_cached_token(const std::string &path,
                          const std::string &token) const;

  std::string client_id_;
  std::string client_secret_;
  std::string refresh_token_;
  std::string access_token_;

  CURL *curl_;
};

} // namespace animations

#endif // SPOTIFY_CLIENT_H
