// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "poll_state_calc.h"

int32_t poll_stamp_sanitize(int32_t stored, int32_t now) {
  if (stored <= 0) { return 0; }
  if (stored > now) { return now; }
  return stored;
}

bool poll_cold_allowed(int32_t lastCold, int32_t now, int32_t minIntervalS) {
  if (lastCold <= 0) { return true; }
  if (lastCold > now) { return true; }
  return (now - lastCold) >= minIntervalS;
}
