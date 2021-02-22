Writing a "bare metal" operating system for Raspberry Pi 4 (Part 10)
====================================================================

Using multiple CPU cores
------------------------
Instead of a background DMA transfer, I suggested that we might use a second CPU core to play the audio whilst our main core continues on. I also said it would be hard on the Raspberry Pi 4... and it is.

I wrote this code as I referenced [Sergey Matyukevich's work](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson02), for which I am very grateful. It did need some modification to ensure the secondary cores are woken up when the time is right. This code isn't particularly "safe" yet, but it's good enough to prove the concept in principle.

You'll need to modify your _config.txt_ file on your SD card to include the following lines:

```c
kernel_old=1
disable_commandline_tags=1
arm_64bit=1
```

Perhaps the most important here is the `kernel_old=1` directive. This tells the bootloader to expect the kernel at offset `0x00000` instead of `0x80000`. As such, we'll need to remove this line from our _link.ld_:

```c
. = 0x80000;     /* Kernel load address for AArch64 */
```

It also won't lock the secondary cores for us on boot, so we will still be able to access them (more on this later).

Setting up the main timer
-------------------------
There is one other important piece of setup that we'll need to take care of ourselves now - setting up the main timer. We add the following `#define` block to the top of _boot.S_:

```c
#define LOCAL_CONTROL   0xff800000
#define LOCAL_PRESCALER 0xff800008
#define OSC_FREQ        54000000
#define MAIN_STACK      0x400000
```

`LOCAL_CONTROL` is the address of the ARM_CONTROL register. At the top of our `_start:` section we'll set this to zero, effectively telling the ARM main timer to use the crystal clock as a source, and to set the increment value to 1:

```c
ldr     x0, =LOCAL_CONTROL   // Sort out the timer
str     wzr, [x0]
```

We go on to set the prescaler - think of this as another clock divisor equivalent. Setting it thus will effectively make this divisor 1 (i.e. it will have no effect):

```c
mov     w1, 0x80000000
str     w1, [x0, #(LOCAL_PRESCALER - LOCAL_CONTROL)]
```

You should remember the expected oscillator frequency of 54Mhz from part9. We set this with the following lines:

```c
ldr     x0, =OSC_FREQ
msr     cntfrq_el0, x0
msr     cntvoff_el2, xzr
```

Spinning up the cores
---------------------
We go on to check the processor ID as we always have. If it's zero then we're on the main core and we jump forward to label `2:`. This time, we have to set our stack pointer slightly differently. We can't set it below our code, because it's at 0x00000 now! Instead, we use the address we defined earlier as `MAIN_STACK` at the top:

```c
// Set stack to start somewhere safe
mov     sp, #MAIN_STACK
```

We then continue to clear the BSS as always, and jump to our `main()` function in C code. If it does happen to return, we branch back to halt the core.

Waking the secondary cores
--------------------------
Previously, we've halted the other cores by spinning them in an infinite loop at label `1:`. Instead, each core will now watch a value at its own unique memory address, initialised to zero at the bottom of _boot.S_, and named as `spin_cpu0-3`. If this value goes non-zero, then that's a signal to wake up and jump to that memory location, executing whatever code is there. Once that code returns, we start watching again.

```c
    adr     x5, spin_cpu0        // Base watch address
1:  wfe
    ldr     x4, [x5, x1, lsl #3] // Add (8 * core_number) to the base address and load what's there into x4
    cbz     x4, 1b               // Loop if zero, otherwise continue

    ldr     x1, =__test_stack    // Get ourselves a fresh stack
    mov     sp, x1

    mov     x0, #0               // Zero registers x0-x3, just in case
    mov     x1, #0
    mov     x2, #0
    mov     x3, #0
    br      x4                   // Run the code at the address in x4
    b       1b
```

You'll notice that we've set our stack pointer elsewhere for this secondary core. This is to avoid it conflicting with the main core activity. We establish this pointer to a safe 512-bytes by adding the following to our _link.ld_:

```c
.testStack :
{
    . = ALIGN(16);               // 16 bit aligned
    . = . + 512;                 // 512 bytes long
    __test_stack  = .;           // Pointer to the end (stack grows down)
}
```

And that's it for the bootloader code. If you use this new bootloader with no further code changes, it should work as before. But we need to go on to implement the signalling required to execute code on the secondary cores.

Signalling to the secondary cores from C
----------------------------------------
For now, I'll signpost the following additional points of interest in the code:

 * The new _multicore.c_ library and related _multicore.h_ header
 * A revised _kernel.c_ with a new multicore approach to `main()`

I will write more soon to attempt to explain what's going on here.
