#include "Animations/Stars.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

namespace animations {

void Stars::animate(double time) {
  // if (time > 0)
  //   return;
  // Generate random pixel values
  offscreen_canvas->Clear();
  u_int8_t brightness = 0;
  for (int x = 0; x < matrix->width(); ++x) {
    for (int y = 0; y < matrix->height(); ++y) {
      // if (rand() % 100 < 1) { // 10% chance of a star
      //   brightness = rand() % 256;
      //   offscreen_canvas->SetPixel(x, y, brightness, brightness, brightness);
      // }
      brightness = table[y * matrix->width() + x];
      int new_brightness = static_cast<int>(
          brightness +
          brightness * sin(time * 0.7f + (4.0f * x - y) / 10.0f) * 0.45f);
      brightness = std::clamp(new_brightness, 0, 255);
      Color stars_color = params_.starsColor.value;
      offscreen_canvas->SetPixel(x, y, brightness * stars_color.r / 255,
                                 brightness * stars_color.g / 255,
                                 brightness * stars_color.b / 255);
    }
  }

  if (rand() % 100 < 1) { // % chance of a shooting star
    ShootingStar star;
    bool valid_star = false;
    int attempts = 0;
    star.age = 0;
    Color shooting_color = params_.shootingStarsColor.value;
    star.color[0] = shooting_color.r;
    star.color[1] = shooting_color.g;
    star.color[2] = shooting_color.b;

    while (!valid_star &&
           attempts < 100) { // Limit attempts to avoid infinite loop
      star.x = rand() % matrix->width();
      star.y = rand() % matrix->height();
      star.angle = (rand() % 360) * M_PI / 180.0; // Angle
      float speed = (rand() % 435 + 165);         // Speed
      star.vx = speed * cos(star.angle);
      star.vy = speed * sin(star.angle);
      star.life = (rand() % 4 + 8) / 10.0; // Life

      // Check if star will be visible for at least 3/4 of its life
      float visible_life = star.life * 1.2;
      float end_x = star.x + star.vx * visible_life;
      float end_y = star.y + star.vy * visible_life;

      // Star is valid if it stays within bounds for 3/4 of its life
      if (end_x >= 0 && end_x < matrix->width() && end_y >= 0 &&
          end_y < matrix->height()) {
        valid_star = true;
        // std::cout << "Added star at (" << star.x << ", " << star.y
        //           << "), ends at (" << end_x << ", " << end_y << ")\n";
      }
      attempts++;
    }

    if (valid_star) {
      shooting_stars.push_back(star);
    }
  }

  // Update shooting stars
  for (auto it = shooting_stars.begin(); it != shooting_stars.end();) {
    ShootingStar &star = *it;
    float delta_time = time - last_time;
    star.x += star.vx * delta_time;
    star.y += star.vy * delta_time;
    star.age += delta_time;

    // Remove star if it has exceeded its life
    if (star.age > star.life) {
      it = shooting_stars.erase(it);
    } else {
      ++it;
    }
  }
  // Draw shooting stars
  for (const auto &star : shooting_stars) {
    float age_mult = sin(pow(star.age / star.life, 1) * M_PI);
    float size_max = 1 * age_mult;
    int max_brightness = 255 * age_mult;
    // Draw a filled circle
    int tail_size = 15 + 20 * age_mult;
    for (int i = 0; i < tail_size; ++i) {
      int x = static_cast<int>(star.x - star.vx * i * 0.005);
      int y = static_cast<int>(star.y - star.vy * i * 0.005);
      float size = size_max * (1 - i / static_cast<float>(tail_size));
      int brightness =
          fmin(max_brightness,
               static_cast<int>(255 * (1 - i / static_cast<float>(tail_size))));
      uint8_t r = brightness * star.color[0] / 255;
      uint8_t g = brightness * star.color[1] / 255;
      uint8_t b = brightness * star.color[2] / 255;
      for (int dx = -size; dx <= size; ++dx) {
        for (int dy = -size; dy <= size; ++dy) {
          if (dx * dx + dy * dy <= size * size) {
            float distance = sqrt(dx * dx + dy * dy);
            // int brightness =
            //     static_cast<int>(255 * (1 - distance * 0.8 / size));
            // brightness = std::clamp(brightness, 0, 255);
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < matrix->width() && py >= 0 &&
                py < matrix->height()) {
              offscreen_canvas->SetPixel(px, py, r, g, b);
            }
          }
        }
      }
    }
  }
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
  last_time = time;
  // usleep(1000000 / 60); // Aim for ~20 FPS
}

} // namespace animations