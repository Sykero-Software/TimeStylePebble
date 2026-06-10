// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pure coin table + CoinGecko-price -> wire-int parsing. No Pebble/browser
   globals, so it is unit-testable with `node --test`. Each widget is a
   CoinGecko coin priced in USD; toWire converts the USD float to the int16
   sent to the watch (= exactly the displayed precision, so the send-on-change
   check in crypto.js never wakes Bluetooth for an invisible change). */

var COINS = [
  { geckoId: 'bitcoin',   widgetId: 15, messageKey: 'BtcPriceThousands',
    disableKey: 'disable_btc',    lastKey: 'btc_last_thousands',
    toWire: function (usd) { return Math.round(usd / 1000); } },
  { geckoId: 'monero',    widgetId: 16, messageKey: 'XmrPriceDollars',
    disableKey: 'disable_xmr',    lastKey: 'xmr_last_dollars',
    toWire: function (usd) { return Math.round(usd); } },
  { geckoId: 'euro-coin', widgetId: 17, messageKey: 'EurUsdMilli',
    disableKey: 'disable_eurusd', lastKey: 'eurusd_last_milli',
    toWire: function (usd) { return Math.round(usd * 1000); } }
];

// json: { <geckoId>: { usd: <number> } } (CoinGecko simple/price response).
// Returns the coin's wire int, or null if no usable number is present.
function parseWire(json, coin) {
  var usd = json && json[coin.geckoId] ? json[coin.geckoId].usd : undefined;
  if (typeof usd !== 'number' || !isFinite(usd)) {
    return null;
  }
  return coin.toWire(usd);
}

module.exports.COINS = COINS;
module.exports.parseWire = parseWire;
