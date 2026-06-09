// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "electricity.h"
#include <string.h>
#include <time.h>

ElectricityInfo Electricity_info;

void Electricity_init() {
  Electricity_info.startEpoch = 0;
  Electricity_info.count = 0;
  memset(Electricity_info.prices, 0, sizeof(Electricity_info.prices));

  if (persist_exists(ELEC_PERSIST_KEY_META)) {
    ElectricityMeta meta;
    persist_read_data(ELEC_PERSIST_KEY_META, &meta, sizeof(meta));
    Electricity_info.startEpoch = meta.startEpoch;
    uint16_t count = meta.count;
    if (count > ELEC_MAX_QUARTERS) { count = ELEC_MAX_QUARTERS; }
    Electricity_info.count = count;

    if (persist_exists(ELEC_PERSIST_KEY_DATA0)) {
      persist_read_data(ELEC_PERSIST_KEY_DATA0, Electricity_info.prices,
                        ELEC_CHUNK0_COUNT * sizeof(int16_t));
    }
    if (persist_exists(ELEC_PERSIST_KEY_DATA1)) {
      persist_read_data(ELEC_PERSIST_KEY_DATA1,
                        &Electricity_info.prices[ELEC_CHUNK0_COUNT],
                        (ELEC_MAX_QUARTERS - ELEC_CHUNK0_COUNT) * sizeof(int16_t));
    }
  }
}

void Electricity_saveData() {
  ElectricityMeta meta = { .startEpoch = Electricity_info.startEpoch,
                           .count = Electricity_info.count };
  persist_write_data(ELEC_PERSIST_KEY_META, &meta, sizeof(meta));
  persist_write_data(ELEC_PERSIST_KEY_DATA0, Electricity_info.prices,
                     ELEC_CHUNK0_COUNT * sizeof(int16_t));
  persist_write_data(ELEC_PERSIST_KEY_DATA1,
                     &Electricity_info.prices[ELEC_CHUNK0_COUNT],
                     (ELEC_MAX_QUARTERS - ELEC_CHUNK0_COUNT) * sizeof(int16_t));
}

void Electricity_deinit() {
  Electricity_saveData();
}

bool Electricity_getCurrentPrice(int16_t *out) {
  int idx;
  if (!elec_current_index(Electricity_info.startEpoch, Electricity_info.count,
                          (int64_t)time(NULL), &idx)) {
    return false;
  }
  *out = Electricity_info.prices[idx];
  return true;
}

bool Electricity_getTodayAverage(int16_t *out) {
  time_t now = time(NULL);

  struct tm start_tm = *localtime(&now);
  start_tm.tm_hour = 0;
  start_tm.tm_min = 0;
  start_tm.tm_sec = 0;
  time_t dayStart = mktime(&start_tm);

  struct tm end_tm = *localtime(&now);
  end_tm.tm_hour = 0;
  end_tm.tm_min = 0;
  end_tm.tm_sec = 0;
  end_tm.tm_mday += 1;            // mktime normalises month/day rollover
  time_t dayEnd = mktime(&end_tm);

  return elec_today_average(Electricity_info.prices, Electricity_info.count,
                            Electricity_info.startEpoch,
                            (int64_t)dayStart, (int64_t)dayEnd, out);
}
