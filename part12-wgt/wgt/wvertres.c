#include "../include/wgt.h"

#define fastcopy memcpy

void wvertres (short x, short y, short y2, block image)
{
  short width, height, base, yy, temp;
  long fy, ctr2;
  long same2, incr2;

  width = wgetblockwidth (image);
  height = wgetblockheight (image);      /* store width and height */

  if  (y > y2)                         /* swap y's if needed */
  {
    temp = y;
    y = y2;
    y2 = temp;
  }

  incr2 = (float)(y2 - y + 1) / height * 2000;
  /* find increment  */
  fy = y2;
  yy = y;
  ctr2 = 0;
  same2 = 0;
  base = 0;
  while  (yy <= fy)
  {
    if  (incr2 >= 2000)
      while  ((same2 - ctr2 > 999) & (yy < fy))
      {
	fastcopy (&abuf[(yy)*WGT_SYS.xres + x], &image[(base)*width + 2],
		width);
	yy ++;
	ctr2 += 2000;
      }
    else
      while  (same2 < ctr2)
      {
	same2 += incr2;
	base ++;
      }
    if  (yy < fy)
      fastcopy (&abuf[(yy)*WGT_SYS.xres + x], &image[(base)*width + 2],
	      width);
    same2 += incr2;
    ctr2 += 2000;
    base ++;
    yy ++;
  }
}
