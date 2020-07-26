#include "io.h"
#include "fb.h"

// UART0

enum {
    ARM_UART0_BASE	= PERIPHERAL_BASE + 0x201000,
    ARM_UART0_DR	= ARM_UART0_BASE + 0x00,
    ARM_UART0_FR     	= ARM_UART0_BASE + 0x18,
    ARM_UART0_IBRD   	= ARM_UART0_BASE + 0x24,
    ARM_UART0_FBRD   	= ARM_UART0_BASE + 0x28,
    ARM_UART0_LCRH   	= ARM_UART0_BASE + 0x2C,
    ARM_UART0_CR     	= ARM_UART0_BASE + 0x30,
    ARM_UART0_IFLS   	= ARM_UART0_BASE + 0x34,
    ARM_UART0_IMSC   	= ARM_UART0_BASE + 0x38,
    ARM_UART0_RIS    	= ARM_UART0_BASE + 0x3C,
    ARM_UART0_MIS    	= ARM_UART0_BASE + 0x40,
    ARM_UART0_ICR    	= ARM_UART0_BASE + 0x44
};

unsigned char lo(unsigned int val) { return (unsigned char)(val & 0xff); }
unsigned char hi(unsigned int val) { return (unsigned char)((val & 0xff00) >> 8); }

unsigned int bt_isReadByteReady() { return (!(mmio_read(ARM_UART0_FR) & 0x10)); }

unsigned char bt_readByte()
{
    unsigned char ch = lo(mmio_read(ARM_UART0_DR));
    return ch;
}

unsigned char bt_waitReadByte()
{
    while (!bt_isReadByteReady());
    return bt_readByte();
}

void bt_writeByte(char byte)
{
    while ((mmio_read(ARM_UART0_FR) & 0x20) != 0);
    mmio_write(ARM_UART0_DR, (unsigned int)byte);
}

void bt_flushrx()
{
    while (bt_isReadByteReady()) bt_readByte();
}

void bt_init()
{
    gpio_useAsAlt3(30);
    gpio_useAsAlt3(31);
    gpio_useAsAlt3(32);
    gpio_useAsAlt3(33);

    bt_flushrx();

    mmio_write(ARM_UART0_IMSC, 0x00);
    mmio_write(ARM_UART0_ICR,  0x7ff);
    mmio_write(ARM_UART0_IBRD, 0x1a);
    mmio_write(ARM_UART0_FBRD, 0x03);
    mmio_write(ARM_UART0_IFLS, 0x08);
    mmio_write(ARM_UART0_LCRH, 0x70);
    mmio_write(ARM_UART0_CR,   0xB01);
    mmio_write(ARM_UART0_IMSC, 0x430);

    wait_msec(0x100000);
}

// HOST SETUP

enum {
    OGF_HOST_CONTROL          = 0x03,
    OGF_LE_CONTROL            = 0x08,
    OGF_VENDOR                = 0x3f,

    COMMAND_RESET_CHIP        = 0x03,
    COMMAND_LOAD_FIRMWARE     = 0x2e,

    HCI_COMMAND_PKT           = 0x01,
    HCI_EVENT_PKT             = 0x04,

    COMMAND_COMPLETE_CODE     = 0x0e,

    LL_SCAN_ACTIVE            = 0x01
};

unsigned char empty[] = {};

int hciCommandBytes(unsigned char *opcodebytes, unsigned char *data, unsigned char length)
{
    unsigned char c=0;

    bt_writeByte(HCI_COMMAND_PKT);
    bt_writeByte(opcodebytes[0]);
    bt_writeByte(opcodebytes[1]);
    bt_writeByte(length);

    while (c++<length) bt_writeByte(*data++);

    if (bt_waitReadByte() != HCI_EVENT_PKT) return 0;
    if (bt_waitReadByte() != COMMAND_COMPLETE_CODE) return 0;
    if (bt_waitReadByte() != 4) return 0;
    if (bt_waitReadByte() == 0) return 0;
    if (bt_waitReadByte() != opcodebytes[0]) return 0;
    if (bt_waitReadByte() != opcodebytes[1]) return 0;
    if (bt_waitReadByte() != 0) return 0;

    return 1;
}

int hciCommand(unsigned short ogf, unsigned short ocf, unsigned char *data, unsigned char length)
{
    unsigned short opcode = ogf << 10 | ocf;
    unsigned char opcodebytes[2] = { lo(opcode), hi(opcode) };

    return hciCommandBytes(opcodebytes, data, length);
}

void bt_loadfirmware()
{
    if (!hciCommand(OGF_VENDOR, COMMAND_LOAD_FIRMWARE, empty, 0)) uart_writeText("loadFirmware() failed\n");

    extern unsigned char _binary_BCM4345C0_hcd_start[];
    extern unsigned char _binary_BCM4345C0_hcd_size[];

    unsigned int c=0;
    unsigned int size = (long)&_binary_BCM4345C0_hcd_size;

    while (c < size) {
        unsigned char opcodebytes[] = { _binary_BCM4345C0_hcd_start[c], _binary_BCM4345C0_hcd_start[c+1] };
        unsigned char length = _binary_BCM4345C0_hcd_start[c+2];
        unsigned char *data = &(_binary_BCM4345C0_hcd_start[c+3]);

        if (!hciCommandBytes(opcodebytes, data, length)) {
	   uart_writeText("Firmware data load failed\n");
	   break;
	}
	c += 3 + length;
    }

    wait_msec(0x100000);
}

void setLEeventmask(unsigned char mask)
{
    unsigned char params[] = { mask, 0, 0, 0, 0, 0, 0, 0 };
    if (!hciCommand(OGF_LE_CONTROL, 0x01, params, 8)) uart_writeText("setLEeventmask failed\n");
}

void setLEscanenable(unsigned char state, unsigned char duplicates) {
    unsigned char params[] = { state, duplicates };
    if (!hciCommand(OGF_LE_CONTROL, 0x0c, params, 2)) uart_writeText("setLEscanenable failed\n");
}

void setLEscanparameters(unsigned char type, unsigned char linterval, unsigned char hinterval, unsigned char lwindow, unsigned char hwindow, unsigned char own_address_type, unsigned char filter_policy) {
    unsigned char params[] = { type, linterval, hinterval, lwindow, hwindow, own_address_type, filter_policy };
    if (!hciCommand(OGF_LE_CONTROL, 0x0b, params, 7)) uart_writeText("setLEscanparameters failed\n");
}

void startActiveScanning() {
    float BleScanUnitsPerSecond = 1600;
    float BleScanInterval = 0.8;
    float BleScanWindow = 0.4;

    unsigned int p = BleScanInterval * BleScanUnitsPerSecond;
    unsigned int q = BleScanWindow * BleScanUnitsPerSecond;

    setLEscanparameters(LL_SCAN_ACTIVE, lo(p), hi(p), lo(q), hi(q), 0, 0);
    setLEscanenable(1, 0);
}

void stopScanning() {
    setLEscanenable(0, 0);
}

void bt_reset() {
    if (!hciCommand(OGF_HOST_CONTROL, COMMAND_RESET_CHIP, empty, 0)) uart_writeText("bt_reset() failed\n");
}
