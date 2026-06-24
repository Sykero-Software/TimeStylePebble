// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Marker-encoded widget-list format, shared by the watch (sidebar/settings/
   messaging/main) and host-compiled for tests/test_widget_list.c. Pebble-free
   (stdint/stdbool only), same pattern as twt_calc.c.

   A "list" is a byte array; each slot is either ONE plain widget id, or a rotating
   group: WIDGET_ROTATING_MARKER, count, interval_code, member_1 .. member_count.
   See docs/superpowers/specs/2026-06-24-timestyle-rotating-widget-design.md */

// Numeric mirrors — keep in sync with src/c/sidebar_widgets.h (MAX_WIDGET_TYPE),
// src/c/crypto.h (CRYPTO_WID_BASE / MAX_CRYPTO), src/ts/widget_list_payload.ts.
#define WL_MAX_WIDGET_TYPE   20    // DEEP_SLEEP_TIMER
#define WL_CRYPTO_WID_BASE   200
#define WL_MAX_CRYPTO        16

#define WIDGET_ROTATING_MARKER 0xFF
#define MAX_GROUP_MEMBERS      6
#define MAX_WIDGET_SLOTS       16

typedef struct {
  uint8_t members[MAX_GROUP_MEMBERS];
  uint8_t count;          // 1 = plain slot; 2..MAX_GROUP_MEMBERS = rotating
  uint8_t interval_code;  // 0..5, meaningful only when count > 1
} WidgetSlot;

// Drawable id: a normal type 1..WL_MAX_WIDGET_TYPE or a crypto wid. EMPTY(0) draws
// nothing, so it is NOT drawable and is dropped from lists.
bool WidgetList_isDrawableId(uint8_t id);

// interval_code 0..5 -> seconds {5,10,30,60,120,300}; any other code -> 60.
int WidgetList_intervalSeconds(uint8_t code);

// Clean a marker-encoded list IN PLACE to canonical form: drop non-drawable plain
// ids and members, clamp interval_code, degrade a group with <2 valid members to a
// plain slot (or drop), drop a truncated trailing group. Reads at most min(len,
// maxBytes) bytes. Returns the new byte length (<= maxBytes).
int WidgetList_sanitize(uint8_t *bytes, int len, int maxBytes);

// Parse a (sanitized) marker-encoded list into WidgetSlot[]. Returns slot count
// (<= maxSlots). Defensive against malformed input.
int WidgetList_parse(const uint8_t *bytes, int len, WidgetSlot *out, int maxSlots);

// Invoke cb(id, ctx) for every underlying drawable widget id (group members + plain
// ids), skipping marker/count/interval bytes.
void WidgetList_forEachId(const uint8_t *bytes, int len,
                          void (*cb)(uint8_t id, void *ctx), void *ctx);

// Shortest sub-minute (5/10/30 s) rotation interval present, or 0 if none.
int WidgetList_minSubMinuteIntervalSec(const uint8_t *bytes, int len);

// The member a slot shows at the given seconds-of-day. count<=1 -> members[0].
uint8_t WidgetSlot_activeMember(const WidgetSlot *slot, int secondsOfDay);
