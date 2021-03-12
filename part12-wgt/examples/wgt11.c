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

void wgt11()
{
  block part1;                    /* part of the screen */
  color pal[256];

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();

  extern unsigned char _binary_bin_wgt1_pal_start[];
  unsigned char *newpal = &_binary_bin_wgt1_pal_start[0];
  wloadpalette (newpal, pal);
  wsetpalette (0, 255, pal);

  extern unsigned char _binary_bin_wgt1_blk_start[];
  unsigned char *newblk = &_binary_bin_wgt1_blk_start[0];
  part1 = wloadblock (newblk);

  wputblock (0, 0, part1, 0);

  wfreeblock(part1);
}

void main()
{
    wgt11();
    while (1);
}
