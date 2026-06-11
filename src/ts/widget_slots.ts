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
