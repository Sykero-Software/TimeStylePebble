#include <pebble.h>
#include "clock_area.h"
#include "messaging.h"
#include "settings.h"
#include "weather.h"
#include "sidebar.h"
#include "util.h"
#include "twt_status.h"
#include "midi_status.h"
#include "date_header.h"
#include "electricity.h"
#include "btc.h"
#include "sidebar_widgets.h"

// windows and layers
static Window* mainWindow;
static Layer* windowLayer;

// current bluetooth state
static bool isPhoneConnected;

// current time service subscription
static bool updatingEverySecond;

// try to randomize when watches call the weather API
static uint8_t weatherRefreshMinute;

void update_clock();
void redrawScreen();
void tick_handler(struct tm *tick_time, TimeUnits units_changed);
void bluetoothStateChanged(bool newConnectionState);
static void apply_twt_layout();


void update_clock() {
  time_t rawTime;
  struct tm* timeInfo;

#ifdef USE_FAKE_TIME
  struct tm fakeTime = {
    .tm_hour = 6,
    .tm_min  = 23,
    .tm_sec  = 0,
    .tm_mday = 24,
    .tm_mon  = 3,
    .tm_year = 126,
    .tm_wday = 5,
  };
  timeInfo = &fakeTime;
#else
  time(&rawTime);
  timeInfo = localtime(&rawTime);
#endif

  ClockArea_update_time(timeInfo);
  Sidebar_updateTime(timeInfo);
  DateHeader_updateTime(timeInfo);
}

// Apply the TrackWorkTime layout. While tracking, shrink the clock to make room for the
// status line and show it. When not tracking, restore the original full-size clock and hide
// the line, so the watchface looks exactly as it did before the integration.
static void apply_twt_layout() {
  if (!TwtStatus_isSupported()) return;  // same support gate as DateHeader

  // The system timeline-peek banner (upcoming calendar event) eats screen
  // space. Rather than cram the bottom status strip into what's left, drop it
  // entirely while obstructed and give the clock + date header the full
  // unobstructed area. Detected by comparing the unobstructed bounds (what we
  // get to draw in) against the full screen bounds.
  Layer *root_layer = window_get_root_layer(mainWindow);
  GRect full = layer_get_bounds(root_layer);
  GRect root = layer_get_unobstructed_bounds(root_layer);
  bool obstructed = root.size.h < full.size.h;

  bool showMidi = !obstructed && midi_status.isRecording;
  bool showTwt  = !obstructed && !showMidi && twt_status.isTracking;

  // Top strip: large date header (independent of tracking state).
  int topReserved = settings.showBigDate ? BIG_DATE_HEIGHT : 0;

  // Bottom strip: TWT (two lines) or MIDI (one line) status.
  int bottomReserved = 0;
  if (showMidi || showTwt) {
    bottomReserved = showTwt ? TWT_STATUS_HEIGHT_2LINE : TWT_STATUS_HEIGHT;
  }

  // Shrink the clock to fit between the two strips; the clock font rescales
  // automatically from the layer height. The clock draws via FCTX in absolute
  // screen coordinates, so update_clock_area_layer() adds the frame's top
  // offset back in manually to follow this move down (see clock_area.c).
  layer_set_frame(clock_area_layer,
      GRect(0, topReserved, root.size.w, root.size.h - topReserved - bottomReserved));

  // Date header: centered in the non-sidebar area of the top strip.
  if (topReserved) {
    int dateX = settings.sidebarOnLeft ? sidebarWidth : 0;
    int dateW = root.size.w - sidebarWidth;
    DateHeader_setFrame(GRect(dateX, 0, dateW, BIG_DATE_HEIGHT));
    DateHeader_setHidden(false);
    DateHeader_redraw();
  } else {
    DateHeader_setHidden(true);
  }

  // Bottom status line (unchanged behavior).
  if (showMidi || showTwt) {
    int statusX = settings.sidebarOnLeft ? sidebarWidth : 0;
    int statusW = root.size.w - sidebarWidth;
    GRect statusFrame = GRect(statusX, root.size.h - bottomReserved, statusW, bottomReserved);
    if (showMidi) {
      MidiStatus_setFrame(statusFrame);
      MidiStatus_setHidden(false); MidiStatus_redraw();
      TwtStatus_setHidden(true);
    } else {
      TwtStatus_setFrame(statusFrame);
      TwtStatus_setHidden(false); TwtStatus_redraw();
      MidiStatus_setHidden(true);
    }
  } else {
    TwtStatus_setHidden(true);
    MidiStatus_setHidden(true);
  }

  layer_mark_dirty(clock_area_layer);
}

// Recompute the layout the instant the timeline-peek banner slides in/out, so
// the bottom status strip never lingers hidden behind it (tick-driven relayout
// alone can lag up to a minute in MINUTE_UNIT mode).
static void unobstructed_did_change(void *context) {
  apply_twt_layout();
}

/* forces everything on screen to be redrawn -- perfect for keeping track of settings! */
void redrawScreen() {

  // check if the tick handler frequency should be changed
  bool wantEverySecond = dynamicSettings.updateScreenEverySecond || midi_status.isRecording;
  if(wantEverySecond != updatingEverySecond) {
    tick_timer_service_unsubscribe();

    if(wantEverySecond) {
      tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
      updatingEverySecond = true;
    } else {
      tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
      updatingEverySecond = false;
    }
  }

  window_set_background_color(mainWindow, settings.timeBgColor);

  // maybe the language changed!
  update_clock();

  // update the sidebar
  Sidebar_redraw();

  ClockArea_redraw();

  apply_twt_layout();
}

static void main_window_load(Window *window) {
  window_set_background_color(window, settings.timeBgColor);

  // create the sidebar
  Sidebar_init(window);

  ClockArea_init(window);

  if (TwtStatus_isSupported()) {
    GRect root = layer_get_bounds(window_get_root_layer(window));
    // Initial frames; apply_twt_layout() resets them per-mode before unhiding.
    GRect twtFrame = GRect(0, root.size.h - TWT_STATUS_HEIGHT_2LINE, root.size.w, TWT_STATUS_HEIGHT_2LINE);
    GRect midiFrame = GRect(0, root.size.h - TWT_STATUS_HEIGHT, root.size.w, TWT_STATUS_HEIGHT);
    TwtStatus_initLayer(window_get_root_layer(window), twtFrame);   // created hidden
    MidiStatus_initLayer(window_get_root_layer(window), midiFrame); // created hidden
    // Date header lives in the top strip; created hidden, apply_twt_layout() shows it per setting.
    GRect dateFrame = GRect(0, 0, root.size.w, BIG_DATE_HEIGHT);
    DateHeader_initLayer(window_get_root_layer(window), dateFrame);
  }

  redrawScreen(); // calls apply_twt_layout() -> sets clock size + line visibility per tracking
}

static void main_window_unload(Window *window) {
  ClockArea_deinit();
  TwtStatus_deinitLayer();
  MidiStatus_deinitLayer();
  DateHeader_deinitLayer();
  Sidebar_deinit();
}


void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // every 30 minutes, request fresh phone data if weather OR the electricity
  // widget needs it (the JS side throttles the actual API call to ~2/day)
  bool needsPhoneData = !dynamicSettings.disableWeather;
  for (int i = 0; i < 3; i++) {
    if (settings.widgets[i] == ELECTRICITY) {
      needsPhoneData = true;
    }
  }
  if (needsPhoneData) {
    if (tick_time->tm_min == weatherRefreshMinute && tick_time->tm_sec == 0) {
      messaging_requestNewWeatherData();
    }
  }

  // every hour, if requested, vibrate
  if(!quiet_time_is_active() && tick_time->tm_sec == 0) {
    if(settings.hourlyVibe == VIBE_EVERY_HOUR) { // hourly vibes only
      if(tick_time->tm_min == 0) {
        vibes_double_pulse();
      }
    } else if(settings.hourlyVibe == VIBE_EVERY_HALF_HOUR) {  // hourly and half-hourly
      if(tick_time->tm_min == 0) {
        vibes_double_pulse();
      } else if(tick_time->tm_min == 30) {
        vibes_short_pulse();
      }
    }
  }

  update_clock();

  // redraw all screen
  Sidebar_redraw();
  ClockArea_redraw();
  TwtStatus_redraw();
  MidiStatus_redraw();
  DateHeader_redraw();
}

void bluetoothStateChanged(bool newConnectionState) {
  // if the phone was connected but isn't anymore and the user has opted in,
  // trigger a vibration
  if(!quiet_time_is_active() && isPhoneConnected && !newConnectionState && settings.btVibe) {
    static uint32_t const segments[] = { 200, 100, 100, 100, 500 };
    VibePattern pat = {
      .durations = segments,
      .num_segments = ARRAY_LENGTH(segments),
      };
    vibes_enqueue_custom_pattern(pat);
  }

  // if the phone was disconnected and isn't anymore, update the data
  if(!isPhoneConnected && newConnectionState) {
    messaging_requestNewWeatherData();
  }

  isPhoneConnected = newConnectionState;

  Sidebar_redraw();
}

// force the sidebar to redraw any time the battery state changes
void batteryStateChanged(BatteryChargeState charge_state) {
  Sidebar_redraw();
}

// fixes for disappearing elements after notifications
// (from http://codecorner.galanter.net/2016/01/08/solved-issue-with-pebble-framebuffer-after-notification-is-dismissed/)
static void app_focus_changing(bool focusing) {
  if (focusing) {
     layer_set_hidden(windowLayer, true);
  }
}

static void app_focus_changed(bool focused) {
  if (focused) {
     layer_set_hidden(windowLayer, false);
     layer_mark_dirty(windowLayer);
  }
}

static void init() {
  setlocale(LC_ALL, "");

  srand(time(NULL));

  weatherRefreshMinute = rand() % 60;

  // init settings
  Settings_init();

  TwtStatus_load();
  MidiStatus_load();

  // init weather system
  Weather_init();
  Electricity_init();
  Btc_init();

  // init the messaging thing
  messaging_init(redrawScreen);

  // Create main Window element and assign to pointer
  mainWindow = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(mainWindow, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(mainWindow, true);

  windowLayer = window_get_root_layer(mainWindow);

  // Register with TickTimerService
  if(dynamicSettings.updateScreenEverySecond) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    updatingEverySecond = true;
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    updatingEverySecond = false;
  }

  bool connected = bluetooth_connection_service_peek();
  bluetoothStateChanged(connected);
  bluetooth_connection_service_subscribe(bluetoothStateChanged);

  // register with battery service
  battery_state_service_subscribe(batteryStateChanged);

  // set up focus change handlers
  app_focus_service_subscribe_handlers((AppFocusHandlers){
    .did_focus = app_focus_changed,
    .will_focus = app_focus_changing
  });

  // relayout the bottom status strip when the timeline-peek banner appears
  if (TwtStatus_isSupported()) {
    unobstructed_area_service_subscribe((UnobstructedAreaHandlers){
      .did_change = unobstructed_did_change
    }, NULL);
  }
}

static void deinit() {
  // Destroy Window
  window_destroy(mainWindow);

  // unload weather stuff
  Weather_deinit();
  Electricity_deinit();
  Settings_deinit();

  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  if (TwtStatus_isSupported()) {
    unobstructed_area_service_unsubscribe();
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
