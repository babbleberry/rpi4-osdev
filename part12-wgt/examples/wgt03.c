#include "wgt.h"
#include "include/mem.h"
#include "include/mb.h"

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

#define TIMERSPEED 60     /* timer_routine is called 60 times per second */

int timer;                /* Counts how many times it has been called */ 
int clearcount;           /* Counts the number of screen clears. */

int curx = 0;
int cury = 0;

extern int get_el(void);

void debugstr(char *str) {
    if (curx + (strlen(str) * 8)  >= 1920) {
       curx = 0; cury += 8;
    }
    if (cury + 8 >= 1080) {
       cury = 0;
    }
    wtextcolor(vgapal[15]);
    wouttextxy (curx, cury, NULL, str);

    curx += (strlen(str) * 8);
}

void debugcrlf(void) {
    curx = 0; cury += 8;
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

int get_max_clock()
{
    mbox[0] = 8*4; // Length of message in bytes
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_GETCLKMAXM; // Tag identifier
    mbox[3] = 8; // Value size in bytes
    mbox[4] = 0; // Value size in bytes
    mbox[5] = 0x3; // Value
    mbox[6] = 0; // Rate
    mbox[7] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP)) {
        if (mbox[5] == 0x3) {
           return mbox[6];
        }
    }
    return 0;
}

int get_clock_rate()
{
    mbox[0] = 8*4; // Length of message in bytes
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_GETCLKRATE; // Tag identifier
    mbox[3] = 8; // Value size in bytes
    mbox[4] = 0; // Value size in bytes
    mbox[5] = 0x3; // Value
    mbox[6] = 0; // Rate
    mbox[7] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP)) {
        if (mbox[5] == 0x3) {
           return mbox[6];
        }
    }
    return 0;
}

int set_clock_rate(unsigned int rate)
{
    mbox[0] = 9*4;  // Length of message in bytes
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_SETCLKRATE; // Tag identifier
    mbox[3] = 12;   // Value size in bytes
    mbox[4] = 0;    // Value size in bytes
    mbox[5] = 0x3;  // Value
    mbox[6] = rate; // Rate
    mbox[7] = 0;    // Rate
    mbox[8] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP)) {
        if (mbox[5] == 0x3 && mbox[6] == rate) {
           return 1;
        }
    }
    return 0;
}

void timer_routine (void)
{
  timer++;
}

void wgt03()
{
  mem_init();
  vga256 ();                     /* Initialize WGT system        */

  debugstr ("WGT Example #3"); debugcrlf(); debugcrlf(); 
  debugstr ("This program will use the wcls routine to clear the screen"); debugcrlf();
  debugstr ("using random colors as fast as it can until you press a key."); debugcrlf();
  debugstr ("It will then report the highest frame rate possible on your computer."); debugcrlf(); debugcrlf(); debugcrlf();

  int el = get_el();
  debugstr("Exception level: "); debughex(el); debugcrlf();
  int setter = set_clock_rate(get_max_clock());
  debugstr("Set clock returned "); debughex(setter); debugcrlf(); debugcrlf(); debugcrlf();

  debugstr ("Press any key to continue.");
  getch ();

  clearcount = 0;
  timer = 0;

  winittimer ();
  wstarttimer (timer_routine, TIMERSPEED);

  curx = 0;
  cury = 0;

  while (!kbhit ())
  {
     wcls(vgapal[rand() % 256]); /* Clear with random color out of 256 */
     clearcount++;
  }

  wstoptimer ();
  wdonetimer ();

  getch ();

  wcls(vgapal[0]);

  int clock = get_clock_rate();
  debugstr("Clock rate: "); debughex(clock); debugcrlf();

  int maxclock = get_max_clock();
  debugstr("Max clock rate: "); debughex(maxclock); debugcrlf();

  unsigned int fps = clearcount / (timer / TIMERSPEED);
  debughex(fps); debugstr("frames per second"); debugcrlf(); debugcrlf(); debugcrlf();

  debugstr ("This is the highest frame rate your computer can produce for full screen\n"); debugcrlf();
  debugstr ("animation.");
}

void main()
{
    wgt03();
    while (1);
}
