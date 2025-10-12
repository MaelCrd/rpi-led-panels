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
  int target_brightness = 100; // Target brightness for smooth transitions
  float current_brightness =
      100.0f; // Current brightness (for smooth interpolation)
  mutable std::mutex mtx_brightness;
  bool state = true;        // ON/OFF state of the animation manager
  bool target_state = true; // Target state for smooth transitions
  mutable std::mutex mtx_state;
  std::vector<animations::BaseAnimation *> animations;
  std::vector<std::string> animationNames;
  bool animations_initialized = false;

  // Transition parameters
  float fade_speed = 100.0f; // Brightness units per second for fade transitions

public:
  AnimationManager(rgb_matrix::RGBMatrix *matrix)
      : current_animation_index(-1), matrix(matrix), target_brightness(100),
        current_brightness(100.0f), target_state(true) {
    initAnimations();
  }

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
    target_brightness = v;
  }
  int getBrightness() {
    std::lock_guard<std::mutex> lock(mtx_brightness);
    return target_brightness;
  }

  void setState(bool v) {
    std::lock_guard<std::mutex> lock(mtx_state);
    target_state = v;
  }
  bool getState() {
    std::lock_guard<std::mutex> lock(mtx_state);
    return target_state;
  }

  nlohmann::json getAllAnimations();

  // Mode management methods
  bool setAnimationMode(int animationId, int mode);
  int getAnimationModeInt(int animationId);

  // Parameter management methods
  bool setAnimationParameters(int animationId,
                              const nlohmann::json &parameters);

  // Image animation specific methods
  bool setImageData(int animationId, const std::vector<uint8_t> &data,
                    int width, int height);
  bool setImageDataBase64(int animationId, const std::string &base64_data,
                          int width, int height);

private:
  // Helper methods for smooth transitions
  void updateBrightnessTransition(double deltaTime);
  bool isInTransition() const;

protected:
  rgb_matrix::RGBMatrix *matrix;
};

#endif // ANIMATIONMANAGER_H