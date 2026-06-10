// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Crypto/currency prices (USD) via CoinGecko. One request per poll fetches all
   enabled coins; each value is pushed to the watch ONLY when its displayed
   value changes — no needless Bluetooth wakeups. Coin table + parsing live in
   crypto_parse.js (pure, unit-tested). */

var weather = require('./weather');          // reuse xhrRequest helper
var parser = require('./crypto_parse');

var BASE_URL = 'https://api.coingecko.com/api/v3/simple/price';

function updateCrypto(forceUpdate) {
  var coins = parser.COINS.filter(function (c) {
    return window.localStorage.getItem(c.disableKey) !== 'yes';
  });
  if (coins.length === 0) {
    return;
  }
  var url = BASE_URL + '?ids=' + coins.map(function (c) {
    return c.geckoId;
  }).join(',') + '&vs_currencies=usd&precision=4';

  weather.xhrRequest(url, 'GET', function (responseText) {
    var json;
    try {
      json = JSON.parse(responseText);
    } catch (e) {
      console.log('crypto: parse error ' + e);
      return;
    }
    var dict = {};
    var pending = [];
    coins.forEach(function (c) {
      var wire = parser.parseWire(json, c);
      if (wire === null) {
        console.log('crypto: no usable ' + c.geckoId + ' price in response');
        return;
      }
      var last = window.localStorage.getItem(c.lastKey);
      if (!forceUpdate && last !== null && parseInt(last, 10) === wire) {
        return;
      }
      dict[c.messageKey] = wire;
      pending.push({ coin: c, wire: wire });
    });
    if (pending.length === 0) {
      console.log('crypto: nothing changed, not sending');
      return;
    }
    Pebble.sendAppMessage(dict, function () {
      pending.forEach(function (p) {
        window.localStorage.setItem(p.coin.lastKey, String(p.wire));
      });
      console.log('crypto: sent ' + JSON.stringify(dict));
    }, function (e) {
      console.log('crypto: failed to send to Pebble');
    });
  });
}

module.exports.updateCrypto = updateCrypto;
