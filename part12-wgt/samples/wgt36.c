#include "include/wgt.h"
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

#define NUM_IN 5       /* number of random points */
#define NUM_OUT 30     /* number of points of bezier curve */
/* Fewer points will make more chunky line */

int ox, oy;  /* old mouse coordinates */
int i;

void wgt36()
{
  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();			/* Initialize graphics mode */

  start_core2(minit);           // Start the comms engine (core 2)
  while (!comms_up);            // Wait for comms up

  for (i = 0; i < 200; i++) /* Draw a background */
    {
     wsetcolor (vgapal[i]);
     wline (0, i, 319, i);
    }

  do {  /* Draws a filled xorbox using mouse coordinates as one corner. */
   ox = mx;
   oy = my;
   wxorbox (50, 50, ox, oy, 128);    /* Draw the box */
   while ((mx == ox) && (my == oy) && (!but));  /* Do nothing while mouse is
						  stationary. */
   wxorbox (50, 50, ox, oy, 128);
   /* Erase the box by drawing the same thing */
  } while (but == 0);

  noclick ();

  do {  /* Draws a hollow rubber box using mouse coordinates as one corner. */
   ox = mx;
   oy = my;
   wxorbox (50, 50, ox, 50, 128);
   wxorbox (ox, 50, ox, oy, 128);    /* Draw the box */
   wxorbox (50, oy, ox, oy, 128);
   wxorbox (50, 50, 50, oy, 128);
   while ((mx == ox) && (my == oy) && (!but));  /* Do nothing while mouse is
						   stationary. */
   wxorbox (50, 50, ox, 50, 128);
   wxorbox (ox, 50, ox, oy, 128);    /* Erase the box */
   wxorbox (50, oy, ox, oy, 128);
   wxorbox (50, 50, 50, oy, 128);
  } while (but == 0);
  mdeinit ();                   /* Deinitialize the mouse handler */
}

void main()
{
    wgt36();
    while (1);
}
