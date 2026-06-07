const test = require('node:test');
const assert = require('node:assert');
const { parseLatestPrices } = require('../src/pkjs/electricity_parse');

test('parses prices little-endian int16 with startEpoch', () => {
  const json = {
    prices: [
      { price: 5.23, startDate: '2025-09-19T22:00:00.000Z', endDate: 'x' },
      { price: -0.30, startDate: '2025-09-19T22:15:00.000Z', endDate: 'x' }
    ]
  };
  const r = parseLatestPrices(json);
  assert.strictEqual(r.count, 2);
  assert.strictEqual(r.startEpoch,
    Math.floor(Date.parse('2025-09-19T22:00:00.000Z') / 1000));
  assert.strictEqual(r.bytes.length, 4);
  // 5.23 snt -> round(523) = 523 = 0x020B -> [0x0B, 0x02]
  assert.strictEqual(r.bytes[0], 0x0B);
  assert.strictEqual(r.bytes[1], 0x02);
  // -0.30 snt -> round(-30) = -30 = int16 0xFFE2 -> [0xE2, 0xFF]
  assert.strictEqual(r.bytes[2], 0xE2);
  assert.strictEqual(r.bytes[3], 0xFF);
});

test('caps at 192 quarters', () => {
  const prices = [];
  for (let i = 0; i < 200; i++) {
    prices.push({ price: 1.0, startDate: '2025-09-19T22:00:00.000Z', endDate: '' });
  }
  const r = parseLatestPrices({ prices });
  assert.strictEqual(r.count, 192);
  assert.strictEqual(r.bytes.length, 384);
});

test('empty / missing prices', () => {
  assert.strictEqual(parseLatestPrices({}).count, 0);
  assert.strictEqual(parseLatestPrices({ prices: [] }).startEpoch, 0);
});
