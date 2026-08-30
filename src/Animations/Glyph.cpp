#include "Animations/Glyphs.h"
#include <Utils.h>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

namespace animations {

// std::string random_real_utf8() {
//   static std::mt19937 rng(std::random_device{}());

//   // Defined [start, end] ranges of real, assigned visible characters:
//   // Basic Latin, Latin Ext, Greek/Cyrillic, General Punct/Symbols, CJK
//   // Ideographs, Emojis/Pictographs
//   static const struct Range {
//     uint32_t start, end;
//   } ranges[] = {
//       {0x0021, 0x007E}, // ASCII Printable (a-z, A-Z, 0-9, punctuation)
//       {0x00A1, 0x024F}, // Latin-1 Supp & Extended-A/B (é, ñ, ø, etc.)
//       //   {0x0370, 0x04FF}, // Greek, Coptic, and Cyrillic // un petit peu
//       //   {0x2010, 0x205E}, // General Punctuation // moyen
//       //   {0x2600, 0x27BF}, // Miscellaneous Symbols & Dingbats (★, ☕, ✂)
//       //
//       //   bcp
//       //   {0x4E00,
//       //    0x9FFF}, // CJK Unified Ideographs (Common
//       Chinese/Japanese/Korean)
//       //    // que ca
//       //   {0x1F600, 0x1F64F}, // Emoticons (😀, 😂, etc.) // que ca
//       //   {0x1F300, 0x1F5FF} // Misc Symbols and Pictographs (🌍, 🍕, 🚀) //
//       //   que ca
//   };

//   static std::uniform_int_distribution<size_t> range_dist(0,
//   std::size(ranges) -
//                                                                  1);
//   const auto &r = ranges[range_dist(rng)];

//   std::uniform_int_distribution<uint32_t> cp_dist(r.start, r.end);
//   uint32_t cp = cp_dist(rng);

//   if (cp <= 0x7F) {
//     return {static_cast<char>(cp)};
//   }
//   if (cp <= 0x7FF) {
//     return {static_cast<char>(0xC0 | (cp >> 6)),
//             static_cast<char>(0x80 | (cp & 0x3F))};
//   }
//   if (cp <= 0xFFFF) {
//     return {static_cast<char>(0xE0 | (cp >> 12)),
//             static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
//             static_cast<char>(0x80 | (cp & 0x3F))};
//   }
//   return {static_cast<char>(0xF0 | (cp >> 18)),
//           static_cast<char>(0x80 | ((cp >> 12) & 0x3F)),
//           static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
//           static_cast<char>(0x80 | (cp & 0x3F))};
// }

// void Glyphs::animate(double time) {
//   // if (time > 0)
//   //   return;

//   offscreen_canvas->Clear();

//   std::string text_str = "";
//   for (int i = 0; i < 30; ++i) {
//     text_str += random_real_utf8();
//   }

//   const char *text = text_str.c_str();
//   rgb_matrix::Font font;
//   font.LoadFont("../deps/matrix/fonts/6x12.bdf");
//   int text_width = 0;
//   for (char c : std::string(text))
//     text_width += font.CharacterWidth(c);
//   int text_height = font.height();
//   int spacing = 10; // Border spacing from the edges of the matrix
//   rgb_matrix::DrawText(offscreen_canvas, font, spacing, spacing + 7,
//                        rgb_matrix::Color(255, 255, 255), nullptr, text);

//   offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
//   usleep(1000000 / 1);
// }

std::string getRandomChar() {
  static const std::string_view chars =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);

  return std::string(1, chars[dist(gen)]);
}

void Glyphs::animate(double time) {
  double delta_time = time - last_time;
  last_time = time;

  offscreen_canvas->Clear();

  //   char text[] = "人類社会のすべての構成員の固有の尊厳と平等で譲ることので";
  //   font.LoadFont(ASSETS_DIR "/fonts/NotoSansJP-12.bdf");
  //   char text[] = "⠁ ⠇ ⠓ ⠥ ⠍ ⠕ ⠗ ⠝";
  //   font.LoadFont(ASSETS_DIR "/fonts/NotoSansSymbols2-7.bdf");
  //
  //   char text[] = "於對人類家庭所有成員嘅固有尊嚴及其平等";
  //   font.LoadFont(ASSETS_DIR "/fonts/WDXLLubrifontSC-10.bdf");

  //   char text[] = "모든 사람은 의견의 자유와 표현의 자";
  //   font.LoadFont(ASSETS_DIR "/fonts/NotoSansKR-10.bdf");

  //   char text[] = "᚛ᚌᚔᚚ ᚓ ᚈᚔᚄᚓᚇ ᚔᚅ ᚃᚐᚔᚇᚉ";
  //   font.LoadFont(ASSETS_DIR "/fonts/NotoSansOgham-13.bdf");

  //   char text[] = "Hello text to test";
  //   font.LoadFont(ASSETS_DIR "/fonts/JacquardaBastarda9-13.bdf");

  //   char text[] = "taulduzeicfyklzjgfehrxj,gfker";
  //   font.LoadFont(ASSETS_DIR "/fonts/Linefont-13.bdf");

  //   char text[] = "azertyuiopqsdfghjklmwxcvbna";
  //   font.LoadFont(ASSETS_DIR "/fonts/Wavefont-32.bdf");

  //////////////////
  // CHINESE FONT //
  //////////////////
  // NotoSansSC-10.bdf
  int spacing = 3; // Border spacing from the edges of the matrix

  // Change some characters in the grid to random Chinese characters
  for (int i = 0; i < grid.size(); ++i) {
    for (int j = 0; j < grid[i].size(); ++j) {
      //   float chance = sin(time * 1.0 + ((i + j) * 0.05));
      // float chance = 0;
      // if (chance * delta_time > 0.5) {
      //   grid[i][j].character = getRandomSCChar();
      // }

      // Tend to stable color over time
      //   float color_change_speed = 4.0 * delta_time;
      //   float color_change_speed = 4.0 * delta_time;
      float color_change_speed = 4.0 * delta_time * params_.fadeSpeed.value;
      //   grid[i][j].color.r =
      //       std::min(255, static_cast<int>(grid[i][j].color.r +
      //                                      color_change_speed *
      //                                          (255 - grid[i][j].color.r)));
      //   grid[i][j].color.g =
      //       std::min(255, static_cast<int>(grid[i][j].color.g +
      //                                      color_change_speed *
      //                                          (255 - grid[i][j].color.g)));
      //   grid[i][j].color.b =
      //       std::min(255, static_cast<int>(grid[i][j].color.b +
      //                                      color_change_speed *
      //                                          (255 - grid[i][j].color.b)));

      // auto target_color = Color(255, 255, 255);
      // auto target_color = Color(0, 0, 0);
      auto target_color = params_.styableColor.value;
      int min_change = 1; // Minimum change per frame to avoid stalling
      grid[i][j].color.r +=
          utils::signum(target_color.r - grid[i][j].color.r) *
          fmax(min_change,
               abs(static_cast<int>(color_change_speed *
                                    (target_color.r - grid[i][j].color.r))));
      grid[i][j].color.g +=
          utils::signum(target_color.g - grid[i][j].color.g) *
          fmax(min_change,
               abs(static_cast<int>(color_change_speed *
                                    (target_color.g - grid[i][j].color.g))));
      grid[i][j].color.b +=
          utils::signum(target_color.b - grid[i][j].color.b) *
          fmax(min_change,
               abs(static_cast<int>(color_change_speed *
                                    (target_color.b - grid[i][j].color.b))));

      // Randomly change color occasionally
      //   float color_change_chance = 0.3 * delta_time;  // 1% chance to
      //   change
      //   float color_change_chance = 1.0 * delta_time; // 1% chance to change
      float color_change_chance =
          0.4 * delta_time * params_.spawnChance.value; // chance to change
      if (static_cast<float>(rand()) / RAND_MAX < color_change_chance) {
        // grid[i][j].color.r = rand() % 256;
        // grid[i][j].color.g = rand() % 256;
        // grid[i][j].color.b = rand() % 256;
        // OPTION 1
        // grid[i][j].color.r = 255;
        // grid[i][j].color.g = 0;
        // grid[i][j].color.b = 0;
        // // OPTION 2
        // if (rand() % 2 == 0) {
        //   grid[i][j].color = Color(255, 0, 0);
        // } else {
        //   grid[i][j].color = Color(255, 255, 255);
        // }
        // // OPTION 3
        grid[i][j].color = params_.color.value;
        grid[i][j].character = getRandomSCChar();
      }

      // The more the color is far from the target color, the more likely it is
      // to change
      float color_distance =
          std::sqrt(std::pow(grid[i][j].color.r - target_color.r, 2) +
                    std::pow(grid[i][j].color.g - target_color.g, 2) +
                    std::pow(grid[i][j].color.b - target_color.b, 2));
      float adjusted_color_change_chance = 0.4 * delta_time * color_distance *
                                           params_.jitterChance.value / 15.0f;
      if (static_cast<float>(rand()) / RAND_MAX <
          adjusted_color_change_chance) {
        grid[i][j].character = getRandomSCChar();
      }
    }
  }

  int font_height = font.height() - 10;
  int text_height = 0;
  for (int line = 0; line < 11; ++line) {
    auto color = params_.color.value;
    for (int j = 0; j < grid[line].size(); ++j) {
      rgb_matrix::DrawText(offscreen_canvas, font, spacing + j * 10,
                           (spacing + font_height) * (line + 1) + 1,
                           rgb_matrix::Color(grid[line][j].color.r,
                                             grid[line][j].color.g,
                                             grid[line][j].color.b),
                           nullptr, grid[line][j].character.c_str());
    }
  }
  //////////////////

  //   //////////////////
  //   // BARCODE FONT //
  //   //////////////////
  //   font.LoadFont(ASSETS_DIR "/fonts/LibreBarcode128-32.bdf");

  //   int text_height = font.height();
  //   int line = 0;
  //   while (text_height < offscreen_canvas->height()) {
  //     std::string text = getRandomChar();

  //     int text_width = 0;
  //     int x_offset = -(rand() % 100); // Random horizontal offset for each
  //     while (text_width + x_offset < offscreen_canvas->width()) {
  //       text += getRandomChar();
  //       text_width = 0;
  //       for (char c : std::string(text))
  //         text_width += font.CharacterWidth(c);
  //     }

  //     int spacing = 0; // Border spacing from the edges of the matrix
  //     auto color = params_.color.value;
  //     if (line % 2 == 1) {
  //       color = color * 0.9;
  //     }
  //     rgb_matrix::DrawText(
  //         offscreen_canvas, font, spacing + x_offset, spacing +
  //         text_height, rgb_matrix::Color(color.r, color.g, color.b),
  //         nullptr, text.c_str());

  //     text_height += font.height();
  //     line++;
  //   }
  //   //////////////////

  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
  usleep(1000000 / 160); // Cap at 160 FPS to avoid visual flickering
}

} // namespace animations