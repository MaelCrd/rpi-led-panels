#include "Animations/Waves.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "graphics.h"

namespace animations {

void Waves::initialize() {
  srand(time(0));

  // Display a loading message
  matrix->SetBrightness(50);
  offscreen_canvas->Clear();
  int center_x = offscreen_canvas->width() / 2;
  int center_y = offscreen_canvas->height() / 2;
  char text[] = "Animation loading...";
  rgb_matrix::Font font;
  font.LoadFont(ASSETS_DIR "/deps/matrix/fonts/6x12.bdf");
  int text_width = 0;
  for (char c : std::string(text))
    text_width += font.CharacterWidth(c);
  int text_height = font.height();
  rgb_matrix::DrawText(offscreen_canvas, font, center_x - text_width / 2,
                       center_y - 3 + text_height / 2,
                       rgb_matrix::Color(255, 255, 255), nullptr, text);
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);

  // Initialize map
  map_size = matrix->width() * matrix->height();
  map = std::vector<float>(map_size, 0.0f);
  for (int y = 0; y < matrix->height(); y++) {
    for (int x = 0; x < matrix->width(); x++) {
      const int i = y * matrix->width() + x;
      map[i] = static_cast<float>(std::rand()) / RAND_MAX;
    }
  }
  last_map = std::vector<float>(map_size, 0.0f);

  double delta = 0.03;
  double st = time(0);
  std::cout << "Initializing Waves animation..." << std::endl;
  float max_t = 13.0;
  for (double t = 0; t < max_t; t += delta) {
    update_map(delta, matrix->width(), matrix->height());
    if (fmod(t, delta * 4) <= delta) { // Update display every X steps
      offscreen_canvas->Clear();
      rgb_matrix::DrawText(offscreen_canvas, font, center_x - text_width / 2,
                           center_y - 3 + text_height / 2,
                           rgb_matrix::Color(255, 255, 255), nullptr, text);
      rgb_matrix::DrawLine(offscreen_canvas, center_x - text_width / 2,
                           center_y + 10,
                           (center_x - text_width / 2) +
                               static_cast<int>((t / max_t) * text_width),
                           center_y + 10, rgb_matrix::Color(120, 120, 120));
      offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
    }
  }
  std::cout << "Waves animation initialized in " << (time(0) - st)
            << " seconds." << std::endl;
}

void Waves::update_map(double delta_time, int width, int height) {
  // Copy the current map
  for (int i = 0; i < width * height; i++) {
    last_map[i] = this->map[i];
  }

  // Update the map - scale changes by delta_time for consistent speed
  // regardless of FPS
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      const int i = y * width + x;
      const float lastValue = last_map[i];

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
            const float nLastValue = last_map[nI];

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
}

void Waves::animate(double time) {
  if (!initialized) {
    initialize();
    initialized = true;
  }

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