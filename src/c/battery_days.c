// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#include "battery_days.h"

static BatteryDaysBuffer s_buf;

void BatteryDays_init(void) {
  s_buf.count = 0;
  if (persist_exists(BATTERY_DAYS_PERSIST_KEY) &&
      persist_exists(BATTERY_DAYS_VERSION_PERSIST_KEY) &&
      persist_read_int(BATTERY_DAYS_VERSION_PERSIST_KEY) == BATTERY_DAYS_VERSION &&
      persist_get_size(BATTERY_DAYS_PERSIST_KEY) == (int)sizeof(s_buf)) {
    persist_read_data(BATTERY_DAYS_PERSIST_KEY, &s_buf, sizeof(s_buf));
    if (s_buf.count > BATTERY_SAMPLE_CAP) { s_buf.count = 0; }  // guard a corrupt blob
  }
  // Seed the current reading so the estimate's clock starts at launch, not only at
  // the first battery-change event (which, at ~1% steps, can be hours away -> long
  // "--"). record() clears on charging, appends on a decrease and is a no-op on an
  // unchanged percent, so this is safe on every launch and keeps history continuous.
  BatteryChargeState st = battery_state_service_peek();
  BatteryDays_record(&s_buf, (uint32_t)time(NULL), st.charge_percent, st.is_charging);
}

void BatteryDays_save(void) {
  persist_write_int(BATTERY_DAYS_VERSION_PERSIST_KEY, BATTERY_DAYS_VERSION);
  persist_write_data(BATTERY_DAYS_PERSIST_KEY, &s_buf, sizeof(s_buf));
}

void BatteryDays_onBattery(BatteryChargeState charge_state) {
  BatteryDays_record(&s_buf, (uint32_t)time(NULL),
                     charge_state.charge_percent, charge_state.is_charging);
  BatteryDays_save();
}

int BatteryDays_currentEstimateTenths(void) {
  return BatteryDays_estimateTenths(&s_buf, (uint32_t)time(NULL));
}
