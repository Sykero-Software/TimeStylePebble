// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pure parser for FMI open-data WFS observation "timevaluepair" XML
   (fmi::observations::weather::timevaluepair). No Pebble/browser globals --
   unit-testable with `node --test`. Each <wfs:member> is one station: a
   SF_SpatialSamplingFeature carrying the station name (gml:name with codeSpace
   ".../locationcode/name") and <gml:pos>lat lon</gml:pos>, followed by a
   MeasurementTimeseries of <wml2:time>/<wml2:value> pairs (value 'NaN' = gap). */

var NAME_RE = /<gml:name[^>]*locationcode\/name[^>]*>([^<]+)<\/gml:name>/;
var POS_RE = /<gml:pos>\s*([-\d.]+)\s+([-\d.]+)/;
var TVP_RE = /<wml2:time>([^<]+)<\/wml2:time>\s*<wml2:value>([^<]+)<\/wml2:value>/g;

// Parse the bbox response into [{name, lat, lon, temp, epoch}] -- one per
// station that has at least one usable (non-NaN) reading; temp is the latest
// such reading, rounded.
function parseFmiObservations(xml) {
  if (typeof xml !== 'string' || xml.indexOf('ExceptionReport') !== -1) { return []; }

  var members = xml.split('<wfs:member>');
  var out = [];
  for (var i = 1; i < members.length; i++) {        // [0] is the header chunk
    var chunk = members[i];
    var nm = NAME_RE.exec(chunk);
    var ps = POS_RE.exec(chunk);
    if (!nm || !ps) { continue; }

    var bestEpoch = -Infinity, bestVal = null, m;
    TVP_RE.lastIndex = 0;
    while ((m = TVP_RE.exec(chunk)) !== null) {
      var epoch = Math.floor(Date.parse(m[1]) / 1000);
      var val = parseFloat(m[2]);
      if (isNaN(epoch) || isNaN(val)) { continue; }
      if (epoch > bestEpoch) { bestEpoch = epoch; bestVal = val; }
    }
    if (bestVal === null) { continue; }

    out.push({
      name: nm[1],
      lat: parseFloat(ps[1]),
      lon: parseFloat(ps[2]),
      temp: Math.round(bestVal),
      epoch: bestEpoch
    });
  }
  return out;
}

// Great-circle distance proxy: we only compare distances, so the squared chord
// in equirectangular projection (cheap, no trig beyond one cos) is enough.
function approxDistSq(lat1, lon1, lat2, lon2) {
  var dLat = lat1 - lat2;
  var dLon = (lon1 - lon2) * Math.cos(((lat1 + lat2) / 2) * Math.PI / 180);
  return dLat * dLat + dLon * dLon;
}

// Nearest station to (lat, lon); null if the list is empty.
function pickNearestStation(stations, lat, lon) {
  var best = null, bestD = Infinity;
  for (var i = 0; i < stations.length; i++) {
    var d = approxDistSq(lat, lon, stations[i].lat, stations[i].lon);
    if (d < bestD) { bestD = d; best = stations[i]; }
  }
  return best;
}

// "Helsinki Kaisaniemi" -> "Kaisaniemi": drop the leading municipality token,
// then plain-truncate to maxLen (no ellipsis glyph -- avoids font tofu; the
// watch's text rect is sized so this fits).
function abbreviateStationName(name, maxLen) {
  if (typeof name !== 'string') { return ''; }
  var s = name.trim();
  var sp = s.indexOf(' ');
  if (sp > 0 && sp < s.length - 1) { s = s.substring(sp + 1); }
  if (maxLen && s.length > maxLen) { s = s.substring(0, maxLen); }
  return s;
}

module.exports.parseFmiObservations = parseFmiObservations;
module.exports.pickNearestStation = pickNearestStation;
module.exports.abbreviateStationName = abbreviateStationName;
