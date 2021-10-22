#include "kernel.h"
#include "../include/fb.h"

char *entry_error_messages[] = {
   "SYNC_INVALID_EL1t",
   "IRQ_INVALID_EL1t",		
   "FIQ_INVALID_EL1t",		
   "ERROR_INVALID_EL1T",		

   "SYNC_INVALID_EL1h",		
   "IRQ_INVALID_EL1h",		
   "FIQ_INVALID_EL1h",		
   "ERROR_INVALID_EL1h",		

   "SYNC_INVALID_EL0_64",		
   "IRQ_INVALID_EL0_64",		
   "FIQ_INVALID_EL0_64",		
   "ERROR_INVALID_EL0_64",	

   "SYNC_INVALID_EL0_32",		
   "IRQ_INVALID_EL0_32",		
   "FIQ_INVALID_EL0_32",		
   "ERROR_INVALID_EL0_32"	
};

void enable_interrupt_controller() {
    REGS_IRQ->irq0_enable_0 = SYS_TIMER_IRQ_1 | SYS_TIMER_IRQ_3;
}

void disable_interrupt_controller() {
    REGS_IRQ->irq0_enable_0 = 0;
}

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address) {
    debugstr(entry_error_messages[type]);
    debugstr(" ESR: ");
    debughex(esr);
    debugstr("Addr: ");
    debughex(address);
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
