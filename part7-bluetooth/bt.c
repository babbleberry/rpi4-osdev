#include "io.h"
#include "fb.h"

unsigned char *params = (unsigned char *)SAFE_ADDRESS;

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

    COMMAND_SET_BDADDR        = 0x01,
    COMMAND_RESET_CHIP        = 0x03,
    COMMAND_SET_BAUD          = 0x18,
    COMMAND_LOAD_FIRMWARE     = 0x2e,

    HCI_COMMAND_PKT           = 0x01,
    HCI_ACL_PKT               = 0x02,
    HCI_EVENT_PKT             = 0x04,
    COMMAND_COMPLETE_CODE     = 0x0e,
    CONNECT_COMPLETE_CODE     = 0x0f,

    LL_SCAN_ACTIVE            = 0x01,
    LL_ADV_NONCONN_IND        = 0x03
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

    if (bt_waitReadByte() != HCI_EVENT_PKT) return 1;

    unsigned char code = bt_waitReadByte();
    if (code == CONNECT_COMPLETE_CODE) {
       if (bt_waitReadByte() != 4) return 2;

       unsigned char err = bt_waitReadByte();
       if (err != 0) {
	  uart_writeText("Saw HCI COMMAND STATUS error "); uart_hex(err); uart_writeText("\n");
	  return 12;
       }

       if (bt_waitReadByte() == 0) return 3;
       if (bt_waitReadByte() != opcodebytes[0]) return 4;
       if (bt_waitReadByte() != opcodebytes[1]) return 5;
    } else if (code == COMMAND_COMPLETE_CODE) {
       if (bt_waitReadByte() != 4) return 6;
       if (bt_waitReadByte() == 0) return 7;
       if (bt_waitReadByte() != opcodebytes[0]) return 8;
       if (bt_waitReadByte() != opcodebytes[1]) return 9;
       if (bt_waitReadByte() != 0) return 10;
    } else return 11;

    return 0;
}

int hciCommand(unsigned short ogf, unsigned short ocf, unsigned char *data, unsigned char length)
{
    unsigned short opcode = ogf << 10 | ocf;
    unsigned char opcodebytes[2] = { lo(opcode), hi(opcode) };

    return hciCommandBytes(opcodebytes, data, length);
}

void bt_reset() {
    if (hciCommand(OGF_HOST_CONTROL, COMMAND_RESET_CHIP, empty, 0)) uart_writeText("bt_reset() failed\n");
}

void bt_loadfirmware()
{
    if (hciCommand(OGF_VENDOR, COMMAND_LOAD_FIRMWARE, empty, 0)) uart_writeText("loadFirmware() failed\n");

    extern unsigned char _binary_BCM4345C0_hcd_start[];
    extern unsigned char _binary_BCM4345C0_hcd_size[];

    unsigned int c=0;
    unsigned int size = (long)&_binary_BCM4345C0_hcd_size;

    while (c < size) {
        params[0] = _binary_BCM4345C0_hcd_start[c];
        params[1] = _binary_BCM4345C0_hcd_start[c+1];
        unsigned char length = _binary_BCM4345C0_hcd_start[c+2];
        unsigned char *data = &(_binary_BCM4345C0_hcd_start[c+3]);

        if (hciCommandBytes(params, data, length)) {
	   uart_writeText("Firmware data load failed\n");
	   break;
	}
	c += 3 + length;
    }

    wait_msec(0x100000);
}

void bt_setbaud()
{
    params[0] = 0;
    params[1] = 0;
    params[2] = 0x00;
    params[3] = 0xc2;
    params[4] = 0x01;
    params[5] = 0x00; // little endian, 115200
    if (hciCommand(OGF_VENDOR, COMMAND_SET_BAUD, params, 6)) uart_writeText("bt_setbaud() failed\n");
}

void bt_setbdaddr()
{
    params[0] = 0xee;
    params[1] = 0xff;
    params[2] = 0xc0;
    params[3] = 0xee;
    params[4] = 0xff;
    params[5] = 0xc0; // reversed
    if (hciCommand(OGF_VENDOR, COMMAND_SET_BDADDR, params, 6)) uart_writeText("bt_setbdaddr() failed\n");
}

void bt_getbdaddr(unsigned char *bdaddr) {
    bt_writeByte(HCI_COMMAND_PKT);
    bt_writeByte(0x09);
    bt_writeByte(0x10);
    bt_writeByte(0x00);

    if (bt_waitReadByte() != HCI_EVENT_PKT) return;
    if (bt_waitReadByte() != COMMAND_COMPLETE_CODE) return;
    if (bt_waitReadByte() != 0x0a) return;
    if (bt_waitReadByte() != 1) return;
    if (bt_waitReadByte() != 0x09) return;
    if (bt_waitReadByte() != 0x10) return;
    if (bt_waitReadByte() != 0x00) return;

    for (int c=0;c<6;c++) bdaddr[c] = bt_waitReadByte();
}

void sendACLsubscribe(unsigned int handle)
{
    bt_writeByte(HCI_ACL_PKT);

    bt_writeByte(lo(handle));
    bt_writeByte(hi(handle));

    unsigned int length = 0x0009;
    bt_writeByte(lo(length));
    bt_writeByte(hi(length));

    unsigned int data_length = 0x0005;
    bt_writeByte(lo(data_length));
    bt_writeByte(hi(data_length));

    unsigned int channel = 0x0004;
    bt_writeByte(lo(channel));
    bt_writeByte(hi(channel));

    params[0] = 0x12;
    params[1] = 0x2b;
    params[2] = 0x00;
    params[3] = 0x01;
    params[4] = 0x00;

    unsigned int c=0;
    while (c++<data_length) bt_writeByte(params[c-1]);
}

void setLEeventmask(unsigned char mask)
{
    params[0] = mask;
    params[1] = 0;
    params[2] = 0;
    params[3] = 0;
    params[4] = 0;
    params[5] = 0;
    params[6] = 0;
    params[7] = 0;
    if (hciCommand(OGF_LE_CONTROL, 0x01, params, 8)) uart_writeText("setLEeventmask failed\n");
}

void setLEscanenable(unsigned char state, unsigned char duplicates) {
    params[0] = state;
    params[1] = duplicates;
    if (hciCommand(OGF_LE_CONTROL, 0x0c, params, 2)) uart_writeText(" setLEscanenable failed\n");
}

void setLEscanparameters(unsigned char type, unsigned char linterval, unsigned char hinterval, unsigned char lwindow, unsigned char hwindow, unsigned char own_address_type, unsigned char filter_policy) {
    params[0] = type;
    params[1] = linterval;
    params[2] = hinterval;
    params[3] = lwindow;
    params[4] = hwindow;
    params[5] = own_address_type;
    params[6] = filter_policy;
    if (hciCommand(OGF_LE_CONTROL, 0x0b, params, 7)) uart_writeText("setLEscanparameters failed\n");
}

void setLEadvertenable(unsigned char state) {
    params[0] = state;
    if (hciCommand(OGF_LE_CONTROL, 0x0a, params, 1)) uart_writeText("setLEadvertenable failed\n");
}

void setLEadvertparameters(unsigned char type, unsigned char linterval_min, unsigned char hinterval_min, unsigned char linterval_max, unsigned char hinterval_max, unsigned char own_address_type, unsigned char filter_policy) {
    params[0] = linterval_min;
    params[1] = hinterval_min;
    params[2] = linterval_max;
    params[3] = hinterval_max;
    params[4] = type;
    params[5] = own_address_type;
    params[6] = 0;
    params[7] = 0;
    params[8] = 0;
    params[9] = 0;
    params[10] = 0;
    params[11] = 0;
    params[12] = 0;
    params[13] = 0x07;
    params[14] = filter_policy;
    if (hciCommand(OGF_LE_CONTROL, 0x06, params, 15)) uart_writeText("setLEadvertparameters failed\n");
}

void setLEadvertdata() {
    params[0] = 0x19;
    params[1] = 0x02; params[2] = 0x01; params[3] = 0x06;
    params[4] = 0x03; params[5] = 0x03; params[6] = 0xAA; params[7] = 0xFE;
    params[8] = 0x11; params[9] = 0x16; params[10] = 0xAA; params[11] = 0xFE; params[12] = 0x10; params[13] = 0x00; params[14] = 0x03;
    params[15] = 0x69; params[16] = 0x73; params[17] = 0x6f; params[18] = 0x6d; params[19] = 0x65; params[20] = 0x74; params[21] = 0x69; params[22] = 0x6d;
    params[23] = 0x2e; params[24] = 0x65; params[25] = 0x73;
    params[26] = 0x00;
    params[27] = 0x00;
    params[28] = 0x00;
    params[29] = 0x00;
    params[30] = 0x00;
    params[31] = 0x00;
    if (hciCommand(OGF_LE_CONTROL, 0x08, params, 32)) uart_writeText("setLEadvertdata failed\n");
}

void stopScanning() {
    setLEscanenable(0, 0);
}

void stopAdvertising() {
    setLEadvertenable(0);
}

void startActiveScanning() {
    float BleScanInterval = 60; // every 60ms
    float BleScanWindow = 60;
    float BleScanDivisor = 0.625;

    unsigned int p = BleScanInterval / BleScanDivisor;
    unsigned int q = BleScanWindow / BleScanDivisor;

    setLEscanparameters(LL_SCAN_ACTIVE, lo(p), hi(p), lo(q), hi(q), 0, 0);
    setLEscanenable(1, 0);
}

void startActiveAdvertising() {
    float advertMinFreq = 100; // every 100ms
    float advertMaxFreq = 100; // every 100ms
    float bleGranularity = 0.625;

    unsigned int min_interval = advertMinFreq / bleGranularity;
    unsigned int max_interval = advertMaxFreq / bleGranularity;

    setLEadvertparameters(LL_ADV_NONCONN_IND, lo(min_interval), hi(min_interval), lo(max_interval), hi(max_interval), 0, 0);
    setLEadvertdata();
    setLEadvertenable(1);
}

void connect(unsigned char *addr)
{
    float BleScanInterval = 60; // every 60ms
    float BleScanWindow = 60;
    float BleScanDivisor = 0.625;

    float connMinFreq = 30; // every 30ms
    float connMaxFreq = 50; // every 50ms
    float BleGranularity = 1.25;

    unsigned int p = BleScanInterval / BleScanDivisor;
    unsigned char lp = lo(p);
    unsigned char hp = hi(p);
    unsigned int q = BleScanWindow / BleScanDivisor;
    unsigned char lq = lo(q);
    unsigned char hq = hi(q);

    unsigned int min_interval = connMinFreq / BleGranularity;
    unsigned char lmini = lo(min_interval);
    unsigned char hmini = hi(min_interval);
    unsigned int max_interval = connMaxFreq / BleGranularity;
    unsigned char lmaxi = lo(max_interval);
    unsigned char hmaxi = hi(max_interval);

    params[0] = lp;
    params[1] = hp;
    params[2] = lq;
    params[3] = hq;
    params[4] = 0;
    params[5] = 0;
    params[6] = addr[5];
    params[7] = addr[4];
    params[8] = addr[3];
    params[9] = addr[2];
    params[10] = addr[1];
    params[11] = addr[0];
    params[12] = 0;
    params[13] = lmini;
    params[14] = hmini;
    params[15] = lmaxi;
    params[16] = hmaxi;
    params[17] = 0;
    params[18] = 0;
    params[19] = 0x2a;
    params[20] = 0x00;
    params[21] = 0;
    params[22] = 0;
    params[23] = 0;
    params[24] = 0;

    if (hciCommand(OGF_LE_CONTROL, 0x0d, params, 25)) uart_writeText("createLEconnection failed\n");
}
