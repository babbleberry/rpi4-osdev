#include "../include/wgt.h"

unsigned int wgetpixel (short x, short y)
{
    return abuf[y * WGT_SYS.xres + x];
}

void wputpixel (short x, short y)
{
    if ((y <= by) & (x <= bx) & (y >= ty) & (x >= tx))
       abuf[y * WGT_SYS.xres + x] = currentcolor;
}

void wfastputpixel (short x, short y)
{
    abuf[y * WGT_SYS.xres + x] = currentcolor;
}
