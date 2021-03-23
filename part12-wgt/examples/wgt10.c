#include "wgt.h"
#include "include/mem.h"
#include "include/mb.h"

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

void wgt10()
{
  short x,y;
  block part1;                    /* part of the screen */

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();

  for (y = 40; y >= 4; y--)
    {
     wfill_circle (y + 40, y + 10, y);  /* draw a pattern */
     wsetcolor (vgapal[y + 20]);
    }

  part1 = wnewblock (0, 0, 160, 100);   /* get the circle in a block */
  getch();

  wcls (0);

  for (x = 0; x < 320; x++)
    {
     wsetcolor (vgapal[x % 255]);
     wline (x, 0, x, 199);
    }

  getch();

  wputblock (160, 0, part1, 0);         /* normal mode */
  wflipblock (part1, 0);

  wputblock (160, 100, part1, 1);       /* XRAY mode */
  wflipblock (part1, 1);

  wputblock (0, 100, part1, 0);         /* normal mode */
  wflipblock (part1, 0);

  wputblock (0, 0, part1, 1);           /* XRAY mode */

  wfreeblock (part1);
}

void main()
{
    wgt10();
    while (1);
}
