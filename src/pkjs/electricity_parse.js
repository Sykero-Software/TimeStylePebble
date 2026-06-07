/* Pure parser for porssisahko.net v2 latest-prices.json.
   No Pebble/browser globals -- unit-testable with `node --test`. */

var ELEC_MAX_QUARTERS = 192;

// Returns { startEpoch: <UTC seconds>, count: <int>, bytes: [<0..255>, ...] }.
// bytes is little-endian int16 per quarter, unit 0.01 snt/kWh.
//
// The watch indexes the table arithmetically as startEpoch + i*900s, so we must
// emit entries ascending by startDate. The v2 endpoint is documented ascending,
// but we sort defensively so the result is correct regardless of API order.
function parseLatestPrices(json) {
  var prices = (json && json.prices) ? json.prices.slice() : [];
  prices.sort(function (a, b) {
    return Date.parse(a.startDate) - Date.parse(b.startDate);
  });
  var count = Math.min(prices.length, ELEC_MAX_QUARTERS);
  var startEpoch = count ? Math.floor(Date.parse(prices[0].startDate) / 1000) : 0;
  var bytes = [];
  for (var i = 0; i < count; i++) {
    var centi = Math.round(prices[i].price * 100);
    if (centi > 32767) { centi = 32767; }
    if (centi < -32768) { centi = -32768; }
    var u = centi & 0xFFFF;
    bytes.push(u & 0xFF);
    bytes.push((u >> 8) & 0xFF);
  }
  return { startEpoch: startEpoch, count: count, bytes: bytes };
}

module.exports.parseLatestPrices = parseLatestPrices;
module.exports.ELEC_MAX_QUARTERS = ELEC_MAX_QUARTERS;
