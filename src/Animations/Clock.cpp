#include "Animations/Clock.h"
#include "graphics.h"
#include <chrono>
#include <cstdlib>
#include <iostream>

namespace animations {

void Clock::animate(double _) {
  // Get current time
  time_t now = time(nullptr);

  struct tm result;
  struct tm *local_time = localtime_r(&now, &result);
  localtime_r(&now, &result);
  int hours = local_time->tm_hour;
  int minutes = local_time->tm_min;
  int seconds = local_time->tm_sec;
  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count() %
                     1000;

  // std::cout << "Current time: " << hours << ":" << minutes << ":" << seconds
  //           << "." << milliseconds << std::endl;

  offscreen_canvas->Clear();

  int center_x = offscreen_canvas->width() / 4 * 3;
  int center_y = offscreen_canvas->height() / 4 * 3;

  // Draw the time as HH:MM:SS
  char text[9];
  std::snprintf(text, sizeof(text), "%02d:%02d:%02d", hours, minutes, seconds);
  int text_width = 0;
  for (char c : std::string(text))
    text_width += font.CharacterWidth(c);
  int text_height = font.height();
  rgb_matrix::DrawText(offscreen_canvas, font, center_x - text_width / 2,
                       center_y - 3 + text_height / 2,
                       rgb_matrix::Color(255, 255, 255), nullptr, text);

  // Draw a circle for each second
  float radius = 29.5 * 1;
  for (int i = 0; i < seconds; ++i) {
    double angle = (i / 60.0) * 2 * M_PI - M_PI / 2; // Start from top
    int x = center_x + radius * cos(angle);
    int y = center_y + radius * sin(angle);
    offscreen_canvas->SetPixel(x, y, 255, 0, 0); // Green dots
  }

  // // Draw clock hands
  // // Hour hand
  // double hour_angle = ((hours % 12) / 12.0) * 2 * M_PI - M_PI / 2;
  // int hour_x = center_x + (radius - 15) * cos(hour_angle);
  // int hour_y = center_y + (radius - 15) * sin(hour_angle);
  // rgb_matrix::DrawLine(offscreen_canvas, center_x, center_y, hour_x, hour_y,
  //                      rgb_matrix::Color(255, 0, 0));
  // // Minute hand
  // double minute_angle = (minutes / 60.0) * 2 * M_PI - M_PI / 2;
  // int minute_x = center_x + (radius - 10) * cos(minute_angle);
  // int minute_y = center_y + (radius - 10) * sin(minute_angle);
  // rgb_matrix::DrawLine(offscreen_canvas, center_x, center_y, minute_x,
  // minute_y,
  //                      rgb_matrix::Color(0, 255, 0));
  // // Second hand
  // double second_angle = (seconds / 60.0) * 2 * M_PI - M_PI / 2;
  // int second_x = center_x + radius * cos(second_angle);
  // int second_y = center_y + radius * sin(second_angle);
  // rgb_matrix::DrawLine(offscreen_canvas, center_x, center_y, second_x,
  // second_y,
  //                      rgb_matrix::Color(0, 0, 255));

  // Swap the offscreen_canvas with matrix on vsync, avoids flickering
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations