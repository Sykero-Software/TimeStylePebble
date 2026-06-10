// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "twt_status.h"
#include "settings.h"
#include "twt_calc.h"

TwtStatus twt_status;

static Layer* s_status_layer;

bool TwtStatus_isSupported() {
#if defined(PBL_RECT) && !defined(PBL_PLATFORM_APLITE)
  return true;
#else
  return false;
#endif
}

void TwtStatus_load() {
  bool versionMatches = persist_exists(TWT_STATUS_VERSION_PERSIST_KEY)
      && persist_read_int(TWT_STATUS_VERSION_PERSIST_KEY) == TWT_STATUS_VERSION;
  if (versionMatches && persist_exists(TWT_STATUS_PERSIST_KEY)) {
    persist_read_data(TWT_STATUS_PERSIST_KEY, &twt_status, sizeof(TwtStatus));
  } else {
    // no data, or a blob from a different struct layout -> start clean
    twt_status = (TwtStatus){0};
    twt_status.taskName[0] = '\0';
  }
}

void TwtStatus_save() {
  persist_write_int(TWT_STATUS_VERSION_PERSIST_KEY, TWT_STATUS_VERSION);
  persist_write_data(TWT_STATUS_PERSIST_KEY, &twt_status, sizeof(TwtStatus));
}

// Line 1: the day total (big & bold) with its percent of the configured workday target in
// parentheses just after it, spanning the whole strip width. A thin bar below shows workday
// completion (only when a target is set). Line 2: the task name (left, truncated) with the
// current-task time right-aligned to the edge (no task percent — TWT Control shows those per
// task). The task time is on line 2 (not crammed onto line 1) so a legible font fits without
// clipping on 144 px. Both times add the running segment live (it belongs to the current
// task); the day percent is hidden when its base is 0. When settings.twtShowRemaining is set
// (and a target exists), the big day-total number instead shows remaining = target - worked
// (negative on overtime); the percent and the bar always reflect worked progress, regardless
// of that toggle.
int32_t TwtStatus_workedTotalMin(void) {
  int32_t running = 0;
  if (twt_status.isTracking && twt_status.segmentStartEpoch > 0) {
    running = ((int32_t)time(NULL) - twt_status.segmentStartEpoch) / 60;
    if (running < 0) running = 0;
  }
  return twt_status.workedBeforeMin + running;
}

static void status_update_proc(Layer* layer, GContext* ctx) {
  GRect b = layer_get_bounds(layer);

  int32_t total = TwtStatus_workedTotalMin();
  int32_t running = total - twt_status.workedBeforeMin;            // live current segment, whole minutes
  int32_t task_today = twt_status.taskWorkedBeforeMin + running;   // today's task time
  int32_t task_total = twt_status.taskTotalBeforeMin + running;    // all-time task time

  int day_pct = twt_percent(total, twt_status.dailyTargetMin);     // -1 -> hide
  // budgeted: all-time total; unbudgeted: today's time
  int32_t task_shown = (twt_status.taskBudgetMin > 0) ? task_total : task_today;

  char total_buf[12];
  char task_buf[12];
  char day_pct_buf[12] = "";
  int32_t shown_total = total;                       // default: worked time
  if (settings.twtShowRemaining && twt_status.dailyTargetMin > 0) {
    shown_total = twt_status.dailyTargetMin - total; // remaining; negative on overtime
  }
  twt_fmt_hhmm_signed(total_buf, sizeof(total_buf), shown_total);
  snprintf(task_buf, sizeof(task_buf), "%d:%02d", (int)(task_shown / 60), (int)(task_shown % 60));
  if (day_pct >= 0)  snprintf(day_pct_buf, sizeof(day_pct_buf), "(%d%%)", day_pct > 999 ? 999 : day_pct);

  graphics_context_set_text_color(ctx, settings.timeColor);

  GFont big = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  GFont small = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

  // --- Line 1: day total (big) + day percent (small) after it, centred together on the strip. ---
  // The day percent (worked / daily target) is the headline metric and gets line 1 to itself;
  // the task time + percent move to line 2 so a legible font fits without clipping on 144 px.
  GSize tot_sz = graphics_text_layout_get_content_size(total_buf, big,
      GRect(0, 0, b.size.w, 30), GTextOverflowModeFill, GTextAlignmentLeft);
  int pct_gap_w = 0;
  if (day_pct_buf[0]) {
    GSize pct_sz = graphics_text_layout_get_content_size(day_pct_buf, small,
        GRect(0, 0, b.size.w, 22), GTextOverflowModeFill, GTextAlignmentLeft);
    pct_gap_w = 4 + pct_sz.w;
  }
  int line1_x = (b.size.w - (tot_sz.w + pct_gap_w)) / 2;
  if (line1_x < 0) line1_x = 0;
  graphics_draw_text(ctx, total_buf, big,
      GRect(line1_x, -2, b.size.w - line1_x, 30),
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  if (day_pct_buf[0]) {
    int px = line1_x + tot_sz.w + 4;
    graphics_draw_text(ctx, day_pct_buf, small,
        GRect(px, 6, b.size.w - px, 22),
        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }

  // --- Thin workday-completion bar (only when a target exists) ---
  if (twt_status.dailyTargetMin > 0) {
    int bar_y = 28, bar_h = 3, bar_w = b.size.w;
    int fill = twt_bar_fill_px(total, twt_status.dailyTargetMin, bar_w);
    graphics_context_set_stroke_color(ctx, settings.timeColor);
    graphics_draw_rect(ctx, GRect(0, bar_y, bar_w, bar_h));
    graphics_context_set_fill_color(ctx, settings.timeColor);
    graphics_fill_rect(ctx, GRect(0, bar_y, fill, bar_h), 0, GCornerNone);
  }

  // --- Line 2: task name (left, truncated) + task time (right-aligned). ---
  GSize tl = graphics_text_layout_get_content_size(task_buf, small,
      GRect(0, 0, b.size.w, 22), GTextOverflowModeFill, GTextAlignmentLeft);
  int task_x = b.size.w - tl.w;
  if (task_x < 0) task_x = 0;
  graphics_draw_text(ctx, task_buf, small,
      GRect(task_x, 40, b.size.w - task_x, 22),
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, twt_status.taskName, big,
      GRect(0, 32, task_x - 2, 30),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

void TwtStatus_redraw() {
  if (s_status_layer) layer_mark_dirty(s_status_layer);
}

void TwtStatus_setFrame(GRect frame) {
  if (s_status_layer) layer_set_frame(s_status_layer, frame);
}

void TwtStatus_initLayer(Layer* parent, GRect frame) {
  if (!TwtStatus_isSupported()) return;
  s_status_layer = layer_create(frame);
  layer_set_update_proc(s_status_layer, status_update_proc);
  layer_add_child(parent, s_status_layer);
  layer_set_hidden(s_status_layer, true); // shown by main.c only while tracking
}

void TwtStatus_setHidden(bool hidden) {
  if (s_status_layer) layer_set_hidden(s_status_layer, hidden);
}

void TwtStatus_deinitLayer() {
  if (s_status_layer) {
    layer_destroy(s_status_layer);
    s_status_layer = NULL;
  }
}
