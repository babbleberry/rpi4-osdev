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

void wait_msec(unsigned int n)
{
    register unsigned long f, t, r;

    // Get the current counter frequency
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));
    // Read the current counter
    asm volatile ("mrs %0, cntpct_el0" : "=r"(t));
    // Calculate expire value for counter
    t+=((f/1000)*n)/1000;
    do{asm volatile ("mrs %0, cntpct_el0" : "=r"(r));}while(r<t);
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

  for (y = 216; y >= 4; y--)
    {
     wfill_circle (y + 40, y + 10, y);  /* draw a pattern */
     wsetcolor (vgapal[(y % 235) + 20]);
    }

  part1 = wnewblock (0, 0, 960, 540);   /* get the circle in a block */
  getch();

  wcls (0);

  for (x = 0; x < 1920; x++)
    {
     wsetcolor (vgapal[x % 255]);
     wline (x, 0, x, 1079);
    }

  getch();

  wputblock (960, 0, part1, 0);         /* normal mode */
  wflipblock (part1, 0);

  wputblock (960, 540, part1, 1);       /* XRAY mode */
  wflipblock (part1, 1);

  wputblock (0, 540, part1, 0);         /* normal mode */
  wflipblock (part1, 0);

  wputblock (0, 0, part1, 1);           /* XRAY mode */

  wfreeblock (part1);
}

void main()
{
    wgt10();
    while (1);
}
