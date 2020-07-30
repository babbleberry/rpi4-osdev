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
    LE_ADREPORT_CODE          = 0x02,
    HCI_EVENT_PKT             = 0x04
};

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

void print_bdaddr(long bd_addr)
{
    unsigned char byte;

    for (int c=9;c>=4;c--) {
        byte = (unsigned char)((bd_addr >> ((c-4)*8) & 0xff));
	uart_hex(byte);
	if (c != 4) uart_writeText(":");
    }
}

void bt_update()
{
    unsigned char *buf;

    while ( (buf = poll()) ) {
       if (data_len >= 2 && buf[0] == LE_ADREPORT_CODE) {
	  unsigned char numreports = buf[1];
	  unsigned int c=0;

	  if (numreports > 0 && data_len >= 11) {
	     uart_writeText("\ra("); 
	     for (int c=9;c>=4;c--) {
		 if (c != 9) uart_writeText(":");
		 uart_hex(buf[c]);
	     }
	     uart_writeText(")");

	     unsigned char buf_len = buf[10];

	     if ((buf_len + 11) <= data_len - 1) {
	        buf += 11;

		while (c < numreports) {
	           unsigned char ad_len = buf[0];
	           unsigned char ad_type = buf[1];
	           buf += 2;

   		   if (ad_len > 2) {
		      if (ad_type == 0x09 || ad_type == 0x08) {
			 unsigned int d=0;

		         uart_writeText(" -> adtype("); uart_hex(ad_type); uart_writeText(":"); uart_hex(ad_len); uart_writeText(")");
			 uart_writeText(" -> ");
			 while (d<ad_len - 1) {
			    uart_writeByteBlockingActual(buf[d]);
			    d++;
			 }
			 uart_writeText("\n");
		      }
	           }

		   buf += ad_len - 1;
	           c++;
		}
	     }
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

    setLEeventmask(0xff);
    startActiveScanning();

    uart_writeText("Waiting for input...\n");
    while (1) {
       uart_update();
       bt_update();
    }
}
