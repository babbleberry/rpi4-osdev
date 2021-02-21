extern unsigned int spin_cpu0;
extern unsigned int spin_cpu1;
extern unsigned int spin_cpu2;
extern unsigned int spin_cpu3;

unsigned long load32(unsigned long address);
void start_core1(void (*func)(void));
void start_core2(void (*func)(void));
void start_core3(void (*func)(void));
unsigned int get_core_number(void);
