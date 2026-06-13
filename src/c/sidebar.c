#include "sidebar.h"
#include "languages.h"
#include "settings.h"
#include "sidebar_widgets.h"
#include "weather.h"
#include "twt_status.h"
#include <ctype.h>
#include <math.h>
#include <pebble.h>

GRect screen_rect;
int sidebarWidth;

static void update_sidebar_width() {
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
  sidebarWidth = settings.useLargeFonts ? 39 : 34;
#else
  sidebarWidth = 30;
#endif
}

// "private" functions
// layer update callbacks
#ifndef PBL_ROUND
void updateRectSidebar(Layer *l, GContext *ctx);
void updateRectSecondarySidebar(Layer *l, GContext *ctx);
#else

void updateRoundSidebarLeft(Layer *l, GContext *ctx);
void updateRoundSidebarRight(Layer *l, GContext *ctx);

// shared drawing stuff between all layers
void drawRoundSidebar(GContext *ctx, GRect bgBounds,
                      SidebarWidgetType widgetType, int widgetXOffset);
#endif

Layer *sidebarLayer;

#ifndef PBL_ROUND
Layer *secondarySidebarLayer;   // right column; shown when rightWidgetList is non-empty
#endif

#ifdef PBL_ROUND
Layer *sidebarLayer2;
#endif

void Sidebar_init(Window *window) {
  update_sidebar_width();
  // init the sidebar layer
  screen_rect = layer_get_bounds(window_get_root_layer(window));
  GRect bounds;

#ifdef PBL_ROUND
  GRect bounds2;
  bounds = GRect(0, 0, 40, screen_rect.size.h);
  bounds2 = GRect(screen_rect.size.w - 40, 0, 40, screen_rect.size.h);
#else
  if (!settings.sidebarOnLeft) {
    bounds = GRect(screen_rect.size.w - sidebarWidth, 0, sidebarWidth,
                   screen_rect.size.h);
  } else {
    bounds = GRect(0, 0, sidebarWidth, screen_rect.size.h);
  }
#endif

  // init the widgets
  SidebarWidgets_init();

  sidebarLayer = layer_create(bounds);
  layer_add_child(window_get_root_layer(window), sidebarLayer);

#ifdef PBL_ROUND
  layer_set_update_proc(sidebarLayer, updateRoundSidebarLeft);
#else
  layer_set_update_proc(sidebarLayer, updateRectSidebar);
#endif

#ifndef PBL_ROUND
  // Secondary panel: created hidden; main.c positions/shows it while a status
  // display is visible. Only meaningful where the status layout runs (rect,
  // non-aplite), but creating it hidden everywhere rect is harmless.
  secondarySidebarLayer = layer_create(GRect(0, 0, sidebarWidth, screen_rect.size.h));
  layer_set_update_proc(secondarySidebarLayer, updateRectSecondarySidebar);
  layer_set_hidden(secondarySidebarLayer, true);
  layer_add_child(window_get_root_layer(window), secondarySidebarLayer);
#endif

#ifdef PBL_ROUND
  sidebarLayer2 = layer_create(bounds2);
  layer_add_child(window_get_root_layer(window), sidebarLayer2);
  layer_set_update_proc(sidebarLayer2, updateRoundSidebarRight);
#endif
}

void Sidebar_deinit() {
  layer_destroy(sidebarLayer);

#ifndef PBL_ROUND
  layer_destroy(secondarySidebarLayer);
#endif

#ifdef PBL_ROUND
  layer_destroy(sidebarLayer2);
#endif

  SidebarWidgets_deinit();
}

void Sidebar_redraw() {
  update_sidebar_width();
#ifndef PBL_ROUND
  // On platforms where the status layout runs, main.c owns the panel frames
  // (apply_twt_layout); don't fight it here. Elsewhere, keep positioning the
  // primary ourselves.
  if (!TwtStatus_isSupported()) {
    // No status layout here (aplite): single sidebar = the LEFT (primary) list,
    // drawn in full (overflow clipped). The secondary panel stays hidden.
    // Known limitation: aplite has no right column, so an upgrade user whose list
    // was migrated to the right (sidebarOnLeft==false) sees an empty sidebar until
    // they reconfigure. Aplite is a legacy/non-goal platform; accepted.
    Sidebar_distributeWidgets(NULL, NULL);
    if (!settings.sidebarOnLeft) {
      layer_set_frame(sidebarLayer, GRect(screen_rect.size.w - sidebarWidth, 0,
                                          sidebarWidth, screen_rect.size.h));
    } else {
      layer_set_frame(sidebarLayer,
                      GRect(0, 0, sidebarWidth, screen_rect.size.h));
    }
  }
  layer_mark_dirty(secondarySidebarLayer);
#endif

  // redraw the layer
  layer_mark_dirty(sidebarLayer);

#ifdef PBL_ROUND
  layer_mark_dirty(sidebarLayer2);
#endif
}

void Sidebar_updateTime(struct tm *timeInfo) {
  SidebarWidgets_updateTime(timeInfo);
}

bool isAutoBatteryShown() {
  if (!settings.disableAutobattery) {
    BatteryChargeState chargeState = battery_state_service_peek();

    if (dynamicSettings.enableAutoBatteryWidget) {
      if (chargeState.charge_percent <= 10 || chargeState.is_charging) {
        return true;
      }
    }
  }

  return false;
}

#ifdef PBL_ROUND

// returns the best candidate widget for replacement by the auto battery
// or the disconnection icon
int getReplacableWidget() {
  if (settings.widgets[0] == EMPTY) {
    return 0;
  } else if (settings.widgets[2] == EMPTY) {
    return 2;
  }

  if (settings.widgets[0] == WEATHER_CURRENT ||
      settings.widgets[0] == WEATHER_FORECAST_TODAY) {
    return 0;
  } else if (settings.widgets[2] == WEATHER_CURRENT ||
             settings.widgets[2] == WEATHER_FORECAST_TODAY) {
    return 2;
  }

  // if we don't have any of those things, just replace the left widget
  return 0;
}

#else

// returns the best candidate widget for replacement by the auto battery
// or the disconnection icon
static int getReplacableWidget(const SidebarWidgetType widgetTypes[], int count) {
  // (rect columns are pre-filtered of EMPTY by copyWidgetList, so in practice the
  //  weather-preference and last-slot fallback below are what fire)
  // empty slot is the obvious choice
  for (int i = 0; i < count; i++) {
    if (widgetTypes[i] == EMPTY) { return i; }
  }
  // bluetooth-dependent widgets are the next-best candidates
  for (int i = 0; i < count; i++) {
    if (widgetTypes[i] == WEATHER_CURRENT || widgetTypes[i] == WEATHER_FORECAST_TODAY) {
      return i;
    }
  }
  // otherwise replace the last (lowest-priority) displayed widget
  return (count > 0) ? (count - 1) : 0;
}

#endif

#ifdef PBL_ROUND

void updateRoundSidebarRight(Layer *l, GContext *ctx) {
  GRect bounds = layer_get_bounds(l);
  GRect bgBounds = GRect(bounds.origin.x, bounds.size.h / -2, bounds.size.h * 2,
                         bounds.size.h * 2);

  bool showDisconnectIcon = !bluetooth_connection_service_peek();
  bool showAutoBattery = isAutoBatteryShown();

  SidebarWidgetType displayWidget = settings.widgets[2];

  if ((showAutoBattery || showDisconnectIcon) && getReplacableWidget() == 2) {
    if (showAutoBattery) {
      displayWidget = BATTERY_METER;
    } else if (showDisconnectIcon) {
      displayWidget = BLUETOOTH_DISCONNECT;
    }
  }

  drawRoundSidebar(ctx, bgBounds, displayWidget, 3);
}

void updateRoundSidebarLeft(Layer *l, GContext *ctx) {
  GRect bounds = layer_get_bounds(l);
  GRect bgBounds =
      GRect(bounds.origin.x - bounds.size.h * 2 + bounds.size.w,
            bounds.size.h / -2, bounds.size.h * 2, bounds.size.h * 2);

  bool showDisconnectIcon = !bluetooth_connection_service_peek();
  bool showAutoBattery = isAutoBatteryShown();
  SidebarWidgetType displayWidget = settings.widgets[0];

  if ((showAutoBattery || showDisconnectIcon) && getReplacableWidget() == 0) {
    if (showAutoBattery) {
      displayWidget = BATTERY_METER;
    } else if (showDisconnectIcon) {
      displayWidget = BLUETOOTH_DISCONNECT;
    }
  }

  drawRoundSidebar(ctx, bgBounds, displayWidget, 7);
}

void Sidebar_distributeWidgets(int *primaryCountOut, int *secondaryCountOut) {
  // Round keeps its fixed two-widget rendering (settings.widgets[0]/[2]).
  if (primaryCountOut) { *primaryCountOut = 0; }
  if (secondaryCountOut) { *secondaryCountOut = 0; }
}

void drawRoundSidebar(GContext *ctx, GRect bgBounds,
                      SidebarWidgetType widgetType, int widgetXOffset) {
  SidebarWidgets_updateFonts();

  // Round has a single continuous sidebar -> use the primary background color
  // (legacy key sidebarBgColorLeft); GColorClear = inherit settings.sidebarColor.
  GColor roundBg = gcolor_equal(settings.sidebarBgColorLeft, GColorClear)
      ? settings.sidebarColor : settings.sidebarBgColorLeft;
  graphics_context_set_fill_color(ctx, roundBg);

  graphics_fill_radial(ctx, bgBounds, GOvalScaleModeFillCircle, 100,
                       DEG_TO_TRIGANGLE(0), TRIG_MAX_ANGLE);

  SidebarWidgets_xOffset = widgetXOffset;
  SidebarWidget widget = getSidebarWidgetByType(widgetType);

  // calculate center position of the widget
  int widgetPosition = bgBounds.size.h / 4 - widget.getHeight() / 2;
  widget.draw(ctx, widgetPosition);
}

#else

// Display columns computed from settings.widgetList by Sidebar_distributeWidgets();
// the rect update procs draw these instead of reading the settings arrays.
static SidebarWidgetType primaryColumn[MAX_WIDGET_LIST];
static int primaryColumnCount = 0;
static SidebarWidgetType secondaryColumn[MAX_WIDGET_LIST];
static int secondaryColumnCount = 0;

// Copy a settings widget list into a column array, skipping EMPTY / out-of-range
// ids and clamping to the buffer. Returns the number copied.
static int copyWidgetList(SidebarWidgetType *out, const uint8_t *list, int count) {
  int n = 0;
  for (int i = 0; i < count && n < MAX_WIDGET_LIST; i++) {
    if (list[i] != EMPTY && list[i] <= MAX_WIDGET_TYPE) { out[n++] = (SidebarWidgetType)list[i]; }
  }
  return n;
}

// Number of widgets (from the top) FULLY visible in a column of inner height
// `innerHeight`, using the overflow layout (top-anchored, fixed V_PADDING gaps).
// In the fits case this equals `count`. The auto-battery / disconnect
// substitution uses this so the icon only ever lands on a widget the user sees.
static int columnVisibleCount(const SidebarWidgetType types[], int count, int innerHeight) {
  if (count <= 0) { return 0; }
  int total = 0;
  for (int i = 0; i < count; i++) { total += getSidebarWidgetByType(types[i]).getHeight(); }
  if (total + (count - 1) * V_PADDING_DEFAULT <= innerHeight) { return count; }
  int y = 0, visible = 0;
  for (int i = 0; i < count; i++) {
    int h = getSidebarWidgetByType(types[i]).getHeight();
    if (y + h <= innerHeight) { visible++; y += h + V_PADDING_DEFAULT; } else { break; }
  }
  return visible;
}

void Sidebar_distributeWidgets(int *primaryCountOut, int *secondaryCountOut) {
  SidebarWidgets_updateFonts();  // ensure `layout` heights are valid before packing
  primaryColumnCount = copyWidgetList(primaryColumn, settings.widgetList, settings.widgetCount);
  secondaryColumnCount = copyWidgetList(secondaryColumn, settings.rightWidgetList, settings.rightWidgetCount);
  if (primaryCountOut) { *primaryCountOut = primaryColumnCount; }
  if (secondaryCountOut) { *secondaryCountOut = secondaryColumnCount; }
}

static void drawWidgetColumn(Layer *l, GContext *ctx,
                             const SidebarWidgetType widgetTypes[], int widgetCount,
                             bool allowReplacement, bool isPrimary) {
  GRect bounds = layer_get_unobstructed_bounds(l);

  // zero on every rectangular platform besides emery
  SidebarWidgets_xOffset = (sidebarWidth - 30) / 2;
  SidebarWidgets_updateFonts();

  // Role-based configurable background (left vs right color).
  GColor sidebarBg = isPrimary ? settings.sidebarBgColorLeft : settings.sidebarBgColorRight;
  if (gcolor_equal(sidebarBg, GColorClear)) { sidebarBg = settings.sidebarColor; }
  graphics_context_set_fill_color(ctx, sidebarBg);
  graphics_fill_rect(ctx, layer_get_bounds(l), 0, GCornerNone);
  graphics_context_set_text_color(ctx, settings.sidebarTextColor);

  if (widgetCount == 0) { return; }

  int v_padding = V_PADDING_DEFAULT;
  int innerTop = v_padding;
  int innerHeight = bounds.size.h - v_padding * 2;

  SidebarWidget displayWidgets[MAX_WIDGET_LIST];
  for (int i = 0; i < widgetCount; i++) {
    displayWidgets[i] = getSidebarWidgetByType(widgetTypes[i]);
  }

  // Auto-battery / disconnect-icon replacement -- only onto a FULLY VISIBLE
  // widget. Column preference: the left column hosts the icon if it has any
  // visible widget; the right column hosts it only when the left has none.
  if (allowReplacement) {
    bool showDisconnectIcon = settings.activateDisconnectIcon && !bluetooth_connection_service_peek();
    bool showAutoBattery = isAutoBatteryShown();
    if (showAutoBattery || showDisconnectIcon) {
      int myVisible = columnVisibleCount(widgetTypes, widgetCount, innerHeight);
      bool hostHere;
      if (isPrimary) {
        hostHere = (myVisible > 0);
      } else {
        int leftVisible = columnVisibleCount(primaryColumn, primaryColumnCount, innerHeight);
        hostHere = (leftVisible == 0 && myVisible > 0);
      }
      if (hostHere) {
        int idx = getReplacableWidget(widgetTypes, myVisible);
        displayWidgets[idx] = showAutoBattery
            ? getSidebarWidgetByType(BATTERY_METER)
            : getSidebarWidgetByType(BLUETOOTH_DISCONNECT);
      }
    }
  }

  if (widgetCount == 1) {
    // a lone widget is centered in the column
    int y = innerTop + (innerHeight - displayWidgets[0].getHeight()) / 2;
    displayWidgets[0].draw(ctx, y);
    return;
  }

  int totalHeight = 0;
  for (int i = 0; i < widgetCount; i++) { totalHeight += displayWidgets[i].getHeight(); }

  // Gap selection: when the whole list fits with at least minimal gaps, spread the
  // slack (space-between -> loose). Otherwise use the fixed minimal gap and let the
  // tail run past the column bottom, where the layer bounds clip it (tight/overflow).
  int gap;
  if (totalHeight + (widgetCount - 1) * v_padding <= innerHeight) {
    int slack = innerHeight - totalHeight;
    gap = slack / (widgetCount - 1);
  } else {
    gap = v_padding;
  }

  int y = innerTop;
  for (int i = 0; i < widgetCount; i++) {
    displayWidgets[i].draw(ctx, y);
    y += displayWidgets[i].getHeight() + gap;
  }
}

void updateRectSidebar(Layer *l, GContext *ctx) {
  drawWidgetColumn(l, ctx, primaryColumn, primaryColumnCount, /*allowReplacement=*/true, /*isPrimary=*/true);
}

void updateRectSecondarySidebar(Layer *l, GContext *ctx) {
  drawWidgetColumn(l, ctx, secondaryColumn, secondaryColumnCount, /*allowReplacement=*/true, /*isPrimary=*/false);
}

#endif

// Frame setters: rect implementations move the real layers; no-ops on round so
// the shared header stays simple and main.c links on every platform.
void Sidebar_setPrimaryFrame(GRect frame) {
#ifndef PBL_ROUND
  layer_set_frame(sidebarLayer, frame);
#endif
}

void Sidebar_setPrimaryHidden(bool hidden) {
#ifndef PBL_ROUND
  layer_set_hidden(sidebarLayer, hidden);
#endif
}

void Sidebar_setSecondaryFrame(GRect frame) {
#ifndef PBL_ROUND
  layer_set_frame(secondarySidebarLayer, frame);
#endif
}

void Sidebar_setSecondaryHidden(bool hidden) {
#ifndef PBL_ROUND
  layer_set_hidden(secondarySidebarLayer, hidden);
#endif
}
