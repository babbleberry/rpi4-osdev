#include "kernel.h"

void enable_interrupt_controller() {
    REGS_IRQ->irq0_enable_0 = SYS_TIMER_IRQ_1 | SYS_TIMER_IRQ_3;
}

void disable_interrupt_controller() {
    REGS_IRQ->irq0_enable_0 = 0;
}

void handle_irq() {
    unsigned int irq = REGS_IRQ->irq0_pending_0;

    while(irq) {
        if (irq & SYS_TIMER_IRQ_1) {
            irq &= ~SYS_TIMER_IRQ_1;

            handle_timer_1();
        }

        if (irq & SYS_TIMER_IRQ_3) {
            irq &= ~SYS_TIMER_IRQ_3;

            handle_timer_3();
        }
    }
}
