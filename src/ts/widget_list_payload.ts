// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

// Converts the `widgetList` Clay value (ordered array of widget ids) into the
// byte-array payload sent under SettingWidgetList: one id per byte, in order,
// dropping non-numeric / out-of-range entries, clamped to the watch buffer cap.
const MAX_WIDGET_LIST = 16;   // matches src/c/sidebar_widgets.h
const MAX_WIDGET_TYPE = 19;   // matches CHEAPEST_ELEC_HOUR
const CRYPTO_WID_BASE = 200;  // matches CRYPTO_WID_BASE in src/c/crypto.h
const MAX_CRYPTO = 16;        // matches MAX_CRYPTO in src/c/crypto.h

function isValidWidgetId(id: number): boolean {
  if (id >= 0 && id <= MAX_WIDGET_TYPE) { return true; }
  if (id >= CRYPTO_WID_BASE && id < CRYPTO_WID_BASE + MAX_CRYPTO) { return true; }
  return false;
}

export function widgetListToPayload(list: any): number[] {
  const arr: any[] = Array.isArray(list) ? list : [];
  const out: number[] = [];
  for (let i = 0; i < arr.length && out.length < MAX_WIDGET_LIST; i++) {
    const id = parseInt(arr[i], 10);
    if (!isNaN(id) && isValidWidgetId(id)) { out.push(id); }
  }
  return out;
}
