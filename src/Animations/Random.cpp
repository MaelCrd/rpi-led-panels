#include "Animations/Random.h"

#include <cstdlib>
#include <iostream>

namespace animations {

Random::Random(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
  // // Initialize random parameters
  // randomAmplitude = 0.5f;
  // randomFrequency = 1.0f;
  // randomColorSpeed = 1.0f;
}

void Random::animate(double time) {

  // Generate random pixel values
  for (int x = 0; x < matrix->width(); ++x) {
    for (int y = 0; y < matrix->height(); ++y) {
      int r = rand() % 256;
      int g = rand() % 256;
      int b = rand() % 256;
      matrix->SetPixel(x, y, r, g, b);
    }
  }
}

} // namespace animations