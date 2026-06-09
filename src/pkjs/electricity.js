// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pörssisähkö (spot electricity price). Fetches porssisahko.net v2 latest prices
   at most ~twice a day and pushes the whole 48 h quarter-hour schedule to the
   watch, which indexes "now" and computes today's average locally — so no
   Bluetooth traffic is needed between the 1-2 daily fetches. */

var weather = require('./weather');            // reuse xhrRequest helper
var parser = require('./electricity_parse');

var LATEST_PRICES_ENDPOINT = 'https://api.porssisahko.net/v2/latest-prices.json';
var MIN_FETCH_INTERVAL_S = 11 * 3600;          // -> at most ~2 fetches/day

function updateElectricity(forceUpdate) {
  if (window.localStorage.getItem('disable_electricity') === 'yes') {
    return;
  }

  var last = parseInt(window.localStorage.getItem('electricity_last_fetch') || '0', 10);
  var now = Math.floor(Date.now() / 1000);
  if (!forceUpdate && (now - last) < MIN_FETCH_INTERVAL_S) {
    console.log('Electricity: skipping fetch, last was ' + (now - last) + 's ago');
    return;
  }

  weather.xhrRequest(LATEST_PRICES_ENDPOINT, 'GET', function (responseText) {
    var parsed;
    try {
      parsed = parser.parseLatestPrices(JSON.parse(responseText));
    } catch (e) {
      console.log('Electricity: parse error ' + e);
      return;
    }
    if (!parsed.count) {
      console.log('Electricity: no prices in response');
      return;
    }

    var dict = {
      'ElecStartEpoch': parsed.startEpoch,
      'ElecPrices': parsed.bytes
    };

    Pebble.sendAppMessage(dict, function () {
      console.log('Electricity: sent ' + parsed.count + ' quarters to Pebble');
      window.localStorage.setItem('electricity_last_fetch', String(now));
    }, function (e) {
      console.log('Electricity: failed to send to Pebble');
    });
  });
}

module.exports.updateElectricity = updateElectricity;
