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

void wgt32()
{
  int i, t, c, b;
  color pal[256];
  block wgt1;
  short top[320];
  short bot[320];

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();			/* Initialize graphics mode */

  extern unsigned char _binary_bin_wgt1_pal_start[];
  wloadpalette (&_binary_bin_wgt1_pal_start[0], pal);
  wsetpalette (0, 255, pal);

  extern unsigned char _binary_bin_wgt1_blk_start[];
  wgt1 = wloadblock (&_binary_bin_wgt1_blk_start[0]);

  wcls (vgapal[0]);			/* Clear screen with black */
  wclip (0, 0, 319, 199);   		/* Use full screen */

  /* Note: wsline merely stores y coordinates of line in array. You may use
	   several calls with various x ranges to make curved lines */

  wsline (0, 199, 319, 0, top);		/* Set array top for diagonal line */
  wsline (0, 199, 319, 199, bot);	/* Array bot is horizontal line */
  wwarp (0, 319, top, bot, wgt1, 0);	/* Warp image between lines */
  getch ();				/* Wait for key */
  // squish it

  wcls (vgapal[0]);			/* Clear screen */
  wsline (0, 100, 100, 0, top);		/* Now create arrow shape */
  wsline (101, 70, 218, 70, top);
  wsline (219, 0, 319, 100, top);

  wsline (0, 100, 100, 199, bot);
  wsline (101, 130, 218, 130, bot);
  wsline (219, 199, 319, 100, bot);
  wwarp (0, 319, top, bot, wgt1, 0);	/* Warp image using arrays */
  getch ();				/* Wait for keypress */
  // make a double arrow

  wcls (vgapal[0]);			/* Clear screen with black */
  do {
    b  =rand() % 100;
    c = (rand() % 100) + 100;
    for (t = 0; t <= 319; t++)
      {
       i = rand() % 2;
       if (i == 0)
	 b++;
       else b--;

       i = rand() % 2;
       if (i == 0)
	 c++;
       else c--;

      if (b > 100)
	b = 100;

      if (b < 0)
	b = 0;
      if (c > 197)
	c = 197;
      if (c < 100)
	c = 100;

      top[t] = b;			/* Create random wavy lines */
      bot[t] = c;
     }
    wwarp (0, 319, top, bot, wgt1, 0);	/* And warp image between them */
    getch ();				/* Wait for keypress */
    wcls (vgapal[0]);			/* Clear screen with black */
   } while (1);				/* End program if Q pressed */

  wfreeblock (wgt1);			/* Free memory from image */
}

void main()
{
    wgt32();
    while (1);
}
