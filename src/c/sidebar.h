#pragma once
#include <pebble.h>

extern int sidebarWidth;

// Per-column vertical padding (top and bottom). Shared with main.c so it can
// compute the per-column inner height available for widget packing.
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define V_PADDING_DEFAULT 9
#else
#define V_PADDING_DEFAULT 8
#endif

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

// Packs settings.widgetList into the primary column (filled first) and, when
// allowSecondary is set and there is overflow, the secondary column. innerHeight
// is the per-column height available for widgets (frame height minus padding).
// Returns true when the secondary column receives >=1 widget.
bool Sidebar_distributeWidgets(int innerHeight, bool allowSecondary,
                               int *primaryCountOut, int *secondaryCountOut);
