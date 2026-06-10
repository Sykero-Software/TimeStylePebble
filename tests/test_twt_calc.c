// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "twt_calc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  // --- twt_percent (rounded 100*value/base; -1 sentinel when base<=0) ---
  assert(twt_percent(240, 450) == 53);   // 4:00 of 7:30 -> 53.3 -> 53
  assert(twt_percent(225, 450) == 50);   // exactly half
  assert(twt_percent(0,   450) == 0);
  assert(twt_percent(450, 450) == 100);
  assert(twt_percent(495, 450) == 110);  // overtime > 100 allowed
  assert(twt_percent(120, 240) == 50);   // task 2:00 of day 4:00
  assert(twt_percent(100, 0)   == -1);   // base 0 -> hidden
  assert(twt_percent(100, -5)  == -1);   // negative base -> hidden
  assert(twt_percent(-10, 450) == 0);    // negative value clamped to 0
  assert(twt_percent(1200, 1200) == 100);  // task at budget
  assert(twt_percent(1500, 1200) == 125);  // over budget allowed

  // --- twt_task_pct_base (denominator for the unbudgeted task percent) ---
  // Phone sent a gross day total -> use it (plus the live running segment), so the
  // percent is gross/gross and a single-task day reads exactly 100%.
  assert(twt_task_pct_base(168, 30, 228) == 198);  // gross 168 + running 30
  // No gross value (older phone app sends nothing -> field stays 0): fall back to
  // the net day total. NOT to 0+running, which would wildly inflate the percent.
  assert(twt_task_pct_base(0, 30, 228) == 228);
  assert(twt_task_pct_base(0, 0, 0) == 0);         // day's first second, nothing anywhere
  assert(twt_task_pct_base(-5, 30, 228) == 228);   // garbled negative gross -> fallback

  // --- twt_bar_fill_px (filled width for value/base over width_px, clamped) ---
  assert(twt_bar_fill_px(225, 450, 100) == 50);
  assert(twt_bar_fill_px(0,   450, 100) == 0);
  assert(twt_bar_fill_px(450, 450, 100) == 100);
  assert(twt_bar_fill_px(900, 450, 100) == 100);  // overtime capped to full
  assert(twt_bar_fill_px(225, 0,   100) == 0);    // no target -> empty
  assert(twt_bar_fill_px(225, 450, 0)   == 0);    // zero width

  // --- twt_fmt_hhmm_signed (sign-aware "h:mm"; negative gets leading '-') ---
  char b[12];
  twt_fmt_hhmm_signed(b, sizeof(b), 90);    assert(strcmp(b, "1:30") == 0);
  twt_fmt_hhmm_signed(b, sizeof(b), 0);     assert(strcmp(b, "0:00") == 0);
  twt_fmt_hhmm_signed(b, sizeof(b), 5);     assert(strcmp(b, "0:05") == 0);
  twt_fmt_hhmm_signed(b, sizeof(b), -45);   assert(strcmp(b, "-0:45") == 0);
  twt_fmt_hhmm_signed(b, sizeof(b), -65);   assert(strcmp(b, "-1:05") == 0);
  twt_fmt_hhmm_signed(b, sizeof(b), 630);   assert(strcmp(b, "10:30") == 0);

  printf("All twt_calc tests passed\n");
  return 0;
}
