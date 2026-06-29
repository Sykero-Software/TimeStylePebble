// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "date_header.h"
#include "settings.h"
#include "languages.h"

// One centered line, e.g. "Ti 6.6" — weekday (title-cased) + day.month. No
// trailing period: the longest date ("Su 31.12") only fits the BITHAM_30_BLACK
// line on emery with two panels + large fonts (122px) without it.
static TextLayer* s_date_layer;
static char s_date_buffer[24];

bool DateHeader_isSupported(void) {
#if defined(PBL_RECT) && !defined(PBL_PLATFORM_APLITE)
  return true;
#else
  return false;
#endif
}

void DateHeader_updateTime(struct tm* timeInfo) {
  char day[8];
  strncpy(day, dayNames[settings.languageId][timeInfo->tm_wday], sizeof(day));
  day[sizeof(day) - 1] = '\0';
  // Title-case ASCII names ("TI" -> "Ti"): lowercase only A-Z after the first
  // byte. High bytes are left untouched so multi-byte UTF-8 names (Greek,
  // Cyrillic, CJK, accented Latin) are never mangled.
  for (int i = 1; day[i] != '\0'; i++) {
    if (day[i] >= 'A' && day[i] <= 'Z') {
      day[i] = day[i] - 'A' + 'a';
    }
  }
  // day-of-month and month with no leading zeros
  snprintf(s_date_buffer, sizeof(s_date_buffer), "%s %d.%d",
           day, timeInfo->tm_mday, timeInfo->tm_mon + 1);
#ifdef SCREENSHOT_FIXTURES
  // Deterministic short demo date for appstore screenshots: a single-digit day
  // fits the narrow centre column even with two sidebars (a wide 2-digit day
  // truncates to "Mon 2..."). Screenshot-only; compiles out of the shipped app.
  snprintf(s_date_buffer, sizeof(s_date_buffer), "%s 8.6", day);
#endif
}

void DateHeader_redraw(void) {
  if (!s_date_layer) return;
  text_layer_set_text_color(s_date_layer, settings.timeColor); // track color setting changes
  text_layer_set_background_color(s_date_layer, settings.dateBgColor); // track color setting changes
  text_layer_set_text(s_date_layer, s_date_buffer);
  layer_mark_dirty(text_layer_get_layer(s_date_layer));
}

void DateHeader_setFrame(GRect frame) {
  if (!s_date_layer) return;
  layer_set_frame(text_layer_get_layer(s_date_layer), frame);
}

void DateHeader_initLayer(Layer* parent, GRect frame) {
  if (!DateHeader_isSupported()) return;
  s_date_layer = text_layer_create(frame);
  text_layer_set_background_color(s_date_layer, settings.dateBgColor); // GColorClear = inherit
  text_layer_set_text_color(s_date_layer, settings.timeColor);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_date_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(parent, text_layer_get_layer(s_date_layer));
  layer_set_hidden(text_layer_get_layer(s_date_layer), true); // shown by main.c per setting
}

void DateHeader_setHidden(bool hidden) {
  if (s_date_layer) {
    layer_set_hidden(text_layer_get_layer(s_date_layer), hidden);
  }
}

void DateHeader_deinitLayer(void) {
  if (s_date_layer) {
    text_layer_destroy(s_date_layer);
    s_date_layer = NULL;
  }
}
