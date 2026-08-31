#include "Animations/Matrix.h"
#include "graphics.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

namespace animations {

void Matrix::animate(double time) {
  double delta_time = time - last_time;
  last_time = time;

  int center_x = offscreen_canvas->width() / 2;
  int center_y = offscreen_canvas->height() / 2;

  offscreen_canvas->Clear();

  uint8_t r = 0, g = 255, b = 0;

  int trail_length = 60;

  for (int i = 0; i < offscreen_canvas->width(); ++i) {
    if ((positions[i] > offscreen_canvas->height() + trail_length + 5 ||
         positions[i] < 0) &&
        (rand() % 250 < 1.0f * delta_time * 4.0f)) {
      positions[i] = 0;
    } else if (positions[i] >= 0 &&
               positions[i] <= offscreen_canvas->height() + trail_length + 5) {
      positions[i] += 1;
    }
  }

  // Display the columns
  for (int x = 0; x < offscreen_canvas->width(); ++x) {
    for (int y = positions[x]; y >= positions[x] - trail_length && y >= 0;
         --y) {
      int brightness = 255 - (positions[x] - y) * (255 / trail_length);
      brightness = std::clamp(brightness, 0, 255);
      int whiteness = std::clamp(255 - (positions[x] - y) * 20, 0, 255);
      offscreen_canvas->SetPixel(x, y, whiteness * brightness / 255,
                                 g * brightness / 255,
                                 whiteness * brightness / 255);
    }
  }

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations