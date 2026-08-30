#ifndef UTILS_H
#define UTILS_H

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>

#ifndef ASSETS_DIR
#define ASSETS_DIR ".."
#endif

namespace utils {

void hsvToRgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b);

int signum(double value);

std::unordered_map<std::string, std::string>
loadEnv(const std::string &path = ASSETS_DIR "/.env");

} // namespace utils

#endif // UTILS_H