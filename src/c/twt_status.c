#include "twt_status.h"
#include "settings.h"

TwtStatus twt_status;

// Two stacked centered lines: the worked time on top, the task name below.
static TextLayer* s_time_text_layer;
static TextLayer* s_name_text_layer;
static char s_time_buffer[16];
static char s_name_buffer[TWT_TASK_NAME_LEN + 1];

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

// Worked time "H:MM", extrapolating the running segment to "now".
// Only shown while tracking (the layers are hidden otherwise), so there is no "stopped" text.
static void build_time_text() {
  int32_t worked = twt_status.workedBeforeMin;
  if (twt_status.isTracking && twt_status.segmentStartEpoch > 0) {
    int32_t running = ((int32_t)time(NULL) - twt_status.segmentStartEpoch) / 60;
    if (running > 0) worked += running;
  }
  int h = worked / 60;
  int m = worked % 60;
  snprintf(s_time_buffer, sizeof(s_time_buffer), "%d:%02d", h, m);
}

static void build_name_text() {
  snprintf(s_name_buffer, sizeof(s_name_buffer), "%s", twt_status.taskName);
}

void TwtStatus_redraw() {
  if (!s_time_text_layer || !s_name_text_layer) return;
  build_time_text();
  build_name_text();
  // track time-color setting changes
  text_layer_set_text_color(s_time_text_layer, settings.timeColor);
  text_layer_set_text_color(s_name_text_layer, settings.timeColor);
  text_layer_set_text(s_time_text_layer, s_time_buffer);
  text_layer_set_text(s_name_text_layer, s_name_buffer);
  layer_mark_dirty(text_layer_get_layer(s_time_text_layer));
  layer_mark_dirty(text_layer_get_layer(s_name_text_layer));
}

// Split the reserved strip into a top half (time) and bottom half (task name).
void TwtStatus_setFrame(GRect frame) {
  if (!s_time_text_layer || !s_name_text_layer) return;
  int half = frame.size.h / 2;
  GRect timeFrame = GRect(frame.origin.x, frame.origin.y, frame.size.w, half);
  GRect nameFrame = GRect(frame.origin.x, frame.origin.y + half, frame.size.w, frame.size.h - half);
  layer_set_frame(text_layer_get_layer(s_time_text_layer), timeFrame);
  layer_set_frame(text_layer_get_layer(s_name_text_layer), nameFrame);
}

static TextLayer* make_status_layer(Layer* parent, GRect frame) {
  TextLayer* layer = text_layer_create(frame);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, settings.timeColor);
  text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(parent, text_layer_get_layer(layer));
  layer_set_hidden(text_layer_get_layer(layer), true); // shown by main.c only while tracking
  return layer;
}

void TwtStatus_initLayer(Layer* parent, GRect frame) {
  if (!TwtStatus_isSupported()) return;
  int half = frame.size.h / 2;
  GRect timeFrame = GRect(frame.origin.x, frame.origin.y, frame.size.w, half);
  GRect nameFrame = GRect(frame.origin.x, frame.origin.y + half, frame.size.w, frame.size.h - half);
  s_time_text_layer = make_status_layer(parent, timeFrame);
  s_name_text_layer = make_status_layer(parent, nameFrame);
  TwtStatus_redraw();
}

void TwtStatus_setHidden(bool hidden) {
  if (s_time_text_layer) {
    layer_set_hidden(text_layer_get_layer(s_time_text_layer), hidden);
  }
  if (s_name_text_layer) {
    layer_set_hidden(text_layer_get_layer(s_name_text_layer), hidden);
  }
}

void TwtStatus_deinitLayer() {
  if (s_time_text_layer) {
    text_layer_destroy(s_time_text_layer);
    s_time_text_layer = NULL;
  }
  if (s_name_text_layer) {
    text_layer_destroy(s_name_text_layer);
    s_name_text_layer = NULL;
  }
}
