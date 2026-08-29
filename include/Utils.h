#ifndef UTILS_H
#define UTILS_H

#include <cmath>
#include <cstdint>

namespace utils {

void hsvToRgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b);

int signum(double value);

} // namespace utils

#endif // UTILS_H