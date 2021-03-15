#include "wgt.h"

#define fastcopy memcpy

void wskew (short x, short y, block image, short degrees)
{
  float newx, offs, slope;
  long lnewx, lslope;
  /* long values converted from floats (speed up) */
  short ctr;                             /* y ctr */
  short hgt, wid, display, shx, of2;
  short onewx;

  hgt = wgetblockheight (image);       /* Find height of block */
  wid = wgetblockwidth (image);        /* Find width of block */
  slope = (float)degrees / 45;         /* Calculate slope of new image */
  offs = ((float)hgt / 2)*slope;       /* Find distance to shift horiz. */
  newx = (float)x + offs;              /* Calculate new x position */
  onewx = newx;                        /* Store it as integer for clipping */
  lnewx = (float)newx * 2000;          /* Convert floating pt to long int */
  lslope = (float)slope * 2000;        /* Convert floating pt to long int */

  /* Draw each row of image */
  for  (ctr = y; ctr < y + hgt - 1; ctr++)
  {
    if  (onewx + wid > bx) display = bx + 1 - onewx;
    /* Clip width */
    else display = wid;
    if  (onewx < tx)                   /* See if image overlaps left clip */
    {
      of2 = tx - onewx;                /* Find offset into bitmap */
      shx = tx;                        /* Set to draw starting here */
      display = display - of2;         /* New width to draw */
    }
    else
    {
      shx = onewx;                     /* X value is fine */
      of2 = 0;                         /* Use entire bitmap */
    }
    /* If width to display >0, and <= original width, and
    we are within top and bottom Y clipping boundaries:
    Copy row of bitmap to visual screen, using DISPLAY contiguous
    bytes.
    */
    if  ((display > 0) & (display <= wid) & (ctr >= ty) & (ctr <= by))
      fastcopy (&abuf[(ctr)*WGT_SYS.xres + shx], &image[(ctr - y)*wid + 2
	+ of2], display);
    lnewx = lnewx - lslope;
    /* Find X position after shifting next row */
    onewx = lnewx / 2000;
    /* Calculate integer value from long int   */
  }
}
