Writing a "bare metal" operating system for Raspberry Pi 4 (Part 1)
===================================================================

How do we code?
---------------

We tell the RPi4 what to do by writing code. You may know that code ultimately ends up as a series of 0's and 1's (binary). You'll be pleased to know, however, that we don't need to write it like this, otherwise we'd easily lose track of what was going on! In fact, it's one of the jobs of the compiler to convert human-readable language into those 0's and 1's.

To get going we need to understand two languages: **assembly language** and **C**. Whilst C will likely be recognisable to most modern software developers, assembly language is "spoken" by fewer folks. It's a lower-level language that most closely resembles how the CPU "thinks" and it therefore gives us a lot of control, whereas C brings us into a higher-level, human-readable world. We lose a little control to the compiler though, but for our purposes I think we can trust it!

We will need to start out in assembly language, but there isn't much to write before we can then pick up in C.

A note about this tutorial
--------------------------

This tutorial is not intended to teach you how to code in assembly language or C. There are plenty of good resources on these topics and I am not an expert/authority! I will therefore be assuming some knowledge along the way. Please do read around the topics that I introduce if you need/want to.

Bootstrapping
-------------

The first code that the RPi4 will run will need to be written in assembly language. It makes some checks, does some setup and launches us into our first C program - the **kernel**.

 * The Arm Cortex-A72 has four cores. We only want our code to run on the master core, so we check the processor ID and either run our code (master) or hang in an infinite loop (slave).
 * We need to tell our OS how to access the **stack**. I think of the stack as temporary storage space used by currently-executing code, like a scratchpad. We need to set memory aside for it and store a pointer to it.
 * We also need to initialise the BSS section. This is the area in memory where uninitialised variables will be stored. It's more efficient to initialise everything to zero here, rather than take up space in our kernel image doing it explicitly.
 * Finally, we can jump to our main() routine in C!

Read and understand the code below and save it as _boot.S_. I suggest using the [Arm Programmer's Guide](https://developer.arm.com/documentation/den0024/a/) as a reference.

```c
.section ".text.boot"  // Make sure the linker puts this at the start of the kernel image

.global _start  // Execution starts here

_start:
    // Check processor ID is zero (executing on main core), else hang
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // We're not on the main core, so hang in an infinite wait loop
1:  wfe
    b       1b
2:  // We're on the main core!

    // Set stack to start below our code
    ldr     x1, =_start
    mov     sp, x1

    // Clean the BSS section
    ldr     x1, =__bss_start     // Start address
    ldr     w2, =__bss_size      // Size of the section
3:  cbz     w2, 4f               // Quit loop if zero
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b               // Loop if non-zero

    // Jump to our main() routine in C (make sure it doesn't return)
4:  bl      main
    // In case it does return, halt the master core too
    b       1b
```

And now we're in C
------------------

You will likely note that the `main()` routine is as yet undefined. We can write this in C (save it as _kernel.c_), keeping it very simple for now:

```c
void main()
{
    while (1);
}
```

This simply spins us in an infinite loop!

Linking it all together
-----------------------

We've written code in two different languages. Somehow we need to glue these together, ensuring that the created image will be executed in the way that we intend. We use a **linker script** for this. The linker script will also define our BSS-related labels (perhaps you were already wondering where they get defined?). I suggest you save the following as _link.ld_:

```c
SECTIONS
{
    . = 0x80000;     /* Kernel load address for AArch64 */
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = (__bss_end - __bss_start)>>3;
```

Writing linker scripts is [worth investigating](http://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_mono/ld.html#SEC6) but, for our purposes, all you need to know is that by referencing `.text.boot` first and using the `KEEP()`, we ensure the `.text` section starts with our assembly code. That means our first instruction starts at 0x80000, which is exactly where the RPi4 will look for it when it boots. Our code will be run.

_Now you're ready to compile and then boot your OS!_
