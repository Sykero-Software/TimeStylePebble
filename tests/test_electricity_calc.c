// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "electricity_calc.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
  // --- elec_format_price (value in 0.01 snt -> "x.x" with one decimal) ---
  char buf[12];
  elec_format_price(523, '.', buf, sizeof(buf));   assert(strcmp(buf, "5.2") == 0);
  elec_format_price(525, '.', buf, sizeof(buf));   assert(strcmp(buf, "5.3") == 0);
  elec_format_price(-30, '.', buf, sizeof(buf));   assert(strcmp(buf, "-0.3") == 0);
  elec_format_price(8000, '.', buf, sizeof(buf));  assert(strcmp(buf, "80.0") == 0);
  elec_format_price(0, ',', buf, sizeof(buf));     assert(strcmp(buf, "0,0") == 0);

  // --- elec_current_index (quarters at startEpoch + i*900) ---
  int idx;
  assert(elec_current_index(1000, 4, 1000, &idx) && idx == 0);
  assert(elec_current_index(1000, 4, 2850, &idx) && idx == 2);
  assert(elec_current_index(1000, 4, 4599, &idx) && idx == 3);
  assert(!elec_current_index(1000, 4, 999, &idx));   // before table
  assert(!elec_current_index(1000, 4, 4600, &idx));  // past table
  assert(!elec_current_index(1000, 0, 1000, &idx));  // empty table

  // --- elec_today_average (quarters at startEpoch + i*900) ---
  int16_t prices[4] = {100, 200, 300, 400};
  int16_t avg;
  assert(elec_today_average(prices, 4, 0, 0, 1800, &avg) && avg == 150);
  assert(elec_today_average(prices, 4, 0, 1800, 100000, &avg) && avg == 350);
  assert(!elec_today_average(prices, 4, 0, 100000, 200000, &avg)); // no entries in window
  int16_t neg[2] = {-100, -300};
  assert(elec_today_average(neg, 2, 0, 0, 1800, &avg) && avg == -200);

  // --- elec_hour_in_quiet (night window [start,end), wraps midnight) ---
  assert(elec_hour_in_quiet(23, 23, 7));   // start hour is quiet
  assert(elec_hour_in_quiet(2, 23, 7));    // after midnight, before end
  assert(!elec_hour_in_quiet(7, 23, 7));   // end hour is NOT quiet (half-open)
  assert(!elec_hour_in_quiet(12, 23, 7));  // midday awake
  assert(elec_hour_in_quiet(13, 12, 18));  // non-wrapping window
  assert(!elec_hour_in_quiet(18, 12, 18)); // non-wrapping end exclusive
  assert(!elec_hour_in_quiet(3, 8, 8));    // start==end => no quiet hours

  printf("All electricity_calc tests passed\n");
  return 0;
}
