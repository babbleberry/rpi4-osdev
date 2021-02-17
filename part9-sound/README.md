Writing a "bare metal" operating system for Raspberry Pi 4 (Part 9)
===================================================================

Playing sound from the audio jack
---------------------------------
One thing our game is missing is the excitement of sound! Some beeps and squeaks would be a wonderful addition to make the gameplay more compelling. Let's work to do just that!

This code was derived from [Peter Lemon's work](https://github.com/PeterLemon/RaspberryPi/tree/master/Sound/PWM/8BIT/44100Hz/Stereo/CPU), for which I am very grateful. It's been ported to C with some modifications to work on the Raspberry Pi 4 hardware.

Todo
----
 * Write-up the CPU-driven `playaudio_cpu()` code, explaining clocks/PWM etc.
 * Code the DMA version so we don't have to tie up the CPU
 * Add a Makefile.gcc (I'm using LLVM Clang these days, so not a priority)
