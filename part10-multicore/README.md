Writing a "bare metal" operating system for Raspberry Pi 4 (Part 10)
====================================================================

Using multiple CPU cores
------------------------
Instead of a background DMA transfer, I suggested that we might use a second CPU core to play the audio whilst our main core continues on. I also said it would be hard on the Raspberry Pi 4... and it is.

This code isn't great yet, but it's enough to prove the concept in principle. Importantly, you'll need to modify your _config.txt_ file on your SD card to include the following lines:

```c
kernel_old=1
disable_commandline_tags=1
arm_64bit=1
```

For now, the points of interest in the code are:

 * The new _boot.S_ and related _boot.h_ loader
 * The new _multicore.c_ and _multicore.h_ files
 * A slimmed down _io.h_ and _kernel.c_ (DMA sound removed), with a new approach to `main()`
 * A revised _link.ld_

I will write more soon!
