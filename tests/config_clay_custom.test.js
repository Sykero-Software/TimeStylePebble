// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const clayConfigCustom = require('../src/pkjs/config_clay_custom');

const GATED_KEYS = ['SettingUseMetric', 'weather_loc_mode', 'weather_datasource',
  'weather_loc', 'weather_loc_lat', 'weather_loc_lng',
  'SettingElecQuietStart', 'SettingElecQuietEnd', 'SettingElecCheapFactorPct',
  'elec_cheap_floor', 'elec_cheap_ceiling', 'SettingAltClockName',
  'SettingAltClockOffset', 'SettingHealthUseDistance',
  'SettingShowBatteryPct', 'SettingDisableAutobattery'];

function makeItem(value) {
  return {
    value: value,
    shown: true,
    changeHandlers: [],
    get() { return this.value; },
    show() { this.shown = true; },
    hide() { this.shown = false; },
    on(_ev, fn) { this.changeHandlers.push(fn); }
  };
}

// widgetVals: left-list widget IDs. opts: {locMode, autoBatteryDisabled, rightVals}
function makeClay(widgetVals, opts) {
  opts = opts || {};
  const byKey = {};
  const byId = {};
  // widgetList component value is the array of selected widget ids (ints)
  byKey['WidgetList'] = makeItem((widgetVals || []).map((v) => parseInt(v, 10) || 0));
  byKey['WidgetListRight'] = makeItem((opts.rightVals || []).map((v) => parseInt(v, 10) || 0));
  GATED_KEYS.forEach((k) => { byKey[k] = makeItem(''); });
  byKey['weather_loc_mode'].value = opts.locMode || 'auto';
  byKey['SettingDisableAutobattery'].value = String(opts.autoBatteryDisabled ? 1 : 0);
  byId['heading-weather'] = makeItem('');
  byId['heading-electricity'] = makeItem('');
  return {
    // Mirror Clay 1.0.4's real event set (lib/clay-config.js). There is NO
    // AFTER_RENDER — using a missing constant passes `undefined` to on(), which
    // Clay's _transformEventNames crashes on (`undefined.split`). The mock below
    // reproduces that crash so a wrong event constant fails the tests.
    EVENTS: { BEFORE_BUILD: 'BEFORE_BUILD', AFTER_BUILD: 'AFTER_BUILD',
              BEFORE_DESTROY: 'BEFORE_DESTROY', AFTER_DESTROY: 'AFTER_DESTROY' },
    _handlers: {},
    getItemByMessageKey(k) { return byKey[k]; },
    getItemById(i) { return byId[i]; },
    on(ev, fn) {
      // ClayEvents.on() does events.split(' ') immediately -> TypeError if ev is
      // undefined (the AFTER_RENDER bug). Reproduce that here.
      if (typeof ev !== 'string') {
        throw new TypeError("Cannot read properties of undefined (reading 'split')");
      }
      this._handlers[ev] = fn;
    },
    byKey,
    byId
  };
}

function render(widgetVals, opts) {
  const clay = makeClay(widgetVals, opts);
  clayConfigCustom.call(clay, {});  // minified arg unused by logic
  // Clay fires AFTER_BUILD once items are built; simulate it.
  assert.ok(clay._handlers.AFTER_BUILD,
    'custom fn must register an AFTER_BUILD handler');
  clay._handlers.AFTER_BUILD();
  return clay;
}

test('no widgets: weather/electricity/alt/health hidden; battery style shown (auto-battery on)', () => {
  const c = render([]);
  assert.strictEqual(c.byId['heading-weather'].shown, false);
  assert.strictEqual(c.byKey['SettingUseMetric'].shown, false);
  assert.strictEqual(c.byKey['weather_loc_mode'].shown, false);
  assert.strictEqual(c.byId['heading-electricity'].shown, false);
  assert.strictEqual(c.byKey['SettingElecQuietStart'].shown, false);
  assert.strictEqual(c.byKey['SettingElecCheapFactorPct'].shown, false);
  assert.strictEqual(c.byKey['SettingAltClockName'].shown, false);
  assert.strictEqual(c.byKey['SettingHealthUseDistance'].shown, false);
  assert.strictEqual(c.byKey['SettingShowBatteryPct'].shown, true);
});

test('weather temp widget (7): weather shown incl units; manual fields hidden in auto mode', () => {
  const c = render([7]);
  assert.strictEqual(c.byId['heading-weather'].shown, true);
  assert.strictEqual(c.byKey['weather_loc_mode'].shown, true);
  assert.strictEqual(c.byKey['weather_datasource'].shown, true);
  assert.strictEqual(c.byKey['SettingUseMetric'].shown, true);
  assert.strictEqual(c.byKey['weather_loc'].shown, false);
  assert.strictEqual(c.byKey['weather_loc_lat'].shown, false);
  assert.strictEqual(c.byKey['weather_loc_lng'].shown, false);
});

test('UV index only (13): weather shown but units hidden (UV is unitless)', () => {
  const c = render([13]);
  assert.strictEqual(c.byId['heading-weather'].shown, true);
  assert.strictEqual(c.byKey['weather_loc_mode'].shown, true);
  assert.strictEqual(c.byKey['SettingUseMetric'].shown, false);
});

test('weather + manual location: manual fields shown', () => {
  const c = render([8], { locMode: 'manual' });
  assert.strictEqual(c.byKey['weather_loc'].shown, true);
  assert.strictEqual(c.byKey['weather_loc_lat'].shown, true);
  assert.strictEqual(c.byKey['weather_loc_lng'].shown, true);
});

test('current-price electricity only (14): whole electricity section hidden', () => {
  const c = render([14]);
  assert.strictEqual(c.byId['heading-electricity'].shown, false);
  assert.strictEqual(c.byKey['SettingElecQuietStart'].shown, false);
  assert.strictEqual(c.byKey['SettingElecCheapFactorPct'].shown, false);
});

test('cheapest-hour (19): heading + quiet hours shown; cheap factor/floor/ceiling hidden', () => {
  const c = render([19]);
  assert.strictEqual(c.byId['heading-electricity'].shown, true);
  assert.strictEqual(c.byKey['SettingElecQuietStart'].shown, true);
  assert.strictEqual(c.byKey['SettingElecQuietEnd'].shown, true);
  assert.strictEqual(c.byKey['SettingElecCheapFactorPct'].shown, false);
  assert.strictEqual(c.byKey['elec_cheap_floor'].shown, false);
  assert.strictEqual(c.byKey['elec_cheap_ceiling'].shown, false);
});

test('next-cheap (18): quiet hours + cheap factor/floor/ceiling all shown', () => {
  const c = render([18]);
  assert.strictEqual(c.byId['heading-electricity'].shown, true);
  assert.strictEqual(c.byKey['SettingElecQuietStart'].shown, true);
  assert.strictEqual(c.byKey['SettingElecCheapFactorPct'].shown, true);
  assert.strictEqual(c.byKey['elec_cheap_floor'].shown, true);
  assert.strictEqual(c.byKey['elec_cheap_ceiling'].shown, true);
});

test('alt time zone widget (3): alt clock name + offset shown', () => {
  const c = render([3]);
  assert.strictEqual(c.byKey['SettingAltClockName'].shown, true);
  assert.strictEqual(c.byKey['SettingAltClockOffset'].shown, true);
});

test('steps (10) shows distance toggle; sleep widget gates no setting now', () => {
  const cs = render([10]);
  assert.strictEqual(cs.byKey['SettingHealthUseDistance'].shown, true);
  const cl = render([9]);
  assert.strictEqual(cl.byKey['SettingHealthUseDistance'].shown, false);
});

test('battery style: shown with battery widget even if auto-battery off', () => {
  const c = render([2], { autoBatteryDisabled: true });
  assert.strictEqual(c.byKey['SettingShowBatteryPct'].shown, true);
});

test('battery style: hidden when no battery widget AND auto-battery off', () => {
  const c = render([], { autoBatteryDisabled: true });
  assert.strictEqual(c.byKey['SettingShowBatteryPct'].shown, false);
});

test('live change: selecting a weather widget reveals weather after re-render', () => {
  const c = render([]);
  assert.strictEqual(c.byId['heading-weather'].shown, false);
  c.byKey['WidgetList'].value = [7];
  c.byKey['WidgetList'].changeHandlers.forEach((fn) => fn());
  assert.strictEqual(c.byId['heading-weather'].shown, true);
  assert.strictEqual(c.byKey['SettingUseMetric'].shown, true);
});

test('live change: switching weather location to manual reveals lat/lng/label', () => {
  const c = render([7]);  // weather present, auto mode -> manual fields hidden
  assert.strictEqual(c.byKey['weather_loc'].shown, false);
  c.byKey['weather_loc_mode'].value = 'manual';
  c.byKey['weather_loc_mode'].changeHandlers.forEach((fn) => fn());
  assert.strictEqual(c.byKey['weather_loc'].shown, true);
  assert.strictEqual(c.byKey['weather_loc_lat'].shown, true);
  assert.strictEqual(c.byKey['weather_loc_lng'].shown, true);
});

test('live change: disabling auto-battery hides battery style when no battery widget', () => {
  const c = render([]);  // no widgets, auto-battery on -> style shown
  assert.strictEqual(c.byKey['SettingShowBatteryPct'].shown, true);
  c.byKey['SettingDisableAutobattery'].value = '1';
  c.byKey['SettingDisableAutobattery'].changeHandlers.forEach((fn) => fn());
  assert.strictEqual(c.byKey['SettingShowBatteryPct'].shown, false);
});

test('right-list weather widget reveals weather section', () => {
  const c = render([], { rightVals: [7] });
  assert.strictEqual(c.byId['heading-weather'].shown, true);
  assert.strictEqual(c.byKey['SettingUseMetric'].shown, true);
});

test('live change: adding a weather widget to the right list reveals weather', () => {
  const c = render([]);
  assert.strictEqual(c.byId['heading-weather'].shown, false);
  c.byKey['WidgetListRight'].value = [7];
  c.byKey['WidgetListRight'].changeHandlers.forEach((fn) => fn());
  assert.strictEqual(c.byId['heading-weather'].shown, true);
});
