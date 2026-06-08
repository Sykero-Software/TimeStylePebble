#include "twt_calc.h"

int twt_percent(int32_t value, int32_t base) {
  if (base <= 0) return -1;
  if (value < 0) value = 0;
  // rounded: (100*value + base/2) / base
  return (int)(((int64_t)value * 100 + base / 2) / base);
}

int twt_bar_fill_px(int32_t value, int32_t base, int width_px) {
  if (base <= 0 || value <= 0 || width_px <= 0) return 0;
  if (value >= base) return width_px;
  return (int)(((int64_t)value * width_px) / base);
}
