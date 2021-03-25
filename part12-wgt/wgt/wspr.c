#include "../include/wgt.h"

/* The following defines should be altered to suit your program needs */
#define MAX_SPRITES        100
#define MAX_ANIMATION      40
#define MAX_MOVE           15

typedef struct
 {
  unsigned char num;          /* Sprite number shown */
  short x, y;                 /* Coordinates on screen */
  unsigned char on;           /* On/Off, for visibility */

  int ox, oy, ox2, oy2;

  signed char animon;                      /* Animation on/off */
  short animation_images[MAX_ANIMATION];   /* Animation numbers */
  unsigned char animation_speeds[MAX_ANIMATION]; /* Animation speeds */
  signed char current_animation;           /* Current animation counter */
  unsigned char animation_count;           /* Delay count for animation */

  signed char movex_on;                    /* X movement on/off */
  short movex_distance[MAX_MOVE];          /* X distance per frame */
  short movex_number[MAX_MOVE];            /* Number of times to move */
  unsigned char movex_speed[MAX_MOVE];     /* Delay between each movement */
  signed char current_movex;               /* Movement index */
  short current_movex_number;              /* Number of times moved */
  unsigned char movex_count;               /* Delay count for X movement */

  signed char movey_on;                    /* Y movement on/off */
  short movey_distance[MAX_MOVE];          /* Y distance per frame */
  short movey_number[MAX_MOVE];            /* Number of times to move */
  unsigned char movey_speed[MAX_MOVE];     /* Delay between each movement */
  signed char current_movey;               /* Movement index */
  short current_movey_number;              /* Number of times moved */
  unsigned char movey_count;               /* Delay count for Y movement */
 } sprite_object;

sprite_object s[MAX_SPRITES];

block backgroundscreen = NULL;          /* Holds the constant background */
block spritescreen = NULL;              /* Work buffer */

short maxsprite;

int tempx1, tempy1, tempx2, tempy2;

block *sprite_images;


void spriteon (short number, short x, short y, short image);
void spriteoff (short number);

void draw_sprites (int movement_multiplier);
void initialize_sprites (block *sprite_blocks);
void deinitialize_sprites (void);
void erase_sprites (void);

void animate (short spnum, char *str);
void animon (short spnum);
void animoff (short spnum);
void movex (short spnum, char *str);
void movey (short spnum, char *str);
void movexon (short spnum);
void movexoff (short spnum);
void moveyon (short spnum);
void moveyoff (short spnum);
short overlap (short s1, short s2);


void expand_dirty_rectangle (int x, int y, int x2, int y2);
void copy_sprites (void);


void spriteon (short number, short x, short y, short image)
/* Turns a sprite on at (x,y), with sprites[image] */
{
 tempx1 = x;
 tempy1 = y;
 tempx2 = x + wgetblockwidth (sprite_images[image]);
 tempy2 = y + wgetblockheight (sprite_images[image]);

 /* Find the update box */
 if (tempx1 < tx)                       /* X1 */
     tempx1 = tx;
 else
 if (tempx1 >= WGT_SYS.xres)
     tempx1  = WGT_SYS.xres - 1;

 if (tempy1 < ty)                       /* Y1 */
     tempy1 = ty;
 else
 if (tempy1 >= WGT_SYS.yres)
     tempy1  = WGT_SYS.yres - 1;

 if (tempx2 < tx)                       /* X2 */
     tempx2 = tx;
 else
 if (tempx2 >= WGT_SYS.xres)
     tempx2  = WGT_SYS.xres - 1;

 if (tempy2 < ty)                       /* Y2 */
     tempy2 = ty;
 else
 if (tempy2 >= WGT_SYS.yres)
     tempy2  = WGT_SYS.yres - 1;

 s[number].x = x;               /* Set up the coords */
 s[number].y = y;
 s[number].num = image;
 s[number].on = 1;

 s[number].ox = tempx1;         /* Set the dirty rectangle location */
 s[number].oy = tempy1;
 s[number].ox2 = tempx2;
 s[number].oy2 = tempy2;
}



void spriteoff (short number)
/* Turns a sprite off */
{
 if (s[number].on == 1)
     s[number].on = 2;     /* Signals sprite is to be taken off */
}



void erase_sprites (void)
/* Erases all sprites from background screen by copying the dirty
   rectangles */
{
short i;
block curscreen;
short osx, osy;

sprite_object *spriteptr;
int x, y, x2, y2;

curscreen = abuf;
osx = WGT_SYS.xres;
osy = WGT_SYS.yres;

 wsetscreen (spritescreen);         /* Go to background screen */

 spriteptr = s;

 for (i = 0; i <= maxsprite; i++)   /* Loop through all sprites */
  {
   if (spriteptr->on == 1)           /* If sprite is on */
     {
      x = spriteptr->ox;      /* Get the old dirty rectangle coordinates */
      y = spriteptr->oy;
      x2 = spriteptr->ox2;
      y2 = spriteptr->oy2;

      if (x < tx)           /* Clip them, but don't change the original */
	  x = tx;           /* values, because we need them later */
      else if (x > bx)
	  x = bx;
      if (y < ty)
	  y = ty;
      else if (y > by)
	  y = by;

      wcopyscreen (x, y, x2, y2, backgroundscreen, x, y, spritescreen);
     }
     spriteptr++;   /* Next sprite */
  }

 abuf = curscreen;
 WGT_SYS.xres = osx;
 WGT_SYS.yres = osy;
}



void expand_dirty_rectangle (int x, int y, int x2, int y2)
/* Find boundaries of the old and new sprite rectangle */
{
 if (x < tempx1)                /* Compare with */
     tempx1 = x;
 if (x2 > tempx2)
     tempx2 = x2;
 if (y < tempy1)
     tempy1 = y;
 if (y2 > tempy2)
     tempy2 = y2;

 if (tempx1 < tx)                   /* Compare with clipping boundaries */
     tempx1 = tx;

 if (tempx2 > bx)
     tempx2 = bx;

 if (tempy1 < ty)
     tempy1 = ty;

 if (tempy2 > by)
     tempy2 = by;
}


void copy_sprites (void)
{
int i;
sprite_object *spriteptr;
int x, y, x2, y2;

  spriteptr = s;
  for (i = 0; i <= maxsprite; i++)
   {
    if (spriteptr->on > 0)             /* If sprite is on */
    {
     if (spriteptr->on == 2) 
	 spriteptr->on = 0;
     
 
     /* Store these values because they are used more than once */
     x  = spriteptr->x;
     y  = spriteptr->y;
     x2 = x + wgetblockwidth (sprite_images[spriteptr->num]) - 1;
     y2 = y + wgetblockheight (sprite_images[spriteptr->num]) - 1;

     /* Set the dirty rectangle to the current position of the sprite */
     tempx1 = x;
     tempy1 = y;
     tempx2 = x2;
     tempy2 = y2;

     expand_dirty_rectangle (spriteptr->ox, spriteptr->oy,
			     spriteptr->ox2, spriteptr->oy2);

     wcopyscreen (tempx1, tempy1, tempx2, tempy2, spritescreen, tempx1, tempy1, 
		  NULL);

     spriteptr->ox = x;
     spriteptr->oy = y;
     spriteptr->ox2 = x2;
     spriteptr->oy2 = y2;

    }
    spriteptr++;
   }
}




void draw_sprites (int movement_multiplier)
/* Draw the sprites on the sprite screen */
{
short i;
block curscreen;
short osx, osy;
sprite_object *spriteptr;
int move;

 curscreen = abuf;
 osx = WGT_SYS.xres;
 osy = WGT_SYS.yres;

 wsetscreen (spritescreen);  /* Go to sprite screen */


 for (move = 0; move < movement_multiplier; move++)
 {
  spriteptr = s;
  for (i = 0; i <= maxsprite; i++)      /* Loop through all sprites */
  {
    if (spriteptr->on == 1)             /* If sprite is on */
    {
      if (spriteptr->movex_on == 1)      /* X movement on */
      {
	if (spriteptr->movex_count != 0)/* Delay Count not reached 0? */
	  spriteptr->movex_count--;     /* Decrease count */
	else                            /* Get the next move */
	{
	  spriteptr->current_movex_number++; /* Increase # of times moved */
	  if (spriteptr->current_movex_number ==
	      spriteptr->movex_number[spriteptr->current_movex])
	       /* Moved the right number of times yet? */
	    {
	     spriteptr->current_movex++; /* Increase ptr in move array */
	     if (spriteptr->movex_number[spriteptr->current_movex] == - 1)
		/* Repeat move */
		spriteptr->current_movex = 0;
	    else if (spriteptr->movex_number[spriteptr->current_movex]
		     == - 2) /* End move */
	    {
	      spriteptr->current_movex--;
	      spriteptr->movex_on = 0; /* Turn off movement */
	    }
	    spriteptr->current_movex_number = 0;
	      /* Reset number of times moved */
	  }
	  spriteptr->movex_count = spriteptr->movex_speed
		  [spriteptr->current_movex]; /* Get delay */
	  spriteptr->x += spriteptr->movex_distance
		  [spriteptr->current_movex];      /* Update X coord */
	}
      }


      if (spriteptr->movey_on == 1)      /* Y movement on */
      {
	if (spriteptr->movey_count != 0)/* Delay Count not reached 0? */
	  spriteptr->movey_count--;     /* Decrease count */
	else                            /* Get the next move */
	{
	  spriteptr->current_movey_number++; /* Increase # of times moved */
	  if (spriteptr->current_movey_number ==
	      spriteptr->movey_number[spriteptr->current_movey])
	       /* Moved the right number of times yet? */
	    {
	     spriteptr->current_movey++; /* Increase ptr in move array */
	     if (spriteptr->movey_number[spriteptr->current_movey] == - 1)
		/* Repeat move */
		spriteptr->current_movey = 0;
	    else if (spriteptr->movey_number[spriteptr->current_movey]
		     == - 2) /* End move */
	    {
	      spriteptr->current_movey--;
	      spriteptr->movey_on = 0; /* Turn off movement */
	    }
	    spriteptr->current_movey_number = 0;
	      /* Reset number of times moved */
	  }
	  spriteptr->movey_count = spriteptr->movey_speed
		  [spriteptr->current_movey]; /* Get delay */
	  spriteptr->y += spriteptr->movey_distance
		  [spriteptr->current_movey];      /* Update Y coord */
	}
      }



      if (spriteptr->animon == 1)             /* update animation */
      {
	if (spriteptr->animation_count != 0)  /* Delay count at 0? */
	  spriteptr->animation_count--;       /* No, so decrease by one. */
	else    /* Otherwise animate the sprite */
	{
	  spriteptr->current_animation++;     /* Increase animation ptr */
	  if (spriteptr->animation_images[spriteptr->current_animation]
		== - 1) /* Repeat animation */
	    spriteptr->current_animation = 0;

	  if (spriteptr->animation_images[spriteptr->current_animation]
		== - 2) /* End animation */
	  {
	    spriteptr->current_animation--;
	    spriteptr->animon = 0;  /* Turn off animation */
	  }

	  spriteptr->num = spriteptr->animation_images
		[spriteptr->current_animation]; /* Change sprite number */
	  spriteptr->animation_count = spriteptr->animation_speeds
		[spriteptr->current_animation]; /* Reset delay count */
	}
      }

    }
   spriteptr++;
  }
 }


  for (i = 0; i <= maxsprite; i++)      /* Loop through all sprites */
   if ((s[i].on == 1) && (sprite_images[ s[i].num ] != NULL))
     wputblock (s[i].x, s[i].y, sprite_images[ s[i].num ], 1);

  abuf = curscreen;
  WGT_SYS.xres = osx;
  WGT_SYS.yres = osy;
  copy_sprites ();
}





void initialize_sprites (block *sprite_blocks)
/* Set up everything for the sprite engine */
{
short i;

 maxsprite = MAX_SPRITES - 1;
 
 sprite_images = sprite_blocks;

 for (i = 0; i < MAX_SPRITES; i++)    /* Turn off the sprites */
   s[i].on = 0;


 if (spritescreen == NULL)
     spritescreen = wnewblock (0, 0, WGT_SYS.xres - 1, WGT_SYS.yres - 1);
    /* Make a virtual screen for the work buffer */

 if (backgroundscreen == NULL)
     backgroundscreen = wnewblock (0, 0, WGT_SYS.xres - 1, WGT_SYS.yres - 1);
    /* Make a virtual screen for the background screen */

}



void deinitialize_sprites (void)
{
short i;

 for (i = 0; i < MAX_SPRITES; i++)
   s[i].on = 0;

  wfreeblock (spritescreen);
  spritescreen = NULL;

  wfreeblock (backgroundscreen);
  backgroundscreen = NULL;
}



void animate (short spnum, char *str)
/* Parse animation string and put the data into an array */
{
short c, temp1;
short x, in, len, minus;
short change;

  s[spnum].current_animation = 0; /* Reset current animation ptr */
  s[spnum].animation_count = s[spnum].animation_speeds[
	s[spnum].current_animation]; /* Reset delay count */

  x = 0;
  in = 0;

  len = strlen (str);

  /* Parse animation string */

  do {

      do {/* Find the first bracket */
	  c = str[x];
	  x++;
	 } while (c != '(');

      change = 0;
      do {
	  temp1 = 0;
	  minus = 0;
	  do {
	      c = str[x];
	      if ((c != ',') && (c != 'R') && (c != '-') && (c != ')'))
		   temp1 = (temp1 * 10) + (c - 48);
	      if (c == '-')
		  minus = 1;
	      x++;
	     } while ((x != len) && (c != ',') && (c != 'R') && (c != ')'));

	  if (minus == 1)
	      temp1 = - temp1;
	  if (change == 0)
	      s[spnum].animation_images[in] = temp1;
	  else
	      s[spnum].animation_speeds[in] = temp1;
	  change++;
	 } while (c != ')'); /* Until the last bracket */

      in++;
      if (in >= MAX_ANIMATION)
	  in = MAX_ANIMATION-1;

      s[spnum].animation_images[in] = - 2;

      c = str[x];
      if (c == 'R')
	{
	 x = len;
	 s[spnum].animation_images[in] = - 1;
	}
     } while (x != len);
}


void animon (short spnum)
/* Turns animation for a sprite on */
{
    s[spnum].animon = 1;
}


void animoff (short spnum)
/* Turns animation for a sprite off */
{
    s[spnum].animon = 0;
}


/* X movements */

void movexon (short spnum)
/* Turns a sprite's x movement on */
{
    s[spnum].movex_on = 1;
}


void movexoff (short spnum)
/* Turns a sprite's x movement off */
{
    s[spnum].movex_on = 0;
}


void movex (short spnum, char *str)
/* Parses a movement string and places data into movement arrays */
{
short c, temp1;
short x, in, len, minus;
short change;

  s[spnum].current_movex = 0;  /* First x movement */
  s[spnum].movex_count = s[spnum].movex_speed[s[spnum].current_movex];
  s[spnum].current_movex_number = 0;   /* First time it moved */

  x = 0;
  in = 0;

  len = strlen (str);

  do {
      do {
	  c = str[x];
	  x++;
	 } while (c != '(');
      change = 0;

      do {
	  temp1 = 0;
	  minus = 0;
	  do {
	      c = str[x];
	      if ((c != ',') && (c != 'R') && (c != '-') && (c != ')'))
		  temp1 = (temp1*10) + (c - 48);
	      if (c == '-')
		  minus = 1;
	      x++;
	     } while ((x != len) && (c != ',') && (c != 'R') && (c != ')'));

	  if (minus == 1)
	     temp1 = - temp1;
	  if (change == 0)
	     s[spnum].movex_distance[in] = temp1;
	  else if (change == 1)
	     s[spnum].movex_number[in] = temp1;
	  else
	     s[spnum].movex_speed[in] = temp1;

	  change++;
	 } while (c != ')');
	in++;

	if (in == MAX_MOVE)
	    x = len;
	s[spnum].movex_number[in] = - 2;
	c = str[x];
	if (c == 'R')
	{
	    x = len;
	    s[spnum].movex_number[in] = - 1;
	}
    } while (x != len);

}




/* Y movements */

void moveyon (short spnum)
/* Turns a sprite's y movement on */
{
    s[spnum].movey_on = 1;
}


void moveyoff (short spnum)
/* Turns a sprite's y movement off */
{
    s[spnum].movey_on = 0;
}


void movey (short spnum, char *str)
/* Parses a movement string and places data into movement arrays */
{
short c, temp1;
short x, in, len, minus;
short change;

  s[spnum].current_movey = 0;  /* First x movement */
  s[spnum].movey_count = s[spnum].movey_speed[s[spnum].current_movey];
  s[spnum].current_movey_number = 0;   /* First time it moved */

  x = 0;
  in = 0;

  len = strlen (str);

  do {
      do {
	  c = str[x];
	  x++;
	 } while (c != '(');
      change = 0;

      do {
	  temp1 = 0;
	  minus = 0;
	  do {
	      c = str[x];
	      if ((c != ',') && (c != 'R') && (c != '-') && (c != ')'))
		  temp1 = (temp1*10) + (c - 48);
	      if (c == '-')
		  minus = 1;
	      x++;
	     } while ((x != len) && (c != ',') && (c != 'R') && (c != ')'));

	  if (minus == 1)
	     temp1 = - temp1;
	  if (change == 0)
	     s[spnum].movey_distance[in] = temp1;
	  else if (change == 1)
	     s[spnum].movey_number[in] = temp1;
	  else
	     s[spnum].movey_speed[in] = temp1;

	  change++;
	 } while (c != ')');
	in++;

	if (in == MAX_MOVE)
	    x = len;
	s[spnum].movey_number[in] = - 2;
	c = str[x];
	if (c == 'R')
	{
	    x = len;
	    s[spnum].movey_number[in] = - 1;
	}
    } while (x != len);

}



short overlap (short s1, short s2)
/* Tests if two sprites overlap
   Not pixel precise, just checks rectangles */
{
short n1, n2;
short width1, height1;
short width2, height2;

 if ((s[s2].on == 1) && (s[s1].on == 1)) // Make sure both are on
   {
    n1 = s[s1].num; // For easier reading
    n2 = s[s2].num;

    width1 = wgetblockwidth (sprite_images[n1]);
    width2 = wgetblockwidth (sprite_images[n2]);
    height1 = wgetblockheight (sprite_images[n1]);
    height2 = wgetblockheight (sprite_images[n2]);

    if (( s[s2].x >= s[s1].x - width2 ) &&
	( s[s2].x <= s[s1].x + width1 ) &&
	( s[s2].y >= s[s1].y - height2 ) &&
	( s[s2].y <= s[s1].y + height1 ))
	return 1; /* Collision! */
    }
    return 0; /* No collision */
}

