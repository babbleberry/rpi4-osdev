#include "wgtspr.h"
#include "include/mem.h"
#include "include/multicore.h"

// ######## REQUIRED FUNCTIONS ########

unsigned long state0 = 1000;
unsigned long state1 = 2000;

unsigned long rand(void)
{
    unsigned long s1 = state0;
    unsigned long s0 = state1;

    state0 = s0;
    s1 ^= s1 << 23;
    s1 ^= s1 >> 17;
    s1 ^= s0;
    s1 ^= s0 >> 26;
    state1 = s1;

    return state0 + state1;
}

// ######## STUB FUNCTIONS ########

unsigned int kb = 0;

unsigned int kbhit(void) {
    kb++;
    return kb / 500;
}

void getch(void) {
    wait_msec(0x500000);
    kb = 0;
}

// ######## WGT EXAMPLES ########

void wgt19()
{
  block screen1;	/* one virtual screen */
  block sprites[10];	/* An array of blocks to load the sprites into.
			   Version 4.0 allows for any number to be
			   loaded in, as long as you pass the same size
			   of the array to the wloadsprites and wfreesprites
			   commands. */
  int y, sp;
  color palette[256];

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();                    /* Initializes WGT system       */

  start_core2(minit);           // Start the comms engine (core 2)
  while (!comms_up);            // Wait for comms up

  extern unsigned char _binary_bin_space_spr_start[];
  wloadsprites (palette, &_binary_bin_space_spr_start[0], sprites, 0, 9);
  /* load first 10 sprites */
  wsetpalette (0, 255, palette);

  screen1 = wnewblock (0, 0, 319, 199);
  wsetscreen (screen1);

  sp = 1;	  		/* sprites always start at 1 in the array */

  msetbounds (0, 0, 160, 199);

  do {
    for (y = 0; y < 200; y++)
      {
       wsetcolor (vgapal[y]);
       wfline (0, y, 319, y); /* clear the screen by drawing horz lines (fast) */
      }

    if (sprites[sp] != NULL)
      {
       wputblock (mx, my, sprites[sp], 1);	/* put the block using xray mode at mouse position */

       wflipblock (sprites[sp], 0); /* flip the sprite vertically */
       wresize (mx - 20, my + 50, mx + 20, my + 90, sprites[sp], 1);
       /* resize the sprite using xray mode just below the first one */

       wflipblock (sprites[sp], 0); /* flip it back to normal */
      }

    wcopyscreen (0, 0, 160, 199, screen1, 0, 0, NULL);  /* copy half the screen */

    if (but == 1)  /* Show the next sprite */
      {
       sp++;
       if (sp > 9)
	 sp = 1;
       noclick ();
      }

  } while (but != 2);

  msetbounds (0, 0, 319, 199);
  mdeinit ();                   /* Deinitialize the mouse handler */
  wfreeblock (screen1);
  wfreesprites (sprites, 0, 9);  /* frees all sprites in that array */
}

void main()
{
    wgt19();
    while (1);
}
