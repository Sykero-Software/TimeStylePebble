// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Clay custom config function: hides settings that are irrelevant under the
   current widget selection, and reveals them live as widgets / gating settings
   change. Runs INSIDE the config webview (Clay serializes it via toSource), so it
   MUST be fully self-contained — no require(), no closure over pkjs scope, and
   the EMITTED code must not reference TS downlevel helpers (__spreadArray etc.),
   which live in module scope and would be undefined once the function is
   re-evaluated in the webview. Keep to native array methods; avoid spread /
   destructuring. Dependency rationale (verified against src/c/sidebar_widgets.c):
     - widget 14 (current price) uses none of the electricity inputs
     - widget 19 (cheapest hour) uses quiet hours only
     - widget 18 (next cheap) uses quiet hours + cheap factor/floor/ceiling
   See docs/superpowers/specs/2026-06-11-timestyle-conditional-config-settings-design.md */

interface ClayItem {
  get(): any;
  show(): void;
  hide(): void;
  on(event: string, cb: () => void): void;
}

interface ClayConfigThis {
  getItemByMessageKey(key: string): ClayItem;
  getItemById(id: string): ClayItem | undefined;
  on(event: string, cb: () => void): void;
  EVENTS: { AFTER_BUILD: string };
}

// `minified` is Clay's minified.js helper passed to every custom fn; unused here.
function clayConfigCustom(this: ClayConfigThis, minified: unknown): void {
  const clayConfig = this;

  function widgetIds(): number[] {
    const v = clayConfig.getItemByMessageKey('WidgetList').get();
    if (!v || !v.length) { return []; }
    const ids: number[] = [];
    for (let i = 0; i < v.length; i++) {
      ids.push(parseInt(v[i], 10) || 0);
    }
    return ids;
  }
  function has(ids: number[], set: number[]): boolean {
    return set.some((v) => ids.indexOf(v) !== -1);
  }
  function key(k: string): ClayItem { return clayConfig.getItemByMessageKey(k); }
  function byId(i: string): ClayItem | undefined { return clayConfig.getItemById(i); }
  function toggle(item: ClayItem | undefined, on: boolean): void {
    if (item) { if (on) { item.show(); } else { item.hide(); } }
  }

  function update(): void {
    const ids = widgetIds();
    const weather = has(ids, [7, 8, 13]);
    const temp = has(ids, [7, 8]);
    const manual = weather && key('weather_loc_mode').get() === 'manual';
    const cheapHour = has(ids, [18, 19]);
    const nextCheap = has(ids, [18]);
    const autoBattery = parseInt(key('SettingDisableAutobattery').get(), 10) === 0;

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

  // AFTER_BUILD fires once items are built and have initial values (Clay 1.0.4
  // has no AFTER_RENDER); items are show/hide-able and getters valid by then.
  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, () => {
    update();
    ['WidgetList', 'weather_loc_mode', 'SettingDisableAutobattery']
      .forEach((k) => { clayConfig.getItemByMessageKey(k).on('change', update); });
  });
}

export = clayConfigCustom;
