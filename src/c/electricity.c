// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "electricity.h"
#include <string.h>
#include <time.h>

ElectricityInfo Electricity_info;

bool Electricity_hasData(void) { return Electricity_info.count > 0; }

void Electricity_init() {
  Electricity_info.startEpoch = 0;
  Electricity_info.count = 0;
  memset(Electricity_info.prices, 0, sizeof(Electricity_info.prices));

  if (persist_exists(ELEC_PERSIST_KEY_META)) {
    ElectricityMeta meta;
    persist_read_data(ELEC_PERSIST_KEY_META, &meta, sizeof(meta));
    Electricity_info.startEpoch = meta.startEpoch;
    uint16_t count = meta.count;
    if (count > ELEC_MAX_QUARTERS) { count = ELEC_MAX_QUARTERS; }
    Electricity_info.count = count;

    if (persist_exists(ELEC_PERSIST_KEY_DATA0)) {
      persist_read_data(ELEC_PERSIST_KEY_DATA0, Electricity_info.prices,
                        ELEC_CHUNK0_COUNT * sizeof(int16_t));
    }
    if (persist_exists(ELEC_PERSIST_KEY_DATA1)) {
      persist_read_data(ELEC_PERSIST_KEY_DATA1,
                        &Electricity_info.prices[ELEC_CHUNK0_COUNT],
                        (ELEC_MAX_QUARTERS - ELEC_CHUNK0_COUNT) * sizeof(int16_t));
    }
  }
#ifdef SCREENSHOT_FIXTURES
  // Demo electricity for appstore screenshots: 4.5 snt today-average, 6.3 snt now.
  {
    time_t now = time(NULL);
    Electricity_info.startEpoch = (uint32_t)(now - (now % 900)) - 8 * 3600;
    Electricity_info.count = 96;                                  // 24 h of 15-min quarters
    for (int i = 0; i < 96; i++) { Electricity_info.prices[i] = 450; }
    int cur;
    if (elec_current_index(Electricity_info.startEpoch, Electricity_info.count,
                           (int64_t)now, &cur)) {
      Electricity_info.prices[cur] = 630;                         // 6.3 snt right now
    }
  }
#endif
}

void Electricity_saveData() {
  ElectricityMeta meta = { .startEpoch = Electricity_info.startEpoch,
                           .count = Electricity_info.count };
  persist_write_data(ELEC_PERSIST_KEY_META, &meta, sizeof(meta));
  persist_write_data(ELEC_PERSIST_KEY_DATA0, Electricity_info.prices,
                     ELEC_CHUNK0_COUNT * sizeof(int16_t));
  persist_write_data(ELEC_PERSIST_KEY_DATA1,
                     &Electricity_info.prices[ELEC_CHUNK0_COUNT],
                     (ELEC_MAX_QUARTERS - ELEC_CHUNK0_COUNT) * sizeof(int16_t));
}

void Electricity_deinit() {
  Electricity_saveData();
}

bool Electricity_getCurrentPrice(int16_t *out) {
  int idx;
  if (!elec_current_index(Electricity_info.startEpoch, Electricity_info.count,
                          (int64_t)time(NULL), &idx)) {
    return false;
  }
  *out = Electricity_info.prices[idx];
  return true;
}

bool Electricity_getTodayAverage(int16_t *out) {
  time_t now = time(NULL);

  struct tm start_tm = *localtime(&now);
  start_tm.tm_hour = 0;
  start_tm.tm_min = 0;
  start_tm.tm_sec = 0;
  time_t dayStart = mktime(&start_tm);

  struct tm end_tm = *localtime(&now);
  end_tm.tm_hour = 0;
  end_tm.tm_min = 0;
  end_tm.tm_sec = 0;
  end_tm.tm_mday += 1;            // mktime normalises month/day rollover
  time_t dayEnd = mktime(&end_tm);

  return elec_today_average(Electricity_info.prices, Electricity_info.count,
                            Electricity_info.startEpoch,
                            (int64_t)dayStart, (int64_t)dayEnd, out);
}

#define ELEC_WIN_QUARTERS 4   // 1 hour = four 15-min quarters

// Fills eligible[] for all valid quarters (true = NOT in quiet hours) and
// returns the current quarter index, or -1 if now is outside the table.
static int elec_build_eligible(bool *eligible, int quietStartHour, int quietEndHour) {
  for (int i = 0; i < (int)Electricity_info.count; i++) {
    time_t qstart = (time_t)(Electricity_info.startEpoch + (uint32_t)i * 900);
    struct tm lt = *localtime(&qstart);
    eligible[i] = !elec_hour_in_quiet(lt.tm_hour, quietStartHour, quietEndHour);
  }
  int idx;
  if (!elec_current_index(Electricity_info.startEpoch, Electricity_info.count,
                          (int64_t)time(NULL), &idx)) {
    return -1;
  }
  return idx;
}

static void elec_fill_display(const ElecWindow *w, int currentIdx, ElecDisplay *out) {
  time_t qstart = (time_t)(Electricity_info.startEpoch + (uint32_t)w->startIdx * 900);
  struct tm wt = *localtime(&qstart);
  out->startHour = wt.tm_hour;
  out->startMin = wt.tm_min;
  out->avgCenti = w->avgCenti;
  out->now = (w->startIdx == currentIdx);
  time_t now = time(NULL);
  struct tm nt = *localtime(&now);
  out->today = (wt.tm_yday == nt.tm_yday && wt.tm_year == nt.tm_year);
}

bool Electricity_getNextCheap(int quietStartHour, int quietEndHour, int factorPct,
                              int16_t floorCenti, int16_t ceilingCenti,
                              ElecDisplay *out) {
  static bool eligible[ELEC_MAX_QUARTERS];
  int currentIdx = elec_build_eligible(eligible, quietStartHour, quietEndHour);
  if (currentIdx < 0) { return false; }
  int16_t mean;
  if (!elec_eligible_mean(Electricity_info.prices, eligible,
                          Electricity_info.count, currentIdx, &mean)) {
    return false;
  }
  int16_t bar = elec_cheap_bar(mean, factorPct, floorCenti, ceilingCenti);
  // Earliest hour below the cheap threshold; if none qualifies, fall back to
  // the cheapest upcoming hour so the widget shows that rather than "--".
  ElecWindow w = elec_find_next_cheap_or_cheapest(Electricity_info.prices, eligible,
                                                  Electricity_info.count, currentIdx,
                                                  bar, ELEC_WIN_QUARTERS);
  if (!w.found) { return false; }
  elec_fill_display(&w, currentIdx, out);
  return true;
}

bool Electricity_getCheapestHour(int quietStartHour, int quietEndHour,
                                 ElecDisplay *out) {
  static bool eligible[ELEC_MAX_QUARTERS];
  int currentIdx = elec_build_eligible(eligible, quietStartHour, quietEndHour);
  if (currentIdx < 0) { return false; }
  ElecWindow w = elec_find_cheapest(Electricity_info.prices, eligible,
                                    Electricity_info.count, currentIdx,
                                    ELEC_WIN_QUARTERS);
  if (!w.found) { return false; }
  elec_fill_display(&w, currentIdx, out);
  return true;
}
