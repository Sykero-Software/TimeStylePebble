// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const { widgetListToPayload } = require('../src/pkjs/widget_list_payload');

test('maps an array of ids to a clamped byte array preserving order', () => {
  assert.deepStrictEqual(widgetListToPayload([12, 15, 17]), [12, 15, 17]);
});

test('drops non-numeric and out-of-range ids', () => {
  assert.deepStrictEqual(widgetListToPayload([12, 'x', 99, 17]), [12, 17]);
});

test('truncates to the 16-entry cap', () => {
  const long = Array.from({ length: 20 }, () => 5);
  assert.strictEqual(widgetListToPayload(long).length, 16);
});

test('non-array input yields an empty payload', () => {
  assert.deepStrictEqual(widgetListToPayload(undefined), []);
});
