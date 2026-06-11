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

// Widget slot options (id per slot). Mirrors docs/_includes/widget_selector.html.
// 19 real options including Empty=0. Extracted once and reused for all 6 slots.
const WIDGETS: ClayOption[] = [
  { label: 'Empty', value: 0 },
  // Time & Date
  { label: 'Alternate Time Zone', value: 3 },
  { label: 'Seconds', value: 5 },
  { label: 'Swatch Internet Time', value: 11 },
  { label: "Today's Date", value: 4 },
  { label: 'Week Number', value: 6 },
  // Weather
  { label: 'Current Weather', value: 7 },
  { label: "Today's Forecast", value: 8 },
  { label: 'UV Index', value: 13 },
  // Sahko (electricity)
  { label: 'Porssisahko', value: 14 },
  { label: 'Seuraava halpa sahko', value: 18 },
  { label: 'Halvin sahkotunti', value: 19 },
  // Crypto
  { label: 'Bitcoin (BTC)', value: 15 },
  { label: 'Monero (XMR)', value: 16 },
  // Currency
  { label: 'EUR/USD rate', value: 17 },
  // Health
  { label: 'Sleep', value: 9 },
  { label: 'Steps', value: 10 },
  { label: 'Heart Rate', value: 12 },
  // Status
  { label: 'Battery', value: 2 },
];

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
  // ---------------------------------------------------------------- Colors
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Colors' },
      { type: 'color', messageKey: 'SettingColorTime', label: 'Time text color',
        defaultValue: '0x000000', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingColorBG', label: 'Background color',
        defaultValue: '0xFFFFFF', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingColorSidebar', label: 'Sidebar color',
        defaultValue: '0xAAFFAA', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingSidebarTextColor', label: 'Sidebar text color',
        defaultValue: '0x000000', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingTwtStatusBgColor', label: 'Work-time status background',
        defaultValue: '0xAAFFAA', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingDateBgColor', label: 'Date background',
        defaultValue: '0xAAFFAA', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingSidebarBgColorLeft', label: 'Left sidebar widget-area background',
        defaultValue: '0xAAFFAA', sunlight: false, allowGray: true },
      { type: 'color', messageKey: 'SettingSidebarBgColorRight', label: 'Right sidebar widget-area background',
        defaultValue: '0xAAFFAA', sunlight: false, allowGray: true },
    ],
  },

  // --------------------------------------------------------------- Widgets
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Widgets' },
      { type: 'select', messageKey: 'SettingWidget0ID', label: 'Widget slot 1', defaultValue: 12, options: WIDGETS },
      { type: 'select', messageKey: 'SettingWidget1ID', label: 'Widget slot 2', defaultValue: 15, options: WIDGETS },
      { type: 'select', messageKey: 'SettingWidget2ID', label: 'Widget slot 3', defaultValue: 17, options: WIDGETS },
      { type: 'select', messageKey: 'SettingWidget2_0ID', label: 'Widget slot 4', defaultValue: 0, options: WIDGETS },
      { type: 'select', messageKey: 'SettingWidget2_1ID', label: 'Widget slot 5', defaultValue: 0, options: WIDGETS },
      { type: 'select', messageKey: 'SettingWidget2_2ID', label: 'Widget slot 6', defaultValue: 0, options: WIDGETS },
      { type: 'radiogroup', messageKey: 'SettingSecondaryAlwaysOn', label: 'Show secondary widgets',
        defaultValue: '1', options: [{ label: 'Always', value: '1' }, { label: 'With status only', value: '0' }] },
      { type: 'radiogroup', messageKey: 'SettingSidebarOnLeft', label: 'Sidebar position',
        defaultValue: '1', options: [{ label: 'Left', value: '1' }, { label: 'Right', value: '0' }] },
      { type: 'radiogroup', messageKey: 'SettingUseLargeFonts', label: 'Sidebar font size',
        defaultValue: '0', options: [{ label: 'Large', value: '1' }, { label: 'Normal', value: '0' }] },
      { type: 'radiogroup', messageKey: 'SettingShowBatteryPct', label: 'Battery meter style',
        defaultValue: '1', options: [{ label: 'Icon & percent', value: '1' }, { label: 'Icon only', value: '0' }] },
      { type: 'radiogroup', messageKey: 'SettingDisableAutobattery', label: 'Automatic battery meter (at 10%)',
        defaultValue: '0', options: [{ label: 'Automatic (on)', value: '0' }, { label: 'Never (off)', value: '1' }] },
      { type: 'radiogroup', messageKey: 'SettingHealthUseDistance', label: 'Steps widget displays',
        defaultValue: '0', options: [{ label: 'Number of steps', value: '0' }, { label: 'Distance', value: '1' }] },
      { type: 'radiogroup', messageKey: 'SettingHealthUseRestfulSleep', label: 'Sleep widget uses',
        defaultValue: '0', options: [{ label: 'Total Sleep', value: '0' }, { label: 'Restful Sleep', value: '1' }] },
    ],
  },

  // ---------------------------------------------------------- Time Display
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Time Display' },
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
      { type: 'radiogroup', messageKey: 'SettingTwtShowRemaining', label: 'Work time display',
        defaultValue: '0', options: [{ label: 'Worked', value: '0' }, { label: 'Remaining', value: '1' }] },
      { type: 'radiogroup', messageKey: 'SettingTwtTargetVibe', label: 'Vibrate when daily target reached',
        defaultValue: '0', options: [{ label: 'None', value: '0' }, { label: 'Vibrate', value: '1' }] },
    ],
  },

  // --------------------------------------------------------------- Weather
  {
    type: 'section',
    items: [
      { type: 'heading', id: 'heading-weather', defaultValue: 'Weather' },
      { type: 'radiogroup', messageKey: 'SettingUseMetric', label: 'Units',
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

  // ------------------------------------------------------------------ MIDI
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'MIDI' },
      { type: 'radiogroup', messageKey: 'SettingMidiVibe', label: 'MIDI recording start/stop',
        defaultValue: '0', options: [{ label: 'Vibrate', value: '1' }, { label: 'None', value: '0' }] },
    ],
  },

  // ---------------------------------------------------------- Notification
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Notification' },
      { type: 'radiogroup', messageKey: 'SettingHourlyVibe', label: 'Vibration on interval',
        defaultValue: '0', options: [
          { label: 'No vibration', value: '0' },
          { label: 'Every 30 minutes', value: '2' },
          { label: 'Every hour', value: '1' },
        ] },
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

  // ----------------------------------------------------------- Electricity
  {
    type: 'section',
    items: [
      { type: 'heading', id: 'heading-electricity', defaultValue: 'Electricity' },
      { type: 'input', messageKey: 'SettingElecQuietStart', label: 'Quiet hours start (0-23)',
        attributes: { type: 'number', min: 0, max: 23, placeholder: '23' }, defaultValue: '23' },
      { type: 'input', messageKey: 'SettingElecQuietEnd', label: 'Quiet hours end (0-23)',
        attributes: { type: 'number', min: 0, max: 23, placeholder: '7' }, defaultValue: '7' },
      { type: 'input', messageKey: 'SettingElecCheapFactorPct', label: 'Cheap factor (% of average)',
        attributes: { type: 'number', min: 1, max: 100, placeholder: '70' }, defaultValue: '70' },
      { type: 'input', messageKey: 'elec_cheap_floor', label: 'Cheap floor (snt/kWh)',
        attributes: { type: 'number', step: 0.1, placeholder: '2.0' }, defaultValue: '2.0' },
      { type: 'input', messageKey: 'elec_cheap_ceiling', label: 'Cheap ceiling (snt/kWh)',
        attributes: { type: 'number', step: 0.1, placeholder: '8.0' }, defaultValue: '8.0' },
    ],
  },

  // ---------------------------------------------------------- Data Refresh
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Data Refresh' },
      { type: 'input', messageKey: 'SettingPollIntervalMin', label: 'Data refresh interval (min)',
        attributes: { type: 'number', min: 5, max: 240, step: 1, placeholder: '30' }, defaultValue: '30' },
    ],
  },

  { type: 'submit', defaultValue: 'Save' },
];

export = config;
