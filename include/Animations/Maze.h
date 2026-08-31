#ifndef MAZE_ANIMATION_H
#define MAZE_ANIMATION_H

#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Animation.h"
#include "parameters/param_system.hpp"

namespace animations {

struct MazeParams : ParameterSet<MazeParams> {
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }

  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
};

class Maze : public Animation<Maze, struct MazeParams> {
public:
  Maze(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
  }

  void animate(double time) override;

  std::string name() const override { return "Maze"; }

  // Override mode parameter methods - Maze has no parameters so all modes are
  // the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;

private:
  // Helpers
  inline int idx_(int x, int y) const { return y * w_ + x; }
  void init_();
  void addFrontierWalls_(int cx, int cy);
  void step_();
  void draw_();

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
};
} // namespace animations

#endif // MAZE_ANIMATION_H