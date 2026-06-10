// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "twt_calc.h"
#include <stdio.h>

int twt_percent(int32_t value, int32_t base) {
  if (base <= 0) return -1;
  if (value < 0) value = 0;
  // rounded: (100*value + base/2) / base
  return (int)(((int64_t)value * 100 + base / 2) / base);
}

int32_t twt_task_pct_base(int32_t gross_before, int32_t running, int32_t net_total) {
  if (gross_before <= 0) return net_total;
  return gross_before + running;
}

int twt_bar_fill_px(int32_t value, int32_t base, int width_px) {
  if (base <= 0 || value <= 0 || width_px <= 0) return 0;
  if (value >= base) return width_px;
  return (int)(((int64_t)value * width_px) / base);
}

void twt_fmt_hhmm_signed(char* buf, size_t bufsize, int32_t minutes) {
  int neg = minutes < 0;
  int32_t mag = neg ? -minutes : minutes;
  snprintf(buf, bufsize, "%s%d:%02d", neg ? "-" : "", (int)(mag / 60), (int)(mag % 60));
}
