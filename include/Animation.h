#ifndef ANIMATION_H
#define ANIMATION_H

#include "led-matrix.h"
#include "parameters/param_system.hpp"
#include <cmath>
#include <nlohmann/json.hpp>

namespace animations {

// Animation mode enumeration
enum class AnimationMode {
  Default = 0, // Default parameters, fixed in code
  Preset = 1,  // Preset parameters, fixed in code
  Custom = 2   // User-provided parameters
};

// Non-templated base class for polymorphic usage
class BaseAnimation {
protected:
  rgb_matrix::RGBMatrix *matrix;
  AnimationMode mode_;

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
    mode_ = mode;
    applyModeParameters();
  }
  AnimationMode getMode() const { return mode_; }

  // Virtual methods for mode-specific parameter handling
  virtual void applyDefaultParameters() = 0;
  virtual void applyPresetParameters() = 0;
  virtual void applyModeParameters() = 0;
};

template <typename Derived, typename ParamStruct>
class Animation : public BaseAnimation {
protected:
  ParamStruct params_;

public:
  Animation(rgb_matrix::RGBMatrix *matrix) : BaseAnimation(matrix) {
    applyModeParameters(); // Initialize with default parameters
  }
  virtual void animate(double time) = 0;

  using Params = ParamStruct;

  Params &params() { return params_; }
  Params const &params() const { return params_; }

  nlohmann::json parametersJson() const override {
    auto json = params_.toJson();
    json["mode"] = static_cast<int>(getMode());
    return json;
  }

  bool setParameter(std::string_view internalName, double value) override {
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
  void applyDefaultParameters() override {
    // Default implementation: parameters are already initialized by their
    // respective PARAM_* macros (e.g., PARAM_COLOR, PARAM_INT, PARAM_FLOAT)
    // with the correct default values during member initialization.
    // We don't need to do anything here - the derived class should override
    // this method if it wants to set specific default parameter values.
    // Doing nothing preserves the values set by the PARAM_* macro constructors.
  }

  void applyPresetParameters() override {
    // Default implementation: same as default parameters
    // Derived classes should override this to provide meaningful presets
    applyDefaultParameters();
  }

  // Default hook
  void onParametersChanged() {}
};

} // namespace animations

#endif // ANIMATION_H