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

uint32_t BatteryDays_bufferRateSecPerPct(const BatteryDaysBuffer *buf) {
  if (buf->count < 2) {
    return 0;
  }
  const BatteryDaysSample *newest = &buf->samples[buf->count - 1];
  const BatteryDaysSample *oldest = &buf->samples[0];
  if (newest->pct >= oldest->pct) {   // no net drop
    return 0;
  }
  if (newest->t <= oldest->t) {       // clock moved backwards / no elapsed time
    return 0;
  }
  int drop = (int)oldest->pct - (int)newest->pct;
  uint32_t dt = newest->t - oldest->t;
  if (drop < BATTERY_DAYS_MIN_DROP || dt < (uint32_t)BATTERY_DAYS_MIN_SPAN_SEC) {
    return 0;
  }
  // seconds per 1% drop = dt / drop (round to nearest)
  return (uint32_t)(((uint64_t)dt + (uint32_t)drop / 2) / (uint32_t)drop);
}

uint32_t BatteryDays_blendLearned(uint32_t learned, uint32_t fresh) {
  if (fresh == 0) {
    return learned;     // nothing valid to fold in
  }
  if (learned == 0) {
    return fresh;       // first real measurement replaces a seeded default
  }
  int64_t delta = (int64_t)fresh - (int64_t)learned;  // signed: fresh may be < learned
  return (uint32_t)((int64_t)learned + delta / 4);
}

int BatteryDays_tenthsFromRate(uint8_t pct, uint32_t sec_per_pct) {
  if (sec_per_pct == 0) {
    return BATTERY_DAYS_NONE;
  }
  // days_left = pct * sec_per_pct / 86400 ; tenths = *10, round to nearest.
  // 64-bit: pct*sec_per_pct*10 overflows 32-bit for slow discharge.
  int64_t tenths = ((int64_t)pct * (int64_t)sec_per_pct * 10 + 86400 / 2) / 86400;
  if (tenths < 0) { tenths = 0; }
  if (tenths > BATTERY_DAYS_MAX_TENTHS) { tenths = BATTERY_DAYS_MAX_TENTHS; }
  return (int)tenths;
}

uint32_t BatteryDays_capRate(uint32_t sec_per_pct) {
  // Bigger sec/% = slower discharge = longer life. The default is the ceiling.
  return (sec_per_pct > BATTERY_DAYS_DEFAULT_SEC_PER_PCT)
             ? BATTERY_DAYS_DEFAULT_SEC_PER_PCT
             : sec_per_pct;
}

int BatteryDays_estimateTenths(const BatteryDaysBuffer *buf, uint32_t now) {
  (void)now;   // rate is measured over the retained history; independent of "now"
  uint32_t rate = BatteryDays_bufferRateSecPerPct(buf);
  if (rate == 0) {
    return BATTERY_DAYS_NONE;   // not enough data yet (guards the [count-1] index below)
  }
  return BatteryDays_tenthsFromRate(buf->samples[buf->count - 1].pct, rate);
}
