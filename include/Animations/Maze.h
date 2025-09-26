#ifndef MAZE_ANIMATION_H
#define MAZE_ANIMATION_H

#include "Animation.h"
#include <cmath>
#include <random>
#include <utility>
#include <vector>

namespace animations {

class Maze : public Animation {
private:
  // Dimensions of the grid (use the canvas size)
  int w_{0}, h_{0};

  // Grid: 0 = wall (LED ON), 1 = path (LED OFF)
  std::vector<uint8_t> grid_;

  // Frontier mask and list (frontier walls considered by Prim's)
  std::vector<uint8_t> in_frontier_;
  std::vector<std::pair<int, int>> frontier_;

  // RNG and control
  std::mt19937 rng_{std::random_device{}()};
  bool initialized_{false};
  bool done_{false};

  // Animation pacing
  int steps_per_frame_{8}; // adjust to taste

  // Helpers
  inline int idx_(int x, int y) const { return y * w_ + x; }
  void init_();
  void addFrontierWalls_(int cx, int cy);
  void step_();
  void draw_();

public:
  Maze(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  };
  void animate(double time) override;

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // MAZE_ANIMATION_H