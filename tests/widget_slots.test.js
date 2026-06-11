// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const { listToSlots } = require('../src/pkjs/widget_slots');

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
