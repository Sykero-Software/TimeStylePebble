// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Pure helpers for Tuya device /specification results: extract the reportable
   datapoint codes (with scale + unit), the config catalog (baked into the config
   page), and a scale/unit lookup for the poll driver. No Pebble/browser globals ->
   unit-tested in tests/tuya_spec.test.js. */

export interface SensorCode { code: string; type: string; scale: number; unit: string }
export interface CatalogDevice { id: string; name: string; codes: SensorCode[] }

function parseValues(values: any): { scale: number; unit: string } {
  try {
    const v = JSON.parse(values);
    const scale = (typeof v.scale === 'number') ? v.scale : 0;
    const unit = (typeof v.unit === 'string') ? v.unit : '';
    return { scale: scale, unit: unit };
  } catch (e) {
    return { scale: 0, unit: '' };
  }
}

// Reportable datapoints from a /specification result. Tuya lists read-only sensor
// dps under result.status; some devices also expose them under result.functions.
// status wins on a code collision.
export function parseSpecCodes(specResult: any): SensorCode[] {
  const byCode: Record<string, SensorCode> = {};
  const order: string[] = [];
  function add(entry: any) {
    if (!entry || typeof entry.code !== 'string') { return; }
    const pv = parseValues(entry.values);
    if (!(entry.code in byCode)) { order.push(entry.code); }
    byCode[entry.code] = { code: entry.code, type: String(entry.type || ''), scale: pv.scale, unit: pv.unit };
  }
  const fns = (specResult && specResult.functions) || [];
  for (let i = 0; i < fns.length; i++) { add(fns[i]); }
  const st = (specResult && specResult.status) || [];
  for (let i = 0; i < st.length; i++) { add(st[i]); }   // status overrides functions
  return order.map((c) => byCode[c]);
}

export function shortUnit(unit: string): string {
  const u = (typeof unit === 'string') ? unit : '';
  if (u === '℃' || u === '°C' || u === '℉' || u === '°F' || u === '°') { return '°'; }
  if (u === '%') { return '%'; }
  return '';
}

export function buildCatalog(devices: any[], codesById: Record<string, SensorCode[]>):
    { v: number; devices: CatalogDevice[] } {
  const arr: CatalogDevice[] = [];
  const list = Array.isArray(devices) ? devices : [];
  for (let i = 0; i < list.length; i++) {
    const d = list[i];
    if (!d || typeof d.id !== 'string') { continue; }
    arr.push({ id: d.id, name: String(d.name || d.id), codes: codesById[d.id] || [] });
  }
  return { v: 1, devices: arr };
}

export function catalogScaleMap(catalog: any):
    Record<string, Record<string, { scale: number; unit: string }>> {
  const out: Record<string, any> = {};
  const devs = (catalog && Array.isArray(catalog.devices)) ? catalog.devices : [];
  for (let i = 0; i < devs.length; i++) {
    const d = devs[i];
    if (!d || typeof d.id !== 'string') { continue; }
    const codes = Array.isArray(d.codes) ? d.codes : [];
    const cm: Record<string, any> = {};
    for (let j = 0; j < codes.length; j++) {
      const c = codes[j];
      if (c && typeof c.code === 'string') { cm[c.code] = { scale: c.scale || 0, unit: c.unit || '' }; }
    }
    out[d.id] = cm;
  }
  return out;
}
