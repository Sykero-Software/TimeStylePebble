#pragma once
#include <pebble.h>

#define TWT_TASK_NAME_LEN 32
#define TWT_STATUS_PERSIST_KEY 300         // unused by TimeStyle (100=settings, 223=weather, 200=migration)
#define TWT_STATUS_VERSION_PERSIST_KEY 301 // guards against reading a stale blob into a changed layout
#define TWT_STATUS_VERSION 1               // bump whenever the TwtStatus struct layout changes

// Height in px reserved at the bottom of the screen for the status line.
#define TWT_STATUS_HEIGHT 22

typedef struct {
  bool isTracking;
  int32_t taskId;
  char taskName[TWT_TASK_NAME_LEN + 1];
  int32_t workedBeforeMin;   // worked minutes today excluding the running segment
  int32_t segmentStartEpoch; // epoch seconds of current clock-in (0 if not tracking)
} TwtStatus;

extern TwtStatus twt_status;

// true on platforms where we render the status line (rect, non-aplite)
bool TwtStatus_isSupported();

void TwtStatus_load();   // read from persistent storage into twt_status
void TwtStatus_save();   // write twt_status to persistent storage

// Create/destroy the status-line layer as a child of `parent`, occupying the bottom strip.
// The layer is created HIDDEN; main.c shows it only while tracking.
void TwtStatus_initLayer(Layer* parent, GRect frame);
void TwtStatus_deinitLayer();

// Show/hide the status line (used to revert to the original watchface when not tracking).
void TwtStatus_setHidden(bool hidden);

// Reposition the status line (so it can be inset to clear the sidebar while tracking).
void TwtStatus_setFrame(GRect frame);

// Mark the status layer dirty (call on tick + after a message updates twt_status).
void TwtStatus_redraw();
