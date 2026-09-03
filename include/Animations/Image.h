#ifndef IMAGE_ANIMATION_H
#define IMAGE_ANIMATION_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "Animation.h"
#include "parameters/param_system.hpp"

#ifndef ASSETS_DIR
#define ASSETS_DIR ".."
#endif

namespace animations {

struct ImageParams : ParameterSet<ImageParams> {
  // Provide tuple of references for iteration
  auto tuple() { return std::tie(image_data); }
  auto tuple() const { return std::tie(image_data); }

  // Image dimensions as parameters
  // Image data as base64 string parameter
  PARAM_STRING(image_data, "", "Image Data")
};

class Image : public Animation<Image, struct ImageParams> {
public:
  Image(rgb_matrix::RGBMatrix *matrix);

  void animate(double time) override;

  std::string name() const override { return "Image"; }

  // Set image data from RGB buffer
  void setImageData(const std::vector<uint8_t> &data, int width, int height);

  // Set image data from base64 string
  void setImageDataBase64(const std::string &base64_data, int width,
                          int height);

  // Clear image data
  void clearImage();

  // Parameter change notification
  void onParametersChanged();

  void applyDefaultParameters() override;
  void applyPresetParameters() override;

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;

private:
  // Helper to refresh cached RGB data when parameters change
  void refreshCache();

  // Cached decoded RGB data for performance
  std::vector<uint8_t> cached_rgb_data;
  std::atomic<bool> cache_valid{false};
  mutable std::mutex image_mutex;
  int width;        // Matrix width (hardcoded)
  int height;       // Matrix height (hardcoded)
  int image_width;  // Actual image width
  int image_height; // Actual image height
};

} // namespace animations

#endif // IMAGE_ANIMATION_H