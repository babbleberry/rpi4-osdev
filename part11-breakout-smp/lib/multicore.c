#include "../include/multicore.h"

void store32(unsigned long address, unsigned long value)
{
    *(unsigned long *) address = value;
}

unsigned long load32(unsigned long address)
{
    return *(unsigned long *) address;
}

void start_core1(void (*func)(void))
{
    store32((unsigned long)&spin_cpu1, (unsigned long)func);
    asm volatile ("sev");
}

void start_core2(void (*func)(void))
{
    store32((unsigned long)&spin_cpu2, (unsigned long)func);
    asm volatile ("sev");
}

void start_core3(void (*func)(void))
{
    store32((unsigned long)&spin_cpu3, (unsigned long)func);
    asm volatile ("sev");
}

void clear_core1(void) 
{
    store32((unsigned long)&spin_cpu1, 0);
}

void clear_core2(void) 
{
    store32((unsigned long)&spin_cpu2, 0);
}

void clear_core3(void) 
{
    store32((unsigned long)&spin_cpu3, 0);
}
