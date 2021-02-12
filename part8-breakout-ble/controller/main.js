var bleno = require('bleno-mac');

var BlenoPrimaryService = bleno.PrimaryService;

var EchoCharacteristic = require('./characteristic');

console.log('bleno - echo');

var e = new EchoCharacteristic();

bleno.on('stateChange', function(state) {
  console.log('on -> stateChange: ' + state);

  if (state === 'poweredOn') {
    bleno.startAdvertising('echo', ['ec00']);
  } else {
    bleno.stopAdvertising();
  }
});

bleno.on('advertisingStart', function(error) {
  console.log('on -> advertisingStart: ' + (error ? 'error ' + error : 'success'));

  if (!error) {
    bleno.setServices([
      new BlenoPrimaryService({
        uuid: 'ec00',
        characteristics: [
           e
        ]
      })
    ]);
  }
});

var ioHook = require('iohook');

var buf = Buffer.allocUnsafe(1);
var obuf = Buffer.allocUnsafe(1);
const scrwidth = 1440;
const divisor = scrwidth / 100;

ioHook.on( 'mousemove', event => {
   buf.writeUInt8(Math.round(event.x / divisor), 0);

   if (Buffer.compare(buf, obuf)) {
      e._value = buf;
      if (e._updateValueCallback) e._updateValueCallback(e._value);
      buf.copy(obuf);
   }
});

ioHook.start();
