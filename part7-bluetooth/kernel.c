#include "io.h"
#include "bt.h"
#include "fb.h"

int curx = 0;
int cury = 0;

int strlen(const char *str) {
    const char *s;

    for (s = str; *s; ++s);
    return (s - str);
}

void debugstr(char *str) {
    if (curx + (strlen(str) * 8)  >= 1920) {
       curx = 0; cury += 8; 
    }
    drawString(curx, cury, str, 0x0f, 1);
    curx += (strlen(str) * 8);
}

void debugcrlf(void) {
    curx = 0; cury += 8;
}

void debugch(unsigned char b) {
    unsigned int n;
    int c;
    for(c=4;c>=0;c-=4) {
        n=(b>>c)&0xF;
        n+=n>9?0x37:0x30;
        debugstr((char *)&n);
    }
    debugstr(" ");
}

void main()
{
    fb_init();
    uart_init();
    bt_init();

    debugstr("Initialising Bluetooth: ");
    debugstr(">> reset: ");
    bt_reset();
    debugstr(">> firmware load: ");
    bt_loadfirmware();
    debugstr(">> set baud: ");
    bt_setbaud();
    debugstr(">> set bdaddr: ");
    bt_setbdaddr();

    // Print the BD_ADDR
    unsigned char local_addr[6];
    bt_getbdaddr(local_addr);
    for (int c=5;c>=0;c--) debugch(local_addr[c]);
    debugcrlf();

    // Start advertising
    debugstr("Setting event mask... ");
    setLEeventmask(0xff);
    debugstr("Starting advertsing... ");
    startActiveAdvertising();

    // Enter an infinite loop
    debugstr("Going loopy...");
    while (1) {
       uart_update();
    }
}
