#include "wgt.h"

#define fastcopy memcpy

void wsetscreen (block image)
{
  if  (image == NULL)
  {
    abuf = fbuf;
    WGT_SYS.xres = WGT_SYS.screenwidth;
    WGT_SYS.yres = WGT_SYS.screenheight;
  }
  else
  {
    WGT_SYS.xres = wgetblockwidth (image);       /* Get virtual screen size */
    WGT_SYS.yres = wgetblockheight (image);
    image += 2;
    abuf = image;
  }
  tx = 0;
  ty = 0;
  bx = WGT_SYS.xres - 1;
  by = WGT_SYS.yres - 1;
}

void wnormscreen (void)
{
  abuf = fbuf;
  WGT_SYS.xres = WGT_SYS.screenwidth;
  WGT_SYS.yres = WGT_SYS.screenheight;
  tx = 0;
  ty = 0;
  bx = WGT_SYS.xres - 1;
  by = WGT_SYS.yres - 1;
}

void wcopyscreen (short x, short y, short x2, short y2, block source, short destx, short desty, block dest)
{
  unsigned int swidth, dwidth;
  short width, height, display;
  short ctr;                            /* Y line counter */

  if (source == NULL)
  {
    source = abuf;                      /* Set to visual screen */
    swidth = 320;
  }
  else {
    swidth = *source;
    source += 2;                        /* Skip over width and height */
  }

  width = abs (x - x2) + 1;             /* Width of each row to copy */
  height = abs (y - y2) + 1;            /* Height of area to copy    */

  if  (destx + width > bx)
    display = bx + 1 - destx;
  else display = width;                 /* Clip width */

  if  (height > (by + 1 - desty))
    height = (by + 1 - desty);

  source += x + (y * swidth);          /* Clip height */
  if  (desty < ty)
  {
    source += (ty - desty) * swidth;
    height -= ty - desty;
    desty = ty;
  }                                           /* clip y */

  if  (destx < tx)
  {
    source += tx - destx;
    display -= tx - destx;
    destx = tx;
  }                                           /* clip x */

  if  (dest == NULL)
  {
    dest = abuf;                              /* Set to visual screen */
    dwidth = 320;
  }
  else
  {
    dwidth = *dest;
    dest += 2;
  }

  /* Skip over width & height in image data */
  dest += destx + (desty * dwidth);  /* Find row offset in image data */

  if  ((height > 0) && (display > 0))
    for  (ctr = y; ctr < y + height; ctr++)
    {
      fastcopy (dest, source, display);    /* Copy DISPLAY bytes to dest */
      source += swidth;                    /* Advance one row */
      dest += dwidth;
    }
}
