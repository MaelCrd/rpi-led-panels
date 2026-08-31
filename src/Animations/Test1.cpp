#include "Animations/Test1.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <unistd.h>

#include "graphics.h"

#include "Utils.h"

namespace animations {

void Test1::animate(double time) {
  // Draw rectangles with increasing size and rotating

  int center_x = offscreen_canvas->width() / 2;
  int center_y = offscreen_canvas->height() / 2;

  // auto c = rgb_matrix::Color(255, 0, 255);
  uint8_t r, g, b;

  offscreen_canvas->Clear();
  // int total = 200;
  // float angle_step = 0.3;
  // for (int i = 0; i < total; i++) {
  //   for (int off = 0; off < 64; off++) {
  //     float angle1 = time + i * (0.1) + off * M_PI_4 / 8.0f;
  //     int dist = i * 0.35;
  //     int x = center_x + dist * cos(angle1);
  //     int y = center_y + dist * sin(angle1);
  //     // offscreen_canvas->SetPixel(x, y, 255, 0, 0);
  //     float angle2 = angle1 + 1 * angle_step;
  //     int x2 = center_x + dist * cos(angle2);
  //     int y2 = center_y + dist * sin(angle2);
  //     // offscreen_canvas->SetPixel(x2, y2, 255, 255, 255);
  //     utils::hsvToRgb(off * 255.0 / 64.0, 255, 255, r, g, b);
  //     c = rgb_matrix::Color(r, g, b);
  //     rgb_matrix::DrawLine(offscreen_canvas, x, y, x2, y2, c);
  //   }
  // }

  for (int x = 0; x < offscreen_canvas->width(); x++) {
    for (int y = 0; y < offscreen_canvas->height(); y++) {
      int dx = x - center_x;
      int dy = y - center_y;
      float dist = sqrt(dx * dx + dy * dy);
      float wave = (sin(dist / 12.0 + time * 5) + 1) / 2.0; // 0 to 1
      utils::hsvToRgb(wave * 255, 1.0f, 1.0f, r, g, b);
      offscreen_canvas->SetPixel(x, y, r, g, b);
    }
  }

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
  // usleep(20000);
}

} // namespace animations