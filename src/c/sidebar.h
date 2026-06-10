#pragma once
#include <pebble.h>

extern int sidebarWidth;

// "public" functions
void Sidebar_init(Window* window);
void Sidebar_deinit();
void Sidebar_redraw();
void Sidebar_updateTime(struct tm* timeInfo);

// Frame ownership: while the TWT/status layout is active, main.c owns the panel
// frames and calls these. On unsupported platforms (round/aplite) Sidebar_redraw
// keeps positioning the primary itself.
void Sidebar_setPrimaryFrame(GRect frame);
void Sidebar_setSecondaryFrame(GRect frame);
void Sidebar_setSecondaryHidden(bool hidden);

// Compacts the widget priority list (settings.widgets ++ settings.widgets2,
// EMPTY entries skipped; max 6) and splits it into the two display columns the
// update procs draw. secondaryWanted = the secondary panel may be shown (a
// status display is visible, or the always-on setting). When it is, the list is
// split evenly (extra widget to the primary); otherwise the primary gets the
// first 3 and the tail is dropped. Counts are returned via the out params
// (NULL ok). No-op on round platforms.
void Sidebar_distributeWidgets(bool secondaryWanted, int *primaryCountOut, int *secondaryCountOut);
