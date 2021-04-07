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

void wgt15()
{
  short y;
  block screen1;
  
  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();                    /* Initializes WGT system       */

  start_core2(minit);           // Start the comms engine (core 2)
  while (!comms_up);            // Wait for comms up

  screen1 = wnewblock (0, 0, 119, 129);
  /* Virtual screens can now be any size, as long as they fit within one
     segment (64k) */

  wtextcolor (vgapal[15]);
  wouttextxy (0, 0, NULL, "Click the mouse button in a few seconds");
  wsetscreen (screen1);		/* sets to screen1 */

  for (y = 0; y < 200; y++)
    {
     wsetcolor (vgapal[y]);
     wline (0, 0, 319, y);	/* draw something on another screen */
     wline (319, 199, 0, y);
    }

  while (!but); // wait for a mouse press

  /* now use putblock to show what happened on the other screen */

  wnormscreen ();		/* make the putblock go onto the default screen */
  wputblock (0, 10, screen1, 0);

  wfreeblock (screen1);
  /* remember to free that memory (64004 bytes for a screen) */
}

void main()
{
    wgt15();
    while (1);
}
