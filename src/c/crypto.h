// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#pragma once
#include <pebble.h>

// Persist keys (free range; in use elsewhere: 100, 4, 200, 223, 310-312).
// 313 is the pre-existing BTC blob — CryptoInfo has the same layout as the old
// BtcInfo {int16_t; bool}, so a watch upgrading keeps its last BTC value.
#define CRYPTO_PERSIST_KEY_BTC    313
#define CRYPTO_PERSIST_KEY_XMR    314
#define CRYPTO_PERSIST_KEY_EURUSD 315

typedef enum {
  CRYPTO_BTC = 0,
  CRYPTO_XMR,
  CRYPTO_EURUSD,
  CRYPTO_COUNT
} CryptoCoin;

typedef struct {
  int16_t value;  // wire value: BTC = kUSD, XMR = USD, EUR/USD = milli (1155 = 1.155)
  bool    valid;  // false until the first value has ever been received
} CryptoInfo;

extern CryptoInfo Crypto_info[CRYPTO_COUNT];

void Crypto_init();                  // load last-known values from persist
void Crypto_saveData(CryptoCoin c);  // persist one coin
// No Crypto_deinit: Crypto_saveData() runs immediately whenever a value
// arrives (the only write path), so there is nothing to flush at shutdown.
