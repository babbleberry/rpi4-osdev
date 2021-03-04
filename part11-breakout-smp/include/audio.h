#include "../include/io.h"

#define PWM_BASE        (PERIPHERAL_BASE + 0x20C000 + 0x800) /* PWM1 register base address on RPi4 */
#define PWM_LEGACY_BASE     (LEGACY_BASE + 0x20C000 + 0x800) /* PWM1 register base legacy address on RPi4 */
#define CLOCK_BASE      (PERIPHERAL_BASE + 0x101000)
#define DMA_BASE        (PERIPHERAL_BASE + 0x007100)         /* DMA register base address */
#define DMA_ENABLE      (DMA_BASE + 0xFF0)                   /* DMA global enable bits */

#define DMA_ADDRESS     0x00500000                           /* A safe address to use for our DMA transfer */

#define BCM2711_PWMCLK_CNTL 40
#define BCM2711_PWMCLK_DIV  41
#define PM_PASSWORD 0x5A000000 

#define BCM2711_PWM_CONTROL 0
#define BCM2711_PWM_STATUS  1
#define BCM2711_PWM_DMAC    2
#define BCM2711_PWM0_RANGE  4
#define BCM2711_PWM0_DATA   5
#define BCM2711_PWM_FIFO    6
#define BCM2711_PWM1_RANGE  8
#define BCM2711_PWM1_DATA   9

#define BCM2711_PWM1_USEFIFO  0x2000  /* Data from FIFO */
#define BCM2711_PWM1_ENABLE   0x0100  /* Channel enable */
#define BCM2711_PWM0_USEFIFO  0x0020  /* Data from FIFO */
#define BCM2711_PWM0_ENABLE   0x0001  /* Channel enable */
#define BCM2711_PWM_ENAB      0x80000000  /* PWM DMA Configuration: DMA Enable (bit 31 set) */

#define BCM2711_GAPO2 0x20
#define BCM2711_GAPO1 0x10
#define BCM2711_RERR1 0x8
#define BCM2711_WERR1 0x4
#define BCM2711_FULL1 0x1
#define ERRORMASK (BCM2711_GAPO2 | BCM2711_GAPO1 | BCM2711_RERR1 | BCM2711_WERR1)

#define DMA_CS        0       /* Control/status register offset for DMA channel 0 */
#define DMA_CONBLK_AD 1
#define DMA_EN1       1 << 1  /* Enable DMA engine 1 */
#define DMA_ACTIVE    1       /* Active bit set */
#define DMA_DEST_DREQ 0x40    /* Use DREQ to pace peripheral writes */
#define DMA_PERMAP_1  0x10000 /* PWM1 peripheral for DREQ */
#define DMA_SRC_INC   0x100   /* Increment source address */

struct dma_cb {
   unsigned int ti;
   unsigned int source_ad;
   unsigned int dest_ad;
   unsigned int txfr_len;
   unsigned int stride;
   unsigned int nextconbk;
   unsigned int null1;
   unsigned int null2;
} __attribute__((aligned(32)));

void audio_init(void);
void audio_play_cpu(unsigned char *data, unsigned int size);
void audio_play_dma(unsigned char *data, unsigned int size);
void audio_wait_dma(void);
