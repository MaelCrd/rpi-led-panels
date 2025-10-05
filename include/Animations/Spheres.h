#ifndef SPHERES_ANIMATION_H
#define SPHERES_ANIMATION_H

#include "Animation.h"
#include "parameters/param_system.hpp"
#include <cmath>

namespace animations {

struct SpheresParams : ParameterSet<SpheresParams> {
  // Empty tuple for animations with no parameters
  std::tuple<> empty_tuple;
  // Provide tuple of references for iteration
  auto &tuple() { return empty_tuple; }
  auto const &tuple() const { return empty_tuple; }
};

struct Point3D {
  double x, y, z;
  Point3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

  Point3D operator+(const Point3D &other) const {
    return Point3D(x + other.x, y + other.y, z + other.z);
  }

  Point3D operator*(double scalar) const {
    return Point3D(x * scalar, y * scalar, z * scalar);
  }

  double length() const { return std::sqrt(x * x + y * y + z * z); }

  Point3D normalized() const {
    double len = length();
    return len > 0 ? Point3D(x / len, y / len, z / len) : Point3D();
  }
};

class SpherePointDistribution;

class Spheres : public Animation<Spheres, struct SpheresParams> {
private:
  std::vector<Point3D> spherePoints1;
  std::vector<Point3D> rotatedPoints1;
  SpherePointDistribution *distribution1;

  std::vector<Point3D> spherePoints2;
  std::vector<Point3D> rotatedPoints2;
  SpherePointDistribution *distribution2;

  float speed = 60.0f;

public:
  Spheres(rgb_matrix::RGBMatrix *matrix);
  void animate(double time) override;

  std::string name() const override { return "Spheres"; }

  // Override mode parameter methods - Spheres has no parameters so all modes
  // are the same
  void applyDefaultParameters() override {
    // No parameters to set
  }

  void applyPresetParameters() override {
    // No parameters to set
  }

protected:
  rgb_matrix::FrameCanvas *offscreen_canvas;
};
} // namespace animations

#endif // SPHERES_ANIMATION_H