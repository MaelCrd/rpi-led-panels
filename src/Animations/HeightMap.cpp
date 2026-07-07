#include <algorithm>
#include <cmath>

#include "Animations/HeightMap.h"
#include "FastNoise/Generators/Simplex.h"
#include "Utils.h"

///////////////////////////////////////////////////////////////////////

namespace animations {

// Standalone 3D Simplex Noise implementation
class SimplexNoise3D {
private:
  // Permutation table
  static const int perm[256];
  static const int permMod12[256];

  // 3D gradient vectors
  static const float grad3[12][3];

  // Skewing and unskewing factors for 3D
  static constexpr float F3 = 1.0f / 3.0f;
  static constexpr float G3 = 1.0f / 6.0f;

  static float dot(const float g[3], float x, float y, float z) {
    return g[0] * x + g[1] * y + g[2] * z;
  }

public:
  static float noise(float xin, float yin, float zin) {
    float n0, n1, n2, n3; // Noise contributions from the four corners

    // Skew the input space to determine which simplex cell we're in
    float s = (xin + yin + zin) * F3; // Very nice and simple skew factor for 3D
    int i = static_cast<int>(std::floor(xin + s));
    int j = static_cast<int>(std::floor(yin + s));
    int k = static_cast<int>(std::floor(zin + s));

    float t = (i + j + k) * G3;
    float X0 = i - t; // Unskew the cell origin back to (x,y,z) space
    float Y0 = j - t;
    float Z0 = k - t;
    float x0 = xin - X0; // The x,y,z distances from the cell origin
    float y0 = yin - Y0;
    float z0 = zin - Z0;

    // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
    // Determine which simplex we are in.
    int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
    int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

    if (x0 >= y0) {
      if (y0 >= z0) {
        i1 = 1;
        j1 = 0;
        k1 = 0;
        i2 = 1;
        j2 = 1;
        k2 = 0; // X Y Z order
      } else if (x0 >= z0) {
        i1 = 1;
        j1 = 0;
        k1 = 0;
        i2 = 1;
        j2 = 0;
        k2 = 1; // X Z Y order
      } else {
        i1 = 0;
        j1 = 0;
        k1 = 1;
        i2 = 1;
        j2 = 0;
        k2 = 1; // Z X Y order
      }
    } else { // x0 < y0
      if (y0 < z0) {
        i1 = 0;
        j1 = 0;
        k1 = 1;
        i2 = 0;
        j2 = 1;
        k2 = 1; // Z Y X order
      } else if (x0 < z0) {
        i1 = 0;
        j1 = 1;
        k1 = 0;
        i2 = 0;
        j2 = 1;
        k2 = 1; // Y Z X order
      } else {
        i1 = 0;
        j1 = 1;
        k1 = 0;
        i2 = 1;
        j2 = 1;
        k2 = 0; // Y X Z order
      }
    }

    // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
    // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
    // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z),
    // where c = 1/6.
    float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;
    float x2 =
        x0 - i2 + 2.0f * G3; // Offsets for third corner in (x,y,z) coords
    float y2 = y0 - j2 + 2.0f * G3;
    float z2 = z0 - k2 + 2.0f * G3;
    float x3 =
        x0 - 1.0f + 3.0f * G3; // Offsets for last corner in (x,y,z) coords
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    // Work out the hashed gradient indices of the four simplex corners
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;

    // Ensure array bounds are respected - mask ALL indices
    int gi0 = permMod12[(ii + perm[(jj + perm[kk & 255]) & 255]) & 255];
    int gi1 = permMod12[((ii + i1) +
                         perm[((jj + j1) + perm[(kk + k1) & 255]) & 255]) &
                        255];
    int gi2 = permMod12[((ii + i2) +
                         perm[((jj + j2) + perm[(kk + k2) & 255]) & 255]) &
                        255];
    int gi3 =
        permMod12[((ii + 1) + perm[((jj + 1) + perm[(kk + 1) & 255]) & 255]) &
                  255];

    // Calculate the contribution from the four corners
    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 < 0)
      n0 = 0.0f;
    else {
      t0 *= t0;
      n0 = t0 * t0 * dot(grad3[gi0], x0, y0, z0);
    }

    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 < 0)
      n1 = 0.0f;
    else {
      t1 *= t1;
      n1 = t1 * t1 * dot(grad3[gi1], x1, y1, z1);
    }

    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 < 0)
      n2 = 0.0f;
    else {
      t2 *= t2;
      n2 = t2 * t2 * dot(grad3[gi2], x2, y2, z2);
    }

    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 < 0)
      n3 = 0.0f;
    else {
      t3 *= t3;
      n3 = t3 * t3 * dot(grad3[gi3], x3, y3, z3);
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to stay just inside [-1,1]
    return 32.0f * (n0 + n1 + n2 + n3);
  }

  static void genPositionArray3D(float *output, int count, const float *xPos,
                                 const float *yPos, float zPos) {
    for (int i = 0; i < count; i++) {
      output[i] = noise(xPos[i], yPos[i], zPos);
    }
  }
};

// Static member definitions
const int SimplexNoise3D::perm[256] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,
    225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190,
    6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117,
    35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
    171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158,
    231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,
    245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209,
    76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
    164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,
    202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,
    58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,
    154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
    19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
    228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,
    145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184,
    84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
    222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156,
    180};

const int SimplexNoise3D::permMod12[256] = {
    7,  4,  5,  7,  6,  3,  11, 1,  9,  11, 0,  5,  2, 5,  7,  9,  8,  0, 7, 6,
    9,  10, 8,  3,  1,  0,  9,  10, 11, 10, 6,  4,  7, 0,  6,  3,  0,  2, 5, 2,
    10, 0,  3,  11, 9,  11, 11, 8,  9,  9,  9,  4,  9, 5,  8,  3,  6,  8, 5, 4,
    3,  0,  8,  7,  2,  9,  11, 2,  7,  0,  3,  10, 5, 2,  2,  3,  11, 3, 1, 2,
    0,  7,  1,  2,  4,  9,  8,  5,  7,  10, 5,  4,  4, 6,  11, 6,  5,  1, 3, 5,
    1,  0,  8,  1,  5,  4,  3,  11, 5,  2,  11, 1,  7, 10, 6,  8,  6,  5, 4, 9,
    2,  8,  11, 2,  7,  5,  1,  4,  4,  1,  10, 10, 0, 11, 11, 6,  1,  4, 2, 8,
    8,  4,  5,  6,  1,  3,  6,  10, 11, 3,  2,  2,  3, 1,  1,  0,  7,  7, 8, 4,
    11, 10, 2,  5,  9,  6,  3,  1,  7,  10, 6,  0,  6, 0,  8,  11, 1,  4, 2, 1,
    4,  11, 0,  0,  11, 11, 2,  5,  5,  6,  9,  7,  7, 1,  9,  1,  2,  8, 7, 0,
    9,  9,  8,  6,  11, 3,  1,  2,  10, 6,  1,  0,  7, 3,  0,  4,  6,  4, 2, 5,
    2,  8,  1,  10, 11, 11, 6,  10, 5,  4,  1,  2,  8, 8,  1,  5,  4,  8, 6, 3,
    0,  11, 1,  1,  0,  6,  9,  7,  4,  5,  4,  4,  7, 4,  2,  9};

const float SimplexNoise3D::grad3[12][3] = {
    {1, 1, 0},  {-1, 1, 0},  {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
    {1, 0, -1}, {-1, 0, -1}, {0, 1, 1},  {0, -1, 1},  {0, 1, -1}, {0, -1, -1}};

HeightMap::HeightMap(rgb_matrix::RGBMatrix *matrix)
    : Animation(matrix), heightMap(FastNoise::New<FastNoise::Simplex>()) {
  int width = matrix->width();
  int height = matrix->height();
  newPixels = new float[width * height];
  xPos = new float[width * height];
  yPos = new float[width * height];
  zPos = new float[width * height];
  float frequency = 0.01f;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int idx = y * width + x;
      xPos[idx] = static_cast<float>(x) * frequency;
      yPos[idx] = static_cast<float>(y) * frequency;
      zPos[idx] = static_cast<float>(0);
    }
  }
  // Pre-compute all possible colors
  for (int i = 0; i < 256; i++) {
    float normalizedValue = i / 255.0f;
    utils::hsvToRgb(275 + normalizedValue * (360.0f - 275), 1.0f,
                    normalizedValue, colorLookup[i][0], colorLookup[i][1],
                    colorLookup[i][2]);
  }
}

void HeightMap::animate(double time) {
  int width = matrix->width();
  int height = matrix->height();

  // float timeZ = static_cast<float>(time + 99) / 5.0f;
  auto delta_time = time - last_time;
  timeZ += static_cast<float>(delta_time / 5.0f) * params_.speed.value;

  // Combined loop for better cache performance
  for (int i = 0; i < width * height; i++) {
    // Generate noise value directly
    float noiseValue = SimplexNoise3D::noise(xPos[i], yPos[i], timeZ);

    // Process and set pixel immediately with bounds checking
    float value = (std::abs(std::pow(noiseValue, 1)) * 1028.0f);
    // uint8_t clampedValue =
    //     static_cast<uint8_t>(std::clamp(value, 0.0f, 1255.0f));
    uint8_t clampedValue = static_cast<uint8_t>(value);

    // Ensure clampedValue is within bounds
    int x = i % width;
    int y = i / width;
    if (x < width && y < height) {
      double fact = 0;
      switch (params_.style.value) {
      case 1:
        // Style 1: Use the pre-computed color lookup table
        matrix->SetPixel(x, y, colorLookup[clampedValue][0],
                         colorLookup[clampedValue][1],
                         colorLookup[clampedValue][2]);
        break;
      case 2:
        fact = pow(value / 125.0, 5);
        matrix->SetPixel(x, y, fact * params_.color.value.r,
                         fact * params_.color.value.g,
                         fact * params_.color.value.b);
        break;
      case 3:
        fact = (1 + sin(noiseValue * 100.0)) / 2.0;
        matrix->SetPixel(x, y, fact * params_.color.value.r,
                         fact * params_.color.value.g,
                         fact * params_.color.value.b);
        break;
      case 4:
        fact = tan(((noiseValue + 1) * 0.97 * 10.0)) * 16.0;
        matrix->SetPixel(x, y, fact * params_.color.value.r / 255.0,
                         fact * params_.color.value.g / 255.0,
                         fact * params_.color.value.b / 255.0);
        break;
      case 5:
        fact = ((int)(((noiseValue + 1) * 0.5) * 8.0));
        matrix->SetPixel(x, y, fact * (params_.color.value.r / 8.0),
                         fact * (params_.color.value.g / 8.0),
                         fact * (params_.color.value.b / 8.0));
        break;
      }

      // matrix->SetPixel(x, y, pow(noiseValue * 255.0, 2), 0, 0);
      // matrix->SetPixel(x, y, (noiseValue + 1) * 255.0 * 10.0, 0, 0);

      // matrix->SetPixel(x, y, tan(((noiseValue + 1) * 0.97 * 10.0)) * 16.0, 0,
      //                  tan(((noiseValue + 1) * 0.97 * 10.0)) * 6.0);
    }
  }
  last_time = time;
}

} // namespace animations