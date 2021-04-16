#include "../include/wgt.h"

void wxorbox (short x, short y, short x2, short y2, unsigned char col)
{
  short i, j;                          /* Loop Control */
  block temp;                          /* Temp pointer to screen */

  if  (y2 < y)
  {
    /* swap y's */
    i = y;
    y = y2;
    y2 = i;
  }

  if  (x2 < x)
  {
    /* swap x's */
    i = x;
    x = x2;
    x2 = i;
  }

  if  (y2 > by) y2 = by;             /* Clip box */
  if  (x2 > bx) x2 = bx;
  if  (y < ty) y = ty;
  if  (x < tx) x = tx;

  temp = &abuf[y*WGT_SYS.xres + x];             /* Move to first offset */

  for  (j = y; j <= y2; j++)
  {
    for  (i = x; i <= x2; i++)
    {
      /* This can be change to perform any logical operation
      on the pixels, such as & or | */
      *temp = *temp ^ col;             /* XOR the pixel on screen with col and put it back on screen */
      temp++;                          /* Go to next pixel */
    }                                  /* x loop */
    temp += (WGT_SYS.xres - 1) - (x2 - x);            /* Advance to next row */
  }                                    /* y loop */
}
