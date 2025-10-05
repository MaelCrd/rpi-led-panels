#include "Animations/Atom.h"
#include "Utils.h"
#include "graphics.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <unistd.h>

namespace animations {

void Atom::animate(double time) {
  int center_x = offscreen_canvas->width() / 2;
  int center_y = offscreen_canvas->height() / 2;

  // Colorful dots orbiting a white black hole in 3D
  offscreen_canvas->Clear();

  // Draw the black hole
  rgb_matrix::DrawCircle(offscreen_canvas, center_x, center_y, 1,
                         rgb_matrix::Color(255, 255, 255));

  // Draw orbiting dots using parameters
  float radius = 63.0f;
  float offset = 1.2f;
  float max_speed = 4.0f * params_.speed.value; // Use speed parameter
  int num_dots = 48; // Could be made into a parameter
  int num_circles = 3;

  // Apply style variations based on style parameter
  switch (params_.style.value) {
  case 1:
    num_circles = 4;
    num_dots = 36;
    break;
  case 2:
    num_circles = 5;
    num_dots = 24;
    offset = 1.5f;
    break;
  case 3:
    num_circles = 2;
    num_dots = 60;
    offset = 0.8f;
    break;
  case 4:
    num_circles = 6;
    num_dots = 18;
    offset = 2.0f;
    break;
  default: // case 0 - default style
    break;
  }

  float x, y, x2, y2;
  for (int rad_fact = 1; rad_fact < num_circles + 1; rad_fact++) {
    float rad = (float)radius * (float)rad_fact / (float)num_circles;
    float mult = rad_fact % 2 == 0 ? 1.0f : -1.0f;
    float speed = max_speed * (float)(num_circles - (float)rad_fact * 0.80) /
                  (float)num_circles;
    uint8_t r, g, b;
    utils::hsvToRgb(
        355.0f - (float)(rad_fact - 1) * 105.0 / (float)(num_circles - 1),
        params_.saturation.value, 1.0f, r, g, b); // Use saturation parameter
    for (int i = 0; i < num_dots; i++) {
      float angle = mult * time * speed + i * (1.0f * M_PI) / num_dots;
      float angle2 = M_PI_4 + i * (6.0f * 2.0f * M_PI) / num_dots +
                     rad_fact * (1.0f * M_PI);
      x = rad * cos(offset) * cos(angle);
      y = rad * sin(offset) * sin(angle);
      x2 = x * cos(angle2) - y * sin(angle2);
      y2 = x * sin(angle2) + y * cos(angle2);
      int draw_x = center_x + (int)x2;
      int draw_y = center_y + (int)y2;
      // offscreen_canvas->SetPixel(draw_x, draw_y, r, g, b);
      rgb_matrix::DrawCircle(offscreen_canvas, draw_x, draw_y, 1,
                             rgb_matrix::Color(r, g, b));
      offscreen_canvas->SetPixel(draw_x, draw_y, r, g, b);
    }
  }

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations