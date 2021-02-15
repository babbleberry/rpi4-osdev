Writing a "bare metal" operating system for Raspberry Pi 4 (Part 8)
===================================================================

Receiving Bluetooth data
------------------------
So we've mastered advertising and we're broadcasting data out into the World. But that's only half the story! In this part, we'll be exploring how to receive data from an external source. This is much more exciting as we can begin to use other devices as remote controllers.

In fact, my idea for this part is to control the Breakout game we wrote in part6 by receiving data from the trackpad on my MacBook Pro over Bluetooth! Neat, eh?

Building a Bluetooth Breakout controller
----------------------------------------
Let's first build the controller code for broadcasting from the laptop. We essentially need to create a BLE peripheral in code.

We don't need to build everything from scratch since we're not on bare metal any more! I used the [Bleno library](https://github.com/noble/bleno), which is extremely popular for this kind of work. It requires some [Node.js](https://nodejs.org/en/download/) knowledge, but I didn't have any before I started and so I'm sure you'll be fine too.

Once you've got Node.js installed, use `npm` to install Bleno with `npm install bleno`. Because I'm on a Mac and running a recent version of Mac OS X (> Catalina), I needed to do this using [these three steps](https://punchthrough.com/how-to-use-node-js-to-speed-up-ble-app-development/) instead:

 * `npm install github:notjosh/bleno#inject-bindings`
 * `npm install github:notjosh/bleno-mac`
 * `npm install github:sandeepmistry/node-xpc-connection#pull/26/head`

I used the [echo example](https://github.com/noble/bleno/tree/master/examples/echo) in the Bleno repository as my base code. This example implements a Bluetooth peripheral, exposing a service which:

 * lets a connected device read a locally stored byte value
 * lets a connected device update the locally stored byte value (it can be changed locally too...!)
 * lets a connected device subscribe to receive updates when the locally stored byte value changes
 * lets a connected device unsubscribe from receiving updates

You won't be surprised to know that my design is for our Raspberry Pi to subscribe to receive updates from this service running on my laptop. That locally stored byte value will be updated locally to reflect the current mouse cursor position as it changes. Our Raspberry Pi will then be notified every time I move the mouse on my MacBook Pro as if by magic!

You can see the changes I made to the Bleno echo example to implement this in the _controller_ subdirectory of this part8-breakout-ble. They boil down to making use of [iohook](https://github.com/wilix-team/iohook), which I installed using `npm install iohook`. Here's the interesting bit (the rest is just plumbing):

```c
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
```

Here, I'm capturing the x coordinate of the mouse cursor and translating it into a number between 0 (far left of the screen) and 100 (far right of the screen). If it changes from the previous value we saw, we update the callback value (our Raspberry Pi only needs to know when the position has changed). As the callback value is updated, so any subscribed devices will be notified.

And we have ourselves a working, albeit a bit hacky, Bluetooth game controller!

Todo
----

 * Write up scanning implementation (receiving advertising reports)
 * Write up device detection (service UUID & name matching)
 * Write up ACL notification of characteristic change (using [bleno echo example](https://github.com/noble/bleno))
