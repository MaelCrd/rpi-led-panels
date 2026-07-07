#include "Animations/MagneticField.h"
#include "Utils.h"
#include "graphics.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <unistd.h>

namespace animations {

void MagneticField::animate(double time) {
  auto c = rgb_matrix::Color(255, 0, 255);
  uint8_t r, g, b;

  offscreen_canvas->Clear();

  // Create a new magnetic element with a certain probability
  if ((rand() % 60 < 1 && magnetic_elements.size() < 5) ||
      magnetic_elements.size() <=
          2) { // % chance of spawning a new magnetic element
    MagneticElement element;

    // Try to spawn the element at a random position, but ensure it is not too
    // close to existing elements
    bool valid_position = false;
    uint16_t max_attempts = 200;
    while (!valid_position && max_attempts > 0) {
      valid_position = true;
      element.x = rand() % offscreen_canvas->width();
      element.y = rand() % offscreen_canvas->height();

      for (const auto &existing_element : magnetic_elements) {
        float dx = existing_element.x - element.x;
        float dy = existing_element.y - element.y;
        float distance_squared = dx * dx + dy * dy;
        if (distance_squared < 5500.0f) { // Minimum distance squared
          valid_position = false;
          break;
        }
      }
      max_attempts--;
    }
    // std::cout << "Max attempts left: " << max_attempts << std::endl;

    element.life = (rand() % 10 + 20) / 3.0;
    element.age = 0;

    // Random hue between 0-20 and 265-360 degrees
    float hue =
        (265 + rand() % (95 + 20)) % 360; // Random hue between 200-360 degrees
    utils::hsvToRgb(hue, 0.8f, 1.0f, r, g, b);
    element.color[0] = r;
    element.color[1] = g;
    element.color[2] = b;

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
      float mixed_r = 0, mixed_g = 0, mixed_b = 0;
      for (const auto &element : magnetic_elements) {
        float dx = element.x - x;
        float dy = element.y - y;
        float distance_squared = pow(dx, 2) + pow(dy, 2);
        if (distance_squared < 1.0f)
          distance_squared = 1.0f; // Avoid division by zero
        float force_magnitude = element.strength / distance_squared;
        force_x += force_magnitude * dx;
        force_y += force_magnitude * dy;

        mixed_r += element.color[0] * force_magnitude;
        mixed_g += element.color[1] * force_magnitude;
        mixed_b += element.color[2] * force_magnitude;
      }

      // Draw a line representing the force vector (in a 3x3 area)
      float angle = atan2(force_y, force_x);
      int angle_index =
          static_cast<int>(round((angle + M_PI) / (M_PI * 2.0) * 8.0));

      rgb_matrix::Color color(255, 255, 255);
      rgb_matrix::Color color_dark(255, 0, 0);
      rgb_matrix::Color color_light(190, 0, 0);
      int dx = 0, dy = 0;
      switch (angle_index) {
      case 0: // Right (pt. 1)
        dx = 1;
        dy = 0;
        color = color_dark;
        break;
      case 1: // Down-Right
        dx = 1;
        dy = 1;
        color = color_light;
        break;
      case 2: // Down
        dx = 0;
        dy = 1;
        color = color_dark;
        break;
      case 3: // Down-Left
        dx = 1;
        dy = -1;
        color = color_light;
        break;
      case 4: // Left
        dx = -1;
        dy = 0;
        color = color_dark;
        break;
      case 5: // Up-Left
        dx = -1;
        dy = -1;
        color = color_light;
        break;
      case 6: // Up
        dx = 0;
        dy = -1;
        color = color_dark;
        break;
      case 7: // Up-Right
        dx = 1;
        dy = -1;
        color = color_light;
        break;
      case 8: // Right (pt. 2)
        dx = 1;
        dy = 0;
        color = color_dark;
        break;
      default:
        dx = 0;
        dy = 0;
        break;
      }

      float max_mixed_value = std::max({mixed_r, mixed_g, mixed_b});
      mixed_r = (mixed_r / max_mixed_value) * 255.0f;
      mixed_g = (mixed_g / max_mixed_value) * 255.0f;
      mixed_b = (mixed_b / max_mixed_value) * 255.0f;
      color = rgb_matrix::Color(static_cast<uint8_t>(mixed_r),
                                static_cast<uint8_t>(mixed_g),
                                static_cast<uint8_t>(mixed_b));

      int x1 = x - dx;
      int y1 = y - dy;
      int x2 = x + dx;
      int y2 = y + dy;
      rgb_matrix::DrawLine(offscreen_canvas, x1, y1, x2, y2, color);

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