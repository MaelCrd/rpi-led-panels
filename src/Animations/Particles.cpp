#include "Animations/Particles.h"
#include <cmath>
#include <cstdlib>
#include <unistd.h>

namespace animations {

void Particles::animate(double time) {
  double delta_time = time - last_time;
  last_time = time;

  offscreen_canvas->Clear();

  // If there are no particles, initialize them
  if (particles.empty()) {
    for (int i = 0; i < 1000; ++i) {
      Particle p;
      p.x = rand() % offscreen_canvas->width();
      p.y = rand() % offscreen_canvas->height();
      p.strength = static_cast<float>(rand()) / RAND_MAX;
      p.vx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) *
             2.0f; // Random velocity between -1 and 1
      p.vy = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
      p.color = params_.color.value;
      p.random_factor = static_cast<float>(rand()) /
                        RAND_MAX; // Random factor between 0 and 1
      particles.push_back(p);
    }
  }

  // // Clear the grid for this frame
  // for (auto &row : grid) {
  //   std::fill(row.begin(), row.end(), Color(0, 0, 0));
  // }
  // Make the grid fade out over time to create a trail effect
  for (auto &row : grid) {
    for (auto &c : row) {
      float speed = 1.0f * delta_time;
      c = Color(static_cast<uint8_t>((c.r) * (1.0f - speed)),
                static_cast<uint8_t>((c.g) * (0.991f - speed)),
                static_cast<uint8_t>((c.b) * (0.991f - speed)));
    }
  }

  float target_speed = 12.0f; // Target speed
  // Update particles
  for (auto &p : particles) {
    switch (params_.movementType.value) {
    case 0: // Random direction
    {
      // Slightly randomize velocity to create a more organic movement
      p.vx +=
          (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 20.0f * delta_time;
      p.vy +=
          (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 20.0f * delta_time;

      // Very slight chance to have a sudden change in direction
      if (static_cast<float>(rand()) / RAND_MAX < 0.1f * delta_time) {
        p.vx += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 70.0f;
        p.vy += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 70.0f;
      }
      break;
    }
    case 1: // Circular movement
    {
      float angle = std::atan2(p.vy, p.vx);
      float speed = std::sqrt(p.vx * p.vx + p.vy * p.vy);
      // Make the speed tend to a target value to prevent particles from
      // accelerating indefinitely
      speed += (target_speed - speed) * 0.5f * delta_time;

      angle += 3.0f * delta_time; // Constantly rotate
      if (static_cast<float>(rand()) / RAND_MAX < 0.18f * delta_time) {
        speed += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 70.0f;
      }
      float x_drift =
          (1.0f - (static_cast<float>(rand()) / RAND_MAX)) * delta_time * 5.0f;
      float y_drift =
          (1.0f - (static_cast<float>(rand()) / RAND_MAX)) * delta_time * 5.0f;
      p.vx = std::cos(angle) * speed + x_drift;
      p.vy = std::sin(angle) * speed + y_drift;
      break;
    }
    case 2: // Sine wave movement
    {
      // Movement : Random direction (0) with sine wave oscillation in the
      // direction perpendicular to the velocity vector

      // Slightly randomize velocity to create a more organic movement
      p.vx +=
          (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 20.0f * delta_time;
      p.vy +=
          (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 20.0f * delta_time;

      // Add sine wave oscillation perpendicular to the velocity vector
      float angle = std::atan2(p.vy, p.vx);
      float sine_amplitude = 0.1f; // Amplitude of the sine wave
      float sine_frequency = 2.9f; // Frequency of the sine wave
      p.vx += sine_amplitude *
              std::sin(sine_frequency * time + p.random_factor * M_PI * 2.0f) *
              std::cos(angle + M_PI_2); // Perpendicular direction
      p.vy += sine_amplitude *
              std::sin(sine_frequency * time + p.random_factor * M_PI * 2.0f) *
              std::sin(angle + M_PI_2); // Perpendicular direction
      break;
    }
    case 3: // Erratic movement
    {
      // Have a chance to randomly change direction and speed
      if (static_cast<float>(rand()) / RAND_MAX < 7.0f * delta_time) {
        p.vx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 55.0f;
        p.vy = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 55.0f;
      }

      break;
    }
    default:
      break;
    }

    // Update position based on velocity
    p.x += p.vx * delta_time;
    p.y += p.vy * delta_time;

    // If the particle goes out of bounds, wrap it around to the other side
    if (p.x < 0)
      p.x += offscreen_canvas->width();
    else if (p.x >= offscreen_canvas->width())
      p.x -= offscreen_canvas->width();
    if (p.y < 0)
      p.y += offscreen_canvas->height();
    else if (p.y >= offscreen_canvas->height())
      p.y -= offscreen_canvas->height();

    // Make the velocity tend to a value to prevent particles from
    // accelerating indefinitely
    float target_vxy = 12.0f; // Target velocity magnitude
    float current_vxy = std::sqrt(p.vx * p.vx + p.vy * p.vy);
    float target_vx = (p.vx / current_vxy) * target_vxy;
    float target_vy = (p.vy / current_vxy) * target_vxy;
    p.vx += (target_vx - p.vx) * 0.5f * delta_time;
    p.vy += (target_vy - p.vy) * 0.5f * delta_time;

    // Draw particle. The position is a float so to make the particle move
    // smoothly, we draw the neighboring pixels with decreasing intensity based
    // on the distance from the particle's position.
    int radius = 2; // Radius of influence for the particle
    for (int dx = -radius; dx <= radius; ++dx) {
      for (int dy = -radius; dy <= radius; ++dy) {
        int draw_x = static_cast<int>(p.x) + dx;
        int draw_y = static_cast<int>(p.y) + dy;
        float distance = std::sqrt((draw_x - p.x) * (draw_x - p.x) +
                                   (draw_y - p.y) * (draw_y - p.y));
        float intensity = std::fmax(
            0.0f,
            1.0f - (distance / (1.415 * radius /
                                2.99f))); // Decrease intensity with distance
        int pos_y = draw_y % offscreen_canvas->height();
        int pos_x = draw_x % offscreen_canvas->width();
        if (pos_y < 0)
          pos_y += offscreen_canvas->height();
        if (pos_x < 0)
          pos_x += offscreen_canvas->width();
        grid[pos_y][pos_x] +=
            Color(static_cast<uint8_t>(p.color.r * intensity),
                  static_cast<uint8_t>(p.color.g * intensity),
                  static_cast<uint8_t>(p.color.b * intensity));
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
  // usleep(1000000 / 120); // ~20 FPS
}

} // namespace animations