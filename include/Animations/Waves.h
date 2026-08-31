#ifndef WAVES_ANIMATION_H
#define WAVES_ANIMATION_H

#include <cmath>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "Animation.h"
#include "parameters/param_system.hpp"

#ifndef ASSETS_DIR
#define ASSETS_DIR ".."
#endif

namespace animations {

struct WavesParams : ParameterSet<WavesParams> {
  auto tuple() { return std::tie(color, depth); }
  auto tuple() const { return std::tie(color, depth); }

  PARAM_COLOR(color, Color(255, 255, 255), "Color")
  PARAM_FLOAT(depth, 1, 0, 10, "Depth");
};

class Waves : public Animation<Waves, struct WavesParams> {
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

private:
  void initialize();
  void update_map(double delta_time, int width, int height);

  std::vector<float> map;
  std::vector<float> last_map;
  int map_size;
  double last_time = 0.0;
  bool initialized = false;
};
} // namespace animations

#endif // WAVES_ANIMATION_H