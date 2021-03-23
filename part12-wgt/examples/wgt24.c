#include "wgt.h"
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

void wgt24()
{
  block screen1, screen2;
  color pal[256];
  int i;

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();

  /* Note: This text will be on our second screen - used for wipes */
  wtextcolor (vgapal[15]);                      /* Use index 15 for text */
  wouttextxy (0, 0, NULL, "SCREEN BUFFER #2");
  wouttextxy (0, 50, NULL, "This screen is one of our 2 screens");
  wouttextxy (0, 58, NULL, "used to perform the wipe effect.");
  wouttextxy (46, 100, NULL, "Press a key to perform wipe");
  screen2 = wnewblock (0, 0, 319, 199); /* Allocate memory for screen2 */

  extern unsigned char _binary_bin_wgt1_pal_start[];
  wloadpalette (&_binary_bin_wgt1_pal_start[0], pal);
  wsetpalette (0, 255, pal);
  extern unsigned char _binary_bin_wgt2_blk_start[];
  screen1 = wloadblock (&_binary_bin_wgt2_blk_start[0]);

  getch();

  for (i = 0; i < 200; i++)
    {
     wwipe (0, 0, 319, i, screen1);             /* Wipe screen down from right */
     delay (1);
     wwipe (319, 199,0, 199 - i, screen1);      /* Wipe screen up from left */
     delay (1);
    }
  getch ();

  /* Now wipe on the black screen using horizontal lines meeting at middle */
  for (i = 0; i < 100; i++)
    {
     wwipe (0, i, 319, i, screen2);
     delay (1);
     wwipe (0, 199 - i, 319, 199 - i, screen2);
     delay (1);
    }

  getch ();

  /* Now perform a "circular" wipe, by going around the screen from center */
  for (i = 0; i < 320; i++)
    {
     wwipe (159, 99, i, 0, screen1);
     delay (1);
    }

  for (i = 0; i < 200; i++)
    {
     wwipe (159, 99, 319, i, screen1);
     delay (1);
    }

  for (i = 319; i >= 0; i--)
    {
     wwipe (159, 99, i, 199, screen1);
     delay (1);
    }

  for (i = 199; i >= 0; i--)
    {
     wwipe (159, 99, 0, i, screen1);
     delay (1);
    }
  getch ();

  /* Clear with black screen by wiping top to bottom */
  for (i = 0; i < 200; i++)
    {
     wwipe (0, i, 319, i, screen2);
     delay (1);
    }
  getch ();

  wfreeblock (screen1); /* remember to free that memory */
  wfreeblock (screen2);
}

void main()
{
    wgt24();
    while (1);
}
