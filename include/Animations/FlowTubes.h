#ifndef FLOW_TUBES_ANIMATION_H
#define FLOW_TUBES_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>
#include <random>

namespace animations {

struct FlowTubesParams : ParameterSet<FlowTubesParams> {
  // Empty tuple for animations with no parameters
  PARAM_COLOR(color, Color(255, 255, 255), "Color")

  // Provide tuple of references for iteration
  auto tuple() { return std::tie(color); }
  auto const tuple() const { return std::tie(color); }
};

class FlowTubes : public Animation<FlowTubes, struct FlowTubesParams> {
private:
  std::vector<double> evolution_values;
  std::vector<double> high;
  std::vector<double> low;

  std::mt19937 gen{std::random_device{}()};

  double last_time = 0;

public:
  FlowTubes(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  };
  void animate(double time) override;

  std::string name() const override { return "FlowTubes"; }

  void applyDefaultParameters() override {
    params_.color.value = Color(255, 255, 255);
  }

  void applyPresetParameters() override {
    params_.color.value = Color(255, 0, 0);
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // FLOW_TUBES_ANIMATION_H