#include "wgt.h"

void wclip (short x, short y, short x2, short y2)
{
    tx = x;                              /* Upper x limit */
    ty = y;                              /* Upper y limit */
    bx = x2;                             /* Lower x limit */
    by = y2;                             /* Lower y limit */
    /* Stay within screen bounds, no negatives */
    if  (tx < 0) tx = 0;
    if  (ty < 0) ty = 0;
    if  (bx >= WGT_SYS.xres) bx = WGT_SYS.xres - 1;
    if  (by >= WGT_SYS.yres) by = WGT_SYS.yres - 1;
}
