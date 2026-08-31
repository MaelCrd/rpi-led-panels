#include "AnimationManager.h"

#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <net/if.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

#include "graphics.h"

#include "Animation.h"
#include "Animations/Atom.h"
#include "Animations/BirdFlock.h"
#include "Animations/ChristmasTree.h"
#include "Animations/Clock.h"
#include "Animations/DropletCircles.h"
#include "Animations/FlowTubes.h"
#include "Animations/GameOfLife.h"
#include "Animations/Glyphs.h"
#include "Animations/HeightMap.h"
#include "Animations/Image.h"
#include "Animations/Lightning.h"
#include "Animations/MagneticField.h"
#include "Animations/Matrix.h"
#include "Animations/Maze.h"
#include "Animations/Particles.h"
#include "Animations/Party.h"
#include "Animations/Random.h"
#include "Animations/Spheres.h"
#include "Animations/Splash.h"
#include "Animations/SpotifyCurrent.h"
#include "Animations/Stars.h"
#include "Animations/Static.h"
#include "Animations/Test1.h"
#include "Animations/Waves.h"
#include "QRCode.h"

using namespace animations;

AnimationManager::AnimationManager(rgb_matrix::RGBMatrix *matrix)
    : current_animation_index(-1), matrix(matrix), target_brightness(100),
      current_brightness(100.0f), target_state(true) {
  initAnimations();
}

AnimationManager::~AnimationManager() = default;

void AnimationManager::initAnimations() {
  if (animations_initialized) {
    return; // Already initialized
  }

  // Create animations vector
  animations.push_back(std::make_unique<Random>(matrix));
  animations.push_back(std::make_unique<HeightMap>(matrix));
  animations.push_back(std::make_unique<GameOfLife>(matrix, "B3/S23"));
  animations.push_back(std::make_unique<Static>(matrix));
  animations.push_back(std::make_unique<Clock>(matrix));
  animations.push_back(std::make_unique<Party>(matrix));
  animations.push_back(std::make_unique<Test1>(matrix));
  animations.push_back(std::make_unique<Stars>(matrix));
  animations.push_back(std::make_unique<DropletCircles>(matrix));
  animations.push_back(std::make_unique<Matrix>(matrix));
  animations.push_back(std::make_unique<Maze>(matrix));
  animations.push_back(std::make_unique<Atom>(matrix));
  animations.push_back(std::make_unique<BirdFlock>(matrix));
  animations.push_back(std::make_unique<Image>(matrix));
  animations.push_back(std::make_unique<Spheres>(matrix));
  animations.push_back(std::make_unique<Waves>(matrix));
  animations.push_back(std::make_unique<Lightning>(matrix));
  animations.push_back(std::make_unique<ChristmasTree>(matrix));
  animations.push_back(std::make_unique<SpotifyCurrent>(matrix));
  animations.push_back(std::make_unique<Splash>(matrix));
  animations.push_back(std::make_unique<MagneticField>(matrix));
  animations.push_back(std::make_unique<FlowTubes>(matrix));
  animations.push_back(std::make_unique<Glyphs>(matrix));
  animations.push_back(std::make_unique<Particles>(matrix));

  // Animation names corresponding to the order above
  animationNames = {};
  for (const auto &anim : animations) {
    animationNames.push_back(anim->name());
  }

  animations_initialized = true;
}

nlohmann::json AnimationManager::getAllAnimations() {
  // Initialize animations if not already done
  if (!animations_initialized) {
    initAnimations();
    std::cout << "SHOULD NOT HAPPEN" << std::endl;
  }

  nlohmann::json result;
  result["animations"] = nlohmann::json::array();

  for (size_t i = 0; i < animations.size(); ++i) {
    nlohmann::json animInfo;
    animInfo["id"] = static_cast<int>(i);
    animInfo["name"] = animationNames[i];
    animInfo["parameters"] = animations[i]->parametersJson();
    result["animations"].push_back(animInfo);
  }

  return result;
}

void AnimationManager::run(std::atomic<int> *interrupt_received) {
  // Implementation of the run method

  // Initialize animations if not already done
  if (!animations_initialized) {
    initAnimations();
  }

  /////
  // test QR code generation
  if (false) {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    char ip[INET_ADDRSTRLEN] = {0};
    if (fd >= 0) {
      struct ifreq ifr{};
      strncpy(ifr.ifr_name, "wlan0",
              sizeof(ifr.ifr_name) -
                  1); // Change interface name as needed (eth0, wlan0, etc.)
      ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
      if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
        auto *addr = reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
        inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
      }
      close(fd);
    }

    std::cout << "IP Address: " << ip << std::endl;

    std::string text = "http://" + std::string(ip);
    // std::string text = "http://pi.local";
    std::vector<std::vector<bool>> qr;
    generateQR(text, qr);

    // Display the QR code on the LED matrix
    int qr_size = qr.size();
    int module_size = 3;    // Size of each QR code module in pixels
    qr_size *= module_size; // Scale the QR code size
    int offset_x = (matrix->width() - qr_size) / 2;
    int offset_y = (matrix->height() - qr_size) / 2;
    for (int y = 0; y < qr_size; ++y) {
      for (int x = 0; x < qr_size; ++x) {
        int qr_x = x / module_size;
        int qr_y = y / module_size;
        if (qr_x < qr.size() && qr_y < qr.size() && qr[qr_y][qr_x]) {
          matrix->SetPixel(x + offset_x, y + offset_y, 255, 255, 255); // White
        } else {
          matrix->SetPixel(x + offset_x, y + offset_y, 0, 0, 0); // Black
        }
      }
    }

    while (true) {
      if (*interrupt_received)
        break;
      usleep(1000);
    }

    return;
  }

  setCurrentAnimation(animations.size() - 1); // Start with last animation

  /////
  int anim_index = getCurrentAnimation();
  int last_index = anim_index;
  BaseAnimation *current_animation =
      animations[anim_index % animations.size()].get();
  float animation_fade_duration = 1.0; // seconds for animation transitions

  auto start = std::chrono::system_clock::now();
  int frame_count = 0;
  double time = 0;
  auto last = start;

  // Loop forever, animating the random animation.
  while (true) {
    if (*interrupt_received)
      break;

    // Check if we should render animation (even when transitioning off, we
    // continue animation)
    bool should_animate = target_state || current_brightness > 0.1f;

    if (!should_animate) {
      // Animation is OFF and we've faded out completely - clear the matrix and
      // wait
      matrix->Clear();
      usleep(30000); // Sleep for 30ms before checking again
      // Update timing to prevent time jump when turning back on
      last = std::chrono::system_clock::now();
      continue;
    }

    auto now = std::chrono::system_clock::now();
    double deltaTime = std::chrono::duration<double>(now - last).count();
    last = now;

    // Update smooth brightness transitions
    updateBrightnessTransition(deltaTime);

    // Check for animation changes first (even when display is off)
    anim_index = getCurrentAnimation();
    if (anim_index != last_index) {
      // Animation change requested
      BaseAnimation *next_animation =
          animations[anim_index % animations.size()].get();

      // Check if we should render animation for fade transition
      bool should_do_fade_transition =
          target_state || current_brightness > 0.1f;

      if (should_do_fade_transition) {
        // Display is on - do fancy fade transition
        auto transition_start = std::chrono::system_clock::now();
        auto transition_last = transition_start;
        bool first_loop = true;

        // Fade out current animation and then fade in next animation
        while (true) {
          if (*interrupt_received)
            break;

          auto transition_now = std::chrono::system_clock::now();
          double transition_deltaTime =
              std::chrono::duration<double>(transition_now - transition_last)
                  .count();
          if (first_loop) {
            // On first loop, use the main loop deltaTime to avoid time jump
            transition_deltaTime = deltaTime;
            first_loop = false;
          }
          transition_last = transition_now;

          // Update brightness transitions during animation change
          updateBrightnessTransition(transition_deltaTime);

          std::chrono::duration<double> elapsed =
              transition_now - transition_start;
          double t = 2.0f * elapsed.count() / animation_fade_duration;

          if (t <= 1.0) {
            // Fade out current animation
            current_animation->animate(time);
            matrix->SetBrightness(
                (1.0 - t) *
                current_brightness); // Use current_brightness for smooth fade
          } else if (t <= 2.0) {
            // Fade in next animation
            next_animation->animate(time);
            matrix->SetBrightness(
                (t - 1.0) *
                current_brightness); // Use current_brightness for smooth fade
          } else {
            // Transition complete
            matrix->SetBrightness(
                current_brightness); // Ensure proper brightness at the end
            break;
          }
          // Only advance time for animations during transition, don't
          // accumulate large jumps
          time += transition_deltaTime;
        }
        // Update the main loop timing after transition to prevent time jump
        last = std::chrono::system_clock::now();
      }

      // Switch to the next animation (whether display is on or off)
      current_animation = next_animation;
      last_index = anim_index;
    }

    // Set the current brightness (which may be transitioning)
    matrix->SetBrightness(current_brightness);

    // Only advance animation time when we're actually animating
    time += deltaTime;
    current_animation->animate(time);
  }
}

bool AnimationManager::setAnimationMode(int animationId, int mode) {
  // Initialize animations if not already done
  if (!animations_initialized) {
    initAnimations();
  }

  // Check if animation ID is valid
  if (animationId < 0 || animationId >= static_cast<int>(animations.size())) {
    return false;
  }

  // Convert int to AnimationMode enum
  animations::AnimationMode modeEnum;
  if (mode == 0) {
    modeEnum = animations::AnimationMode::Default;
  } else if (mode == 1) {
    modeEnum = animations::AnimationMode::Preset;
  } else if (mode == 2) {
    modeEnum = animations::AnimationMode::Custom;
  } else {
    return false; // Invalid mode
  }

  // Set the mode
  animations[animationId]->setMode(modeEnum);
  return true;
}

int AnimationManager::getAnimationModeInt(int animationId) {
  // Initialize animations if not already done
  if (!animations_initialized) {
    initAnimations();
  }

  // Check if animation ID is valid
  if (animationId < 0 || animationId >= static_cast<int>(animations.size())) {
    return -1; // Invalid
  }

  // Convert AnimationMode enum to int
  auto mode = animations[animationId]->getMode();
  return static_cast<int>(mode);
}

bool AnimationManager::setAnimationParameters(
    int animationId, const nlohmann::json &parameters) {
  // Initialize animations if not already done
  if (!animations_initialized) {
    initAnimations();
  }

  // Check if animation ID is valid
  if (animationId < 0 || animationId >= static_cast<int>(animations.size())) {
    return false;
  }

  // Check if parameters is an object
  if (!parameters.is_object()) {
    return false;
  }

  bool allSuccess = true;
  // Set each parameter
  for (auto &[key, value] : parameters.items()) {
    bool success = false;
    if (value.is_number()) {
      // Handle numeric parameters (int, float)
      success = animations[animationId]->setParameter(key, value.get<double>());
    } else if (value.is_string()) {
      // Handle color parameters as hex strings (e.g., "#FF0000")
      success =
          animations[animationId]->setParameter(key, value.get<std::string>());
    } else if (value.is_object() && value.contains("r") &&
               value.contains("g") && value.contains("b")) {
      // Handle color parameters as RGB objects (e.g., {"r": 255, "g": 0, "b":
      // 0})
      try {
        uint8_t r = value["r"].get<int>();
        uint8_t g = value["g"].get<int>();
        uint8_t b = value["b"].get<int>();
        char hex_buffer[8];
        snprintf(hex_buffer, sizeof(hex_buffer), "#%02X%02X%02X", r, g, b);
        success =
            animations[animationId]->setParameter(key, std::string(hex_buffer));
      } catch (const std::exception &) {
        success = false;
      }
    } else {
      // Unsupported parameter type
      allSuccess = false;
      continue;
    }

    if (!success) {
      allSuccess = false;
    }
  }

  return allSuccess;
}

bool AnimationManager::setImageData(int animationId,
                                    const std::vector<uint8_t> &data, int width,
                                    int height) {
  // Initialize animations if not already done
  if (!animations_initialized) {
    initAnimations();
  }

  // Check if animation ID is valid
  if (animationId < 0 || animationId >= static_cast<int>(animations.size())) {
    return false;
  }

  // Try to cast to Image animation
  animations::Image *imageAnim =
      dynamic_cast<animations::Image *>(animations[animationId].get());
  if (!imageAnim) {
    return false; // Not an Image animation
  }

  imageAnim->setImageData(data, width, height);
  return true;
}

bool AnimationManager::setImageDataBase64(int animationId,
                                          const std::string &base64_data,
                                          int width, int height) {
  // Initialize animations if not already done
  if (!animations_initialized) {
    initAnimations();
  }

  // Check if animation ID is valid
  if (animationId < 0 || animationId >= static_cast<int>(animations.size())) {
    return false;
  }

  // Try to cast to Image animation
  animations::Image *imageAnim =
      dynamic_cast<animations::Image *>(animations[animationId].get());
  if (!imageAnim) {
    return false; // Not an Image animation
  }

  imageAnim->setImageDataBase64(base64_data, width, height);
  return true;
}

void AnimationManager::updateBrightnessTransition(double deltaTime) {
  // Get current target values (using individual locks to avoid deadlock)
  float effective_target_brightness;
  {
    std::scoped_lock lock(mtx_state, mtx_brightness); // deadlock-safe
    if (target_state) {
      effective_target_brightness = static_cast<float>(target_brightness);
    } else {
      effective_target_brightness = 0.0f; // Fade to 0 when off
    }
  }

  // Smoothly interpolate current brightness towards target
  float brightness_diff = effective_target_brightness - current_brightness;

  if (std::abs(brightness_diff) < 0.1f) {
    // Close enough, snap to target
    current_brightness = effective_target_brightness;
  } else {
    // Smoothly interpolate using fade_speed
    float change_amount = fade_speed * deltaTime;
    if (brightness_diff > 0) {
      current_brightness = std::min(current_brightness + change_amount,
                                    effective_target_brightness);
    } else {
      current_brightness = std::max(current_brightness - change_amount,
                                    effective_target_brightness);
    }
  }

  // Ensure current_brightness stays within valid bounds
  current_brightness = std::max(0.0f, std::min(current_brightness, 255.0f));
}

bool AnimationManager::isInTransition() const {
  float effective_target_brightness;
  {
    std::scoped_lock lock(mtx_state, mtx_brightness); // deadlock-safe
    effective_target_brightness =
        target_state ? static_cast<float>(target_brightness) : 0.0f;
  }

  return std::abs(current_brightness - effective_target_brightness) > 0.1f;
}