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

test('keeps crypto wids (legacy 15/16/17 and the 200+ range), drops other out-of-range', () => {
  assert.deepStrictEqual(widgetListToPayload([15, 16, 17, 200, 215]), [15, 16, 17, 200, 215]);
  assert.deepStrictEqual(widgetListToPayload([216, 100, 199, 20]), []);
  assert.deepStrictEqual(widgetListToPayload([7, 200, 999]), [7, 200]);
});
