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
  settings.timeBgColor = GColorBlack;
  settings.sidebarTextColor = GColorBlack;

#ifdef PBL_COLOR
  settings.timeColor = GColorOrange;
  settings.sidebarColor = GColorOrange;
#else
  settings.timeColor = GColorWhite;
  settings.sidebarColor = GColorWhite;
#endif

  settings.widgets[0] = WEATHER_CURRENT;
  settings.widgets[1] = EMPTY;
  settings.widgets[2] = DATE;

  settings.activateDisconnectIcon = true;
  strncpy(settings.altclockName, "ALT", sizeof(settings.altclockName));
  settings.altclockOffset = 0;
  settings.decimalSeparator = '.';
  settings.showBatteryPct = true;
  settings.midiVibe = false;   // opt-in default; also the upgrade default (appended field, no settings-version bump)
  settings.showBigDate = false;   // opt-in default; appended field, no settings-version bump
  settings.twtShowRemaining = false;   // opt-in default; appended field, no settings-version bump
  settings.twtTargetVibe = false;   // opt-in default; appended field, no settings-version bump
  settings.pollIntervalMin = 30;   // default; appended field, no settings-version bump
  settings.widgets2[0] = EMPTY;   // secondary panel off by default; appended field, no settings-version bump
  settings.widgets2[1] = EMPTY;
  settings.widgets2[2] = EMPTY;
  settings.secondaryAlwaysOn = false;   // auto-hide by default; appended field, no settings-version bump
  settings.elecQuietStart = 23;        // appended field, no settings-version bump
  settings.elecQuietEnd = 7;
  settings.elecCheapFactorPct = 70;
  settings.elecCheapFloorCenti = 200;  // 2.0 snt/kWh
  settings.elecCheapCeilingCenti = 800;// 8.0 snt/kWh
  settings.twtStatusBgColor = GColorClear;    // inherit watchface bg; appended field
  settings.dateBgColor = GColorClear;         // inherit watchface bg; appended field
  settings.sidebarBgColorLeft = GColorClear;  // inherit sidebarColor; appended field
  settings.sidebarBgColorRight = GColorClear; // inherit sidebarColor; appended field

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

  // Scan both panels: a widget in either the primary sidebar or the secondary
  // panel should toggle its corresponding dynamic flag / fetcher.
  for (int pass = 0; pass < 2; pass++) {
    const SidebarWidgetType *w = (pass == 0) ? settings.widgets : settings.widgets2;
    for (int i = 0; i < 3; i++) {
      // if there are any weather widgets, enable weather checking
      if (w[i] == WEATHER_CURRENT ||
          w[i] == WEATHER_FORECAST_TODAY ||
          w[i] == WEATHER_UV_INDEX) {
        dynamicSettings.disableWeather = false;
      }

      // if any widget is "seconds", we'll need to update the sidebar every second
      if (w[i] == SECONDS) {
        dynamicSettings.updateScreenEverySecond = true;
      }

      // if any widget is "battery", disable the automatic battery indication
      if (w[i] == BATTERY_METER) {
        dynamicSettings.enableAutoBatteryWidget = false;
      }

      // if any widget is "beats", enable the beats calculation
      if (w[i] == BEATS) {
        dynamicSettings.enableBeats = true;
      }

      // if any widget is "alt_time_zone", enable the alternative time calculation
      if (w[i] == ALT_TIME_ZONE) {
        dynamicSettings.enableAltTimeZone = true;
      }
    }
  }

  // if the sidebar is black, use inverted colors for icons
  if (gcolor_equal(settings.sidebarColor, GColorBlack)) {
    dynamicSettings.iconFillColor = GColorBlack;
    dynamicSettings.iconStrokeColor = settings.sidebarTextColor;
  } else {
    dynamicSettings.iconFillColor = GColorWhite;
    dynamicSettings.iconStrokeColor = GColorBlack;
  }
}
