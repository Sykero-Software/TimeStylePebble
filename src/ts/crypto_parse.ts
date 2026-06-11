// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pure coin table + CoinGecko-price -> wire-int parsing. No Pebble/browser
   globals, so it is unit-testable with `node --test`. Each widget is a
   CoinGecko coin priced in USD; toWire converts the USD float to the int16
   sent to the watch (= exactly the displayed precision, so the send-on-change
   check in crypto.js never wakes Bluetooth for an invisible change). */

export interface Coin {
  geckoId: string;
  widgetId: number;
  messageKey: string;
  disableKey: string;
  lastKey: string;
  toWire(usd: number): number;
}

export const COINS: Coin[] = [
  {
    geckoId: 'bitcoin', widgetId: 15, messageKey: 'BtcPriceThousands',
    disableKey: 'disable_btc', lastKey: 'btc_last_thousands',
    toWire: (usd) => Math.round(usd / 1000),
  },
  {
    geckoId: 'monero', widgetId: 16, messageKey: 'XmrPriceDollars',
    disableKey: 'disable_xmr', lastKey: 'xmr_last_dollars',
    toWire: (usd) => Math.round(usd),
  },
  {
    geckoId: 'euro-coin', widgetId: 17, messageKey: 'EurUsdMilli',
    disableKey: 'disable_eurusd', lastKey: 'eurusd_last_milli',
    toWire: (usd) => Math.round(usd * 1000),
  },
];

// CoinGecko simple/price response: { <geckoId>: { usd: <number> } }.
type GeckoPriceResponse = Record<string, { usd?: number } | undefined>;

// Returns the coin's wire int, or null if no usable number is present.
export function parseWire(json: GeckoPriceResponse | null | undefined, coin: Coin): number | null {
  const usd = json?.[coin.geckoId]?.usd;
  if (typeof usd !== 'number' || !isFinite(usd)) {
    return null;
  }
  return coin.toWire(usd);
}
