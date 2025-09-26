#ifndef BIRDFLOCK_ANIMATION_H
#define BIRDFLOCK_ANIMATION_H

#include "Animation.h"
#include <cmath>

namespace animations {

class BirdFlock : public Animation {
private:
public:
  BirdFlock(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  };
  void animate(double time) override;

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // BIRDFLOCK_ANIMATION_H