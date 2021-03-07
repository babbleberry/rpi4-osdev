#include "wgt.h"

void wellipse (short x_center, short y_center, short x_radius, short y_radius)
{
  short x, y;
  long aa, aa2, bb, bb2, d, dx, dy;

  x = 0;
  y = y_radius;
  aa = (int)x_radius * (int)x_radius;
  aa2 = 2 * aa;
  bb = (int)y_radius * (int)y_radius;
  bb2 = 2 * bb;

  d = bb - aa * (int)y_radius + aa / 4;
  dx = 0;
  dy = aa2 * (int)y_radius;

  wputpixel (x_center, y_center - y);
  wputpixel (x_center, y_center + y);
  wputpixel (x_center - x_radius, y_center);
  wputpixel (x_center + x_radius, y_center);

  while  (dx < dy)
  {
    if  (d > 0)
    {
      y--;
      dy -= aa2;
      d -= dy;
    }
    x++;
    dx += bb2;
    d += bb + dx;

    wputpixel (x_center + x, y_center + y);
    wputpixel (x_center - x, y_center + y);
    wputpixel (x_center + x, y_center - y);
    wputpixel (x_center - x, y_center - y);
  }

  d += (3 * (aa - bb) / 2 - (dx + dy)) / 2;
  while  (y > 0)
  {
    if  (d < 0)
    {
      x++;
      dx += bb2;
      d += bb + dx;
    }

    y--; dy -= aa2; d += aa - dy;
    wputpixel (x_center + x, y_center + y);
    wputpixel (x_center - x, y_center + y);
    wputpixel (x_center + x, y_center - y);
    wputpixel (x_center - x, y_center - y);
  }
}

void wfill_ellipse (short x_center, short y_center, short x_radius, short y_radius)
{
  short x, y;
  long aa, aa2, bb, bb2, d, dx, dy;

  x = 0;
  y = y_radius;
  aa = (int)x_radius * (int)x_radius;
  aa2 = 2 * aa;
  bb = (int)y_radius * (int)y_radius;
  bb2 = 2 * bb;
  d = bb - aa * (int)y_radius + aa / 4;
  dx = 0;              
  dy = aa2 * (int)y_radius;

  while  (dx < dy)
  {
    if  (d > 0)
    {
      wline (x_center + x, y_center + y, x_center - x, y_center + y);
      wline (x_center + x, y_center - y, x_center - x, y_center - y);
      y--;
      dy -= aa2;
      d -= dy;
    }
    x++;
    dx += bb2;
    d += bb + dx;

  }
  wline (x_center + x, y_center + y, x_center - x, y_center + y);
  wline (x_center + x, y_center - y, x_center - x, y_center - y);

  d += (3 * (aa - bb) / 2 - (dx + dy)) / 2;
  while  (y > 0)
  {
    if  (d < 0)
    {
      x++;
      dx += bb2;
      d += bb + dx;
    }

    y--;
    dy -= aa2;
    d += aa - dy;

    wline (x_center + x, y_center + y, x_center - x, y_center + y);
    wline (x_center + x, y_center - y, x_center - x, y_center - y);
  }
}
