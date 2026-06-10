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
#include "crypto.h"
#include "sidebar_widgets.h"

// windows and layers
static Window* mainWindow;
static Layer* windowLayer;

// current bluetooth state
static bool isPhoneConnected;

// current time service subscription
static bool updatingEverySecond;

static time_t lastDataRequest = 0;   // epoch of the last watch->phone data request
static bool twtTargetAlerted = false;   // already vibrated for the current daily-target crossing
static bool twtTargetInit = false;      // have we seeded twtTargetAlerted since launch?

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
  if (!TwtStatus_isSupported()) return;  // rect, non-aplite only

  // The system timeline-peek banner (upcoming calendar event) eats screen
  // space. Rather than cram the bottom status strip into what's left, drop it
  // entirely while obstructed and give the clock + date header the full
  // unobstructed area. Detected by comparing the unobstructed bounds (what we
  // get to draw in) against the full screen bounds.
  Layer *root_layer = window_get_root_layer(mainWindow);
  GRect full = layer_get_bounds(root_layer);
  GRect root = layer_get_unobstructed_bounds(root_layer);
  bool obstructed = root.size.h < full.size.h;

  // Generic "active bottom status" resolution: {visible, height}. MIDI wins over
  // TWT when both active. A future status display adds another case here; the
  // panel/status geometry below is status-type agnostic.
  bool showMidi = !obstructed && midi_status.isRecording;
  bool showTwt  = !obstructed && !showMidi && twt_status.isTracking;
  bool statusVisible = showMidi || showTwt;
  // All status displays reserve the SAME fixed height (the 2-line TWT height), so
  // the clock stays the same size regardless of which status is showing. A
  // single-line status (MIDI) is vertically centered within this area by its own
  // setFrame.
  int statusHeight = statusVisible ? TWT_STATUS_HEIGHT_2LINE : 0;

  int topReserved = settings.showBigDate ? BIG_DATE_HEIGHT : 0;
  int statusTop = root.size.h - statusHeight;   // == root.size.h when no status

  bool primaryOnLeft = settings.sidebarOnLeft;
  // Distribute the widget priority list across the panels. The secondary panel
  // may show while a status is visible, or always if so configured.
  bool secondaryWanted = statusVisible || settings.secondaryAlwaysOn;
  int primaryCount, secondaryCount;
  Sidebar_distributeWidgets(secondaryWanted, &primaryCount, &secondaryCount);
  bool showSecondary = secondaryCount >= 1;

  // A side with exactly 3 widgets stays full height (blocks the status strip on
  // that side); a side with <=2 widgets is shortened to the status-strip top so
  // the status strip flows under it to the screen edge. The primary is full
  // height whenever no status is visible.
  int primaryHeight   = (primaryCount == 3) ? root.size.h
                                            : (statusVisible ? statusTop : root.size.h);
  int secondaryHeight = (secondaryCount == 3) ? root.size.h : statusTop;

  // Primary sidebar frame (always present).
  int primaryX = primaryOnLeft ? 0 : (root.size.w - sidebarWidth);
  Sidebar_setPrimaryFrame(GRect(primaryX, 0, sidebarWidth, primaryHeight));

  // Secondary panel frame (opposite side), shown only while a status is visible.
  if (showSecondary) {
    int secondaryX = primaryOnLeft ? (root.size.w - sidebarWidth) : 0;
    Sidebar_setSecondaryFrame(GRect(secondaryX, 0, sidebarWidth, secondaryHeight));
    Sidebar_setSecondaryHidden(false);
  } else {
    Sidebar_setSecondaryHidden(true);
  }

  // Horizontal insets for the clock/date: always inset by the primary; inset the
  // opposite side too when the secondary is shown.
  int leftInset = 0, rightInset = 0;
  if (primaryOnLeft) { leftInset = sidebarWidth; } else { rightInset = sidebarWidth; }
  if (showSecondary) {
    if (primaryOnLeft) { rightInset = sidebarWidth; } else { leftInset = sidebarWidth; }
  }
  int contentX = leftInset;
  int contentW = root.size.w - leftInset - rightInset;

  // Clock between the panels, between the top strip and the status strip.
  layer_set_frame(clock_area_layer,
      GRect(contentX, topReserved, contentW, statusTop - topReserved));

  // Date header in the top strip, between the panels.
  if (topReserved) {
    DateHeader_setFrame(GRect(contentX, 0, contentW, BIG_DATE_HEIGHT));
    DateHeader_setHidden(false);
    DateHeader_redraw();
  } else {
    DateHeader_setHidden(true);
  }

  // Status strip span: reaches the screen edge on a side unless a FULL-HEIGHT
  // panel sits there. Determine per physical side which panel is present and
  // whether it is full height.
  if (statusVisible) {
    bool leftFull, rightFull;
    if (primaryOnLeft) {
      leftFull  = (primaryCount == 3);
      rightFull = showSecondary && (secondaryCount == 3);
    } else {
      rightFull = (primaryCount == 3);
      leftFull  = showSecondary && (secondaryCount == 3);
    }
    int statusLeft  = leftFull  ? sidebarWidth : 0;
    int statusRight = rightFull ? (root.size.w - sidebarWidth) : root.size.w;
    GRect statusFrame = GRect(statusLeft, statusTop, statusRight - statusLeft, statusHeight);

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
  // One watch-driven request per configured interval serves ALL phone-fetched
  // data (weather, electricity, BTC). This is the documented Pebble pattern: the
  // tick service is always running and the AppMessage wakes the phone JS out of
  // power-save. The JS side throttles per source (electricity ~2/day; BTC sends
  // only on change), so a short interval does not over-fetch slow sources.
  bool needsPhoneData = !dynamicSettings.disableWeather;
  for (int i = 0; i < 3; i++) {
    if (settings.widgets[i] == ELECTRICITY || settings.widgets[i] == BTC_PRICE ||
        settings.widgets2[i] == ELECTRICITY || settings.widgets2[i] == BTC_PRICE) {
      needsPhoneData = true;
    }
  }
  if (needsPhoneData && tick_time->tm_sec == 0) {
    int intervalSec = (int)settings.pollIntervalMin * 60;
    if (intervalSec < 300) { intervalSec = 300; }   // floor 5 min
    time_t now = time(NULL);
    if (lastDataRequest == 0 || (now - lastDataRequest) >= intervalSec) {
      messaging_requestNewWeatherData();
      lastDataRequest = now;
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

  // Daily work-time target reached: vibrate once, on the rising edge, the minute
  // the worked total first hits the configured target. running is whole-minutes,
  // so the crossing is minute-granular and the MINUTE_UNIT tick lands on it.
  bool twtReached = twt_status.isTracking && twt_status.dailyTargetMin > 0
      && TwtStatus_workedTotalMin() >= twt_status.dailyTargetMin;
  if (!twtTargetInit) {
    twtTargetAlerted = twtReached;   // seed at launch: already-over-target -> no spurious vibe
    twtTargetInit = true;
  } else if (!twtReached) {
    twtTargetAlerted = false;        // reset for next day / target raised / tracking stopped
  } else if (!twtTargetAlerted) {
    twtTargetAlerted = true;
    if (settings.twtTargetVibe && !quiet_time_is_active()) {
      // distinct ascending pattern: short - short - long ("day complete")
      static const uint32_t segs[] = { 100, 80, 100, 80, 300 };
      VibePattern pat = { .durations = segs, .num_segments = ARRAY_LENGTH(segs) };
      vibes_enqueue_custom_pattern(pat);
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
  if (TwtStatus_isSupported()) { apply_twt_layout(); }
}

// force the sidebar to redraw any time the battery state changes
void batteryStateChanged(BatteryChargeState charge_state) {
  Sidebar_redraw();
  if (TwtStatus_isSupported()) { apply_twt_layout(); }
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

  lastDataRequest = time(NULL);   // first periodic request fires one interval after launch

  // init settings
  Settings_init();

  TwtStatus_load();
  MidiStatus_load();

  // init weather system
  Weather_init();
  Electricity_init();
  Crypto_init();

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
