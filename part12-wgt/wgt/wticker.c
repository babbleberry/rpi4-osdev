#include "../include/wgt.h"
#include "../include/multicore.h"

short ticker_speed, ticker_running;
unsigned int ticker_interval;
void (*ticker_routine)();

void twait(unsigned int n)
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

void widletimer (void)
{
   /* do nothing */
}

void winittimer (void)
{
   ticker_running = 0;
   wsettimerspeed (60);
}

void wdonetimer (void)
{
   wsettimerspeed (60);
   ticker_running = 0;
}

void wsettimerspeed (int speed)
{
   ticker_speed = speed;
   ticker_interval = 1000000 / speed;
}

void wtick(void)
{
   clear_core1();

   while (ticker_running) {
      twait(ticker_interval);
      ticker_routine();
   }
}

void wstarttimer (void (*Rout)(), int speed)
{
   ticker_routine = Rout;
   wsettimerspeed (speed);
   ticker_running = 1;

   start_core1(wtick);
}

void wstoptimer (void)
{
   ticker_routine = widletimer;
   wsettimerspeed (19);
}
