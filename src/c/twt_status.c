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

// Top line: day total (big & bold) ending near the centre, with the current-task
// time (smaller) just to its right. Bottom line: the task name, centred. Both
// times add the running segment live (it belongs to the current task). Only drawn
// while tracking (the layer is hidden otherwise).
static void status_update_proc(Layer* layer, GContext* ctx) {
  GRect b = layer_get_bounds(layer);

  int32_t running = 0;
  if (twt_status.isTracking && twt_status.segmentStartEpoch > 0) {
    running = ((int32_t)time(NULL) - twt_status.segmentStartEpoch) / 60;
    if (running < 0) running = 0;
  }
  int32_t total = twt_status.workedBeforeMin + running;
  int32_t task = twt_status.taskWorkedBeforeMin + running;

  int day_pct = twt_percent(total, twt_status.dailyTargetMin);  // -1 -> hide
  int task_pct = twt_percent(task, total);                      // -1 -> hide

  char total_buf[12];
  char task_buf[12];
  char day_pct_buf[8] = "";
  char task_pct_buf[8] = "";
  snprintf(total_buf, sizeof(total_buf), "%d:%02d", (int)(total / 60), (int)(total % 60));
  snprintf(task_buf, sizeof(task_buf), "%d:%02d", (int)(task / 60), (int)(task % 60));
  if (day_pct >= 0)  snprintf(day_pct_buf, sizeof(day_pct_buf), "(%d%%)", day_pct);
  if (task_pct >= 0) snprintf(task_pct_buf, sizeof(task_pct_buf), "(%d%%)", task_pct);

  graphics_context_set_text_color(ctx, settings.timeColor);

  GFont big = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  GFont small = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  GFont pct_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  int mid = b.size.w / 2;

  // --- Left: day total (big) + day percent (small), left-aligned from x=0 ---
  graphics_draw_text(ctx, total_buf, big,
      GRect(0, -2, mid, 30),
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  if (day_pct_buf[0]) {
    GSize tw = graphics_text_layout_get_content_size(total_buf, big,
        GRect(0, 0, mid, 30), GTextOverflowModeFill, GTextAlignmentLeft);
    graphics_draw_text(ctx, day_pct_buf, pct_font,
        GRect(tw.w + 2, 6, mid - tw.w - 2, 18),
        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }

  // --- Right: task time (small) + task percent (small), left-aligned from mid ---
  graphics_draw_text(ctx, task_buf, small,
      GRect(mid + 2, 2, b.size.w - mid - 2, 22),
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  if (task_pct_buf[0]) {
    GSize tw2 = graphics_text_layout_get_content_size(task_buf, small,
        GRect(mid + 2, 0, b.size.w - mid - 2, 22), GTextOverflowModeFill, GTextAlignmentLeft);
    graphics_draw_text(ctx, task_pct_buf, pct_font,
        GRect(mid + 2 + tw2.w + 2, 6, b.size.w - (mid + 2 + tw2.w + 2), 18),
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

  // --- Bottom line: task name, centred ---
  graphics_draw_text(ctx, twt_status.taskName, big,
      GRect(0, 32, b.size.w, 30),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
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
