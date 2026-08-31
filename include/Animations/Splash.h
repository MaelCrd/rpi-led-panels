#ifndef SPLASH_ANIMATION_H
#define SPLASH_ANIMATION_H

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <string>
#include <tuple>
#include <vector>

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

struct SplashParams : ParameterSet<SplashParams> {
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }

  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
};

struct Drop {
  float x;
  float y;
  float radius;
  uint8_t color[3];
};

class Splash : public Animation<Splash, struct SplashParams> {
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

private:
  std::vector<Drop> droplets;
};
} // namespace animations

#endif // SPLASH_ANIMATION_H