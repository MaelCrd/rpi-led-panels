#ifndef SPLASH_ANIMATION_H
#define SPLASH_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>

namespace animations {

struct SplashParams : ParameterSet<SplashParams> {
  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }
};

struct Drop {
  float x;
  float y;
  float radius;
  uint8_t color[3];
};

class Splash : public Animation<Splash, struct SplashParams> {
private:
  std::vector<Drop> droplets;

public:
  Splash(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    srand(time(0));
    offscreen_canvas = matrix->CreateFrameCanvas();
  }
  void animate(double time) override;

  std::string name() const override { return "Splash"; }

  // Override mode parameter methods - Splash has no parameters so all
  // modes are the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

protected:
  int color[3]{0, 0, 0};
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // SPLASH_ANIMATION_H