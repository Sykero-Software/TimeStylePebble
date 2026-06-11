// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const { listToSlots, slotsToList } = require('../src/pkjs/widget_slots');

test('empty / non-array -> six zeros', () => {
  assert.deepStrictEqual(listToSlots([]), [0, 0, 0, 0, 0, 0]);
  assert.deepStrictEqual(listToSlots(undefined), [0, 0, 0, 0, 0, 0]);
  assert.deepStrictEqual(listToSlots(null), [0, 0, 0, 0, 0, 0]);
  assert.deepStrictEqual(listToSlots('nope'), [0, 0, 0, 0, 0, 0]);
});

test('short list is right-padded with 0', () => {
  assert.deepStrictEqual(listToSlots([12, 15, 17]), [12, 15, 17, 0, 0, 0]);
});

test('full list passes through', () => {
  assert.deepStrictEqual(listToSlots([1, 2, 3, 4, 5, 6]), [1, 2, 3, 4, 5, 6]);
});

test('list longer than 6 is truncated', () => {
  assert.deepStrictEqual(listToSlots([1, 2, 3, 4, 5, 6, 7, 8]), [1, 2, 3, 4, 5, 6]);
});

test('string and junk entries coerce to int / 0', () => {
  assert.deepStrictEqual(listToSlots(['7', '8']), [7, 8, 0, 0, 0, 0]);
  assert.deepStrictEqual(listToSlots([null, 'x', 13]), [0, 0, 13, 0, 0, 0]);
});

test('slotsToList: no legacy slot keys -> null (no migration)', () => {
  assert.strictEqual(slotsToList({}), null);
  assert.strictEqual(slotsToList({ SettingColorBG: 16777215 }), null);
});

test('slotsToList: all 6 slots filled -> full array, no trim', () => {
  assert.deepStrictEqual(slotsToList({
    SettingWidget0ID: 12, SettingWidget1ID: 15, SettingWidget2ID: 17,
    SettingWidget2_0ID: 2, SettingWidget2_1ID: 10, SettingWidget2_2ID: 9,
  }), [12, 15, 17, 2, 10, 9]);
});

test('slotsToList: trailing Empty(0) slots are trimmed', () => {
  assert.deepStrictEqual(slotsToList({
    SettingWidget0ID: 12, SettingWidget1ID: 15, SettingWidget2ID: 17,
    SettingWidget2_0ID: 0, SettingWidget2_1ID: 0, SettingWidget2_2ID: 0,
  }), [12, 15, 17]);
});

test('slotsToList: interior Empty(0) is preserved, only trailing trimmed', () => {
  assert.deepStrictEqual(slotsToList({
    SettingWidget0ID: 12, SettingWidget1ID: 0, SettingWidget2ID: 17,
    SettingWidget2_0ID: 5, SettingWidget2_1ID: 0, SettingWidget2_2ID: 0,
  }), [12, 0, 17, 5]);
});

test('slotsToList: string slot values coerce to int; present-but-empty counts as a key', () => {
  // values stored as strings (Clay/getSettings can produce either)
  assert.deepStrictEqual(slotsToList({
    SettingWidget0ID: '7', SettingWidget1ID: '8',
    SettingWidget2ID: '0', SettingWidget2_0ID: '0',
    SettingWidget2_1ID: '0', SettingWidget2_2ID: '0',
  }), [7, 8]);
});

test('slotsToList: all-Empty legacy config -> empty list (present keys, all trimmed)', () => {
  assert.deepStrictEqual(slotsToList({
    SettingWidget0ID: 0, SettingWidget1ID: 0, SettingWidget2ID: 0,
    SettingWidget2_0ID: 0, SettingWidget2_1ID: 0, SettingWidget2_2ID: 0,
  }), []);
});
