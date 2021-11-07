Writing a "bare metal" operating system for Raspberry Pi 4 (Part 12)
====================================================================

Porting the WordUp Graphics Toolkit
-----------------------------------
Back in the mid-1990s (when I was young!), programmers who wanted to build their own games didn't have rich frameworks like Unity. Perhaps the closest we got was the WordUp Graphics Toolkit, which I came across on the Hot Sound & Vision CD-ROM - a BBS archive. If you have a moment, perhaps use Google to see what "bulletin board systems" were... nostaglia awaits!

Much like my very simple _fb.c_, the WGT provides a library of graphics routines which can be depended upon for reuse. This library, however, is much more fully-fledged than mine, and makes it easy to build sprite-based games (like Breakout, Space Invaders, Pacman etc.).

The directory structure
-----------------------
As I port the WGT to my OS (a.k.a. make it work on my OS), I am using the following directories:

 * _bin/_ : for WGT binary files (fonts, sprites, bitmaps etc.)
 * _controller-ios/_ : a sample Swift BLE controller for the iOS platform
 * _controller-node/_ : a sample Node.js BLE controller 
 * _include/_ : now contains _wgt.h_ and _wgtspr.h_ too (header files necessary for WGT code)
 * _samples/_ : sample "kernels" for my OS which exercise certain WGT library functions. To build them, copy one of these (and only one at a time) to the same directory as the _Makefile_.
 * _wgt/_ : the library itself. Where possible, I have stayed true to the original code, but do bear in mind it was written for the x86 architecture and we're on AArch64!

Please note: I am neither a Node.js developer, nor a Swift developer, and so the controllers are purely samples that serve my purpose. They are not intended to be exemplars! I am very aware of the multitudinous problems with both...

Building
--------
So... to build the first WGT sample simply type `cp samples/wgt01.c .` from the top-level directory, and then type `make`. When you boot with the generated _kernel8.img_ you will see the screen go into 320x200 (VGA!) mode and draw a white line from corner to corner. If you do, the library is doing its stuff!

boot/boot.S changes
-------------------
We're still booting into a multicore environment (just in case we need it). There are a few significant changes to _boot/boot.S_ though. They are:

 * Enable FPU (floating-point unit) access so we can do non-integer mathematics
 * Switch from EL3 (supervisor exception level) down to EL1 (kernel exception level), disabling the MMU all the same
 * Move the addresses for the `spin_cpu` variables to accommodate a larger _boot.S_
 * Implement a `get_el` function to check which exception level we're at (for debug mainly)

Using the iOS BLE controller
----------------------------
To use the iOS BLE controller instead of the Node.JS controller, ensure that you have:

```c
#define IOS_CONTROL
```

at the top of each of _wgt/mouse.c_ and _lib/bt.c_. Without this `#define`, the code will be looking for the Node.JS controller (so remove these lines if that's what you want!).

Work in progress!
-----------------
There's always more that can be done, but I do think this was a good exercise in exploring the joy of getting other people's code to run on your own OS! It's quite a thrill.

_Do have a go at building some of the samples (hint: wgt20 and wgt60 are super fun!)..._

I'm going to move on from here now so we can continue to make progress on the OS itself.

[Go to part13-interrupts >](../part13-interrupts)
