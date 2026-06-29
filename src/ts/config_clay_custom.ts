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
    const ids: number[] = [];
    ['WidgetList', 'WidgetListRight'].forEach((k) => {
      const v = clayConfig.getItemByMessageKey(k).get();
      if (v && v.length) {
        for (let i = 0; i < v.length; i++) { ids.push((parseInt(v[i], 10) || 0) & 0xdf); }
      }
    });
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
  function injectFloatingSaveStyle(): void {
    if (typeof document === 'undefined') { return; }
    if (document.getElementById('ts-floating-save')) { return; }
    const style = document.createElement('style');
    style.id = 'ts-floating-save';
    style.textContent =
      '.component-submit{position:fixed;bottom:0;left:0;right:0;margin:0;' +
      'z-index:100;background:#262626;padding:8px 0;' +
      'box-shadow:0 -2px 6px rgba(0,0,0,0.4);}' +
      // Reserve clearance for the last setting on the SCROLLING FORM, not body.
      // Clay's base CSS sets html,body{height:100%} (border-box). With a
      // fixed-height body whose content overflows, a body padding-bottom sits
      // INSIDE the body box (at the first screen's bottom), never after the
      // overflowing content — so it reserves no trailing space and the fixed
      // Save bar overlaps the last setting (the reported bug). #main-form is the
      // in-flow content that grows with the page and sets the scroll extent, so
      // its padding-bottom does create real clearance. 96px clears the bar
      // (~72px: button line-box 1.4rem + 0.6rem padding + 0.7rem margin + the
      // 8px container padding top/bottom) with a comfortable margin.
      '#main-form{padding-bottom:96px;}';
    document.head.appendChild(style);
  }

  function update(): void {
    const ids = widgetIds();
    const weather = has(ids, [7, 8, 13]);
    const temp = has(ids, [7, 8]);
    const manual = weather && key('weather_loc_mode').get() === 'manual';
    const cheapHour = has(ids, [18, 19]);
    const nextCheap = has(ids, [18]);
    const autoBattery = parseInt(key('SettingDisableAutobattery').get(), 10) === 0;
    const fallbackManual = parseInt(key('SettingFallbackColumn').get(), 10) !== 0;
    const analog = parseInt(key('SettingClockStyle').get(), 10) === 1;

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

    // Crypto: show the section heading once any crypto coin widget is placed.
    const anyCrypto = ids.some((id) =>
      id === 15 || id === 16 || id === 17 || (id >= 200 && id < 216));
    toggle(byId('heading-crypto'), anyCrypto);

    // Regional (alternate clock)
    toggle(key('SettingAltClockName'), has(ids, [3]));
    toggle(key('SettingAltClockOffset'), has(ids, [3]));

    // Battery meter style (relevant when the meter can appear)
    toggle(key('SettingShowBatteryPct'), has(ids, [2]) || autoBattery);

    // Auto-battery threshold: only relevant when auto-battery is on.
    toggle(key('SettingAutoBatteryThreshold'), autoBattery);
    // Fallback position: only when a specific (non-Automatic) column is chosen.
    toggle(key('SettingFallbackPosition'), fallbackManual);

    // Analog clock: these rows are only relevant when the analog face is selected.
    toggle(key('SettingAnalogTicks'), analog);
    toggle(key('SettingAnalogDigitalClock'), analog);
    toggle(byId('analog-credit'), analog);
  }

  // AFTER_BUILD fires once items are built and have initial values (Clay 1.0.4
  // has no AFTER_RENDER); items are show/hide-able and getters valid by then.
  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, () => {
    injectFloatingSaveStyle();
    update();
    ['WidgetList', 'WidgetListRight', 'weather_loc_mode', 'SettingDisableAutobattery', 'SettingFallbackColumn', 'SettingClockStyle']
      .forEach((k) => { clayConfig.getItemByMessageKey(k).on('change', update); });
  });
}

export = clayConfigCustom;
