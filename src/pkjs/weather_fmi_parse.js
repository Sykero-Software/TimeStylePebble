// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pure parser for FMI open-data WFS "simple feature" forecast XML.
   No Pebble/browser globals -- unit-testable with `node --test`.
   The XML is a flat list of <BsWfs:BsWfsElement> with consecutive
   <BsWfs:Time> / <BsWfs:ParameterName> / <BsWfs:ParameterValue>. */

var ELEMENT_RE =
  /<BsWfs:Time>([^<]+)<\/BsWfs:Time>\s*<BsWfs:ParameterName>([^<]+)<\/BsWfs:ParameterName>\s*<BsWfs:ParameterValue>([^<]+)<\/BsWfs:ParameterValue>/g;

// Higher rank = more "notable" weather; used to pick the day's forecast symbol.
function symbolRank(base) {
  if (base >= 71) { return 9; }                                  // thundershowers
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

function parseFmiForecast(xml, nowEpochSec) {
  if (typeof xml !== 'string' || xml.indexOf('ExceptionReport') !== -1) {
    return { ok: false };
  }

  var temps = [], syms = [], uvs = [];
  var m;
  ELEMENT_RE.lastIndex = 0;
  while ((m = ELEMENT_RE.exec(xml)) !== null) {
    var epoch = Math.floor(Date.parse(m[1]) / 1000);
    var value = parseFloat(m[3]);
    if (isNaN(epoch) || isNaN(value)) { continue; }
    if (m[2] === 'temperature') { temps.push({ epoch: epoch, value: value }); }
    else if (m[2] === 'smartsymbol') { syms.push({ epoch: epoch, value: value }); }
    else if (m[2] === 'uvindex') { uvs.push({ epoch: epoch, value: value }); }
  }

  if (temps.length === 0) { return { ok: false }; }

  function nearest(series) {
    var best = null, bestDiff = Infinity;
    for (var i = 0; i < series.length; i++) {
      var d = Math.abs(series[i].epoch - nowEpochSec);
      if (d < bestDiff) { bestDiff = d; best = series[i]; }
    }
    return best;
  }

  var curTemp = nearest(temps);
  var curSym = syms.length ? nearest(syms) : null;
  var curUv = uvs.length ? nearest(uvs) : null;

  var high = temps[0].value, low = temps[0].value;
  for (var i = 1; i < temps.length; i++) {
    if (temps[i].value > high) { high = temps[i].value; }
    if (temps[i].value < low) { low = temps[i].value; }
  }

  // Forecast condition = the day's most notable weather, as a DAY-base code
  // (strip the +100 night offset so the icon mapping yields a daytime icon).
  var fcSym = null, fcRank = -1;
  for (var j = 0; j < syms.length; j++) {
    var base = syms[j].value % 100;
    var r = symbolRank(base);
    if (r > fcRank) { fcRank = r; fcSym = base; }
  }

  return {
    ok: true,
    currentTemp: Math.round(curTemp.value),
    currentSymbol: curSym ? curSym.value : null,
    forecastSymbol: fcSym,
    forecastHigh: Math.round(high),
    forecastLow: Math.round(low),
    uvIndex: curUv ? Math.round(curUv.value) : 0
  };
}

// Map an FMI smartsymbol code to a TimeStyle WeatherIcons value. `icons` is the
// WeatherIcons map (injected so this stays pure/testable). Night = code >= 100.
function smartSymbolToIcon(code, icons) {
  if (typeof code !== 'number' || isNaN(code)) { return icons.WEATHER_GENERIC; }
  var night = code >= 100;
  var base = code % 100;

  switch (base) {
    case 1: case 2:
      return night ? icons.CLEAR_NIGHT : icons.CLEAR_DAY;
    case 4: case 6:
      return night ? icons.PARTLY_CLOUDY_NIGHT : icons.PARTLY_CLOUDY;
    case 7:
    case 9:
      return icons.CLOUDY_DAY;
  }
  if (base >= 71 && base <= 77) { return icons.THUNDERSTORM; }               // 71-77 thunder
  if (base >= 61 && base <= 67) { return icons.RAINING_AND_SNOWING; }        // 61-67 hail (no hail icon)
  if (base >= 51 && base <= 59) { return base >= 56 ? icons.HEAVY_SNOW : icons.LIGHT_SNOW; } // 51-59
  if (base >= 41 && base <= 49) { return icons.RAINING_AND_SNOWING; }        // 41-49 sleet
  if (base === 33 || base === 36 || base === 39) { return icons.HEAVY_RAIN; }
  if (base >= 11 && base <= 39) { return icons.LIGHT_RAIN; }                 // 11-39 (minus heavy) rain/drizzle/showers
  return icons.WEATHER_GENERIC;
}

module.exports.parseFmiForecast = parseFmiForecast;
module.exports.smartSymbolToIcon = smartSymbolToIcon;
module.exports.symbolRank = symbolRank;
