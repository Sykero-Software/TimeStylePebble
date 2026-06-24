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
  assert.deepStrictEqual(widgetListToPayload([216, 100, 199, 22]), []);
  assert.deepStrictEqual(widgetListToPayload([7, 200, 999]), [7, 200]);
});

test('keeps the new Distance widget (id 21, current MAX_WIDGET_TYPE); drops id 22', () => {
  // Guards the MAX_WIDGET_TYPE mirror against src/c/widget_list.h WL_MAX_WIDGET_TYPE:
  // id 21 must survive the payload builder (a stale max=20 would drop it); 22 is out of range.
  assert.deepStrictEqual(widgetListToPayload([9, 20, 10, 21]), [9, 20, 10, 21]);
  assert.deepStrictEqual(widgetListToPayload([21, 22]), [21]);
});

test('passes a valid rotating group through unchanged', () => {
  // battery, group{interval=10s(code1), BTC, XMR}, altTZ
  assert.deepStrictEqual(
    widgetListToPayload([2, 255, 2, 1, 15, 16, 3]),
    [2, 255, 2, 1, 15, 16, 3]);
});

test('drops invalid members and fixes the group count', () => {
  // members 0 (Empty) and 99 dropped -> count 2
  assert.deepStrictEqual(
    widgetListToPayload([255, 3, 0, 15, 99, 16]),
    [255, 2, 0, 15, 16]);
});

test('degrades a single-valid-member group to a plain slot', () => {
  assert.deepStrictEqual(widgetListToPayload([255, 2, 1, 15, 99]), [15]);
});

test('clamps a bad interval code to 1min (3)', () => {
  assert.deepStrictEqual(widgetListToPayload([255, 2, 9, 15, 16]), [255, 2, 3, 15, 16]);
});

test('clamps group count to 6 members', () => {
  const eight = [255, 8, 1, 200, 201, 202, 203, 204, 205, 206, 207];
  // only first 6 members kept
  assert.deepStrictEqual(widgetListToPayload(eight),
    [255, 6, 1, 200, 201, 202, 203, 204, 205]);
});

test('packs whole groups only within the 16-byte cap', () => {
  // two 6-member groups would be 9+9=18 bytes; only the first whole group fits
  const g = [255, 6, 1, 200, 201, 202, 203, 204, 205];
  assert.deepStrictEqual(widgetListToPayload(g.concat(g)), g);
});

test('drops Empty(0) from a plain list (was a no-op widget)', () => {
  assert.deepStrictEqual(widgetListToPayload([12, 0, 17]), [12, 17]);
});

test('preserves the hidden-identifier flag (0x20) on plain ids and group members', () => {
  // 7|0x20 = 39 (hidden weather), 200|0x20 = 232 (hidden crypto)
  assert.deepStrictEqual(widgetListToPayload([39, 232]), [39, 232]);
  // hidden member inside a group: 15|0x20 = 47
  assert.deepStrictEqual(widgetListToPayload([2, 255, 2, 1, 47, 16]), [2, 255, 2, 1, 47, 16]);
});

test('drops a hidden flag when the base id is non-drawable', () => {
  // 22|0x20 = 54; base 22 is not drawable (past MAX_WIDGET_TYPE) -> dropped
  assert.deepStrictEqual(widgetListToPayload([54]), []);
});
