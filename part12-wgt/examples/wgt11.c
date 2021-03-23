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

#define TIMERSPEED 60
int timer;
int size;
int direction = -1;

void timer_routine (void)
{
  timer++;
}

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

  getch ();
  wcls (vgapal[0]);

  timer = 0;
  size = 50;
  winittimer ();
  wstarttimer (timer_routine, TIMERSPEED);

  wclip (0, 0, 319, 199);
  do {
    if (direction > 0)
      {
       size += timer * 2;
       timer = 0;
       if (size > 50)
         direction = -1;
      }   
    else 
      {
       size -= timer * 2;
       timer = 0;
       if (size < -1000)
         direction = 1;
      }   

    wresize (size, size, 319 - size, 199 - size, part1, NORMAL);
  
  } while (1);
  getch ();

  wstoptimer (); 
  wdonetimer ();

  wfreeblock(part1);
}

void main()
{
    wgt11();
    while (1);
}
