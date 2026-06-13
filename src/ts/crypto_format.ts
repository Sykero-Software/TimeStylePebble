// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Per-coin display formatting. One signed integer `p` controls the rendered
   value: p >= 0 -> that many decimal places; p < 0 -> round to the nearest
   10^(-p) and divide it out (e.g. p=-3 shows a USD price in thousands). Pure,
   no Pebble/browser globals, unit-tested in crypto_format.test.js. */

export function formatPrice(price: number, p: number): string {
  if (typeof price !== 'number' || !isFinite(price)) {
    return '--';
  }
  if (p >= 0) {
    return price.toFixed(p);
  }
  const scale = Math.pow(10, -p);
  return String(Math.round(price / scale));
}
