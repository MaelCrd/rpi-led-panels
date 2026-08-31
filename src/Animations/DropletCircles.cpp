#include "Animations/DropletCircles.h"
#include "Utils.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace animations {

void DropletCircles::animate(double time) {
  double delta_time = time - last_time;
  last_time = time;

  // At random times, spawn a new droplet
  if (rand() % 100 < 1.0f * delta_time * 4.0f) { // % chance each frame
    Droplet droplet;
    droplet.x = rand() % matrix->width();
    droplet.y = rand() % matrix->height();
    droplet.radius = 1;
    utils::hsvToRgb(rand() % 256, 1.0f, 1.0f, droplet.color[0],
                    droplet.color[1], droplet.color[2]);
    droplets.push_back(droplet);
  }
  // Update droplets
  for (auto it = droplets.begin(); it != droplets.end();) {
    Droplet &droplet = *it;
    droplet.radius += 0.5; // Expand radius
    if (droplet.radius > 200) {
      it = droplets.erase(it); // Remove droplet if too big
    } else {
      ++it;
    }
  }

  // Clear canvas
  offscreen_canvas->Clear();
  // Draw droplets
  for (const auto &droplet : droplets) {
    int radius = static_cast<int>(droplet.radius);
    int brightness =
        std::clamp(255 - static_cast<int>(droplet.radius * 1.3), 0, 255);
    rgb_matrix::DrawCircle(
        offscreen_canvas, droplet.x, droplet.y, radius,
        rgb_matrix::Color(droplet.color[0] * brightness / 255,
                          droplet.color[1] * brightness / 255,
                          droplet.color[2] * brightness / 255));
  }
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations