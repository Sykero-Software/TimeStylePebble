#include <pebble.h>
#include "weather.h"
#include "settings.h"
#include "twt_status.h"
#include "midi_status.h"
#include "messaging.h"
#include "electricity.h"
#include "languages.h"

void (*message_processed_callback)(void);

// Suppress recording-transition vibration on the first MIDI status message after
// launch, so we don't buzz merely because we learned recording was already active.
static bool s_midiSeen = false;

void messaging_requestNewWeatherData() {
  // just send an empty message for now
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_uint32(iter, 0, 0);
  app_message_outbox_send();
}

void messaging_init(void (*processed_callback)(void)) {
  s_midiSeen = false;
  // register my custom callback
  message_processed_callback = processed_callback;

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  // inbox must hold the 384-byte electricity price table (192 * int16) + overhead
  app_message_open(1024, 64);

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Watch messaging is started!");
  app_message_register_inbox_received(inbox_received_callback);
}

void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  bool weatherDataUpdated = false;

  // does this message contain current weather conditions?
  Tuple *weatherTemp_tuple = dict_find(iterator, MESSAGE_KEY_WeatherTemperature);
  if(weatherTemp_tuple != NULL) {
    Weather_weatherInfo.currentTemp = (int)weatherTemp_tuple->value->int32;
    weatherDataUpdated = true;
  }

  Tuple *weatherConditions_tuple = dict_find(iterator, MESSAGE_KEY_WeatherCondition);
  if(weatherConditions_tuple != NULL) {
    Weather_setCurrentCondition(weatherConditions_tuple->value->int32);
    weatherDataUpdated = true;
  }

  Tuple *weatherUVIndex_tuple = dict_find(iterator, MESSAGE_KEY_WeatherUVIndex);
  if(weatherUVIndex_tuple != NULL) {
    Weather_weatherInfo.currentUVIndex = (int)weatherUVIndex_tuple->value->int32;
    weatherDataUpdated = true;
  }

  Tuple *weatherForecastCondition_tuple = dict_find(iterator, MESSAGE_KEY_WeatherForecastCondition);
  if(weatherForecastCondition_tuple != NULL) {
    Weather_setForecastCondition(weatherForecastCondition_tuple->value->int32);
    weatherDataUpdated = true;
  }

  Tuple *weatherForecastHigh_tuple = dict_find(iterator, MESSAGE_KEY_WeatherForecastHighTemp);
  if(weatherForecastHigh_tuple != NULL) {
    Weather_weatherInfo.todaysHighTemp = (int)weatherForecastHigh_tuple->value->int32;
    weatherDataUpdated = true;
  }

  Tuple *weatherForecastLow_tuple = dict_find(iterator, MESSAGE_KEY_WeatherForecastLowTemp);
  if(weatherForecastLow_tuple != NULL) {
    Weather_weatherInfo.todaysLowTemp = (int)weatherForecastLow_tuple->value->int32;
    weatherDataUpdated = true;
  }

  // only save new weather if weather info was recieved
  if(weatherDataUpdated) {
    Weather_saveData();
  }

  // does this message contain electricity price data?
  bool electricityUpdated = false;
  Tuple *elecStart_tuple = dict_find(iterator, MESSAGE_KEY_ElecStartEpoch);
  if (elecStart_tuple != NULL) {
    Electricity_info.startEpoch = (uint32_t)elecStart_tuple->value->uint32;
    electricityUpdated = true;
  }
  Tuple *elecPrices_tuple = dict_find(iterator, MESSAGE_KEY_ElecPrices);
  if (elecPrices_tuple != NULL) {
    int n = elecPrices_tuple->length / 2;          // little-endian int16 per quarter
    if (n > ELEC_MAX_QUARTERS) { n = ELEC_MAX_QUARTERS; }
    uint8_t *d = elecPrices_tuple->value->data;
    for (int i = 0; i < n; i++) {
      Electricity_info.prices[i] =
          (int16_t)((uint16_t)d[2 * i] | ((uint16_t)d[2 * i + 1] << 8));
    }
    Electricity_info.count = (uint16_t)n;
    electricityUpdated = true;
  }
  if (electricityUpdated) {
    Electricity_saveData();
  }

  // does this message contain new config information?
  Tuple *timeColor_tuple = dict_find(iterator, MESSAGE_KEY_SettingColorTime);
  Tuple *bgColor_tuple = dict_find(iterator, MESSAGE_KEY_SettingColorBG);
  Tuple *sidebarColor_tuple = dict_find(iterator, MESSAGE_KEY_SettingColorSidebar);
  Tuple *sidebarPos_tuple = dict_find(iterator, MESSAGE_KEY_SettingSidebarOnLeft);
  Tuple *sidebarTextColor_tuple = dict_find(iterator, MESSAGE_KEY_SettingSidebarTextColor);
  Tuple *useMetric_tuple = dict_find(iterator, MESSAGE_KEY_SettingUseMetric);
  Tuple *btVibe_tuple = dict_find(iterator, MESSAGE_KEY_SettingBluetoothVibe);
  Tuple *midiVibe_tuple = dict_find(iterator, MESSAGE_KEY_SettingMidiVibe);
  Tuple *bigDate_tuple = dict_find(iterator, MESSAGE_KEY_SettingBigDate);
  Tuple *twtShowRemaining_tuple = dict_find(iterator, MESSAGE_KEY_SettingTwtShowRemaining);
  Tuple *language_tuple = dict_find(iterator, MESSAGE_KEY_SettingLanguageID);
  Tuple *leadingZero_tuple = dict_find(iterator, MESSAGE_KEY_SettingShowLeadingZero);
  Tuple *batteryPct_tuple = dict_find(iterator, MESSAGE_KEY_SettingShowBatteryPct);
  Tuple *clockFont_tuple = dict_find(iterator, MESSAGE_KEY_SettingClockFontId);
  Tuple *hourlyVibe_tuple = dict_find(iterator, MESSAGE_KEY_SettingHourlyVibe);
  Tuple *useLargeFonts_tuple = dict_find(iterator, MESSAGE_KEY_SettingUseLargeFonts);

  Tuple *widget0Id_tuple = dict_find(iterator, MESSAGE_KEY_SettingWidget0ID);
  Tuple *widget1Id_tuple = dict_find(iterator, MESSAGE_KEY_SettingWidget1ID);
  Tuple *widget2Id_tuple = dict_find(iterator, MESSAGE_KEY_SettingWidget2ID);

  Tuple *altclockName_tuple = dict_find(iterator, MESSAGE_KEY_SettingAltClockName);
  Tuple *altclockOffset_tuple = dict_find(iterator, MESSAGE_KEY_SettingAltClockOffset);

  Tuple *decimalSeparator_tuple = dict_find(iterator, MESSAGE_KEY_SettingDecimalSep);
  Tuple *healthUseDistance_tuple = dict_find(iterator, MESSAGE_KEY_SettingHealthUseDistance);
  Tuple *healthUseRestfulSleep_tuple = dict_find(iterator, MESSAGE_KEY_SettingHealthUseRestfulSleep);

  Tuple *autobattery_tuple = dict_find(iterator, MESSAGE_KEY_SettingDisableAutobattery);

  Tuple *activateDisconnectIcon_tuple = dict_find(iterator, MESSAGE_KEY_SettingDisconnectIcon);


  if(timeColor_tuple != NULL) {
    settings.timeColor = GColorFromHEX(timeColor_tuple->value->int32);
  }

  if(bgColor_tuple != NULL) {
    settings.timeBgColor = GColorFromHEX(bgColor_tuple->value->int32);
  }

  if(sidebarColor_tuple != NULL) {
    settings.sidebarColor = GColorFromHEX(sidebarColor_tuple->value->int32);
  }

  if(sidebarTextColor_tuple != NULL) {
    // text can only be black or white, so we'll enforce that here
    settings.sidebarTextColor = GColorFromHEX(sidebarTextColor_tuple->value->int32);
  }

  if(sidebarPos_tuple != NULL) {
    settings.sidebarOnLeft = (bool)sidebarPos_tuple->value->int8;
  }

  if(useMetric_tuple != NULL) {
    settings.useMetric = (bool)useMetric_tuple->value->int8;
  }

  if(btVibe_tuple != NULL) {
    settings.btVibe = (bool)btVibe_tuple->value->int8;
  }

  if(midiVibe_tuple != NULL) {
    settings.midiVibe = (bool)midiVibe_tuple->value->int8;
  }

  if(bigDate_tuple != NULL) {
    settings.showBigDate = (bool)bigDate_tuple->value->int8;
  }

  if(twtShowRemaining_tuple != NULL) {
    settings.twtShowRemaining = (bool)twtShowRemaining_tuple->value->int8;
  }

  if(leadingZero_tuple != NULL) {
    settings.showLeadingZero = (bool)leadingZero_tuple->value->int8;
  }

  if(batteryPct_tuple != NULL) {
    settings.showBatteryPct = (bool)batteryPct_tuple->value->int8;
  }

  if(autobattery_tuple != NULL) {
    settings.disableAutobattery = (bool)autobattery_tuple->value->int8;
  }

  if(clockFont_tuple != NULL && clockFont_tuple->value->int8 >= FONT_SETTING_DEFAULT
     && clockFont_tuple->value->int8 <= FONT_SETTING_BOLD_M) {
    settings.clockFontId = clockFont_tuple->value->int8;
  }

  if(useLargeFonts_tuple != NULL) {
    settings.useLargeFonts = (bool)useLargeFonts_tuple->value->int8;
  }

  if(hourlyVibe_tuple != NULL && hourlyVibe_tuple->value->int8 >= NO_VIBE
     && hourlyVibe_tuple->value->int8 <= VIBE_EVERY_HALF_HOUR) {
    settings.hourlyVibe = hourlyVibe_tuple->value->int8;
  }

  if(language_tuple != NULL && language_tuple->value->int8 >= LANGUAGE_EN
     && language_tuple->value->int8 <= LANGUAGE_IW) {
    settings.languageId = language_tuple->value->int8;
  }

  // Apply widget IDs, but reject out-of-range values. Defends against a stale /
  // mismatched config dict (e.g. from an app-message key-id change) writing
  // garbage widget types into settings.
  if(widget0Id_tuple != NULL && widget0Id_tuple->value->int8 >= EMPTY
     && widget0Id_tuple->value->int8 <= ELECTRICITY) {
    settings.widgets[0] = widget0Id_tuple->value->int8;
  }

  if(widget1Id_tuple != NULL && widget1Id_tuple->value->int8 >= EMPTY
     && widget1Id_tuple->value->int8 <= ELECTRICITY) {
    settings.widgets[1] = widget1Id_tuple->value->int8;
  }

  if(widget2Id_tuple != NULL && widget2Id_tuple->value->int8 >= EMPTY
     && widget2Id_tuple->value->int8 <= ELECTRICITY) {
    settings.widgets[2] = widget2Id_tuple->value->int8;
  }

  if(altclockName_tuple != NULL) {
    strncpy(settings.altclockName, altclockName_tuple->value->cstring, sizeof(settings.altclockName));
  }

  if(altclockOffset_tuple != NULL) {
    settings.altclockOffset = altclockOffset_tuple->value->int8;
  }

  if(decimalSeparator_tuple != NULL) {
    settings.decimalSeparator = (char)decimalSeparator_tuple->value->int8;
  }

  if(healthUseDistance_tuple != NULL) {
    settings.healthUseDistance = (bool)healthUseDistance_tuple->value->int8;
  }

  if(healthUseRestfulSleep_tuple != NULL) {
    settings.healthUseRestfulSleep = (bool)healthUseRestfulSleep_tuple->value->int8;
  }

  if(activateDisconnectIcon_tuple != NULL) {
    settings.activateDisconnectIcon = (bool)activateDisconnectIcon_tuple->value->int8;
  }

  // save the new settings to persistent storage
  Settings_saveToStorage();

  // does this message contain TrackWorkTime status?
  bool twtUpdated = false;
  Tuple *twtTracking_tuple = dict_find(iterator, MESSAGE_KEY_TWT_IS_TRACKING);
  if (twtTracking_tuple != NULL) {
    twt_status.isTracking = (twtTracking_tuple->value->uint8 != 0);
    twtUpdated = true;
  }
  Tuple *twtTaskId_tuple = dict_find(iterator, MESSAGE_KEY_TWT_TASK_ID);
  if (twtTaskId_tuple != NULL) {
    twt_status.taskId = twtTaskId_tuple->value->int32;
    twtUpdated = true;
  }
  Tuple *twtTaskName_tuple = dict_find(iterator, MESSAGE_KEY_TWT_TASK_NAME);
  if (twtTaskName_tuple != NULL) {
    strncpy(twt_status.taskName, twtTaskName_tuple->value->cstring, TWT_TASK_NAME_LEN);
    twt_status.taskName[TWT_TASK_NAME_LEN] = '\0';
    twtUpdated = true;
  }
  Tuple *twtWorked_tuple = dict_find(iterator, MESSAGE_KEY_TWT_WORKED_BEFORE_MIN);
  if (twtWorked_tuple != NULL) {
    twt_status.workedBeforeMin = twtWorked_tuple->value->int32;
    twtUpdated = true;
  }
  Tuple *twtTaskWorked_tuple = dict_find(iterator, MESSAGE_KEY_TWT_TASK_WORKED_BEFORE_MIN);
  if (twtTaskWorked_tuple != NULL) {
    twt_status.taskWorkedBeforeMin = twtTaskWorked_tuple->value->int32;
    twtUpdated = true;
  }
  Tuple *twtSegStart_tuple = dict_find(iterator, MESSAGE_KEY_TWT_SEGMENT_START);
  if (twtSegStart_tuple != NULL) {
    twt_status.segmentStartEpoch = twtSegStart_tuple->value->int32;
    twtUpdated = true;
  }
  Tuple *twtDailyTarget_tuple = dict_find(iterator, MESSAGE_KEY_TWT_DAILY_TARGET_MIN);
  if (twtDailyTarget_tuple != NULL) {
    int32_t t = twtDailyTarget_tuple->value->int32;
    // defence-in-depth: clamp to a sane range (0 .. 24h) so a stale/garbled dict
    // can't produce a wild denominator. 0 means "no target -> hide percent/bar".
    if (t < 0) t = 0;
    if (t > 1440) t = 1440;
    twt_status.dailyTargetMin = t;
    twtUpdated = true;
  }
  if (twtUpdated) {
    TwtStatus_save();
    // the redraw + layout happen via message_processed_callback() (redrawScreen -> apply_twt_layout)
  }

  bool midiUpdated = false;
  Tuple *midiRec_tuple = dict_find(iterator, MESSAGE_KEY_MIDI_IS_RECORDING);
  if (midiRec_tuple != NULL) {
    bool wasRecording = midi_status.isRecording;
    bool isRecording = (midiRec_tuple->value->uint8 != 0);
    midi_status.isRecording = isRecording;
    midiUpdated = true;

    if (settings.midiVibe && s_midiSeen && !quiet_time_is_active()
        && wasRecording != isRecording) {
      if (isRecording) {
        vibes_double_pulse();   // recording started
      } else {
        vibes_long_pulse();     // recording stopped
      }
    }
    s_midiSeen = true;
  }
  Tuple *midiName_tuple = dict_find(iterator, MESSAGE_KEY_MIDI_DEVICE_NAME);
  if (midiName_tuple != NULL) {
    strncpy(midi_status.deviceName, midiName_tuple->value->cstring, MIDI_DEVICE_NAME_LEN);
    midi_status.deviceName[MIDI_DEVICE_NAME_LEN] = '\0';
    midiUpdated = true;
  }
  Tuple *midiStart_tuple = dict_find(iterator, MESSAGE_KEY_MIDI_REC_START);
  if (midiStart_tuple != NULL) {
    midi_status.recStartEpoch = midiStart_tuple->value->int32;
    midiUpdated = true;
  }
  if (midiUpdated) {
    MidiStatus_save();
    // redraw + relayout happen via message_processed_callback() -> redrawScreen -> apply_twt_layout
  }

  // notify the main screen, in case something changed
  message_processed_callback();
}

void inbox_dropped_callback(AppMessageResult reason, void *context) {
  // APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  // APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! %d %d %d", reason, APP_MSG_SEND_TIMEOUT, APP_MSG_SEND_REJECTED);

}

void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  // APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}
