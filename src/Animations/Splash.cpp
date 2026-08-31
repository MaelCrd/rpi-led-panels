#include "Animations/Splash.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <utility>

#include "Utils.h"

namespace animations {

void Splash::animate(double time) {
  // At random times, spawn a new droplet
  if (rand() % 100 < 1) { // % chance each frame
    Drop droplet;
    droplet.x = rand() % matrix->width() + 0.5f;
    droplet.y = rand() % matrix->height() + 0.5f;
    droplet.radius = 0.5f;
    utils::hsvToRgb(rand() % 256, 1.0f, 1.0f, droplet.color[0],
                    droplet.color[1], droplet.color[2]);
    droplets.push_back(droplet);
  }
  // Update droplets
  for (auto it = droplets.begin(); it != droplets.end();) {
    Drop &droplet = *it;
    droplet.radius += 0.9872568f; // Expand radius
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
    // int radius = static_cast<int>(droplet.radius);
    //
    // int brightness =
    // //     std::clamp(255 - static_cast<int>(droplet.radius * 1.3), 0, 255);

    // // Draw the circle outline (only 1 pixel wide)
    // for (int y = -radius; y <= radius; ++y) {
    //   for (int x = -radius; x <= radius; ++x) {
    //     // If inside the circle outline
    //     if (x * x + y * y <= radius * radius) {
    //       offscreen_canvas->SetPixel(droplet.x + x, droplet.y + y,
    //                                  droplet.color[0], droplet.color[1],
    //                                  droplet.color[2]);
    //     }
    //   }
    // }

    // // Draw the circle outline (only 1 pixel wide)
    // for (int dy = -radius; dy <= radius; ++dy) {
    //   float dx = sqrt(radius * radius - dy * dy);
    //   int left = ceil(-dx), right = floor(dx);
    //   offscreen_canvas->SetPixel(droplet.x + left, droplet.y + dy,
    //                              droplet.color[0], droplet.color[1],
    //                              droplet.color[2]);
    //   offscreen_canvas->SetPixel(droplet.x + right, droplet.y + dy,
    //                              droplet.color[0], droplet.color[1],
    //                              droplet.color[2]);
    // }

    for (int r = 0; r <= floor(droplet.radius * sqrt(0.5)); r++) {
      int d = floor(sqrt(droplet.radius * droplet.radius - r * r));
      // draw tile (center.x - d, center.y + r)
      // draw tile (center.x + d, center.y + r)
      // draw tile (center.x - d, center.y - r)
      // draw tile (center.x + d, center.y - r)
      // draw tile (center.x + r, center.y - d)
      // draw tile (center.x + r, center.y + d)
      // draw tile (center.x - r, center.y - d)
      // draw tile (center.x - r, center.y + d)

      auto p0 = std::pair(droplet.x + d, droplet.y + r);
      auto p1 = std::pair(droplet.x - d, droplet.y + r);
      auto p2 = std::pair(droplet.x + d, droplet.y - r);
      auto p3 = std::pair(droplet.x - d, droplet.y - r);
      auto p4 = std::pair(droplet.x + r, droplet.y + d);
      auto p5 = std::pair(droplet.x + r, droplet.y - d);
      auto p6 = std::pair(droplet.x - r, droplet.y + d);
      auto p7 = std::pair(droplet.x - r, droplet.y - d);

      for (const auto &p : {p0, p1, p2, p3, p4, p5, p6, p7}) {
        // if (p.first >= 0 && p.first < matrix->width() && p.second >= 0 &&
        //     p.second < matrix->height()) {
        //   offscreen_canvas->SetPixel(p.first, p.second, droplet.color[0],
        //                              droplet.color[1], droplet.color[2]);
        // }

        // random angle
        // auto angle = static_cast<float>(rand()) / RAND_MAX * 2 * M_PI;
        // auto angle = sin(10.0f * atan2(p.second, p.first));
        // auto angle = sin(
        //     time + atan2(p.second - droplet.y, p.first - droplet.x) * 10.0f);
        auto angle =
            -time + atan2(p.second - droplet.y, p.first - droplet.x) * 4.0f;
        float factor = droplet.radius * 0.02f;
        factor = 12.5f;
        int dx = static_cast<int>(cos(angle) * factor);
        int dy = static_cast<int>(sin(angle) * factor);
        offscreen_canvas->SetPixel(p.first + dx, p.second + dy,
                                   droplet.color[0], droplet.color[1],
                                   droplet.color[2]);
      }
    }
  }
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
  usleep(30 * 1000);
}

} // namespace animations