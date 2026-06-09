// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pure CoinGecko price -> displayed thousands. No Pebble/browser globals, so it
   is unit-testable with `node --test`. */

// json: { bitcoin: { usd: <number> } }. Returns the price rounded to the nearest
// thousand (e.g. 63499 -> 63, 63500 -> 64), or null if no usable number present.
function parsePriceThousands(json) {
  var usd = json && json.bitcoin ? json.bitcoin.usd : undefined;
  if (typeof usd !== 'number' || !isFinite(usd)) {
    return null;
  }
  return Math.round(usd / 1000);
}

module.exports.parsePriceThousands = parsePriceThousands;
