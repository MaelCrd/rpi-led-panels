#ifndef MAGNETIC_FIELD_ANIMATION_H
#define MAGNETIC_FIELD_ANIMATION_H

#include <cmath>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

struct MagneticFieldParams : ParameterSet<MagneticFieldParams> {
  // Provide tuple of references for iteration
  auto tuple() { return std::tie(speed, color); }
  auto tuple() const { return std::tie(speed, color); }

  PARAM_FLOAT(speed, 1.0, 0.1, 3, "Speed")
  PARAM_COLOR(color, Color(0, 0, 0), "Color")
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
public:
  MagneticField(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  }

  void animate(double time) override;

  std::string name() const override { return "MagneticField"; }

  // Override mode parameter methods - MagneticField has no parameters so all
  // modes are the same
  void applyDefaultParameters() override {
    params_.speed.value = 1.0;
    params_.color.value = Color(
        0, 0, 0); // Default to black (no color) so that the mixed color is used
  }

  void applyPresetParameters() override {
    params_.speed.value = 0.75;
    params_.color.value = Color(255, 0, 0);
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;

private:
  void setup();
  void spawnMagneticElement(bool initial_spawn);

  // Initialize a vector with one magnetic element for testing
  std::vector<MagneticElement> magnetic_elements =
      {}; // {100, 50, 1.0f, 50.0f, 0.0f, {255, 0, 0}}
  double last_time = 0;
  bool initialized = false;
};
} // namespace animations

#endif // MAGNETIC_FIELD_ANIMATION_H