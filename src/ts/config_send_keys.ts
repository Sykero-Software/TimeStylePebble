// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

// Watch settings sent "straight through" from the Clay config to the watchapp in
// webviewclosed (index.ts): each is coerced to an int (or kept as a string for the
// STRING_KEYS) and placed in the outbound AppMessage dict. Every scalar
// toggle / radiogroup / select / numeric-input setting the watch reads MUST appear
// here, or its config value never reaches the watch and it silently stays at the C
// default (the bug that first shipped SettingStatusClockDigital unwired: the render
// swap worked, but the phone never sent the flag). Kept in its own module so tests
// can assert coverage without loading index.ts's Pebble runtime.
export const STRAIGHT_THROUGH_KEYS: string[] = [
  'SettingLanguageID', 'SettingShowLeadingZero', 'SettingClockFontId', 'SettingDisconnectIcon',
  'SettingBluetoothVibe', 'SettingMidiVibe', 'SettingMidiSecondPrecision', 'SettingBigDate', 'SettingBigDateMonth', 'SettingBigDateFont', 'SettingTwtShowRemaining',
  'SettingTwtTargetVibe', 'SettingTwtBudgetVibe', 'SettingHourlyVibe',
  'SettingStatusStripFullWidth', 'SettingUseLargeFonts', 'SettingUseMetric',
  'SettingShowBatteryPct', 'SettingDisableAutobattery', 'SettingAltClockName', 'SettingAltClockOffset',
  'SettingDecimalSep',
  'SettingPollIntervalMin', 'SettingElecQuietStart', 'SettingElecQuietEnd',
  'SettingElecCheapFactorPct',
  'SettingAutoBatteryThreshold', 'SettingFallbackColumn', 'SettingFallbackPosition',
  'SettingClockStyle', 'SettingAnalogTicks', 'SettingAnalogDigitalClock', 'SettingStatusClockDigital',
];
