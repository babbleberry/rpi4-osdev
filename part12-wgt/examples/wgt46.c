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

int loopctr;

void timerctr(void)
{
  loopctr++;
}

void wgt46()
{
  short row;

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();                    /* Initializes WGT system       */
  wcls (vgapal[0]);             /* Clear screen with color 0    */

  row = 0;                      /* Start drawing at top row of screen */
  loopctr = 0;
  winittimer ();
  wstarttimer (timerctr, 100);  /* Set timer to 100th of a second accuracy */
  do {
    wsetcolor (vgapal[rand () % 256]);  /* Pick a random color */
    whline (0, 319, row++);     /* Draw the line */
    if (row > 199)              /* Loop at end of screen */
      row = 0;
  } while (loopctr < 500);       

  /* As soon as we reach 500 hundredths of a second (5 secs), end the routine
     and reset the video mode */
  wstoptimer ();
  wdonetimer ();
}

void main()
{
    wgt46();
    while (1);
}
