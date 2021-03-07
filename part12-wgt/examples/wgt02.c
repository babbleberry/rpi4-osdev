#include "wgt.h"
#include "wpal.h"

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

void wait_msec(unsigned int n)
{
    register unsigned long f, t, r;

    // Get the current counter frequency
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));
    // Read the current counter
    asm volatile ("mrs %0, cntpct_el0" : "=r"(t));
    // Calculate expire value for counter
    t+=((f/1000)*n)/1000;
    do{asm volatile ("mrs %0, cntpct_el0" : "=r"(r));}while(r<t);
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
       wsetcolor (vgapal[rand () % 256]);
       x = rand () % 1920;
       y = rand() % 1080;
       wputpixel (x, y);
    } while (kbhit () == 0);
    getch ();
 
    wcls (vgapal[0]);             /* Clear screen with color 0    */
    wsetcolor (vgapal[10]);       /* Now we will draw with #10    */
    for (x = 0; x < 1920; x++)
       for (y = 0; y < 1080; y++)
           wfastputpixel (x, y);  /* Fast due to no clipping checking */
 
    getch ();
    wcls (vgapal[0]);             /* Clears screen with color 0   */
 
    /* Put randomly coloured pixels in the top left corner of the screen  */
    for (i = 0; i < 15000; i++)
    {
       wsetcolor (vgapal[rand () % 256]);
       x = rand () % 960;
       y = rand () % 540;
       wputpixel (x, y);
    }
 
    /* Now use wgetpixel to read image off screen */
    for (y = 0; y < 540; y++)
       for (x = 0; x < 960; x++)
       {
          wsetcolor (wgetpixel (x, y));
          wputpixel (x + 960, y + 540);
       }
 
    getch ();

    while (1);
}

void main()
{
    wgt02();
    while (1);
}
