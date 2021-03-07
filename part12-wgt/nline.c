#include "wgt.h"

void wfline (short x, short y, short x2, short y2)
{
  short t, distance;
  short wx = 0, wy = 0, dx, dy, bdx, bdy, incx, incy;
  unsigned int *temp;

  if  ((y == y2) & (y >= ty) & (y < by + 1))
    {
     if  (x > x2)
       {
	t = x;    /* swap */
	x = x2;
	x2 = t;
       }

     if  (x < tx)
       x = tx;
     if  (x2 > bx)
       x2 = bx;
     if (x2 - x + 1 > 0)
       memset (&abuf[y * WGT_SYS.xres + x], currentcolor, x2 - x + 1);
    }
  else if ((x == x2) & (x >= tx) & (x < bx + 1))
    {
     if (y > y2)
	{
	 t = y;
	 y = y2;
	 y2 = t;
	}
     if (y < ty)
       y = ty;
     if  (y2 > by)
       y2 = by;

     temp = &abuf[y*WGT_SYS.xres + x];
     for (t = y; t <= y2; t++)
       {
	*temp = currentcolor;
	temp += WGT_SYS.xres;
       }
    }
  else
    {
     dx = x2 - x;
     dy = y2 - y;
     t = 0;
     wx = 0;
     wy = 0;

     if  (dy < 0)
       incy = - 1;
     else incy = 1;

     if  (dx < 0)
       incx = - 1;
     else incx = 1;

     bdx = abs (dx);
     bdy = abs (dy);
     if  (bdx > bdy)
       distance = bdx;
     else distance = bdy;

     temp = &abuf[y * WGT_SYS.xres + x];
     if  (distance == bdx)
       {
	while  (t <= distance)
	  {
	   *temp = currentcolor;
	   wy += bdy;
	   x += incx;
	   temp += incx;
	   t++;
	   if  (wy >= distance)
	     {
	      wy -= distance;
	      y += incy;
	      temp += incy * WGT_SYS.xres;
	     }
	  }
       }
     else
       {
	while  (t <= distance)
	  {
	   *temp = currentcolor;
	   wx += bdx;
	   if  (wx >= distance)
	     {
	      wx -= distance;
	      x += incx;
	      temp += incx;
	     }
	   y += incy;
	   temp += incy * WGT_SYS.xres;
	   t++;
	  }
       }
    }
}

void wstyleline (short x, short y, short x2, short y2, unsigned short style)
{
  short lc;
  short t, distance;
  short wx = 0, wy = 0, dx, dy, bdx, bdy, incx, incy;
  unsigned int *temp;

  dx = x2 - x;
  dy = y2 - y;
  t = 0;
  wx = 0;
  wy = 0;
  lc = 0;

  if  (dy < 0)
    incy = - 1;
  else incy = 1;

  if  (dx < 0)
    incx = - 1;
  else incx = 1;

  bdx = abs (dx);
  bdy = abs (dy);

  if  (bdx > bdy)
    distance = bdx;
  else distance = bdy;

  temp = &abuf[y * WGT_SYS.xres + x];

  if  (distance == bdx)
    {
     while  (t <= distance)
       {
	if  ((style >> lc) % 2)
	  *temp = currentcolor;
	lc++;
	if (lc > 15)
	  lc = 0;
	wy += bdy;
	x += incx;
	temp += incx;
	t++;
	if (wy >= distance)
	  {
	   wy -= distance;
	   y += incy;
	   temp += incy * WGT_SYS.xres;
	  }
       }
    }
  else
    while  (t <= distance)
      {
       if  ((style >> lc) % 2)
	 *temp = currentcolor;
       /* Only draw a pixel if the bit is set */
       lc++;
       if (lc > 15)
	 lc = 0;
       wx += bdx;
       if (wx >= distance)
	 {
	  wx -= distance;
	  x += incx;
	  temp += incx;
	 }
       y += incy;
       temp += incy * WGT_SYS.xres;
       t++;
      }
}
