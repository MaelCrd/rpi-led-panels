#include "AnimationManager.h"
#include "Animation.h"
#include "Animations/Atom.h"
#include "Animations/BirdFlock.h"
#include "Animations/Clock.h"
#include "Animations/DropletCircles.h"
#include "Animations/GameOfLife.h"
#include "Animations/HeightMap.h"
#include "Animations/Matrix.h"
#include "Animations/Maze.h"
#include "Animations/Party.h"
#include "Animations/Random.h"
#include "Animations/Stars.h"
#include "Animations/Static.h"
#include "Animations/Test1.h"
#include <cstdio> // For snprintf

#include "QRCode.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <ctime>
#include <iostream>
#include <unistd.h>

using namespace animations;

AnimationManager::~AnimationManager() {
  // Clean up all animation objects
  for (auto *animation : animations) {
    delete animation;
  }
  animations.clear();
}

void AnimationManager::initAnimations() {
  if (animations_initialized) {
    return; // Already initialized
  }

  // Create animations vector
  animations.push_back(new Random(matrix));
  animations.push_back(new HeightMap(matrix));
  animations.push_back(new GameOfLife(matrix, "B3/S23"));
  animations.push_back(new Static(matrix));
  animations.push_back(new Clock(matrix));
  animations.push_back(new Party(matrix));
  animations.push_back(new Test1(matrix));
  animations.push_back(new Stars(matrix));
  animations.push_back(new DropletCircles(matrix));
  animations.push_back(new Matrix(matrix));
  animations.push_back(new Maze(matrix));
  animations.push_back(new Atom(matrix));
  animations.push_back(new BirdFlock(matrix));
  // animations.push_back(new Spheres(matrix));

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

void AnimationManager::run(volatile int *interrupt_received) {
  // Implementation of the run method

  // Initialize animations if not already done
  if (!animations_initialized) {
    initAnimations();
  }

  /////
  // test QR code generation
  if (false) {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct ifreq ifr{};
    strcpy(ifr.ifr_name,
           "wlan0"); // Change interface name as needed (eth0, wlan0, etc.)
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);

    char ip[INET_ADDRSTRLEN];
    strcpy(ip, inet_ntoa(((sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    std::cout << "IP Address: " << ip << std::endl;

    std::string text = ip;
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
  /////
  int anim_index = getCurrentAnimation();
  int last_index = anim_index;
  BaseAnimation *current_animation = animations[anim_index % animations.size()];
  float fade_duration = 1.0; // seconds

  auto start = std::chrono::system_clock::now();
  int frame_count = 0;
  double time = 0;
  auto last = start;
  // Loop forever, animating the random animation.
  while (true) {
    if (*interrupt_received)
      break;

    anim_index = getCurrentAnimation();
    if (anim_index != last_index) {
      // Start transition
      auto transition_start = std::chrono::system_clock::now();
      auto transition_last = transition_start;
      BaseAnimation *next_animation =
          animations[anim_index % animations.size()];

      // Fade out current animation and then fade in next animation
      while (true) {
        if (*interrupt_received)
          break;
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = now - transition_start;
        std::chrono::duration<double> delta = now - transition_last;
        transition_last = now;
        double t = 2.0f * elapsed.count() / fade_duration;

        if (t <= 1.0) {
          // Fade out current animation
          current_animation->animate(time);
          matrix->SetBrightness((1.0 - t) * brightness); // Fade out brightness
        } else if (t <= 2.0) {
          // Fade in next animation
          next_animation->animate(time);
          matrix->SetBrightness((t - 1.0) * brightness); // Fade in brightness
        } else {
          // Transition complete
          matrix->SetBrightness(
              brightness); // Ensure full brightness at the end
          break;
        }
        time += delta.count();
        last = now;
      }
      // Switch to the next animation
      current_animation = next_animation;
      last_index = anim_index;
    }

    matrix->SetBrightness(brightness); // opti ?
    current_animation->animate(time);

    auto now = std::chrono::system_clock::now();
    time += std::chrono::duration<double>(now - last).count();
    last = now;
    // std::chrono::duration<double> elapsed = now - start;
    // frame_count++;
    // if (elapsed.count() >= 1.0) {
    //   double fps = frame_count / elapsed.count();
    //   // std::cout << "FPS: " << fps << " | Frame (" << frame_count << ")
    //   // Time:
    //   // " << time << std::endl;
    //   start = now;
    //   frame_count = 0;
    // }
    // // usleep(1 * 100); // wait a little to slow down things.
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