#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

#include "led-matrix.h"
#include <cmath>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Forward declaration
namespace animations {
class BaseAnimation;
}

class AnimationManager {
private:
  int current_animation_index;
  std::mutex mtx_current_animation;
  int brightness = 100;
  std::mutex mtx_brightness;
  std::vector<animations::BaseAnimation *> animations;
  std::vector<std::string> animationNames;
  bool animations_initialized = false;

public:
  AnimationManager(rgb_matrix::RGBMatrix *matrix)
      : current_animation_index(12), matrix(matrix) {}

  ~AnimationManager();

  void initAnimations();
  void run(volatile int *interrupt_received);

  void setCurrentAnimation(int v) {
    std::lock_guard<std::mutex> lock(mtx_current_animation);
    current_animation_index = v;
  }
  int getCurrentAnimation() {
    std::lock_guard<std::mutex> lock(mtx_current_animation);
    return current_animation_index;
  }

  void setBrightness(int v) {
    std::lock_guard<std::mutex> lock(mtx_brightness);
    brightness = v;
  }
  int getBrightness() {
    std::lock_guard<std::mutex> lock(mtx_brightness);
    return brightness;
  }

  nlohmann::json getAllAnimations();

  // Mode management methods
  bool setAnimationMode(int animationId, int mode);
  int getAnimationModeInt(int animationId);

  // Parameter management methods
  bool setAnimationParameters(int animationId,
                              const nlohmann::json &parameters);

protected:
  rgb_matrix::RGBMatrix *matrix;
};

#endif // ANIMATIONMANAGER_H