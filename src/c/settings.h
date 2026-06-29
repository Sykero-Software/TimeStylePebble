#pragma once
#include <pebble.h>
#include "sidebar_widgets.h"

#define CURRENT_SETTINGS_VERSION 7

// persistent storage keys
#define SETTINGS_PERSIST_KEY 100
#define SETTINGS_VERSION_PERSIST_KEY 4
#define MIGRATION_EMERY_FONT_PERSIST_KEY 200

#define FONT_SETTING_DEFAULT 0
#define FONT_SETTING_LECO    1
#define FONT_SETTING_BOLD    2
#define FONT_SETTING_BOLD_H  3
#define FONT_SETTING_BOLD_M  4

#define CLOCK_STYLE_DIGITAL 0
#define CLOCK_STYLE_ANALOG  1

// Analog hour-tick style (SettingAnalogTicks): 0=hide, 1=normal, 2=bold.
#define ANALOG_TICKS_HIDE   0
#define ANALOG_TICKS_NORMAL 1
#define ANALOG_TICKS_BOLD   2

typedef enum {
  NO_VIBE = 0,
  VIBE_EVERY_HOUR = 1,
  VIBE_EVERY_HALF_HOUR = 2
} VibeIntervalType;

// Settings struct -- note, all new settings should ALWAYS be added to the bottom
typedef struct {
  // color settings
  GColor timeColor;
  GColor timeBgColor;
  GColor sidebarColor;
  GColor sidebarTextColor;

  // general settings
  uint8_t languageId;
  bool showLeadingZero;
  uint8_t clockFontId;

  // vibration settings
  bool btVibe;
  VibeIntervalType hourlyVibe;

  // sidebar settings
  SidebarWidgetType widgets[3];
  bool sidebarOnLeft;   // legacy: drives only the one-time dual-list migration + aplite/round side; no longer the rect layout side
  bool useLargeFonts;
  bool activateDisconnectIcon;
  
  // weather widget settings
  bool useMetric;

  // battery meter widget settings
  bool showBatteryPct;
  bool disableAutobattery;

  // alt tz widget settings
  char altclockName[8];
  int altclockOffset;

  // health widget Settings
  bool healthUseDistance;  // retained for persisted-blob layout; no longer used
  bool healthUseRestfulSleep;  // retained for persisted-blob layout; no longer used
  char decimalSeparator;

  // MIDI recorder settings
  bool midiVibe;   // vibrate on MIDI recording start/stop

  // clock area extras
  bool showBigDate;   // large date line above the clock (e.g. "Ti 6.6.")
  bool twtShowRemaining;   // TWT strip: show remaining (target - worked), "+h:mm" on overtime
  bool twtTargetVibe;   // vibrate once when the daily work-time target is reached
  bool twtBudgetVibe;   // vibrate once when the current task's budget is reached

  // polling
  uint8_t pollIntervalMin;   // shared watch->phone data request interval (min); appended field

  // secondary widget panel (shown opposite the primary sidebar while a bottom
  // status display is visible); EMPTY x3 = feature off. Appended field.
  SidebarWidgetType widgets2[3];

  // Secondary widget panel: always visible (true) vs only while a bottom
  // status display is shown (false = historical auto-hide). Appended field.
  bool secondaryAlwaysOn;

  // cheap-electricity widgets (next-cheap + cheapest-hour); appended fields,
  // no settings-version bump (defaults set in Settings_init).
  uint8_t elecQuietStart;        // quiet-hours start hour 0-23 (default 23)
  uint8_t elecQuietEnd;          // quiet-hours end hour 0-23 (default 7)
  uint8_t elecCheapFactorPct;    // cheapBar = mean*pct/100 (default 70)
  int16_t elecCheapFloorCenti;   // cheapBar floor, 0.01 snt (default 200)
  int16_t elecCheapCeilingCenti; // cheapBar ceiling, 0.01 snt (default 800)

  // configurable panel backgrounds; GColorClear = "inherit". For status/date,
  // inherit = draw no fill (watchface bg). For the sidebars, inherit = fall back to
  // settings.sidebarColor. Appended fields, no settings-version bump (defaults in
  // Settings_init).
  GColor twtStatusBgColor;    // status strip background (work-time + MIDI strips)
  GColor twtFlashColor;       // status strip flash colour on a target-reached vibe (always solid)
  GColor dateBgColor;         // date header background
  // Repurposed as PRIMARY / SECONDARY sidebar backgrounds (role-based). The
  // "Left"/"Right" names are legacy (kept so AppMessage IDs don't drift); the
  // primary column always uses sidebarBgColorLeft, the secondary always
  // sidebarBgColorRight, whichever physical side each is on. Round uses the
  // primary one. See sidebar.c drawWidgetColumn / drawRoundSidebar.
  GColor sidebarBgColorLeft;  // PRIMARY sidebar background
  GColor sidebarBgColorRight; // SECONDARY sidebar background

  // LEFT sidebar widget list (ordered). Drawn in full in the left column:
  // space-between when it fits, top-anchored + clipped when it overflows. The
  // legacy widgets[]/widgets2[] arrays above are retained only for the one-time
  // migration and for round-board mirroring (settings.widgets[0]/[2]).
  uint8_t widgetList[MAX_WIDGET_LIST];
  uint8_t widgetCount;

  // Status strip layout: false (default) = side columns stay full height and the
  // strip is inset between them; true = columns shorten to the strip top and the
  // strip spans full width. Appended field, zero-default (=false).
  bool statusStripFullWidth;

  // RIGHT sidebar widget list (ordered), independent of the left list. Appended
  // fields, zero-default on load of an older blob.
  uint8_t rightWidgetList[MAX_WIDGET_LIST];
  uint8_t rightWidgetCount;

  // One-time dual-list migration sentinel. false on a pre-dual-list blob; on the
  // first load we split the single widgetList onto left/right per the legacy
  // sidebarOnLeft, then set this true and persist. Appended field, zero-default.
  bool dualListInit;

  // MIDI recorder: show the live recording timer. false (default) = off — strip
  // shows only the REC dot + device name (MINUTE_UNIT tick while recording);
  // true = seconds timer (m:ss / h:mm:ss, SECOND_UNIT tick).
  // Appended field, zero-default on load of an older blob (= off).
  bool midiSecondPrecision;

  // Auto-battery appearance threshold (percent). The battery widget auto-appears
  // when charge_percent <= this value (or while charging). Appended field,
  // defaulted to 10 in Settings_loadFromStorage. No settings-version bump.
  uint8_t autoBatteryThreshold;

  // Fallback (auto-battery / BT-disconnect) placement, shared by both fallbacks:
  //   fallbackColumn: 0 = Automatic (legacy host-column + getReplacableWidget),
  //                   1 = Left/primary list, 2 = Right/secondary list.
  //   fallbackPosition: 1-based slot in the chosen column. <= count replaces that
  //                   slot; == count+1 appends the fallback at the bottom.
  // Appended fields, defaulted (0 / 1) in Settings_loadFromStorage. No version bump.
  uint8_t fallbackColumn;
  uint8_t fallbackPosition;

  // Clock display style: 0 = digital HH:MM (default), 1 = analog face.
  // Appended field, zero-default on load of an older blob (= digital). No
  // settings-version bump.
  uint8_t clockStyle;

  // Analog clock hour-tick style: 0=hide, 1=normal (default), 2=bold (thicker +
  // slightly longer). Was a bool (analogShowTicks); now uint8_t at the SAME
  // 1-byte slot, so the persisted blob is unchanged and an old 0/1 maps to
  // hide/normal. The default is set explicitly in Settings_loadFromStorage
  // before the persisted blob is read, so upgrading users keep their ticks.
  uint8_t analogTickStyle;
} Settings;

// Dynamic settings (calculated at runtime based on currently-selected widgets)
typedef struct {
  bool disableWeather;
  bool updateScreenEverySecond;
  bool enableAutoBatteryWidget;
  bool enableBeats;
  bool enableAltTimeZone;
  
  GColor iconFillColor;
  GColor iconStrokeColor;
} DynamicSettings;

// Legacy packed settings struct
// this is now deprecated, and will be removed in the next version
typedef struct {
  GColor timeColor;
  GColor timeBgColor;
  GColor sidebarColor;
  GColor sidebarTextColor;

  // general settings
  uint8_t languageId;
  uint8_t showLeadingZero:1;
  uint8_t clockFontId:7;

  // vibration settings
  uint8_t btVibe:1;
  int8_t hourlyVibe:7;

  // sidebar settings
  uint8_t widgets[3];
  uint8_t sidebarOnLeft:1;
  uint8_t useLargeFonts:1;

  // weather widget settings
  uint8_t useMetric:1;

  // battery meter widget settings
  uint8_t showBatteryPct:1;
  uint8_t disableAutobattery:1;

  // health widget Settings
  uint8_t healthUseDistance:1;  // retained for persisted-blob layout; no longer used
  uint8_t healthUseRestfulSleep:1;
  char decimalSeparator;

  // alt tz widget settings
  char altclockName[8];
  int8_t altclockOffset;

  // bluetooth disconnection icon
  int8_t activateDisconnectIcon:1;
} LegacyStoredSettings;

extern Settings settings;
extern DynamicSettings dynamicSettings;

void Settings_init();
void Settings_deinit();
void Settings_loadFromStorage();
void Settings_saveToStorage();
void Settings_updateDynamicSettings();
