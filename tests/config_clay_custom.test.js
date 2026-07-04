// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const clayConfigCustom = require('../src/pkjs/config_clay_custom');

const GATED_KEYS = ['SettingUseMetric', 'weather_loc_mode', 'weather_datasource',
  'weather_loc', 'weather_loc_lat', 'weather_loc_lng',
  'SettingElecQuietStart', 'SettingElecQuietEnd', 'SettingElecCheapFactorPct',
  'elec_cheap_floor', 'elec_cheap_ceiling', 'SettingAltClockName',
  'SettingAltClockOffset',
  'SettingShowBatteryPct', 'SettingDisableAutobattery',
  'SettingAutoBatteryThreshold', 'SettingFallbackColumn', 'SettingFallbackPosition',
  'SettingClockStyle', 'SettingAnalogDigitalClock', 'SettingAnalogTicks',
  'SettingBigDateMonth', 'SettingBigDateFont'];

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

function makeDocument() {
  const head = { children: [], appendChild(el) { this.children.push(el); } };
  return {
    head: head,
    getElementById(id) {
      for (let i = 0; i < head.children.length; i++) {
        if (head.children[i].id === id) { return head.children[i]; }
      }
      return null;
    },
    createElement(tag) { return { tagName: tag, id: '', textContent: '' }; }
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
  byKey['SettingFallbackColumn'].value = String(opts.fallbackColumn !== undefined ? opts.fallbackColumn : 0);
  byKey['SettingClockStyle'].value = String(opts.clockStyle !== undefined ? opts.clockStyle : 0);
  byKey['SettingBigDate'] = makeItem(String(opts.bigDate !== undefined ? opts.bigDate : 1));
  byId['heading-weather'] = makeItem('');
  byId['heading-electricity'] = makeItem('');
  byId['analog-credit'] = makeItem('');
  byId['heading-currency'] = makeItem('');
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

test('no widgets: weather/electricity/alt hidden; battery style shown (auto-battery on)', () => {
  const c = render([]);
  assert.strictEqual(c.byId['heading-weather'].shown, false);
  assert.strictEqual(c.byKey['SettingUseMetric'].shown, false);
  assert.strictEqual(c.byKey['weather_loc_mode'].shown, false);
  assert.strictEqual(c.byId['heading-electricity'].shown, false);
  assert.strictEqual(c.byKey['SettingElecQuietStart'].shown, false);
  assert.strictEqual(c.byKey['SettingElecCheapFactorPct'].shown, false);
  assert.strictEqual(c.byKey['SettingAltClockName'].shown, false);
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

test('hidden-identifier flag (0x20) still gates the widget\'s settings', () => {
  const c = render([7 | 0x20]);   // hidden weather is still weather
  assert.strictEqual(c.byId['heading-weather'].shown, true);
  assert.strictEqual(c.byKey['SettingUseMetric'].shown, true);
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

test('battery threshold: shown when auto-battery on, hidden when off', () => {
  const on = render([]);
  assert.strictEqual(on.byKey['SettingAutoBatteryThreshold'].shown, true);
  const off = render([], { autoBatteryDisabled: true });
  assert.strictEqual(off.byKey['SettingAutoBatteryThreshold'].shown, false);
});

test('fallback position: hidden in Automatic column, shown for Left/Right', () => {
  const auto = render([], { fallbackColumn: 0 });
  assert.strictEqual(auto.byKey['SettingFallbackPosition'].shown, false);
  const left = render([], { fallbackColumn: 1 });
  assert.strictEqual(left.byKey['SettingFallbackPosition'].shown, true);
  const right = render([], { fallbackColumn: 2 });
  assert.strictEqual(right.byKey['SettingFallbackPosition'].shown, true);
});

test('live change: choosing a fallback column reveals the position input', () => {
  const c = render([], { fallbackColumn: 0 });
  assert.strictEqual(c.byKey['SettingFallbackPosition'].shown, false);
  c.byKey['SettingFallbackColumn'].value = '1';
  c.byKey['SettingFallbackColumn'].changeHandlers.forEach((fn) => fn());
  assert.strictEqual(c.byKey['SettingFallbackPosition'].shown, true);
});

test('live change: disabling auto-battery hides the threshold chooser', () => {
  const c = render([]);
  assert.strictEqual(c.byKey['SettingAutoBatteryThreshold'].shown, true);
  c.byKey['SettingDisableAutobattery'].value = '1';
  c.byKey['SettingDisableAutobattery'].changeHandlers.forEach((fn) => fn());
  assert.strictEqual(c.byKey['SettingAutoBatteryThreshold'].shown, false);
});

test('floating save: AFTER_BUILD injects a fixed-position style once (idempotent)', () => {
  const prev = global.document;
  global.document = makeDocument();
  try {
    const c = render([]);   // render() fires AFTER_BUILD once
    const styles = global.document.head.children;
    assert.strictEqual(styles.length, 1, 'exactly one <style> injected');
    assert.strictEqual(styles[0].tagName, 'style');
    assert.match(styles[0].textContent, /\.component-submit/);
    assert.match(styles[0].textContent, /position\s*:\s*fixed/);
    // Clearance for the last setting MUST be reserved on the scrolling form
    // (#main-form), NOT body: Clay sets html,body{height:100%}, so body
    // padding-bottom is swallowed inside the fixed-height body box and never
    // clears the fixed Save bar (the original 8.x bug — last setting unreachable).
    assert.match(styles[0].textContent, /#main-form\s*\{[^}]*padding-bottom/);
    assert.doesNotMatch(styles[0].textContent, /(^|[^-])\bbody\s*\{[^}]*padding-bottom/,
      'must NOT reserve clearance on body (height:100% swallows it)');
    // Firing AFTER_BUILD again must not add a duplicate <style>.
    c._handlers.AFTER_BUILD();
    assert.strictEqual(global.document.head.children.length, 1, 'no duplicate <style>');
  } finally {
    global.document = prev;
  }
});

test('digital clock style: analog rows hidden', () => {
  const c = render([], { clockStyle: 0 });
  assert.strictEqual(c.byKey['SettingAnalogDigitalClock'].shown, false);
  assert.strictEqual(c.byKey['SettingAnalogTicks'].shown, false);
  assert.strictEqual(c.byId['analog-credit'].shown, false);
});

test('analog clock style: analog rows shown', () => {
  const c = render([], { clockStyle: 1 });
  assert.strictEqual(c.byKey['SettingAnalogDigitalClock'].shown, true);
  assert.strictEqual(c.byKey['SettingAnalogTicks'].shown, true);
  assert.strictEqual(c.byId['analog-credit'].shown, true);
});

test('live change: switching to analog reveals the analog rows', () => {
  const c = render([], { clockStyle: 0 });
  assert.strictEqual(c.byKey['SettingAnalogDigitalClock'].shown, false);
  c.byKey['SettingClockStyle'].value = '1';
  c.byKey['SettingClockStyle'].changeHandlers.forEach((fn) => fn());
  assert.strictEqual(c.byKey['SettingAnalogDigitalClock'].shown, true);
  assert.strictEqual(c.byKey['SettingAnalogTicks'].shown, true);
});

test('big date shown: month toggle visible', () => {
  const c = render([], { bigDate: 1 });
  assert.strictEqual(c.byKey['SettingBigDateMonth'].shown, true);
});

test('big date none: month toggle hidden', () => {
  const c = render([], { bigDate: 0 });
  assert.strictEqual(c.byKey['SettingBigDateMonth'].shown, false);
});

test('currency pair widget (216): currency heading shown', () => {
  const c = render([216]);
  assert.strictEqual(c.byId['heading-currency'].shown, true);
});

test('no currency widget (weather only): currency heading hidden', () => {
  const c = render([7]);
  assert.strictEqual(c.byId['heading-currency'].shown, false);
});

test('currency wid boundary: 222 is currency, 223 is not', () => {
  assert.strictEqual(render([222]).byId['heading-currency'].shown, true);
  // 223 is excluded from the currency range (223|0x20 == 0xFF, the rotating marker).
  assert.strictEqual(render([223]).byId['heading-currency'].shown, false);
});

test('right-list currency widget reveals the currency heading', () => {
  const c = render([], { rightVals: [216] });
  assert.strictEqual(c.byId['heading-currency'].shown, true);
});

test('live change: turning the big date off hides the month toggle', () => {
  const c = render([], { bigDate: 1 });
  assert.strictEqual(c.byKey['SettingBigDateMonth'].shown, true);
  c.byKey['SettingBigDate'].value = '0';
  c.byKey['SettingBigDate'].changeHandlers.forEach((fn) => fn());
  assert.strictEqual(c.byKey['SettingBigDateMonth'].shown, false);
});

test('big date shown: font picker visible', () => {
  const c = render([], { bigDate: 1 });
  assert.strictEqual(c.byKey['SettingBigDateFont'].shown, true);
});

test('big date none: font picker hidden', () => {
  const c = render([], { bigDate: 0 });
  assert.strictEqual(c.byKey['SettingBigDateFont'].shown, false);
});
