
var weather = require('./weather');
var electricity = require('./electricity');
var crypto = require('./crypto');
var cryptoParse = require('./crypto_parse');

var Clay = require('pebble-clay');
var clayConfig = require('./config_clay');
var clayConfigCustom = require('./config_clay_custom');
var clay = new Clay(clayConfig, clayConfigCustom, { autoHandleEvents: false });

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
  function (e) {
    console.log('JS component is now READY');

    // if it has never been started, set the weather to enabled
    if (window.localStorage.getItem('disable_weather') === null) {
      window.localStorage.setItem('disable_weather', 'no');
    }

    console.log('the wdisabled value is: "' + window.localStorage.getItem('disable_weather') + '"');
    // if applicable, get the weather data
    if (window.localStorage.getItem('disable_weather') != 'yes') {
      weather.updateWeather();
    }

    // electricity: default disabled until a widget selects it (set in webviewclosed)
    if (window.localStorage.getItem('disable_electricity') === null) {
      window.localStorage.setItem('disable_electricity', 'yes');
    }
    if (window.localStorage.getItem('disable_electricity') !== 'yes') {
      // Force a fetch on (re)launch: the watch's persisted price table is wiped
      // by a watchface reinstall/reboot, but electricity_last_fetch lives in
      // phone localStorage and survives, so a throttled call would skip and the
      // widget would stay empty for up to MIN_FETCH_INTERVAL_S. (weather has no
      // throttle, so it self-heals; electricity needs this explicit force.)
      electricity.updateElectricity(true);
    }

    // crypto coins: each defaults to disabled until a widget selects it (set
    // in webviewclosed). When enabled, force a send on (re)launch for the same
    // reason as electricity above: the watch's persisted values are wiped by a
    // reinstall/reboot, but the *_last_* keys survive in phone localStorage,
    // so a non-forced call would suppress the send as "unchanged" and leave
    // the widget empty until the price next moves. Periodic refresh is driven
    // by the watch's data request (see the appmessage handler), not a JS timer.
    var anyCryptoEnabled = false;
    cryptoParse.COINS.forEach(function (c) {
      if (window.localStorage.getItem(c.disableKey) === null) {
        window.localStorage.setItem(c.disableKey, 'yes');
      }
      if (window.localStorage.getItem(c.disableKey) !== 'yes') {
        anyCryptoEnabled = true;
      }
    });
    if (anyCryptoEnabled) {
      crypto.updateCrypto(true);
    }
  }
);

// Listen for incoming messages
// when one is received, we treat it as the watch's request for fresh phone data
// (weather + electricity + crypto) -- this is the single shared, watch-driven poll.
Pebble.addEventListener('appmessage',
  function (msg) {
    console.log('Recieved message: ' + JSON.stringify(msg.payload));

    // in the case of recieving this, we assume the watch does, in fact, need weather data
    window.localStorage.setItem('disable_weather', 'no');
    weather.updateWeather();
    electricity.updateElectricity();
    crypto.updateCrypto();
  }
);

Pebble.addEventListener('showConfiguration', function (e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) { console.log('No settings changed!'); return; }

  // getSettings(response, false) returns the raw parsed config keyed by
  // messageKey, with each value wrapped as { value: X[, precision] } (Clay's
  // serialize() shape). Flatten those wrappers into `s` so each `s[k]` is the
  // bare value the rest of this handler expects.
  var raw = clay.getSettings(e.response, false);
  var s = {};
  Object.keys(raw).forEach(function (k) {
    var v = raw[k];
    s[k] = (v && typeof v === 'object' && 'value' in v) ? v.value : v;
  });

  var dict = {};

  // colors: Clay returns a 24-bit RGB decimal int; C decodes via GColorFromHEX
  function colorInt(v) {
    return (typeof v === 'string') ? (parseInt(v.replace(/^0x/, ''), 16) & 0xFFFFFF) : (v & 0xFFFFFF);
  }
  ['SettingColorTime','SettingColorBG','SettingColorSidebar','SettingSidebarTextColor',
   'SettingTwtStatusBgColor','SettingDateBgColor','SettingSidebarBgColorLeft',
   'SettingSidebarBgColorRight'].forEach(function (k) {
    if (s[k] !== undefined && s[k] !== null) { dict[k] = colorInt(s[k]); }
  });

  // Straight-through settings. Clay returns radiogroup/select/input values as
  // strings (DOM values), so coerce every numeric watch key to int; only genuine
  // string settings (alt-clock name, decimal separator char) stay as-is.
  var STRING_KEYS = { SettingAltClockName: true, SettingDecimalSep: true };
  ['SettingLanguageID','SettingShowLeadingZero','SettingClockFontId','SettingDisconnectIcon',
   'SettingBluetoothVibe','SettingMidiVibe','SettingBigDate','SettingTwtShowRemaining',
   'SettingTwtTargetVibe','SettingHourlyVibe','SettingWidget0ID','SettingWidget1ID',
   'SettingWidget2ID','SettingWidget2_0ID','SettingWidget2_1ID','SettingWidget2_2ID',
   'SettingSecondaryAlwaysOn','SettingSidebarOnLeft','SettingUseLargeFonts','SettingUseMetric',
   'SettingShowBatteryPct','SettingDisableAutobattery','SettingAltClockName','SettingAltClockOffset',
   'SettingDecimalSep','SettingHealthUseDistance','SettingHealthUseRestfulSleep',
   'SettingPollIntervalMin','SettingElecQuietStart','SettingElecQuietEnd',
   'SettingElecCheapFactorPct'].forEach(function (k) {
    if (s[k] === undefined || s[k] === null || s[k] === '') { return; }
    dict[k] = STRING_KEYS[k] ? s[k] : parseInt(s[k], 10);
  });
  if (dict.SettingPollIntervalMin === undefined || isNaN(dict.SettingPollIntervalMin)) {
    dict.SettingPollIntervalMin = 30;
  }

  // decimal electricity fields: snt/kWh -> centi (x100)
  if (s.elec_cheap_floor !== undefined && s.elec_cheap_floor !== '') {
    dict.SettingElecCheapFloorCenti = Math.round(parseFloat(s.elec_cheap_floor) * 100);
  }
  if (s.elec_cheap_ceiling !== undefined && s.elec_cheap_ceiling !== '') {
    dict.SettingElecCheapCeilingCenti = Math.round(parseFloat(s.elec_cheap_ceiling) * 100);
  }

  // phone-only: weather location + data source -> localStorage (weather.js reads these)
  if (s.weather_loc_mode === 'manual') {
    window.localStorage.setItem('weather_loc', s.weather_loc || '');
    window.localStorage.setItem('weather_loc_lat', s.weather_loc_lat || '');
    window.localStorage.setItem('weather_loc_lng', s.weather_loc_lng || '');
  } else {
    window.localStorage.removeItem('weather_loc');
    window.localStorage.removeItem('weather_loc_lat');
    window.localStorage.removeItem('weather_loc_lng');
  }
  if (s.weather_datasource) {
    window.localStorage.setItem('weather_datasource', s.weather_datasource);
    window.localStorage.setItem('weather_api_key', '');
  }

  // derive disable_* flags from selected widget IDs (preserved from old index.js)
  var widgetIDs = [dict.SettingWidget0ID, dict.SettingWidget1ID, dict.SettingWidget2ID,
                   dict.SettingWidget2_0ID, dict.SettingWidget2_1ID, dict.SettingWidget2_2ID];
  window.localStorage.setItem('disable_weather',
    (widgetIDs.indexOf(7) != -1 || widgetIDs.indexOf(8) != -1 || widgetIDs.indexOf(13) != -1) ? 'no' : 'yes');
  window.localStorage.setItem('disable_electricity',
    (widgetIDs.indexOf(14) != -1 || widgetIDs.indexOf(18) != -1 || widgetIDs.indexOf(19) != -1) ? 'no' : 'yes');
  cryptoParse.COINS.forEach(function (c) {
    window.localStorage.setItem(c.disableKey, (widgetIDs.indexOf(c.widgetId) != -1) ? 'no' : 'yes');
  });

  console.log('Preparing message: ' + JSON.stringify(dict));
  Pebble.sendAppMessage(dict, function () {
    weather.updateWeather(true);
    electricity.updateElectricity(true);
    crypto.updateCrypto(true);
  }, function () { console.log('Failed to send config data!'); });
});
