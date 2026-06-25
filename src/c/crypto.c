// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include <pebble.h>
#include <string.h>
#include <stdlib.h>
#include "crypto.h"

#define CRYPTO_DELIM '\x1f'      // unit separator between wire fields

CryptoSlot Crypto_slots[MAX_CRYPTO];
uint8_t    Crypto_count = 0;

bool Crypto_isWid(uint8_t wid) {
  return wid == 15 || wid == 16 || wid == 17 ||
         (wid >= CRYPTO_WID_BASE && wid < CRYPTO_WID_BASE + MAX_CRYPTO);
}

CryptoSlot *Crypto_find(uint8_t wid) {
  for (int i = 0; i < Crypto_count; i++) {
    if (Crypto_slots[i].wid == wid) { return &Crypto_slots[i]; }
  }
  return NULL;
}

// Copy up to dstSize-1 chars of [start, end) into dst, NUL-terminated.
static void copy_field(char *dst, size_t dstSize, const char *start, const char *end) {
  size_t n = (size_t)(end - start);
  if (n > dstSize - 1) { n = dstSize - 1; }
  memcpy(dst, start, n);
  dst[n] = '\0';
}

// Parse "wid<US>label<US>value<US>wid<US>label<US>value..." into the slot table.
void Crypto_parse(const char *data) {
  Crypto_count = 0;
  if (!data) { return; }
  const char *p = data;
  while (*p && Crypto_count < MAX_CRYPTO) {
    const char *e = strchr(p, CRYPTO_DELIM);
    if (!e) { break; }
    char widBuf[6];
    copy_field(widBuf, sizeof(widBuf), p, e);
    p = e + 1;
    e = strchr(p, CRYPTO_DELIM);
    if (!e) { break; }
    CryptoSlot *s = &Crypto_slots[Crypto_count];
    s->wid = (uint8_t)atoi(widBuf);
    copy_field(s->label, sizeof(s->label), p, e);
    p = e + 1;
    e = strchr(p, CRYPTO_DELIM);
    const char *valEnd = e ? e : (p + strlen(p));
    copy_field(s->value, sizeof(s->value), p, valEnd);
    s->valid = (strcmp(s->value, "--") != 0);
    Crypto_count++;
    if (!e) { break; }
    p = e + 1;
  }
}

void Crypto_init() {
  Crypto_count = 0;
  if (persist_exists(CRYPTO_PERSIST_KEY_DATA)) {
    char buf[256];
    int n = persist_read_string(CRYPTO_PERSIST_KEY_DATA, buf, sizeof(buf));
    if (n > 0) { Crypto_parse(buf); }
  }
#ifdef SCREENSHOT_FIXTURES
  // Demo coins for appstore screenshots (no phone): BTC, ETH, EUR/USD.
  Crypto_count = 3;
  Crypto_slots[0] = (CryptoSlot){ .wid = 15,  .valid = true };
  strcpy(Crypto_slots[0].label, "BTC"); strcpy(Crypto_slots[0].value, "62");
  Crypto_slots[1] = (CryptoSlot){ .wid = 201, .valid = true };
  strcpy(Crypto_slots[1].label, "ETH"); strcpy(Crypto_slots[1].value, "17");
  Crypto_slots[2] = (CryptoSlot){ .wid = 17,  .valid = true };
  strcpy(Crypto_slots[2].label, "EUR"); strcpy(Crypto_slots[2].value, "136");
#endif
}
