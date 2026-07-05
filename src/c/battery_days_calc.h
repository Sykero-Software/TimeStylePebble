// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Estimate "days of battery left" from charge_percent samples over time.
// Pure logic (no Pebble API) so it is host-gcc unit-testable; the Pebble glue
// (persist + time + battery service) lives in battery_days.c.

#define BATTERY_SAMPLE_CAP        16     // ring-buffer capacity (~several days at 15-30d life)
#define BATTERY_DAYS_MIN_DROP     2      // require >=2% drop before showing an estimate
#define BATTERY_DAYS_MIN_SPAN_SEC 3600   // ...spanning >=1 h (suppresses noisy early estimates)
#define BATTERY_DAYS_MAX_TENTHS   999    // clamp display to 99.9 days
#define BATTERY_DAYS_NONE         (-1)   // sentinel: not enough data yet (warm-up / charging)

// Default assumed battery life for a fresh install (no learned rate yet):
// 30-day life = 30*86400/100 seconds per 1% drop. The manufacturer's promised
// future life; picked optimistically. Washes out at the first real measurement.
#define BATTERY_DAYS_DEFAULT_SEC_PER_PCT 25920u

typedef struct {
  uint32_t t;     // unix timestamp of the sample
  uint8_t  pct;   // charge_percent at that time
} BatteryDaysSample;

typedef struct {
  BatteryDaysSample samples[BATTERY_SAMPLE_CAP];  // oldest .. newest
  uint8_t count;                                  // number of valid samples
} BatteryDaysBuffer;

// Record a battery reading. Clears the buffer on charging or a percent rise
// (discharge history becomes stale); pushes (now,pct) on a decrease; no-op on
// no change. Evicts the oldest sample when full.
void BatteryDays_record(BatteryDaysBuffer *buf, uint32_t now, uint8_t pct, bool is_charging);

// Estimate remaining battery life in TENTHS of a day, or BATTERY_DAYS_NONE.
// The rate is averaged over the whole retained history (oldest..newest sample) so
// day/night usage swings don't jolt the number; `now` is currently unused.
int  BatteryDays_estimateTenths(const BatteryDaysBuffer *buf, uint32_t now);

// Whole-history discharge rate in SECONDS per 1% drop (dt/drop over oldest..newest),
// or 0 if the buffer is not yet valid (same count>=2 / drop>=MIN_DROP / span>=MIN_SPAN
// / net-drop / elapsed-time guards as the estimate).
uint32_t BatteryDays_bufferRateSecPerPct(const BatteryDaysBuffer *buf);

// Fold a fresh valid rate (sec/%) into the persisted learned rate.
//   fresh   == 0 -> keep learned (nothing valid to fold)
//   learned == 0 -> take fresh as-is (a seeded default is replaced by the first
//                   real measurement)
//   otherwise    -> light EWMA toward fresh: learned + (fresh - learned)/4
// Signed-safe: fresh may be smaller than learned.
uint32_t BatteryDays_blendLearned(uint32_t learned, uint32_t fresh);

// Remaining life in TENTHS of a day from a charge level + rate, clamped to
// [0, BATTERY_DAYS_MAX_TENTHS]. sec_per_pct == 0 -> BATTERY_DAYS_NONE.
int BatteryDays_tenthsFromRate(uint8_t pct, uint32_t sec_per_pct);

// Cap a discharge rate to the default ceiling: the 30-day default is the MAXIMUM
// plausible life (the manufacturer's promised max), so a slower measured rate
// (bigger sec/%, implying >30d) is implausible and falls back to the default.
// A faster rate (fewer days) is kept; 0 (none) passes through unchanged.
// Prevents a tiny-drop/long-span measurement from projecting an absurd life.
uint32_t BatteryDays_capRate(uint32_t sec_per_pct);
