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

void wgt29()
{
color p[256];			/* Our palette */
block sprites[1001];		/* Pointers to blocks */

  char *scrolltext="THIS IS A SMALL TEXT SCROLLER CREATED WITH THE WCOPYSCREEN COMMAND.  PRESS A KEY TO END THIS SCROLLER...    ";

int scrnum;			/* Counter for current letter of scroller */
int nextlet, i, j;		/* Counters */

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();			/* Initialize graphics mode */

  for (i = 0; i < 180; i++)	/* Draws a background for the screen */
    {
     wsetcolor (vgapal[i]);
     wline (0, 0, 319, i);
     wline (319, 199, 0, 180 - i);
    }

  wsetcolor (vgapal[0]);	/* Draw with black */
  wbar (0, 189, 319, 199);      /* Clear area off bottom for scroller */

  extern unsigned char _binary_bin_letters_spr_start[];
  unsigned char *lettersspr = &_binary_bin_letters_spr_start[0];
  wloadsprites (p, lettersspr, sprites, 0, 1000);	// Load our blocks 

  do {
    scrnum = 0;	    	        /* Start at first character of scroller string */
    do {
      /* Find pixel width of current letter in string */
      nextlet = wgetblockwidth (sprites[scrolltext[scrnum] + 1]);
      for (j = 0; j <= nextlet + 1; j += 2)
	{
	 wbar (318, 189, 319, 199);	/* Erase right-hand side of scroller area */

	 /* Now copy the last letter on at the end of the string */
	 wputblock (319 - j, 189, sprites[scrolltext[scrnum] + 1], 0);

	 /* Wait for monitor to finish vertical retrace */
         wait_msec(0x3E9F); // Ugly - this needs to sync better... could use the timer I suppose... NITV!

	 /* Shift scroller to the left */
	 wcopyscreen (2, 189, 319, 199, NULL, 0, 189, NULL);
	}
      scrnum++;                         	/* Advance to next letter */
    } while (scrolltext[scrnum + 1] != 0);
  } while (1);

  wfreesprites (sprites, 0, 1000);		/* Free our sprites */
}

void main()
{
    wgt29();
    while (1);
}
