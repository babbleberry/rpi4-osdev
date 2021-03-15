#include "wgt.h"

block abuf;                              /* pointer to the active screen */
block fbuf;                              /* pointer to the hardware framebuffer */
unsigned int currentcolor;
short tx = 0,ty = 0,bx = 319,by = 199; /* clipping variables */

int curx = 0;
int cury = 0;

wgt_sys WGT_SYS;

// ######## HELPER FUNCTIONS ########

void *memset(void *dest, unsigned int val, unsigned len)
{
    unsigned int *ptr = dest;
    while (len-- > 0)
       *ptr++ = val;
    return dest;
}

void *memcpy(void *dest, const void *src, unsigned len)
{
    unsigned int *d = dest;
    const unsigned int *s = src;
    while (len--)
       *d++ = *s++;
    return dest;
}

void *memcpy_xray(void *dest, const void *src, unsigned len)
{
    unsigned int *d = dest;
    const unsigned int *s = src;
    while (len--) {
       if (*s != 0) {
          *d++ = *s++;
       } else {
          d++;
          s++;
       }
    }
 
    return dest;
}

void *memcpy_char(void *dest, const void *src, unsigned len)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (len--)
       *d++ = *s++;
    return dest;
}

int memcmp(char *str1, char *str2, unsigned count)
{
    char *s1 = str1;
    char *s2 = str2;

    while (count-- > 0)
    {
       if (*s1++ != *s2++)
          return s1[-1] < s2[-1] ? -1 : 1;
    }

    return 0;
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

void debugstr(char *str) {
    if (curx + (strlen(str) * 8)  >= 320) {
       curx = 0; cury += 8;
    }
    if (cury + 8 >= 200) {
       cury = 0;
    }
    wtextcolor(vgapal[15]);
    wouttextxy (curx, cury, NULL, str);

    curx += (strlen(str) * 8);
}

void debugcrlf(void) {
    curx = 0; cury += 8;
}

void debugreset(void) {
    curx = 0; cury = 0;
}

void debugch(unsigned char b) {
    unsigned int n;
    int c;
    for(c=4;c>=0;c-=4) {
        n=(b>>c)&0xF;
        n+=n>9?0x37:0x30;
        debugstr((char *)&n);
    }
    debugstr(" ");
}

void debughex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        n=(d>>c)&0xF;
        n+=n>9?0x37:0x30;
        debugstr((char *)&n);
    }
    debugstr(" ");
}

void delay(unsigned int n)
{
    register unsigned long f, t, r;

    // Get the current counter frequency
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));
    // Read the current counter
    asm volatile ("mrs %0, cntpct_el0" : "=r"(t));
    // Calculate expire value for counter
    t+=(f/1000)*n;
    do{asm volatile ("mrs %0, cntpct_el0" : "=r"(r));}while(r<t);
}
