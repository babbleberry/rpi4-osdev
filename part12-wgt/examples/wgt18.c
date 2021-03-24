#include "wgt.h"
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

void wgt18()
{
  block screen1;  /* one virtual screen */
  block circ[4];  /* and four circle images */
  short y;

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();

  start_core2(minit);         // Start the comms engine (core 2)
  while (!comms_up);          // Wait for comms up

  screen1 = wnewblock (0, 0, 319, 199);

  for (y = 0; y < 4; y++)
    {
     wsetcolor (vgapal[40 + y * 5]);
     wfill_circle (30, 30, 20 + y * 4);
        /* draw a circle with a box cut out in middle */
        /* Note that the circle turns into a square when we increase the radius
           because I'm still grabbing the same area from the wnewblock... */

     wsetcolor (vgapal[0]);
     wbar (20, 20, 40, 40);
     circ[y] = wnewblock (10, 10, 50, 50);  /* get the sprite */
    }

  wsetscreen (screen1);

  do {
    for (y = 0; y < 200; y++)
      {
       wsetcolor (vgapal[y]);
       wline (0, y, 319, y);  /* clear the screen by drawing horz lines (fast) */
      }

    wputblock (mx, my, circ[0], 1);
    wputblock (279 - mx, my, circ[1], 1);
    wputblock (mx, 159 - my, circ[2], 1);
    wputblock (279 - mx, 159 - my, circ[3], 1);
    /* Put four blocks on the screen depending on where the mouse is. */
    /* The first one displayed is the one in the back. */

    wcopyscreen (0, 0, 319, 199, screen1, 0, 0, NULL);
    /* copy the whole screen */
    /* notice how we never use wnormscreen at all! */

  } while (but == 0);

  mdeinit();
  wfreeblock (screen1);
  for (y = 0; y < 4; y ++)
    wfreeblock (circ[y]);
}

void main()
{
    wgt18();
    while (1);
}
