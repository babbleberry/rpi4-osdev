Writing a "bare metal" operating system for Raspberry Pi 4 (Part 11)
====================================================================

Putting it all together
-----------------------
Frankly, I'm unlikely to write much documentation for this part. I'm also only providing a Clang _Makefile_ for now. If you're using gcc, have a go at putting your own _Makefile_ together, referencing the previous parts.

This part simply builds on the work we've done so far, and delivers a new Breakout codebase with:

 * Gameplay running in the foreground on the main CPU core 0
 * Graphics updated in the background on CPU core 1
 * Looped 8-bit music playing in the background on CPU core 2
 * Bluetooth communications managed in the background on CPU core 3

I've taken the opportunity to organise the code a little better and so you'll see some changes to the _Makefile_, and a new directory structure in place. Tidy codebase = tidy mind!

Important takeaway
------------------
As you read through this code, you'll maybe notice that a very different style has emerged. Multi-processing adds a dimension of complexity when coding and requires a total mindset shift.

There's a lot more signalling/semaphores, so the cores can be sure they're doing the right thing at the right time.

You're no longer "dry-running" a single, sequential thread. You're having to spot the potential for bugs/unexpected behaviour from the interplay of four concurrent threads!

_Good luck as you explore the code!_

Click on the picture below to watch a quick video of the game in action. Sorry for the bad audio quality - the game itself sounds great in real life, I promise!

[![A video of the Breakout game in action](https://img.youtube.com/vi/W6fE2q_1voM/0.jpg)](https://www.youtube.com/watch?v=W6fE2q_1voM)
