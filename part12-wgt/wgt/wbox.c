#include "../include/wgt.h"

void wrectangle (short x, short y, short x2, short y2)
{
  wline (x, y, x2, y);
  wline (x2, y2, x, y2);
  wline (x, y, x, y2);
  wline (x2, y, x2, y2);
}

void wbar (short x, short y, short x2, short y2)
{
  short ctr,len;
  unsigned int *temp;

  if (y2 < y)
    {
     /* swap y's */
     ctr = y;
     y = y2;
     y2 = ctr;
    }

  if (x2 < x)
    {
     ctr = x;
     x = x2;
     x2 = ctr;
    }

  if ((y <= by) & (y2 >= ty) & (x <= bx) & (x2 >= tx))
    /* If anything is within clipping */
    {
     if (y2 > by) y2 = by;             /* Clip bar */
     if (x2 > bx) x2 = bx;
     if (y < ty) y = ty;
     if (x < tx) x = tx;

     len = x2 - x + 1;                  /* Find number of pixels to set */
     if (len > 0)
       {
        temp = &abuf[y*WGT_SYS.xres + x];
        for (ctr = y; ctr <= y2; ctr++)
          {
           memset (temp, currentcolor, len);
           /* Draw a horizontal line */
           temp += WGT_SYS.xres;                   /* Go to next row */
          }
       }
    }
}
