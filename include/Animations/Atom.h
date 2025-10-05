#ifndef ATOM_ANIMATION_H
#define ATOM_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>

namespace animations {

struct AtomParams : ParameterSet<AtomParams> {
  // Declare each parameter ONCE: Param<T>{default, min, max, displayName,
  // internalName}
  PARAM_INT(style, 0, 0, 4, "Style")
  PARAM_FLOAT(speed, 1.0f, 0.2f, 5.0f, "Speed")
  PARAM_FLOAT(saturation, 1.0f, 0.0f, 1.0f, "Saturation")

  // Provide tuple of references for iteration
  auto tuple() { return std::tie(style, speed, saturation); }
  auto tuple() const { return std::tie(style, speed, saturation); }
};

class Atom : public Animation<Atom, struct AtomParams> {
private:
public:
  Atom(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  };
  void animate(double time);

  std::string name() const override { return "Atom"; }

  // Override mode parameter methods
  void applyDefaultParameters() override {
    params_.style.value = 0;
    params_.speed.value = 1.0f;
    params_.saturation.value = 1.0f;
  }

  void applyPresetParameters() override {
    // Preset: Fast colorful atoms
    params_.style.value = 2;
    params_.speed.value = 2.5f;
    params_.saturation.value = 0.8f;
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // ATOM_ANIMATION_H