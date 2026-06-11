// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* FMI (Finnish Meteorological Institute / Ilmatieteen laitos) weather provider.
   One request: the official "edited" forecast via the ::point::timevaluepair
   stored query, which snaps the queried lat/lon to FMI's nearest *named*
   forecast location (the same one ilmatieteenlaitos.fi shows for you, e.g.
   "Jyskä") and returns its name alongside the forecast values. The current-temp
   widget therefore shows that named point's forecast (matching the website), and
   the abbreviated location name is shown beneath it.
   Falls back to Open-Meteo when FMI returns no usable data. */

import * as weatherCommon from './weather';
import type { GeoPosition, WeatherDict } from './weather';
import * as openmeteo from './weather_openmeteo';
import { smartSymbolToIcon } from './weather_fmi_parse';
import { parseFmiForecastTvp, abbreviateLocationName } from './weather_fmi_fc_parse';

const STORED_QUERY = 'fmi::forecast::edited::weather::scandinavia::point::timevaluepair';

// Cap the location name sent to the watch; the C side stacks it as two 4-char
// lines (8 chars total). Truncated plainly, no ellipsis, first char uppercased.
const LOCATION_NAME_MAXLEN = 8;

export function getWeatherFromCoords(pos: GeoPosition): void {
  const lat = pos.coords.latitude;
  const lon = pos.coords.longitude;

  const now = new Date();
  const start = new Date(now); start.setHours(0, 0, 0, 0);   // local start of today
  const end = new Date(now); end.setHours(23, 59, 0, 0);     // local end of today

  const url = 'https://opendata.fmi.fi/wfs?service=WFS&version=2.0.0' +
    '&request=getFeature&storedquery_id=' + STORED_QUERY +
    '&latlon=' + lat + ',' + lon +
    '&parameters=temperature,smartsymbol,uvindex' +
    '&timestep=60' +
    '&starttime=' + start.toISOString() +
    '&endtime=' + end.toISOString();

  console.log('FMI forecast URL: ' + url);

  weatherCommon.xhrRequest(url, 'GET', (responseText) => {
    const f = parseFmiForecastTvp(responseText, Math.floor(now.getTime() / 1000));

    if (!f.ok) {
      console.log('FMI: no usable data, falling back to Open-Meteo');
      openmeteo.getWeatherFromCoords(pos);
      return;
    }

    const icons = weatherCommon.icons;
    const dictionary: WeatherDict = {
      WeatherTemperature: f.currentTemp,
      WeatherCondition: smartSymbolToIcon(f.currentSymbol, icons),
      WeatherForecastHighTemp: f.forecastHigh,
      WeatherForecastLowTemp: f.forecastLow,
      WeatherForecastCondition: smartSymbolToIcon(f.forecastSymbol, icons),
      WeatherUVIndex: f.uvIndex,
      WeatherStationName: abbreviateLocationName(f.name, LOCATION_NAME_MAXLEN),
    };

    console.log('FMI weather: ' + JSON.stringify(dictionary));
    weatherCommon.sendWeatherToPebble(dictionary);
  });
}
