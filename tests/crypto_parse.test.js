// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const {
  buildPriceUrl, normalizeRows, packCryptoData, DELIM,
} = require('../src/pkjs/crypto_parse');

const ROWS = [
  { wid: 15, coin: 'bitcoin', vs: 'usd', p: -3, label: 'BTC' },
  { wid: 200, coin: 'ethereum', vs: 'eur', p: 0, label: 'ETH' },
];

test('buildPriceUrl requests unique ids and the union of vs-currencies', () => {
  const url = buildPriceUrl(ROWS);
  assert.ok(url.indexOf('ids=bitcoin,ethereum') !== -1, url);
  assert.ok(/vs_currencies=(usd,eur|eur,usd)/.test(url), url);
  assert.ok(url.indexOf('precision=8') !== -1, url);
});

test('buildPriceUrl dedupes repeated coin ids', () => {
  const url = buildPriceUrl([
    { wid: 15, coin: 'bitcoin', vs: 'usd', p: -3, label: 'BTC' },
    { wid: 200, coin: 'bitcoin', vs: 'eur', p: -3, label: 'BTCE' },
  ]);
  assert.ok(url.indexOf('ids=bitcoin&') !== -1 || url.indexOf('ids=bitcoin&vs') !== -1, url);
});

test('normalizeRows drops malformed rows and coerces types', () => {
  const out = normalizeRows([
    { wid: '15', coin: 'bitcoin', vs: 'usd', p: '-3', label: 'BTC' },
    { wid: 200, coin: '', vs: 'usd', p: 0, label: 'X' },
    { coin: 'monero', vs: 'usd', p: 0, label: 'XMR' },
    'garbage',
  ]);
  assert.strictEqual(out.length, 1);
  assert.deepStrictEqual(out[0], { wid: 15, coin: 'bitcoin', vs: 'usd', p: -3, label: 'BTC' });
});

test('packCryptoData formats each row and joins wid/label/value triplets', () => {
  const json = { bitcoin: { usd: 104235 }, ethereum: { eur: 3520.4 } };
  const packed = packCryptoData(ROWS, json);
  assert.strictEqual(packed, ['15', 'BTC', '104', '200', 'ETH', '3520'].join(DELIM));
});

test('packCryptoData emits -- for a missing price but keeps the slot', () => {
  const json = { bitcoin: { usd: 104235 } };
  const packed = packCryptoData(ROWS, json);
  assert.strictEqual(packed, ['15', 'BTC', '104', '200', 'ETH', '--'].join(DELIM));
});

test('label falls back to the uppercased coin id when empty', () => {
  const packed = packCryptoData(
    [{ wid: 200, coin: 'dogecoin', vs: 'usd', p: 4, label: '' }],
    { dogecoin: { usd: 0.1234 } });
  assert.strictEqual(packed, ['200', 'DOGECOIN', '0.1234'].join(DELIM));
});
