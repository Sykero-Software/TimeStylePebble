// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#include "poll_state_calc.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  const int32_t now = 1000000;

  // a sane stored stamp survives untouched
  assert(poll_stamp_sanitize(now - 500, now) == now - 500);

  // never requested -> 0, which makes the tick handler poll at the next minute
  assert(poll_stamp_sanitize(0, now) == 0);
  assert(poll_stamp_sanitize(-5, now) == 0);

  // a future stamp (clock/timezone moved backwards) is clamped to now, so the
  // interval starts over instead of blocking polls for years
  assert(poll_stamp_sanitize(now + 10000, now) == now);

  // cold rate limit: never-cold is allowed, just-cold is not, expired is
  assert(poll_cold_allowed(0, now, 600) == true);
  assert(poll_cold_allowed(now - 599, now, 600) == false);
  assert(poll_cold_allowed(now - 600, now, 600) == true);
  // a future cold stamp must not lock cold requests out forever
  assert(poll_cold_allowed(now + 5000, now, 600) == true);

  printf("PASS\n");
  return 0;
}
