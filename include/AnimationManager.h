#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

#include "led-matrix.h"
#include <cmath>
#include <mutex>

class AnimationManager {
private:
  int current_animation_index;
  std::mutex mtx;

public:
  AnimationManager(rgb_matrix::RGBMatrix *matrix)
      : current_animation_index(0), matrix(matrix) {}

  void run(volatile int *interrupt_received);

  void setValue(int v) {
    std::lock_guard<std::mutex> lock(mtx);
    current_animation_index = v;
  }
  int getValue() {
    std::lock_guard<std::mutex> lock(mtx);
    return current_animation_index;
  }

protected:
  rgb_matrix::RGBMatrix *matrix;
};

#endif // ANIMATIONMANAGER_H