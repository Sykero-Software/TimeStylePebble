#include "sidebar.h"
#include "languages.h"
#include "settings.h"
#include "sidebar_widgets.h"
#include "weather.h"
#include "twt_status.h"
#include <ctype.h>
#include <math.h>
#include <pebble.h>

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define V_PADDING_DEFAULT 9
#else
#define V_PADDING_DEFAULT 8
#endif
#define V_PADDING_COMPACT 4

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
Layer *secondarySidebarLayer;   // shown opposite the primary while a status display is visible
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
    // No status layout here (aplite): the primary always shows the list head;
    // the secondary panel stays hidden (always-on needs the status layout's
    // clock-inset machinery, which doesn't run on this platform).
    Sidebar_distributeWidgets(false, NULL, NULL);
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
static int getReplacableWidget(const SidebarWidgetType widgetTypes[3]) {
  // if any widgets are empty, it's an obvious choice
  for (int i = 0; i < 3; i++) {
    if (widgetTypes[i] == EMPTY) {
      return i;
    }
  }

  // are there any bluetooth-enabled widgets? if so, they're the second-best
  // candidates
  for (int i = 0; i < 3; i++) {
    if (widgetTypes[i] == WEATHER_CURRENT ||
        widgetTypes[i] == WEATHER_FORECAST_TODAY) {
      return i;
    }
  }

  // if we don't have any of those things, just replace the middle widget
  return 1;
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

void Sidebar_distributeWidgets(bool secondaryWanted, int *primaryCountOut, int *secondaryCountOut) {
  // Round keeps its fixed two-widget rendering (settings.widgets[0]/[2]).
  (void)secondaryWanted;
  if (primaryCountOut) *primaryCountOut = 0;
  if (secondaryCountOut) *secondaryCountOut = 0;
}

void drawRoundSidebar(GContext *ctx, GRect bgBounds,
                      SidebarWidgetType widgetType, int widgetXOffset) {
  SidebarWidgets_updateFonts();

  graphics_context_set_fill_color(ctx, settings.sidebarColor);

  graphics_fill_radial(ctx, bgBounds, GOvalScaleModeFillCircle, 100,
                       DEG_TO_TRIGANGLE(0), TRIG_MAX_ANGLE);

  SidebarWidgets_xOffset = widgetXOffset;
  SidebarWidget widget = getSidebarWidgetByType(widgetType);

  // calculate center position of the widget
  int widgetPosition = bgBounds.size.h / 4 - widget.getHeight() / 2;
  widget.draw(ctx, widgetPosition);
}

#else

// Display columns computed from the widget priority list by
// Sidebar_distributeWidgets(); the rect update procs draw these instead of
// reading the settings arrays directly.
static SidebarWidgetType primaryColumn[3] = {EMPTY, EMPTY, EMPTY};
static SidebarWidgetType secondaryColumn[3] = {EMPTY, EMPTY, EMPTY};

void Sidebar_distributeWidgets(bool secondaryWanted, int *primaryCountOut, int *secondaryCountOut) {
  // Compact: the list is settings.widgets followed by settings.widgets2, EMPTY
  // slots skipped, in priority order.
  SidebarWidgetType list[6];
  int n = 0;
  for (int i = 0; i < 3; i++) {
    if (settings.widgets[i] != EMPTY) list[n++] = settings.widgets[i];
  }
  for (int i = 0; i < 3; i++) {
    if (settings.widgets2[i] != EMPTY) list[n++] = settings.widgets2[i];
  }

  // Even split when the secondary panel may show (extra widget to the primary):
  // N=2 -> 1+1, 3 -> 2+1, 4 -> 2+2, 5 -> 3+2, 6 -> 3+3. Otherwise the primary
  // takes the head and anything past 3 stays hidden (priority order).
  int secondaryCount = secondaryWanted ? n / 2 : 0;
  if (secondaryCount > 3) secondaryCount = 3;
  int primaryCount = n - secondaryCount;
  if (primaryCount > 3) primaryCount = 3;

  for (int i = 0; i < 3; i++) {
    primaryColumn[i]   = (i < primaryCount)   ? list[i]                : EMPTY;
    secondaryColumn[i] = (i < secondaryCount) ? list[primaryCount + i] : EMPTY;
  }
  // A column of exactly 2 looks better split top + bottom (also matches the
  // classic TimeStyle default of date-at-the-bottom) than top + middle.
  if (primaryCount == 2)   { primaryColumn[2] = primaryColumn[1];     primaryColumn[1] = EMPTY; }
  if (secondaryCount == 2) { secondaryColumn[2] = secondaryColumn[1]; secondaryColumn[1] = EMPTY; }

  if (primaryCountOut) *primaryCountOut = primaryCount;
  if (secondaryCountOut) *secondaryCountOut = secondaryCount;
}

// Draw a column of three widgets into the layer's frame. Shared by the primary
// sidebar and the secondary panel. `allowReplacement` enables the auto-battery /
// disconnect-icon substitution (primary only).
static void drawWidgetColumn(Layer *l, GContext *ctx,
                             const SidebarWidgetType widgetTypes[3],
                             bool allowReplacement, bool isLeftSide) {
  GRect bounds = layer_get_unobstructed_bounds(l);

  // this ends up being zero on every rectangular platform besides emery
  SidebarWidgets_xOffset = (sidebarWidth - 30) / 2;

  SidebarWidgets_updateFonts();

  // per-side configurable background; GColorClear = inherit settings.sidebarColor
  GColor sidebarBg = isLeftSide ? settings.sidebarBgColorLeft : settings.sidebarBgColorRight;
  if (gcolor_equal(sidebarBg, GColorClear)) sidebarBg = settings.sidebarColor;
  graphics_context_set_fill_color(ctx, sidebarBg);
  graphics_fill_rect(ctx, layer_get_bounds(l), 0, GCornerNone);

  graphics_context_set_text_color(ctx, settings.sidebarTextColor);

  SidebarWidget displayWidgets[3];

  displayWidgets[0] = getSidebarWidgetByType(widgetTypes[0]);
  displayWidgets[1] = getSidebarWidgetByType(widgetTypes[1]);
  displayWidgets[2] = getSidebarWidgetByType(widgetTypes[2]);

  // auto-battery / disconnect-icon replacement applies to the primary only
  if (allowReplacement) {
    bool showDisconnectIcon = false;
    bool showAutoBattery = isAutoBatteryShown();

    // if the pebble is disconnected and activated, show the disconnect icon
    if (settings.activateDisconnectIcon) {
      showDisconnectIcon = !bluetooth_connection_service_peek();
    }

    // do we need to replace a widget?
    // if so, determine which widget should be replaced
    if (showAutoBattery || showDisconnectIcon) {
      int widget_to_replace = getReplacableWidget(widgetTypes);

      if (showAutoBattery) {
        displayWidgets[widget_to_replace] = getSidebarWidgetByType(BATTERY_METER);
      } else if (showDisconnectIcon) {
        displayWidgets[widget_to_replace] =
            getSidebarWidgetByType(BLUETOOTH_DISCONNECT);
      }
    }
  }

  // if the widgets are too tall, enable "compact mode"
  int compact_mode_threshold = bounds.size.h - V_PADDING_DEFAULT * 2 - 3;
  int v_padding = V_PADDING_DEFAULT;

  SidebarWidgets_useCompactMode =
      false; // ensure that we compare the non-compacted heights
  int totalHeight = displayWidgets[0].getHeight() +
                    displayWidgets[1].getHeight() +
                    displayWidgets[2].getHeight();
  SidebarWidgets_useCompactMode = (totalHeight > compact_mode_threshold);
  // printf("Total Height: %i, Threshold: %i", totalHeight,
  // compact_mode_threshold);

  // now that they have been compacted, check if they fit a second time,
  // if they still don't fit, our only choice is MURDER (of the middle widget)
  totalHeight = displayWidgets[0].getHeight() + displayWidgets[1].getHeight() +
                displayWidgets[2].getHeight();
  bool hide_middle_widget = (totalHeight > compact_mode_threshold);
  // printf("Compact Mode Enabled. Total Height: %i, Threshold: %i",
  // totalHeight, compact_mode_threshold);

  // still doesn't fit? try compacting the vertical padding
  totalHeight = displayWidgets[0].getHeight() + displayWidgets[2].getHeight();
  if (totalHeight > compact_mode_threshold) {
    v_padding = V_PADDING_COMPACT;
  }

  // calculate the three widget positions
  int topWidgetPos = v_padding;
  int lowerWidgetPos =
      bounds.size.h - v_padding - displayWidgets[2].getHeight();

  // vertically center the middle widget using MATH
  int middleWidgetPos = ((lowerWidgetPos - displayWidgets[1].getHeight()) +
                         (topWidgetPos + displayWidgets[0].getHeight())) /
                        2;

  // draw the widgets
  displayWidgets[0].draw(ctx, topWidgetPos);
  if (!hide_middle_widget) {
    displayWidgets[1].draw(ctx, middleWidgetPos);
  }
  displayWidgets[2].draw(ctx, lowerWidgetPos);
}

void updateRectSidebar(Layer *l, GContext *ctx) {
  // primary sidebar sits on the left iff sidebarOnLeft
  drawWidgetColumn(l, ctx, primaryColumn, true, settings.sidebarOnLeft);
}

void updateRectSecondarySidebar(Layer *l, GContext *ctx) {
  // secondary panel is on the opposite side from the primary
  drawWidgetColumn(l, ctx, secondaryColumn, false, !settings.sidebarOnLeft);
}

#endif

// Frame setters: rect implementations move the real layers; no-ops on round so
// the shared header stays simple and main.c links on every platform.
void Sidebar_setPrimaryFrame(GRect frame) {
#ifndef PBL_ROUND
  layer_set_frame(sidebarLayer, frame);
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
