#include "wgt.h"
#include "include/mem.h"

// ######## REQUIRED FUNCTIONS ########

unsigned long state0 = 1000;
unsigned long state1 = 2000;

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
    kb = 0;
}

// ######## WGT EXAMPLES ########

void wgt08()
{
    short x;

    mem_init();
    vga256 ();
    wcls (vgapal[0]);

    wsetcolor (vgapal[1]);
    wcircle (160, 100, 50);           /* try filling a circle */
    getch ();

    wsetcolor (vgapal[40]);
    wregionfill (160, 100);
    wsetcolor (vgapal[170]);
    wregionfill (0, 0);

    getch ();

    wcls (vgapal[0]);
    for (x = 1; x < 10000; x++)        /* try filling 10,000 random pixels */
    {
        wsetcolor (vgapal[rand() % 255]);
        wputpixel (rand() % 320, rand() % 200);
    }

    getch ();
    wsetcolor (vgapal[40]);
    wclip (50, 50, 250, 150);       /* fill works with clipping too! */
    wregionfill (160, 100);

    wsetcolor (vgapal[7]);
    wclip (10, 10, 40, 40);
    wregionfill (20, 20);

    wsetcolor (vgapal[9]);
    wclip (260, 160, 300, 190);
    wregionfill (270, 170);

    wsetcolor (vgapal[10]);
    wclip (0, 0, 319, 199);
    wregionfill (0, 0);

    getch ();
}

void main()
{
    wgt08();
    while (1);
}
