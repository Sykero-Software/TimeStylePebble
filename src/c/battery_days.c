// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#include "battery_days.h"

static BatteryDaysBuffer s_buf;
static uint32_t s_learned;   // learned discharge rate (sec per 1% drop); 0 = none yet

void BatteryDays_init(void) {
  s_buf.count = 0;
  if (persist_exists(BATTERY_DAYS_PERSIST_KEY) &&
      persist_exists(BATTERY_DAYS_VERSION_PERSIST_KEY) &&
      persist_read_int(BATTERY_DAYS_VERSION_PERSIST_KEY) == BATTERY_DAYS_VERSION &&
      persist_get_size(BATTERY_DAYS_PERSIST_KEY) == (int)sizeof(s_buf)) {
    persist_read_data(BATTERY_DAYS_PERSIST_KEY, &s_buf, sizeof(s_buf));
    if (s_buf.count > BATTERY_SAMPLE_CAP) { s_buf.count = 0; }  // guard a corrupt blob
  }
  s_learned = persist_exists(BATTERY_DAYS_RATE_PERSIST_KEY)
                  ? (uint32_t)persist_read_int(BATTERY_DAYS_RATE_PERSIST_KEY)
                  : 0;
  // Seed the current reading so the estimate's clock starts at launch, not only at
  // the first battery-change event (which, at ~1% steps, can be hours away -> long
  // "--"). record() clears on charging, appends on a decrease and is a no-op on an
  // unchanged percent, so this is safe on every launch and keeps history continuous.
  BatteryChargeState st = battery_state_service_peek();
  BatteryDays_record(&s_buf, (uint32_t)time(NULL), st.charge_percent, st.is_charging);
  // Fold any currently-valid history into the learned rate. Usually the freshly
  // re-seeded buffer has count<2 -> bufferRate 0 -> no change; when a valid span
  // persisted it only nudges s_learned toward the correct whole-history rate (e.g.
  // first launch after this upgrade, when key 316 was absent).
  s_learned = BatteryDays_capRate(
      BatteryDays_blendLearned(s_learned, BatteryDays_bufferRateSecPerPct(&s_buf)));
}

void BatteryDays_save(void) {
  persist_write_int(BATTERY_DAYS_VERSION_PERSIST_KEY, BATTERY_DAYS_VERSION);
  persist_write_data(BATTERY_DAYS_PERSIST_KEY, &s_buf, sizeof(s_buf));
  persist_write_int(BATTERY_DAYS_RATE_PERSIST_KEY, (int)s_learned);
}

void BatteryDays_onBattery(BatteryChargeState charge_state) {
  BatteryDays_record(&s_buf, (uint32_t)time(NULL),
                     charge_state.charge_percent, charge_state.is_charging);
  // Update the learned rate from the (possibly now-valid) whole-history rate.
  // Charging just cleared s_buf, so bufferRate is 0 there -> learned is preserved.
  s_learned = BatteryDays_capRate(
      BatteryDays_blendLearned(s_learned, BatteryDays_bufferRateSecPerPct(&s_buf)));
  BatteryDays_save();
}

int BatteryDays_currentEstimateTenths(void) {
  BatteryChargeState st = battery_state_service_peek();
  // s_learned is already capped on update; cap again defensively so a pre-fix
  // persisted rate (the >30d "99.9" bug) is bounded even on the very first draw.
  uint32_t rate = (s_learned > 0) ? BatteryDays_capRate(s_learned)
                                  : BATTERY_DAYS_DEFAULT_SEC_PER_PCT;
  return BatteryDays_tenthsFromRate(st.charge_percent, rate);
}
