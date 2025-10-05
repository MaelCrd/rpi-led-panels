#ifndef HEIGHT_MAP_ANIMATION_H
#define HEIGHT_MAP_ANIMATION_H

#include "Animation.h"
#include "led-matrix.h"
#include "parameters/param_system.hpp"
#include <cmath>

#include <FastNoise/FastNoise.h>

namespace animations {

struct HeightMapParams : ParameterSet<HeightMapParams> {
  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }
};

class HeightMap : public Animation<HeightMap, struct HeightMapParams> {
private:
  FastNoise::SmartNode<> heightMap;
  float *newPixels;
  float *xPos, *yPos, *zPos;
  std::array<std::array<uint8_t, 3>, 256> colorLookup;

public:
  HeightMap(rgb_matrix::RGBMatrix *matrix);
  void animate(double time) override;

  std::string name() const override { return "HeightMap"; }

  // Override mode parameter methods - HeightMap has no parameters so all modes
  // are the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

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