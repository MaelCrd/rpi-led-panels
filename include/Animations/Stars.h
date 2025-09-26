#ifndef STARS_ANIMATION_H
#define STARS_ANIMATION_H

#include "Animation.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace animations {

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

class Stars : public Animation {
private:
  std::vector<ShootingStar> shooting_stars;
  double last_time = 0;

public:
  Stars(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    srand(time(0));
    table = new uint8_t[matrix->width() * matrix->height()];
    for (int i = 0; i < matrix->width() * matrix->height(); ++i)
      if (rand() % 100 < 1)
        table[i] = rand() % 256;
      else
        table[i] = 0;
  }
  void animate(double time) override;

protected:
  uint8_t stars_color[3] = {230, 245, 255};
  // std::vector<std::array<uint8_t, 3>> shooting_colors = {
  //     std::array<uint8_t, 3>{255, 255, 255}, std::array<uint8_t, 3>{255, 0,
  //     0}, std::array<uint8_t, 3>{150, 0, 255}, std::array<uint8_t, 3>{255,
  //     150, 0}, std::array<uint8_t, 3>{0, 180, 255}};
  std::vector<std::array<uint8_t, 3>> shooting_colors = {
      std::array<uint8_t, 3>{255, 255, 255}};
  rgb_matrix::FrameCanvas *offscreen_canvas;
  uint8_t *table;
};
} // namespace animations

#endif // STARS_ANIMATION_H