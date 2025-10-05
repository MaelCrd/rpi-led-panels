#ifndef RANDOM_ANIMATION_H
#define RANDOM_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>

namespace animations {

struct RandomParams : ParameterSet<RandomParams> {
  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }
};

class Random : public Animation<Random, struct RandomParams> {
private:
public:
  Random(rgb_matrix::RGBMatrix *matrix);
  void animate(double time) override;

  std::string name() const override { return "Random"; }

  // Override mode parameter methods - Random has no parameters so all modes are
  // the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

protected:
};

} // namespace animations

#endif // RANDOM_ANIMATION_H