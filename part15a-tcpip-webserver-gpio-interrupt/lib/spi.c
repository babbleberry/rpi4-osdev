#include "../include/io.h"
#include "../include/fb.h"

// SPI

struct Spi0Regs {
    volatile unsigned int cs;
    volatile unsigned int fifo;
    volatile unsigned int clock;
    volatile unsigned int data_length;
    volatile unsigned int ltoh;
    volatile unsigned int dc;
};

#define REGS_SPI0 ((struct Spi0Regs *)(PERIPHERAL_BASE + 0x00204000))

// CS Register
#define CS_LEN_LONG	(1 << 25)
#define CS_DMA_LEN	(1 << 24)
#define CS_CSPOL2	(1 << 23)
#define CS_CSPOL1	(1 << 22)
#define CS_CSPOL0	(1 << 21)
#define CS_RXF		(1 << 20)
#define CS_RXR		(1 << 19)
#define CS_TXD		(1 << 18)
#define CS_RXD		(1 << 17)
#define CS_DONE		(1 << 16)
#define CS_LEN		(1 << 13)
#define CS_REN		(1 << 12)
#define CS_ADCS		(1 << 11)
#define CS_INTR		(1 << 10)
#define CS_INTD		(1 << 9)
#define CS_DMAEN	(1 << 8)
#define CS_TA		(1 << 7)
#define CS_CSPOL	(1 << 6)
#define CS_CLEAR_RX	(1 << 5)
#define CS_CLEAR_TX	(1 << 4)
#define CS_CPOL__SHIFT	3
#define CS_CPHA__SHIFT	2
#define CS_CS		(1 << 0)
#define CS_CS__SHIFT	0

void spi_init() {
    gpio_useAsAlt0(7);                  //CS1
    gpio_initOutputPinWithPullNone(8);  //CS0
    gpio_useAsAlt0(9);                  //MISO 
    gpio_useAsAlt0(10);                 //MOSI
    gpio_useAsAlt0(11);                 //SCLK
}

void spi_chip_select(unsigned char chip_select) {
    gpio_setPinOutputBool(8, chip_select);
}

void spi_send_recv(unsigned char *sbuffer, unsigned char *rbuffer, unsigned int size) {
    REGS_SPI0->data_length = size;
    REGS_SPI0->cs = REGS_SPI0->cs | CS_CLEAR_RX | CS_CLEAR_TX | CS_TA;
    
    unsigned int read_count = 0;
    unsigned int write_count = 0;

    while(read_count < size || write_count < size) {
        while(write_count < size && REGS_SPI0->cs & CS_TXD) {
            if (sbuffer) {
                REGS_SPI0->fifo = *sbuffer++;
            } else {
                REGS_SPI0->fifo = 0;
            }

            write_count++;
        }

        while(read_count < size && REGS_SPI0->cs & CS_RXD) {
            unsigned int data = REGS_SPI0->fifo;

            if (rbuffer) {
                *rbuffer++ = data;
            }

            read_count++;
        }
    }

    while(!(REGS_SPI0->cs & CS_DONE)) {
        while(REGS_SPI0->cs & CS_RXD) {
            unsigned int r = REGS_SPI0->fifo;
	    debughex(r);
        }
    }

    REGS_SPI0->cs = (REGS_SPI0->cs & ~CS_TA);
}

void spi_send(unsigned char *data, unsigned int size) {
    spi_send_recv(data, 0, size);
}

void spi_recv(unsigned char *data, unsigned int size) {
    spi_send_recv(0, data, size);
}
