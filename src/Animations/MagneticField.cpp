#include "Animations/MagneticField.h"
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

void MagneticField::animate(double time) {
  auto c = rgb_matrix::Color(255, 0, 255);
  uint8_t r, g, b;

  offscreen_canvas->Clear();

  // Create a new magnetic element with a certain probability
  if (rand() % 60 < 1) { // % chance of spawning a new magnetic element
    MagneticElement element;
    element.x = rand() % offscreen_canvas->width();
    element.y = rand() % offscreen_canvas->height();
    element.life = (rand() % 10 + 20) / 10.0;
    element.age = 0;
    element.color[0] = rand() % 256;
    element.color[1] = rand() % 256;
    element.color[2] = rand() % 256;
    magnetic_elements.push_back(element);
  }

  // Update magnetic elements
  for (auto it = magnetic_elements.begin(); it != magnetic_elements.end();) {
    MagneticElement &element = *it;
    float delta_time = time - last_time;
    element.age += delta_time;
    element.strength =
        (1 + sin((element.age / element.life - 0.25f) * M_PI * 2.0f)) / 2.0f;
    if (element.age > element.life) {
      it = magnetic_elements.erase(it);
    } else {
      ++it;
    }
  }

  // Display
  for (int x = 1; x < offscreen_canvas->width(); x += 3) {
    for (int y = 1; y < offscreen_canvas->height(); y += 3) {
      float force_x = 0;
      float force_y = 0;
      for (const auto &element : magnetic_elements) {
        float dx = element.x - x;
        float dy = element.y - y;
        float distance_squared = dx * dx + dy * dy;
        if (distance_squared < 1.0f)
          distance_squared = 1.0f; // Avoid division by zero
        float force_magnitude =
            1000000.0f / distance_squared * element.strength;
        force_x += force_magnitude * (dx / sqrt(distance_squared));
        force_y += force_magnitude * (dy / sqrt(distance_squared));
      }
      // Normalize the force
      float force_magnitude =
          sqrt((force_x * force_x + force_y * force_y)) * 0.1f;
      force_magnitude = std::min(force_magnitude, 255.0f);

      // Draw a line representing the force vector (in a 3x3 area)
      float angle = atan2(force_y, force_x);
      int angle_index =
          static_cast<int>(round((angle + M_PI) / (M_PI * 2.0) * 8.0));
      int dx = 0, dy = 0;
      switch (angle_index) {
      case 0: // Right (pt. 1)
        dx = 1;
        dy = 0;
        break;
      case 1: // Down-Right
        dx = 1;
        dy = 1;
        break;
      case 2: // Down
        dx = 0;
        dy = 1;
        break;
      case 3: // Down-Left
        dx = 1;
        dy = -1;
        break;
      case 4: // Left
        dx = -1;
        dy = 0;
        break;
      case 5: // Up-Left
        dx = -1;
        dy = -1;
        break;
      case 6: // Up
        dx = 0;
        dy = -1;
        break;
      case 7: // Up-Right
        dx = 1;
        dy = -1;
        break;
      case 8: // Right (pt. 2)
        dx = 1;
        dy = 0;
        break;
      default:
        dx = 0;
        dy = 0;
        break;
      }

      int x1 = x - dx;
      int y1 = y - dy;
      int x2 = x + dx;
      int y2 = y + dy;
      uint8_t r, g, b = static_cast<uint8_t>(force_magnitude);
      rgb_matrix::DrawLine(offscreen_canvas, x1, y1, x2, y2,
                           rgb_matrix::Color(255, 255, 255));

      // r = static_cast<uint8_t>(force_magnitude);
      // g = static_cast<uint8_t>(255 - force_magnitude);
      // b = 128; // Constant blue value
      // offscreen_canvas->SetPixel(x, y, r, g, b);
    }
  }

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
  // usleep(20000);
  last_time = time;
}

} // namespace animations