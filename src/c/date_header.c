// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

#include "date_header.h"
#include "date_header_calc.h"
#include "settings.h"
#include "languages.h"

// One centered line, e.g. "Ti 6.6" — weekday (title-cased) + day.month. No
// trailing period: the longest date ("Su 31.12") only fits the BITHAM_30_BLACK
// line on emery with two panels + large fonts (122px) without it.
static TextLayer* s_date_layer;
static char s_date_buffer[24];
static GRect s_frame;   // current layer frame; drives auto-scaling
#define DATE_FIT_MARGIN 4   // total px of width slack (TextLayer inset + safety);
                            // if even the smallest font overflows, the layer's
                            // trailing-ellipsis mode is the fallback.

// Largest -> smallest. Bitham 30 keeps today's look on days it fits; Gothic
// bold sizes are the fallback for tight dates. All fit BIG_DATE_HEIGHT (34px).
// `asciiOnly`: Bitham is a decorative display font with no accented/extended-
// Latin, Cyrillic, Greek, etc. glyphs — it renders them blank (Polish "Śro" ->
// "ro"), so it is skipped for any non-ASCII date name. Gothic has full coverage
// (and is what the sidebar already uses for these names).
static const struct { const char* key; bool asciiOnly; } s_date_font_ladder[] = {
  { FONT_KEY_BITHAM_30_BLACK, true  },
  { FONT_KEY_GOTHIC_28_BOLD,  false },
  { FONT_KEY_GOTHIC_24_BOLD,  false },
  { FONT_KEY_GOTHIC_18_BOLD,  false },
};

// Return the largest ladder font whose rendered width fits `width`. Falls back
// to the smallest if none fit (the layer's trailing-ellipsis then applies).
static GFont pick_date_font(const char* text, int16_t width) {
  const int count = sizeof(s_date_font_ladder) / sizeof(s_date_font_ladder[0]);
  const bool ascii = DateHeader_textIsAscii(text);
  GFont chosen = fonts_get_system_font(s_date_font_ladder[count - 1].key);
  for (int i = 0; i < count; i++) {
    if (s_date_font_ladder[i].asciiOnly && !ascii) continue;  // font lacks the glyphs
    GFont f = fonts_get_system_font(s_date_font_ladder[i].key);
    // Wide box -> no wrapping -> natural single-line width.
    GSize sz = graphics_text_layout_get_content_size(
        text, f, GRect(0, 0, 1000, 1000),
        GTextOverflowModeWordWrap, GTextAlignmentCenter);
    if (sz.w <= width - DATE_FIT_MARGIN) {
      chosen = f;
      break;
    }
  }
  return chosen;
}

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
  // day-of-month (+ optional month) with no leading zeros
  if (settings.showBigDateMonth) {
    snprintf(s_date_buffer, sizeof(s_date_buffer), "%s %d.%d",
             day, timeInfo->tm_mday, timeInfo->tm_mon + 1);
  } else {
    snprintf(s_date_buffer, sizeof(s_date_buffer), "%s %d",
             day, timeInfo->tm_mday);
  }
#ifdef SCREENSHOT_FIXTURES
  // Deterministic short demo date for appstore screenshots; honours the month
  // setting so both states can be captured. Screenshot-only; compiles out.
  if (settings.showBigDateMonth) {
    snprintf(s_date_buffer, sizeof(s_date_buffer), "%s 8.6", day);
  } else {
    snprintf(s_date_buffer, sizeof(s_date_buffer), "%s 8", day);
  }
#endif
}

void DateHeader_redraw(void) {
  if (!s_date_layer) return;
  text_layer_set_text_color(s_date_layer, settings.timeColor); // track color setting changes
  text_layer_set_background_color(s_date_layer, settings.dateBgColor); // track color setting changes
  // Re-measured every redraw: the string/width only change on a date rollover,
  // setting, or layout change, but ≤4 measurements of a short static string are
  // cheap enough to not bother caching, even on the per-second tick path.
  text_layer_set_font(s_date_layer, pick_date_font(s_date_buffer, s_frame.size.w));
  text_layer_set_text(s_date_layer, s_date_buffer);
  layer_mark_dirty(text_layer_get_layer(s_date_layer));
}

void DateHeader_setFrame(GRect frame) {
  if (!s_date_layer) return;
  s_frame = frame;
  layer_set_frame(text_layer_get_layer(s_date_layer), frame);
}

void DateHeader_initLayer(Layer* parent, GRect frame) {
  if (!DateHeader_isSupported()) return;
  s_date_layer = text_layer_create(frame);
  s_frame = frame;
  text_layer_set_background_color(s_date_layer, settings.dateBgColor); // GColorClear = inherit
  text_layer_set_text_color(s_date_layer, settings.timeColor);
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
