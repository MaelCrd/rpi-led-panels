#ifndef BIRDFLOCK_ANIMATION_H
#define BIRDFLOCK_ANIMATION_H

#include <cmath>
#include <memory>
#include <string>
#include <tuple>

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

class BirdFlockImpl;

struct BirdFlockParams : ParameterSet<BirdFlockParams> {
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }

  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
};

class BirdFlock : public Animation<BirdFlock, struct BirdFlockParams> {
public:
  BirdFlock(rgb_matrix::RGBMatrix *matrix);
  ~BirdFlock();

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

private:
  std::unique_ptr<BirdFlockImpl> flockImpl;
};
} // namespace animations

#endif // BIRDFLOCK_ANIMATION_H