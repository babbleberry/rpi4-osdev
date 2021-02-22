#define LOCAL_CONTROL		        0xff800000
#define LOCAL_PRESCALER		        0xff800008
#define OSC_FREQ		        54000000
#define MAIN_STACK              	0x400000

.section ".text.boot"

.globl _start
_start:
        ldr     x0, =LOCAL_CONTROL      // Sort out the timer
	str     wzr, [x0]
	mov     w1, 0x80000000
	str     w1, [x0, #(LOCAL_PRESCALER - LOCAL_CONTROL)]

        ldr     x0, =OSC_FREQ
	msr     cntfrq_el0, x0
	msr     cntvoff_el2, xzr

	mrs	x6, mpidr_el1		
	and	x6, x6,#0xFF		// Check processor id
	cbz	x6, primary_cpu         // Hang for all non-primary CPU

	adr     x5, spin_cpu0
proc_hang:
	wfe
	ldr     x4, [x5, x6, lsl #3]
	cbz     x4, proc_hang

        ldr     x1, =__test_stack       // Get ourselves a fresh stack
        mov     sp, x1
secondary_cpu:
	mov     x0, #0
	mov     x1, #0
	mov     x2, #0
	mov     x3, #0
	br      x4
        b       proc_hang
primary_cpu:
	ldr	x1, =__bss_start
	ldr	w2, =__bss_size
memzero:
        cbz     w2, startup
	str     xzr, [x1], #8
	sub     w2, w2, #1
	cbnz    w2, memzero
startup:
	mov	sp, #MAIN_STACK
	bl	main
	b 	proc_hang		// should never come here

.ltorg

.org 0xd8
.globl spin_cpu0
spin_cpu0:
        .quad 0

.org 0xe0
.globl spin_cpu1
spin_cpu1:
        .quad 0

.org 0xe8
.globl spin_cpu2
spin_cpu2:
        .quad 0

.org 0xf0
.globl spin_cpu3
spin_cpu3:
        .quad 0
