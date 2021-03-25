#include "../include/wgt.h"

void wsetcolor (unsigned int col)
{
    currentcolor = col;
}

void wsetrgb (unsigned char num, unsigned char red, unsigned char green, unsigned char blue, color *pal)
{
    if (red > 255) red = 255;            /* Check for maximum values */
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;

    pal += num;                          /* Adjust pointer to proper index */
    pal->r = red;                        /* Set values for RGB */
    pal->g = green;
    pal->b = blue;
}

void wsetpalette (unsigned char start, unsigned char finish, color *pal)
{
    for (int i = start; i <= finish; i++)
    {
       vgapal[i] = (i << 24) + rgb(pal[i].r, pal[i].g, pal[i].b);
    }
}

void wreadpalette (unsigned char start, unsigned char finish, color *palc)
{
    for  (int i = start; i <= finish; i++)
    {
       palc[i].r = (vgapal[i] >> 16) & 0xFF;
       palc[i].g = (vgapal[i] >> 8) & 0xFF;
       palc[i].b = vgapal[i] & 0xFF;
    }
}
