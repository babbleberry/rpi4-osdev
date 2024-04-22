extern unsigned long spin_cpu0;
extern unsigned long spin_cpu1;
extern unsigned long spin_cpu2;
extern unsigned long spin_cpu3;

void start_core1(void (*func)(void));
void start_core2(void (*func)(void));
void start_core3(void (*func)(void));
void clear_core1(void);
void clear_core2(void);
void clear_core3(void);
