Writing a "bare metal" operating system for Raspberry Pi 4 (Part 14)
====================================================================

Bare metal Ethernet for under £10
---------------------------------
It's exciting to build your own OS, but until you give it the ability to communicate with the outside world, your possibilities are limited. Indeed, our simple Bluetooth comms got us up and running - but if we're to do anything meaningful then we need proper networking.

In this tutorial, we're going to connect to an external Ethernet controller (a network card, if you like) using the RPi4's Serial Peripheral Interface (SPI).

Things you'll need:

 * An [ENC28J60 Ethernet module](https://www.amazon.co.uk/dp/B00DB76ZSK) - it cost me less than £6 and was worth every penny (n.b. code only tested on this exact model)
 * Some [female-to-female jumper cables](https://www.amazon.co.uk/dp/B072LN3HLG) - cost me less than £2.50
 * An Ethernet cable to connect to your Internet router

Connecting up the ENC28J60 Ethernet module
------------------------------------------
I followed the very helpful instructions [here](https://www.instructables.com/Super-Cheap-Ethernet-for-the-Raspberry-Pi/) to hook up the ENC28J60 to the RPi4's SPI0 interface.

We won't be connecting the interrupt line for now, so there are just six jumper leads (I've suggested colours) that need connecting:

 | Pi pin | Pi GPIO     | Jumper colour | ENC28J60 pin |
 | ------ | ----------- | ------------- | ------------ |
 | Pin 17 | +3V3 power  | Red           | VCC          |
 | Pin 19 | GPIO10/MOSI | Green         | SI           |
 | Pin 20 | GND         | Black         | GND          |
 | Pin 21 | GPIO09/MISO | Yellow        | SO           |
 | Pin 23 | GPIO11/SCLK | Blue          | SCK          |
 | Pin 24 | GPIO08/CE0  | Green         | CS           |

![GPIO location](../part3-helloworld/images/3-helloworld-pinloc.png)

Here's a (not very useful) photo of my RPi4 connected correctly:

![ENC28J60 connections](images/14-spi-ethernet-photo.jpg)

The SPI library
---------------
Let's start by looking at how we implement SPI communication.

I'm not going to write a long paper on how SPI works and why we need it, because it's [very well documented elsewhere](https://learn.sparkfun.com/tutorials/serial-peripheral-interface-spi/). It's recommended background reading, but not essential if all you want to do is get something working.

Look at _lib/spi.c_. It uses some of existing functions in _lib/io.c_ that you'll remember from earlier tutorials. In fact, I've added two functions to the _include/io.h_ header file so we can call them from our SPI library:

```c
void gpio_setPinOutputBool(unsigned int pin_number, unsigned int onOrOff);
void gpio_initOutputPinWithPullNone(unsigned int pin_number);
```

Specifically, `spi_init()` sets GPIO 7, 9, 10, and 11 to use the ALT0 function. Cross-referencing with the [BCM2711 ARM Peripherals document](https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf), page 77, you'll see that this maps SPI0 to the GPIO header. GPIO 8 is mapped as an output pin, since we'll use this to signal to the ENC28J60 that we want to talk. In fact, the `spi_chip_select()` function takes a true/false (boolean) parameter which either sets or clears this pin.

Looking at the SPI0 register map on page 134, we see this reflected in our `REGS_SPI0` structure. This gives us handy access to the SPI0 peripheral's memory-mapped registers.

Our `spi_send_recv()` function then sets us up for some communcation:

 * Sets the DLEN Register to the number of bytes to transfer (a length we passed into the function)
 * Clears the RX & TX FIFOs
 * Sets the Transfer Active (TA) flag

Then, whilst there's either data to write or data to read (and we haven't written/read more bytes than we asked for), we write to/read from the FIFO using the buffers we passed in. Once we think we're done, we wait until the SPI interface agrees i.e. the DONE flag in the CS Register is set. If there are extraneous bytes to read, we just throw them away (well, dump them to the screen for now because this shouldn't happen).

Finally, to be absolutely sure, we clear the TA flag.

I've then set up two convenient functions - `spi_send()` and `spi_recv()` - which exercise `spi_send_recv()`, mainly to make future code more readable.

The ENC28J60 drivers
--------------------
Let's now look into the _net/_ subdirectory.

Both _enc28j60.c_ and _enc28j60.h_ make up the driver code for the ENC28J60 Ethernet module. Whilst we could have laboured for months writing our own driver based on [the module's datasheet](http://ww1.microchip.com/downloads/en/devicedoc/39662c.pdf), I chose to leverage somebody else's hard work instead. It felt like a win that I could effortlessly bring somebody else's good code into my own OS! I did, however, make sure I understood what the code was doing at every turn (optional!).

Thanks to [this Github repository](https://github.com/wolfgangr/enc28j60) for saving me months of work. I made a very few changes to the code, but nothing worth documenting here. If you're keen to see how little I needed to change, clone the repo and make good use of the `diff` command.

What I did need to do is write some bridging code between the driver and the RPi4 hardware. Essentially, I'm talking about hooking up our SPI library to the driver - the whole reason for _encspi.c_.

It defines four functions that the driver requires (well documented in the _enc28j60.h_ file):

```c
void ENC_SPI_Select(unsigned char truefalse) {
    spi_chip_select(!truefalse); // If it's true, select 0 (the ENC), if false, select 1 (i.e. deselect the ENC)
}

void ENC_SPI_SendBuf(unsigned char *master2slave, unsigned char *slave2master, unsigned short bufferSize) {
    spi_chip_select(0);
    spi_send_recv(master2slave, slave2master, bufferSize);
    spi_chip_select(1); // De-select the ENC
}

void ENC_SPI_Send(unsigned char command) {
    spi_chip_select(0);
    spi_send(&command, 1);
    spi_chip_select(1); // De-select the ENC
}

void ENC_SPI_SendWithoutSelection(unsigned char command) {
    spi_send(&command, 1);
}
```

Perhaps the most confusing aspect is the chip selection. Through a bit of trial & error I discovered that when GPIO08 is clear, the device is selected, and when it's set, the device is deselected. If you can explain this to me, I'd love to hear from you - frankly, I was just pleased to get it working, so I moved on!

Some more timer functions
-------------------------
The only other thing our ENC28J60 driver requires is access to a couple of well-defined timer functions:

 * `HAL_GetTick()` - returns the current number of timer ticks since start
 * `HAL_Delay()` - delays by a specified number of milliseconds

These are quickly implemented in _kernel/kernel.c_ and weren't too much of a stretch after _part13-interrupts_:

```c
unsigned long HAL_GetTick(void) {
    unsigned int hi = REGS_TIMER->counter_hi;
    unsigned int lo = REGS_TIMER->counter_lo;

    //double check hi value didn't change after setting it...
    if (hi != REGS_TIMER->counter_hi) {
        hi = REGS_TIMER->counter_hi;
        lo = REGS_TIMER->counter_lo;
    }

    return ((unsigned long)hi << 32) | lo;
}

void HAL_Delay(unsigned int ms) {
    unsigned long start = HAL_GetTick();

    while(HAL_GetTick() < start + (ms * 1000));
}
```

Let's connect!
--------------
So we have a working driver, that's interfacing with our hardware via _net/encspi.c_ and a few timer functions in _kernel/kernel.c_. Now what?

The design goals of our kernel's networking routine will be to:

 1. Prove we can talk to the hardware
 2. Bring the network up successfully
 3. Prove we can connect to something else on the network and get a response

My proposals for how we fulfil these goals are:

 1. Prove we can detect whether a network link has been established at a physical level (CAT5 cable plugged in and connected to a working switch)
 2. Rely on the ENC28J60 driver to tell us that we've started up successfully
 3. Handcraft and send an [ARP](https://en.wikipedia.org/wiki/Address_Resolution_Protocol) request and await an ARP response from my Internet router (the traditional way devices "find each other" on a network from a point of zero knowledge)

Look at _kernel/arp.c_. First we create a handle to reference our driver instance `ENC_HandleTypeDef handle`.

We then initialise this structure in `init_network()`:

```c
handle.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
handle.Init.MACAddr = myMAC;
handle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
handle.Init.InterruptEnableBits = EIE_LINKIE | EIE_PKTIE;
```

This starts the module in half duplex mode (can't transmit & receive simultaneously), sets its MAC address (my favourite: `C0:FF:EE:C0:FF:EE`!), tells the hardware to add its own packet checksums (we don't want to have to create them in software), and enables interrupt messages for "link up/down" and "packet received".

We then call the driver routine `ENC_Start(&handle)` and check it returns true (this fulfils design requirement 2 - the driver tells us we've started correctly). We go on to set the MAC address using `ENC_SetMacAddr(&handle)`.

This line waits until a physical network link has been established (fulfilling design requirement 1):

```c
while (!(handle.LinkStatus & PHSTAT2_LSTAT)) ENC_IRQHandler(&handle);
```

The driver's `ENC_IRQHandler(&handle)` routine would ordinarily be called if interrupt was raised, and it refreshes the driver status flags. Because we didn't hook up the interrupt line, we're just polling in the software for now. When we see the `handle.LinkStatus` flag has the `PHSTAT2_LSTAT` bit set, we know the link is up and we carry on (documented on page 24 of the module's datasheet).
