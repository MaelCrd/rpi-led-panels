#ifndef GAME_OF_LIFE_ANIMATION_H
#define GAME_OF_LIFE_ANIMATION_H

#include "Animation.h"
#include "led-matrix.h"
#include <cmath>
#include <string>

namespace animations {

class GameOfLife : public Animation {

public:
  GameOfLife(rgb_matrix::RGBMatrix *matrix,
             const std::string &rules = "B3/S23");
  void animate(double time) override;

protected:
private:
  // Add private members for the Game of Life grid state
  bool **grid;
  bool **nextGrid;
  uint16_t birthRulesBits = 0;
  uint16_t survivalRulesBits = 0;

  void parseRules(const std::string &rules);
};

} // namespace animations

#endif // GAME_OF_LIFE_ANIMATION_H