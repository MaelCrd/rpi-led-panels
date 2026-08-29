#ifndef PARTICLES_ANIMATION_H
#define PARTICLES_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>

namespace animations {

struct Particle {
  float x;
  float y;
  float strength;
  float vx, vy;
  Color color;
  std::vector<std::tuple<int, int>> trail;
};

struct ParticlesParams : ParameterSet<ParticlesParams> {
  // Empty tuple for animations with no parameters
  PARAM_COLOR(color, Color(255, 45, 0), "Color")

  // Provide tuple of references for iteration
  auto tuple() { return std::tie(color); }
  auto const tuple() const { return std::tie(color); }
};

class Particles : public Animation<Particles, struct ParticlesParams> {
private:
  std::vector<Particle> particles;
  double last_time = 0;
  std::vector<std::vector<Color>> grid;

public:
  Particles(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    // Initialize grid with the size of the matrix
    grid.resize(offscreen_canvas->height(),
                std::vector<Color>(offscreen_canvas->width(), Color(0, 0, 0)));
  };
  void animate(double time) override;

  std::string name() const override { return "Particles"; }

  // Override mode parameter methods - Static has no parameters so all modes are
  // the same
  void applyDefaultParameters() override {
    params_.color.value = Color(255, 255, 255);
  }

  void applyPresetParameters() override {
    params_.color.value = Color(255, 0, 0);
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // PARTICLES_ANIMATION_H