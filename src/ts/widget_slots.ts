// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

// Maps the widget-list value (an ordered array of widget IDs, from the
// `widgetList` Clay custom component) onto the watchface's 6 fixed slots.
// Pads/truncates to exactly 6 ints; unparseable / missing entries become
// 0 (the Empty widget). Runs PKJS-side (not serialized into the webview).
export function listToSlots(list: any): number[] {
  const arr: any[] = Array.isArray(list) ? list : [];
  const out: number[] = [];
  for (let i = 0; i < 6; i++) {
    out.push(parseInt(arr[i], 10) || 0);
  }
  return out;
}

// The 6 legacy per-slot setting keys, in slot order. Mirrors the order used by
// listToSlots and the watch C side.
const WIDGET_SLOT_KEYS = ['SettingWidget0ID', 'SettingWidget1ID', 'SettingWidget2ID',
  'SettingWidget2_0ID', 'SettingWidget2_1ID', 'SettingWidget2_2ID'];

// Inverse of listToSlots, for the one-time migration from the 6 legacy per-slot
// settings (saved by the old 6-dropdown config) to the single `WidgetList` array.
// Given the flattened clay-settings object, returns the ordered id array with
// trailing Empty(0) slots trimmed, or null if NONE of the legacy slot keys are
// present (so the caller leaves WidgetList untouched and the component falls back
// to its defaultValue).
export function slotsToList(settings: Record<string, any>): number[] | null {
  let hasAny = false;
  const ids: number[] = [];
  for (let i = 0; i < WIDGET_SLOT_KEYS.length; i++) {
    const raw = settings[WIDGET_SLOT_KEYS[i]];
    if (raw !== undefined && raw !== null && raw !== '') { hasAny = true; }
    ids.push(parseInt(raw, 10) || 0);
  }
  if (!hasAny) { return null; }
  while (ids.length > 0 && ids[ids.length - 1] === 0) { ids.pop(); }
  return ids;
}
