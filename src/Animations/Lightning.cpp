#include "Animations/Lightning.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>

namespace animations {

void Lightning::animate(double time) {
  const int width = matrix->width();
  const int height = matrix->height();
  const double delta_time = time - last_time;
  last_time = time;

  // Create lightning bolts
  if (rand() % 100 < pow(params_.probability.value * 2, 3) * 200 *
                         delta_time) { // chance per second to create a new bolt
    LightningBolt bolt;

    // Create main branch
    LightningBranch main_branch;
    main_branch.is_main_branch = true;
    main_branch.active = true;
    main_branch.direction = 0; // Main branch is neutral
    main_branch.growth_counter = 0;
    main_branch.points.reserve(height); // Reserve space for points
    main_branch.points.push_back({rand() % width, 0});

    bolt.branches.reserve(8); // Reserve max branches
    bolt.branches.push_back(std::move(main_branch));
    bolts.push_back(std::move(bolt));
  }

  // Update bolts
  std::vector<int> to_remove;
  to_remove.reserve(bolts.size()); // Reserve space

  for (auto &bolt : bolts) {
    if (!bolt.struck) {
      // Grow the lightning bolt
      for (int step = 0; step < 8; ++step) {
        bool reached_bottom = false;
        std::vector<LightningBranch> new_branches;
        new_branches.reserve(4); // Reserve some space

        // Grow all active branches using index-based loop to avoid iterator
        // invalidation
        const size_t num_branches = bolt.branches.size();
        for (size_t b = 0; b < num_branches; ++b) {
          auto &branch = bolt.branches[b];
          if (!branch.active || branch.points.empty())
            continue;

          // Side branches grow slower than main branch
          if (!branch.is_main_branch) {
            branch.growth_counter++;
            if (branch.growth_counter < 3) { // Skip 2 out of 3 growth cycles
              continue;
            }
            branch.growth_counter = 0;
          }

          const auto &last_point = branch.points.back();
          const int lastX = last_point.first;
          const int lastY = last_point.second;

          if (lastY < height - 1) {
            // Move downward with horizontal variation based on direction
            int horizontal_drift;

            if (branch.is_main_branch) {
              // Main branch has random variation
              horizontal_drift = rand() % 5 - 2; // -2, -1, 0, +1, or +2
            } else {
              // Side branches tend toward their direction
              // X% chance to move in the preferred direction
              if (rand() % 100 < 40) {
                horizontal_drift =
                    branch.direction *
                    (rand() % 2 + 1); // 1 or 2 pixels in preferred direction
              } else {
                horizontal_drift =
                    rand() % 5 - 2; // Random movement occasionally
              }
            }

            int nextX = std::clamp(lastX + horizontal_drift, 0, width - 1);
            int nextY = lastY + 1;
            branch.points.push_back({nextX, nextY});

            // Check if main branch reached the bottom
            if (branch.is_main_branch && nextY >= height - 1) {
              reached_bottom = true;
            }

            // Chance to create a new branch from active branches
            if (rand() % 100 < 1 &&
                (bolt.branches.size() + new_branches.size()) < 8) {
              LightningBranch new_branch;
              new_branch.is_main_branch = false;
              new_branch.active = true;
              new_branch.growth_counter = 0;
              new_branch.direction = (rand() % 2 == 0) ? -1 : 1;

              int branch_point = rand() % branch.points.size();
              new_branch.points.reserve(height);
              new_branch.points.push_back(branch.points[branch_point]);

              // Give it an initial step in its direction
              int branch_x =
                  std::clamp(new_branch.points.back().first +
                                 (new_branch.direction * (rand() % 3 + 1)),
                             0, width - 1);
              int branch_y = new_branch.points.back().second + 1;
              if (branch_y < height) {
                new_branch.points.push_back({branch_x, branch_y});
                new_branches.push_back(std::move(new_branch));
              }
            }
          } else {
            // This branch reached the bottom
            if (branch.is_main_branch) {
              reached_bottom = true;
            } else {
              branch.active = false; // Stop growing side branches
            }
          }
        }

        // Add new branches after iteration
        for (auto &new_branch : new_branches) {
          bolt.branches.push_back(std::move(new_branch));
        }

        // If main branch struck, deactivate all other branches
        if (reached_bottom) {
          bolt.struck = true;
          // Keep only the main branch active
          for (auto &branch : bolt.branches) {
            if (!branch.is_main_branch) {
              branch.active = false;
            }
          }
          break;
        }
      }
    }

    if (bolt.struck) {
      // Decay the bolt
      bolt.decay -= 8.2f * delta_time;
      if (bolt.decay < 0.0f)
        to_remove.push_back(&bolt - &bolts[0]);
    }
  }

  // Remove decayed bolts
  for (int i = to_remove.size() - 1; i >= 0; --i) {
    bolts.erase(bolts.begin() + to_remove[i]);
  }

  // Clear pixel buffer
  std::fill(pixel_buffer.begin(), pixel_buffer.end(), 0);

  // Draw the bolts into pixel_buffer
  for (auto &bolt : bolts) {
    const int base_brightness = static_cast<int>(255.0 / 13.0 * bolt.decay);

    for (auto &branch : bolt.branches) {
      // If bolt has struck, only draw the main branch
      if (bolt.struck && !branch.is_main_branch) {
        continue;
      }

      const bool is_struck = branch.is_main_branch && bolt.struck;
      const size_t num_points = branch.points.size();

      // FIRST PASS: Draw half-brightness pixels for thickness (if struck)
      if (is_struck) {
        for (size_t i = 0; i < num_points; ++i) {
          const auto &point = branch.points[i];

          int brightness = base_brightness * 13;
          brightness = std::clamp(brightness, 0, 255);

          const int half_brightness = brightness / 2;

          // Draw left, right, top, and bottom pixels with half brightness
          const int x = point.first;
          const int y = point.second;

          if (x > 0) {
            const int idx = (y * width) + (x - 1);
            pixel_buffer[idx] = std::max(pixel_buffer[idx],
                                         static_cast<uint8_t>(half_brightness));
          }
          if (x < width - 1) {
            const int idx = (y * width) + (x + 1);
            pixel_buffer[idx] = std::max(pixel_buffer[idx],
                                         static_cast<uint8_t>(half_brightness));
          }
          if (y > 0) {
            const int idx = ((y - 1) * width) + x;
            pixel_buffer[idx] = std::max(pixel_buffer[idx],
                                         static_cast<uint8_t>(half_brightness));
          }
          if (y < height - 1) {
            const int idx = ((y + 1) * width) + x;
            pixel_buffer[idx] = std::max(pixel_buffer[idx],
                                         static_cast<uint8_t>(half_brightness));
          }

          // Draw half-brightness for lines between points
          if (i + 1 < num_points) {
            const auto &next_point = branch.points[i + 1];

            int x0 = x, y0 = y;
            int x1 = next_point.first, y1 = next_point.second;

            const int dx = abs(x1 - x0);
            const int dy = abs(y1 - y0);
            const int sx = (x0 < x1) ? 1 : -1;
            const int sy = (y0 < y1) ? 1 : -1;
            int err = dx - dy;

            while (x0 != x1 || y0 != y1) {
              if (x0 > 0) {
                const int idx = (y0 * width) + (x0 - 1);
                pixel_buffer[idx] = std::max(
                    pixel_buffer[idx], static_cast<uint8_t>(half_brightness));
              }
              if (x0 < width - 1) {
                const int idx = (y0 * width) + (x0 + 1);
                pixel_buffer[idx] = std::max(
                    pixel_buffer[idx], static_cast<uint8_t>(half_brightness));
              }
              if (y0 > 0) {
                const int idx = ((y0 - 1) * width) + x0;
                pixel_buffer[idx] = std::max(
                    pixel_buffer[idx], static_cast<uint8_t>(half_brightness));
              }
              if (y0 < height - 1) {
                const int idx = ((y0 + 1) * width) + x0;
                pixel_buffer[idx] = std::max(
                    pixel_buffer[idx], static_cast<uint8_t>(half_brightness));
              }

              const int e2 = 2 * err;
              if (e2 > -dy) {
                err -= dy;
                x0 += sx;
              }
              if (e2 < dx) {
                err += dx;
                y0 += sy;
              }
            }
          }
        }
      }

      // SECOND PASS: Draw full-brightness pixels for the main line
      for (size_t i = 0; i < num_points; ++i) {
        const auto &point = branch.points[i];

        int brightness = base_brightness;
        if (is_struck) {
          brightness *= 13;
        }
        brightness = std::clamp(brightness, 0, 255);

        // Draw center pixel at full brightness
        const int center_idx = (point.second * width) + point.first;
        pixel_buffer[center_idx] = std::max(pixel_buffer[center_idx],
                                            static_cast<uint8_t>(brightness));

        // Draw line to next point if there is one
        if (i + 1 < num_points) {
          const auto &next_point = branch.points[i + 1];

          int x0 = point.first, y0 = point.second;
          int x1 = next_point.first, y1 = next_point.second;

          const int dx = abs(x1 - x0);
          const int dy = abs(y1 - y0);
          const int sx = (x0 < x1) ? 1 : -1;
          const int sy = (y0 < y1) ? 1 : -1;
          int err = dx - dy;

          while (x0 != x1 || y0 != y1) {
            if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
              const int idx = (y0 * width) + x0;
              pixel_buffer[idx] =
                  std::max(pixel_buffer[idx], static_cast<uint8_t>(brightness));
            }
            const int e2 = 2 * err;
            if (e2 > -dy) {
              err -= dy;
              x0 += sx;
            }
            if (e2 < dx) {
              err += dx;
              y0 += sy;
            }
          }
        }
      }
    }
  }

  // Clear canvas
  offscreen_canvas->Clear();

  // Apply color from parameters to all pixels based on brightness in
  // pixel_buffer
  const uint8_t color_r = params_.color.value.r;
  const uint8_t color_g = params_.color.value.g;
  const uint8_t color_b = params_.color.value.b;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int idx = (y * width) + x;
      const uint8_t brightness = pixel_buffer[idx];

      if (brightness > 0) {
        const uint8_t r = (color_r * brightness) / 255;
        const uint8_t g = (color_g * brightness) / 255;
        const uint8_t b = (color_b * brightness) / 255;
        offscreen_canvas->SetPixel(x, y, r, g, b);
      }
    }
  }

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations