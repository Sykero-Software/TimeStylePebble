#include "settings.h"
#include "languages.h"
#include <pebble.h>

Settings settings;
DynamicSettings dynamicSettings;

void Settings_init() {
  Settings_loadFromStorage();

  // for anyone who already installed timestyle on an emery, reset
  // their large font setting, since the old "large" setting is
  // roughly equivalent to the new default, and the new "large"
  // setting is EVEN LARGER
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
  if (!persist_exists(MIGRATION_EMERY_FONT_PERSIST_KEY)) {
    if (settings.useLargeFonts) {
      settings.useLargeFonts = false;
      Settings_saveToStorage();
    }
    persist_write_bool(MIGRATION_EMERY_FONT_PERSIST_KEY, true);
  }
#endif
}

void Settings_deinit() { Settings_saveToStorage(); }

void Settings_loadFromStorage() {
  // Set defaults
  settings.sidebarTextColor = GColorBlack;

#ifdef PBL_COLOR
  settings.timeBgColor = GColorWhite;
  settings.timeColor = GColorBlack;
  settings.sidebarColor = GColorMintGreen;
#else
  settings.timeBgColor = GColorBlack;
  settings.timeColor = GColorWhite;
  settings.sidebarColor = GColorWhite;
#endif

  settings.widgets[0] = HEARTRATE;
  settings.widgets[1] = BTC_PRICE;
  settings.widgets[2] = EURUSD_RATE;

  settings.activateDisconnectIcon = true;
  strncpy(settings.altclockName, "ALT", sizeof(settings.altclockName));
  settings.altclockOffset = 0;
  settings.decimalSeparator = '.';
  settings.clockFontId = FONT_SETTING_LECO;   // LECO clock font by default (configurable)
  settings.showBatteryPct = true;
  settings.midiVibe = false;   // opt-in default; also the upgrade default (appended field, no settings-version bump)
  settings.showBigDate = true;   // on by default (configurable); appended field, no settings-version bump
  settings.twtShowRemaining = false;   // opt-in default; appended field, no settings-version bump
  settings.twtTargetVibe = false;   // opt-in default; appended field, no settings-version bump
  settings.twtBudgetVibe = false;   // opt-in default; appended field, no settings-version bump
  settings.pollIntervalMin = 30;   // default; appended field, no settings-version bump
  settings.widgets2[0] = EMPTY;   // secondary panel off by default; appended field, no settings-version bump
  settings.widgets2[1] = EMPTY;
  settings.widgets2[2] = EMPTY;
  settings.secondaryAlwaysOn = false;   // auto-hide by default; appended field, no settings-version bump
  // Default priority list mirrors the historical widgets[0..2] defaults.
  settings.widgetList[0] = HEARTRATE;
  settings.widgetList[1] = BTC_PRICE;
  settings.widgetList[2] = EURUSD_RATE;
  settings.widgetCount = 3;
  settings.statusStripFullWidth = false;   // full-height columns by default; appended field
  settings.elecQuietStart = 23;        // appended field, no settings-version bump
  settings.elecQuietEnd = 7;
  settings.elecCheapFactorPct = 70;
  settings.elecCheapFloorCenti = 200;  // 2.0 snt/kWh
  settings.elecCheapCeilingCenti = 800;// 8.0 snt/kWh
#ifdef PBL_COLOR
  settings.twtStatusBgColor = GColorMintGreen;    // light-green panels by default
  settings.twtFlashColor = GColorRed;             // bright flash; appended field, no settings-version bump
  settings.dateBgColor = GColorMintGreen;
  settings.sidebarBgColorLeft = GColorMintGreen;
  settings.sidebarBgColorRight = GColorMintGreen;
#else
  settings.twtStatusBgColor = GColorClear;    // inherit watchface bg; appended field
  settings.twtFlashColor = GColorBlack;       // high-contrast flash on b&w; appended field
  settings.dateBgColor = GColorClear;         // inherit watchface bg; appended field
  settings.sidebarBgColorLeft = GColorClear;  // inherit sidebarColor; appended field
  settings.sidebarBgColorRight = GColorClear; // inherit sidebarColor; appended field
#endif

  // to correct settings migration bug (settings key v6), we must do another
  // migration (nooooooooooo)
  if (persist_exists(SETTINGS_PERSIST_KEY)) {
    int version = 0;

    if (persist_exists(SETTINGS_VERSION_PERSIST_KEY)) {
      version = persist_read_int(SETTINGS_VERSION_PERSIST_KEY);

      if (version < CURRENT_SETTINGS_VERSION) {
        // v6 settings: load via memcpy
        LegacyStoredSettings s;
        persist_read_data(SETTINGS_PERSIST_KEY, &s, sizeof(s));
        memcpy(&settings, &s, sizeof(LegacyStoredSettings));

        // re-save in new v7 format
        Settings_saveToStorage();
      } else {
        // v7 settings: load directly via single persist read
        persist_read_data(SETTINGS_PERSIST_KEY, &settings, sizeof(settings));
        settings.altclockName[sizeof(settings.altclockName) - 1] = '\0';
      }
    }
  }

  // Migration to the variable-length widget list: an older persisted blob has
  // widgetCount==0 (the field zero-defaulted). Rebuild the priority list from the
  // legacy widgets[0..2] + widgets2[0..2] (compacting EMPTY entries), once.
  if (settings.widgetCount == 0) {
    int n = 0;
    for (int i = 0; i < 3; i++) {
      if (settings.widgets[i] != EMPTY && settings.widgets[i] <= MAX_WIDGET_TYPE)
        settings.widgetList[n++] = settings.widgets[i];
    }
    for (int i = 0; i < 3; i++) {
      if (settings.widgets2[i] != EMPTY && settings.widgets2[i] <= MAX_WIDGET_TYPE)
        settings.widgetList[n++] = settings.widgets2[i];
    }
    settings.widgetCount = n;
  }

  // Sanitize loaded settings: a value that is out of range (e.g. from an
  // earlier build whose message-key IDs were shifted, writing a color/garbage
  // value into languageId) would index arrays like dayNames[37] out of bounds
  // and crash on every draw -- including at launch, since the bad value is
  // persisted and survives reinstalls. Clamp anything used as an array index
  // back to a safe default so the watchface can always start.
  bool clamped = false;

  if (settings.languageId > LANGUAGE_IW) { settings.languageId = LANGUAGE_EN; clamped = true; }
  if (settings.clockFontId > FONT_SETTING_BOLD_M) { settings.clockFontId = FONT_SETTING_DEFAULT; clamped = true; }
  if (settings.hourlyVibe > VIBE_EVERY_HALF_HOUR) { settings.hourlyVibe = NO_VIBE; clamped = true; }
  for (int i = 0; i < 3; i++) {
    if (settings.widgets[i] > MAX_WIDGET_TYPE) { settings.widgets[i] = EMPTY; clamped = true; }
    if (settings.widgets2[i] > MAX_WIDGET_TYPE) { settings.widgets2[i] = EMPTY; clamped = true; }
  }
  if (settings.widgetCount > MAX_WIDGET_LIST) { settings.widgetCount = MAX_WIDGET_LIST; clamped = true; }
  for (int i = 0; i < settings.widgetCount; i++) {
    if (settings.widgetList[i] > MAX_WIDGET_TYPE) { settings.widgetList[i] = EMPTY; clamped = true; }
  }
  if (settings.decimalSeparator != '.' && settings.decimalSeparator != ',') {
    settings.decimalSeparator = '.'; clamped = true;
  }
  settings.altclockName[sizeof(settings.altclockName) - 1] = '\0';
  if (settings.pollIntervalMin < 5 || settings.pollIntervalMin > 240) {
    settings.pollIntervalMin = 30; clamped = true;
  }
  if (settings.elecQuietStart > 23) { settings.elecQuietStart = 23; clamped = true; }
  if (settings.elecQuietEnd > 23) { settings.elecQuietEnd = 7; clamped = true; }
  if (settings.elecCheapFactorPct < 1 || settings.elecCheapFactorPct > 100) {
    settings.elecCheapFactorPct = 70; clamped = true;
  }

  if (clamped) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "settings out of range, reset to defaults");
    persist_write_data(SETTINGS_PERSIST_KEY, &settings, sizeof(settings));
    persist_write_int(SETTINGS_VERSION_PERSIST_KEY, CURRENT_SETTINGS_VERSION);
  }

  Settings_updateDynamicSettings();
}

void Settings_saveToStorage() {
  Settings_updateDynamicSettings();

  // We're limited to 256 bytes, so make sure that settings fits
  _Static_assert(sizeof(Settings) <= PERSIST_DATA_MAX_LENGTH,
                 "Warning: settings struct is too large!");
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Current settings size %d", sizeof(settings));

  // Write the data
  persist_write_data(SETTINGS_PERSIST_KEY, &settings, sizeof(settings));
  persist_write_int(SETTINGS_VERSION_PERSIST_KEY, CURRENT_SETTINGS_VERSION);
}

void Settings_updateDynamicSettings() {
  dynamicSettings.disableWeather = false;
  dynamicSettings.updateScreenEverySecond = false;
  dynamicSettings.enableAutoBatteryWidget = true;
  dynamicSettings.enableBeats = false;
  dynamicSettings.enableAltTimeZone = false;

  // Scan the full priority list: a widget anywhere in it (even one currently not
  // visible because it didn't fit) should keep its data source / tick enabled,
  // since it can become visible when the status strip toggles.
  for (int i = 0; i < settings.widgetCount; i++) {
    SidebarWidgetType w = settings.widgetList[i];
    if (w == WEATHER_CURRENT || w == WEATHER_FORECAST_TODAY || w == WEATHER_UV_INDEX) {
      dynamicSettings.disableWeather = false;
    }
    if (w == SECONDS) { dynamicSettings.updateScreenEverySecond = true; }
    if (w == BATTERY_METER) { dynamicSettings.enableAutoBatteryWidget = false; }
    if (w == BEATS) { dynamicSettings.enableBeats = true; }
    if (w == ALT_TIME_ZONE) { dynamicSettings.enableAltTimeZone = true; }
  }

  // if the (primary) sidebar background is black, use inverted icon colors.
  // sidebarBgColorLeft is the primary-background key; GColorClear = inherit
  // settings.sidebarColor.
  GColor primaryBg = gcolor_equal(settings.sidebarBgColorLeft, GColorClear)
      ? settings.sidebarColor : settings.sidebarBgColorLeft;
  if (gcolor_equal(primaryBg, GColorBlack)) {
    dynamicSettings.iconFillColor = GColorBlack;
    dynamicSettings.iconStrokeColor = settings.sidebarTextColor;
  } else {
    dynamicSettings.iconFillColor = GColorWhite;
    dynamicSettings.iconStrokeColor = GColorBlack;
  }
}
