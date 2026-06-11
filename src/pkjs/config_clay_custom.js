// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Clay custom config function: hides settings that are irrelevant under the
   current widget selection, and reveals them live as widgets / gating settings
   change. Runs INSIDE the config webview (Clay serializes it via toSource), so it
   MUST be fully self-contained — no require(), no closure over pkjs scope, ES5
   only. Dependency rationale (verified against src/c/sidebar_widgets.c):
     - widget 14 (current price) uses none of the electricity inputs
     - widget 19 (cheapest hour) uses quiet hours only
     - widget 18 (next cheap) uses quiet hours + cheap factor/floor/ceiling
   See docs/superpowers/specs/2026-06-11-timestyle-conditional-config-settings-design.md */

module.exports = function clayConfigCustom(minified) {
  var clayConfig = this;

  var WIDGET_KEYS = ['SettingWidget0ID', 'SettingWidget1ID', 'SettingWidget2ID',
                     'SettingWidget2_0ID', 'SettingWidget2_1ID', 'SettingWidget2_2ID'];

  function widgetIds() {
    return WIDGET_KEYS.map(function (k) {
      return parseInt(clayConfig.getItemByMessageKey(k).get(), 10);
    });
  }
  function has(ids, set) {
    return set.some(function (v) { return ids.indexOf(v) !== -1; });
  }
  function key(k) { return clayConfig.getItemByMessageKey(k); }
  function byId(i) { return clayConfig.getItemById(i); }
  function toggle(item, on) { if (item) { if (on) { item.show(); } else { item.hide(); } } }

  function update() {
    var ids = widgetIds();
    var weather = has(ids, [7, 8, 13]);
    var temp = has(ids, [7, 8]);
    var manual = weather && key('weather_loc_mode').get() === 'manual';
    var cheapHour = has(ids, [18, 19]);
    var nextCheap = has(ids, [18]);
    var autoBattery = parseInt(key('SettingDisableAutobattery').get(), 10) === 0;

    // Weather
    toggle(byId('heading-weather'), weather);
    toggle(key('weather_loc_mode'), weather);
    toggle(key('weather_datasource'), weather);
    toggle(key('SettingUseMetric'), temp);
    toggle(key('weather_loc'), manual);
    toggle(key('weather_loc_lat'), manual);
    toggle(key('weather_loc_lng'), manual);

    // Electricity
    toggle(byId('heading-electricity'), cheapHour);
    toggle(key('SettingElecQuietStart'), cheapHour);
    toggle(key('SettingElecQuietEnd'), cheapHour);
    toggle(key('SettingElecCheapFactorPct'), nextCheap);
    toggle(key('elec_cheap_floor'), nextCheap);
    toggle(key('elec_cheap_ceiling'), nextCheap);

    // Regional (alternate clock)
    toggle(key('SettingAltClockName'), has(ids, [3]));
    toggle(key('SettingAltClockOffset'), has(ids, [3]));

    // Health
    toggle(key('SettingHealthUseDistance'), has(ids, [10]));
    toggle(key('SettingHealthUseRestfulSleep'), has(ids, [9]));

    // Battery meter style (relevant when the meter can appear)
    toggle(key('SettingShowBatteryPct'), has(ids, [2]) || autoBattery);
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_RENDER, function () {
    update();
    WIDGET_KEYS.concat(['weather_loc_mode', 'SettingDisableAutobattery'])
      .forEach(function (k) { clayConfig.getItemByMessageKey(k).on('change', update); });
  });
};
