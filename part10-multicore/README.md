Writing a "bare metal" operating system for Raspberry Pi 4 (Part 10)
====================================================================

Using multiple CPU cores
------------------------
Instead of a background DMA transfer, I suggested that we might use a second CPU core to play the audio whilst our main core continues on. I also said it would be hard on the Raspberry Pi 4... and it is.

I wrote this code as I referenced [Sergey Matyukevich's work](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson02), for which I am very grateful. It did need some modification to ensure the secondary cores are woken up when the time is right. This code isn't particularly "safe" yet, but it's good enough to prove the concept in principle.

Importantly, you'll need to modify your _config.txt_ file on your SD card to include the following lines:

```c
kernel_old=1
disable_commandline_tags=1
arm_64bit=1
```

For now, I'll signpost the following points of interest in the code:

 * The new _boot.S_ loader
 * The new _multicore.c_ library and related _multicore.h_ header
 * A slimmed down _io.h_ and _kernel.c_ (DMA sound removed), with a new multicore approach to `main()`
 * A revised _link.ld_ adding provisions for a secondary core's stack and the 0x00000 entry point (a result of setting `kernel_old=1`

I will write more soon to attempt to explain what's going on here.
