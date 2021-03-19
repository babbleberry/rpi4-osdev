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

#define NUM_IN 5       /* number of random points */
#define NUM_OUT 30     /* number of points of bezier curve */
/* Fewer points will make more chunky line */

void wgt33()
{
  tpolypoint inpoint[NUM_IN];   /* Array with the normal curve points */
  tpolypoint outpoint[NUM_OUT]; /* Array with smooth curve points */
  block other;                  /* Pointer to our second screen */
  int i;			/* Loop counter */

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();			/* Initialize graphics mode */

  other = wnewblock (0, 0, 319, 199);	/* Allocate second screen */

  wsetcolor (vgapal[15]);	/* Draw with white */

  do {
    wsetscreen (other);		/* We'll draw on the hidden screen */
    wcls (vgapal[0]);		/* Clear it with black */

    for (i = 0; i < NUM_IN; i++)  /* Randomize the BEZIER control pts */
      {
       inpoint[i].x = rand() % 320;
       inpoint[i].y = rand() % 200;
      }

    wbezier (inpoint, NUM_IN, outpoint, NUM_OUT);	/* Generate line */

    whollowpoly (outpoint, NUM_OUT, 0, 0, OPEN_POLY);
      /* draw the smooth curve */

    wnormscreen ();		/* Reset drawing to visual screen */
    wputblock (0, 0, other, 0);	/* Show the screen */
    getch();
  } while (1);

  wfreeblock (other);		/* Free our screen buffer */
}

void main()
{
    wgt33();
    while (1);
}
