#include "include/multicore.h"
#include "include/audio.h"

void snd_core(void)
{
    clear_core2();

    // Initialise the audio
    audio_init();

    // Write data out to FIFO and loop infinitely

    extern unsigned char _binary_bin_audio_bin_start[];
    extern unsigned char _binary_bin_audio_bin_size[];
    
    unsigned int size = (long)&_binary_bin_audio_bin_size;
    unsigned char *data = &(_binary_bin_audio_bin_start[0]);

    while (1) audio_play_cpu(data, size);
}
