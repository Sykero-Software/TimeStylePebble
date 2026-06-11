// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pure parser for porssisahko.net v2 latest-prices.json.
   No Pebble/browser globals -- unit-testable with `node --test`. */

export const ELEC_MAX_QUARTERS = 192;

interface PriceEntry {
  startDate: string;
  price: number;
}

interface LatestPricesResponse {
  prices?: PriceEntry[];
}

export interface ParsedPrices {
  startEpoch: number; // UTC seconds
  count: number;
  bytes: number[]; // little-endian int16 per quarter, unit 0.01 snt/kWh
}

// The watch indexes the table arithmetically as startEpoch + i*900s, so we must
// emit entries ascending by startDate. The v2 endpoint is documented ascending,
// but we sort defensively so the result is correct regardless of API order.
export function parseLatestPrices(json: LatestPricesResponse | null | undefined): ParsedPrices {
  const prices = json?.prices ? json.prices.slice() : [];
  prices.sort((a, b) => Date.parse(a.startDate) - Date.parse(b.startDate));

  const count = Math.min(prices.length, ELEC_MAX_QUARTERS);
  const startEpoch = count ? Math.floor(Date.parse(prices[0].startDate) / 1000) : 0;

  const bytes: number[] = [];
  for (let i = 0; i < count; i++) {
    let centi = Math.round(prices[i].price * 100);
    if (centi > 32767) { centi = 32767; }
    if (centi < -32768) { centi = -32768; }
    const u = centi & 0xFFFF;
    bytes.push(u & 0xFF);
    bytes.push((u >> 8) & 0xFF);
  }

  return { startEpoch, count, bytes };
}
