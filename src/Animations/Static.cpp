#include "Animations/Static.h"
#include <cstdlib>
#include <unistd.h>

namespace animations {

void Static::animate(double time) {
  // if (time > 0)
  //   return;

  offscreen_canvas->Clear();

  for (int x = 0; x < matrix->width(); ++x) {
    for (int y = 0; y < matrix->height(); ++y) {
      offscreen_canvas->SetPixel(x, y, params_.color.value.r,
                                 params_.color.value.g, params_.color.value.b);
    }
  }
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
  usleep(1000000 / 20); // ~20 FPS
}

} // namespace animations