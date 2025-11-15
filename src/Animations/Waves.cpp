#include "Animations/Waves.h"
#include <algorithm>
#include <cstdlib>

namespace animations {

void Waves::update_map(double delta_time, int width, int height) {
  // Create a copy of the current map
  float *lastMap = new float[width * height];
  for (int i = 0; i < width * height; i++) {
    lastMap[i] = this->map[i];
  }

  // Update the map - scale changes by delta_time for consistent speed
  // regardless of FPS
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      const int i = y * width + x;
      const float lastValue = lastMap[i];

      // Decay rate scaled by delta_time (assuming ~60fps baseline)
      float decayFactor = 1.0 - (0.04 * delta_time * 60.0);
      this->map[i] =
          lastValue *
          (decayFactor + 0.02 * (static_cast<float>(std::rand()) / RAND_MAX));

      if (lastValue <=
          (0.18 + 0.03 * (static_cast<float>(std::rand()) / RAND_MAX))) {
        float n = 0;

        for (int u = -1; u <= 1; u++) {
          for (int v = -1; v <= 1; v++) {
            if (u == 0 && v == 0) {
              continue;
            }

            int nX = std::abs((x + u) % width);
            int nY = std::abs((y + v) % height);

            const int nI = nY * width + nX;
            const float nLastValue = lastMap[nI];

            if (nLastValue >=
                (0.5 + 0.04 * (static_cast<float>(std::rand()) / RAND_MAX))) {
              n += 1;
              this->map[i] +=
                  nLastValue * (0.9 + 0.1 * (static_cast<float>(std::rand()) /
                                             RAND_MAX)); // ici la size !!
            }
          }
        }

        if (n > 0) {
          this->map[i] *= 1 / n;
        }

        if (this->map[i] > 1)
          this->map[i] = 1;
      }
    }
  }

  delete[] lastMap;
}

void Waves::animate(double time) {
  double delta_time = time - this->last_time;
  if (delta_time <= 0.0 || delta_time > 1.0)
    delta_time = 0.03; // Default to ~30ms if time jump is too large
  this->last_time = time;

  int width = matrix->width();
  int height = matrix->height();

  // Update the map
  update_map(delta_time, width, height);

  // Clear canvas
  offscreen_canvas->Clear();

  // Draw the map
  float r_max = 255.0 / 137;
  float g_max = 255.0 / 214;
  r_max = 1; // Disable scaling for now
  g_max = 1;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      const int i = y * width + x;
      int r = std::clamp(
          static_cast<int>(
              pow(this->map[i], 1 + (4 * params_.depth.value / 10.0)) *
              params_.color.value.r * r_max),
          0, 255);
      int g = std::clamp(
          static_cast<int>(
              pow(this->map[i], 1 + (4 * params_.depth.value / 10.0)) *
              params_.color.value.g * g_max),
          0, 255);
      int b = std::clamp(
          static_cast<int>(
              pow(this->map[i], 1 + (4 * params_.depth.value / 10.0)) *
              params_.color.value.b * g_max),
          0, 255);
      offscreen_canvas->SetPixel(x, y, r, g, b);
    }
  }

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations