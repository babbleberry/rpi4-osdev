#include "wgt.h"
#include "wpal.h"

// ######## WGT EXAMPLES ########

void wgt01()
{
    vga256 ();                    /* Initializes WGT system */
    wsetcolor (vgapal[15]);
    wline (0, 0, 1919, 1079);     /* Draw a line */
}

void main()
{
    wgt01();
    while (1);
}
