#include "twt_status.h"
#include "settings.h"

TwtStatus twt_status;

static TextLayer* s_status_text_layer;
static char s_status_buffer[TWT_TASK_NAME_LEN + 16];

bool TwtStatus_isSupported() {
#if defined(PBL_RECT) && !defined(PBL_PLATFORM_APLITE)
  return true;
#else
  return false;
#endif
}

void TwtStatus_load() {
  if (persist_exists(TWT_STATUS_PERSIST_KEY)) {
    persist_read_data(TWT_STATUS_PERSIST_KEY, &twt_status, sizeof(TwtStatus));
  } else {
    twt_status = (TwtStatus){0};
    twt_status.taskName[0] = '\0';
  }
}

void TwtStatus_save() {
  persist_write_data(TWT_STATUS_PERSIST_KEY, &twt_status, sizeof(TwtStatus));
}

// Compose "TaskName  H:MM" extrapolating the running segment to "now".
// Only shown while tracking (the layer is hidden otherwise), so there is no "stopped" text.
static void build_status_text() {
  int32_t worked = twt_status.workedBeforeMin;
  if (twt_status.isTracking && twt_status.segmentStartEpoch > 0) {
    int32_t running = ((int32_t)time(NULL) - twt_status.segmentStartEpoch) / 60;
    if (running > 0) worked += running;
  }
  int h = worked / 60;
  int m = worked % 60;
  snprintf(s_status_buffer, sizeof(s_status_buffer), "%s  %d:%02d",
           twt_status.taskName, h, m);
}

void TwtStatus_redraw() {
  if (!s_status_text_layer) return;
  build_status_text();
  text_layer_set_text(s_status_text_layer, s_status_buffer);
  layer_mark_dirty(text_layer_get_layer(s_status_text_layer));
}

void TwtStatus_initLayer(Layer* parent, GRect frame) {
  if (!TwtStatus_isSupported()) return;
  s_status_text_layer = text_layer_create(frame);
  text_layer_set_background_color(s_status_text_layer, GColorClear);
  text_layer_set_text_color(s_status_text_layer, settings.timeColor);
  text_layer_set_font(s_status_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_status_text_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_status_text_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(parent, text_layer_get_layer(s_status_text_layer));
  layer_set_hidden(text_layer_get_layer(s_status_text_layer), true); // shown by main.c only while tracking
  TwtStatus_redraw();
}

void TwtStatus_setHidden(bool hidden) {
  if (s_status_text_layer) {
    layer_set_hidden(text_layer_get_layer(s_status_text_layer), hidden);
  }
}

void TwtStatus_deinitLayer() {
  if (s_status_text_layer) {
    text_layer_destroy(s_status_text_layer);
    s_status_text_layer = NULL;
  }
}
