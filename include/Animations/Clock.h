#ifndef CLOCK_ANIMATION_H
#define CLOCK_ANIMATION_H

#include <cmath>
#include <string>
#include <tuple>

#include "led-matrix.h"

#include "Animation.h"
#include "parameters/param_system.hpp"

#ifndef ASSETS_DIR
#define ASSETS_DIR ".."
#endif

namespace animations {

struct ClockParams : ParameterSet<ClockParams> {
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }

  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
};

class Clock : public Animation<Clock, struct ClockParams> {
public:
  Clock(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    font.LoadFont(ASSETS_DIR "/deps/matrix/fonts/6x12.bdf");
  }

  void animate(double time) override;

  std::string name() const override { return "Clock"; }

  // Override mode parameter methods - Clock has no parameters so all modes are
  // the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
  rgb_matrix::Font font;
};
} // namespace animations

#endif // CLOCK_ANIMATION_H