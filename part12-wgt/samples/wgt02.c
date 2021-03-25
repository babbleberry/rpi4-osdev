#include "include/wgt.h"

// ######## REQUIRED FUNCTIONS ########

unsigned long state0 = 1;
unsigned long state1 = 2;

unsigned long rand(void)
{
    unsigned long s1 = state0;
    unsigned long s0 = state1;

    state0 = s0;
    s1 ^= s1 << 23;
    s1 ^= s1 >> 17;
    s1 ^= s0;
    s1 ^= s0 >> 26;
    state1 = s1;

    return state0 + state1;
}

// ######## STUB FUNCTIONS ########

unsigned int kb = 0;

unsigned int kbhit(void) {
    kb++;
    return kb / 500;
}

void getch(void) {
    wait_msec(0x500000);
}

// ######## WGT EXAMPLES ########

void wgt02()
{
    short x;
    short y;
    short i;

    vga256 ();
    wcls (vgapal[0]);             /* Clear screen with color 0    */

    /* Put randomly coloured pixels in random screen coordinates  */
    do {
       wsetcolor (vgapal[rand () % 255]);
       x = rand () % 320;
       y = rand() % 200;
       wputpixel (x, y);
    } while (kbhit () == 0);
    getch ();
 
    wcls (vgapal[0]);             /* Clear screen with color 0    */
    wsetcolor (vgapal[10]);       /* Now we will draw with #10    */
    for (x = 0; x < 320; x++)
       for (y = 0; y < 200; y++)
           wfastputpixel (x, y);  /* Fast due to no clipping checking */
 
    getch ();
    wcls (vgapal[0]);             /* Clears screen with color 0   */
 
    /* Put randomly coloured pixels in the top left corner of the screen  */
    for (i = 0; i < 15000; i++)
    {
       wsetcolor (vgapal[rand () % 255]);
       x = rand () % 160;
       y = rand () % 100;
       wputpixel (x, y);
    }
 
    /* Now use wgetpixel to read image off screen */
    for (y = 0; y < 100; y++)
       for (x = 0; x < 160; x++)
       {
          wsetcolor (wgetpixel (x, y));
          wputpixel (x + 160, y + 100);
       }
 
    getch ();

    while (1);
}

void main()
{
    wgt02();
    while (1);
}
