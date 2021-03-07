#include "wgt.h"

block abuf;                              /* pointer to the active screen */
unsigned int currentcolor;
short tx = 0,ty = 0,bx = 1919,by = 1079; /* clipping variables */

wgt_sys WGT_SYS;

// ######## HELPER FUNCTIONS ########

void *memset(void *dest, int val, unsigned len)
{
    unsigned int *ptr = dest;
    while (len-- > 0)
       *ptr++ = val;
    return dest;
}

int abs(int i)
{
    return i < 0 ? -i : i;
}

int strlen(const char *str) 
{
    const char *s;

    for (s = str; *s; ++s);
    return (s - str);
}

// ######## WGT FUNCTIONS ########

void wcls (unsigned int col)
{
    memset (abuf, col, WGT_SYS.xres * WGT_SYS.yres);
}
