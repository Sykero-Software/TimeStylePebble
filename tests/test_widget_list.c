// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#include "widget_list.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t collected[64];
static int collectedN;
static void collect_cb(uint8_t id, void *ctx) { (void)ctx; collected[collectedN++] = id; }

int main(void) {
  // --- isDrawableId ---
  assert(!WidgetList_isDrawableId(0));      // EMPTY not drawable
  assert(WidgetList_isDrawableId(2));       // battery
  assert(WidgetList_isDrawableId(19));      // last normal type
  assert(!WidgetList_isDrawableId(20));     // gap
  assert(WidgetList_isDrawableId(200));     // crypto base
  assert(WidgetList_isDrawableId(215));     // crypto last
  assert(!WidgetList_isDrawableId(216));
  assert(!WidgetList_isDrawableId(255));    // marker is not an id

  // --- intervalSeconds ---
  assert(WidgetList_intervalSeconds(0) == 5);
  assert(WidgetList_intervalSeconds(2) == 30);
  assert(WidgetList_intervalSeconds(3) == 60);
  assert(WidgetList_intervalSeconds(5) == 300);
  assert(WidgetList_intervalSeconds(9) == 60);   // unknown -> 1min

  // --- sanitize: plain list unchanged ---
  { uint8_t b[] = {12, 15, 17}; int n = WidgetList_sanitize(b, 3, 16);
    assert(n == 3 && b[0] == 12 && b[1] == 15 && b[2] == 17); }

  // --- sanitize: drop EMPTY + invalid plain ids ---
  { uint8_t b[] = {12, 0, 99, 17}; int n = WidgetList_sanitize(b, 4, 16);
    assert(n == 2 && b[0] == 12 && b[1] == 17); }

  // --- sanitize: valid group preserved ---
  { uint8_t b[] = {2, 0xFF, 2, 1, 15, 16, 3}; int n = WidgetList_sanitize(b, 7, 16);
    assert(n == 7 && b[0] == 2 && b[1] == 0xFF && b[2] == 2 && b[3] == 1
           && b[4] == 15 && b[5] == 16 && b[6] == 3); }

  // --- sanitize: drop invalid members, fix count ---
  { uint8_t b[] = {0xFF, 3, 0, 15, 99, 16}; int n = WidgetList_sanitize(b, 6, 16);
    assert(n == 5 && b[0] == 0xFF && b[1] == 2 && b[2] == 0 && b[3] == 15 && b[4] == 16); }

  // --- sanitize: single valid member degrades to plain ---
  { uint8_t b[] = {0xFF, 2, 1, 15, 99}; int n = WidgetList_sanitize(b, 5, 16);
    assert(n == 1 && b[0] == 15); }

  // --- sanitize: bad interval code clamped to 3 ---
  { uint8_t b[] = {0xFF, 2, 9, 15, 16}; int n = WidgetList_sanitize(b, 5, 16);
    assert(n == 5 && b[2] == 3); }

  // --- sanitize: truncated group with 1 present valid member degrades to plain ---
  { uint8_t b[] = {12, 0xFF, 3, 1, 15}; int n = WidgetList_sanitize(b, 5, 16);
    assert(n == 2 && b[0] == 12 && b[1] == 15); }   // group declared 3, only 15 present -> plain

  // --- sanitize: truncated group with 0 present members is dropped ---
  { uint8_t b[] = {12, 0xFF, 3, 1}; int n = WidgetList_sanitize(b, 4, 16);
    assert(n == 1 && b[0] == 12); }

  // --- parse: plain -> count 1 slots ---
  { WidgetSlot s[MAX_WIDGET_SLOTS]; uint8_t b[] = {12, 15, 17};
    int n = WidgetList_parse(b, 3, s, MAX_WIDGET_SLOTS);
    assert(n == 3 && s[0].count == 1 && s[0].members[0] == 12 && s[2].members[0] == 17); }

  // --- parse: group -> members + interval ---
  { WidgetSlot s[MAX_WIDGET_SLOTS]; uint8_t b[] = {2, 0xFF, 3, 1, 15, 16, 17};
    int n = WidgetList_parse(b, 7, s, MAX_WIDGET_SLOTS);
    assert(n == 2);
    assert(s[0].count == 1 && s[0].members[0] == 2);
    assert(s[1].count == 3 && s[1].interval_code == 1);
    assert(s[1].members[0] == 15 && s[1].members[1] == 16 && s[1].members[2] == 17); }

  // --- forEachId: members + plain ids, skip marker/count/interval ---
  { collectedN = 0; uint8_t b[] = {2, 0xFF, 2, 1, 15, 16, 3};
    WidgetList_forEachId(b, 7, collect_cb, NULL);
    assert(collectedN == 4 && collected[0] == 2 && collected[1] == 15
           && collected[2] == 16 && collected[3] == 3); }

  // --- minSubMinuteIntervalSec ---
  { uint8_t none[] = {12, 0xFF, 2, 3, 15, 16};   // group @ 1min -> no sub-minute
    assert(WidgetList_minSubMinuteIntervalSec(none, 6) == 0); }
  { uint8_t a[] = {0xFF, 2, 1, 15, 16};          // 10s
    assert(WidgetList_minSubMinuteIntervalSec(a, 5) == 10); }
  { uint8_t a[] = {0xFF, 2, 2, 15, 16, 0xFF, 2, 1, 17, 200}; // 30s + 10s -> min 10
    assert(WidgetList_minSubMinuteIntervalSec(a, 10) == 10); }

  // --- activeMember: rotation by seconds ---
  { WidgetSlot s; s.count = 3; s.interval_code = 1; // 10s
    s.members[0] = 15; s.members[1] = 16; s.members[2] = 17;
    assert(WidgetSlot_activeMember(&s, 0)  == 15);   // step 0
    assert(WidgetSlot_activeMember(&s, 10) == 16);   // step 1
    assert(WidgetSlot_activeMember(&s, 25) == 17);   // step 2 (20..29)
    assert(WidgetSlot_activeMember(&s, 30) == 15); } // step 3 -> wrap
  { WidgetSlot s; s.count = 1; s.members[0] = 12;
    assert(WidgetSlot_activeMember(&s, 12345) == 12); } // plain ignores time

  printf("All widget_list tests passed\n");
  return 0;
}
