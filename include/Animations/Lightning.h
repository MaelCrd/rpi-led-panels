#ifndef LIGHTNING_ANIMATION_H
#define LIGHTNING_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>
#include <cstdint>

namespace animations {

struct LightningParams : ParameterSet<LightningParams> {
  PARAM_COLOR(color, Color(255, 255, 255), "Color")
  PARAM_FLOAT(probability, 0.2, 0, 1, "Probability")

  auto tuple() { return std::tie(color, probability); }
  auto tuple() const { return std::tie(color, probability); }
};

struct LightningBranch {
  std::vector<std::pair<int, int>> points;
  bool is_main_branch = false;
  bool active = true;
  int direction =
      0; // -1 for left tendency, +1 for right tendency, 0 for neutral
  int growth_counter = 0; // Counter for slower growth of side branches
};

struct LightningBolt {
  std::vector<LightningBranch> branches;
  bool struck = false;
  float decay = 1.0f;
  int growth_index = 0; // How far the lightning has grown
};

class Lightning : public Animation<Lightning, struct LightningParams> {
private:
  double last_time = 0.0;
  std::vector<LightningBolt> bolts;
  std::vector<uint8_t> pixel_buffer; // Buffer to store pixel brightness values

public:
  Lightning(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    srand(time(0));
    offscreen_canvas = matrix->CreateFrameCanvas();
    pixel_buffer.resize(matrix->width() * matrix->height(), 0);
  }

  ~Lightning() {}

  void animate(double time) override;

  std::string name() const override { return "Lightning"; }
  void applyDefaultParameters() override {
    params_.color.value = {255, 255, 255};
    params_.probability.value = 0.2;
  }

  void applyPresetParameters() override {
    params_.color.value = {255, 0, 0};
    params_.probability.value = 0.5;
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // LIGHTNING_ANIMATION_H