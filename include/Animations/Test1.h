#ifndef TEST1_ANIMATION_H
#define TEST1_ANIMATION_H

#include "Animation.h"
#include <cmath>

namespace animations {

class Test1 : public Animation {
private:
public:
  Test1(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  };
  void animate(double time) override;

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // TEST1_ANIMATION_H