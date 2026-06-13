// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const { STATIC_WIDGETS, buildOptions } = require('../src/pkjs/widget_options');

test('STATIC_WIDGETS contains no crypto coin entries (those are dynamic)', () => {
  const ids = STATIC_WIDGETS.map((o) => o.id);
  [15, 16, 17].forEach((cid) => assert.strictEqual(ids.indexOf(cid), -1));
  assert.ok(ids.indexOf(0) !== -1, 'Empty present');
  assert.ok(ids.indexOf(2) !== -1, 'Battery present');
});

test('buildOptions appends one option per crypto row, labelled by label||coin', () => {
  const opts = buildOptions([
    { wid: 15, coin: 'bitcoin', label: 'BTC' },
    { wid: 200, coin: 'dogecoin', label: '' },
  ]);
  const tail = opts.slice(opts.length - 2);
  assert.deepStrictEqual(tail, [
    { id: 15, label: 'BTC' },
    { id: 200, label: 'DOGECOIN' },
  ]);
  assert.strictEqual(opts.length, STATIC_WIDGETS.length + 2);
});

test('buildOptions tolerates an empty / non-array row list', () => {
  assert.deepStrictEqual(buildOptions([]), STATIC_WIDGETS);
  assert.deepStrictEqual(buildOptions(null), STATIC_WIDGETS);
});
