// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "crypto.h"

CryptoInfo Crypto_info[CRYPTO_COUNT];

static const uint32_t persistKeys[CRYPTO_COUNT] = {
  CRYPTO_PERSIST_KEY_BTC,
  CRYPTO_PERSIST_KEY_XMR,
  CRYPTO_PERSIST_KEY_EURUSD,
};

void Crypto_init() {
  for (int i = 0; i < CRYPTO_COUNT; i++) {
    Crypto_info[i].value = 0;
    Crypto_info[i].valid = false;
    if (persist_exists(persistKeys[i])) {
      persist_read_data(persistKeys[i], &Crypto_info[i], sizeof(CryptoInfo));
    }
  }
}

void Crypto_saveData(CryptoCoin c) {
  persist_write_data(persistKeys[c], &Crypto_info[c], sizeof(CryptoInfo));
}
