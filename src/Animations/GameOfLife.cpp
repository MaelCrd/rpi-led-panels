#include <Animations/GameOfLife.h>
#include <cstdint>
#include <ctime>
#include <sstream>

namespace animations {

GameOfLife::GameOfLife(rgb_matrix::RGBMatrix *matrix, const std::string &rules)
    : Animation(matrix) {
  // Parse rules string (e.g., "B3/S23")
  parseRules(rules);

  // Initialize the Game of Life grid
  grid = new bool *[matrix->height()];
  nextGrid = new bool *[matrix->height()];
  for (int i = 0; i < matrix->height(); ++i) {
    grid[i] = new bool[matrix->width()];
    nextGrid[i] = new bool[matrix->width()];
  }

  // Example: Randomly initialize the grid
  for (int y = 0; y < matrix->height(); ++y) {
    for (int x = 0; x < matrix->width(); ++x) {
      grid[y][x] = (rand() % 2) == 0; // Randomly alive or dead
    }
  }
}

void GameOfLife::parseRules(const std::string &rules) {
  birthRulesBits = 0;
  survivalRulesBits = 0;

  std::istringstream iss(rules);
  std::string part;

  while (std::getline(iss, part, '/')) {
    if (part[0] == 'B') {
      for (size_t i = 1; i < part.length(); ++i) {
        if (isdigit(part[i])) {
          birthRulesBits |= (1 << (part[i] - '0'));
        }
      }
    } else if (part[0] == 'S') {
      for (size_t i = 1; i < part.length(); ++i) {
        if (isdigit(part[i])) {
          survivalRulesBits |= (1 << (part[i] - '0'));
        }
      }
    }
  }
}

void GameOfLife::animate(double time) {
  const int width = matrix->width();
  const int height = matrix->height();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int aliveNeighbors = 0;

      const int x_left = (x - 1 + width) % width;
      const int x_right = (x + 1) % width;
      const int y_up = (y - 1 + height) % height;
      const int y_down = (y + 1) % height;

      aliveNeighbors +=
          grid[y_up][x_left] + grid[y_up][x] + grid[y_up][x_right];
      aliveNeighbors += grid[y][x_left] + grid[y][x_right];
      aliveNeighbors +=
          grid[y_down][x_left] + grid[y_down][x] + grid[y_down][x_right];

      // Use bitwise operations for rule checking
      nextGrid[y][x] = grid[y][x] ? ((survivalRulesBits >> aliveNeighbors) & 1)
                                  : ((birthRulesBits >> aliveNeighbors) & 1);
    }
  }

  std::swap(grid, nextGrid);

  // auto canvas = matrix->CreateFrameCanvas();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint8_t color = grid[y][x] ? 255 : 0;
      matrix->SetPixel(x, y, color, color, color);
    }
  }

  // canvas = matrix->SwapOnVSync(canvas);
}

} // namespace animations
