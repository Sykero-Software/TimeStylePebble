#include "twt_calc.h"
#include <assert.h>
#include <stdio.h>

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

  // --- twt_bar_fill_px (filled width for value/base over width_px, clamped) ---
  assert(twt_bar_fill_px(225, 450, 100) == 50);
  assert(twt_bar_fill_px(0,   450, 100) == 0);
  assert(twt_bar_fill_px(450, 450, 100) == 100);
  assert(twt_bar_fill_px(900, 450, 100) == 100);  // overtime capped to full
  assert(twt_bar_fill_px(225, 0,   100) == 0);    // no target -> empty
  assert(twt_bar_fill_px(225, 450, 0)   == 0);    // zero width

  printf("All twt_calc tests passed\n");
  return 0;
}
