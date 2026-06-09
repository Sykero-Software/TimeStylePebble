/* Bitcoin price (USD). Polls CoinGecko on a configurable interval (default 30 min,
   floor 5 min) and pushes the price to the watch ONLY when the displayed value (USD
   rounded to the nearest thousand) changes — no needless Bluetooth wakeups. The poll
   interval is purely a JS-side timer; it is never sent to the watch. */

var weather = require('./weather');          // reuse xhrRequest helper
var parser = require('./btc_parse');

var COINGECKO_URL =
  'https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd';
var DEFAULT_INTERVAL_MIN = 30;
var MIN_INTERVAL_MIN = 5;

var pollTimer = null;

function readIntervalMin() {
  var v = parseInt(window.localStorage.getItem('btc_poll_interval_min'), 10);
  if (isNaN(v)) { return DEFAULT_INTERVAL_MIN; }
  if (v < MIN_INTERVAL_MIN) { return MIN_INTERVAL_MIN; }
  return v;
}

function updateBtc(forceUpdate) {
  if (window.localStorage.getItem('disable_btc') === 'yes') {
    return;
  }
  weather.xhrRequest(COINGECKO_URL, 'GET', function (responseText) {
    var thousands;
    try {
      thousands = parser.parsePriceThousands(JSON.parse(responseText));
    } catch (e) {
      console.log('BTC: parse error ' + e);
      return;
    }
    if (thousands === null) {
      console.log('BTC: no usable price in response');
      return;
    }
    var last = window.localStorage.getItem('btc_last_thousands');
    if (!forceUpdate && last !== null && parseInt(last, 10) === thousands) {
      console.log('BTC: unchanged (' + thousands + 'k), not sending');
      return;
    }
    Pebble.sendAppMessage({ 'BtcPriceThousands': thousands }, function () {
      console.log('BTC: sent ' + thousands + 'k to Pebble');
      window.localStorage.setItem('btc_last_thousands', String(thousands));
    }, function (e) {
      console.log('BTC: failed to send to Pebble');
    });
  });
}

// (Re)start the polling timer from the current configured interval. Idempotent.
function setupBtcPolling() {
  if (pollTimer !== null) {
    clearInterval(pollTimer);
    pollTimer = null;
  }
  if (window.localStorage.getItem('disable_btc') === 'yes') {
    return;
  }
  var ms = readIntervalMin() * 60 * 1000;
  pollTimer = setInterval(function () { updateBtc(false); }, ms);
  console.log('BTC: polling every ' + readIntervalMin() + ' min');
}

module.exports.updateBtc = updateBtc;
module.exports.setupBtcPolling = setupBtcPolling;
