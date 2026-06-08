#pragma once
#include <stdint.h>

// Rounded percentage 100*value/base. Returns -1 when base <= 0 (caller hides the
// percent). value < 0 is clamped to 0. The result MAY exceed 100 (overtime).
int twt_percent(int32_t value, int32_t base);

// Filled width in pixels for a progress bar of total width `width_px` representing
// value/base, clamped to [0, width_px]. Returns 0 when base <= 0, value <= 0, or
// width_px <= 0.
int twt_bar_fill_px(int32_t value, int32_t base, int width_px);
