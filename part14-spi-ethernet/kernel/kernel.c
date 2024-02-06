#include "../include/fb.h"
#include "../include/io.h"
#include "../include/spi.h"
#include "../include/multicore.h"
#include "kernel.h"

void initProgress(void)
{
    drawRect(0, 0, 301, 50, 0x0f, 0);
    drawString(309, 21, "Core 0", 0x0f, 1);

    drawRect(0, 60, 301, 110, 0x0f, 0);
    drawString(309, 81, "Core 1", 0x0f, 1);

    drawRect(0, 120, 301, 170, 0x0f, 0);
    drawString(309, 141, "Timer 1", 0x0f, 1);

    drawRect(0, 180, 301, 230, 0x0f, 0);
    drawString(309, 201, "Timer 3", 0x0f, 1);
}

void drawProgress(unsigned int core, unsigned int val) {
    unsigned char col = (core + 1) + ((core + 1) << 4);

    // val should be 0-100
    if (val == 0) drawRect(1, (60 * core) + 1, 300, (60 * core) + 49, 0x00, 1);
    if (val > 0) drawRect(1, (60 * core) + 1, (val * 3), (60 * core) + 49, col, 1);
}

void core3_main(void) {
    clear_core3();                // Only run once

    // Test the network card

    spi_init();
    init_network();
    arp_test();

    while(1);
}

void core0_main(void)
{
    unsigned int core0_val = 0;

    while (core0_val <= 100) {
       wait_msec(0x100000);
       drawProgress(0, core0_val);
       core0_val++;
    }

    debugstr("Core 0 done.");
    debugcrlf();
}

void core1_main(void)
{
    unsigned int core1_val = 0;

    clear_core1();                // Only run once

    while (core1_val <= 100) {
       wait_msec(0x3FFFF);
       drawProgress(1, core1_val);
       core1_val++;
    }

    debugstr("Core 1 done.");
    debugcrlf();
 
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
       drawProgress(2, progval);
    } else {
       debugstr("Timer 1 done.");
       debugcrlf();
    }
}

void handle_timer_3() {
    timer3_val += timer3_int;
    REGS_TIMER->compare[3] = timer3_val;
    REGS_TIMER->control_status |= SYS_TIMER_IRQ_3;

    unsigned int progval = timer3_val / timer3_int;
    if (progval <= 100) drawProgress(3, progval);
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
    fb_init();

    unsigned int i=0;
    while (i++<30) debugcrlf();

    initProgress();

    // Kick it off on core 1

    start_core1(core1_main);

    // Kick off the timers

    irq_init_vectors();
    enable_interrupt_controller();
    irq_barrier();
    irq_enable();
    timer_init();

    // Kick it off on core 3

    start_core3(core3_main);

    // Kick it off on core 0

    core0_main();

    // Disable IRQs and loop endlessly

    irq_disable();
    disable_interrupt_controller();

    while(1);
}
