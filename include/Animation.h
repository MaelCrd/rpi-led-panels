#ifndef ANIMATION_H
#define ANIMATION_H

#include <cmath>
#include <mutex>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "led-matrix.h"
#include "parameters/param_system.hpp"

namespace animations {

// Animation mode enumeration
enum class AnimationMode {
  Default = 0, // Default parameters, fixed in code
  Preset = 1,  // Preset parameters, fixed in code
  Custom = 2   // User-provided parameters
};

// Non-templated base class for polymorphic usage
class BaseAnimation {
public:
  BaseAnimation(rgb_matrix::RGBMatrix *matrix)
      : matrix(matrix), mode_(AnimationMode::Default) {}
  virtual ~BaseAnimation() = default;

  virtual void animate(double time) = 0;
  virtual nlohmann::json parametersJson() const = 0;
  virtual bool setParameter(std::string_view internalName, double value) = 0;
  virtual bool setParameter(std::string_view internalName,
                            std::string_view colorValue) = 0;
  virtual std::string name() const = 0;

  // Mode management
  virtual void setMode(AnimationMode mode) {
    std::lock_guard<std::mutex> lock(params_mutex_);
    mode_ = mode;
    applyModeParameters();
  }
  AnimationMode getMode() const {
    std::lock_guard<std::mutex> lock(params_mutex_);
    return mode_;
  }

  // Virtual methods for mode-specific parameter handling
  virtual void applyDefaultParameters() = 0;
  virtual void applyPresetParameters() = 0;
  virtual void applyModeParameters() = 0;

protected:
  rgb_matrix::RGBMatrix *matrix;
  AnimationMode mode_;
  mutable std::mutex params_mutex_;
};

template <typename Derived, typename ParamStruct>
class Animation : public BaseAnimation {
public:
  using Params = ParamStruct;

  Animation(rgb_matrix::RGBMatrix *matrix) : BaseAnimation(matrix) {
    applyModeParameters(); // Initialize with default parameters
  }
  virtual ~Animation() = default;

  virtual void animate(double time) = 0;

  Params &params() { return params_; }
  Params const &params() const { return params_; }

  nlohmann::json parametersJson() const override {
    std::lock_guard<std::mutex> lock(params_mutex_);
    auto json = params_.toJson();
    json["mode"] = static_cast<int>(mode_);
    return json;
  }

  bool setParameter(std::string_view internalName, double value) override {
    std::lock_guard<std::mutex> lock(params_mutex_);
    bool ok = params_.setByName(internalName, value);
    if (ok) {
      // If we're setting a parameter manually, switch to Custom mode
      if (mode_ != AnimationMode::Custom) {
        mode_ = AnimationMode::Custom;
      }
      static_cast<Derived *>(const_cast<Animation *>(this))
          ->onParametersChanged();
    }
    return ok;
  }

  bool setParameter(std::string_view internalName,
                    std::string_view colorValue) override {
    std::lock_guard<std::mutex> lock(params_mutex_);
    bool ok = params_.setByName(internalName, colorValue);
    if (ok) {
      // If we're setting a parameter manually, switch to Custom mode
      if (mode_ != AnimationMode::Custom) {
        mode_ = AnimationMode::Custom;
      }
      static_cast<Derived *>(const_cast<Animation *>(this))
          ->onParametersChanged();
    }
    return ok;
  }

  // Override base class mode handling
  void setMode(AnimationMode mode) override {
    std::lock_guard<std::mutex> lock(params_mutex_);
    mode_ = mode;
    applyModeParameters();
    static_cast<Derived *>(const_cast<Animation *>(this))
        ->onParametersChanged();
  }

  // Implementation of mode parameter application
  void applyModeParameters() override {
    switch (mode_) {
    case AnimationMode::Default:
      applyDefaultParameters();
      break;
    case AnimationMode::Preset:
      applyPresetParameters();
      break;
    case AnimationMode::Custom:
      // Don't change parameters - they are user-controlled
      break;
    }
  }

  // Default implementations - derived classes should override these
  void applyDefaultParameters() override {}

  void applyPresetParameters() override {
    // Default implementation: same as default parameters
    // Derived classes should override this to provide meaningful presets
    applyDefaultParameters();
  }

  // Default hook
  void onParametersChanged() {}

protected:
  ParamStruct params_;
};

} // namespace animations

#endif // ANIMATION_H