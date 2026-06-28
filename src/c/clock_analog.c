// Analog clock face for TimeStyle.
//
// Ported from the watchface truhanen/pebble-nyquist-watchface
// (https://github.com/truhanen/pebble-nyquist-watchface), GPL-3.0. TimeStyle is
// itself GPL-3.0, so this reuse is license-compatible. Only the clock-face
// drawing (hands + ticks + pivot) is reused; nyquist's corner widgets are not.
//
// Drawn with the plain Pebble GContext graphics API (gpath + graphics_draw_line)
// in LAYER-LOCAL coordinates, so — unlike the FCTX digital path in
// clock_area.c — the layer frame origin is honored automatically and no manual
// offset compensation is needed.
#ifndef PBL_PLATFORM_APLITE
#include <pebble.h>
#include "clock_analog.h"

// integer square root
static int32_t isqrt32(int32_t n) {
  if (n <= 0) return 0;
  int32_t x = n, y = (x + 1) / 2;
  while (y < x) { x = y; y = (x + n / x) / 2; }
  return x;
}

// inner tick point inset by tick_len toward center
static GPoint tick_inner(GPoint outer, GPoint center, int tick_len) {
  int32_t dx = center.x - outer.x;
  int32_t dy = center.y - outer.y;
  int32_t dist = isqrt32(dx * dx + dy * dy);
  if (dist == 0) return outer;
  return (GPoint){
    outer.x + (int32_t)((int64_t)dx * tick_len / dist),
    outer.y + (int32_t)((int64_t)dy * tick_len / dist)
  };
}

// draw all 12 tick marks: halo then body, fixed radius
static void draw_all_ticks(GContext *ctx, GPoint center, int tick_r, int tick_len,
                           int tick_halo_width, int tick_width,
                           GColor halo_color, GColor body_color) {
  for (int i = 0; i < 12; i++) {
    int32_t angle = i * TRIG_MAX_ANGLE / 12;
    int32_t sin_a = sin_lookup(angle);
    int32_t cos_a = cos_lookup(angle);

    GPoint outer = {
      center.x + (int32_t)((int64_t)sin_a * tick_r / TRIG_MAX_RATIO),
      center.y - (int32_t)((int64_t)cos_a * tick_r / TRIG_MAX_RATIO),
    };
    GPoint inner = tick_inner(outer, center, tick_len);

    graphics_context_set_stroke_color(ctx, halo_color);
    graphics_context_set_stroke_width(ctx, tick_halo_width);
    graphics_draw_line(ctx, outer, inner);
    graphics_context_set_stroke_color(ctx, body_color);
    graphics_context_set_stroke_width(ctx, tick_width);
    graphics_draw_line(ctx, outer, inner);
  }
}

// pixel-perfect trig offset helpers
static int32_t prv_trig_offset_trunc(int32_t value, int32_t trig) {
  return ((int64_t)value * trig) / TRIG_MAX_RATIO;
}

static int32_t prv_trig_offset_compensated(int32_t value, int32_t trig) {
  int64_t scaled = (int64_t)value * trig;
  int32_t base = scaled / TRIG_MAX_RATIO;
  if (scaled % TRIG_MAX_RATIO != 0) {
    base += (scaled > 0 ? 1 : -1);
  }
  return base;
}

static int64_t prv_abs64(int64_t v) {
  return v < 0 ? -v : v;
}

// pentagon outline of a hand
static void draw_hand_border(GContext *ctx, GPoint center, int32_t angle,
                             int outer_dist, int half_width, int apex_ext,
                             int tail, int stroke_w, GColor color) {
  int32_t sin_a = sin_lookup(angle);
  int32_t cos_a = cos_lookup(angle);

  GPoint inner_pt = {
    center.x - (int32_t)tail * sin_a / TRIG_MAX_RATIO,
    center.y + (int32_t)tail * cos_a / TRIG_MAX_RATIO,
  };
  GPoint outer_pt = {
    center.x + prv_trig_offset_compensated(outer_dist, sin_a),
    center.y - prv_trig_offset_compensated(outer_dist, cos_a),
  };

  int32_t x_diff_left  = prv_trig_offset_trunc(half_width, cos_a);
  int32_t y_diff_left  = prv_trig_offset_trunc(half_width, sin_a);
  int32_t x_diff_right = prv_trig_offset_compensated(half_width, cos_a);
  int32_t y_diff_right = prv_trig_offset_compensated(half_width, sin_a);

  GPoint inner_right = { inner_pt.x + x_diff_right, inner_pt.y + y_diff_right };
  GPoint inner_left  = { inner_pt.x - x_diff_left,  inner_pt.y - y_diff_left };
  GPoint outer_right = { outer_pt.x + x_diff_right, outer_pt.y + y_diff_right };
  GPoint outer_left  = { outer_pt.x - x_diff_left,  outer_pt.y - y_diff_left };
  GPoint apex        = {
    outer_pt.x + prv_trig_offset_compensated(apex_ext, sin_a),
    outer_pt.y - prv_trig_offset_compensated(apex_ext, cos_a),
  };

  // Shift the inner edge by up to 1px to keep the hand axis through center.
  int32_t inner_mid_x = (inner_left.x + inner_right.x) / 2;
  int32_t inner_mid_y = (inner_left.y + inner_right.y) / 2;
  int32_t outer_mid_x = (outer_left.x + outer_right.x) / 2;
  int32_t outer_mid_y = (outer_left.y + outer_right.y) / 2;
  int32_t axis_dx = outer_mid_x - inner_mid_x;
  int32_t axis_dy = outer_mid_y - inner_mid_y;

  if (axis_dx != 0 || axis_dy != 0) {
    const int shifts[5][2] = { {0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
    int best_index = 0;
    int64_t best_error = -1;

    for (int i = 0; i < 5; i++) {
      int32_t sx = shifts[i][0];
      int32_t sy = shifts[i][1];
      int32_t test_inner_x = inner_mid_x + sx;
      int32_t test_inner_y = inner_mid_y + sy;
      int64_t cross = (int64_t)axis_dx * (center.y - test_inner_y)
                    - (int64_t)axis_dy * (center.x - test_inner_x);
      int64_t err = prv_abs64(cross);
      if (best_error < 0 || err < best_error) {
        best_error = err;
        best_index = i;
      }
    }

    if (best_index != 0) {
      int32_t sx = shifts[best_index][0];
      int32_t sy = shifts[best_index][1];
      inner_left.x += sx;
      inner_left.y += sy;
      inner_right.x += sx;
      inner_right.y += sy;
    }
  }

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, stroke_w);
  graphics_draw_line(ctx, inner_left,  inner_right);
  graphics_draw_line(ctx, inner_right, outer_right);
  graphics_draw_line(ctx, inner_left,  outer_left);
  graphics_draw_line(ctx, outer_right, apex);
  graphics_draw_line(ctx, outer_left,  apex);
}

// filled pentagon body of a hand
static void draw_hand_fill(GContext *ctx, GPoint center, int32_t angle,
                           int length, int width, int tail, GColor color) {
  int hw = width / 2;
  GPoint pts[5] = {
    {-hw, tail}, {hw, tail}, {hw, -length}, {0, -(length + hw)}, {-hw, -length}
  };
  GPathInfo info = { .num_points = 5, .points = pts };
  GPath *path = gpath_create(&info);
  gpath_rotate_to(path, angle);
  gpath_move_to(path, center);
  graphics_context_set_fill_color(ctx, color);
  gpath_draw_filled(ctx, path);
  gpath_destroy(path);
}

// Scale an emery-reference dimension (a percent of a 100px half-screen) to this
// face's `half`; floor strokes at 1px when floor1 is set.
static int dim(int half, int pct, int floor1) {
  int v = half * pct / 100;
  if (floor1 && v < 1) v = 1;
  return v;
}

void ClockAnalog_draw(GContext *ctx, GRect bounds, int hours, int minutes,
                      GColor fg, GColor bg, bool show_ticks) {
  int w = bounds.size.w;
  int h = bounds.size.h;
  GPoint center = GPoint(bounds.origin.x + w / 2, bounds.origin.y + h / 2);
  int half = (w < h ? w : h) / 2;

  // Dimensions derived from `half` using nyquist's emery ratios (emery half~100).
  int tick_r         = dim(half, 85, 0);
  int minute_len     = dim(half, 89, 0);
  int hour_len       = dim(half, 60, 0);
  int hand_edge_w    = dim(half, 12, 1);
  int hand_halo_w    = dim(half, 2,  1);
  int minute_outer_w = dim(half, 24, 0);
  int minute_fill_w  = dim(half, 12, 0);
  int hour_outer_w   = dim(half, 33, 0);
  int hour_fill_w    = dim(half, 22, 0);
  int hand_tail      = dim(half, 18, 0);
  int tick_w         = dim(half, 2,  1);
  int tick_halo_w    = dim(half, 6,  1);
  int tick_len       = dim(half, 6,  1);
  int pivot_r        = dim(half, 3,  1);

  int32_t min_angle  = minutes * TRIG_MAX_ANGLE / 60;
  int32_t hour_angle = (hours % 12) * TRIG_MAX_ANGLE / 12
                       + minutes * TRIG_MAX_ANGLE / 720;

  // background (matches the window bg; also needed for the halo/pivot gaps)
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // hour hand
  draw_hand_border(ctx, center, hour_angle,
                   hour_len - hand_edge_w / 2,
                   hour_outer_w / 2 - hand_edge_w / 2,
                   hour_outer_w / 2 - hand_edge_w / 2,
                   hand_tail, hand_edge_w, fg);
  draw_hand_fill(ctx, center, hour_angle,
                 hour_len - hand_edge_w / 2 - 1, hour_fill_w, hand_tail, fg);

  // minute hand: halo border, body border, fill
  draw_hand_border(ctx, center, min_angle,
                   minute_len - hand_edge_w / 2,
                   minute_outer_w / 2 - hand_edge_w / 2,
                   minute_outer_w / 2 - hand_edge_w / 2,
                   hand_tail, hand_edge_w + 2 * hand_halo_w, bg);
  draw_hand_border(ctx, center, min_angle,
                   minute_len - hand_edge_w / 2,
                   minute_outer_w / 2 - hand_edge_w / 2,
                   minute_outer_w / 2 - hand_edge_w / 2,
                   hand_tail, hand_edge_w, fg);
  draw_hand_fill(ctx, center, min_angle,
                 minute_len - hand_edge_w / 2 - 1, minute_fill_w, hand_tail, fg);

  // center pivot
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_circle(ctx, center, pivot_r);

  // ticks last, so they sit on top of the hands (optional)
  if (show_ticks) {
    draw_all_ticks(ctx, center, tick_r, tick_len, tick_halo_w, tick_w, bg, fg);
  }
}
#endif  // !PBL_PLATFORM_APLITE
