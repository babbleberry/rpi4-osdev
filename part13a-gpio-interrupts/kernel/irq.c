#include "kernel.h"

// textdefinitionmap of invalid error-types
const char entry_error_messages[16][32] = {
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

void show_invalid_entry_message(unsigned int type, unsigned long esr, unsigned long address) {
    debugstr("ERROR CAUGHT: %s - %d, ESR: %X, Address: %X\n", 
        entry_error_messages[type], type, esr, address);
}

void enable_interrupt_controller() {
    REGS_IRQ->irq0_enable_0 = SYS_TIMER_IRQ_1 | SYS_TIMER_IRQ_3 | SPI_INT;
}

void disable_interrupt_controller() {
    REGS_IRQ->irq0_enable_0 = 0;
}

void handle_irq() {
    unsigned int irq = REGS_IRQ->irq0_pending_0;

    while(irq & (SYS_TIMER_IRQ_1 | SYS_TIMER_IRQ_3 | SPI_INT)) {
        switch(irq) {
            case (SYS_TIMER_IRQ_1):
                irq &= ~SYS_TIMER_IRQ_1;
                handle_timer_1();
                break;
            case (SYS_TIMER_IRQ_3):
                irq &= ~SYS_TIMER_IRQ_3;
                handle_timer_3();
                break;
            case(SPI_INT):
                irq &= ~SPI_INT;
                handle_spi();
                break;
            default:
                debugstr("Unknown pending irq: %x /r/n",irq);
        }
    }
}
