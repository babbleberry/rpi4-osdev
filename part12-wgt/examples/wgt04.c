#include "wgt.h"

#define MAX_RADIUS 100

// ######## REQUIRED FUNCTIONS ########

unsigned long state0 = 1;
unsigned long state1 = 2;

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

void wgt04()
{
  color pal[256];
  int x, y, x2, y2, col, ctr;

  vga256 ();                    /* Start graphics mode */

  wtextcolor (vgapal[15]);      /* Text will be white */
  ctr = 0;                      /* Start counter for first primitive */

  do {
    wclip (0, 0, 1919, 1079);                   /* Clip to full screen */
    wouttextxy (230, 0, NULL, "WGT DEMO 4");    /* Display text */
    switch (ctr)                                /* Show primitive type */
    {
      case 0 : wouttextxy (0, 0, NULL, "Now using WLINE"); break;
      case 1 : wouttextxy (0, 0, NULL, "Now using WFLINE"); break;
      case 2 : wouttextxy (0, 0, NULL, "Now using WRECTANGLE"); break;
      case 3 : wouttextxy (0, 0, NULL, "Now using WBAR"); break;
      case 4 : wouttextxy (0, 0, NULL, "Now using WCIRCLE"); break;
      case 5 : wouttextxy (0, 0, NULL, "Now using WFILL_CIRCLE"); break;
      case 6 : wouttextxy (0, 0, NULL, "Now using WELLIPSE"); break;
      case 7 : wouttextxy (0, 0, NULL, "Now using WFILL_ELLIPSE"); break;
      case 8 : wouttextxy (0, 0, NULL, "Now using WSTYLELINE"); break;
      case 9 : wouttextxy (0, 0, NULL, "Now using WBUTT");
               wreadpalette (0, 255, pal);
               wsetrgb (253, 60, 60, 60, pal);
               wsetrgb (254, 50, 50, 50, pal);
               wsetrgb (255, 40, 40, 40, pal);
               wsetpalette (0, 255, pal);
               break;
    }
    wclip (0, 8, 1919, 1079);     /* Clip all primitives below text line */

    do {
      x = rand() % 1920;         /* Randomize first point  -  (x,y)   */
      y = rand() % 1080;
      x2 = rand() % 1920;        /* Randomize second point -  (x2,y2) */
      y2 = rand() % 1080;
      col = rand() % 256;       /* Pick a color index to use */
      wsetcolor (vgapal[col]);          /* Now use it */

      switch (ctr)              /* Perform primitive */
        {
         case 0 : wline (x, y, x2, y2); break;
         case 1 : wfline (x, rand() % 1072 + 8, x2, rand() % 1072 + 8); break;
         case 2 : wrectangle (x, y, x2, y2); break;
         case 3 : wbar (x, y, x2, y2); break;
         case 4 : wcircle (x, y, rand() % MAX_RADIUS); break;
         case 5 : wfill_circle (x, y, rand() % 100); break;
         case 6 : wellipse (x, y, rand() % MAX_RADIUS,
                            rand() % MAX_RADIUS); break;
         case 7 : wfill_ellipse (x, y, rand() % MAX_RADIUS,
                                 rand() % MAX_RADIUS); break;
         case 8 : wstyleline (x, rand() % 1072 + 8, x2, rand() % 1072 + 8,
                              rand() ); break;
         case 9 : wbutt (x, y, x2, y2); break;
        }
    } while (!kbhit ());        /* Stop when key is pressed */

    getch ();                   /* Get key from buffer */
    wcls (vgapal[0]);                   /* Clear screen with black */
    ctr++;                      /* Increment counter to next primitive */
  } while (ctr < 10);           /* Have we done all 10 ? */
}

void main()
{
    wgt04();
    while (1);
}
