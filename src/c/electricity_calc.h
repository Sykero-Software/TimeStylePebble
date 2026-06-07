#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ELEC_MAX_QUARTERS 192

// Quarter index for `now`; quarters are at startEpoch + i*900 (seconds, UTC).
// Returns false if count==0 or now is outside [startEpoch, startEpoch+count*900).
bool elec_current_index(uint32_t startEpoch, uint16_t count, int64_t now, int *outIdx);

// Average of all quarters whose start falls in [dayStart, dayEnd) (epoch seconds).
// out is in 0.01 snt (rounded). Returns false if no quarter falls in the window.
bool elec_today_average(const int16_t *prices, uint16_t count, uint32_t startEpoch,
                        int64_t dayStart, int64_t dayEnd, int16_t *out);

// Formats value_centi (0.01 snt/kWh) as "x.x" with one decimal of snt into buf.
// `sep` is the decimal separator char. Handles negatives. buf should be >= 12 bytes.
void elec_format_price(int16_t value_centi, char sep, char *buf, size_t buflen);
