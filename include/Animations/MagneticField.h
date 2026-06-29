#ifndef MAGNETIC_FIELD_ANIMATION_H
#define MAGNETIC_FIELD_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>

namespace animations {

struct MagneticFieldParams : ParameterSet<MagneticFieldParams> {
  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }
};

struct MagneticElement {
  float x;
  float y;
  float strength;
  float life; // in seconds
  float age;  // in seconds
  uint8_t color[3];
};

class MagneticField
    : public Animation<MagneticField, struct MagneticFieldParams> {
private:
  // Initialize a vector with one magnetic element for testing
  std::vector<MagneticElement> magnetic_elements =
      {}; // {100, 50, 1.0f, 50.0f, 0.0f, {255, 0, 0}}
  double last_time = 0;

public:
  MagneticField(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  };
  void animate(double time) override;

  std::string name() const override { return "MagneticField"; }

  // Override mode parameter methods - MagneticField has no parameters so all
  // modes are the same
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

#endif // MAGNETIC_FIELD_ANIMATION_H