// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Bitcoin price (USD). Fetches CoinGecko when the watch requests phone data (on
   the shared, watch-driven poll interval) and pushes the price to the watch ONLY
   when the displayed value (USD rounded to the nearest thousand) changes — no
   needless Bluetooth wakeups. */

var weather = require('./weather');          // reuse xhrRequest helper
var parser = require('./btc_parse');

var COINGECKO_URL =
  'https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd';

function updateBtc(forceUpdate) {
  if (window.localStorage.getItem('disable_btc') === 'yes') {
    return;
  }
  weather.xhrRequest(COINGECKO_URL, 'GET', function (responseText) {
    var thousands;
    try {
      thousands = parser.parsePriceThousands(JSON.parse(responseText));
    } catch (e) {
      console.log('BTC: parse error ' + e);
      return;
    }
    if (thousands === null) {
      console.log('BTC: no usable price in response');
      return;
    }
    var last = window.localStorage.getItem('btc_last_thousands');
    if (!forceUpdate && last !== null && parseInt(last, 10) === thousands) {
      console.log('BTC: unchanged (' + thousands + 'k), not sending');
      return;
    }
    Pebble.sendAppMessage({ 'BtcPriceThousands': thousands }, function () {
      console.log('BTC: sent ' + thousands + 'k to Pebble');
      window.localStorage.setItem('btc_last_thousands', String(thousands));
    }, function (e) {
      console.log('BTC: failed to send to Pebble');
    });
  });
}

module.exports.updateBtc = updateBtc;
