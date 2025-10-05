#ifndef STATIC_ANIMATION_H
#define STATIC_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>

namespace animations {

struct StaticParams : ParameterSet<StaticParams> {
  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }
};

class Static : public Animation<Static, struct StaticParams> {
private:
public:
  Static(rgb_matrix::RGBMatrix *matrix);
  void animate(double time) override;

  std::string name() const override { return "Static"; }

  // Override mode parameter methods - Static has no parameters so all modes are
  // the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

protected:
  int color[3]{0, 0, 0};
};
} // namespace animations

#endif // STATIC_ANIMATION_H