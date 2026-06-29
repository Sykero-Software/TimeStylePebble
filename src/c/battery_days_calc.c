// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#include "battery_days_calc.h"

void BatteryDays_record(BatteryDaysBuffer *buf, uint32_t now, uint8_t pct, bool is_charging) {
  if (is_charging) {            // charging: discharge history is meaningless
    buf->count = 0;
    return;
  }
  if (buf->count > 0) {
    uint8_t last = buf->samples[buf->count - 1].pct;
    if (pct > last) {           // battery rose (e.g. just unplugged) -> restart, anchor below
      buf->count = 0;
    } else if (pct == last) {   // unchanged -> nothing to record
      return;
    }
  }
  if (buf->count >= BATTERY_SAMPLE_CAP) {     // full: drop the oldest
    for (int i = 1; i < BATTERY_SAMPLE_CAP; i++) {
      buf->samples[i - 1] = buf->samples[i];
    }
    buf->count = BATTERY_SAMPLE_CAP - 1;
  }
  buf->samples[buf->count].t = now;
  buf->samples[buf->count].pct = pct;
  buf->count++;
}

int BatteryDays_estimateTenths(const BatteryDaysBuffer *buf, uint32_t now) {
  if (buf->count < 2) {
    return BATTERY_DAYS_NONE;
  }
  const BatteryDaysSample *newest = &buf->samples[buf->count - 1];

  // window start = oldest sample within WINDOW_SEC of now
  int start = buf->count - 1;
  for (int i = 0; i < buf->count; i++) {
    if (now - buf->samples[i].t <= (uint32_t)BATTERY_DAYS_WINDOW_SEC) {
      start = i;
      break;
    }
  }
  if (start >= buf->count - 1) {      // window left only the newest -> use last 2
    start = buf->count - 2;
  }
  const BatteryDaysSample *oldest = &buf->samples[start];

  if (newest->pct >= oldest->pct) {   // no net drop
    return BATTERY_DAYS_NONE;
  }
  int drop = (int)oldest->pct - (int)newest->pct;
  uint32_t dt = newest->t - oldest->t;
  if (drop < BATTERY_DAYS_MIN_DROP || dt < (uint32_t)BATTERY_DAYS_MIN_SPAN_SEC) {
    return BATTERY_DAYS_NONE;
  }

  // days_left_from_now = remaining% / (drop% / dt_sec) / 86400  -> tenths = *10
  //   = newest.pct * dt * 10 / (drop * 86400)
  // 64-bit intermediates: newest.pct*dt*10 overflows 32-bit long for multi-week spans.
  int64_t denom = (int64_t)drop * 86400;
  int64_t tenths = ((int64_t)newest->pct * (int64_t)dt * 10 + denom / 2) / denom;
  if (tenths < 0) { tenths = 0; }
  if (tenths > BATTERY_DAYS_MAX_TENTHS) { tenths = BATTERY_DAYS_MAX_TENTHS; }
  return (int)tenths;
}
