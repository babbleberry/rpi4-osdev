#include "../include/io.h"
#include "../include/spi.h"
#include "../include/multicore.h"
#include "kernel.h"

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
void core3_main(void) {
    clear_core3();                // Only run once

    uart_writeText("testing the network card\n");

    spi_init();
    init_network();
    arp_test();

    while(1) {
        arp_test();
        wait_msec(0x100000);
    }
}

void core0_main(void)
{
    unsigned int core0_val = 0;

    while (core0_val <= 1000) {
       wait_msec(0x100000);
       uart_writeText("another second on core 0\n");
       //drawProgress(0, core0_val/10);
       core0_val++;
    }

    uart_writeText("Core 0 done.\n");
}

void core1_main(void)
{
    unsigned int core1_val = 0;

    clear_core1();                // Only run once

    while (core1_val <= 100) {
       wait_msec(0x3FFFF);
       //drawProgress(1, core1_val);
       core1_val++;
    }
    uart_writeText("Core 1 done.\n");
 
    while(1);
}

// TIMER FUNCTIONS

const unsigned int timer1_int = CLOCKHZ;
const unsigned int timer3_int = CLOCKHZ / 4;
unsigned int timer1_val = 0;
unsigned int timer3_val = 0;

void timer_init() {
    timer1_val = REGS_TIMER->counter_lo;
    timer1_val += timer1_int;
    REGS_TIMER->compare[1] = timer1_val;

    timer3_val = REGS_TIMER->counter_lo;
    timer3_val += timer3_int;
    REGS_TIMER->compare[3] = timer3_val;
}

void handle_timer_1() {
    timer1_val += timer1_int;
    REGS_TIMER->compare[1] = timer1_val;
    REGS_TIMER->control_status |= SYS_TIMER_IRQ_1;

    unsigned int progval = timer1_val / timer1_int;
    if (progval <= 100) {
       //drawProgress(2, progval);
    } else {
        uart_writeText("Timer 1 done.\n");
    }
}

void handle_timer_3() {
    timer3_val += timer3_int;
    REGS_TIMER->compare[3] = timer3_val;
    REGS_TIMER->control_status |= SYS_TIMER_IRQ_3;

    unsigned int progval = timer3_val / timer3_int;
    if (progval <= 100); // drawProgress(3, progval);
}

unsigned long HAL_GetTick(void) {
    unsigned int hi = REGS_TIMER->counter_hi;
    unsigned int lo = REGS_TIMER->counter_lo;

    //double check hi value didn't change after setting it...
    if (hi != REGS_TIMER->counter_hi) {
        hi = REGS_TIMER->counter_hi;
        lo = REGS_TIMER->counter_lo;
    }

    return ((unsigned long)hi << 32) | lo;
}

void HAL_Delay(unsigned int ms) {
    unsigned long start = HAL_GetTick();

    while(HAL_GetTick() < start + (ms * 1000));
}

void main(void)
{
    uart_init();
    uart_writeText("uart initialised.\n");

    //uart_writeText("Kick it off on core 1\n");
    //start_core1(core1_main);

    // Kick off the timers

    irq_init_vectors();
    enable_interrupt_controller();
    irq_barrier();
    irq_enable();
    timer_init();

    uart_writeText("Kick it off on core 3\n");

    start_core3(core3_main);

    //uart_writeText("Kick it off on core 0\n");
    //core0_main();

    // Disable IRQs and loop endlessly

    irq_disable();
    disable_interrupt_controller();

    while(1);
}
