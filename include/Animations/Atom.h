#ifndef ATOM_ANIMATION_H
#define ATOM_ANIMATION_H

#include "Animation.h"
#include <cmath>

namespace animations {

class Atom : public Animation {
private:
public:
  Atom(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  };
  void animate(double time) override;

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // ATOM_ANIMATION_H