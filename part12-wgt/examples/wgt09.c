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

void wgt09()
{
  short i, x, y;
  block screen1;                  /* a full screen */
  block part1;                    /* part of the screen */

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();

  for (i = 1; i < 200; i++)
  {
     wsetcolor (vgapal[i]);
     wline (0, 0, 319, i);
     wline (319, 199, 0, 199 - i);
  }

  getch();

  screen1 = wnewblock (0, 0, 319, 199); /* capture the entire screen */
  part1 = wnewblock (0, 0, 150, 150);   /* get a part of the screen */
  /* Note that wnewblock allocates the memory for the block */

  wcls (vgapal[0]);

  do {
    x = rand() % 320;
    y = rand() % 200;
    wputblock (x, y, part1, 0);         /* put the part somewhere */
  } while (!kbhit ());

  getch ();

  wputblock (0, 0, screen1, 0);         /* replace the mess with the */
                                        /* original screen */

  wfreeblock (screen1);                 /* *** make sure to free the memory! */
  wfreeblock (part1);
}

void main()
{
    wgt09();
    while (1);
}
