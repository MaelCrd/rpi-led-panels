#ifndef BIRDFLOCK_ANIMATION_H
#define BIRDFLOCK_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>

namespace animations {

class BirdFlockImpl;

struct BirdFlockParams : ParameterSet<BirdFlockParams> {
  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }
};

class BirdFlock : public Animation<BirdFlock, struct BirdFlockParams> {
private:
  BirdFlockImpl *flockImpl = nullptr;

public:
  BirdFlock(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  };
  void animate(double time) override;

  std::string name() const override { return "BirdFlock"; }

  // Override mode parameter methods - BirdFlock has no parameters so all modes
  // are the same
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

#endif // BIRDFLOCK_ANIMATION_H