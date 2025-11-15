#ifndef WAVES_ANIMATION_H
#define WAVES_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>
#include <iostream>

namespace animations {

struct WavesParams : ParameterSet<WavesParams> {
  PARAM_COLOR(color, Color(255, 255, 255), "Color")
  PARAM_FLOAT(depth, 1, 0, 10, "Depth");

  auto tuple() { return std::tie(color, depth); }
  auto tuple() const { return std::tie(color, depth); }
};

class Waves : public Animation<Waves, struct WavesParams> {
private:
  float *map;
  int map_size;
  double last_time = 0.0;

  void update_map(double delta_time, int width, int height);

public:
  Waves(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    srand(time(0));
    offscreen_canvas = matrix->CreateFrameCanvas();

    // Initialize map
    map_size = matrix->width() * matrix->height();
    map = new float[map_size];
    for (int y = 0; y < matrix->height(); y++) {
      for (int x = 0; x < matrix->width(); x++) {
        const int i = y * matrix->width() + x;
        map[i] = static_cast<float>(std::rand()) / RAND_MAX;
      }
    }

    double delta = 0.03;
    double st = time(0);
    std::cout << "Initializing Waves animation..." << std::endl;
    for (double t = 0; t < 13.0; t += delta) {
      update_map(delta, matrix->width(), matrix->height());
    }
    std::cout << "Waves animation initialized in " << (time(0) - st)
              << " seconds." << std::endl;
  }

  ~Waves() { delete[] map; }

  void animate(double time) override;

  std::string name() const override { return "Waves"; }

  void applyDefaultParameters() override {
    params_.color.value = {255, 255, 255};
    params_.depth.value = 1;
  }

  void applyPresetParameters() override {
    params_.color.value = {255, 40, 0};
    params_.depth.value = 1;
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // WAVES_ANIMATION_H