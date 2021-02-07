#include "io.h"
#include "bt.h"

void main()
{
    uart_init();
    bt_init();

    uart_writeText("Initialising Bluetooth: ");
    uart_writeText(">> reset: ");
    bt_reset();
    uart_writeText(">> firmware load: ");
    bt_loadfirmware();
    uart_writeText(">> set baud: ");
    bt_setbaud();
    uart_writeText(">> set bdaddr: ");
    bt_setbdaddr();

    // Print the BD_ADDR
    unsigned char local_addr[6];
    bt_getbdaddr(local_addr);
    for (int c=5;c>=0;c--) uart_byte(local_addr[c]);
    uart_writeText("\n");

    // Start advertising
    uart_writeText("Setting event mask... ");
    setLEeventmask(0xff);
    uart_writeText("Starting advertsing... ");
    startActiveAdvertising();

    // Enter an infinite loop
    uart_writeText("Going loopy...");
    while (1) {
       uart_update();
    }
}
