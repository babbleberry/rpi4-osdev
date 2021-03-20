#include "wgtspr.h"
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

block sprites[1001];			/* Pointers to sprites */
int quit;				/* If quit !=0, program quits */
int sprite_toggle = 0;			/* Toggles movement and animation */
char c;					/* Input from keyboard - q quits */

void looper (void);	        /* a routine which controls the sprites */

void wgt25()
{
  color palette[256];		/* Our palette */
  int i;			/* Loop counter */

  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();
  extern unsigned char _binary_bin_mouse_spr_start[];
  wloadsprites (palette, &_binary_bin_mouse_spr_start[0], sprites, 0, 1000);
  wsetpalette (0, 255, palette);
  initialize_sprites (sprites);			  /* initialize them */
  maxsprite = 4;				  /* number of sprites on */

  for (i = 0; i < 200; i++)			  /* draw a background */
    {
     wsetcolor (vgapal[i]);
     wfline (0, i, 159, i);
     wfline (160, 199 - i, 319, 199 - i);
    }

  wcopyscreen (0, 0, 319, 199, NULL, 0, 0, spritescreen);
  wcopyscreen (0, 0, 319, 199, NULL, 0, 0, backgroundscreen);
  /* when using sprites, whatever is on the visual page must be on
     spritescreen and backgroundscreen too! */

  /* Also, you must make sure you turn a sprite on AFTER you draw
     the background or it will leave a black spot where the sprite
     is first shown. */
  spriteon (1, 0, 0, 1);
  animate (1, "(1,30)(2,30)(3,30)(4,30)(3,30)(2,30)R");
  movex (1, "(2,150,0)(0,90,0)(-2,150,0)(0,90,0)R");
  movey (1, "(0,150,0)(2,90,0)(0,150,0)(-2,90,0)R");

  spriteon (2, 160, 0, 1);
  animate (2, "(1,30)(2,30)(3,30)(4,30)R");
  movex (2, "(-1,150,0)(1,300,0)(-1,150,0)R");
  movey (2, "(1,180,0)(-1,180,0)R");

  spriteon (3, 0, 100, 1);
  animate (3, "(1,30)(4,30)R");
  movex (3, "(1,300,1)(-300,1,0)R");
  movey (3, "(0,1,0)");		/* must set a y move since
				   I turn it on below even
				   if it doens't do anything */
  for (i = 1; i < 4; i++)
    {
     animon (i);
     movexon (i);
     moveyon (i);
    }

  do {
    looper ();
  } while (1);

  spriteoff (1);		/* turn off sprite */
  spriteoff (2);		/* turn off sprite */
  spriteoff (3);		/* turn off sprite */
  /* To be safe, turn off all sprites before ending program.
     This will free any memory used from them. */
  deinitialize_sprites();

  wfreesprites (sprites, 0, 1000);		/* free memory */
}

void looper (void)
{
  erase_sprites ();			/* clear the sprites */

  /*
  if (kbhit ())
    {
     c = toupper (getch ());
     if (c == 'Q')
	quit = 1;

     sprite_toggle = !sprite_toggle;
     if (sprite_toggle)
       for (i = 1; i < 4; i++)
	 {
	  movexoff (i);
	  moveyoff (i);
	  animoff (i);
	 }
     else
	for (i = 1; i < 4; i++)
	 {
	  movexon (i);
	  moveyon (i);
	  animon (i);
	 }
    }
  */

  draw_sprites (1);		/* draw them back on */
				/* This loop is required to update sprite
				   positions */
  wait_msec(0x3E9F);    	/* Try removing this to see how fast the
			   	   sprite engine really is */
}

void main()
{
    wgt25();
    while (1);
}
