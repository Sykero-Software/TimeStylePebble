// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* FMI (Finnish Meteorological Institute / Ilmatieteen laitos) weather provider.
   Two requests:
   - the official "edited" forecast (icon, today's high/low, UV, and a baseline
     current temperature), and
   - the nearest observation station (measured current temperature + station
     name) from fmi::observations::weather::timevaluepair.
   The observation result overrides the baseline temperature and adds the
   station name; if it is unavailable, the forecast values stand.
   Falls back to Open-Meteo when the FMI forecast returns no usable data. */

var weatherCommon = require('./weather');
var openmeteo = require('./weather_openmeteo');
var parser = require('./weather_fmi_parse');
var obsParser = require('./weather_fmi_obs_parse');

var STORED_QUERY = 'fmi::forecast::edited::weather::scandinavia::point::simple';
var OBS_STORED_QUERY = 'fmi::observations::weather::timevaluepair';

// bbox half-size in degrees around the configured point; wide enough to catch a
// station even where the network is sparse (~78 km N-S).
var OBS_BBOX_HALF = 0.7;
// Cap the station name sent to the watch (the watch's small font + narrow
// sidebar fit only a few chars; the C side ellipsises any remainder).
var STATION_NAME_MAXLEN = 12;

module.exports.getWeatherFromCoords = getWeatherFromCoords;

function getWeatherFromCoords(pos) {
  var lat = pos.coords.latitude;
  var lon = pos.coords.longitude;

  var now = new Date();
  var start = new Date(now); start.setHours(0, 0, 0, 0);   // local start of today
  var end = new Date(now);   end.setHours(23, 59, 0, 0);   // local end of today

  var url = 'https://opendata.fmi.fi/wfs?service=WFS&version=2.0.0' +
    '&request=getFeature&storedquery_id=' + STORED_QUERY +
    '&latlon=' + lat + ',' + lon +
    '&parameters=temperature,smartsymbol,uvindex' +
    '&timestep=60' +
    '&starttime=' + start.toISOString() +
    '&endtime=' + end.toISOString();

  console.log('FMI forecast URL: ' + url);

  weatherCommon.xhrRequest(url, 'GET', function (responseText) {
    var f = parser.parseFmiForecast(responseText, Math.floor(now.getTime() / 1000));

    if (!f.ok) {
      console.log('FMI: no usable forecast data, falling back to Open-Meteo');
      openmeteo.getWeatherFromCoords(pos);
      return;
    }

    var icons = weatherCommon.icons;
    var dictionary = {
      'WeatherTemperature': f.currentTemp,    // baseline; observation overrides below
      'WeatherCondition': parser.smartSymbolToIcon(f.currentSymbol, icons),
      'WeatherForecastHighTemp': f.forecastHigh,
      'WeatherForecastLowTemp': f.forecastLow,
      'WeatherForecastCondition': parser.smartSymbolToIcon(f.forecastSymbol, icons),
      'WeatherUVIndex': f.uvIndex
    };

    console.log('FMI forecast: ' + JSON.stringify(dictionary));
    weatherCommon.sendWeatherToPebble(dictionary);

    // Now refine the current temperature + station name from observations.
    fetchObservation(lat, lon, now);
  });
}

function fetchObservation(lat, lon, now) {
  var latN = parseFloat(lat), lonN = parseFloat(lon);
  var bbox = (lonN - OBS_BBOX_HALF) + ',' + (latN - OBS_BBOX_HALF) + ',' +
             (lonN + OBS_BBOX_HALF) + ',' + (latN + OBS_BBOX_HALF);
  var start = new Date(now.getTime() - 90 * 60 * 1000);    // last 90 minutes

  var url = 'https://opendata.fmi.fi/wfs?service=WFS&version=2.0.0' +
    '&request=getFeature&storedquery_id=' + OBS_STORED_QUERY +
    '&bbox=' + bbox +
    '&parameters=temperature' +
    '&timestep=30' +
    '&starttime=' + start.toISOString() +
    '&endtime=' + now.toISOString();

  console.log('FMI observation URL: ' + url);

  weatherCommon.xhrRequest(url, 'GET', function (responseText) {
    var stations = obsParser.parseFmiObservations(responseText);
    var st = obsParser.pickNearestStation(stations, latN, lonN);
    if (!st) {
      console.log('FMI: no usable observation station; keeping forecast temp');
      return;
    }
    var dictionary = {
      'WeatherTemperature': st.temp,
      'WeatherStationName': obsParser.abbreviateStationName(st.name, STATION_NAME_MAXLEN)
    };
    console.log('FMI observation: ' + JSON.stringify(dictionary));
    weatherCommon.sendWeatherToPebble(dictionary);
  });
}
