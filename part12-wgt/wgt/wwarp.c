#include "../include/wgt.h"

void wsline (short x, short y, short x2, short y2, short *y_array)
{
  short t, distance;
  short wx, wy, dx, dy, bdx, bdy, incx, incy;
  
  dx = x2 - x;
  dy = y2 - y;
  t = 0; wx = 0; wy = 0;
  if  (dy < 0) incy = - 1;
  else incy = 1;
  if  (dx < 0) incx = - 1;
  else incx = 1;
  bdx = abs (dx);
  bdy = abs (dy);
  if  (bdx > bdy) distance = bdx;
  else distance = bdy;
  y_array += x; 
  if  (distance == bdx)
  {
    while  (t <= distance)
    {
      if  ((x >= tx) && (y >= ty) && (y <= by) && (x <= bx))
	*y_array = y;
      /* instead of plotting pixels, just store the y value in an array */

      wy += bdy;
      x += incx;
      y_array++;
      t ++;
      if  (wy >= distance)
      {
	wy -= distance;
	y += incy;
      }
    }
  }
  else
  {
    while  (t <= distance)
    {
      if  ((x >= tx) && (y >= ty) && (y <= by) && (x <= bx))
	*y_array = y;
      wx += bdx;
      if  (wx >= distance)
      {
	wx -= distance;
	x += incx;
	y_array++;
      }
      y += incy;
      t ++;
    }
  }
}
 
void wwarp (short sx, short ex, short *tpy, short *bty, block ptr, short mode)
{
  long column;
  int wid;

  short finalwidth;
  short origwidth;
  long xstepper;

  origwidth = wgetblockwidth (ptr);
  /* Get the original width of the block */

  finalwidth = abs (ex - sx) + 1;
  /* Find the new width */

  xstepper = ((long)(origwidth) << 16) / ((long)(finalwidth));
  /* Calculate the amount to add to the source bitmap for every pixel across.
  This is done using a fixed point number by multiplying it by 65536 (<<16)
  and using 0-65535 as a fractional amount. */

  column = 0;
  
  for (wid = sx; wid <= ex; wid++)
  {
   wresize_column (wid, tpy[wid], bty[wid], ptr, column >> 16, mode);
   column += xstepper;
  }
}
