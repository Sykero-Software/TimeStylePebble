// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#include "battery_days_calc.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  // empty / single sample -> NONE
  { BatteryDaysBuffer b = {0};
    assert(BatteryDays_estimateTenths(&b, 1000) == BATTERY_DAYS_NONE);
    BatteryDays_record(&b, 1000, 90, false);
    assert(b.count == 1);
    assert(BatteryDays_estimateTenths(&b, 1000) == BATTERY_DAYS_NONE); }

  // charging clears the buffer
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,    90, false);
    BatteryDays_record(&b, 3600, 89, false);
    assert(b.count == 2);
    BatteryDays_record(&b, 7200, 89, true);   // charging
    assert(b.count == 0); }

  // a percent rise (e.g. just unplugged) restarts the history
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,    50, false);
    BatteryDays_record(&b, 3600, 49, false);
    BatteryDays_record(&b, 7200, 70, false);  // rose -> clear then anchor at 70
    assert(b.count == 1 && b.samples[0].pct == 70); }

  // no-change reading is ignored
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,    50, false);
    BatteryDays_record(&b, 3600, 50, false);  // unchanged
    assert(b.count == 1); }

  // known rate: 50% -> 40% over exactly 1 day; remaining 40% / 10%/day = 4.0d = 40 tenths
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,     50, false);
    BatteryDays_record(&b, 86400, 40, false);
    assert(BatteryDays_estimateTenths(&b, 86400) == 40); }

  // warm-up gate: drop < 2% -> NONE
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,    80, false);
    BatteryDays_record(&b, 7200, 79, false);  // only 1% drop
    assert(BatteryDays_estimateTenths(&b, 7200) == BATTERY_DAYS_NONE); }

  // warm-up gate: span < 1h -> NONE
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,   80, false);
    BatteryDays_record(&b, 600, 78, false);   // 2% but only 10 min
    assert(BatteryDays_estimateTenths(&b, 600) == BATTERY_DAYS_NONE); }

  // whole-buffer average: the OLDEST retained sample anchors the rate, even when
  // it is older than 24h. Averaging over the full retained history (several days)
  // is what smooths out day/night usage swings.
  // 70->58 over 3 days (4%/day); remaining 58% / 4%/day = 14.5d = 145 tenths.
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,       70, false);  // 3 days before "now"
    BatteryDays_record(&b, 2*86400, 60, false);
    BatteryDays_record(&b, 3*86400, 58, false);  // "now"
    assert(BatteryDays_estimateTenths(&b, 3*86400) == 145); }

  // two samples: rate is just oldest->newest.
  // 50->40 over 10 days (1%/day); remaining 40% / 1%/day = 40.0d = 400 tenths
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,        50, false);
    BatteryDays_record(&b, 10*86400, 40, false);
    assert(BatteryDays_estimateTenths(&b, 10*86400) == 400); }

  // diurnal robustness: a fast active day (100->96 in 12h) followed by slow
  // idle/overnight steps must NOT inflate the estimate. The old 24h-window logic
  // dropped the fast first-day segment and reported ~47d (470 tenths); the
  // whole-buffer average keeps it -> 6% over 36h = 4%/day, remaining 94% ->
  // 23.5d = 235 tenths. (This is the "jumps up after the night" bug.)
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,       100, false);  // active day start
    BatteryDays_record(&b, 12*3600,  96, false);  // fast: 4% in 12h
    BatteryDays_record(&b, 24*3600,  95, false);  // slow: 1% overnight
    BatteryDays_record(&b, 36*3600,  94, false);  // slow: 1% overnight
    assert(BatteryDays_estimateTenths(&b, 36*3600) == 235); }

  // clamp: extremely slow discharge capped at 99.9 days
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,        100, false);
    BatteryDays_record(&b, 30*86400,  98, false);  // 2% over 30 days
    assert(BatteryDays_estimateTenths(&b, 30*86400) == BATTERY_DAYS_MAX_TENTHS); }

  // eviction: more than CAP samples keeps exactly CAP, newest last
  { BatteryDaysBuffer b = {0};
    for (int i = 0; i < BATTERY_SAMPLE_CAP + 5; i++) {
      BatteryDays_record(&b, (uint32_t)(i * 3600), (uint8_t)(100 - i), false);
    }
    assert(b.count == BATTERY_SAMPLE_CAP);
    assert(b.samples[b.count - 1].pct == (uint8_t)(100 - (BATTERY_SAMPLE_CAP + 4))); }

  // --- rate helpers (issue: estimate available right after charging) ---

  // tenthsFromRate: math, defaults, clamp, zero-rate sentinel
  assert(BatteryDays_tenthsFromRate(0,  8640) == 0);                       // empty battery
  assert(BatteryDays_tenthsFromRate(40, 8640) == 40);                      // 40% at 8640 s/% = 4.0 d
  assert(BatteryDays_tenthsFromRate(100, BATTERY_DAYS_DEFAULT_SEC_PER_PCT) == 300); // fresh: 100% -> 30.0 d
  assert(BatteryDays_tenthsFromRate(50,  BATTERY_DAYS_DEFAULT_SEC_PER_PCT) == 150); // 50% -> 15.0 d
  assert(BatteryDays_tenthsFromRate(100, 1u << 30) == BATTERY_DAYS_MAX_TENTHS);     // absurdly slow -> clamp
  assert(BatteryDays_tenthsFromRate(90, 0) == BATTERY_DAYS_NONE);          // no rate -> sentinel

  // bufferRateSecPerPct: valid vs below-threshold
  { BatteryDaysBuffer b = {0};
    assert(BatteryDays_bufferRateSecPerPct(&b) == 0);                      // empty
    BatteryDays_record(&b, 0, 50, false);
    assert(BatteryDays_bufferRateSecPerPct(&b) == 0);                      // single sample
    BatteryDays_record(&b, 86400, 40, false);                             // 10% over 1 day
    assert(BatteryDays_bufferRateSecPerPct(&b) == 8640); }                 // 86400/10
  { BatteryDaysBuffer b = {0};                                             // 1% drop < MIN_DROP
    BatteryDays_record(&b, 0,    80, false);
    BatteryDays_record(&b, 7200, 79, false);
    assert(BatteryDays_bufferRateSecPerPct(&b) == 0); }

  // blendLearned: seed-replace, EWMA up/down, no underflow, keep-on-zero
  assert(BatteryDays_blendLearned(0,    8640)  == 8640);                   // first real replaces default
  assert(BatteryDays_blendLearned(8640, 0)     == 8640);                   // nothing valid -> keep
  assert(BatteryDays_blendLearned(8000, 12000) == 9000);                   // 8000 + (12000-8000)/4
  assert(BatteryDays_blendLearned(12000, 8000) == 11000);                  // 12000 + (8000-12000)/4, no underflow

  printf("All battery_days tests passed\n");
  return 0;
}
