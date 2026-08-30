#ifndef WAVES_ANIMATION_H
#define WAVES_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>
#include <iostream>

#ifndef ASSETS_DIR
#define ASSETS_DIR ".."
#endif

namespace animations {

struct WavesParams : ParameterSet<WavesParams> {
  PARAM_COLOR(color, Color(255, 255, 255), "Color")
  PARAM_FLOAT(depth, 1, 0, 10, "Depth");

  auto tuple() { return std::tie(color, depth); }
  auto tuple() const { return std::tie(color, depth); }
};

class Waves : public Animation<Waves, struct WavesParams> {
private:
  std::vector<float> map;
  std::vector<float> last_map;
  int map_size;
  double last_time = 0.0;
  bool initialized = false;

  void initialize();
  void update_map(double delta_time, int width, int height);

public:
  Waves(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  }

  ~Waves() override = default;

  void animate(double time) override;

  std::string name() const override { return "Waves"; }

  void applyDefaultParameters() override {
    params_.color.value = {255, 255, 255};
    params_.depth.value = 1;
  }

  void applyPresetParameters() override {
    params_.color.value = {255, 5, 0};
    params_.depth.value = 1;
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // WAVES_ANIMATION_H