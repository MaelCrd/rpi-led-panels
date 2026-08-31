#ifndef HEIGHT_MAP_ANIMATION_H
#define HEIGHT_MAP_ANIMATION_H

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include <FastNoise/FastNoise.h>

#include "led-matrix.h"

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

struct HeightMapParams : ParameterSet<HeightMapParams> {
  // Provide tuple of references for iteration
  auto tuple() { return std::tie(style, color, speed); }
  auto const tuple() const { return std::tie(style, color, speed); }

  PARAM_INT(style, 1, 1, 5, "Style")
  PARAM_COLOR(color, Color(255, 0, 0), "Color")
  PARAM_FLOAT(speed, 1.0, 0.1, 3, "Speed")
};

class HeightMap : public Animation<HeightMap, struct HeightMapParams> {
public:
  HeightMap(rgb_matrix::RGBMatrix *matrix);

  void animate(double time) override;

  std::string name() const override { return "HeightMap"; }

  // Override mode parameter methods
  void applyDefaultParameters() override {
    params_.style.value = 1;
    params_.color.value = Color(255, 0, 0);
    params_.speed.value = 1.0;
  }

  void applyPresetParameters() override {
    params_.style.value = 4;
    params_.color.value = Color(255, 0, 0);
    params_.speed.value = 1.0;
  }

private:
  FastNoise::SmartNode<> heightMap;
  std::vector<float> newPixels;
  std::vector<float> xPos, yPos, zPos;
  std::array<std::array<uint8_t, 3>, 256> colorLookup;
  double last_time = 0;
  float timeZ = 99.0f / 5.0f;
};

} // namespace animations

#endif // HEIGHT_MAP_ANIMATION_H