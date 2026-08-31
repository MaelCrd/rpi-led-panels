#include "Animations/MagneticField.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <unistd.h>

#include "graphics.h"

#include "Utils.h"

namespace animations {

void MagneticField::spawnMagneticElement(bool initial_spawn = false) {
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
      if (distance_squared < 7200.0f) { // Minimum distance squared
        valid_position = false;
        break;
      }
    }
    max_attempts--;
  }

  element.life = ((rand() % (4 * 100)) / 100.0f + 3);
  element.age = initial_spawn
                    ? (rand() % static_cast<int>(element.life * 1000)) / 1000.0f
                    : 0.0f;

  // Random hue between 0-12 and 285-360 degrees
  float hue = (285 + rand() % (75 + 12)) % 360;
  uint8_t r, g, b;
  utils::hsvToRgb(hue, 0.73f, 1.0f, r, g, b);
  element.color[0] = r;
  element.color[1] = g;
  element.color[2] = b;

  magnetic_elements.push_back(element);
}

void MagneticField::setup() {
  last_time = 0;
  magnetic_elements.clear();
  // Spawn a few initial magnetic elements
  for (int i = 0; i < 5; ++i) {
    spawnMagneticElement(true);
  }
}

void MagneticField::animate(double time) {
  float delta_time = (time - last_time) * params_.speed.value;
  if (!initialized) {
    setup();
    initialized = true;
  }
  offscreen_canvas->Clear();

  // Create a new magnetic element with a certain probability
  if (magnetic_elements.size() < 2 ||
      (rand() % 100 < 100.0f * delta_time &&
       magnetic_elements.size() <
           3)) { // % chance of spawning a new magnetic element
    spawnMagneticElement();
  }

  // std::cout << "Magnetic elements count: " << magnetic_elements.size()
  //           << std::endl;

  // Update magnetic elements
  for (auto it = magnetic_elements.begin(); it != magnetic_elements.end();) {
    MagneticElement &element = *it;
    element.age += delta_time;
    element.strength =
        (1 + sin((element.age / element.life - 0.25f) * M_PI * 2.0f)) / 2.0f;
    if (element.age > element.life) {
      it = magnetic_elements.erase(it);
    } else {
      ++it;
      // offscreen_canvas->SetPixel(static_cast<int>(element.x),
      //                            static_cast<int>(element.y),
      //                            element.color[0], element.color[1],
      //                            element.color[2]);
    }
  }

  // Display
  for (int x = 1; x < offscreen_canvas->width(); x += 3) {
    for (int y = 1; y < offscreen_canvas->height(); y += 3) {
      float force_x = 0;
      float force_y = 0;
      float total_force = 0;
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
        total_force += force_magnitude;

        mixed_r += element.color[0] * force_magnitude;
        mixed_g += element.color[1] * force_magnitude;
        mixed_b += element.color[2] * force_magnitude;
      }

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

      float max_mixed_value = std::max({mixed_r, mixed_g, mixed_b});
      mixed_r = (mixed_r / max_mixed_value) * 255.0f;
      mixed_g = (mixed_g / max_mixed_value) * 255.0f;
      mixed_b = (mixed_b / max_mixed_value) * 255.0f;

      // temp test
      float fact = std::pow(total_force * 1000.0f, 0.11f);
      fact = std::min(fact, 1.0f); // Clamp to 1.0
      mixed_r = mixed_r * fact;
      mixed_g = mixed_g * fact;
      mixed_b = mixed_b * fact;
      //

      auto color = rgb_matrix::Color(static_cast<uint8_t>(mixed_r),
                                     static_cast<uint8_t>(mixed_g),
                                     static_cast<uint8_t>(mixed_b));
      // If not black, use the user-defined color instead
      if (!(params_.color.value.r == 0 && params_.color.value.g == 0 &&
            params_.color.value.b == 0)) {
        color = rgb_matrix::Color(params_.color.value.r, params_.color.value.g,
                                  params_.color.value.b);
      }

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