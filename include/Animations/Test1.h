#ifndef TEST1_ANIMATION_H
#define TEST1_ANIMATION_H

#include <cmath>
#include <string>
#include <tuple>

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

struct Test1Params : ParameterSet<Test1Params> {
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }

  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
};

class Test1 : public Animation<Test1, struct Test1Params> {
public:
  Test1(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  }

  void animate(double time) override;

  std::string name() const override { return "Test1"; }

  // Override mode parameter methods - Test1 has no parameters so all modes are
  // the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // TEST1_ANIMATION_H