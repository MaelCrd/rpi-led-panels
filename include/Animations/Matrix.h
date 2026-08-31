#ifndef MATRIX_ANIMATION_H
#define MATRIX_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>

namespace animations {

struct MatrixParams : ParameterSet<MatrixParams> {
  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }
};

class Matrix : public Animation<Matrix, struct MatrixParams> {
private:
  std::vector<int> positions;
  double last_time = 0.0;

public:
  Matrix(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    positions.resize(offscreen_canvas->width());
    for (int i = 0; i < offscreen_canvas->width(); ++i)
      positions[i] = -1;
  };
  void animate(double time) override;

  std::string name() const override { return "Matrix"; }

  // Override mode parameter methods - Matrix has no parameters so all modes are
  // the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // MATRIX_ANIMATION_H