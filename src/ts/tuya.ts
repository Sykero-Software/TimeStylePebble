// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Phone-side Tuya sensor driver. Reads the user's sensor rows (Clay key TuyaList in
   clay-settings), authenticates to the Tuya cloud (creds in TuyaAccessId/Secret/Region),
   and on each shared poll fetches device /status and pushes packed readings to the watch
   under the TuyaData key. Scale/unit come from the catalog discovered at config-open and
   cached in localStorage['tuya-catalog'] (no per-poll /specification call). Throttled far
   longer than crypto since sensor readings change slowly. Pure helpers live in
   tuya_client.ts / tuya_spec.ts / tuya_parse.ts (unit-tested). */

import { createClient } from './tuya_client';
import { parseSpecCodes, buildCatalog, catalogScaleMap } from './tuya_spec';
import {
  normalizeRows, distinctDevices, buildStatusPath, packTuyaData,
  countValidValues, parseLastSent, TuyaRow,
} from './tuya_parse';

const REGION_HOST: Record<string, string> = {
  eu: 'https://openapi.tuyaeu.com', us: 'https://openapi.tuyaus.com',
  cn: 'https://openapi.tuyacn.com', in: 'https://openapi.tuyain.com',
};

const LAST_SENT_KEY = 'tuya_last_sent';
const LAST_FETCH_KEY = 'tuya_last_fetch';
const CATALOG_KEY = 'tuya-catalog';
const TOKEN_KEY = 'tuya-token';
const MIN_FETCH_INTERVAL_S = 10 * 60;   // sensor readings change slowly

function readSettings(): any {
  try { return JSON.parse(window.localStorage.getItem('clay-settings') || '{}') || {}; }
  catch (e) { return {}; }
}

function readCfg(): any {
  const s = readSettings();
  const id = s.TuyaAccessId, secret = s.TuyaAccessSecret, region = s.TuyaRegion || 'eu';
  if (!id || !secret) { return null; }
  return { clientId: id, secret: secret, host: REGION_HOST[region] || REGION_HOST.eu };
}

function readTuyaRows(): TuyaRow[] {
  return normalizeRows(readSettings().TuyaList);
}

function loadCatalog(): any {
  try { return JSON.parse(window.localStorage.getItem(CATALOG_KEY) || 'null'); }
  catch (e) { return null; }
}
function saveCatalog(cat: any): void {
  try { window.localStorage.setItem(CATALOG_KEY, JSON.stringify(cat)); } catch (e) {}
}

// XMLHttpRequest -> parsed Tuya envelope (same wrapper as PebbleTuyaControl).
function http(opts: any): Promise<any> {
  return new Promise((resolve, reject) => {
    const xhr = new XMLHttpRequest();
    xhr.open(opts.method, opts.url, true);
    const h = opts.headers || {};
    Object.keys(h).forEach((k) => { xhr.setRequestHeader(k, h[k]); });
    xhr.onload = () => { try { resolve(JSON.parse(xhr.responseText)); } catch (e) { reject(new Error('Bad JSON from Tuya')); } };
    xhr.onerror = () => { reject(new Error('Network error')); };
    xhr.timeout = 15000;
    xhr.ontimeout = function () { reject(new Error('Timeout')); };
    xhr.send(opts.body || null);
  });
}

const deps = {
  now: () => Date.now(),
  nonce: () => 'xxxxxxxx'.replace(/x/g, () => Math.floor(Math.random() * 16).toString(16)),
  loadToken: (clientId: string) => {
    try { const o = JSON.parse(window.localStorage.getItem(TOKEN_KEY) || 'null'); return (o && o.clientId === clientId) ? o : null; }
    catch (e) { return null; }
  },
  saveToken: (clientId: string, v: any) => {
    try { window.localStorage.setItem(TOKEN_KEY, JSON.stringify({ clientId: clientId, token: v.token, expiresAt: v.expiresAt })); }
    catch (e) {}
  },
};

let _client: any = null, _clientKey: string | null = null;
function getClient(): any {
  const cfg = readCfg();
  if (!cfg) { return null; }
  const key = cfg.clientId + '|' + cfg.host;
  if (!_client || _clientKey !== key) { _client = createClient(cfg, http, deps); _clientKey = key; }
  return _client;
}

// Config-open: list devices + fetch each /specification, build the catalog, cache it,
// then run cb (best-effort: any failure / no creds still calls cb so config opens).
export function discoverCatalog(cb: () => void): void {
  const c = getClient();
  if (!c) { cb(); return; }
  let devices: any[] = [];
  const codesById: Record<string, any> = {};
  c.request('GET', '/v1.0/iot-01/associated-users/devices').then((resp: any) => {
    devices = (resp.result && resp.result.devices) || [];
    let chain: Promise<any> = Promise.resolve();
    devices.forEach((d: any) => {
      chain = chain.then(() =>
        c.request('GET', '/v1.0/iot-03/devices/' + d.id + '/specification').then((spec: any) => {
          codesById[d.id] = parseSpecCodes(spec.result);
        }).catch(() => { codesById[d.id] = []; }));
    });
    return chain;
  }).then(() => {
    saveCatalog(buildCatalog(devices, codesById));
    cb();
  }).catch(() => { cb(); });
}

export function updateTuya(forceUpdate?: boolean): void {
  if (window.localStorage.getItem('disable_tuya') === 'yes') { return; }
  const rows = readTuyaRows();
  if (rows.length === 0) { return; }

  const last = parseInt(window.localStorage.getItem(LAST_FETCH_KEY) || '0', 10);
  const now = Math.floor(Date.now() / 1000);
  if (!forceUpdate && (now - last) < MIN_FETCH_INTERVAL_S) {
    console.log('tuya: skipping fetch, last was ' + (now - last) + 's ago');
    return;
  }
  window.localStorage.setItem(LAST_FETCH_KEY, String(now));

  const c = getClient();
  if (!c) { console.log('tuya: no credentials'); return; }
  const scaleMap = catalogScaleMap(loadCatalog());
  const prev = parseLastSent(window.localStorage.getItem(LAST_SENT_KEY) || '');
  const devices = distinctDevices(rows);
  const valuesById: Record<string, any> = {};

  let chain: Promise<any> = Promise.resolve();
  devices.forEach((id) => {
    chain = chain.then(() =>
      c.request('GET', buildStatusPath(id)).then((stat: any) => {
        const arr = (stat && stat.result) || [];
        const m: Record<string, any> = {};
        for (let i = 0; i < arr.length; i++) { m[arr[i].code] = arr[i].value; }
        valuesById[id] = m;
      }).catch(() => { /* leave this device out -> prev/-- fallback */ }));
  });
  chain.then(() => {
    if (countValidValues(rows, valuesById) === 0) { console.log('tuya: no valid readings, keeping last'); return; }
    const packed = packTuyaData(rows, valuesById, scaleMap, prev);
    const lastSent = window.localStorage.getItem(LAST_SENT_KEY);
    if (!forceUpdate && lastSent === packed) { console.log('tuya: nothing changed'); return; }
    Pebble.sendAppMessage({ TuyaData: packed },
      () => { window.localStorage.setItem(LAST_SENT_KEY, packed); console.log('tuya: sent ' + packed); },
      () => { console.log('tuya: failed to send to Pebble'); });
  });
}
