#include "io.h"
#include "bt.h"

#define MAX_MSG_LEN    50
#define MAX_READ_RUN   100

unsigned char data_buf[MAX_MSG_LEN];
unsigned int data_len;
unsigned int messages_received = 0;
unsigned int poll_state = 0;

enum {
    LE_EVENT_CODE             = 0x3e,
    LE_CONNECT_CODE           = 0x01,
    LE_ADREPORT_CODE          = 0x02,
    HCI_EVENT_PKT             = 0x04
};

unsigned int connected = 0;
unsigned int connection_handle = 0;

void poll2(unsigned char byte)
{
    switch (poll_state) {
       case 0:
	  if (byte != HCI_EVENT_PKT) poll_state = 0;
	  else poll_state = 1;
	  break;
       case 1:
	  if (byte != LE_EVENT_CODE) poll_state = 0;
	  else poll_state = 2;
	  break;
       case 2:
	  if (byte > MAX_MSG_LEN) poll_state = 0;
	  else {
	     poll_state = 3;
	     data_len = byte;
	  }
	  break;
       default:
	  data_buf[poll_state - 3] = byte;
	  if (poll_state == data_len + 3 - 1) {
	     messages_received++;
             poll_state = 0;
	  } else poll_state++;
   }
}

unsigned char *poll()
{
    unsigned int goal = messages_received + 1;

    if (bt_isReadByteReady()) {
       unsigned int run = 0;

       while (run < MAX_READ_RUN && messages_received < goal && bt_isReadByteReady()) {
          unsigned char byte = bt_readByte(); 
	  poll2(byte);
	  run++;
       }
       if (run == MAX_READ_RUN) return 0;
       else return data_buf;
    }
    return 0;
}

void bt_update()
{
    unsigned char *buf;

    while ( (buf = poll()) ) {
       if (data_len >= 2) {
          if (buf[0] == LE_ADREPORT_CODE) {
	     unsigned char numreports = buf[1];

	     if (numreports == 1) {
                unsigned char event_type = buf[2];

	        if (event_type == 0x00) {
	           unsigned char buf_len = buf[10];
		   unsigned char ad_len = buf[11];

	           if (ad_len < data_len && buf_len + 11 == data_len - 1) {
	              for (int c=9;c>=4;c--) uart_byte(buf[c]);
	              buf += 11;

		      unsigned char rssi = buf[buf_len];
		      uart_writeText("-> rssi("); uart_hex(rssi); uart_writeText(")");

		      do {
	                 ad_len = buf[0];
	                 unsigned char ad_type = buf[1];
	                 buf += 2;

   		         if (ad_len >= 2) {
		            uart_writeText(" -> adtype("); uart_hex(ad_type); uart_writeText(":"); uart_hex(ad_len); uart_writeText(")");

		            if (ad_type == 0x09) {
		               unsigned int d=0;
		               uart_writeText(" -> ");
		               while (d<ad_len - 1) {
		                  uart_writeByteBlockingActual(buf[d]);
		                  d++;
		               }
	                    }
	                 }

		         buf += ad_len - 1;
		      } while (buf[1]);

		      uart_writeText("\n");
	           }
	        }
             }
          } else if (buf[0] == LE_CONNECT_CODE && !connected) {
             unsigned char status = buf[1];
	     connection_handle = buf[2] | (buf[3] << 8);
	     connected = (status == 0 && connection_handle != 0) ? 1 : 0;
	  }
       }
    }
}

void main()
{
    uart_init();
    bt_init();

    uart_writeText("bt_reset()\n");
    bt_reset();

    uart_writeText("bt_loadfirmware()\n");
    bt_loadfirmware();

    uart_writeText("bt_setbaud()\n");
    bt_setbaud();

    uart_writeText("bt_setbdaddr()\n");
    bt_setbdaddr();

    // Check we set the BD_ADDR correctly
    unsigned char local_addr[6];
    uart_writeText("bt_getbdaddr()\n");
    bt_getbdaddr(local_addr);
    uart_writeText("BD_ADDR is ");
    for (int c=5;c>=0;c--) uart_byte(local_addr[c]);
    uart_writeText("\n");

    /*
    // Start scanning for devices around us
    uart_writeText("startActiveScanning()\n");
    setLEeventmask(0xff);
    startActiveScanning();
    */

    uart_writeText("connect()\n");
    connect();
    
    /*
    // Get the Eddystone beacon going
    uart_writeText("startActiveAdvertising()\n");
    startActiveAdvertising();
    */

    uart_writeText("Waiting for connection...\n");
    while (!connected) bt_update();
    uart_writeText("Connected - handle "); uart_hex(connection_handle); uart_writeText("\n");

    uart_writeText("Waiting for input...\n");
    while (1) {
       uart_update();
       bt_update();
    }
}
