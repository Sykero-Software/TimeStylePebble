#include "electricity_calc.h"
#include <stdio.h>

bool elec_current_index(uint32_t startEpoch, uint16_t count, int64_t now, int *outIdx) {
  if (count == 0) { return false; }
  if (now < (int64_t)startEpoch) { return false; }
  int64_t idx = (now - (int64_t)startEpoch) / 900;
  if (idx >= (int64_t)count) { return false; }
  *outIdx = (int)idx;
  return true;
}

bool elec_today_average(const int16_t *prices, uint16_t count, uint32_t startEpoch,
                        int64_t dayStart, int64_t dayEnd, int16_t *out) {
  int64_t sum = 0;
  int n = 0;
  for (int i = 0; i < (int)count; i++) {
    int64_t start = (int64_t)startEpoch + (int64_t)i * 900;
    if (start >= dayStart && start < dayEnd) {
      sum += prices[i];
      n++;
    }
  }
  if (n == 0) { return false; }
  int64_t avg = (sum >= 0) ? (sum + n / 2) / n : -(((-sum) + n / 2) / n);
  *out = (int16_t)avg;
  return true;
}

void elec_format_price(int16_t value_centi, char sep, char *buf, size_t buflen) {
  // value_centi is 0.01 snt/kWh; display one decimal of snt (0.1 snt), rounded.
  int deci = (value_centi >= 0) ? (value_centi + 5) / 10
                                : -(((-value_centi) + 5) / 10);
  int neg = (deci < 0);
  int a = neg ? -deci : deci;
  int whole = a / 10;
  int frac = a % 10;
  snprintf(buf, buflen, "%s%d%c%d", neg ? "-" : "", whole, sep, frac);
}
