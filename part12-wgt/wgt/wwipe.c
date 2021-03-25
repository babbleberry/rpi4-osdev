#include "../include/wgt.h"

void wwipe (short x, short y, short x2, short y2, block image)
{
  short t, distance;
  short wx, wy, dx, dy, bdx, bdy, incx, incy;
  unsigned int col;
  short swidth;

    swidth = wgetblockwidth (image);

    /* Do the line formula */
    dx = x2 - x;
    dy = y2 - y;
    t = 0; wx = 0; wy = 0;
    if  (dy < 0) incy = - 1; else incy = 1;
    if  (dx < 0) incx = - 1; else incx = 1;
    bdx = abs (dx);
    bdy = abs (dy);
    if  (bdx > bdy) distance = bdx;
    else distance = bdy;
    if  (distance == bdx)
    {
      while  (t <= distance)
      {
	if  ((x >= tx) & (y >= ty) & (y <= by) & (x <= bx))
	  /* inside clipping? */
	{
	  col = image[y * swidth + x + 2];
	  /* get the colour */
	  abuf[y * WGT_SYS.xres + x] = col;
	  /* put the pixel */
	}
	wy += bdy;
	x += incx;
	t++;
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
	if  ((x >= tx) & (y >= ty) & (y <= by) & (x <= bx))
	{
	  col = image[y * swidth + x + 2];
	  abuf[y*WGT_SYS.xres + x] = col;
	}
	wx += bdx;
	if  (wx >= distance)
	{
	  wx -= distance;
	  x += incx;
	}
	y += incy;
	t++;
      }
    }
}
