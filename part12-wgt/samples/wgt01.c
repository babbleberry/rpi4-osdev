#include "include/wgt.h"

// ######## WGT EXAMPLES ########

void wgt01()
{
    vga256 ();                    /* Initializes WGT system */
    wsetcolor (vgapal[15]);
    wline (0, 0, 319, 199);     /* Draw a line */
}

void main()
{
    wgt01();
    while (1);
}
