#define PERIPHERAL_BASE 0xFE000000
#define LEGACY_BASE     0x7E000000
#define SAFE_ADDRESS    0x00400000 // Somewhere safe to store a lot of data

void uart_init();
void uart_writeText(char *buffer);
void uart_loadOutputFifo();
unsigned char uart_readByte();
unsigned int uart_isReadByteReady();
void uart_writeByteBlockingActual(unsigned char ch);
void uart_update();
void mmio_write(long reg, unsigned int val);
unsigned int mmio_read(long reg);
void gpio_useAsAlt0(unsigned int pin_number);
void gpio_useAsAlt3(unsigned int pin_number);
void uart_hex(unsigned int d);
void uart_byte(unsigned char b);
