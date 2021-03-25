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

block screen1, screen2;
color pal[256];

void crush (block b1, block b2, int dir)
{
  int q;

  for (q = 199; q >= 0; q -= dir)
    {
     wvertres (0, 0, q, b1);
     wvertres (0, q, 199, b2);
    }
}

void wgt21()
{
  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();

  extern unsigned char _binary_bin_wgt1_pal_start[];
  wloadpalette (&_binary_bin_wgt1_pal_start[0], pal);
  wsetpalette (0, 255, pal);

  extern unsigned char _binary_bin_wgt1_blk_start[];
  screen1 = wloadblock (&_binary_bin_wgt1_blk_start[0]);

  extern unsigned char _binary_bin_wgt2_blk_start[];
  screen2 = wloadblock (&_binary_bin_wgt2_blk_start[0]);

  wputblock (0, 0, screen1, 0);

  do {
    crush (screen1, screen2, 2);
    crush (screen2, screen1, 2);
  } while (1);

  wfreeblock (screen1); /* remember to free that memory */
  wfreeblock (screen2);
}

void main()
{
    wgt21();
    while (1);
}
