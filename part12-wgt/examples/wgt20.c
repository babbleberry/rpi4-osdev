#include "wgtspr.h"
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

/* Large example demonstrating many functions from WGT  */

#define MAXEXPLOS 220
#define MAXALIEN 170

void scrolldown (void);		/* scrolls the screen down */
void credits (void);
void changepalette256 (void);
void colorloop (void);
void play (void);

block scr, scr2;
block sprites[21];

typedef struct {
	 int x, y;
	 int dirx, diry;
	 int shootx, shooty;
         int power;
	} spaceship;

typedef struct {
	int x, y;
	int num;
	} explosion;
explosion explos[MAXEXPLOS];

spaceship aliens[MAXALIEN];
int numaliens, aliensleft;

int shoot[10], sx[10], sy[10];
int ship;			/* Sprite to show */
int shipx[MAXALIEN], shipy[MAXALIEN];	/* Ship coordinates */
float incx, incy, yourx, youry;

int cd;
int i, j, k;			/* Loop control */
int scrll = 1;             	/* scrll is a counter for the scroll offset */
int ix = 30, iy = 0, ix2 = 251, iy2 = 199;	/* coords of scrolling area */
int speed;		       	/* the scrolling speed */
				/* try changing speed and sign */
int nextlevel;		       	/* Performing level change = 1 */
int leveltextx;		       	/* X coordinate of next level text */
int level;

color palette[256];

void strrev(char *str)
{
  int i;
  int j;
  unsigned char a;
  unsigned len = strlen((const char *)str);
  for (i = 0, j = len - 1; i < j; i++, j--) {
    a = str[i];
    str[i] = str[j];
    str[j] = a;
  }
}

int itoa(int num, char* str, int base)
{
  int sum = num;
  int i = 0;
  int digit;

  do {
    digit = sum % base;
    if (digit < 0xA) str[i++] = '0' + digit;
    else str[i++] = 'A' + digit - 0xA;
    sum /= base;
  } while (sum);

  str[i] = '\0';
  strrev(str);
  return 0;
}

void initialize_aliens (int numaliens)
/* Sets the initial positions of the aliens when starting a level. */
{
int i;

  for (i = 0; i < numaliens; i++)
    {
     aliens[i].x = rand() % 200 + 30;
     aliens[i].y = rand() % 50;
     aliens[i].dirx = rand() % 8 - 4;
     aliens[i].diry = rand() % 8 - 4;
     aliens[i].power = numaliens / 3;
     aliens[i].shootx = -1;
    }
}

void make_starfield (void)
/* Gets two virtual screens and puts the star sprites on them to make
   a starfield background. */
{
int i;

  scr = wnewblock (0, 0, 319, 199);		/* get two virtual screens */
  scr2 = wnewblock (0, 0, 319, 199);
  wsetscreen (scr2);				/* go to second one */
  wcls (vgapal[0]);				/* clear it */

  for (i = 1; i < 300; i++)
    wputblock (rand() % 320, rand() % 200, sprites[5], 0);
  /* sprite[5] is small stars */

  for (i = 1; i < 5; i++)
    wputblock (rand() % 320, rand() % 200, sprites[6], 0);
  /* sprite[6] is a large star */
}

void play (void)
{
  scrolldown ();			/* Scroll the second
					   screen by copying it to
					   a different offset on
					   screen one. */

  if (but == 1)
     {			/* if you clicked the mouse */
      for (i = 0; i  <9; i++)
	if (shoot[i] == 0)		/* check to see if slot available */
	  {
	   shoot[i] = 1;		/* make it unavailable */
	   sx[i] = yourx + 9;		/* set coords for shot */
	   sy[i] = youry - 7;
	   break;
	  }
     }
 
  for (i = 0; i < 9; i++)     		/* if shot is active */
    {
     if (shoot[i] == 1)			/* then show the sprite */
       {
	wputblock (sx[i], sy[i], sprites[4], 0); /* at the right coords */
	sy[i] -= 8;			    /* make it go up */
	if (sy[i] < 1)		            /* if it is at top, */
	  shoot[i] = 0;		  	    /* make it available again */

	/* Now check to see if it hit an alien by seeing if their
	   rectangular regions overlap. */
	for (j = 0; j < numaliens; j++)
	if ((aliens[j].x != -255) &&
	    (sx[i] > aliens[j].x) && (sx[i] < aliens[j].x + 25) &&
	    (sy[i] > aliens[j].y) && (sy[i] < aliens[j].y + 20))

	  {
	   shoot[i] = 0; /* Turn off that missile */

	   aliens[j].power--;
	   if (aliens[j].power == 0)
	     {
	      aliensleft--;
	      if (aliensleft == 0)
		{
		 nextlevel = 1;
		 leveltextx = ix2 + 100;
		}
	      for (k = 0; k < MAXEXPLOS; k++)  /* Add an explosion */
		{
		 if (explos[k].num == 0)
		    {
		     explos[k].x = aliens[j].x;
		     explos[k].y = aliens[j].y;
		     explos[k].num = 8;
		     break;
		    }
		}
	      aliens[j].x = -255;
	     }
	  } /* Collision */
       } /* Shot active */
    } /* for loop */

  incx = (float)(mx - yourx) / 5.0;  /* Move your spaceship according to the */
  incy = (float)(my - youry) / 5.0;  /* position of the mouse. */
  yourx += incx;
  youry += incy;

  /* Tilt the ship */
  if ((incx < 2) && (incx > -2))
    ship = 2;
  else if (incx > 5)
    ship=3;
  else if (incx < 5)
    ship = 1;

  wsetscreen (scr);
  wputblock (yourx, youry, sprites[ship], 1); /* Put your ship on */

  for (i = 0; i < numaliens; i++)
    {
     if (aliens[i].x != -255) /* Alien is still alive */
       {
	aliens[i].x += aliens[i].dirx; /* Make them move */
	aliens[i].y += aliens[i].diry;

	/* Make them bounce off the walls if they go too far in one direction. */
	if (aliens[i].x <= ix)
	  aliens[i].dirx = rand() % 4;
	else if (aliens[i].x > ix2 - 20)
	  aliens[i].dirx = -(rand() % 4);

	if (aliens[i].y < 0)
	  aliens[i].diry = rand() % 4;
	else if (aliens[i].y > 150)
	  aliens[i].diry = -(rand() % 4);

	if ((aliens[i].shootx == -1) && (rand() % 10 == 1))
	  /* 1 in 10 chance of shooting */
	  {
	   aliens[i].shootx = aliens[i].x + 9;
	   aliens[i].shooty = aliens[i].y + 5;
	  }

	if (aliens[i].power > 0)
	  wputblock (aliens[i].x, aliens[i].y, sprites[7], 1);

	/* Display the alien's missile. */
	if (aliens[i].shootx != -1)
	  {
	   wputblock (aliens[i].shootx, aliens[i].shooty, sprites[8], 1);
	   aliens[i].shooty += 4;
	   if (aliens[i].shooty > 199)
	     aliens[i].shootx = -1;
	  }
       }
    }

  /* Now draw any explosions on the screen. */
  for (i = 0; i < MAXEXPLOS; i++)
    {
     if (explos[i].num > 0)
       {
	explos[i].num++;
	if (explos[i].num > 14)
	  explos[i].num = 0;
	else
	  wputblock (explos[i].x, explos[i].y, sprites[explos[i].num], 1);
       }
    }

  if (nextlevel == 1)
    {
     leveltextx -= 5;

     wouttextxy (leveltextx, 50, NULL, "LEVEL ");

     char levelstr[10];
     itoa(level + 1, levelstr, 10);
     wouttextxy (leveltextx + (6 * 8), 50, NULL, levelstr);

     if (leveltextx < -100)
       {
	level++;
	if (level > MAXALIEN)
	  level = MAXALIEN;
	numaliens = level + 5;
	aliensleft = numaliens;
	initialize_aliens (numaliens);
	nextlevel = 0;
       }
    }

  wait_msec(0x3E9F);
  wcopyscreen (ix, iy, ix2, iy2, scr, ix, iy, NULL);
	/* copy the first screen */
	/* to the base screen */
}

void scrolldown (void)
{
  wcopyscreen (ix, scrll, ix2, iy2, scr2, ix, iy, scr);
  wcopyscreen (ix, iy, ix2, scrll, scr2, ix, iy2 - scrll, scr);
  /* Cut the screen into two sections, then swap them vertically on the
     destination page */

  scrll += speed;
  if ((scrll < iy) && (speed < 0))
    scrll = iy2 + 1 - abs (scrll);
  if ((scrll > iy2) && (speed > 0))
    scrll = abs (iy2 - scrll);
}

void credits (void)
{
  wnormscreen ();
  wtextcolor (vgapal[1]);
  wtextbackground (vgapal[0]);

  cd = 2;
  /* draw a pattern on the screen */
  for (i = 0; i < 320; i++)
    {
     colorloop ();
     wsetcolor (vgapal[cd]);
     wfline (160, 100, i, 0);
    }
  for (i = 0; i < 200; i++)
    {
     colorloop ();
     wsetcolor (vgapal[cd]);
     wfline (160, 100, 319, i);
    }
  for (i = 319; i >= 0; i--)
    {
     colorloop ();
     wsetcolor (vgapal[cd]);
     wfline (160, 100, i, 199);
    }
  for (i = 199; i >= 0; i--)
    {
     colorloop ();
     wsetcolor (vgapal[cd]);
     wfline (160, 100, 0, i);
    }


  changepalette256 ();

  wouttextxy (50, 20, NULL, "Shoot 'Em Up Sprite Example");
  wouttextxy (50, 28, NULL, "Showing what YOU could do with");
  wouttextxy (50, 36, NULL, "the WordUp Graphics Toolkit!");
  wouttextxy (50, 44, NULL, "Use the mouse to move");
  wouttextxy (50, 52, NULL, "Left button shoots");
  wouttextxy (50, 60, NULL, "Right button quits");
  wouttextxy (50, 68, NULL, "Right button to start");
  wsetrgb (1, 63, 63, 63, palette);
  // wfade_in (0, 255, 1, palette);

  do {
    /*
    wcolrotate (2, 106, 0, palette);
    wsetpalette (2, 106, palette);
    */
    wait_msec(0x3E9F);
  } while (!but);

  noclick();
  wcls (vgapal[0]);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* change the palette to the right colours */
void changepalette256 (void)
{
  wsetrgb (0, 0, 0, 0, palette);

  for (i = 2; i < 54; i++)
    wsetrgb (i, i, 0, 0, palette);
  for (i = 54; i < 106; i++)
    wsetrgb (i, 105 - i, 0, 0, palette);

  wsetrgb (253, 60, 60, 60, palette);
  wsetrgb (254, 45, 45, 45, palette);
  wsetrgb (255, 30, 30, 30, palette);
  wsetrgb (1, 63, 63, 63, palette);
}

void colorloop (void)
/* Simple loop for colours on credit screen */
{
  cd++;
  if (cd > 105)
    cd = 2;
}

void wgt20()
{
  set_clock_rate(get_max_clock());
  mem_init();
  vga256 ();                    /* Initializes WGT system       */

  start_core2(minit);           // Start the comms engine (core 2)
  while (!comms_up);            // Wait for comms up

  credits ();

  extern unsigned char _binary_bin_space_spr_start[];
  wloadsprites (palette, &_binary_bin_space_spr_start[0], sprites, 0, 20);
  /* load first 10 sprites */
  wsetpalette (0, 255, palette);

  make_starfield ();

  wnormscreen ();			/* go back to normal screen */
  wcls (vgapal[0]);			/* clear it */
  wbutt (0, 0, 29, 199);		/* make some side panels */
  wbutt (252, 0, 319, 199);
  wtextbackground (vgapal[0]);
  wtextcolor (vgapal[1]);
  wtexttransparent (TEXTFG);

  yourx = 128;
  youry = 170;

  wsetscreen (scr);				/* go to first screen */
  msetbounds (ix, 150, ix2 - 20, 179);

  speed = -4;

  nextlevel = 0;
  level = 1;
  numaliens = level + 5;
  aliensleft = numaliens;

  initialize_aliens (numaliens);

  do {
    play ();
  } while (but != 2);		/* until right button is pressed */

  mdeinit ();                   /* Deinitialize the mouse handler */
  wfreeblock (scr);
  wfreeblock (scr2);
  wfreesprites (sprites, 0, 20);

  wnormscreen();
  wcls(vgapal[0]);
}

void main()
{
  wgt20();
  while (1);
}
