#ifndef CLOCK_ANIMATION_H
#define CLOCK_ANIMATION_H

#include "Animation.h"
#include "led-matrix.h"
#include <cmath>

namespace animations {

class Clock : public Animation {
private:
public:
  Clock(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    font.LoadFont("../deps/matrix/fonts/6x12.bdf");
  };
  void animate(double time) override;

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
  rgb_matrix::Font font;
};
} // namespace animations

#endif // CLOCK_ANIMATION_H