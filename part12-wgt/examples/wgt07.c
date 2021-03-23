#include "wgt.h"

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

void wgt07()
{
  short x;
  short y;
  short col;
  color palette[255];

  vga256 ();

  // Set our palette
  for (y = 0; y < 64; y++)
    wsetrgb (y, y*4, y*4, y*4, palette);
  for (y = 0; y < 64; y++)
    wsetrgb (y + 64, y*4, 0, 0, palette);
  for (y = 0; y < 64; y++)
    wsetrgb (y + 128, 0, y*4, 0, palette);
  for (y = 0; y < 64; y++)
    wsetrgb (y + 192, 0, 0, y*4, palette);
  wsetpalette (0, 255, palette);

  wcls (vgapal[0]);

  wtextgrid (TEXTGRID_OFF);
  wtexttransparent (TEXTFGBG);  /* Turn foreground and background on */

  do {
    x = rand() % 320;
    y = rand() % 200;
    col = rand() % 256;
    wtextcolor (vgapal[col]);
    wouttextxy (x, y, NULL, "WordUp Graphics Toolkit");
  } while (!kbhit ());
  getch ();

  wcls (0);
  wtextgrid (TEXTGRID_ON);

  do {
    x = rand() % 80;
    y = rand() % 25;
    col = rand() % 256;
    wtextcolor (vgapal[col]);
    wtextbackground (vgapal[rand() % 256]);
    wouttextxy (x, y, NULL, "WordUp Graphics Toolkit");
  } while (!kbhit ());

  getch ();
}

void main()
{
    wgt07();
    while (1);
}
