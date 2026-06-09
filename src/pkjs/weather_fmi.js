// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* FMI (Finnish Meteorological Institute / Ilmatieteen laitos) weather provider.
   Fetches the official "edited" forecast -- the most accurate forecast for
   Finnish/Nordic conditions -- from the FMI open-data WFS, parses the
   simple-feature XML, and sends the dictionary the watch already understands.
   Falls back to Open-Meteo when FMI returns no usable data (e.g. outside the
   Nordic coverage area). */

var weatherCommon = require('./weather');
var openmeteo = require('./weather_openmeteo');
var parser = require('./weather_fmi_parse');

var STORED_QUERY = 'fmi::forecast::edited::weather::scandinavia::point::simple';

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

  console.log('FMI weather URL: ' + url);

  weatherCommon.xhrRequest(url, 'GET', function (responseText) {
    var f = parser.parseFmiForecast(responseText, Math.floor(now.getTime() / 1000));

    if (!f.ok) {
      console.log('FMI: no usable data, falling back to Open-Meteo');
      openmeteo.getWeatherFromCoords(pos);
      return;
    }

    var icons = weatherCommon.icons;
    var dictionary = {
      'WeatherTemperature': f.currentTemp,
      'WeatherCondition': parser.smartSymbolToIcon(f.currentSymbol, icons),
      'WeatherForecastHighTemp': f.forecastHigh,
      'WeatherForecastLowTemp': f.forecastLow,
      'WeatherForecastCondition': parser.smartSymbolToIcon(f.forecastSymbol, icons),
      'WeatherUVIndex': f.uvIndex
    };

    console.log('FMI weather: ' + JSON.stringify(dictionary));
    weatherCommon.sendWeatherToPebble(dictionary);
  });
}
