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

var buf = Buffer.allocUnsafe(4);
var obuf = Buffer.allocUnsafe(4);
var buttbuf = Buffer.allocUnsafe(2);
const scrwidth = 1440;
const scrheight = 900;
const divisorx = scrwidth / 320;
const divisory = scrheight / 200;

ioHook.on( 'mousemove', event => {
   buf.writeUInt16LE(Math.round(event.x / divisorx), 0);
   buf.writeUInt16LE(Math.round(event.y / divisory), 2);

   if (Buffer.compare(buf, obuf)) {
      e._value = buf;
      if (e._updateValueCallback) e._updateValueCallback(e._value);
      buf.copy(obuf);
   }
});

ioHook.on( 'mousedown', event => {
   buttbuf.writeUInt8(1, 0);
   buttbuf.writeUInt8(event.button, 1);

   e._value = buttbuf;
   if (e._updateValueCallback) e._updateValueCallback(e._value);
});

ioHook.on( 'mouseup', event => {
   buttbuf.writeUInt8(2, 0);
   buttbuf.writeUInt8(event.button, 1);

   e._value = buttbuf;
   if (e._updateValueCallback) e._updateValueCallback(e._value);
});

ioHook.start();
