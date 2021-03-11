#include "../include/io.h"
#include "../include/mb.h"

// The buffer must be 16-byte aligned as only the upper 28 bits of the address can be passed via the mailbox
volatile unsigned int __attribute__((aligned(16))) mbox[36];

enum {
    VIDEOCORE_MBOX = (PERIPHERAL_BASE + 0x0000B880),
    MBOX_READ      = (VIDEOCORE_MBOX + 0x0),
    MBOX_POLL      = (VIDEOCORE_MBOX + 0x10),
    MBOX_SENDER    = (VIDEOCORE_MBOX + 0x14),
    MBOX_STATUS    = (VIDEOCORE_MBOX + 0x18),
    MBOX_CONFIG    = (VIDEOCORE_MBOX + 0x1C),
    MBOX_WRITE     = (VIDEOCORE_MBOX + 0x20),
    MBOX_RESPONSE  = 0x80000000,
    MBOX_FULL      = 0x80000000,
    MBOX_EMPTY     = 0x40000000
};

unsigned int mbox_call(unsigned char ch)
{
    // 28-bit address (MSB) and 4-bit value (LSB)
    unsigned int r = ((unsigned int)((long) &mbox) &~ 0xF) | (ch & 0xF);

    // Wait until we can write
    while (mmio_read(MBOX_STATUS) & MBOX_FULL);
    
    // Write the address of our buffer to the mailbox with the channel appended
    mmio_write(MBOX_WRITE, r);

    while (1) {
        // Is there a reply?
        while (mmio_read(MBOX_STATUS) & MBOX_EMPTY);

        // Is it a reply to our message?
        if (r == mmio_read(MBOX_READ)) return mbox[1]==MBOX_RESPONSE; // Is it successful?
           
    }
    return 0;
}

int get_max_clock()
{
    mbox[0] = 8*4; // Length of message in bytes
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_GETCLKMAXM; // Tag identifier
    mbox[3] = 8; // Value size in bytes
    mbox[4] = 0; // Value size in bytes
    mbox[5] = 0x3; // Value
    mbox[6] = 0; // Rate
    mbox[7] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP)) {
        if (mbox[5] == 0x3) {
           return mbox[6];
        }
    }
    return 0;
}

int get_clock_rate()
{
    mbox[0] = 8*4; // Length of message in bytes
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_GETCLKRATE; // Tag identifier
    mbox[3] = 8; // Value size in bytes
    mbox[4] = 0; // Value size in bytes
    mbox[5] = 0x3; // Value
    mbox[6] = 0; // Rate
    mbox[7] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP)) {
        if (mbox[5] == 0x3) {
           return mbox[6];
        }
    }
    return 0;
}

int set_clock_rate(unsigned int rate)
{
    mbox[0] = 9*4;  // Length of message in bytes
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_SETCLKRATE; // Tag identifier
    mbox[3] = 12;   // Value size in bytes
    mbox[4] = 0;    // Value size in bytes
    mbox[5] = 0x3;  // Value
    mbox[6] = rate; // Rate
    mbox[7] = 0;    // Rate
    mbox[8] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP)) {
        if (mbox[5] == 0x3 && mbox[6] == rate) {
           return 1;
        }
    }
    return 0;
}
