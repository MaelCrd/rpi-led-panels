#ifndef SPOTIFY_TRACK_DATA_H
#define SPOTIFY_TRACK_DATA_H

#include <cstdint>
#include <string>
#include <vector>

namespace animations {

struct TrackData {
  std::string id;
  std::string name;
  std::string artists;
  std::vector<uint8_t> cover_rgb_data;
  int cover_width = 0;
  int cover_height = 0;
  long progress_ms = 0;
  long duration_ms = 0;
  bool is_playing = false;
};

} // namespace animations

#endif // SPOTIFY_TRACK_DATA_H
