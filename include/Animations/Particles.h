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
  float random_factor; // For random movement variations (0.0 to 1.0)
};

struct ParticlesParams : ParameterSet<ParticlesParams> {
  // Empty tuple for animations with no parameters
  PARAM_COLOR(color, Color(255, 255, 255), "Color")
  PARAM_FLOAT(fadeSpeed, 1.0f, 0.1, 3, "Fade speed")
  PARAM_INT(numParticles, 500, 1, 1000, "Number of particles")
  PARAM_INT(movementType, 2, 0, 3,
            "Movement type (0: random, 1: circular, 2: sine wave, 3: erratic)")

  // Provide tuple of references for iteration
  auto tuple() {
    return std::tie(color, fadeSpeed, numParticles, movementType);
  }
  auto const tuple() const {
    return std::tie(color, fadeSpeed, numParticles, movementType);
  }
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
    params_.fadeSpeed.value = 1.0f;
    params_.numParticles.value = 800;
    params_.movementType.value = 2;
  }

  void applyPresetParameters() override {
    params_.color.value = Color(255, 37, 0);
    params_.fadeSpeed.value = 1.0f;
    params_.numParticles.value = 800;
    params_.movementType.value = 0;
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // PARTICLES_ANIMATION_H