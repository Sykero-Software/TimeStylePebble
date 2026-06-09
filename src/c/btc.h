// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#pragma once
#include <pebble.h>

// Persist key (free range; in use elsewhere: 100, 4, 200, 223, 310-312).
#define BTC_PERSIST_KEY 313

typedef struct {
  int16_t priceThousands;  // USD price rounded to nearest 1000 (e.g. 63)
  bool    valid;           // false until the first price has ever been received
} BtcInfo;

extern BtcInfo Btc_info;

void Btc_init();       // load last-known value from persist (valid=false if none)
void Btc_saveData();   // persist current Btc_info
// No Btc_deinit: Btc_saveData() runs immediately whenever a price arrives (the
// only write path), so there is nothing to flush at shutdown.
