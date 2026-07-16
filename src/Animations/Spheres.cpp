#include "Animations/Spheres.h"

#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <unistd.h>
#include <vector>

namespace animations {

class SpherePointDistribution {
private:
  std::vector<Point3D> points;
  int numPoints;
  double radius;

  // Paramètres adaptatifs basés sur le nombre de points
  double baseLearningRate; // Plus de points = taux plus faible
  const double minDistance = 0.001;
  double convergenceThreshold; // Seuil adaptatif
  int iterationCount = 0;

public:
  SpherePointDistribution(int n, double r = 1.0) : numPoints(n), radius(r) {
    initializeRandomPoints();
    // optimizeDistribution();
    baseLearningRate =
        std::min(0.1, 0.3 / numPoints); // Plus de points = taux plus faible
    convergenceThreshold = 0.001 / std::sqrt(numPoints); // Seuil adaptatif
  }

  void initializeRandomPoints() {
    points.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);

    for (int i = 0; i < numPoints; ++i) {
      Point3D p;
      do {
        p.x = dis(gen);
        p.y = dis(gen);
        p.z = dis(gen);
      } while (p.length() > 1.0); // Reject points outside unit sphere

      // Project to sphere surface
      p = p.normalized() * radius;
      points.push_back(p);
    }
  }

  void optimizeDistribution(int iterations = 20, int total_iterations = 100) {
    std::cout << "Starting optimization with " << iterations
              << " iterations for " << numPoints << " points..." << std::endl;
    std::cout << "Base learning rate: " << baseLearningRate << std::endl;

    for (int iter = 0; iter < iterations; ++iter) {
      std::vector<Point3D> forces(numPoints, Point3D());
      double maxForce = 0.0;
      double totalForce = 0.0;

      // Calculate repulsion forces between all pairs of points
      for (int i = 0; i < numPoints; ++i) {
        for (int j = 0; j < numPoints; ++j) {
          if (i == j)
            continue;

          Point3D diff =
              Point3D(points[i].x - points[j].x, points[i].y - points[j].y,
                      points[i].z - points[j].z);

          double distance = diff.length();
          if (distance < minDistance)
            distance = minDistance;

          // Force de répulsion avec atténuation pour éviter l'explosion
          double forceStrength =
              1.0 / (distance * distance + 0.01); // Régularisation
          Point3D force = diff.normalized() * forceStrength;

          forces[i] = forces[i] + force;
        }

        // Track forces for convergence monitoring
        double forceLen = forces[i].length();
        totalForce += forceLen;
        if (forceLen > maxForce)
          maxForce = forceLen;
      }

      // Taux d'apprentissage adaptatif avec décroissance
      double progress = (double)0 / total_iterations;
      // double currentLearningRate =
      //     baseLearningRate * (1.0 - progress * 0.9) * // Décroissance plus
      //     douce (1.0 /
      //      (1.0 + maxForce * 0.1)); // Réduction si forces trop importantes
      double currentLearningRate = baseLearningRate;

      // Apply forces and project back to sphere
      for (int i = 0; i < numPoints; ++i) {
        Point3D newPos = points[i] + forces[i] * currentLearningRate;
        // Project back to sphere surface
        points[i] = newPos.normalized() * radius;
      }

      // Progress monitoring avec plus d'infos
      if (iterationCount % (total_iterations / 10) == 0 ||
          iterationCount < 10) {
        double avgForce = totalForce / numPoints;
        std::cout << "Iteration " << iterationCount
                  << ", max force: " << maxForce << ", avg force: " << avgForce
                  << ", learning rate: " << currentLearningRate << std::endl;
      }

      // Early stopping avec seuil adaptatif
      double avgForce = totalForce / numPoints;
      if (avgForce < convergenceThreshold && iterationCount > 100) {
        std::cout << "Converged at iteration " << iterationCount
                  << " (avg force: " << avgForce << ")" << std::endl;
        break;
      }

      // // Si les forces sont trop importantes, réduire le taux d'apprentissage
      // if (maxForce > 1000.0) {
      //   std::cout << "High forces detected, reducing learning rate"
      //             << std::endl;
      //   baseLearningRate *= 0.5;
      // }

      iterationCount++;
    }

    std::cout << "Optimization completed!" << std::endl;
  }

  const std::vector<Point3D> &getPoints() const { return points; }

  void printDistributionQuality() const {
    if (points.size() < 2)
      return;

    double minDist = std::numeric_limits<double>::max();
    double maxDist = 0.0;
    double totalDist = 0.0;
    int pairCount = 0;

    for (int i = 0; i < numPoints; ++i) {
      for (int j = i + 1; j < numPoints; ++j) {
        Point3D diff =
            Point3D(points[i].x - points[j].x, points[i].y - points[j].y,
                    points[i].z - points[j].z);
        double distance = diff.length();

        minDist = std::min(minDist, distance);
        maxDist = std::max(maxDist, distance);
        totalDist += distance;
        pairCount++;
      }
    }

    double avgDist = totalDist / pairCount;
    std::cout << "Distribution quality:" << std::endl;
    std::cout << "  Min distance: " << minDist << std::endl;
    std::cout << "  Max distance: " << maxDist << std::endl;
    std::cout << "  Avg distance: " << avgDist << std::endl;
    std::cout << "  Uniformity ratio: " << minDist / maxDist << std::endl;
  }
};

void Spheres::initialize() {
  // Display a loading message
  matrix->SetBrightness(50);
  offscreen_canvas->Clear();
  int center_x = offscreen_canvas->width() / 2;
  int center_y = offscreen_canvas->height() / 2;
  char text[] = "Animation loading...";
  rgb_matrix::Font font;
  font.LoadFont("../deps/matrix/fonts/6x12.bdf");
  int text_width = 0;
  for (char c : std::string(text))
    text_width += font.CharacterWidth(c);
  int text_height = font.height();
  rgb_matrix::DrawText(offscreen_canvas, font, center_x - text_width / 2,
                       center_y - 3 + text_height / 2,
                       rgb_matrix::Color(255, 255, 255), nullptr, text);
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);

  // Initialize first sphere point distribution
  const int numPoints = 700; // Adjust as needed
  distribution1 = new SpherePointDistribution(numPoints);

  std::cout << "Optimizing first sphere point distribution..." << std::endl;
  distribution1->printDistributionQuality();

  // Store the optimized points for rendering
  spherePoints1 = distribution1->getPoints();

  distribution1->optimizeDistribution(30);
  spherePoints1 = distribution1->getPoints();

  // Initialize second sphere point distribution (inner sphere with half radius)
  distribution2 = new SpherePointDistribution(numPoints * 0.5, 0.5);

  std::cout << "Optimizing second sphere point distribution..." << std::endl;
  distribution2->printDistributionQuality();

  // Store the optimized points for rendering
  spherePoints2 = distribution2->getPoints();

  distribution2->optimizeDistribution(30);
  spherePoints2 = distribution2->getPoints();
}

void Spheres::animate(double time) {
  if (!initialized) {
    initialize();
    initialized = true;
  }

  // Rotation animation around Y and X axes
  double rotationSpeedY = 0.02; // Y axis rotation speed
  double rotationSpeedX =
      0.006; // X axis rotation speed (smaller for subtle effect)

  // rotationSpeedY /= 5;
  // rotationSpeedX /= 5;

  double angleY = time * speed * rotationSpeedY;
  double angleX = time * speed * rotationSpeedX;

  // Apply rotation to first sphere points
  rotatedPoints1.clear();
  for (const auto &point : spherePoints1) {
    Point3D rotated;

    // First apply Y axis rotation
    rotated.x = point.x * cos(angleY) - point.z * sin(angleY);
    rotated.y = point.y;
    rotated.z = point.x * sin(angleY) + point.z * cos(angleY);

    // Then apply X axis rotation
    Point3D finalRotated;
    finalRotated.x = rotated.x;
    finalRotated.y = rotated.y * cos(angleX) - rotated.z * sin(angleX);
    finalRotated.z = rotated.y * sin(angleX) + rotated.z * cos(angleX);

    rotatedPoints1.push_back(finalRotated);
  }

  // Apply rotation to second sphere points (with slightly different rotation
  // speeds)
  double angleY2 =
      time * speed * -rotationSpeedY * 0.8; // Different speed for variety
  double angleX2 = time * speed * -rotationSpeedX * 1.2;

  rotatedPoints2.clear();
  for (const auto &point : spherePoints2) {
    Point3D rotated;

    // First apply Y axis rotation
    rotated.x = point.x * cos(angleY2) - point.z * sin(angleY2);
    rotated.y = point.y;
    rotated.z = point.x * sin(angleY2) + point.z * cos(angleY2);

    // Then apply X axis rotation
    Point3D finalRotated;
    finalRotated.x = rotated.x;
    finalRotated.y = rotated.y * cos(angleX2) - rotated.z * sin(angleX2);
    finalRotated.z = rotated.y * sin(angleX2) + rotated.z * cos(angleX2);

    rotatedPoints2.push_back(finalRotated);
  }

  // Clear pixel buffer
  offscreen_canvas->Clear();
  int width = offscreen_canvas->width();
  int height = offscreen_canvas->height();

  int centerX = width / 2;
  int centerY = height / 2;
  int radius = std::min(centerX, centerY) - 3;

  // Create a depth buffer to handle occlusion
  std::vector<double> depthBuffer(width * height,
                                  -std::numeric_limits<double>::max());

  // Collect all points with their depths for sorting
  struct PointWithDepth {
    Point3D point;
    double depth;
    bool isSphere1;
  };

  std::vector<PointWithDepth> allPoints;

  // Add points from first sphere (outer sphere)
  for (const auto &point : rotatedPoints1) {
    allPoints.push_back({point, point.z, true});
  }

  // Add points from second sphere (inner sphere)
  for (const auto &point : rotatedPoints2) {
    allPoints.push_back({point, point.z, false});
  }

  // Sort by depth (back to front - negative z values are closer to viewer)
  std::sort(allPoints.begin(), allPoints.end(),
            [](const PointWithDepth &a, const PointWithDepth &b) {
              return a.depth < b.depth; // Draw back points first
            });

  // Draw all points with proper depth testing
  for (const auto &pointData : allPoints) {
    const auto &point = pointData.point;

    int x = static_cast<int>(centerX + point.x * radius);
    int y = static_cast<int>(centerY - point.y * radius); // Invert Y for screen

    if (x >= 0 && x < width && y >= 0 && y < height) {
      int depthIndex = y * width + x;

      // For outer sphere points, check if they should be occluded by inner
      // sphere
      bool shouldDraw = true;
      if (pointData.isSphere1) {
        // Check if this outer sphere point is behind the inner sphere
        // A point on the outer sphere is behind the inner sphere if its
        // distance from center (in 2D projection) is less than inner sphere
        // radius and its z is less than 0
        double distFromCenter2D =
            std::sqrt(point.x * point.x + point.y * point.y);
        if (distFromCenter2D <= 0.5 && point.z < 0) {
          shouldDraw =
              false; // This outer sphere point is occluded by inner sphere
        }
      }
      shouldDraw = true; // Disable occlusion for testing

      // Only draw if this point should be drawn and is closer than what's
      // already drawn
      if (shouldDraw && point.z > depthBuffer[depthIndex]) {
        depthBuffer[depthIndex] = point.z;

        if (pointData.isSphere1) {
          // Outer sphere
          float zFactor = (point.z + 1.0) * 0.5; // Normalize z to [0,1]
          // pixels[index] = 255 * zFactor;         // Red
          // pixels[index + 1] = 255 * zFactor;     // Green
          // pixels[index + 2] = 255 * zFactor;     // Blue
          // offscreen_canvas->SetPixel(x, y, 255 * zFactor, 255 * zFactor,
          //                            255 * zFactor);
          // offscreen_canvas->SetPixel(x, y, 255 * zFactor, 0 * zFactor,
          //                            0 * zFactor);
          uint8_t r, g, b;
          utils::hsvToRgb(360 - (1.0 - zFactor) * 145, 1.0,
                          0.05 + zFactor * 0.95, r, g, b);
          offscreen_canvas->SetPixel(x, y, r, g, b);
        } else {
          // continue;
          // Inner sphere
          float zFactor = (point.z + 0.5); // Normalize z to [0,1]
          // pixels[index] = 255 * zFactor;   // Red
          // pixels[index + 1] = 0;           // Green
          // pixels[index + 2] = 0;           // Blue
          // offscreen_canvas->SetPixel(x, y, 255 * zFactor, 0 * zFactor,
          //                            0 * zFactor);
          offscreen_canvas->SetPixel(x, y, 255 * zFactor, 255 * zFactor,
                                     255 * zFactor);
        }
      }
    }
  }

  // Swap the offscreen canvas to display the new frame
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations