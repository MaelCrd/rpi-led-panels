#ifndef DROPLETCIRCLES_ANIMATION_H
#define DROPLETCIRCLES_ANIMATION_H

#include "Animation.h"
#include <cmath>

namespace animations {

struct Droplet {
  float x;
  float y;
  float radius;
  uint8_t color[3];
};

class DropletCircles : public Animation {
private:
  std::vector<Droplet> droplets;

public:
  DropletCircles(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    srand(time(0));
    offscreen_canvas = matrix->CreateFrameCanvas();
  }
  void animate(double time) override;

protected:
  int color[3]{0, 0, 0};
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // DROPLETCIRCLES_ANIMATION_H