// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#pragma once

#include <pebble.h>
#include "battery_days_calc.h"

// Pebble glue around the pure battery_days_calc estimator: owns the singleton
// sample buffer, its persistence, and the battery-event hook.

#define BATTERY_DAYS_PERSIST_KEY          314
#define BATTERY_DAYS_VERSION_PERSIST_KEY  315
#define BATTERY_DAYS_VERSION              1

void BatteryDays_init(void);                              // load persisted history + seed current reading
void BatteryDays_save(void);                              // persist history
void BatteryDays_onBattery(BatteryChargeState charge_state); // record a reading + save
int  BatteryDays_currentEstimateTenths(void);            // estimate now (tenths) or BATTERY_DAYS_NONE
