// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "btc.h"

BtcInfo Btc_info;

void Btc_init() {
  Btc_info.priceThousands = 0;
  Btc_info.valid = false;
  if (persist_exists(BTC_PERSIST_KEY)) {
    persist_read_data(BTC_PERSIST_KEY, &Btc_info, sizeof(Btc_info));
  }
}

void Btc_saveData() {
  persist_write_data(BTC_PERSIST_KEY, &Btc_info, sizeof(Btc_info));
}
