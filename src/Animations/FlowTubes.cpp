#include "Animations/FlowTubes.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <random>
#include <unistd.h>
#include <vector>

namespace animations {

// Smoothly interpolate between y1 and y2 based on progress mu (0.0 to 1.0)
double cosine_interpolate(double y1, double y2, double mu) {
  double mu2 = (1.0 - std::cos(mu * M_PI)) / 2.0;
  return y1 * (1.0 - mu2) + y2 * mu2;
}

void FlowTubes::animate(double time) {
  // if (time > 0)
  //   return;
  double delta_time = time - last_time;

  offscreen_canvas->Clear();

  // Setup modern C++ random number generation
  std::uniform_real_distribution<double> dis(0.0, 1.0);

  if (evolution_values.empty()) {
    // Initialize evolution values
    int steps = 128;
    std::vector<double> evolution_steps(steps);
    for (int x = 0; x < evolution_steps.capacity(); ++x) {
      evolution_steps[x] = dis(gen);
    }
    // Evolution values are a smoothed version of the random steps
    evolution_values.resize(steps * 4);
    for (int x = 0; x < evolution_values.capacity(); ++x) {
      double mu = static_cast<double>(x) / (evolution_values.capacity() - 1);
      int keyframe_index = static_cast<int>(mu * (steps - 1));
      double local_mu = (mu * (steps - 1)) - keyframe_index;
      evolution_values[x] =
          2 * cosine_interpolate(
                  evolution_steps[keyframe_index],
                  evolution_steps[std::min(keyframe_index + 1, steps - 1)],
                  local_mu) -
          1.0;
    }
  }

  // // Display evolution_steps values for debugging
  // for (int i = 0; i < evolution_values.size(); ++i) {
  //   if (i % 10 == 0) {
  //     std::cout << "\n";
  //   }
  //   std::cout << evolution_values[i] << " ";
  // }
  // std::cout << "\n";

  // usleep(1000000 * 90);

  int w_count = 8;
  if (high.size() != w_count || low.size() != w_count) {
    high.resize(w_count);
    low.resize(w_count);

    // 1. Generate random keyframes
    for (int i = 0; i < w_count; ++i) {
      high[i] = dis(gen);
      low[i] = dis(gen);
    }
  }

  // 1. Change slightly over time to create a flowing effect
  for (int i = 0; i < w_count; ++i) {
    high[i] += evolution_values[(static_cast<int>(time * 10) + i * 10) %
                                evolution_values.size()] *
               delta_time * 0.01;
    low[i] += evolution_values[(static_cast<int>(time * 10) + i * 10 + 5) %
                               evolution_values.size()] *
              delta_time * 0.01;
    high[i] = std::clamp(high[i], 0.0, 1.0);
    low[i] = std::clamp(low[i], 0.0, 1.0);
  }

  // 2. Interpolate between keyframes
  int steps = offscreen_canvas->width();
  for (int x = 0; x < steps; ++x) {
    double mu = static_cast<double>(x) / (steps - 1);
    int keyframe_index = static_cast<int>(mu * (w_count - 1));
    double local_mu = (mu * (w_count - 1)) - keyframe_index;
    int next_index = std::min(keyframe_index + 1, w_count - 1);
    double interpolated_value =
        cosine_interpolate(high[keyframe_index], high[next_index], local_mu);
    double interpolated_value_low =
        cosine_interpolate(low[keyframe_index], low[next_index], local_mu);

    // 3. Draw the interpolated values on the canvas
    int y_high =
        static_cast<int>(interpolated_value * offscreen_canvas->height() / 2 +
                         offscreen_canvas->height() / 2);
    int y_low = static_cast<int>(interpolated_value_low *
                                 offscreen_canvas->height() / 2);
    float intensity =
        1 - ((y_high - y_low) / static_cast<float>(offscreen_canvas->height()));
    for (int y = 0; y < offscreen_canvas->height(); ++y) {
      if (y >= y_low && y <= y_high) {
        offscreen_canvas->SetPixel(x, y, params_.color.value.r * intensity * 16,
                                   params_.color.value.g * intensity * 16,
                                   params_.color.value.b * intensity * 16);
      } else {
        // offscreen_canvas->SetPixel(x, y, 0, 0, 0); // Black background
      }
    }
  }

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
  // usleep(1000000 / 490); // ~90 FPS
  last_time = time;
}

} // namespace animations