// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const {
  normalizeRows, distinctBases, buildRateUrl, packCurrencyData,
  countValidRates, parseLastSent, DELIM,
} = require('../src/pkjs/currency_parse');

const ROWS = [
  { wid: 216, base: 'EUR', quote: 'USD', p: 4, label: '' },
  { wid: 217, base: 'USD', quote: 'JPY', p: 2, label: 'Yen' },
];
const RATES = { EUR: { rates: { USD: 1.0823 } }, USD: { rates: { JPY: 162.4 } } };
// packCurrencyData takes ratesByBase = { base: <rates object> }, so unwrap:
const BY_BASE = { EUR: { USD: 1.0823 }, USD: { JPY: 162.4 } };

test('normalizeRows uppercases codes, clamps decimals, drops malformed', () => {
  const out = normalizeRows([
    { wid: '216', base: 'eur', quote: 'usd', p: '4', label: '' },
    { wid: 217, base: 'USD', quote: '', p: 2, label: 'X' },   // empty quote -> dropped
    { base: 'GBP', quote: 'USD', p: 2 },                       // no wid -> dropped
    { wid: 218, base: 'gbp', quote: 'jpy', p: 99, label: '' }, // p clamps to 6
    'garbage',
  ]);
  assert.strictEqual(out.length, 2);
  assert.deepStrictEqual(out[0], { wid: 216, base: 'EUR', quote: 'USD', p: 4, label: '' });
  assert.deepStrictEqual(out[1], { wid: 218, base: 'GBP', quote: 'JPY', p: 6, label: '' });
});

test('normalizeRows defaults missing/NaN decimals to 4 and floors negatives to 0', () => {
  const out = normalizeRows([
    { wid: 216, base: 'EUR', quote: 'USD', label: '' },        // no p -> 4
    { wid: 217, base: 'USD', quote: 'JPY', p: -3, label: '' }, // negative -> 0
  ]);
  assert.strictEqual(out[0].p, 4);
  assert.strictEqual(out[1].p, 0);
});

test('distinctBases returns unique bases in order', () => {
  assert.deepStrictEqual(distinctBases(ROWS), ['EUR', 'USD']);
  assert.deepStrictEqual(distinctBases([
    { wid: 216, base: 'EUR', quote: 'USD', p: 4, label: '' },
    { wid: 217, base: 'EUR', quote: 'GBP', p: 4, label: '' },
  ]), ['EUR']);
});

test('buildRateUrl targets the open.er-api latest endpoint for the base', () => {
  assert.strictEqual(buildRateUrl('EUR'), 'https://open.er-api.com/v6/latest/EUR');
});

test('packCurrencyData formats each row; label falls back to the quote currency', () => {
  const packed = packCurrencyData(ROWS, BY_BASE, {});
  assert.strictEqual(packed, ['216', 'USD', '1.0823', '217', 'Yen', '162.40'].join(DELIM));
});

test('packCurrencyData uses prevValues when a rate is missing, else --', () => {
  const partial = { EUR: { USD: 1.0823 } };   // USD base absent this round
  const withPrev = packCurrencyData(ROWS, partial, { 217: '161.00' });
  assert.strictEqual(withPrev, ['216', 'USD', '1.0823', '217', 'Yen', '161.00'].join(DELIM));
  const noPrev = packCurrencyData(ROWS, partial, {});
  assert.strictEqual(noPrev, ['216', 'USD', '1.0823', '217', 'Yen', '--'].join(DELIM));
});

test('countValidRates counts rows with a finite fresh rate', () => {
  assert.strictEqual(countValidRates(ROWS, BY_BASE), 2);
  assert.strictEqual(countValidRates(ROWS, { EUR: { USD: 1.0823 } }), 1);
  assert.strictEqual(countValidRates(ROWS, {}), 0);
  assert.strictEqual(countValidRates(ROWS, null), 0);
});

test('parseLastSent maps wid -> value from a packed string', () => {
  const packed = ['216', 'EUR/USD', '1.0823', '217', 'Yen', '162.40'].join(DELIM);
  assert.deepStrictEqual(parseLastSent(packed), { '216': '1.0823', '217': '162.40' });
  assert.deepStrictEqual(parseLastSent(''), {});
});
