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

  // 24h window: an anchor older than 24h is excluded; rate uses in-window samples.
  // 60->58 over the last 24h (2%/day); remaining 58% / 2%/day = 29.0d = 290 tenths
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,       70, false);  // 3 days before "now" (outside window)
    BatteryDays_record(&b, 2*86400, 60, false);  // 24h before "now"  (in window)
    BatteryDays_record(&b, 3*86400, 58, false);  // "now"
    assert(BatteryDays_estimateTenths(&b, 3*86400) == 290); }

  // fallback to last-2 when the 24h window holds only the newest sample
  // 50->40 over 10 days (1%/day); remaining 40% / 1%/day = 40.0d = 400 tenths
  { BatteryDaysBuffer b = {0};
    BatteryDays_record(&b, 0,        50, false);
    BatteryDays_record(&b, 10*86400, 40, false);
    assert(BatteryDays_estimateTenths(&b, 10*86400) == 400); }

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

  printf("All battery_days tests passed\n");
  return 0;
}
