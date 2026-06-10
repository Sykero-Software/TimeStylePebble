// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

const test = require('node:test');
const assert = require('node:assert');
const {
  parseFmiObservations, pickNearestStation, abbreviateStationName
} = require('../src/pkjs/weather_fmi_obs_parse');

// One <wfs:member> per station, mirroring the real timevaluepair WML2 shape:
// a SF_SpatialSamplingFeature carrying the station name (codeSpace .../name)
// and <gml:pos>lat lon</gml:pos>, followed by a MeasurementTimeseries of
// <wml2:time>/<wml2:value> pairs.
function obsMember(name, lat, lon, tvps) {
  var pts = tvps.map(function (tv) {
    return '<wml2:point><wml2:MeasurementTVP>' +
      '<wml2:time>' + tv[0] + '</wml2:time>' +
      '<wml2:value>' + tv[1] + '</wml2:value>' +
      '</wml2:MeasurementTVP></wml2:point>';
  }).join('');
  return '<wfs:member><om:OM_Observation><om:featureOfInterest>' +
    '<sams:SF_SpatialSamplingFeature>' +
    '<sam:sampledFeature><target:LocationCollection>' +
    '<gml:identifier codeSpace="http://xml.fmi.fi/namespace/stationcode/fmisid">1</gml:identifier>' +
    '<gml:name codeSpace="http://xml.fmi.fi/namespace/locationcode/name">' + name + '</gml:name>' +
    '<gml:name codeSpace="http://xml.fmi.fi/namespace/locationcode/geoid">-1</gml:name>' +
    '</target:LocationCollection></sam:sampledFeature>' +
    '<gml:name>' + name + '</gml:name>' +
    '<gml:pos>' + lat + ' ' + lon + ' </gml:pos>' +
    '</sams:SF_SpatialSamplingFeature></om:featureOfInterest>' +
    '<wml2:MeasurementTimeseries>' + pts + '</wml2:MeasurementTimeseries>' +
    '</om:OM_Observation></wfs:member>';
}

const SAMPLE =
  '<wfs:FeatureCollection>' +
  obsMember('Helsinki Kaisaniemi', '60.17523', '24.94459', [
    ['2026-06-10T04:00:00Z', '12.6'],
    ['2026-06-10T04:30:00Z', '12.7'],
    ['2026-06-10T05:00:00Z', '13.1'],
    ['2026-06-10T05:30:00Z', 'NaN']   // latest reading missing -> ignored
  ]) +
  obsMember('Espoo Tapiola', '60.17797', '24.78743', [
    ['2026-06-10T05:00:00Z', '11.0']
  ]) +
  '</wfs:FeatureCollection>';

test('parseFmiObservations returns one entry per station with latest non-NaN temp', () => {
  const st = parseFmiObservations(SAMPLE);
  assert.strictEqual(st.length, 2);
  const kaisa = st[0];
  assert.strictEqual(kaisa.name, 'Helsinki Kaisaniemi');
  assert.strictEqual(kaisa.lat, 60.17523);
  assert.strictEqual(kaisa.lon, 24.94459);
  assert.strictEqual(kaisa.temp, 13);                       // 13.1 rounded, NaN skipped
  assert.strictEqual(kaisa.epoch, Math.floor(Date.parse('2026-06-10T05:00:00Z') / 1000));
  assert.strictEqual(st[1].name, 'Espoo Tapiola');
  assert.strictEqual(st[1].temp, 11);
});

test('parseFmiObservations drops stations with no usable reading; bad input -> []', () => {
  const none = '<wfs:FeatureCollection>' +
    obsMember('Nowhere Station', '60.0', '24.0', [['2026-06-10T05:00:00Z', 'NaN']]) +
    '</wfs:FeatureCollection>';
  assert.strictEqual(parseFmiObservations(none).length, 0);
  assert.strictEqual(parseFmiObservations('<ExceptionReport/>').length, 0);
  assert.strictEqual(parseFmiObservations(null).length, 0);
});

test('pickNearestStation chooses the closest station to the configured coords', () => {
  const st = parseFmiObservations(SAMPLE);
  // Query point right next to Kaisaniemi.
  const near = pickNearestStation(st, 60.175, 24.945);
  assert.strictEqual(near.name, 'Helsinki Kaisaniemi');
  // Query point closer to Tapiola.
  const far = pickNearestStation(st, 60.178, 24.79);
  assert.strictEqual(far.name, 'Espoo Tapiola');
  assert.strictEqual(pickNearestStation([], 60, 24), null);
});

test('abbreviateStationName drops the municipality and caps length', () => {
  assert.strictEqual(abbreviateStationName('Helsinki Kaisaniemi', 12), 'Kaisaniemi');
  assert.strictEqual(abbreviateStationName('Espoo Tapiola', 12), 'Tapiola');
  // long specific part is truncated to maxLen
  assert.strictEqual(abbreviateStationName('Vantaa Helsinki-Vantaan lentoasema', 12),
    'Helsinki-Van');
  // single-token name kept as-is
  assert.strictEqual(abbreviateStationName('Utsjoki', 12), 'Utsjoki');
  assert.strictEqual(abbreviateStationName('', 12), '');
  assert.strictEqual(abbreviateStationName(null, 12), '');
});
