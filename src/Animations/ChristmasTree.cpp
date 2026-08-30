#include "Animations/ChristmasTree.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "stb_image.h"

namespace animations {

void ChristmasTree::animate(double time) {
  // We'll load a set of 4 images and crossfade between them.
  static bool images_loaded = false;
  static const std::vector<std::string> filenames = {
      ASSETS_DIR "/img/image-4-1.png", ASSETS_DIR "/img/image-4-2.png",
      ASSETS_DIR "/img/image-4-3.png", ASSETS_DIR "/img/image-4-4.png"};

  // Per-image storage
  static std::vector<std::vector<uint8_t>> images_rgb;
  static std::vector<int> img_widths;
  static std::vector<int> img_heights;

  if (!images_loaded) {
    images_rgb.clear();
    img_widths.clear();
    img_heights.clear();

    for (const auto &fn : filenames) {
      int w = 0, h = 0, channels = 0;
      unsigned char *img = stbi_load(fn.c_str(), &w, &h, &channels, 3);
      if (img && w > 0 && h > 0) {
        size_t sz = static_cast<size_t>(w) * static_cast<size_t>(h) * 3;
        images_rgb.emplace_back(img, img + sz);
        img_widths.push_back(w);
        img_heights.push_back(h);
        stbi_image_free(img);
      } else {
        // If a specific image fails to load, push an empty placeholder so
        // indices remain aligned.
        images_rgb.emplace_back();
        img_widths.push_back(0);
        img_heights.push_back(0);
        if (img) {
          stbi_image_free(img);
        }
      }
    }

    // Check at least one image loaded successfully
    bool any = false;
    for (const auto &v : images_rgb) {
      if (!v.empty()) {
        any = true;
        break;
      }
    }
    if (!any) {
      // Nothing to display
      offscreen_canvas->Clear();
      offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
      images_loaded = true; // avoid retrying every frame
      return;
    }

    images_loaded = true;
  }

  // Timing parameters: read from animation parameters so they are configurable
  const double hold_seconds = static_cast<double>(params_.hold_seconds.value);
  const double fade_seconds = static_cast<double>(params_.fade_seconds.value);
  const int num_images = static_cast<int>(images_rgb.size());

  // Preserve animation phase when parameters change by maintaining an
  // offset applied to the incoming time. When hold/fade change we adjust
  // this offset so the visible position (time within the cycle) stays the
  // same, avoiding jumps.
  static double time_offset = 0.0;
  static double prev_hold = hold_seconds;
  static double prev_fade = fade_seconds;
  static bool params_initialized = false;

  const double segment = hold_seconds + fade_seconds; // per-image segment
  const double cycle = segment * num_images;

  if (!params_initialized) {
    // Initialize tracking values on first use
    prev_hold = hold_seconds;
    prev_fade = fade_seconds;
    params_initialized = true;
  } else if (hold_seconds != prev_hold || fade_seconds != prev_fade) {
    // Parameters changed: compute old and new cycles and adjust time_offset
    const double old_segment = prev_hold + prev_fade;
    const double old_cycle = old_segment * num_images;
    const double new_cycle = cycle;

    // Protect against zero-length cycles
    const double safe_old = (old_cycle <= 0.0) ? 1e-6 : old_cycle;
    const double safe_new = (new_cycle <= 0.0) ? 1e-6 : new_cycle;

    // Absolute phase time before change
    double abs_phase = time + time_offset;

    // current position in old cycle
    double old_t = fmod(abs_phase, safe_old);
    if (old_t < 0)
      old_t += safe_old;

    // Determine which image index we were on and the position inside that
    // per-image segment. We must preserve whether we were in the hold or
    // the fade subsegment and the fractional progress within that
    // subsegment, then map that to the new hold/fade lengths. This avoids
    // visible jumps when changing parameters during a crossfade.
    double old_segment_safe = (old_segment <= 0.0) ? 1e-6 : old_segment;
    int cur_index_old = static_cast<int>(old_t / old_segment_safe) % num_images;
    double t_in_segment_old = fmod(old_t, old_segment_safe);
    if (t_in_segment_old < 0)
      t_in_segment_old += old_segment_safe;

    // Previous hold/fade lengths (prev_hold/prev_fade) may be zero.
    double prev_hold_safe = (prev_hold <= 0.0) ? 0.0 : prev_hold;
    double prev_fade_safe = (prev_fade <= 0.0) ? 0.0 : prev_fade;

    double new_hold = hold_seconds;
    double new_fade = fade_seconds;
    double new_segment_safe =
        (new_hold + new_fade <= 0.0) ? 1e-6 : (new_hold + new_fade);

    double new_t_in_seg = 0.0;

    if (prev_fade_safe > 0.0 && t_in_segment_old >= prev_hold_safe) {
      // We were in the fade portion.
      double old_fade_pos = t_in_segment_old - prev_hold_safe;
      double frac_in_fade = old_fade_pos / prev_fade_safe; // in [0,1]
      double new_fade_safe = (new_fade <= 0.0) ? 0.0 : new_fade;
      double new_fade_pos = frac_in_fade * new_fade_safe;
      new_t_in_seg = new_hold + new_fade_pos;
    } else {
      // We were in the hold portion (or previous hold was zero and we treat
      // as hold start). Preserve fractional position within hold.
      double old_hold_safe = (prev_hold_safe <= 0.0) ? 0.0 : prev_hold_safe;
      double frac_in_hold =
          (old_hold_safe > 0.0) ? (t_in_segment_old / old_hold_safe) : 0.0;
      double new_hold_safe = (new_hold <= 0.0) ? 0.0 : new_hold;
      double new_hold_pos = frac_in_hold * new_hold_safe;
      new_t_in_seg = new_hold_pos;
    }

    // Compose new time within the new cycle keeping the same image index
    double new_t =
        static_cast<double>(cur_index_old) * new_segment_safe + new_t_in_seg;

    // Compute current time mod new cycle and choose an offset so
    // (time + time_offset) mod new_cycle == new_t.
    double time_mod_new = fmod(time, safe_new);
    if (time_mod_new < 0)
      time_mod_new += safe_new;

    time_offset = new_t - time_mod_new;

    // Update previous values
    prev_hold = hold_seconds;
    prev_fade = fade_seconds;
  }

  // Compute which image and next image we are between using adjusted time
  double t = fmod(time + time_offset, (cycle <= 0.0) ? 1e-6 : cycle);
  if (t < 0)
    t += (cycle <= 0.0) ? 1e-6 : cycle;

  int cur_index = static_cast<int>(t / segment) % num_images;
  double t_in_segment = fmod(t, segment);
  double alpha = 0.0; // progression from current -> next
  if (t_in_segment >= hold_seconds && fade_seconds > 0.0) {
    alpha = (t_in_segment - hold_seconds) / fade_seconds;
    if (alpha > 1.0)
      alpha = 1.0;
  }

  int next_index = (cur_index + 1) % num_images;

  // Prepare canvas
  int matrix_width = matrix->width();
  int matrix_height = matrix->height();
  offscreen_canvas->Clear();

  // Precompute display widths/heights and centering offsets for each image
  // so we don't recompute inside the per-pixel loop.
  const double EPS_TIME = 1e-6;
  std::vector<int> display_widths(num_images), display_heights(num_images);
  std::vector<int> offset_xs(num_images), offset_ys(num_images);
  for (int i = 0; i < num_images; ++i) {
    int w = img_widths[i];
    int h = img_heights[i];
    display_widths[i] = (w > 0) ? std::min(w, matrix_width) : 0;
    display_heights[i] = (h > 0) ? std::min(h, matrix_height) : 0;
    offset_xs[i] = (matrix_width - display_widths[i]) / 2;
    offset_ys[i] = (matrix_height - display_heights[i]) / 2;
  }

  // Small helper that samples a pixel (r,g,b) from image `idx` at matrix
  // coordinates (x,y). If outside the image, returns black (0,0,0).
  auto sample_from = [&](int idx, int x, int y, uint8_t &r, uint8_t &g,
                         uint8_t &b) {
    r = g = b = 0;
    if (idx < 0 || idx >= num_images)
      return;
    if (images_rgb[idx].empty())
      return;
    int disp_w = display_widths[idx];
    int disp_h = display_heights[idx];
    if (disp_w <= 0 || disp_h <= 0)
      return;
    int ox = offset_xs[idx];
    int oy = offset_ys[idx];
    int sx = x - ox;
    int sy = y - oy;
    if (sx < 0 || sx >= disp_w || sy < 0 || sy >= disp_h)
      return;
    // Map directly to image coordinates (no scaling)
    int img_w = img_widths[idx];
    int img_x = sx;
    int img_y = sy;
    int pos = (img_y * img_w + img_x) * 3;
    if (pos + 2 < static_cast<int>(images_rgb[idx].size())) {
      r = images_rgb[idx][pos];
      g = images_rgb[idx][pos + 1];
      b = images_rgb[idx][pos + 2];
    }
  };

  // For each pixel on the matrix, sample from current and next images,
  // blend and write to the canvas.
  for (int y = 0; y < matrix_height; ++y) {
    for (int x = 0; x < matrix_width; ++x) {
      uint8_t r_cur, g_cur, b_cur;
      uint8_t r_next, g_next, b_next;
      sample_from(cur_index, x, y, r_cur, g_cur, b_cur);
      sample_from(next_index, x, y, r_next, g_next, b_next);

      // Blend: out = (1-alpha) * cur + alpha * next
      uint8_t r_out = static_cast<uint8_t>(
          std::min(255.0, (1.0 - alpha) * r_cur + alpha * r_next + 0.5));
      uint8_t g_out = static_cast<uint8_t>(
          std::min(255.0, (1.0 - alpha) * g_cur + alpha * g_next + 0.5));
      uint8_t b_out = static_cast<uint8_t>(
          std::min(255.0, (1.0 - alpha) * b_cur + alpha * b_next + 0.5));

      offscreen_canvas->SetPixel(x, y, r_out, g_out, b_out);
    }
  }

  // Swap the canvas to display
  offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
}

} // namespace animations
