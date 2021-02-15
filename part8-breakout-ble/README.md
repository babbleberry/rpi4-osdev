Writing a "bare metal" operating system for Raspberry Pi 4 (Part 8)
===================================================================

Breakout with a Bluetooth controller
------------------------------------
A very early demo of how we might control our Breakout game using the simple BLE techniques we learned in part7-bluetooth.

 * Recommended reading to get the controller up: https://punchthrough.com/how-to-use-node-js-to-speed-up-ble-app-development/

Todo
----

 * Write up scanning implementation (receiving advertising reports)
 * Write up device detection (service UUID & name matching)
 * Write up ACL notification of characteristic change (using [bleno echo example](https://github.com/noble/bleno))
