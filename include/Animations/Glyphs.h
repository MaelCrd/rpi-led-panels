#ifndef GLYPHS_ANIMATION_H
#define GLYPHS_ANIMATION_H

#include <cmath>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include "Animation.h"
#include "parameters/param_system.hpp"

#ifndef ASSETS_DIR
#define ASSETS_DIR ".."
#endif

namespace animations {

inline std::string getRandomSCChar() {
  static std::mt19937 gen(std::random_device{}());
  // Common CJK Unified Ideographs range: 0x4E00 to 0x9FA5
  static std::uniform_int_distribution<char32_t> dist(0x4E00, 0x9FA5);

  char32_t cp = dist(gen);

  // Encode 3-byte UTF-8 code point to std::string
  std::string s;
  s += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
  s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
  s += static_cast<char>(0x80 | (cp & 0x3F));
  return s;
}

struct GlyphChar {
  std::string character;
  Color color;
};

struct GlyphsParams : ParameterSet<GlyphsParams> {
  // Provide tuple of references for iteration
  auto tuple() {
    return std::tie(color, stableColor, fadeSpeed, spawnChance, jitterChance);
  }
  auto const tuple() const {
    return std::tie(color, stableColor, fadeSpeed, spawnChance, jitterChance);
  }

  // Parameter definitions for the Glyphs animation
  PARAM_COLOR(color, Color(255, 0, 0), "Color")
  PARAM_COLOR(stableColor, Color(255, 255, 255), "Stable color")
  PARAM_FLOAT(fadeSpeed, 1.0f, 0.1, 3, "Fade speed")
  PARAM_FLOAT(spawnChance, 1.0f, 0.01, 3, "Spawn chance")
  PARAM_FLOAT(jitterChance, 1.0f, 0.0, 5, "Jitter chance")
};

class Glyphs : public Animation<Glyphs, struct GlyphsParams> {
public:
  Glyphs(rgb_matrix::RGBMatrix *matrix) : Animation(matrix) {
    offscreen_canvas = matrix->CreateFrameCanvas();
    font.LoadFont(ASSETS_DIR "/fonts/NotoSansSC-10.bdf"); // 11 lines, 25 cols
    grid.resize(11);
    for (size_t i = 0; i < grid.size(); ++i) {
      grid[i].resize(25);
      for (size_t j = 0; j < grid[i].size(); ++j) {
        grid[i][j] = GlyphChar{getRandomSCChar(), params_.color.value};
      }
    }
  }

  void animate(double time) override;

  std::string name() const override { return "Glyphs"; }

  // Override mode parameter methods - Glyphs has no parameters so all modes are
  // the same
  void applyDefaultParameters() override {
    params_.color.value = Color(255, 0, 0);
    params_.stableColor.value = Color(255, 255, 255);
    params_.fadeSpeed.value = 0.4f;
    params_.spawnChance.value = 1.0f;
    params_.jitterChance.value = 1.0f;
  }

  void applyPresetParameters() override {
    params_.color.value = Color(255, 47, 0);
    params_.stableColor.value = Color(5, 5, 5);
    params_.fadeSpeed.value = 0.48f;
    params_.spawnChance.value = 0.46f;
    params_.jitterChance.value = 1.09f;
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;

private:
  rgb_matrix::Font font;
  std::vector<std::vector<GlyphChar>> grid;
  double last_time = 0;
};
} // namespace animations

#endif // GLYPHS_ANIMATION_H