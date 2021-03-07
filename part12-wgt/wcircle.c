#include "wgt.h"

static void hplotcircle (short x, short y, short x_center, short y_center)
{
  unsigned short a,b,c,d;
  unsigned short startx, endx, x1, starty, endy, y1;
 
  starty = y;
  endy = y + 1;
  startx = x;
  endx = x + 1;
  a = y + y_center;
  b = y_center - y;
  for  (x1 = startx; x1 < endx; x1++)
  { 
    c = x1 + x_center;
    d = x_center - x1;
    wputpixel(c, a);
    wputpixel(c, b);
    wputpixel(d, b);
    wputpixel(d, a);
  }
  a = x + y_center;
  b = y_center - x;
  for  (y1 = starty; y1 < endy; y1++)
  { 
    c = y1 + x_center;
    d = x_center - y1;
    wputpixel(c, a);
    wputpixel(c, b);
    wputpixel(d, b);
    wputpixel(d, a);
  }
}

void wcircle (short x_center, short y_center, short radius)
{ 
  short x,y,delta;
  y = radius; 
  delta = 3 - 2 * radius;
  x = 0; 
  while  (x < y)
  { 
    hplotcircle (x, y, x_center, y_center);
    if  (delta < 0)
      delta += 4 * x + 6;
    else
    { 
      delta += 4 * (x - y) + 10;
      y--;
    }
    x++;
  } 
  x = y;
  if (y >= 0) 
    hplotcircle (x, y, x_center, y_center);
}

static void fplotcircle (short x, short y, short x_center, short y_center)
{
  unsigned int *ptr;
  short display,display2;

  display = x * 2;
  display2 = x_center - x;
  if  (x + x_center > bx)
    display = bx + 1 - (x_center - x);
  if  (display2 < tx)
  {
    display -= tx - display2;
    display2 = tx;
  }
  if (display > 0)
  {
    if  ((y + y_center <= by) && (y + y_center >= ty))
    {
      ptr = &abuf[(y + y_center)*(WGT_SYS.xres) + display2];
      memset (ptr, currentcolor, display);
    }

    if  ((y_center - y >= ty) && (y_center - y <= by))
    {
      ptr = &abuf[(y_center - y)*(WGT_SYS.xres) + display2];
      memset (ptr, currentcolor, display);
    }
  }
  display = y*2;
  display2 = x_center - y;

  if  (y + x_center > bx)
    display = bx + 1 - (x_center - y);
  if  (display2 < tx)
  {
    display -= tx - display2;
    display2 = tx;
  }
 
  if  (display < 0)
    return;

  if  ((x + y_center <= by) && (x + y_center >= ty))
  {
    ptr = &abuf[(x + y_center) * (WGT_SYS.xres) + display2];
    memset (ptr, currentcolor, display);
  }
 
  if  ((y_center - x >= ty) && (y_center - x <= by))
  {
    ptr = &abuf[(y_center - x) * (WGT_SYS.xres) + display2];
    memset (ptr, currentcolor, display);
  }
}

void wfill_circle (short x_center, short y_center, short radius)
{
  short x,y,delta;
  y = radius;
  delta = 3 - 2 * radius;
  x = 0;
  while  (x < y)
  {
    fplotcircle (x, y, x_center, y_center);
    if  (delta < 0)
      delta += (4 * x + 6);
    else
    {
      delta += 4 * (x - y) + 10;
      y--;
    }
    x++;
  }
  x = y;
  if  (y >= 0)
    fplotcircle (x, y, x_center, y_center);
}
