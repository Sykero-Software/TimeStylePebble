// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Crypto/currency prices via CoinGecko, driven by the user's configurable coin
   list (Clay key `CryptoList`, stored in clay-settings localStorage). One
   request per poll fetches every configured coin; the whole packed CryptoData
   string is sent only when it changes vs. the last send — no needless Bluetooth
   wakeups. Pure helpers (url/pack/normalize/format) live in crypto_parse.ts +
   crypto_format.ts (unit-tested). */

import * as weather from './weather';          // reuse xhrRequest helper
import { CoinRow, normalizeRows, buildPriceUrl, packCryptoData } from './crypto_parse';

const LAST_SENT_KEY = 'crypto_last_sent';      // last CryptoData string we pushed

export function readCoinRows(): CoinRow[] {
  let stored: any;
  try {
    stored = JSON.parse(window.localStorage.getItem('clay-settings') || '{}') || {};
  } catch (e) {
    return [];
  }
  return normalizeRows(stored.CryptoList);
}

export function updateCrypto(forceUpdate?: boolean): void {
  if (window.localStorage.getItem('disable_crypto') === 'yes') {
    return;
  }
  const rows = readCoinRows();
  if (rows.length === 0) {
    return;
  }
  const url = buildPriceUrl(rows);

  weather.xhrRequest(url, 'GET', (responseText) => {
    let json;
    try {
      json = JSON.parse(responseText);
    } catch (e) {
      console.log('crypto: parse error ' + e);
      return;
    }
    const packed = packCryptoData(rows, json);
    const last = window.localStorage.getItem(LAST_SENT_KEY);
    if (!forceUpdate && last === packed) {
      console.log('crypto: nothing changed, not sending');
      return;
    }
    Pebble.sendAppMessage({ CryptoData: packed }, () => {
      window.localStorage.setItem(LAST_SENT_KEY, packed);
      console.log('crypto: sent ' + packed);
    }, () => {
      console.log('crypto: failed to send to Pebble');
    });
  });
}
