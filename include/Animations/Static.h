#ifndef STATIC_ANIMATION_H
#define STATIC_ANIMATION_H

#include "Animation.h"
#include <cmath>

namespace animations {

class Static : public Animation {
private:
public:
  Static(rgb_matrix::RGBMatrix *matrix);
  void animate(double time) override;

protected:
  int color[3]{0, 0, 0};
};
} // namespace animations

#endif // STATIC_ANIMATION_H