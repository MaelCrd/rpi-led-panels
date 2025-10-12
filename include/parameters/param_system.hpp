#pragma once
#include <algorithm> // For std::min, std::max, std::clamp
#include <cstdio>    // For snprintf
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace animations {

// Forward declaration for Color struct
struct Color;

enum class ParamKind { Int, Float, Color, String, Unknown };

// Color structure to represent RGB values
struct Color {
  uint8_t r, g, b;

  // Default constructor (black)
  constexpr Color() : r(0), g(0), b(0) {}

  // RGB constructor
  constexpr Color(uint8_t red, uint8_t green, uint8_t blue)
      : r(red), g(green), b(blue) {}

  // Hex constructor (e.g., 0xFF0000 for red)
  constexpr Color(uint32_t hex)
      : r((hex >> 16) & 0xFF), g((hex >> 8) & 0xFF), b(hex & 0xFF) {}

  // Convert to hex value
  constexpr uint32_t toHex() const {
    return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) |
           static_cast<uint32_t>(b);
  }

  // Equality operators
  constexpr bool operator==(const Color &other) const {
    return r == other.r && g == other.g && b == other.b;
  }

  constexpr bool operator!=(const Color &other) const {
    return !(*this == other);
  }

  // Arithmetic operators for animation parameter calculations
  constexpr Color operator+(const Color &other) const {
    return Color(static_cast<uint8_t>(std::min(
                     255, static_cast<int>(r) + static_cast<int>(other.r))),
                 static_cast<uint8_t>(std::min(
                     255, static_cast<int>(g) + static_cast<int>(other.g))),
                 static_cast<uint8_t>(std::min(
                     255, static_cast<int>(b) + static_cast<int>(other.b))));
  }

  constexpr Color operator-(const Color &other) const {
    return Color(static_cast<uint8_t>(std::max(
                     0, static_cast<int>(r) - static_cast<int>(other.r))),
                 static_cast<uint8_t>(std::max(
                     0, static_cast<int>(g) - static_cast<int>(other.g))),
                 static_cast<uint8_t>(std::max(
                     0, static_cast<int>(b) - static_cast<int>(other.b))));
  }

  constexpr Color operator*(double factor) const {
    return Color(
        static_cast<uint8_t>(std::clamp(static_cast<int>(r * factor), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(g * factor), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(b * factor), 0, 255)));
  }
};

template <typename T> constexpr ParamKind kindOf() {
  if constexpr (std::is_same_v<T, int> || std::is_same_v<T, long> ||
                std::is_same_v<T, short>) {
    return ParamKind::Int;
  } else if constexpr (std::is_floating_point_v<T>) {
    return ParamKind::Float;
  } else if constexpr (std::is_same_v<T, Color>) {
    return ParamKind::Color;
  } else if constexpr (std::is_same_v<T, std::string>) {
    return ParamKind::String;
  } else {
    return ParamKind::Unknown;
  }
}

template <typename T> struct Param {
  using value_type = T;
  T value;
  T min;
  T max;
  std::string_view displayName;
  std::string_view
      internalName; // field identifier (can be same as member name)

  constexpr Param(T def, T mn, T mx, std::string_view display,
                  std::string_view internal)
      : value(def), min(mn), max(mx), displayName(display),
        internalName(internal) {}

  constexpr ParamKind kind() const { return kindOf<T>(); }
};

// Specialization of Param for Color type (no min/max needed)
template <> struct Param<Color> {
  using value_type = Color;
  Color value;
  Color min; // Not used for colors, but kept for template compatibility
  Color max; // Not used for colors, but kept for template compatibility
  std::string_view displayName;
  std::string_view internalName;

  constexpr Param(Color def, Color mn, Color mx, std::string_view display,
                  std::string_view internal)
      : value(def), min(mn), max(mx), displayName(display),
        internalName(internal) {}

  // Simplified constructor for colors (no min/max needed)
  constexpr Param(Color def, std::string_view display,
                  std::string_view internal)
      : value(def), min(Color(0, 0, 0)), max(Color(255, 255, 255)),
        displayName(display), internalName(internal) {}

  constexpr ParamKind kind() const { return ParamKind::Color; }
};

// Specialization of Param for std::string type (no min/max needed)
template <> struct Param<std::string> {
  using value_type = std::string;
  std::string value;
  std::string min; // Not used for strings, but kept for template compatibility
  std::string max; // Not used for strings, but kept for template compatibility
  std::string_view displayName;
  std::string_view internalName;

  Param(const std::string &def, const std::string &mn, const std::string &mx,
        std::string_view display, std::string_view internal)
      : value(def), min(mn), max(mx), displayName(display),
        internalName(internal) {}

  // Simplified constructor for strings (no min/max needed)
  Param(const std::string &def, std::string_view display,
        std::string_view internal)
      : value(def), min(""), max(""), displayName(display),
        internalName(internal) {}

  constexpr ParamKind kind() const { return ParamKind::String; }
};

struct ParameterView {
  std::string internalName;
  std::string displayName;
  ParamKind kind;
  double min;
  double max;
  double value;
  std::string colorValue; // For color parameters: hex string like "#FF0000"
};

inline std::string kindToString(ParamKind k) {
  switch (k) {
  case ParamKind::Int:
    return "int";
  case ParamKind::Float:
    return "float";
  case ParamKind::Color:
    return "color";
  case ParamKind::String:
    return "string";
  default:
    return "unknown";
  }
}

// Tuple iteration helper
template <typename F, typename... Ts>
void forEachInTuple(std::tuple<Ts...> &tup, F &&f) {
  std::apply([&](auto &...elems) { (f(elems), ...); }, tup);
}
template <typename F, typename... Ts>
void forEachInTuple(const std::tuple<Ts...> &tup, F &&f) {
  std::apply([&](auto const &...elems) { (f(elems), ...); }, tup);
}

// Base mixin for parameter aggregate
template <typename Derived> struct ParameterSet {
  using Self = Derived;

  // Derived must implement: auto tuple() -> std::tuple<Param<...>&, ...>

  std::vector<ParameterView> describe() const {
    std::vector<ParameterView> out;
    auto const tup = static_cast<Derived const *>(this)->tuple();
    forEachInTuple(tup, [&](auto const &p) {
      ParameterView view;
      view.internalName = std::string(p.internalName);
      view.displayName = std::string(p.displayName);
      view.kind = p.kind();

      using VT = typename std::decay_t<decltype(p)>::value_type;
      if constexpr (std::is_same_v<VT, Color>) {
        // For color parameters
        view.min = 0;        // Not meaningful for colors
        view.max = 0xFFFFFF; // Max hex value
        view.value = static_cast<double>(p.value.toHex());
        // Format as hex string with #
        char hex_buffer[8];
        snprintf(hex_buffer, sizeof(hex_buffer), "#%06X", p.value.toHex());
        view.colorValue = std::string(hex_buffer);
      } else if constexpr (std::is_same_v<VT, std::string>) {
        // For string parameters
        view.min = 0;
        view.max = 0;
        view.value = 0;            // Not meaningful for strings
        view.colorValue = p.value; // Store the string value
      } else {
        // For numeric parameters
        view.min = static_cast<double>(p.min);
        view.max = static_cast<double>(p.max);
        view.value = static_cast<double>(p.value);
        view.colorValue = ""; // Empty for non-color parameters
      }

      out.push_back(view);
    });
    return out;
  }

  bool setByName(std::string_view name, double v) {
    bool changed = false;
    auto tup = static_cast<Derived *>(this)->tuple();
    forEachInTuple(tup, [&](auto &p) {
      if (p.internalName == name) {
        using VT = typename std::decay_t<decltype(p)>::value_type;
        if constexpr (std::is_same_v<VT, Color>) {
          // For color parameters, treat the double as a hex value
          uint32_t hex_value = static_cast<uint32_t>(v);
          p.value = Color(hex_value);
          changed = true;
        } else if constexpr (std::is_same_v<VT, std::string>) {
          // For string parameters, ignore double values (strings are set via
          // the string overload) This prevents compilation errors when trying
          // to set string params with numeric values
        } else {
          // For numeric parameters
          double clamped =
              (v < static_cast<double>(p.min))   ? static_cast<double>(p.min)
              : (v > static_cast<double>(p.max)) ? static_cast<double>(p.max)
                                                 : v;
          if constexpr (std::is_integral_v<VT>) {
            p.value = static_cast<VT>(static_cast<long long>(clamped));
          } else {
            p.value = static_cast<VT>(clamped);
          }
          changed = true;
        }
      }
    });
    return changed;
  }

  // Overload for setting color parameters by hex string
  bool setByName(std::string_view name, std::string_view colorHex) {
    bool changed = false;
    auto tup = static_cast<Derived *>(this)->tuple();
    forEachInTuple(tup, [&](auto &p) {
      if (p.internalName == name) {
        using VT = typename std::decay_t<decltype(p)>::value_type;
        if constexpr (std::is_same_v<VT, Color>) {
          // Parse hex string (with or without #)
          std::string hex_str(colorHex);
          if (hex_str.size() > 0 && hex_str[0] == '#') {
            hex_str = hex_str.substr(1);
          }
          if (hex_str.size() == 6) {
            uint32_t hex_value = std::stoul(hex_str, nullptr, 16);
            p.value = Color(hex_value);
            changed = true;
          }
        } else if constexpr (std::is_same_v<VT, std::string>) {
          // For string parameters, set the value directly
          p.value = std::string(colorHex);
          changed = true;
        }
      }
    });
    return changed;
  }

  nlohmann::json toJson() const {
    nlohmann::json j;
    j["parameters"] = nlohmann::json::array();
    auto desc = describe();
    for (auto const &pd : desc) {
      nlohmann::json param_json = {
          {"type", kindToString(pd.kind)},
          {"name", pd.displayName},
          {"internalName", pd.internalName}, // for programmatic updates
          {"min", pd.min},
          {"max", pd.max},
          {"value", pd.value}};

      // Add color-specific field for color parameters
      if (pd.kind == ParamKind::Color) {
        param_json["colorValue"] = pd.colorValue;
      }

      j["parameters"].push_back(param_json);
    }
    return j;
  }
};

#define PARAM_INT(field, defaultVal, minVal, maxVal, display)                  \
  animations::Param<int> field{defaultVal, minVal, maxVal, display, #field};

#define PARAM_FLOAT(field, defaultVal, minVal, maxVal, display)                \
  animations::Param<float> field{defaultVal, minVal, maxVal, display, #field};

#define PARAM_COLOR(field, defaultVal, display)                                \
  animations::Param<animations::Color> field{defaultVal, display, #field};

#define PARAM_STRING(field, defaultVal, display)                               \
  animations::Param<std::string> field{defaultVal, display, #field};

} // namespace animations