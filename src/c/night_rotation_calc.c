// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "night_rotation_calc.h"
#include "electricity_calc.h"

bool night_rotation_active(int mode, int hour, int startHour, int endHour,
                           bool quietTimeActive) {
  switch (mode) {
    case NIGHT_ROTATION_QUIET_TIME:
      return quietTimeActive;
    case NIGHT_ROTATION_CUSTOM:
      // Same half-open, midnight-wrapping window as the electricity quiet hours, reused
      // rather than re-derived so the two settings behave identically (and so this stays
      // covered by the existing host tests for that predicate).
      return elec_hour_in_quiet(hour, startHour, endHour);
    case NIGHT_ROTATION_OFF:
    default:
      return false;
  }
}

int night_rotation_interval(int configuredSec, bool nightActive) {
  return nightActive ? 0 : configuredSec;
}
