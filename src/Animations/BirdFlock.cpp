#include "Animations/BirdFlock.h"
#include "Utils.h"
#include "graphics.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <random>
#include <unistd.h>
#include <vector>

namespace animations {

struct Bird {
  float x, y;   // Position
  float vx, vy; // Velocity
  float ax, ay; // Acceleration
  rgb_matrix::Color color;

  Bird(float x, float y) : x(x), y(y), vx(0), vy(0), ax(0), ay(0) {
    // Random velocity
    vx = (rand() % 200 - 100) / 100.0f;
    vy = (rand() % 200 - 100) / 100.0f;

    // Random color (bird-like colors)
    color = rgb_matrix::Color(255, 255, 255);
    // int colorChoice = rand() % 3;
    // switch (colorChoice) {
    // case 0:
    //   color = rgb_matrix::Color(255, 100, 0);
    //   break; // Orange
    // case 1:
    //   color = rgb_matrix::Color(100, 100, 255);
    //   break; // Blue
    // case 2:
    //   color = rgb_matrix::Color(200, 200, 200);
    //   break; // Gray
    // }
  }
};

class BirdFlockImpl {
public:
  std::vector<Bird> birds;
  int width, height;
  double lastTime;

  // Flocking parameters - adjusted for smaller flocks
  float separationRadius = 6.5f; // Reduced from 8.0f
  float alignmentRadius = 9.0f;  // Reduced from 15.0f
  float cohesionRadius = 15.0f;  // Reduced from 20.0f
  float maxSpeed = 2.0f;
  float maxForce = 0.1f;

  float speedMultiplier = 0.8f;

  // Wind parameters
  float windX = 0.0f, windY = 0.0f;
  double lastWindChange = 0.0;
  float windChangeInterval = 1.5f; // Change wind every X seconds
  float windStrength = 0.021f;

  BirdFlockImpl(int w, int h) : width(w), height(h), lastTime(0.0) {
    // Initialize random number generator
    srand(time(nullptr));

    // Create initial flock - reduced for smaller flocks
    int numBirds = 200;
    for (int i = 0; i < numBirds; i++) {
      birds.emplace_back(rand() % width, rand() % height);
    }

    // Initialize first wind direction
    updateWind();
  }

  ~BirdFlockImpl() = default;

  void updateWind() {
    // Create random wind direction
    float windAngle = (rand() % 360) * M_PI / 180.0f;
    windX = cos(windAngle) * windStrength;
    windY = sin(windAngle) * windStrength;
  }

  // Add edge avoidance to encourage direction changes
  void edgeAvoidance(Bird &bird, float &fx, float &fy) {
    fx = fy = 0;
    float margin = 15.0f; // Distance from edge to start avoiding
    float avoidForce = 0.2f;

    // Avoid edges by applying force away from them
    if (bird.x < margin) {
      fx += avoidForce * (margin - bird.x) / margin;
    }
    if (bird.x > width - margin) {
      fx -= avoidForce * (bird.x - (width - margin)) / margin;
    }
    if (bird.y < margin) {
      fy += avoidForce * (margin - bird.y) / margin;
    }
    if (bird.y > height - margin) {
      fy -= avoidForce * (bird.y - (height - margin)) / margin;
    }
  }

  // Calculate separation force (avoid crowding)
  void separation(Bird &bird, float &fx, float &fy) {
    fx = fy = 0;
    int count = 0;

    for (const auto &other : birds) {
      if (&bird == &other)
        continue;

      float dx = bird.x - other.x;
      float dy = bird.y - other.y;
      float dist = sqrt(dx * dx + dy * dy);

      if (dist > 0 && dist < separationRadius) {
        fx += dx / dist;
        fy += dy / dist;
        count++;
      }
    }

    if (count > 0) {
      fx /= count;
      fy /= count;

      // Normalize and scale
      float mag = sqrt(fx * fx + fy * fy);
      if (mag > 0) {
        fx = (fx / mag) * maxSpeed - bird.vx;
        fy = (fy / mag) * maxSpeed - bird.vy;

        // Limit force
        float forceMag = sqrt(fx * fx + fy * fy);
        if (forceMag > maxForce) {
          fx = (fx / forceMag) * maxForce;
          fy = (fy / forceMag) * maxForce;
        }
      }
    }
  }

  // Calculate alignment force (steer towards average heading)
  void alignment(Bird &bird, float &fx, float &fy) {
    fx = fy = 0;
    int count = 0;

    for (const auto &other : birds) {
      if (&bird == &other)
        continue;

      float dx = bird.x - other.x;
      float dy = bird.y - other.y;
      float dist = sqrt(dx * dx + dy * dy);

      if (dist > 0 && dist < alignmentRadius) {
        fx += other.vx;
        fy += other.vy;
        count++;
      }
    }

    if (count > 0) {
      fx /= count;
      fy /= count;

      // Normalize and scale
      float mag = sqrt(fx * fx + fy * fy);
      if (mag > 0) {
        fx = (fx / mag) * maxSpeed - bird.vx;
        fy = (fy / mag) * maxSpeed - bird.vy;

        // Limit force
        float forceMag = sqrt(fx * fx + fy * fy);
        if (forceMag > maxForce) {
          fx = (fx / forceMag) * maxForce;
          fy = (fy / forceMag) * maxForce;
        }
      }
    }
  }

  // Calculate cohesion force (steer towards center of neighbors)
  void cohesion(Bird &bird, float &fx, float &fy) {
    fx = fy = 0;
    int count = 0;

    for (const auto &other : birds) {
      if (&bird == &other)
        continue;

      float dx = bird.x - other.x;
      float dy = bird.y - other.y;
      float dist = sqrt(dx * dx + dy * dy);

      if (dist > 0 && dist < cohesionRadius) {
        fx += other.x;
        fy += other.y;
        count++;
      }
    }

    if (count > 0) {
      fx /= count;
      fy /= count;

      // Seek towards center
      fx = fx - bird.x;
      fy = fy - bird.y;

      // Normalize and scale
      float mag = sqrt(fx * fx + fy * fy);
      if (mag > 0) {
        fx = (fx / mag) * maxSpeed - bird.vx;
        fy = (fy / mag) * maxSpeed - bird.vy;

        // Limit force
        float forceMag = sqrt(fx * fx + fy * fy);
        if (forceMag > maxForce) {
          fx = (fx / forceMag) * maxForce;
          fy = (fy / forceMag) * maxForce;
        }
      }
    }
  }

  void update(double currentTime) {
    // Calculate delta time (time since last frame)
    double deltaTime =
        (lastTime == 0.0)
            ? 0.016
            : (currentTime - lastTime); // Default to ~60fps on first frame
    lastTime = currentTime;

    // Update wind direction periodically
    if (currentTime - lastWindChange > windChangeInterval) {
      updateWind();
      lastWindChange = currentTime;
    }

    for (auto &bird : birds) {
      float sepX, sepY, aliX, aliY, cohX, cohY;

      // Calculate forces
      separation(bird, sepX, sepY);
      alignment(bird, aliX, aliY);
      cohesion(bird, cohX, cohY);

      // Apply forces with weights, including wind (removed edge avoidance)
      bird.ax = sepX * 1.5f + aliX * 0.8f + cohX * 1.2f + windX;
      bird.ay = sepY * 1.5f + aliY * 0.8f + cohY * 1.2f + windY;

      // Increase randomness for more direction changes
      if (rand() % 100 < 8) { // 8% chance per frame (increased from 2%)
        bird.ax += (rand() % 200 - 100) / 800.0f; // Stronger random impulse
        bird.ay += (rand() % 200 - 100) / 800.0f;
      }

      // Add occasional strong directional changes to break patterns
      if (rand() % 500 < 1) { // 0.2% chance per frame
        float changeAngle = (rand() % 360) * M_PI / 180.0f;
        float changeForce = 0.15f;
        bird.ax += cos(changeAngle) * changeForce;
        bird.ay += sin(changeAngle) * changeForce;
      }

      // Update velocity with delta time
      bird.vx += bird.ax * deltaTime * 60.0f * speedMultiplier;
      bird.vy += bird.ay * deltaTime * 60.0f * speedMultiplier;

      // Limit speed
      float speed = sqrt(bird.vx * bird.vx + bird.vy * bird.vy);
      if (speed > maxSpeed) {
        bird.vx = (bird.vx / speed) * maxSpeed;
        bird.vy = (bird.vy / speed) * maxSpeed;
      }

      // Update position with delta time
      bird.x += bird.vx * deltaTime * 60.0f * speedMultiplier;
      bird.y += bird.vy * deltaTime * 60.0f * speedMultiplier;

      // Wrap around edges
      if (bird.x < 0)
        bird.x = width - 1;
      if (bird.x >= width)
        bird.x = 0;
      if (bird.y < 0)
        bird.y = height - 1;
      if (bird.y >= height)
        bird.y = 0;
    }
  }

  void draw(rgb_matrix::Canvas *canvas) {
    for (const auto &bird : birds) {
      int x = (int)round(bird.x);
      int y = (int)round(bird.y);

      if (x >= 0 && x < width && y >= 0 && y < height) {
        canvas->SetPixel(x, y, bird.color.r, bird.color.g, bird.color.b);

        // Draw a small tail based on velocity direction
        float tailX = bird.x - bird.vx * 2;
        float tailY = bird.y - bird.vy * 2;
        int tx = (int)round(tailX);
        int ty = (int)round(tailY);

        if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
          canvas->SetPixel(tx, ty, bird.color.r / 3, bird.color.g / 3,
                           bird.color.b / 3);
        }
      }
    }
  }
};

void BirdFlock::animate(double time) {
  offscreen_canvas->Clear();

  // Initialize on first call
  if (!flockImpl) {
    flockImpl = new BirdFlockImpl(offscreen_canvas->width(),
                                  offscreen_canvas->height());
  }

  // Update bird positions
  flockImpl->update(time);

  // Draw birds
  flockImpl->draw(offscreen_canvas);

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations