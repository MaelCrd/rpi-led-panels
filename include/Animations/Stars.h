#ifndef STARS_ANIMATION_H
#define STARS_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>
#include <cstdint>

namespace animations {

struct StarsParams : ParameterSet<StarsParams> {
  // Add color parameters for stars and shooting stars
  PARAM_COLOR(starsColor, Color(230, 245, 255), "Stars Color")
  PARAM_COLOR(shootingStarsColor, Color(255, 255, 255), "Shooting Stars Color")

  // Provide tuple of references for iteration
  auto tuple() { return std::tie(starsColor, shootingStarsColor); }
  auto tuple() const { return std::tie(starsColor, shootingStarsColor); }
};

struct ShootingStar {
  float x;
  float y;
  float vx;
  float vy;
  float angle;
  float life; // in seconds
  float age;  // in seconds
  uint8_t color[3];
};

class Stars : public Animation<Stars, struct StarsParams> {
private:
  std::vector<ShootingStar> shooting_stars;
  double last_time = 0;

public:
  Stars(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    srand(time(0));
    table.resize(matrix->width() * matrix->height());
    for (int i = 0; i < matrix->width() * matrix->height(); ++i)
      if (rand() % 100 < 1)
        table[i] = rand() % 256;
      else
        table[i] = 0;
  }
  void animate(double time) override;

  std::string name() const override { return "Stars"; }

  // Override mode parameter methods
  void applyDefaultParameters() override {
    params_.starsColor.value = Color(230, 245, 255);
    params_.shootingStarsColor.value = Color(255, 255, 255);
  }

  void applyPresetParameters() override {
    // Preset: Blue stars with red shooting stars
    params_.starsColor.value = Color(100, 150, 255);
    params_.shootingStarsColor.value = Color(255, 100, 100);
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
  std::vector<uint8_t> table;
};
} // namespace animations

#endif // STARS_ANIMATION_H