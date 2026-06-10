// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const { COINS, parseWire } = require('../src/pkjs/crypto_parse');

const btc = COINS[0], xmr = COINS[1], eur = COINS[2];

test('coin table is wired to the right widget ids and message keys', () => {
  assert.strictEqual(btc.geckoId, 'bitcoin');
  assert.strictEqual(btc.widgetId, 15);
  assert.strictEqual(btc.messageKey, 'BtcPriceThousands');
  // BTC keeps its historical localStorage names so phone state carries over
  assert.strictEqual(btc.disableKey, 'disable_btc');
  assert.strictEqual(btc.lastKey, 'btc_last_thousands');
  assert.strictEqual(xmr.geckoId, 'monero');
  assert.strictEqual(xmr.widgetId, 16);
  assert.strictEqual(xmr.messageKey, 'XmrPriceDollars');
  assert.strictEqual(eur.geckoId, 'euro-coin');
  assert.strictEqual(eur.widgetId, 17);
  assert.strictEqual(eur.messageKey, 'EurUsdMilli');
});

test('BTC rounds USD to nearest thousand', () => {
  assert.strictEqual(parseWire({ bitcoin: { usd: 63000 } }, btc), 63);
  assert.strictEqual(parseWire({ bitcoin: { usd: 63499 } }, btc), 63);
  assert.strictEqual(parseWire({ bitcoin: { usd: 63500 } }, btc), 64);
  assert.strictEqual(parseWire({ bitcoin: { usd: 102000 } }, btc), 102);
});

test('XMR rounds to whole dollars', () => {
  assert.strictEqual(parseWire({ monero: { usd: 316.2502 } }, xmr), 316);
  assert.strictEqual(parseWire({ monero: { usd: 316.5 } }, xmr), 317);
});

test('EUR/USD scales to milli (3 decimals)', () => {
  assert.strictEqual(parseWire({ 'euro-coin': { usd: 1.1552 } }, eur), 1155);
  assert.strictEqual(parseWire({ 'euro-coin': { usd: 1.1556 } }, eur), 1156);
  assert.strictEqual(parseWire({ 'euro-coin': { usd: 0.9994 } }, eur), 999);
});

test('returns null on missing / malformed price', () => {
  assert.strictEqual(parseWire({}, btc), null);
  assert.strictEqual(parseWire({ monero: {} }, xmr), null);
  assert.strictEqual(parseWire({ bitcoin: { usd: 'x' } }, btc), null);
  assert.strictEqual(parseWire(null, eur), null);
  // response for a different coin only
  assert.strictEqual(parseWire({ bitcoin: { usd: 63000 } }, xmr), null);
});
