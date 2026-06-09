// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "midi_status.h"
#include "twt_status.h"   // reuse TwtStatus_isSupported() for the rect/non-aplite gate
#include "settings.h"

MidiStatus midi_status;

static TextLayer* s_status_text_layer;
static char s_status_buffer[MIDI_DEVICE_NAME_LEN + 24];

void MidiStatus_load() {
  bool versionMatches = persist_exists(MIDI_STATUS_VERSION_PERSIST_KEY)
      && persist_read_int(MIDI_STATUS_VERSION_PERSIST_KEY) == MIDI_STATUS_VERSION;
  if (versionMatches && persist_exists(MIDI_STATUS_PERSIST_KEY)) {
    persist_read_data(MIDI_STATUS_PERSIST_KEY, &midi_status, sizeof(MidiStatus));
  } else {
    midi_status = (MidiStatus){0};
    midi_status.deviceName[0] = '\0';
  }
}

void MidiStatus_save() {
  persist_write_int(MIDI_STATUS_VERSION_PERSIST_KEY, MIDI_STATUS_VERSION);
  persist_write_data(MIDI_STATUS_PERSIST_KEY, &midi_status, sizeof(MidiStatus));
}

// Compose "● m:ss  DeviceName" (h:mm:ss past an hour). Bullet glyph acts as the REC dot.
static void build_status_text() {
  int32_t elapsed = 0;
  if (midi_status.isRecording && midi_status.recStartEpoch > 0) {
    elapsed = (int32_t)time(NULL) - midi_status.recStartEpoch;
    if (elapsed < 0) elapsed = 0;
  }
  int h = elapsed / 3600;
  int m = (elapsed % 3600) / 60;
  int s = elapsed % 60;
  if (h > 0) {
    snprintf(s_status_buffer, sizeof(s_status_buffer), "\xe2\x97\x8f %d:%02d:%02d  %s",
             h, m, s, midi_status.deviceName);
  } else {
    snprintf(s_status_buffer, sizeof(s_status_buffer), "\xe2\x97\x8f %d:%02d  %s",
             m, s, midi_status.deviceName);
  }
}

void MidiStatus_redraw() {
  if (!s_status_text_layer) return;
  build_status_text();
  text_layer_set_text_color(s_status_text_layer, settings.timeColor);
  text_layer_set_text(s_status_text_layer, s_status_buffer);
  layer_mark_dirty(text_layer_get_layer(s_status_text_layer));
}

void MidiStatus_setFrame(GRect frame) {
  if (s_status_text_layer) {
    // main.c reserves a fixed (2-line) status height so the clock size is
    // consistent across status types. MIDI is a single line, so center it
    // vertically within that taller area instead of letting it float at the top.
    int lineH = TWT_STATUS_HEIGHT;   // single-line height
    int dy = (frame.size.h - lineH) / 2;
    if (dy < 0) dy = 0;
    layer_set_frame(text_layer_get_layer(s_status_text_layer),
                    GRect(frame.origin.x, frame.origin.y + dy, frame.size.w, lineH));
  }
}

void MidiStatus_initLayer(Layer* parent, GRect frame) {
  if (!TwtStatus_isSupported()) return;
  s_status_text_layer = text_layer_create(frame);
  text_layer_set_background_color(s_status_text_layer, GColorClear);
  text_layer_set_text_color(s_status_text_layer, settings.timeColor);
  text_layer_set_font(s_status_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_status_text_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(s_status_text_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(parent, text_layer_get_layer(s_status_text_layer));
  layer_set_hidden(text_layer_get_layer(s_status_text_layer), true);
  MidiStatus_redraw();
}

void MidiStatus_setHidden(bool hidden) {
  if (s_status_text_layer) {
    layer_set_hidden(text_layer_get_layer(s_status_text_layer), hidden);
  }
}

void MidiStatus_deinitLayer() {
  if (s_status_text_layer) {
    text_layer_destroy(s_status_text_layer);
    s_status_text_layer = NULL;
  }
}
