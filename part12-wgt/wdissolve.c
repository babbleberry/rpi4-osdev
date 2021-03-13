#include "wgt.h"

void wdissolve (block sourceimage, short *pattern, short speed)
{
  short j, k, l, t, a, b;
  unsigned short addr;

  t = *pattern;
  pattern++;

  for  (j = 0; j < t; j++)
  {
    a = *pattern;
    pattern++;
    b = *pattern;
    pattern++;
    delay (speed);
    for  (k = 0; k < WGT_SYS.yres; k += 16)
      /* Since the fade pattern matrix is 16x16, we change every
      16th pixel at the same time. */
    {
      for  (l = 0; l < WGT_SYS.xres; l += 16)
      {
        if  ((k + b < by + 1) & (l + a < bx + 1) & (k + b >= ty)
            & (l + a >= tx))
          /* Make sure the pixel is within clipping */
        {
          addr = ((k + b) * WGT_SYS.xres + l + a);
          abuf[addr] = sourceimage[addr + 2];
        }
      }
    }
  }

}
