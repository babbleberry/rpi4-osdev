#include "wgt.h"

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

extern sprite_object s[MAX_SPRITES];

extern block backgroundscreen;          /* Holds the constant background */
extern block spritescreen;              /* Work buffer */

extern short maxsprite;

extern short tempx1, tempx2, tempy1, tempy2;

extern block *sprite_images;

void animate (short spritenum, char *animation_sequence);
void animoff (short spritenum);
void animon (short spritenum);
void copy_sprites (void);
void deinitialize_sprites (void);
void draw_sprites (int movement_multiplier);
void erase_sprites (void);
void expand_dirty_rectangle (int sprite_num, int x, int y, int x2, int y2);
void initialize_sprites (block *sprite_blocks);
void movex (short spritenum, char *movement_sequence);
void movexoff (short spritenum);
void movexon (short spritenum);
void movey (short spritenum, char *movement_sequence);
void moveyoff (short spritenum);
void moveyon (short spritenum);
short overlap (short spritenum_1, short spritenum_2);
void spriteoff (short spritenum);
void spriteon (short spritenum, short xcoord, short ycoord, short arrnumber);
