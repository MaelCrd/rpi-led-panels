#include "Animations/Party.h"

#include <cstdint>
#include <cstdlib>
#include <unistd.h>

namespace animations {

void Party::animate(double /*time*/) {
  // int index = static_cast<int>(time * 20) % 7; // Change color every 0.5
  // seconds
  index = (index + 1) % 2;
  offscreen_canvas->Clear();
  offscreen_canvas->Fill(uint8_t(colors[index].r), uint8_t(colors[index].g),
                         uint8_t(colors[index].b));
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
  usleep(50000);
}

} // namespace animations
