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

void wgt16()
{
  short y, tempx, tempy;
  block screen1;

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();                    /* Initializes WGT system       */

  start_core2(minit);           // Start the comms engine (core 2)
  while (!comms_up);            // Wait for comms up

  screen1 = wnewblock (0, 0, 319, 199);

  wsetscreen (screen1);          /* sets to screen1 */
  for (y = 0; y < 200; y++)
    {
     wsetcolor (vgapal[y]);
     wfline (0, 0, 319, y);          /* draw something on another screen */
     wfline (319, 199, 0, y);
    }

  wnormscreen ();                /* make the putblock go onto the default screen */

  do {
    tempx = mx;
    tempy = my;
    wcopyscreen (tempx, tempy, tempx + 19, tempy + 19, screen1,
		 tempx, tempy, NULL);
    /* this means copy a square 20*20 from screen1 to the same spot
       on the default screen.  Move the mouse around and watch the black
       wipe away as screen1 copies over. */

    /* NULL means the default screen. */
  } while (!but);

  mdeinit ();                   /* Deinitialize the mouse handler */
  wfreeblock (screen1);
}

void main()
{
    wgt16();
    while (1);
}
