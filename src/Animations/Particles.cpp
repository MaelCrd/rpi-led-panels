#include "Animations/Particles.h"
#include <cstdlib>
#include <unistd.h>

namespace animations {

void Particles::animate(double time) {
  double delta_time = time - last_time;
  last_time = time;

  offscreen_canvas->Clear();

  // If there are no particles, initialize them
  if (particles.empty()) {
    for (int i = 0; i < 50; ++i) {
      Particle p;
      p.x = rand() % offscreen_canvas->width();
      p.y = rand() % offscreen_canvas->height();
      p.strength = static_cast<float>(rand()) / RAND_MAX;
      p.vx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) *
             2.0f; // Random velocity between -1 and 1
      p.vy = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
      p.color = params_.color.value;
      particles.push_back(p);
    }
  }

  // Clear the grid for this frame
  for (auto &row : grid) {
    std::fill(row.begin(), row.end(), Color(0, 0, 0));
  }

  // Update particles
  for (auto &p : particles) {
    // Update position
    p.x += p.vx * delta_time;
    p.y += p.vy * delta_time;

    // Slightly randomize velocity to create a more organic movement
    p.vx += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.3f;
    p.vy += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.3f;

    // Draw particle. The position is a float so to make the particle move
    // smoothly, we draw the neighboring pixels with decreasing intensity based
    // on the distance from the particle's position.
    for (int dx = -2; dx <= 2; ++dx) {
      for (int dy = -2; dy <= 2; ++dy) {
        int draw_x = static_cast<int>(p.x) + dx;
        int draw_y = static_cast<int>(p.y) + dy;
        if (draw_x >= 0 && draw_x < offscreen_canvas->width() && draw_y >= 0 &&
            draw_y < offscreen_canvas->height()) {
          float distance = std::sqrt((draw_x - p.x) * (draw_x - p.x) +
                                     (draw_y - p.y) * (draw_y - p.y));
          float intensity =
              std::max(0.0f, 1.0f - (distance /
                                     2.0f)); // Decrease intensity with distance
          grid[draw_y][draw_x] +=
              Color(static_cast<uint8_t>(p.color.r * intensity),
                    static_cast<uint8_t>(p.color.g * intensity),
                    static_cast<uint8_t>(p.color.b * intensity));
        }
      }
    }
  }

  // Draw the grid to the offscreen canvas
  for (int y = 0; y < offscreen_canvas->height(); ++y) {
    for (int x = 0; x < offscreen_canvas->width(); ++x) {
      const Color &c = grid[y][x];
      offscreen_canvas->SetPixel(x, y, c.r, c.g, c.b);
    }
  }

  // std::cout << "----------" << std::endl;

  // // Debug : print particle positions and velocities
  // for (const auto &p : particles) {
  //   std::cout << "Particle at (" << p.x << ", " << p.y << ") with velocity ("
  //             << p.vx << ", " << p.vy << ") and color ("
  //             << static_cast<int>(p.color.r) << ", "
  //             << static_cast<int>(p.color.g) << ", "
  //             << static_cast<int>(p.color.b) << ")\n";
  // }

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
  usleep(1000000 / 120); // ~20 FPS
}

} // namespace animations