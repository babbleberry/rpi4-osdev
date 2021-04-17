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

void wgt39()
{
  int i;
  int px[256], py[256];
  color p[256];
  int numpix;

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();			/* Initialize graphics mode */

  start_core2(minit);           // Start the comms engine (core 2)
  while (!comms_up);            // Wait for comms up

  numpix = 255;

  extern unsigned char _binary_bin_wgt1_pal_start[];
  wloadpalette (&_binary_bin_wgt1_pal_start[0], p);
  wsetpalette (0, 255, p);

  for (i = 0; i < 256; i++)
    {
     px[i] = 0;
     py[i] = 0;
    }

  do {
    wsetcolor (vgapal[0]);
    wait_msec(0x3E9F); // wait for retrace (ish)

    for (i = numpix; i >= 1; i--)
      {
       wfastputpixel (px[i], py[i]);
       wfastputpixel (319 - px[i], 199 - py[i]);
       px[i] = px[i-1];
       py[i] = py[i-1];
      }

   wsetcolor (vgapal[0]);
   wfastputpixel (px[0], py[0]);
   wfastputpixel (319 - px[0], 199 - py[0]);

   px[0] = mx;
   py[0] = my;

   for (i = 0; i < numpix; i++)
     {
      wsetcolor (vgapal[i]);
      wfastputpixel (px[i], py[i]);
      wfastputpixel (319 - px[i], 199 - py[i]);
     }
  } while (!but);

  mdeinit ();                   /* Deinitialize the mouse handler */
}

void main()
{
    wgt39();
    while (1);
}
