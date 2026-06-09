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
