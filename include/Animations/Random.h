#ifndef RANDOM_ANIMATION_H
#define RANDOM_ANIMATION_H

#include "Animation.h"
#include <cmath>

namespace animations {

class Random : public Animation {
private:
public:
  Random(rgb_matrix::RGBMatrix *matrix);
  void animate(double time) override;

protected:
};

} // namespace animations

#endif // RANDOM_ANIMATION_H