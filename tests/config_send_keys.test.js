// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const { STRAIGHT_THROUGH_KEYS } = require('../src/pkjs/config_send_keys');
const configArr = require('../src/pkjs/config_clay');

// Mirror the section/item shape of config_clay (sections carry an items[] array).
function flattenConfig(cfg) {
  const out = [];
  cfg.forEach((entry) => {
    if (entry.type === 'section' && Array.isArray(entry.items)) {
      entry.items.forEach((it) => out.push(it));
    } else {
      out.push(entry);
    }
  });
  return out;
}

// Every boolean toggle the watch reads is a straight-through scalar (booleans, not
// colours/lists), so all of them MUST be in STRAIGHT_THROUGH_KEYS. A toggle absent
// from the send path renders/gates fine in the config webview but its value never
// reaches the watch — it silently stays at the C default. This is exactly how
// SettingStatusClockDigital shipped broken; this test fails if any toggle is
// forgotten again.
test('every config toggle setting is wired into the straight-through send path', () => {
  const set = new Set(STRAIGHT_THROUGH_KEYS);
  const toggles = flattenConfig(configArr)
    .filter((it) => it && it.type === 'toggle' && typeof it.messageKey === 'string'
      && it.messageKey.indexOf('Setting') === 0)
    .map((it) => it.messageKey);
  assert.ok(toggles.length > 0, 'expected at least one Setting* toggle in the config');
  const missing = toggles.filter((k) => !set.has(k));
  assert.deepStrictEqual(missing, [],
    'toggle settings missing from STRAIGHT_THROUGH_KEYS (their config value never reaches the watch): '
    + missing.join(', '));
});

test('SettingStatusClockDigital reaches the watch', () => {
  assert.ok(STRAIGHT_THROUGH_KEYS.indexOf('SettingStatusClockDigital') !== -1,
    'SettingStatusClockDigital must be sent so the status-strip digital-clock swap works on a real watch');
});
