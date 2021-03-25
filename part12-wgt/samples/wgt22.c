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

#define SPRITES_IN_FILE 50

color palette[256];		
block sprites[SPRITES_IN_FILE+1];
void looper (void);			/* a routine which controls the sprites */

void wgt22()
{
  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();                    /* Initializes WGT system       */

  start_core2(minit);           // Start the comms engine (core 2)
  while (!comms_up);            // Wait for comms up

  extern unsigned char _binary_bin_invader_spr_start[];
  wloadsprites (palette, &_binary_bin_invader_spr_start[0], sprites, 0, SPRITES_IN_FILE);
  /* load first 50 sprites */
  wsetpalette (0, 255, palette);

  initialize_sprites (sprites);					/* initialize them */
  maxsprite = 10;				/* number of sprites on */

/* Spriteon has the following format:
   Sprite number, x coord, y coord, sprite number in array of sprites
   Therefore sprite #1 would be displayed at 160,150 with sprite 1 in the array */

  spriteon (1, 160, 100, 1);                    /* turn on any sprites */
  spriteon (2, 10, 100, 3);                     /* you need */

/* This move will go left 1, for 300 times, and right 1 for 300 times,
   and repeat */

  movex (2, "(1,300,0)(-1,300,0)R");            /* set up any movement */
  movexon (2);                                  /* or animation needed */

/* This animation will animate sprite 2 through a sequence of sprites
   in the sprite array and keep repeating. */

  animate (2,"(3,50)(4,50)(5,50)(4,50)R");
  animon (2);

  do {
    looper ();
  } while (1);

  spriteoff (1);			/* turn off sprites */
  spriteoff (2);
  /* To be safe, turn off all sprites before ending program.
     This will free any memory used from them. */

  wfreesprites (sprites, 0, SPRITES_IN_FILE);	/* free memory */

  mdeinit();
  deinitialize_sprites ();
}

void looper (void)
{
  erase_sprites ();			/* clear the sprites */

  s[1].x = mx;  /* any direct sprite movements must be placed */
  s[1].y = my;  /* between erase_sprites and draw_sprites */
                /* This will set sprite one's coordinate to the mouse
                   coordinates. Move it around! */
                /* notice how sprite #2 moves and animates on its own now!
                   You don't need to change anything to make it move! */

  draw_sprites (1);	/* draw them back on */
  wait_msec(0x3E9F);    /* Try removing this to see how fast the
			   sprite engine really is */
}

void main()
{
    wgt22();
    while (1);
}
