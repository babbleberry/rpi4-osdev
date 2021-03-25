#include "../include/wgt.h"

void wbutt (short x, short y, short x2, short y2)
{
  short ctr;

  if  (y2 < y)
  {
    /* swap y's */
    ctr = y;
    y = y2;
    y2 = ctr;
  }

  if  (x2 < x)
  {
    /* swap x's */
    ctr = x;
    x = x2;
    x2 = ctr;
  }
  wsetcolor (vgapal[0]);
  wrectangle (x - 1, y - 1, x2 + 1, y2 + 1);
  /* Clear area for button */
  wsetcolor (vgapal[254]);
  wbar (x, y, x2, y2);                 /* Draw inner bar */
  wsetcolor (vgapal[255]);
  wline (x2, y, x2, y2);               /* Outline right and bottom edges */
  wline (x2, y2, x, y2);

  wsetcolor (vgapal[253]);             /* Use a different shade */
  wline (x, y, x2, y);                 /* Outline upper and left edges */
  wline (x, y, x, y2);
}
