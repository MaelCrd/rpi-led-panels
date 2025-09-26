#ifndef HEIGHT_MAP_ANIMATION_H
#define HEIGHT_MAP_ANIMATION_H

#include "Animation.h"
#include "led-matrix.h"
#include <cmath>

#include <FastNoise/FastNoise.h>

namespace animations {

class HeightMap : public Animation {
private:
  FastNoise::SmartNode<> heightMap;
  float *newPixels;
  float *xPos, *yPos, *zPos;
  std::array<std::array<uint8_t, 3>, 256> colorLookup;

public:
  HeightMap(rgb_matrix::RGBMatrix *matrix);
  void animate(double time) override;

  ~HeightMap() {
    delete[] newPixels;
    delete[] xPos;
    delete[] yPos;
    delete[] zPos;
  }

protected:
};

} // namespace animations

#endif // HEIGHT_MAP_ANIMATION_H