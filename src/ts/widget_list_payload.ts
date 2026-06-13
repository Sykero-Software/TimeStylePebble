// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

// Converts the `widgetList` Clay value (ordered array of widget ids) into the
// byte-array payload sent under SettingWidgetList: one id per byte, in order,
// dropping non-numeric / out-of-range entries, clamped to the watch buffer cap.
const MAX_WIDGET_LIST = 16;   // matches src/c/sidebar_widgets.h
const MAX_WIDGET_TYPE = 19;   // matches CHEAPEST_ELEC_HOUR

export function widgetListToPayload(list: any): number[] {
  const arr: any[] = Array.isArray(list) ? list : [];
  const out: number[] = [];
  for (let i = 0; i < arr.length && out.length < MAX_WIDGET_LIST; i++) {
    const id = parseInt(arr[i], 10);
    if (!isNaN(id) && id >= 0 && id <= MAX_WIDGET_TYPE) { out.push(id); }
  }
  return out;
}
