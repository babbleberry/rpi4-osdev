#include "../include/audio.h"
#include "../include/fb.h"

volatile unsigned* clk = (void*)CLOCK_BASE;
volatile unsigned* pwm = (void*)PWM_BASE;
volatile unsigned* dma = (void*)DMA_BASE;
volatile unsigned* dmae = (void*)DMA_ENABLE;
volatile unsigned* safe = (void*)DMA_ADDRESS;
struct dma_cb playback_cb;

void audio_init(void)
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

void audio_play_cpu(unsigned char *data, unsigned int size)
{
    int i=0;
    long status;

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

void audio_play_dma(unsigned char *data, unsigned int size)
{
    // Convert data (we expect 8-bit samples, but need to convert them to 32-bit words)

    for (int i=0;i<size;i++) *(safe+i) = *(data+i);

    wait_msec(2);

    // Set up control block

    playback_cb.ti = DMA_DEST_DREQ + DMA_PERMAP_1 + DMA_SRC_INC;
    playback_cb.source_ad = DMA_ADDRESS;
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

void audio_wait_dma(void)
{
    while (*(dma+DMA_CS) & 0x1); // Wait for DMA transfer to finish - we could do anything here instead!
}
