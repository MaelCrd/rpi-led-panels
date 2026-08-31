#ifndef GAME_OF_LIFE_ANIMATION_H
#define GAME_OF_LIFE_ANIMATION_H

#include <cmath>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "led-matrix.h"

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

struct GameOfLifeParams : ParameterSet<GameOfLifeParams> {
  // Provide tuple of references for iteration
  auto tuple() { return std::tie(color, speed); }
  auto tuple() const { return std::tie(color, speed); }

  // Add color parameter
  PARAM_COLOR(color, Color(255, 255, 255), "Color")
  PARAM_FLOAT(speed, 1.0, 0.05, 2.0, "Speed")
};

class GameOfLife : public Animation<GameOfLife, struct GameOfLifeParams> {
public:
  GameOfLife(rgb_matrix::RGBMatrix *matrix,
             const std::string &rules = "B3/S23");

  void animate(double time) override;

  std::string name() const override { return "Game of Life"; }

  // Override mode parameter methods
  void applyDefaultParameters() override {
    params_.color.value = Color(255, 255, 255);
    params_.speed.value = 1.0;
  }

  void applyPresetParameters() override {
    params_.color.value = Color(255, 0, 0);
    params_.speed.value = 1.0;
  }

private:
  void parseRules(const std::string &rules);

  // Add private members for the Game of Life grid state
  std::vector<std::vector<bool>> grid;
  std::vector<std::vector<bool>> nextGrid;
  uint16_t birthRulesBits = 0;
  uint16_t survivalRulesBits = 0;
  double lastTime = 0;
  int fps = 24;
};

} // namespace animations

#endif // GAME_OF_LIFE_ANIMATION_H