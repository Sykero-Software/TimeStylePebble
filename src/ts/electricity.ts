// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pörssisähkö (spot electricity price). Fetches porssisahko.net v2 latest prices
   at most ~twice a day and pushes the whole 48 h quarter-hour schedule to the
   watch, which indexes "now" and computes today's average locally — so no
   Bluetooth traffic is needed between the 1-2 daily fetches. */

import * as weather from './weather';            // reuse xhrRequest helper
import { parseLatestPrices } from './electricity_parse';

const LATEST_PRICES_ENDPOINT = 'https://api.porssisahko.net/v2/latest-prices.json';
const MIN_FETCH_INTERVAL_S = 11 * 3600;          // -> at most ~2 fetches/day

export function updateElectricity(forceUpdate?: boolean): void {
  if (window.localStorage.getItem('disable_electricity') === 'yes') {
    return;
  }

  const last = parseInt(window.localStorage.getItem('electricity_last_fetch') || '0', 10);
  const now = Math.floor(Date.now() / 1000);
  if (!forceUpdate && (now - last) < MIN_FETCH_INTERVAL_S) {
    console.log('Electricity: skipping fetch, last was ' + (now - last) + 's ago');
    return;
  }

  weather.xhrRequest(LATEST_PRICES_ENDPOINT, 'GET', (responseText) => {
    let parsed;
    try {
      parsed = parseLatestPrices(JSON.parse(responseText));
    } catch (e) {
      console.log('Electricity: parse error ' + e);
      return;
    }
    if (!parsed.count) {
      console.log('Electricity: no prices in response');
      return;
    }

    const dict = {
      ElecStartEpoch: parsed.startEpoch,
      ElecPrices: parsed.bytes,
    };

    Pebble.sendAppMessage(dict, () => {
      console.log('Electricity: sent ' + parsed.count + ' quarters to Pebble');
      window.localStorage.setItem('electricity_last_fetch', String(now));
    }, () => {
      console.log('Electricity: failed to send to Pebble');
    });
  });
}
