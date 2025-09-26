#include "Animations/Static.h"
#include <cstdlib>

namespace animations {

Static::Static(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
  srand(time(0));
  // color[0] = rand() % 256;
  // color[1] = rand() % 256;
  // color[2] = rand() % 256;
  color[0] = 255;
  color[1] = 255;
  color[2] = 255;
}

void Static::animate(double time) {
  if (time > 0)
    return;
  // Generate random pixel values
  for (int x = 0; x < matrix->width(); ++x) {
    for (int y = 0; y < matrix->height(); ++y) {
      matrix->SetPixel(x, y, color[0], color[1], color[2]);
    }
  }
}

} // namespace animations