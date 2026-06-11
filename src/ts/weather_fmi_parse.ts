// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pure parser for FMI open-data WFS "simple feature" forecast XML.
   No Pebble/browser globals -- unit-testable with `node --test`.
   The XML is a flat list of <BsWfs:BsWfsElement> with consecutive
   <BsWfs:Time> / <BsWfs:ParameterName> / <BsWfs:ParameterValue>. */

const ELEMENT_RE =
  /<BsWfs:Time>([^<]+)<\/BsWfs:Time>\s*<BsWfs:ParameterName>([^<]+)<\/BsWfs:ParameterName>\s*<BsWfs:ParameterValue>([^<]+)<\/BsWfs:ParameterValue>/g;

interface Sample {
  epoch: number;
  value: number;
}

// The WeatherIcons map (injected so this stays pure/testable).
export type WeatherIcons = Record<string, number>;

export interface FmiForecast {
  ok: true;
  currentTemp: number;
  currentSymbol: number | null;
  forecastSymbol: number | null;
  forecastHigh: number;
  forecastLow: number;
  uvIndex: number;
}

export type FmiForecastResult = FmiForecast | { ok: false };

// Higher rank = more "notable" weather; used to pick the day's forecast symbol.
export function symbolRank(base: number): number {
  if (base >= 71 && base <= 77) { return 9; }                    // thundershowers
  if (base >= 61) { return 8; }                                  // hail showers
  if (base >= 51) { return 7; }                                  // snow
  if (base >= 41) { return 7; }                                  // sleet
  if (base === 33 || base === 36 || base === 39) { return 8; }   // heavy rain
  if (base >= 31) { return 6; }                                  // light/moderate rain
  if (base >= 11) { return 5; }                                  // drizzle/freezing/showers
  if (base === 9) { return 3; }                                  // fog
  if (base === 7) { return 2; }                                  // overcast
  if (base === 4 || base === 6) { return 1; }                    // partly/mostly cloudy
  return 0;                                                      // clear
}

function nearest(series: Sample[], nowEpochSec: number): Sample | null {
  let best: Sample | null = null;
  let bestDiff = Infinity;
  for (const s of series) {
    const d = Math.abs(s.epoch - nowEpochSec);
    if (d < bestDiff) { bestDiff = d; best = s; }
  }
  return best;
}

export function parseFmiForecast(xml: unknown, nowEpochSec: number): FmiForecastResult {
  if (typeof xml !== 'string' || xml.indexOf('ExceptionReport') !== -1) {
    return { ok: false };
  }

  const temps: Sample[] = [];
  const syms: Sample[] = [];
  const uvs: Sample[] = [];
  let m: RegExpExecArray | null;
  ELEMENT_RE.lastIndex = 0;
  while ((m = ELEMENT_RE.exec(xml)) !== null) {
    const epoch = Math.floor(Date.parse(m[1]) / 1000);
    const value = parseFloat(m[3]);
    if (isNaN(epoch) || isNaN(value)) { continue; }
    if (m[2] === 'temperature') { temps.push({ epoch, value }); }
    else if (m[2] === 'smartsymbol') { syms.push({ epoch, value }); }
    else if (m[2] === 'uvindex') { uvs.push({ epoch, value }); }
  }

  if (temps.length === 0) { return { ok: false }; }

  const curTemp = nearest(temps, nowEpochSec)!;
  const curSym = syms.length ? nearest(syms, nowEpochSec) : null;
  const curUv = uvs.length ? nearest(uvs, nowEpochSec) : null;

  let high = temps[0].value;
  let low = temps[0].value;
  for (let i = 1; i < temps.length; i++) {
    if (temps[i].value > high) { high = temps[i].value; }
    if (temps[i].value < low) { low = temps[i].value; }
  }

  // Forecast condition = the day's most notable weather, as a DAY-base code
  // (strip the +100 night offset so the icon mapping yields a daytime icon).
  let fcSym: number | null = null;
  let fcRank = -1;
  for (const s of syms) {
    const base = s.value % 100;
    const r = symbolRank(base);
    if (r > fcRank) { fcRank = r; fcSym = base; }
  }

  return {
    ok: true,
    currentTemp: Math.round(curTemp.value),
    currentSymbol: curSym ? Math.round(curSym.value) : null,
    forecastSymbol: fcSym,
    forecastHigh: Math.round(high),
    forecastLow: Math.round(low),
    uvIndex: curUv ? Math.round(curUv.value) : 0,
  };
}

// Map an FMI smartsymbol code to a TimeStyle WeatherIcons value. `icons` is the
// WeatherIcons map (injected so this stays pure/testable). Night = code >= 100.
export function smartSymbolToIcon(code: unknown, icons: WeatherIcons): number {
  if (typeof code !== 'number' || isNaN(code)) { return icons.WEATHER_GENERIC; }
  const night = code >= 100;
  const base = code % 100;

  switch (base) {
    case 1: case 2:
      return night ? icons.CLEAR_NIGHT : icons.CLEAR_DAY;
    case 4: case 6:
      return night ? icons.PARTLY_CLOUDY_NIGHT : icons.PARTLY_CLOUDY;
    case 7:
    case 9:
      return icons.CLOUDY_DAY;
  }
  // night variants exist only for clear/partly-cloudy (handled in the switch);
  // all precipitation/thunder conditions share a single day icon.
  if (base >= 71 && base <= 77) { return icons.THUNDERSTORM; }               // 71-77 thunder
  if (base >= 61 && base <= 67) { return icons.RAINING_AND_SNOWING; }        // 61-67 hail (no hail icon)
  if (base >= 51 && base <= 59) { return base >= 56 ? icons.HEAVY_SNOW : icons.LIGHT_SNOW; } // 51-59
  if (base >= 41 && base <= 49) { return icons.RAINING_AND_SNOWING; }        // 41-49 sleet
  if (base === 33 || base === 36 || base === 39) { return icons.HEAVY_RAIN; }
  if (base >= 11 && base <= 39) { return icons.LIGHT_RAIN; }                 // 11-39 (minus heavy) rain/drizzle/showers
  return icons.WEATHER_GENERIC;
}
