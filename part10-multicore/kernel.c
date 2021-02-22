#include "fb.h"
#include "io.h"
#include "multicore.h"

#define PWM_BASE        (PERIPHERAL_BASE + 0x20C000 + 0x800) /* PWM1 register base address on RPi4 */
#define PWM_LEGACY_BASE     (LEGACY_BASE + 0x20C000 + 0x800) /* PWM1 register base legacy address on RPi4 */
#define CLOCK_BASE      (PERIPHERAL_BASE + 0x101000)
#define DMA_BASE        (PERIPHERAL_BASE + 0x007100)         /* DMA register base address */
#define DMA_ENABLE      (DMA_BASE + 0xFF0)                   /* DMA global enable bits */

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

volatile unsigned* clk = (void*)CLOCK_BASE;
volatile unsigned* pwm = (void*)PWM_BASE;
volatile unsigned* dma = (void*)DMA_BASE;
volatile unsigned* dmae = (void*)DMA_ENABLE;
volatile unsigned* safe = (void*)SAFE_ADDRESS;

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

struct dma_cb playback_cb;

static void playaudio_dma(void)
{
    extern unsigned char _binary_audio_bin_start[];
    extern unsigned char _binary_audio_bin_size[];
    
    unsigned int size = (long)&_binary_audio_bin_size;
    unsigned char *data = &(_binary_audio_bin_start[0]);

    // Convert data

    for (int i=0;i<size;i++) *(safe+i) = *(data+i);

    wait_msec(2);

    // Set up control block

    playback_cb.ti = DMA_DEST_DREQ + DMA_PERMAP_1 + DMA_SRC_INC;
    playback_cb.source_ad = SAFE_ADDRESS;
    playback_cb.dest_ad = PWM_LEGACY_BASE + 0x18; // Points to PWM_FIFO
    playback_cb.txfr_len = size * 4; // They're unsigned ints now, not unsigned chars
    playback_cb.stride = 0x00;
    playback_cb.nextconbk = 0x00; // Don't loop
    playback_cb.null1 = 0x00;
    playback_cb.null2 = 0x00;

    wait_msec(2);

    // Enable DMA

    *(pwm+BCM2711_PWM_DMAC) = 
          BCM2711_PWM_ENAB + 0x0707; // Bits 0-7 Threshold For DREQ Signal = 1, Bits 8-15 Threshold For PANIC Signal = 0
    *dmae = DMA_EN1;
    *(dma+DMA_CONBLK_AD) = (long)&playback_cb; // checked and correct

    wait_msec(2);

    *(dma+DMA_CS) = DMA_ACTIVE;
}

static void audio_init(void)
{   
    gpio_useAsAlt0(40); // Ensure PWM1 is mapped to GPIO 40/41
    gpio_useAsAlt0(41);
    wait_msec(2);

    // Setup clock
    
    *(clk + BCM2711_PWMCLK_CNTL) = PM_PASSWORD | (1 << 5); // Stop clock
    wait_msec(2);

    int idiv = 2;
    *(clk + BCM2711_PWMCLK_DIV)  = PM_PASSWORD | (idiv<<12);
    *(clk + BCM2711_PWMCLK_CNTL) = PM_PASSWORD | 16 | 1;  // Osc + Enable
    wait_msec(2);

    // Setup PWM
    
    *(pwm + BCM2711_PWM_CONTROL) = 0;
    wait_msec(2);
    
    *(pwm+BCM2711_PWM0_RANGE) = 0x264; // 44.1khz, Stereo, 8-bit (54Mhz / 44100 / 2)
    *(pwm+BCM2711_PWM1_RANGE) = 0x264;
    
    *(pwm+BCM2711_PWM_CONTROL) =
          BCM2711_PWM1_USEFIFO |
          BCM2711_PWM1_ENABLE | 
          BCM2711_PWM0_USEFIFO | 
          BCM2711_PWM0_ENABLE | 1<<6;
}

void playaudio_cpu()
{
    int i=0;
    long status;

    extern unsigned char _binary_audio_bin_start[];
    extern unsigned char _binary_audio_bin_size[];
    
    unsigned int size = (long)&_binary_audio_bin_size;
    unsigned char *data = &(_binary_audio_bin_start[0]);

    // Write data out to FIFO

    while (i < size) {
        status = *(pwm + BCM2711_PWM_STATUS);
        if (!(status & BCM2711_FULL1)) {
            *(pwm+BCM2711_PWM_FIFO) = *(data + i);
            i++;
            *(pwm+BCM2711_PWM_FIFO) = *(data + i);
            i++;
        }
        if ((status & ERRORMASK)) {
            *(pwm+BCM2711_PWM_STATUS) = ERRORMASK;
        }
    }
}

void core2_main(void)
{
    clear_core2();                // Only run once

    debugstr("Playing on CPU Core #2 using DMA... ");
    playaudio_dma();
    debugstr("done"); debugcrlf();

    debugstr("core2_main() running still... ");
    while (*(dma+DMA_CS) & 0x1) { // Wait for DMA transfer to finish - we could do anything here instead!
       wait_msec(0x47FFFF);
       debugstr("o");             // Print an o roughly every 4.5 seconds
    }
    debugstr(" ----> finished");
}

void core1_main(void)
{
    clear_core1();                // Only run once

    debugstr("Playing on CPU Core #1... ");
    playaudio_cpu();
    debugstr(" done"); debugcrlf();

    start_core2(core2_main);      // Kick it off on core 2
}

void core0_main(void)
{
    while (1) {
       wait_msec(0x100000);
       debugstr("x");             // Print an x roughly every 1 second
    }
}

void main(void)
{
    fb_init();

    debugstr("Initialising audio unit... ");
    audio_init();
    debugstr("done"); debugcrlf();

    start_core1(core1_main);      // Kick it off on core 1
    core0_main();                 // Loop endlessly, printing x's
}
