// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#pragma once
#include <stdint.h>
#include <stdbool.h>

// Arithmetic for the persisted watch->phone poll clock. Pebble-free so it
// host-compiles for tests/test_poll_state_calc.c.

// Normalise a timestamp restored from persist: <= 0 means "never requested" (0, so
// the caller polls at the next opportunity); a future value means the clock moved
// backwards, so clamp it to now instead of blocking polls until it is reached.
int32_t poll_stamp_sanitize(int32_t stored, int32_t now);

// True when a cold (forced) request may be sent again. A future lastCold is treated
// as allowed, for the same clock-jump reason.
bool poll_cold_allowed(int32_t lastCold, int32_t now, int32_t minIntervalS);

// True when the shared poll interval has elapsed, i.e. a watch->phone data request
// is due. Used by BOTH the tick poll and the BT-reconnect refresh, so a reconnect
// (or a flapping link) can never out-pace the configured interval.
bool poll_due(int32_t lastRequest, int32_t now, int32_t intervalS);
