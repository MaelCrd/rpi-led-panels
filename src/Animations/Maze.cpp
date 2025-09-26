#include "Animations/Maze.h"
#include "Utils.h"
#include "graphics.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <random>
#include <unistd.h>
#include <utility>
#include <vector>

namespace animations {

// Return true if (x,y) is a valid "cell" position (odd, inside border)
static inline bool is_cell(int x, int y, int w, int h) {
  return (x > 0 && x < w - 1 && y > 0 && y < h - 1 && (x & 1) == 1 &&
          (y & 1) == 1);
}

// Add walls between cell (cx,cy) and its neighbors two steps away to frontier.
void Maze::addFrontierWalls_(int cx, int cy) {
  static const int d2[4][2] = {{2, 0}, {-2, 0}, {0, 2}, {0, -2}};
  for (auto &d : d2) {
    int nx = cx + d[0];
    int ny = cy + d[1];
    int wx = cx + d[0] / 2; // wall between the two cells
    int wy = cy + d[1] / 2;

    if (nx <= 0 || nx >= w_ - 1 || ny <= 0 || ny >= h_ - 1)
      continue;
    if (!is_cell(nx, ny, w_, h_))
      continue;
    if (grid_[idx_(nx, ny)] != 0)
      continue; // neighbor already path; skip
    int wi = idx_(wx, wy);
    if (!in_frontier_[wi]) {
      in_frontier_[wi] = 1;
      frontier_.emplace_back(wx, wy);
    }
  }
}

// Perform one step of Randomized Prim's algorithm.
void Maze::step_() {
  if (frontier_.empty()) {
    done_ = true;
    return;
  }

  std::uniform_int_distribution<int> pick(
      0, static_cast<int>(frontier_.size()) - 1);
  int k = pick(rng_);
  auto wall = frontier_[k];
  // remove from frontier (swap-pop)
  frontier_[k] = frontier_.back();
  frontier_.pop_back();
  in_frontier_[idx_(wall.first, wall.second)] = 0;

  int wx = wall.first;
  int wy = wall.second;

  // Determine the two neighboring cells this wall separates.
  int c1x = -1, c1y = -1, c2x = -1, c2y = -1;
  if ((wx % 2 == 0) && (wy % 2 == 1)) {
    // horizontal wall between left and right cells
    c1x = wx - 1;
    c1y = wy;
    c2x = wx + 1;
    c2y = wy;
  } else if ((wx % 2 == 1) && (wy % 2 == 0)) {
    // vertical wall between top and bottom cells
    c1x = wx;
    c1y = wy - 1;
    c2x = wx;
    c2y = wy + 1;
  } else {
    // Not a proper separating wall; ignore.
    return;
  }

  // Validate cells
  auto valid_cell = [&](int x, int y) {
    return x >= 0 && x < w_ && y >= 0 && y < h_ && is_cell(x, y, w_, h_);
  };
  if (!valid_cell(c1x, c1y) || !valid_cell(c2x, c2y))
    return;

  bool v1 = grid_[idx_(c1x, c1y)] == 1;
  bool v2 = grid_[idx_(c2x, c2y)] == 1;

  // Carve only if the wall separates exactly one visited cell and one unvisited
  // cell.
  if (v1 ^ v2) {
    // Turn wall into path
    grid_[idx_(wx, wy)] = 1;

    // Carve the unvisited cell
    int nx = v1 ? c2x : c1x;
    int ny = v1 ? c2y : c1y;
    grid_[idx_(nx, ny)] = 1;

    // Add new frontier walls around the newly carved cell
    addFrontierWalls_(nx, ny);
  }
}

// Initialize the maze state.
void Maze::init_() {
  w_ = offscreen_canvas->width() / 2;
  h_ = offscreen_canvas->height() / 2;

  grid_.assign(w_ * h_, 0); // start with all walls
  in_frontier_.assign(w_ * h_, 0);
  frontier_.clear();
  done_ = false;

  // Find a random starting cell at odd coordinates (1..w-2, 1..h-2).
  auto rand_odd = [&](int limit) -> int {
    if (limit < 3)
      return -1;
    int count = (limit - 2 + 1) / 2; // number of odd values in [1, limit-2]
    std::uniform_int_distribution<int> dist(0, count - 1);
    return 1 + 2 * dist(rng_);
  };

  int sx = rand_odd(w_);
  int sy = rand_odd(h_);
  if (sx < 0 || sy < 0) {
    // Panel too small to build a maze with walls; mark done.
    done_ = true;
    initialized_ = true;
    return;
  }

  grid_[idx_(sx, sy)] = 1; // starting cell becomes path
  addFrontierWalls_(sx, sy);

  initialized_ = true;
}

// Draw the current grid to the LED panel.
void Maze::draw_() {
  // Colors
  const uint8_t wall_r = 255, wall_g = 0, wall_b = 0; // walls
  const uint8_t path_r = 0, path_g = 0, path_b = 0;   // paths
  const uint8_t frontier_r = 255, frontier_g = 255,
                frontier_b = 255; // frontier highlight

  offscreen_canvas->Fill(wall_r, wall_g, wall_b);
  // offscreen_canvas->Clear();

  // Full redraw
  for (int y = 0; y < h_; ++y) {
    for (int x = 0; x < w_; ++x) {
      bool is_path = grid_[idx_(x, y)] == 1;
      if (is_path) {
        offscreen_canvas->SetPixel(2 * x + 1, 2 * y + 1, path_g, path_g,
                                   path_b);
        offscreen_canvas->SetPixel(2 * x + 2, 2 * y + 1, path_g, path_g,
                                   path_b);
        offscreen_canvas->SetPixel(2 * x + 1, 2 * y + 2, path_g, path_g,
                                   path_b);
        offscreen_canvas->SetPixel(2 * x + 2, 2 * y + 2, path_g, path_g,
                                   path_b);
      } else {
        offscreen_canvas->SetPixel(2 * x + 1, 2 * y + 1, wall_r, wall_g,
                                   wall_b);
        offscreen_canvas->SetPixel(2 * x + 2, 2 * y + 1, wall_r, wall_g,
                                   wall_b);
        offscreen_canvas->SetPixel(2 * x + 1, 2 * y + 2, wall_r, wall_g,
                                   wall_b);
        offscreen_canvas->SetPixel(2 * x + 2, 2 * y + 2, wall_r, wall_g,
                                   wall_b);
      }
    }
  }

  // Overlay frontier (only during generation)
  if (!done_) {
    for (const auto &p : frontier_) {
      int x = p.first, y = p.second;
      // Only paint if it's still a wall
      if (grid_[idx_(x, y)] == 0) {
        offscreen_canvas->SetPixel(2 * x + 1, 2 * y + 1, frontier_r, frontier_g,
                                   frontier_b);
        offscreen_canvas->SetPixel(2 * x + 2, 2 * y + 1, frontier_r, frontier_g,
                                   frontier_b);
        offscreen_canvas->SetPixel(2 * x + 1, 2 * y + 2, frontier_r, frontier_g,
                                   frontier_b);
        offscreen_canvas->SetPixel(2 * x + 2, 2 * y + 2, frontier_r, frontier_g,
                                   frontier_b);
      }
    }
  }

  // Draw start and end points
  if (done_) {
    int cw = offscreen_canvas->width();
    int ch = offscreen_canvas->height();
    auto color = rgb_matrix::Color(0, 0, 0);
    rgb_matrix::DrawLine(offscreen_canvas, 3, 0, 3, 3, color);
    rgb_matrix::DrawLine(offscreen_canvas, 4, 0, 4, 3, color);
    rgb_matrix::DrawLine(offscreen_canvas, cw - 4, ch - 4, cw - 4, ch - 1,
                         color);
    rgb_matrix::DrawLine(offscreen_canvas, cw - 5, ch - 4, cw - 5, ch - 1,
                         color);
  }
}

void Maze::animate(double time) {
  if (!initialized_)
    init_();

  // Advance a few steps per frame to visualize growth
  if (!done_) {
    for (int i = 0; i < steps_per_frame_ && !done_; ++i) {
      step_();
    }
  }

  draw_();
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations