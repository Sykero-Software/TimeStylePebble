// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#pragma once
#include <stdint.h>
#include <stddef.h>

// Rounded percentage 100*value/base. Returns -1 when base <= 0 (caller hides the
// percent). value < 0 is clamped to 0. The result MAY exceed 100 (overtime).
int twt_percent(int32_t value, int32_t base);

// Denominator for the unbudgeted task percent. The phone's per-task minutes are
// GROSS (no auto-pause deduction) while its day total is NET, so dividing the task
// time by the net total overshoots 100% on a single-task day. When the phone sent a
// gross day total (gross_before > 0), use it plus the live running segment
// (gross/gross). Otherwise (older phone app -> field is 0/garbled) fall back to the
// net day total — never to 0 + running, which would wildly inflate the percent.
int32_t twt_task_pct_base(int32_t gross_before, int32_t running, int32_t net_total);

// Filled width in pixels for a progress bar of total width `width_px` representing
// value/base, clamped to [0, width_px]. Returns 0 when base <= 0, value <= 0, or
// width_px <= 0.
int twt_bar_fill_px(int32_t value, int32_t base, int width_px);

// Formats minutes as "h:mm". Negative values get a leading '-' on the magnitude
// (e.g. -65 -> "-1:05", 0 -> "0:00", 90 -> "1:30"). Writes into buf (bufsize >= 8).
void twt_fmt_hhmm_signed(char* buf, size_t bufsize, int32_t minutes);
