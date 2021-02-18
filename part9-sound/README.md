Writing a "bare metal" operating system for Raspberry Pi 4 (Part 9)
===================================================================

Playing sound from the audio jack
---------------------------------
One thing our game is missing is the excitement of sound! Some beeps and squeaks would be a wonderful addition to make the gameplay more compelling. Let's work to do just that!

I wrote this code as I referenced [Peter Lemon's work](https://github.com/PeterLemon/RaspberryPi/tree/master/Sound/PWM/8BIT/44100Hz/Stereo/CPU), for which I am very grateful. It did need some significant modification to work on the Raspberry Pi 4 hardware.

Design goals
------------
Perhaps most importantly, we must be able to play sounds in the background. If our audio playback ties up the CPU, then gameplay will stop whilst the sound is playing. I think any player would be immediately put off by the rude intrusion into their adventure!

One solution for this is to implement multi-tasking, thereby making use of the four CPU cores (so far we've only used one). This is no small feat, and a big commitment for a fews beeps and squeaks.

Fortunately, the Raspberry Pi 4's hardware allows us to avoid this pain (for now), using something called DMA. This allows specific hardware subsystems to access main system memory completely independently of the CPU.

Notes to self
-------------
Here are a few things I learned on this journey, which helped me along significantly:

 * The Raspberry Pi 4 uses PWM1 for output on the audio jack when GPIO 40 and 41 are mapped to Alternate Function 0
 * The Raspberry Pi 4's clock oscillator frequency is 54Mhz
 * PWM1 DMA is mapped to DMA channel 1
 * PWM1 is mapped to DREQ 1
 * We must use Legacy Master (starting `0x7E`, not `0xFE`) addresses for DMA transfers to peripherals
 * The DMA Control Block structures must be 32-bit aligned

And always, always have a browser tab open on the [BCM2711 ARM Peripherals document](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2711/rpi_DATA_2711_1p0.pdf). It's a very handy reference for register addresses and bitmaps etc.

Audio sample format
-------------------
_audio.bin_ is the audio file we'll be playing. Technically speaking, it's 8-bit, unsigned PCM data at 44.1Khz. This is an unusual format in this modern day and age, but it's easily converted using a tool like [ffmpeg](https://ffmpeg.org/).

To convert from our _.bin_ file to a _.wav_ file that any laptop can play natively, do this:

`ffmpeg -f u8 -ar 44.1k -ac 2 -i audio.bin audio.wav`

To convert back to our binary format, do this:

`ffmpeg -i audio.wav -f u8 -ar 44.1k -ac 2 audio.bin`

This should help you try the code with your own audio samples!

Testing playback using the CPU and PWM module
---------------------------------------------
I knew DMA transfers might be a tricky beast, so I began by just proving I could play audio to the jack output of the Raspberry Pi 4.

Looking at `main()`, you'll see that we first call `audio_init()`. This function ensures that PWM1 is correctly mapped. PWM is a technique used to control analogue devices using digital signals. Whilst digital signals are either on (1 - full power) or off (0 - no power), analogue signals may be an infinite number of values between 1 and 0. PWM fakes an analogue signal by applying power in quick pulses/bursts of regulated voltage. The resultant average voltage will end up looking roughly like an analogue signal, despite not being one. Clever, eh?

These pulses/bursts do need to be highly accurate for this trick to work, and so we need a reliable clock source. Just like your kitchen clock ticks every second, so the oscillator on the Raspberry Pi 4 has a regular 'tick' - in this case, 54,000,000 times per second (54Mhz)! Our audio sample is at 44.1Khz though, so we need to 'slow it down'. We do this by first stopping the clock, then setting a clock divisor, setting the PWM range, and enabling the clock again. In my code, I use 2 as the clock divisor (so we're down to 27Mhz) and set the range to 612 (0x264). Essentially, this means that our PWM module will move to a new sample every 27,000,000/612 times per second - roughly equivalent to 44.1Khz, which just happens to be the sample rate of the included audio sample _audio.bin_ (I've included _audio.wav_ so you can listen normally on your laptop too!).

We then enable the PWM module, telling it to wait for sample data on its FIFO input, and we're good to go. Until we start filling the buffer, no audio will play.

Hopefully you'll notice that we set both channel 0 and channel 1 up similarly. This is because we're working with a stereo sample.

Starting the playback
---------------------
In `playaudio_cpu()` we use the CPU to drive our sample data into the FIFO buffer. The sample data is built into the kernel and referenced exactly as we did with the Bluetooth firmware file back in part7-bluetooth (we still haven't done SD card file access, sorry!).

The code is fairly self-documenting. We essentially check the FIFO buffer isn't full before we send the left channel byte and the right channel byte (stereo, remember!). If we see errors, we clear them as we go.

And that's it... The PWM will pick up the digital data in the buffer and send it, PWM-style (faking an analogue signal), at the right speed (thanks to our clock divisor/PWM range mastery), to the audio jack.

Todo
----
 * Write-up the DMA version `playaudio_dma()` version, which doesn't tie up the CPU but still plays sound!
 * Add a Makefile.gcc (I'm using LLVM Clang these days, so not a priority)
