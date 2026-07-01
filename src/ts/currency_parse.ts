// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pure helpers for the configurable currency-pair list: validating config rows,
   building the ExchangeRate-API request per base, and packing fetched rates into
   the wire string sent under the CurrencyData message key. No Pebble/browser
   globals, so unit-testable with `node --test`. The pair list comes from config
   at runtime (read in currency.ts); this module never hardcodes pairs. */

import { formatPrice } from './crypto_format';

export interface CurrencyRow {
  wid: number;      // stable widget id in [216, 223)
  base: string;     // 3-letter ISO, uppercase (request base)
  quote: string;    // 3-letter ISO, uppercase (looked up in rates)
  p: number;        // decimals (0..6)
  label: string;    // sidebar label; '' -> auto "BASE/QUOTE"
}

export const DELIM = '\x1f';                 // unit separator between wire fields
const BASE_URL = 'https://open.er-api.com/v6/latest/';

function up(s: any): string {
  return (typeof s === 'string') ? s.trim().toUpperCase() : '';
}

export function normalizeRows(raw: any): CurrencyRow[] {
  const arr: any[] = Array.isArray(raw) ? raw : [];
  const out: CurrencyRow[] = [];
  for (let i = 0; i < arr.length; i++) {
    const r = arr[i];
    if (!r || typeof r !== 'object') { continue; }
    const wid = parseInt(r.wid, 10);
    const base = up(r.base);
    const quote = up(r.quote);
    if (isNaN(wid) || base === '' || quote === '') { continue; }
    let p = parseInt(r.p, 10);
    if (isNaN(p)) { p = 4; }
    if (p < 0) { p = 0; }
    if (p > 6) { p = 6; }
    const label = (typeof r.label === 'string') ? r.label : '';
    out.push({ wid: wid, base: base, quote: quote, p: p, label: label });
  }
  return out;
}

export function distinctBases(rows: CurrencyRow[]): string[] {
  const out: string[] = [];
  for (let i = 0; i < rows.length; i++) {
    if (out.indexOf(rows[i].base) === -1) { out.push(rows[i].base); }
  }
  return out;
}

export function buildRateUrl(base: string): string {
  return BASE_URL + base;
}

function labelFor(r: CurrencyRow): string {
  return r.label !== '' ? r.label : (r.base + '/' + r.quote);
}

// ratesByBase: { BASE: { QUOTE: number, ... } } accumulated from the per-base
// responses. prevValues: { wid: value } from the last send, used as a fallback
// for a pair whose base hasn't answered (or failed) this round.
export function packCurrencyData(rows: CurrencyRow[], ratesByBase: any, prevValues: any): string {
  const fields: string[] = [];
  const prev = prevValues || {};
  for (let i = 0; i < rows.length; i++) {
    const r = rows[i];
    const rates = ratesByBase ? ratesByBase[r.base] : undefined;
    const rate = rates ? rates[r.quote] : undefined;
    let value: string;
    if (typeof rate === 'number' && isFinite(rate)) {
      value = formatPrice(rate, r.p, 0);
    } else if (prev[r.wid] !== undefined) {
      value = prev[r.wid];
    } else {
      value = '--';
    }
    fields.push(String(r.wid), labelFor(r), value);
  }
  return fields.join(DELIM);
}

export function countValidRates(rows: CurrencyRow[], ratesByBase: any): number {
  let n = 0;
  for (let i = 0; i < rows.length; i++) {
    const r = rows[i];
    const rates = ratesByBase ? ratesByBase[r.base] : undefined;
    const rate = rates ? rates[r.quote] : undefined;
    if (typeof rate === 'number' && isFinite(rate)) { n++; }
  }
  return n;
}

export function parseLastSent(packed: string): Record<string, string> {
  const out: Record<string, string> = {};
  if (!packed) { return out; }
  const f = packed.split(DELIM);
  for (let i = 0; i + 2 <= f.length - 1; i += 3) {
    out[f[i]] = f[i + 2];
  }
  return out;
}
