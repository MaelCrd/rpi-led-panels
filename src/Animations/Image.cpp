#include "Animations/Image.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "FastNoise/Base64.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace animations {

Image::Image(rgb_matrix::RGBMatrix *matrix)
    : Animation<Image, ImageParams>(matrix) {
  width = matrix->width();
  height = matrix->height();
  image_width = 0;  // Will be set when image is loaded
  image_height = 0; // Will be set when image is loaded
  cache_valid = false;
  offscreen_canvas = matrix->CreateFrameCanvas();
}

void Image::animate(double /*time*/) {
  std::lock_guard<std::mutex> lock(image_mutex);

  // Refresh cache if needed
  if (!cache_valid.load(std::memory_order_acquire)) {
    refreshCache();
  }

  if (cached_rgb_data.empty()) {
    // No image data, clear the matrix
    matrix->Clear();
    return;
  }

  // Display the image data directly on the matrix
  int matrix_width = matrix->width();
  int matrix_height = matrix->height();

  // Use actual image dimensions if available, otherwise fallback to matrix size
  int img_w = (image_width > 0) ? image_width : matrix_width;
  int img_h = (image_height > 0) ? image_height : matrix_height;

  // Calculate how much of the image we can display
  int display_width = std::min(img_w, matrix_width);
  int display_height = std::min(img_h, matrix_height);

  // Calculate centering offsets if image is smaller than matrix
  int offset_x = (matrix_width - display_width) / 2;
  int offset_y = (matrix_height - display_height) / 2;

  // Clear the matrix first
  offscreen_canvas->Clear();

  // Copy image data to matrix
  for (int y = 0; y < display_height; ++y) {
    for (int x = 0; x < display_width; ++x) {
      // Calculate index in image data (RGB format) using actual image width
      int image_index = (y * img_w + x) * 3;

      if (image_index + 2 < static_cast<int>(cached_rgb_data.size())) {
        uint8_t r = cached_rgb_data[image_index];
        uint8_t g = cached_rgb_data[image_index + 1];
        uint8_t b = cached_rgb_data[image_index + 2];

        offscreen_canvas->SetPixel(x + offset_x, y + offset_y, r, g, b);
      }
    }
  }
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

void Image::setImageData(const std::vector<uint8_t> &data, int width,
                         int height) {
  std::lock_guard<std::mutex> lock(image_mutex);
  std::lock_guard<std::mutex> param_lock(params_mutex_);

  // Validate input
  if (width <= 0 || height <= 0 ||
      data.size() != static_cast<size_t>(width * height * 3)) {
    params_.image_data.value.clear();
    cached_rgb_data.clear();
    cache_valid.store(false, std::memory_order_release);
    return;
  }

  // Convert to base64 and store in parameters
  std::string base64_data = FastNoise::Base64::Encode(data);
  params_.image_data.value = base64_data;

  // Update cache
  cached_rgb_data = data;
  cache_valid.store(true, std::memory_order_release);
}

void Image::setImageDataBase64(const std::string &base64_data, int width,
                               int height) {
  std::lock_guard<std::mutex> lock(image_mutex);
  std::lock_guard<std::mutex> param_lock(params_mutex_);

  // Validate dimensions
  if (width <= 0 || height <= 0) {
    params_.image_data.value.clear();
    cached_rgb_data.clear();
    cache_valid.store(false, std::memory_order_release);
    return;
  }

  // Decode to validate size
  std::vector<uint8_t> decoded_data =
      FastNoise::Base64::Decode(base64_data.c_str());
  size_t expected_size = static_cast<size_t>(width * height * 3);
  if (decoded_data.size() != expected_size) {
    params_.image_data.value.clear();
    cached_rgb_data.clear();
    cache_valid.store(false, std::memory_order_release);
    return;
  }

  // Update parameters
  params_.image_data.value = base64_data;

  // Update cache
  cached_rgb_data = decoded_data;
  cache_valid.store(true, std::memory_order_release);
}

void Image::clearImage() {
  std::lock_guard<std::mutex> lock(image_mutex);
  std::lock_guard<std::mutex> param_lock(params_mutex_);

  params_.image_data.value.clear();
  cached_rgb_data.clear();
  cache_valid.store(false, std::memory_order_release);
}

void Image::onParametersChanged() {
  cache_valid.store(false, std::memory_order_release);
}

void Image::refreshCache() {
  // Decode the base64 data from parameters
  std::string base64_data;
  {
    std::lock_guard<std::mutex> lock(params_mutex_);
    base64_data = params_.image_data.value;
  }

  if (base64_data.empty()) {
    cached_rgb_data.clear();
    cache_valid.store(true, std::memory_order_release);
    return;
  }

  // Decode base64 data
  std::vector<uint8_t> png_data =
      FastNoise::Base64::Decode(base64_data.c_str());

  // Use stb_image to decode PNG to RGB
  int img_width, img_height, img_channels;
  unsigned char *rgb_data = stbi_load_from_memory(
      png_data.data(), static_cast<int>(png_data.size()), &img_width,
      &img_height, &img_channels, 3 // Force 3 channels (RGB)
  );

  if (!rgb_data) {
    cached_rgb_data.clear();
    cache_valid.store(true, std::memory_order_release);
    return;
  }

  // Copy decoded RGB data to our cache
  size_t rgb_size = img_width * img_height * 3;
  cached_rgb_data.assign(rgb_data, rgb_data + rgb_size);

  // Update our internal dimensions to match the actual image
  image_width = img_width;
  image_height = img_height;

  // Free stb_image memory
  stbi_image_free(rgb_data);

  cache_valid.store(true, std::memory_order_release);
}

} // namespace animations