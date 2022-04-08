#include "sysregs.h"

#define LOCAL_CONTROL   0xff800000
#define LOCAL_PRESCALER 0xff800008
#define OSC_FREQ        54000000
#define MAIN_STACK      0x400000

.section ".text.boot"  // Make sure the linker puts this at the start of the kernel image

.global _start  // Execution starts here

_start:
    ldr     x0, =LOCAL_CONTROL   // Sort out the timer
    str     wzr, [x0]
    mov     w1, 0x80000000
    str     w1, [x0, #(LOCAL_PRESCALER - LOCAL_CONTROL)]

    ldr     x0, =OSC_FREQ
    msr     cntfrq_el0, x0
    msr     cntvoff_el2, xzr

    // Check processor ID is zero (executing on main core), else hang
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f

    // We're not on the main core, so hang in an infinite wait loop
    adr     x5, spin_cpu0
1:  wfe
    ldr     x4, [x5, x1, lsl #3]
    cbz     x4, 1b

    ldr     x2, =__stack_start   // Get ourselves a fresh stack - location depends on CPU core asking
    lsl     x1, x1, #9           // Multiply core_number by 512
    add     x3, x2, x1           // Add to the address
    mov     sp, x3

    mov     x0, #0
    mov     x1, #0
    mov     x2, #0
    mov     x3, #0
    br      x4
    b       1b
2:  // We're on the main core!
    // First enable the FPU

    mov     x0, #0x33ff
    msr     cptr_el3, x0 	 // Disable coprocessor traps to EL3
    mov     x0, #3 << 20
    msr     cpacr_el1, x0	 // Enable FP/SIMD at EL1

    // Now get ready to switch from EL3 down to EL1

    ldr     x0, =SCTLR_VALUE_MMU_DISABLED
    msr	    sctlr_el1, x0		

    ldr     x0, =HCR_VALUE
    msr     hcr_el2, x0

    ldr     x0, =SCR_VALUE
    msr     scr_el3, x0

    ldr     x0, =SPSR_VALUE
    msr     spsr_el3, x0
    
    adr     x0, el1_entry		
    msr     elr_el3, x0

    eret			
el1_entry:
    // We're in EL1
    // Clean the BSS section
    ldr     x1, =__bss_start     // Start address
    ldr     w2, =__bss_size      // Size of the section
3:  cbz     w2, 4f               // Quit loop if zero
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b               // Loop if non-zero

    // Set stack to start somewhere safe
    mov     sp, #MAIN_STACK

    // Jump to our main() routine in C (make sure it doesn't return)
4:  bl      main
    // In case it does return, halt the master core too
    b       1b

.ltorg

.org 0x110
.globl spin_cpu0
spin_cpu0:
        .quad 0

.org 0x118
.globl spin_cpu1
spin_cpu1:
        .quad 0

.org 0x120
.globl spin_cpu2
spin_cpu2:
        .quad 0

.org 0x128
.globl spin_cpu3
spin_cpu3:
        .quad 0

.globl get_el
get_el:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret
