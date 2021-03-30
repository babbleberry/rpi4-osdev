#include "include/wgtspr.h"
#include "include/mem.h"
#include "include/multicore.h"

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

#define SPRITES_IN_FILE 20		/* We're loading 20 sprites */

int i;					/* Generic counter */
color palette[256];			/* Our palette */
block sprites[SPRITES_IN_FILE+1];	/* Pointers to sprites */
int quit;				/* if quit !=0, program quits */

void looper (void);			/* Function to move cursor */

void wgt23()
{
  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();

  start_core2(minit);           // Start the comms engine (core 2)
  while (!comms_up);            // Wait for comms up

  /* Load our sprites from the file. Palette is loaded too */
  extern unsigned char _binary_bin_mouse_spr_start[];
  wloadsprites (palette, &_binary_bin_mouse_spr_start[0], sprites, 0, SPRITES_IN_FILE);
  wsetpalette (0, 255, palette);

  /* Initialize the WGT sprite system */
  initialize_sprites (sprites);
  maxsprite = 1;		/* number of sprites on */

  for (i = 0; i < 200; i++)		/* draw a background */
    {
     wsetcolor (vgapal[i]);
     wline (0, i, 159, i);
     wline (160, 199 - i, 319, 199 - i);
    }

  wcopyscreen (0, 0, 319, 199, NULL, 0, 0, backgroundscreen);
  wcopyscreen (0, 0, 319, 199, NULL, 0, 0, spritescreen);
  /* when using sprites, whatever is on the visual page must be on
     spritescreen too! */

  /* Also, you must make sure you turn a sprite on AFTER you draw
     the background or it will leave a black spot where the sprite
     is first shown. */
  wsetscreen (spritescreen);
  spriteon (1, 160, 100, 1);		/* turn on any sprites */

  animate (1, "(1,30)(2,30)(3,30)(4,30)(3,30)(2,30)R");
  animon (1);
  /* animate the sprite */

  do {
    looper ();		/* Position cursor where mouse is */
  } while (!quit);

  spriteoff (1);			/* turn off sprites */
  /* To be safe, turn off all sprites before ending your program.
     This will free any memory used from them. */

  deinitialize_sprites ();
  mdeinit ();                   /* Deinitialize the mouse handler */
  wfreesprites (sprites, 0, SPRITES_IN_FILE);	/* free memory */
}

void looper (void)
{
  erase_sprites();	   /* clear the sprites */

  s[1].x = mx;		   /* any direct sprite movements must be placed */
  s[1].y = my;		   /* between erasespr and drawspr */
  /* This will place sprite number 1 where the mouse cursor is. */

  draw_sprites (1);	   /* draw them back on */
  wait_msec(0x3E9F);
  if (but)
    quit = 1;
}

void main()
{
    wgt23();
    while (1);
}
