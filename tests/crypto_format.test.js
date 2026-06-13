// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const { formatPrice } = require('../src/pkjs/crypto_format');

test('p >= 0 shows that many decimal places', () => {
  assert.strictEqual(formatPrice(104000, 2), '104000.00');
  assert.strictEqual(formatPrice(104235, 0), '104235');
  assert.strictEqual(formatPrice(0.1234, 4), '0.1234');
  assert.strictEqual(formatPrice(1.15521, 4), '1.1552');
});

test('p < 0 divides by 10^|p| and rounds to an integer (stable across powers of 10)', () => {
  assert.strictEqual(formatPrice(104235, -3), '104');
  assert.strictEqual(formatPrice(99000, -3), '99');
  assert.strictEqual(formatPrice(104500, -3), '105');   // rounds
  assert.strictEqual(formatPrice(2500000, -6), '3');     // millions, rounds
});

test('handles non-finite input defensively', () => {
  assert.strictEqual(formatPrice(NaN, 2), '--');
  assert.strictEqual(formatPrice(Infinity, -3), '--');
});
