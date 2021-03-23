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

color palette[256];             /* Our palette */
block sprites[31];              /* Sprites to be loaded */
short x, y, i;                    /* Counters and position variables */
short chk1, chk2, chk3, chk4;     /* Result of pixel checks */
short blk[10][28] = {             /* Our wall of bricks is defined here */
   { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
   { 0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 },
   { 0,3,4,3,3,3,4,3,3,3,3,3,4,4,4,4,4,3,3,3,3,3,4,4,4,4,4,3 },
   { 0,3,4,3,3,3,4,3,3,3,3,3,4,3,3,3,3,3,3,3,3,3,3,3,4,3,3,3 },
   { 0,3,4,3,3,3,4,3,3,3,3,3,4,3,3,3,3,3,3,3,3,3,3,3,4,3,3,3 },
   { 0,3,4,3,4,3,4,3,3,3,3,3,4,3,4,4,4,3,3,3,3,3,3,3,4,3,3,3 },
   { 0,3,4,3,4,3,4,3,3,3,3,3,4,3,3,3,4,3,3,3,3,3,3,3,4,3,3,3 },
   { 0,3,4,3,4,3,4,3,3,3,3,3,4,3,3,3,4,3,3,3,3,3,3,3,4,3,3,3 },
   { 0,3,4,4,4,4,4,3,3,3,3,3,4,4,4,4,4,3,3,3,3,3,3,3,4,3,3,3 },
   { 0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 }
};

void dobounce (void);           /* Makes a sound when the ball bounces */

float sbx, sby, sp, lx, ly;     /* Speed of ball and position of ball */
float xsp, ysp;                 /* More motion variables */

void looper ();                 /* Main program loop is in this routine */
void hit(short, short);         /* Performs update when ball hits brick */

short hits;                     /* Number of bricks hit */

void wgt60()
{
  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();                    /* Initializes WGT system       */

  wtexttransparent (TEXTFGBG);  /* Turn foreground and background on */

  extern unsigned char _binary_bin_break_spr_start[];
  wloadsprites (palette, &_binary_bin_break_spr_start[0], sprites, 0, 30);
  wsetpalette (0, 255, palette);

  /* Initialize the sprite system and tell it we've got 2 sprites to use */
  initialize_sprites (sprites);
  maxsprite = 2;

  wsetscreen (spritescreen);    /* Draw our game screen on the sprite screen */

  for (y = 0; y < 200; y++)     /* Background is a series of gray lines */
    {
     wsetcolor (vgapal[(y / 8) + 1]);
     wline (0, y, 319, y);
    }

  wsetcolor (vgapal[0]);                 /* Make black playing area */
  wbar (50, 10, 270, 189);

  wsetcolor (vgapal[16]);                /* Outline it */
  wrectangle (49, 9, 271, 190);

  for (x = 1; x < 28; x++)      /* Now draw the bricks */
    for (y = 1; y < 10; y++)
      {
       wputblock (x * 7 + 57, y * 5 + 20, sprites[blk[y][x]], 0);
      }

  /* After it has been drawn, show it on the visual screen too */      
  wcopyscreen (0, 0, 319, 199, spritescreen, 0, 0, NULL);
  wcopyscreen (0, 0, 319, 199, spritescreen, 0, 0, backgroundscreen);
  wnormscreen ();

  getch();

  spriteon (0, 160, 150, 1);    /* Turn on our sprites */
  spriteon (1, 160, 100, 2);

  xsp = 1.1;                    /* Set initial ball speeds */
  ysp = 1.3;

  sbx = xsp;
  sby = ysp;
  lx = 160;                     /* And the initial position */
  ly = 100;

  do {                          /* Now play the game */
    looper ();
  } while (1);			/* Until the right mouse button is clicked */

  deinitialize_sprites ();      /* Deinit the sprite system */
}

void looper(void)
{
  // wretrace ();                  /* Time our updates to the vertical retrace */
  wait_msec(0x3E9F);
  erase_sprites ();             /* Clear sprites from sprite screen */

  if (lx > 267)                 /* Is ball bouncing off right wall? */
    {
     dobounce ();
     lx = 267;
     sbx = -sbx;                 /* Change direction */
    }
  if (lx < 49)                  /* Is ball bouncing off left wall? */
    {
     dobounce ();
     lx = 49;
     sbx = -sbx;                 /* Change direction */
    }
  if (ly < 9)                   /* Is ball bouncing off top wall? */
    {
     dobounce ();
     ly = 9;
     sby = -sby;                 /* Change direction */
    }

  lx += sbx;                    /* Update  ball positions */
  ly += sby;

  s[1].x = (float)lx;
  s[1].y = (float)ly;

  if (s[1].y > 186)             /* Did it get by the user and go off the bottom? */
    {
/*
     for (i = 2000; i >= 200; i--)
       sound (i);
     nosound ();
*/
     lx = 160;                   // Reset it to the starting position
     ly = 100;
    }

  if (overlap (0, 1))           /* Hit the ball with paddle? */
    {
     // sound (900);
     sby = -ysp;                 /* Change speed and direction based on
				    part of paddle hit */
     if (s[1].x > s[0].x + 21)
       sbx = xsp * 4;
     else if (s[1].x > s[0].x + 18)
       sbx = xsp * 2;
     else if (s[1].x > s[0].x + 12)
       sbx = xsp;
     else if (s[1].x > s[0].x + 6)
       sbx = -xsp;
     else if (s[1].x > s[0].x + 3)
       sbx = -xsp * 2;
     else
       sbx = -xsp * 4;
    }

  /* Now check edges of ball for brick contact */
  chk1 = wgetpixel (s[1].x + 3, s[1].y - 1) >> 24;    /* Upper edge */
  chk2 = wgetpixel (s[1].x + 3, s[1].y + 6) >> 24;    /* Lower edge */
  chk3 = wgetpixel (s[1].x - 1, s[1].y + 3) >> 24;    /* Left edge */
  chk4 = wgetpixel (s[1].x + 6, s[1].y + 3) >> 24;    /* Right edge */

  if (chk1 > 28)
    {
     hit (3, -1);
     sby = ysp;
    }
  else if (chk2 > 28)
    {
     hit (3, 6);
     sby = -ysp;
    }
  if (chk3 > 28)
    {
     hit (-1, 3);
     sbx = -sbx;
     lx += 2;
    }
  else if (chk4 > 28)
    {
     hit (6, 3);
     sbx = -sbx;
     lx -= 2;
    }
  // nosound ();                   /* Turn off all sound */
  draw_sprites (1);             /* Draw the sprites again */
}

void hit (short ix, short iy)
{
short x1, x2;
short y1, y2;

  // sound (600);                  /* Beep because we hit something */
  wsetcolor (vgapal[0]);
  /* Loop through all the bricks to see what we hit */
  for (x = 1; x < 28; x++)
    {
     x1 = x * 7 + 57;
     x2 = x1 + 6;
     for (y = 1; y < 10; y++)
       {
	y1 = y * 5 + 20;
	y2 = y1 + 4;
	if ((s[1].x + ix >= x1) && (s[1].x + ix <= x2)
	 && (s[1].y + iy >= y1) && (s[1].y + iy <= y2) && (blk[y][x] != 0))
	  {
	   wsetscreen (spritescreen);
	   wbar (x1, y1, x2, y2);          /* Erase brick */
	   wsetscreen (backgroundscreen);
	   wbar (x1, y1, x2, y2);          /* Erase brick */
	   wnormscreen ();
					   /* And update visual screen */
	   wcopyscreen (x1, y1, x2, y2, spritescreen, x1, y1, NULL);
	   blk[y][x] = 0;                  /* Disable brick in array */
	   hits++;
	   if ((hits % 15) == 0)           /* Increase speed after every 15 hits */
	     {
	      xsp += .1;
	      ysp += .1;
	     }
	  }
       }
    }
}

void dobounce (void)
{
/*
  sound (200);
  nosound ();
*/
}

void main()
{
    wgt60();
    while (1);
}
