#ifndef PARTY_ANIMATION_H
#define PARTY_ANIMATION_H

#include "Animation.h"
#include "graphics.h"
#include "led-matrix.h"
#include <cmath>
#include <cstdint>

namespace animations {

class Party : public Animation {
private:
public:
  Party(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  }
  void animate(double time) override;

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
  uint8_t index = 0;
  rgb_matrix::Color colors[2] = {
      // rgb_matrix::Color(255, 0, 0),    // Red
      // rgb_matrix::Color(0, 255, 0),    // Green
      // rgb_matrix::Color(0, 0, 255),    // Blue
      // rgb_matrix::Color(255, 255, 0),  // Yellow
      // rgb_matrix::Color(0, 255, 255),  // Cyan
      // rgb_matrix::Color(255, 0, 255),  // Magenta
      rgb_matrix::Color(255, 255, 255), // White
      rgb_matrix::Color(0, 0, 0)        // Black
  };
};
} // namespace animations

#endif // PARTY_ANIMATION_H