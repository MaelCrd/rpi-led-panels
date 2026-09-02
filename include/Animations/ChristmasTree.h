#ifndef CHRISTMASTREE_ANIMATION_H
#define CHRISTMASTREE_ANIMATION_H

#include <cmath>
#include <string>

#include "led-matrix.h"

#include "Animation.h"
#include "parameters/param_system.hpp"

#ifndef ASSETS_DIR
#define ASSETS_DIR ".."
#endif

namespace animations {

struct ChristmasTreeParams : ParameterSet<ChristmasTreeParams> {
  // Timing parameters for image hold and crossfade durations (seconds)
  PARAM_FLOAT(hold_seconds, 0.0f, 0.0f, 60.0f, "Hold Seconds")
  PARAM_FLOAT(fade_seconds, 2.0f, 0.0f, 60.0f, "Fade Seconds")

  // Provide tuple of references for iteration
  auto tuple() { return std::tie(hold_seconds, fade_seconds); }
  auto tuple() const { return std::tie(hold_seconds, fade_seconds); }
};

class ChristmasTree
    : public Animation<ChristmasTree, struct ChristmasTreeParams> {
public:
  ChristmasTree(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  }

  void animate(double time) override;

  std::string name() const override { return "ChristmasTree"; }

  // Override mode parameter methods
  void applyDefaultParameters() override {
    // No parameters to set
    params_.hold_seconds.value = 0.0f;
    params_.fade_seconds.value = 2.0f;
  }

  void applyPresetParameters() override {
    // No parameters to set
    params_.hold_seconds.value = 3.0f;
    params_.fade_seconds.value = 2.0f;
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // CHRISTMASTREE_ANIMATION_H