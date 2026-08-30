#ifndef GAME_OF_LIFE_ANIMATION_H
#define GAME_OF_LIFE_ANIMATION_H

#include "Animation.h"
#include "led-matrix.h"
#include "parameters/param_system.hpp"
#include <cmath>
#include <string>

namespace animations {

struct GameOfLifeParams : ParameterSet<GameOfLifeParams> {
  // Add color parameter
  PARAM_COLOR(color, Color(255, 255, 255), "Color")
  PARAM_FLOAT(speed, 1.0, 0.05, 2.0, "Speed")

  // Provide tuple of references for iteration
  auto tuple() { return std::tie(color, speed); }
  auto tuple() const { return std::tie(color, speed); }
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
    params_.color.value = Color(255, 255, 255);
    params_.speed.value = 1.0;
  }

  void applyPresetParameters() override {
    params_.color.value = Color(255, 0, 0);
    params_.speed.value = 1.0;
  }

  ~GameOfLife() override {
    // Clean up dynamically allocated memory for the grid
    if (grid) {
      for (int i = 0; i < matrix->height(); ++i) {
        delete[] grid[i];
      }
      delete[] grid;
    }
    if (nextGrid) {
      for (int i = 0; i < matrix->height(); ++i) {
        delete[] nextGrid[i];
      }
      delete[] nextGrid;
    }
  }

protected:
private:
  // Add private members for the Game of Life grid state
  bool **grid;
  bool **nextGrid;
  uint16_t birthRulesBits = 0;
  uint16_t survivalRulesBits = 0;
  double lastTime = 0;
  int fps = 24;

  void parseRules(const std::string &rules);
};

} // namespace animations

#endif // GAME_OF_LIFE_ANIMATION_H