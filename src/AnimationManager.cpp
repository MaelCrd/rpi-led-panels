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

#include <chrono>
#include <ctime>

void AnimationManager::run(volatile int *interrupt_received) {
  // Implementation of the run method

  // Create animations vector
  auto animations = std::vector<animations::Animation *>{};
  // Add animations to the vector
  animations.push_back(new animations::Random(matrix));
  animations.push_back(new animations::HeightMap(matrix));
  animations.push_back(new animations::GameOfLife(matrix, "B3/S23"));
  animations.push_back(new animations::Static(matrix));
  animations.push_back(new animations::Clock(matrix));
  animations.push_back(new animations::Party(matrix));
  animations.push_back(new animations::Test1(matrix));
  animations.push_back(new animations::Stars(matrix));
  animations.push_back(new animations::DropletCircles(matrix));
  animations.push_back(new animations::Matrix(matrix));
  animations.push_back(new animations::Maze(matrix));
  animations.push_back(new animations::Atom(matrix));
  animations.push_back(new animations::BirdFlock(matrix));

  /////
  int anim_index = getValue();

  auto start = std::chrono::system_clock::now();
  int frame_count = 0;
  double time = 0;
  auto last = start;
  // Loop forever, animating the random animation.
  while (true) {
    if (*interrupt_received)
      break;

    anim_index = getValue();
    animations[anim_index % animations.size()]->animate(time);

    auto now = std::chrono::system_clock::now();
    time += std::chrono::duration<double>(now - last).count();
    last = now;
    std::chrono::duration<double> elapsed = now - start;
    frame_count++;
    if (elapsed.count() >= 1.0) {
      double fps = frame_count / elapsed.count();
      // std::cout << "FPS: " << fps << " | Frame (" << frame_count << ")
      // Time:
      // " << time << std::endl;
      start = now;
      frame_count = 0;
    }
    // usleep(1 * 100); // wait a little to slow down things.
  }
}