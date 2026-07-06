// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen
const test = require('node:test');
const assert = require('node:assert');
const {
  parseSpecCodes, shortUnit, buildCatalog, catalogScaleMap,
} = require('../src/pkjs/tuya_spec');

const SPEC = { status: [
  { code: 'va_temperature', type: 'Integer', values: '{"unit":"℃","min":-400,"max":1000,"scale":1,"step":1}' },
  { code: 'va_humidity', type: 'Integer', values: '{"unit":"%","min":0,"max":100,"scale":0,"step":1}' },
  { code: 'battery_percentage', type: 'Integer', values: '{"unit":"%","scale":0}' },
  { code: 'doorcontact_state', type: 'Boolean', values: '{}' },
]};

test('parseSpecCodes reads scale/unit from the values JSON', () => {
  const codes = parseSpecCodes(SPEC);
  const temp = codes.find((c) => c.code === 'va_temperature');
  assert.deepStrictEqual(temp, { code: 'va_temperature', type: 'Integer', scale: 1, unit: '℃' });
  const hum = codes.find((c) => c.code === 'va_humidity');
  assert.strictEqual(hum.scale, 0);
  assert.strictEqual(hum.unit, '%');
  const bool = codes.find((c) => c.code === 'doorcontact_state');
  assert.strictEqual(bool.scale, 0);   // no values -> scale 0
  assert.strictEqual(bool.unit, '');
});

test('parseSpecCodes tolerates missing/garbage spec', () => {
  assert.deepStrictEqual(parseSpecCodes(null), []);
  assert.deepStrictEqual(parseSpecCodes({}), []);
});

test('shortUnit maps temperature to ° and keeps %', () => {
  assert.strictEqual(shortUnit('℃'), '°');
  assert.strictEqual(shortUnit('°C'), '°');
  assert.strictEqual(shortUnit('℉'), '°');
  assert.strictEqual(shortUnit('%'), '%');
  assert.strictEqual(shortUnit('ppm'), '');
  assert.strictEqual(shortUnit(''), '');
});

test('buildCatalog groups codes under devices', () => {
  const cat = buildCatalog(
    [{ id: 'dev1', name: 'Sauna' }],
    { dev1: parseSpecCodes(SPEC) });
  assert.strictEqual(cat.v, 1);
  assert.strictEqual(cat.devices[0].name, 'Sauna');
  assert.strictEqual(cat.devices[0].codes.length, 4);
});

test('catalogScaleMap builds a deviceId->code->{scale,unit} lookup', () => {
  const cat = buildCatalog([{ id: 'dev1', name: 'Sauna' }], { dev1: parseSpecCodes(SPEC) });
  const m = catalogScaleMap(cat);
  assert.deepStrictEqual(m.dev1.va_temperature, { scale: 1, unit: '℃' });
  assert.strictEqual(m.dev1.va_humidity.scale, 0);
});
