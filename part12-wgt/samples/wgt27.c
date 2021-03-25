#include "include/wgt.h"
#include "include/mem.h"

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

void wgt27()
{
  block skewit;                   /* Pointer to our block */
  color palette[256];             /* Our palette */
  int i = 0;                      /* Loop counter */

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();

  wreadpalette (0, 255, palette); /* Store our current palette */

  wcls (vgapal[0]);               /* Clear screen with black */

  for (i = 100; i > 0; i--)       /* Draw 100 filled circles */
    {
     wsetcolor (vgapal[i]);
     wfill_circle (160, 100, i);
    }

  getch();

  wsetcolor (vgapal[0]);                /* Use black as active color */
  wbar (0, 0, 104, 199);                /* Draw two solid rectangles */
  wbar (216, 0, 319, 199);
  skewit=wnewblock (100, 40, 220, 160); /* Grab a block for skewing */

  getch();

  wcls (vgapal[0]);                     /* Clear screen with black */
  do {
    for (i = -100; i < 100; i += 2)     /* Skew image 2 pixels at a time */
      {
       wskew (100, 40, skewit, i);
       delay(2);
      }

    for (i = 100; i > -100; i -= 2)     /* Skew image back to starting pos */
      {
       wskew (100, 40, skewit, i);
       delay(2);
      }
   } while (1);
}

void main()
{
    wgt27();
    while (1);
}
