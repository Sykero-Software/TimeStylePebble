// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Clay-native configuration for the TimeStyle watchface. Replaces the hosted
   Jekyll HTML config page. Every watch `messageKey` matches package.json's
   messageKeys list; every option `value` matches the integer/string the C side
   expects (cross-checked against the historical index.js webviewclosed mapping).

   Phone-only settings (weather location / data source, decimal electricity
   thresholds) use NON-watch messageKey labels and are handled entirely in the
   index.js webviewclosed handler (written to localStorage; never sent to the
   watch). */

interface ClayOption {
  label: string;
  value: number | string;
}

// Language options. Mirrors docs/_includes/config_common_options.html:349-387.
const LANGUAGES: ClayOption[] = [
  { label: 'English (Default)', value: 0 },
  { label: 'Bahasa Indonesia', value: 30 },
  { label: 'Catala', value: 15 },
  { label: 'Cestina', value: 7 },
  { label: 'Cymraeg', value: 32 },
  { label: 'Dansk', value: 21 },
  { label: 'Deutsch', value: 2 },
  { label: 'Eesti', value: 18 },
  { label: 'Espanol', value: 3 },
  { label: 'Euskara', value: 19 },
  { label: 'Francais', value: 1 },
  { label: 'Gaeilge', value: 26 },
  { label: 'Galego', value: 33 },
  { label: 'Hrvatski', value: 25 },
  { label: 'Italiano', value: 4 },
  { label: 'Latviesu', value: 27 },
  { label: 'Lietuviskai', value: 22 },
  { label: 'Magyar', value: 24 },
  { label: 'Nederlands', value: 5 },
  { label: 'Norsk', value: 16 },
  { label: 'Romana', value: 14 },
  { label: 'Polski', value: 11 },
  { label: 'Portugues', value: 8 },
  { label: 'Slovencina', value: 12 },
  { label: 'Slovenscina', value: 23 },
  { label: 'Suomi', value: 20 },
  { label: 'Srpski', value: 28 },
  { label: 'Svenska', value: 10 },
  { label: 'Tieng Viet', value: 13 },
  { label: 'Turkce', value: 6 },
  { label: 'Ellinika (requires language pack)', value: 9 },
  { label: 'Russkiy (requires language pack)', value: 17 },
  { label: 'Ukrayinska (requires language pack)', value: 31 },
  { label: 'Zhongwen (requires language pack)', value: 29 },
  { label: 'Nihongo (requires language pack)', value: 34 },
  { label: 'Hangugeo (requires language pack)', value: 35 },
  { label: 'Ivrit (requires language pack)', value: 36 },
];

// Alternate-clock timezone offsets. Mirrors config_common_options.html:127-164.
// Commented-out half-hour zones are intentionally omitted.
const ALT_CLOCK_OFFSETS: ClayOption[] = [
  { label: 'UTC-12:00 (Baker Island)', value: -12 },
  { label: 'UTC-11:00 (American Samoa)', value: -11 },
  { label: 'UTC-10:00 (Hawaii, Tahiti)', value: -10 },
  { label: 'UTC-09:00 (Alaska)', value: -9 },
  { label: 'UTC-08:00 (Los Angeles, Vancouver)', value: -8 },
  { label: 'UTC-07:00 (Denver, Phoenix)', value: -7 },
  { label: 'UTC-06:00 (Chicago, Mexico City)', value: -6 },
  { label: 'UTC-05:00 (New York, Bogota)', value: -5 },
  { label: 'UTC-04:00 (Santiago, Halifax)', value: -4 },
  { label: 'UTC-03:00 (Buenos Aires, Montevideo)', value: -3 },
  { label: 'UTC-02:00 (South Georgia)', value: -2 },
  { label: 'UTC-01:00 (Azores, Cape Verde)', value: -1 },
  { label: 'UTC+00:00 (London, Reykjavik)', value: 0 },
  { label: 'UTC+01:00 (Berlin, Lagos, Paris)', value: 1 },
  { label: 'UTC+02:00 (Cairo, Johannesburg)', value: 2 },
  { label: 'UTC+03:00 (Moscow, Riyadh)', value: 3 },
  { label: 'UTC+04:00 (Dubai, Baku)', value: 4 },
  { label: 'UTC+05:00 (Karachi, Tashkent)', value: 5 },
  { label: 'UTC+06:00 (Dhaka, Almaty)', value: 6 },
  { label: 'UTC+07:00 (Bangkok, Jakarta)', value: 7 },
  { label: 'UTC+08:00 (Beijing, Perth, Singapore)', value: 8 },
  { label: 'UTC+09:00 (Tokyo, Seoul)', value: 9 },
  { label: 'UTC+10:00 (Sydney, Vladivostok)', value: 10 },
  { label: 'UTC+11:00 (Solomon Islands, Magadan)', value: 11 },
  { label: 'UTC+12:00 (Auckland, Fiji)', value: 12 },
  { label: 'UTC+13:00 (Tonga, Phoenix Islands)', value: 13 },
  { label: 'UTC+14:00 (Kiritimati, Line Islands)', value: 14 },
];

const config = [
  // ------------------------------------------------------------------ Clock
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Clock' },
      { type: 'radiogroup', messageKey: 'SettingClockStyle', label: 'Clock style',
        defaultValue: '0', options: [{ label: 'Digital', value: '0' }, { label: 'Analog', value: '1' }] },
      { type: 'radiogroup', messageKey: 'SettingAnalogTicks', label: 'Analog hour marks',
        defaultValue: '2', options: [
          { label: 'Hide', value: '0' },
          { label: 'Normal', value: '1' },
          { label: 'Bold', value: '2' },
        ] },
      { type: 'toggle', messageKey: 'SettingAnalogDigitalClock',
        label: 'Digital time below clock', defaultValue: false },
      { type: 'text', id: 'analog-credit', defaultValue:
        'The <b>Analog</b> clock face is ported from the <b>Nyquist</b> watchface ' +
        'by truhanen, used under the GPL-3.0 license.<br>' +
        'github.com/truhanen/pebble-nyquist-watchface' },
      { type: 'radiogroup', messageKey: 'SettingShowLeadingZero', label: 'Leading zero',
        defaultValue: '0', options: [{ label: 'No', value: '0' }, { label: 'Yes', value: '1' }] },
      { type: 'radiogroup', messageKey: 'SettingClockFontId', label: 'Clock font',
        defaultValue: '1', options: [
          { label: 'Default', value: '0' },
          { label: 'LECO', value: '1' },
          { label: 'Bold', value: '2' },
          { label: 'Bold Hour', value: '3' },
          { label: 'Bold Minute', value: '4' },
        ] },
      { type: 'radiogroup', messageKey: 'SettingBigDate', label: 'Large date above clock',
        defaultValue: '1', options: [{ label: 'None', value: '0' }, { label: 'Show', value: '1' }] },
      { type: 'radiogroup', messageKey: 'SettingBigDateMonth', label: 'Show month in large date',
        defaultValue: '1', options: [{ label: 'Hide', value: '0' }, { label: 'Show', value: '1' }] },
      { type: 'radiogroup', messageKey: 'SettingBigDateFont', label: 'Large date font',
        defaultValue: '0', options: [
          { label: 'Bold (Bitham)', value: '0' },
          { label: 'System (Gothic)', value: '1' },
          { label: 'Serif', value: '2' },
        ] },
      { type: 'radiogroup', messageKey: 'SettingHourlyVibe', label: 'Periodic vibration',
        defaultValue: '0', options: [
          { label: 'No vibration', value: '0' },
          { label: 'Every 30 minutes', value: '2' },
          { label: 'Every hour', value: '1' },
        ] },
    ],
  },

  // ---------------------------------------------------------------- Colors
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Colors' },
      { type: 'color', messageKey: 'SettingColorTime', label: 'Time text color',
        defaultValue: '0x000000', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingColorBG', label: 'Background color',
        defaultValue: '0xFFFFFF', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingDateBgColor', label: 'Date background',
        defaultValue: '0xAAFFAA', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingTwtStatusBgColor', label: 'Status area background',
        defaultValue: '0xAAFFAA', sunlight: false, allowGray: true },
      // NOTE: the SettingSidebarBgColor{Left,Right} messageKeys are repurposed as
      // PRIMARY / SECONDARY sidebar backgrounds (role-based, not physical side).
      // The keys keep their legacy "Left"/"Right" names so the positional
      // AppMessage IDs don't drift (see CLAUDE.md). The watch C side maps
      // Left=primary, Right=secondary regardless of which side the primary is on.
      { type: 'color', messageKey: 'SettingSidebarBgColorLeft', label: 'Primary sidebar background',
        defaultValue: '0xAAFFAA', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingSidebarBgColorRight', label: 'Secondary sidebar background',
        defaultValue: '0xAAFFAA', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingSidebarTextColor', label: 'Sidebar text color',
        defaultValue: '0x000000', sunlight: false, allowGray: true },
    ],
  },

  // --------------------------------------------------------------- Widgets
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Sidebar widgets' },
      { type: 'toggle', messageKey: 'SettingStatusStripFullWidth',
        label: 'Status strip full width (shorten widget columns)', defaultValue: false },
      { type: 'text', id: 'widgets-help', defaultValue:
        'Each row is one widget. Pick the widget from the dropdown, reorder with ' +
        '▲ / ▼, remove with ✕, and add more with “+ Add widget”.<br><br>' +
        'The <b>◉ / ⊘</b> button after the dropdown shows or hides that ' +
        'widget’s <b>identifier</b> — the icon (battery, weather, steps, heart ' +
        'rate, …) or the short title label (UV, week number, electricity, a coin ' +
        'code, …). <b>◉</b> = shown (default); <b>⊘</b> = hidden, leaving ' +
        'only the value and making the widget shorter so you can fit more into a ' +
        'column.<br>' +
        '(Widgets with no separate identifier — Empty, Today’s Date, Seconds, ' +
        'Bluetooth disconnect — have no such button.)' },
      { type: 'heading', defaultValue: 'Left sidebar widgets' },
      { type: 'widgetList', messageKey: 'WidgetList', defaultValue: [12, 15, 17] },
      { type: 'heading', defaultValue: 'Right sidebar widgets' },
      { type: 'widgetList', messageKey: 'WidgetListRight', defaultValue: [] },
      { type: 'radiogroup', messageKey: 'SettingUseLargeFonts', label: 'Sidebar font size',
        defaultValue: '0', options: [{ label: 'Large', value: '1' }, { label: 'Normal', value: '0' }] },
      { type: 'radiogroup', messageKey: 'SettingShowBatteryPct', label: 'Battery meter style',
        defaultValue: '1', options: [{ label: 'Icon & percent', value: '1' }, { label: 'Icon only', value: '0' }] },
      { type: 'radiogroup', messageKey: 'SettingDisableAutobattery', label: 'Auto-show battery meter',
        defaultValue: '0', options: [{ label: 'On', value: '0' }, { label: 'Off', value: '1' }] },
      { type: 'select', messageKey: 'SettingAutoBatteryThreshold', label: 'Auto-show below battery %',
        defaultValue: '10', options: [
          { label: '5 %', value: '5' }, { label: '10 %', value: '10' }, { label: '15 %', value: '15' },
          { label: '20 %', value: '20' }, { label: '25 %', value: '25' }, { label: '30 %', value: '30' },
          { label: '40 %', value: '40' }, { label: '50 %', value: '50' },
        ] },
      { type: 'radiogroup', messageKey: 'SettingFallbackColumn',
        label: 'Battery / disconnect icon column', defaultValue: '0', options: [
          { label: 'Automatic', value: '0' }, { label: 'Left bar', value: '1' }, { label: 'Right bar', value: '2' },
        ] },
      { type: 'input', messageKey: 'SettingFallbackPosition', label: 'Position in that bar',
        attributes: { type: 'number', min: 1, max: 17, step: 1, placeholder: '1' }, defaultValue: '1' },
      { type: 'text', id: 'fallback-help', defaultValue:
        'Position N replaces the Nth widget in the chosen bar. A position one past ' +
        'the last widget adds the battery / disconnect icon as an extra at the bottom ' +
        'instead of replacing one.' },
    ],
  },

  // --------------------------------------------------------------- Weather
  {
    type: 'section',
    items: [
      { type: 'heading', id: 'heading-weather', defaultValue: 'Weather' },
      { type: 'radiogroup', messageKey: 'SettingUseMetric', label: 'Temperature units',
        defaultValue: '1', options: [{ label: 'Celsius', value: '1' }, { label: 'Fahrenheit', value: '0' }] },
      { type: 'radiogroup', messageKey: 'weather_loc_mode', label: 'Location',
        defaultValue: 'auto', options: [{ label: 'Automatic (GPS)', value: 'auto' }, { label: 'Manual', value: 'manual' }] },
      { type: 'input', messageKey: 'weather_loc', label: 'Manual location label',
        attributes: { placeholder: 'e.g. Helsinki' }, defaultValue: '' },
      { type: 'input', messageKey: 'weather_loc_lat', label: 'Latitude',
        attributes: { type: 'number', step: 0.0001, placeholder: '60.1699' }, defaultValue: '' },
      { type: 'input', messageKey: 'weather_loc_lng', label: 'Longitude',
        attributes: { type: 'number', step: 0.0001, placeholder: '24.9384' }, defaultValue: '' },
      { type: 'radiogroup', messageKey: 'weather_datasource', label: 'Data source',
        defaultValue: 'openmeteo', options: [
          { label: 'Open-Meteo', value: 'openmeteo' },
          { label: 'FMI (Ilmatieteen laitos)', value: 'fmi' },
        ] },
    ],
  },

  // ----------------------------------------------------------- Electricity
  {
    type: 'section',
    items: [
      { type: 'heading', id: 'heading-electricity', defaultValue: 'Electricity' },
      { type: 'input', messageKey: 'SettingElecQuietStart', label: 'Quiet hours start (0-23)',
        attributes: { type: 'number', min: 0, max: 23, placeholder: '23' }, defaultValue: '23' },
      { type: 'input', messageKey: 'SettingElecQuietEnd', label: 'Quiet hours end (0-23)',
        attributes: { type: 'number', min: 0, max: 23, placeholder: '7' }, defaultValue: '7' },
      { type: 'text', id: 'elec-cheap-help', defaultValue:
        'The “Next cheap electricity” and “Cheapest electricity hour” widgets ' +
        'find upcoming low-price hours. An hour counts as ' +
        '<b>cheap</b> when its price is below a threshold worked out from the ' +
        'three settings below:<br>' +
        'threshold = <b>average</b> upcoming price × <b>Cheap factor</b>, then ' +
        'kept within <b>Cheap floor … Cheap ceiling</b>.<br><br>' +
        '<b>Cheap factor</b> — the threshold as a percentage of the average ' +
        'upcoming price (quiet hours excluded). 70 % means “below 70 % of the ' +
        'average”. Lower = stricter (fewer hours qualify), higher = more ' +
        'lenient.<br>' +
        '<b>Cheap floor</b> (snt/kWh) — the lowest the threshold can ever go. ' +
        'When prices are low across the board this stops the threshold ' +
        'collapsing, so a genuinely cheap hour is still found instead of ' +
        'none.<br>' +
        '<b>Cheap ceiling</b> (snt/kWh) — the highest the threshold can ever go. ' +
        'During an expensive period nothing above the ceiling is called cheap, so ' +
        'the widget honestly shows “--” rather than a merely “less bad” ' +
        'hour.<br><br>' +
        'Examples with factor 70 %, floor 2, ceiling 8: average 2 → threshold ' +
        '2.0 · average 10 → 7.0 · average 30 → 8.0 (so usually nothing ' +
        'qualifies).' },
      { type: 'input', messageKey: 'SettingElecCheapFactorPct', label: 'Cheap factor (% of average)',
        attributes: { type: 'number', min: 1, max: 100, placeholder: '70' }, defaultValue: '70' },
      { type: 'input', messageKey: 'elec_cheap_floor', label: 'Cheap floor (snt/kWh)',
        attributes: { type: 'number', step: 0.1, placeholder: '2.0' }, defaultValue: '2.0' },
      { type: 'input', messageKey: 'elec_cheap_ceiling', label: 'Cheap ceiling (snt/kWh)',
        attributes: { type: 'number', step: 0.1, placeholder: '8.0' }, defaultValue: '8.0' },
    ],
  },

  // --------------------------------------------------------------- Crypto
  {
    type: 'section',
    items: [
      { type: 'heading', id: 'heading-crypto', defaultValue: 'Crypto' },
      { type: 'text', defaultValue:
        'Prices load from CoinGecko; the Data Refresh interval (bottom of this page) ' +
        'sets how often. Add coins, then place each in the sidebar widget list above. ' +
        'Precision: positive = decimals (2 → 104000.00), negative = round (−3 → 104). ' +
        'Trim: cut N leading digits (1.160 with trim 2 → 60).' },
      { type: 'cryptoList', messageKey: 'CryptoList', defaultValue: [] },
    ],
  },

  // ------------------------------------------------------------- Currency
  {
    type: 'section',
    items: [
      { type: 'heading', id: 'heading-currency', defaultValue: 'Currency pairs' },
      { type: 'text', defaultValue:
        'Live fiat exchange rates from ExchangeRate-API (open access, no key). ' +
        'Add pairs (base / quote), then place each in the sidebar widget list above. ' +
        'Precision: positive = decimals (EUR/USD 4 → 1.0823), negative = round. ' +
        'Trim: cut N leading digits (1.160, trim 2 → 60). ' +
        'Leave the label empty to show the quote currency (e.g. USD); short labels fit the narrow sidebar best.' },
      { type: 'currencyList', messageKey: 'CurrencyList', defaultValue: [] },
    ],
  },

  // ------------------------------------------------------------- Work time
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Work time' },
      { type: 'radiogroup', messageKey: 'SettingTwtShowRemaining', label: 'Work time display',
        defaultValue: '0', options: [{ label: 'Worked', value: '0' }, { label: 'Remaining', value: '1' }] },
      { type: 'radiogroup', messageKey: 'SettingTwtTargetVibe', label: 'Vibrate and flash when daily target reached',
        defaultValue: '0', options: [{ label: 'None', value: '0' }, { label: 'Vibrate', value: '1' }] },
      { type: 'radiogroup', messageKey: 'SettingTwtBudgetVibe', label: 'Vibrate and flash when budget target reached',
        defaultValue: '0', options: [{ label: 'None', value: '0' }, { label: 'Vibrate', value: '1' }] },
      { type: 'color', messageKey: 'SettingTwtFlashColor', label: 'Status flash color',
        defaultValue: '0xFF0000', sunlight: false, allowGray: true },
    ],
  },

  // ----------------------------------------------------------- MIDI recorder
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'MIDI recorder' },
      { type: 'radiogroup', messageKey: 'SettingMidiVibe', label: 'Vibrate on recording start/stop',
        defaultValue: '0', options: [{ label: 'Vibrate', value: '1' }, { label: 'None', value: '0' }] },
      { type: 'radiogroup', messageKey: 'SettingMidiSecondPrecision', label: 'Show recording timer',
        defaultValue: '0', options: [{ label: 'Off', value: '0' }, { label: 'Seconds', value: '1' }] },
    ],
  },

  // ------------------------------------------------------------- Bluetooth
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Bluetooth' },
      { type: 'radiogroup', messageKey: 'SettingDisconnectIcon', label: 'Disconnect icon',
        defaultValue: '1', options: [{ label: 'Show', value: '1' }, { label: 'Hide', value: '0' }] },
      { type: 'radiogroup', messageKey: 'SettingBluetoothVibe', label: 'Disconnect vibration',
        defaultValue: '0', options: [{ label: 'Vibrate', value: '1' }, { label: 'None', value: '0' }] },
    ],
  },

  // -------------------------------------------------------------- Regional
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Regional' },
      { type: 'select', messageKey: 'SettingLanguageID', label: 'Language', defaultValue: 0, options: LANGUAGES },
      { type: 'input', messageKey: 'SettingAltClockName', label: 'Alternate clock name (max 4)',
        attributes: { maxlength: 4, placeholder: 'ALT' }, defaultValue: 'ALT' },
      { type: 'select', messageKey: 'SettingAltClockOffset', label: 'Alternate clock time zone',
        defaultValue: 0, options: ALT_CLOCK_OFFSETS },
      { type: 'radiogroup', messageKey: 'SettingDecimalSep', label: 'Decimal separator',
        defaultValue: '.', options: [{ label: '.', value: '.' }, { label: ',', value: ',' }] },
    ],
  },

  // ---------------------------------------------------------- Data Refresh
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Data Refresh' },
      { type: 'text', defaultValue:
        'How often the watch asks the phone for fresh online data: weather, ' +
        'electricity (spot prices) and crypto / currency prices. Lower = more ' +
        'up-to-date but more battery/Bluetooth use. (Electricity self-limits to ' +
        '~2 fetches/day; crypto is sent only when a displayed price changes.)' },
      { type: 'input', messageKey: 'SettingPollIntervalMin', label: 'Data refresh interval (min)',
        attributes: { type: 'number', min: 5, max: 240, step: 1, placeholder: '30' }, defaultValue: '30' },
    ],
  },

  { type: 'submit', defaultValue: 'Save' },
];

export = config;
