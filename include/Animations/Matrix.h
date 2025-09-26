#ifndef MATRIX_ANIMATION_H
#define MATRIX_ANIMATION_H

#include "Animation.h"
#include <cmath>

namespace animations {

class Matrix : public Animation {
private:
  int *positions;

public:
  Matrix(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    positions = new int[offscreen_canvas->width()];
    for (int i = 0; i < offscreen_canvas->width(); ++i)
      positions[i] = -1;
  };
  void animate(double time) override;

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // MATRIX_ANIMATION_H