// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
#pragma once
#include <stdbool.h>

// True when every byte of `s` is 7-bit ASCII (< 0x80). The big-date top font
// (Bitham) is a decorative display face that lacks accented / extended-Latin,
// Cyrillic, Greek, etc. glyphs and renders them blank (e.g. Polish "Śro" ->
// "ro"). Any non-ASCII date name must therefore fall back to Gothic (full
// coverage). Pure/SDK-free so it can be host-unit-tested.
bool DateHeader_textIsAscii(const char* s);
