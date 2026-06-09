const test = require('node:test');
const assert = require('node:assert');
const { parsePriceThousands } = require('../src/pkjs/btc_parse');

test('rounds USD price to nearest thousand', () => {
  assert.strictEqual(parsePriceThousands({ bitcoin: { usd: 63000 } }), 63);
  assert.strictEqual(parsePriceThousands({ bitcoin: { usd: 63499 } }), 63);
  assert.strictEqual(parsePriceThousands({ bitcoin: { usd: 63500 } }), 64);
  assert.strictEqual(parsePriceThousands({ bitcoin: { usd: 102000 } }), 102);
});

test('returns null on missing / malformed price', () => {
  assert.strictEqual(parsePriceThousands({}), null);
  assert.strictEqual(parsePriceThousands({ bitcoin: {} }), null);
  assert.strictEqual(parsePriceThousands({ bitcoin: { usd: 'x' } }), null);
  assert.strictEqual(parsePriceThousands(null), null);
});
