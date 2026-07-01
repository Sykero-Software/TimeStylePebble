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
