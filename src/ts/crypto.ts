// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Crypto/currency prices (USD) via CoinGecko. One request per poll fetches all
   enabled coins; each value is pushed to the watch ONLY when its displayed
   value changes — no needless Bluetooth wakeups. Coin table + parsing live in
   crypto_parse.ts (pure, unit-tested). */

import * as weather from './weather';          // reuse xhrRequest helper
import { COINS, parseWire, Coin } from './crypto_parse';

const BASE_URL = 'https://api.coingecko.com/api/v3/simple/price';

export function updateCrypto(forceUpdate?: boolean): void {
  const coins = COINS.filter((c) => window.localStorage.getItem(c.disableKey) !== 'yes');
  if (coins.length === 0) {
    return;
  }
  const url = BASE_URL + '?ids=' + coins.map((c) => c.geckoId).join(',') +
    '&vs_currencies=usd&precision=4';

  weather.xhrRequest(url, 'GET', (responseText) => {
    let json;
    try {
      json = JSON.parse(responseText);
    } catch (e) {
      console.log('crypto: parse error ' + e);
      return;
    }
    const dict: Record<string, number> = {};
    const pending: { coin: Coin; wire: number }[] = [];
    coins.forEach((c) => {
      const wire = parseWire(json, c);
      if (wire === null) {
        console.log('crypto: no usable ' + c.geckoId + ' price in response');
        return;
      }
      const last = window.localStorage.getItem(c.lastKey);
      if (!forceUpdate && last !== null && parseInt(last, 10) === wire) {
        return;
      }
      dict[c.messageKey] = wire;
      pending.push({ coin: c, wire });
    });
    if (pending.length === 0) {
      console.log('crypto: nothing changed, not sending');
      return;
    }
    Pebble.sendAppMessage(dict, () => {
      pending.forEach((p) => {
        window.localStorage.setItem(p.coin.lastKey, String(p.wire));
      });
      console.log('crypto: sent ' + JSON.stringify(dict));
    }, () => {
      console.log('crypto: failed to send to Pebble');
    });
  });
}
