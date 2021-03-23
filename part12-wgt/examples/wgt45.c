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

void wgt45()
{
  block sprites[10];      /* An array of blocks to load the sprites into.
                           Version 5.0 allows for any number to be 
                           loaded in, as long as you pass the same size
                           of the array to the wloadsprites and wfreesprites
                           commands. */
  short i;
  color palette[256];

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();                    /* Initializes WGT system       */

  extern unsigned char _binary_bin_space_spr_start[];
  wloadsprites (palette, &_binary_bin_space_spr_start[0], sprites, 0, 9);
  /* loads the first 10 sprites */
  wsetpalette (0, 255, palette);

  for (i = 0; i < 10; i++)                      /* Display them */
    wputblock (i * 30, 0, sprites[i], NORMAL);
}

void main()
{
    wgt45();
    while (1);
}
