#ifndef STATIC_ANIMATION_H
#define STATIC_ANIMATION_H

#include <cmath>
#include <string>
#include <tuple>

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

struct StaticParams : ParameterSet<StaticParams> {
  // Provide tuple of references for iteration
  auto tuple() { return std::tie(color); }
  auto const tuple() const { return std::tie(color); }

  PARAM_COLOR(color, Color(255, 255, 255), "Color")
};

class Static : public Animation<Static, struct StaticParams> {
public:
  Static(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  }

  void animate(double time) override;

  std::string name() const override { return "Static"; }

  // Override mode parameter methods
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

#endif // STATIC_ANIMATION_H