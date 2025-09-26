#ifndef ANIMATION_H
#define ANIMATION_H

#include "led-matrix.h"
#include <cmath>

namespace animations {

class Animation {
private:
public:
  Animation(rgb_matrix::RGBMatrix *matrix) : matrix(matrix) {}
  virtual void animate(double time) = 0;

protected:
  rgb_matrix::RGBMatrix *matrix;
};

} // namespace animations

#endif // ANIMATION_H