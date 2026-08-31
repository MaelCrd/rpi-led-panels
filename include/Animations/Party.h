#ifndef PARTY_ANIMATION_H
#define PARTY_ANIMATION_H

#include <cmath>
#include <cstdint>
#include <string>
#include <tuple>

#include "graphics.h"
#include "led-matrix.h"

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

struct PartyParams : ParameterSet<PartyParams> {
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }

  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
};

class Party : public Animation<Party, struct PartyParams> {
public:
  Party(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  }

  void animate(double time) override;

  std::string name() const override { return "Party"; }

  // Override mode parameter methods - Party has no parameters so all modes are
  // the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

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