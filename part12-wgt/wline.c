#include "wgt.h"

void wline (short x, short y, short x2, short y2)
{
    short t;
    short wx = 0, wy = 0;
    short dx, dy;
    short bdx, bdy;
    short incx, incy, addrow;
    unsigned int *temp;
    
    if ((x != x2) && (y != y2))    /* Diagonal lines */
    {
       dx = x2 - x;
       dy = y2 - y;
       t = 0; 
       wx = 0; 
       wy = 0;
       if (dy < 0) 
          incy = - 1; 
       else incy = 1;
       addrow = incy * WGT_SYS.xres;
       if (dx < 0) 
          incx = - 1; 
       else incx = 1;
       bdx = abs (dx);
       bdy = abs (dy);
       temp = &abuf[y * WGT_SYS.xres + x];
       if (bdx > bdy) 
       {
          while (t <= bdx)
          {
 	     if ((x >= tx) && (y >= ty) && (y <= by) && (x <= bx)) /* Clip */
 	        *temp = currentcolor;
 	     wy += bdy;
 	     x += incx;
	     temp += incx;
	     t++;
	     if (wy >= bdx)
	     {
	        wy -= bdx;
	        y += incy;
	        temp += addrow;
	     }
          }
       }
       else 
       {
          while (t <= bdy)
          {
 	     if ((x >= tx) & (y >= ty) & (y <= by) & (x <= bx))
 	        *temp = currentcolor;
	     wx += bdx;
	     if (wx >= bdy) 
    	     {
	        wx -= bdy;
	        x += incx;
	        temp += incx;
	     }
	     y += incy;
	     temp += addrow;
	     t++;
          }
       }
    }
    else if ((y == y2) && (y >= ty) && (y <= by))
    /* Horizontal line */
    {
       if (x > x2)  /* Swap x coords */
       {
          t = x; 
          x = x2; 
          x2 = t;
       }
       if (x < tx) 
          x = tx;   /* Clip the line */
       if (x2 > bx) 
          x2 = bx;
       if (x2 - x >= 0)
          memset (&abuf[y * WGT_SYS.xres + x], currentcolor, x2 - x + 1);
    }
    else if ((x == x2) && (x >= tx) && (x <= bx))
    /* Vertical line */
    {
       if (y > y2)  /* Swap y coords */
       {
          t = y; 
          y = y2; 
          y2 = t;
       }
       if (y < ty) 
          y = ty;   /* Clip the line */
       if (y2 > by) 
          y2 = by;
       temp = &abuf[y * WGT_SYS.xres + x];
       for (t = y; t <= y2; t++)
       {
          *temp = currentcolor;
          temp += WGT_SYS.xres;
       }
    }
}

void whline (short x1, short x2, short y)
{
    int t;
    int length;

    if (x1 > x2)  /* Swap x coords */
    {
       t = x1;
       x1 = x2;
       x2 = t;
    }
    if (x1 < tx)
       x1 = tx;   /* Clip the line */
    if (x2 > bx)
       x2 = bx;
 
    length = x2 - x1 + 1;
    if (length > 0)
       memset (&abuf[y * WGT_SYS.xres + x1], currentcolor, length);
}
