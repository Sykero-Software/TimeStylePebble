#include "twt_status.h"
#include "settings.h"

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

  char total_buf[12];
  char task_buf[12];
  snprintf(total_buf, sizeof(total_buf), "%d:%02d", (int)(total / 60), (int)(total % 60));
  snprintf(task_buf, sizeof(task_buf), "%d:%02d", (int)(task / 60), (int)(task % 60));

  graphics_context_set_text_color(ctx, settings.timeColor);

  int mid = b.size.w / 2;
  graphics_draw_text(ctx, total_buf,
      fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
      GRect(0, 0, mid - 2, 30),
      GTextOverflowModeFill, GTextAlignmentRight, NULL);
  graphics_draw_text(ctx, task_buf,
      fonts_get_system_font(FONT_KEY_GOTHIC_18),
      GRect(mid + 3, 8, b.size.w - mid - 3, 22),
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);

  graphics_draw_text(ctx, twt_status.taskName,
      fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
      GRect(0, 30, b.size.w, 32),
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
