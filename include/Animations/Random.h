#ifndef RANDOM_ANIMATION_H
#define RANDOM_ANIMATION_H

#include <cmath>
#include <string>
#include <tuple>

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

struct RandomParams : ParameterSet<RandomParams> {
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }

  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
};

class Random : public Animation<Random, struct RandomParams> {
public:
  Random(rgb_matrix::RGBMatrix *matrix);

  void animate(double time) override;

  std::string name() const override { return "Random"; }

  // Override mode parameter methods
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }
};

} // namespace animations

#endif // RANDOM_ANIMATION_H