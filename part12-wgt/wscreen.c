#include "wgt.h"

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
