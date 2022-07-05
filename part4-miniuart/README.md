Writing a "bare metal" operating system for Raspberry Pi 4 (Part 4)
===================================================================

[< Go back to part3-helloworld](../part3-helloworld)

Memory-Mapped I/O
-----------------

We have our "Hello world!" example up and running. Let's just take a little time to explain the concepts that _io.c_ is using to send this message over the UART to our dev machine.

We started with the UART for a reason - it's a (relatively) simple piece of hardware to talk to because it uses **memory-mapped I/O** (MMIO). That means we can talk directly to the hardware by reading from and writing to a set of predetermined memory addresses on the RPi4. We can write to different addresses to influence the hardware's behaviour in different ways.

These memory addresses start at `0xFE000000` (our `PERIPHERAL_BASE`).

Note: you might wonder why this base address differs from the one shown throughout the [BCM2711 ARM Peripherals document](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2711/rpi_DATA_2711_1p0.pdf). It's because the RPi4 boots into Low Peripheral Mode by default. This maps the peripherals over the last 64mb of RAM, therefore they're "visible to the ARM at 0x0_FEnn_nnnn".

People might wish to enable High Peripheral mode (full 35-bit address map) so as to avoid "losing" that last 64mb of RAM. There are various side effects, however, of doing this and it would require some refactoring of the kernel (even in this simple tutorial) to make it work.

Configuring the GPIO (General Purpose Input/Output) pins
--------------------------------------------------------

The GPIO pins (remember - we connected our USB to serial TTL cable to these) use MMIO. The top section of _io.c_ (marked with `// GPIO`) implements a few functions to configure these pins.

At this point, I recommend digging into the [BCM2711 ARM Peripherals document](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2711/rpi_DATA_2711_1p0.pdf). It has a very detailed section on GPIO. Just don't believe everything you read as there are plenty of mistakes in this document at the moment.

It will, however, tell you what our memory-mapped GPIO **registers** like `GPFSEL0`, `GPSET0`, `GPCLR0` and `GPPUPPDN0` do. These are all at known offsets from the `PERIPHERAL_BASE` and are defined by our first `enum`.

The two functions `mmio_read` and `mmio_write` can be used to read a value from and write a value to these registers.

About the GPIO pins
-------------------

Remember how we said that computers communicate in 1's and 0's? One thing we might want to do is to **set a pin** high (binary 1) or **clear a pin** low (binary 0). The two functions `gpio_set` and `gpio_clear` do just this. The corresponding hardware pin will receive a voltage when it is set high, and not when it cleared low. 

That said, however, pins can also have one of three **pull states**. This tells the RPi4 what the default state of a pin is. If a pin is set to "Pull Up", then its resting state is high (receiving voltage) unless it's told otherwise. If a pin is set to "Pull Down", then its resting state is low. This can be useful for connecting different types of devices. If a pin is set to "Pull None" then it is said to be "floating", and this is what our UART needs. The `gpio_pull` function sets the pull state of a given pin for us.

You need to know just a few more things about the GPIO pins:

 * The RPi4 is capable of more functions than there are hardware pins available for
 * To solve this, our code can dynamically map a pin to a function
 * In our case, we want GPIO 14 and GPIO 15 to take alternate function 5 (TXD1 and RXD1 respectively)
 * This maps the RPi4's mini UART (UART1) to the pins we connected our cable to!
 * We use the `gpio_function` call to set this up

Now the GPIO section of _io.c_ should be clear. Let's move on.

Configuring the UART
--------------------

The second section of _io.c_ (marked with `// UART`) implements a few functions to help us talk to the UART. Thankfully, this device also uses MMIO, and you'll see the registers set up in the first `enum` just like you saw before. Look in the [BCM2711 ARM Peripherals document](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2711/rpi_DATA_2711_1p0.pdf) for a more detailed explanation of these registers.

I do just want to call out the `AUX_UART_CLOCK` parameter, which we set to `500000000`. Remember how I said that UART communication is all about timing? Well, this is exactly the same clock speed (500 MHz) that we set in _config.txt_ when we added the `core_freq_min=500` line. This is no coincidence!

You'll also note some other familiar numbers in the `uart_init()` function, which we call directly from our `main()` routine in _kernel.c_. We set the baud rate to `115200`, and the number of bits to `8`.

Finally we add some useful functions:

 * `uart_isWriteByteReady` - checks the UART line status to ensure we are "ready to send"
 * `uart_writeByteBlockingActual` - waits until we are "ready to send" and then sends a single character
 * `uart_writeText` - sends a whole string using `uart_writeByteBlockingActual` 

You'll remember that `uart_writeText` is what we call from `main()` to print "Hello world!".

Some extra code
---------------

I don't want this tutorial to just be an explanation so, in the code, you'll see I've added some more functionality to _io.c_ and made use of it in our kernel. Have a read through and see if you can understand what's going on. Refer to the documentation again if you need to.

We can now read from our UART too! If you build the kernel and power on the RPi4 just like before, it'll say hello to the world again. But, after that, you can type into the terminal emulator window and the RPi4 sends the characters right back to you.

_Now we're communicating in two directions!_

We also implemented a software [FIFO buffer](https://en.wikipedia.org/wiki/FIFO_(computing_and_electronics)) for our UART communication. The RPi4 has limited buffer space for data arriving on the UART, and incorporating our own is likely to make it easier to manage incoming data in future.

[Go to part5-framebuffer >](../part5-framebuffer)
