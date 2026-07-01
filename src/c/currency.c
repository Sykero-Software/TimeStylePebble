// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include <pebble.h>
#include <string.h>
#include <stdlib.h>
#include "currency.h"

#define CURRENCY_DELIM '\x1f'      // unit separator between wire fields

CryptoSlot Currency_slots[MAX_CURRENCY];
uint8_t    Currency_count = 0;

bool Currency_isWid(uint8_t wid) {
  return wid >= CURRENCY_WID_BASE && wid < CURRENCY_WID_BASE + MAX_CURRENCY;
}

CryptoSlot *Currency_find(uint8_t wid) {
  for (int i = 0; i < Currency_count; i++) {
    if (Currency_slots[i].wid == wid) { return &Currency_slots[i]; }
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
void Currency_parse(const char *data) {
  Currency_count = 0;
  if (!data) { return; }
  const char *p = data;
  while (*p && Currency_count < MAX_CURRENCY) {
    const char *e = strchr(p, CURRENCY_DELIM);
    if (!e) { break; }
    char widBuf[6];
    copy_field(widBuf, sizeof(widBuf), p, e);
    p = e + 1;
    e = strchr(p, CURRENCY_DELIM);
    if (!e) { break; }
    CryptoSlot *s = &Currency_slots[Currency_count];
    s->wid = (uint8_t)atoi(widBuf);
    copy_field(s->label, sizeof(s->label), p, e);
    p = e + 1;
    e = strchr(p, CURRENCY_DELIM);
    const char *valEnd = e ? e : (p + strlen(p));
    copy_field(s->value, sizeof(s->value), p, valEnd);
    s->valid = (strcmp(s->value, "--") != 0);
    Currency_count++;
    if (!e) { break; }
    p = e + 1;
  }
}

void Currency_init() {
  Currency_count = 0;
  if (persist_exists(CURRENCY_PERSIST_KEY_DATA)) {
    char buf[256];
    int n = persist_read_string(CURRENCY_PERSIST_KEY_DATA, buf, sizeof(buf));
    if (n > 0) { Currency_parse(buf); }
  }
#ifdef SCREENSHOT_FIXTURES
  // Demo pairs for appstore screenshots (no phone): EUR/USD, USD/JPY.
  Currency_count = 2;
  Currency_slots[0] = (CryptoSlot){ .wid = 216, .valid = true };
  strcpy(Currency_slots[0].label, "EUR/USD"); strcpy(Currency_slots[0].value, "1.0823");
  Currency_slots[1] = (CryptoSlot){ .wid = 217, .valid = true };
  strcpy(Currency_slots[1].label, "USD/JPY"); strcpy(Currency_slots[1].value, "162.4");
#endif
}
