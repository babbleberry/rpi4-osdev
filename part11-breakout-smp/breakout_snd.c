#include "include/fb.h"
#include "include/io.h"
#include "include/multicore.h"

#define PWM_BASE        (PERIPHERAL_BASE + 0x20C000 + 0x800) /* PWM1 register base address on RPi4 */
#define PWM_LEGACY_BASE     (LEGACY_BASE + 0x20C000 + 0x800) /* PWM1 register base legacy address on RPi4 */
#define CLOCK_BASE      (PERIPHERAL_BASE + 0x101000)

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

volatile unsigned* clk = (void*)CLOCK_BASE;
volatile unsigned* pwm = (void*)PWM_BASE;

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

void snd_core(void)
{
    clear_core2();

    int i=0;
    long status;

    extern unsigned char _binary_bin_audio_bin_start[];
    extern unsigned char _binary_bin_audio_bin_size[];
    
    unsigned int size = (long)&_binary_bin_audio_bin_size;
    unsigned char *data = &(_binary_bin_audio_bin_start[0]);

    // Initialise the audio
    audio_init();

    // Write data out to FIFO and loop infinitely

    while (1) {
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
       i=0;
    }
}
