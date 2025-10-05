#ifndef GAME_OF_LIFE_ANIMATION_H
#define GAME_OF_LIFE_ANIMATION_H

#include "Animation.h"
#include "led-matrix.h"
#include "parameters/param_system.hpp"
#include <cmath>
#include <string>

namespace animations {

struct GameOfLifeParams : ParameterSet<GameOfLifeParams> {
  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }
};

class GameOfLife : public Animation<GameOfLife, struct GameOfLifeParams> {

public:
  GameOfLife(rgb_matrix::RGBMatrix *matrix,
             const std::string &rules = "B3/S23");
  void animate(double time) override;

  std::string name() const override { return "Game of Life"; }

  // Override mode parameter methods - GameOfLife has no parameters so all modes
  // are the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

protected:
private:
  // Add private members for the Game of Life grid state
  bool **grid;
  bool **nextGrid;
  uint16_t birthRulesBits = 0;
  uint16_t survivalRulesBits = 0;
  double lastTime = 0;
  int fps = 35;

  void parseRules(const std::string &rules);
};

} // namespace animations

#endif // GAME_OF_LIFE_ANIMATION_H