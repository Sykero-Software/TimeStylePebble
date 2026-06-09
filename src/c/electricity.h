// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#pragma once
#include <pebble.h>
#include "electricity_calc.h"

// Persist keys (free range; in use elsewhere: 100, 4, 200, 223, 300-303)
#define ELEC_PERSIST_KEY_META  310
#define ELEC_PERSIST_KEY_DATA0 311
#define ELEC_PERSIST_KEY_DATA1 312
// First chunk holds 128 quarters (256 bytes = persist max); rest go to DATA1.
#define ELEC_CHUNK0_COUNT 128

typedef struct {
  uint32_t startEpoch;
  uint16_t count;
} ElectricityMeta;

typedef struct {
  uint32_t startEpoch;                 // UTC epoch (s) of prices[0]
  uint16_t count;                      // valid entries (<= ELEC_MAX_QUARTERS)
  int16_t  prices[ELEC_MAX_QUARTERS];  // 0.01 snt/kWh
} ElectricityInfo;

extern ElectricityInfo Electricity_info;

void Electricity_init();
void Electricity_deinit();
void Electricity_saveData();
// out in 0.01 snt; false if now is outside the table / no data.
bool Electricity_getCurrentPrice(int16_t *out);
// out in 0.01 snt; false if the current local day has no entries.
bool Electricity_getTodayAverage(int16_t *out);
