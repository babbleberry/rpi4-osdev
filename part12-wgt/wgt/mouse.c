#include "../include/wgt.h"
#include "../include/multicore.h"
#include "../include/io.h"
#include "../include/bt.h"

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
    HCI_ACL_PKT               = 0x02,
    HCI_EVENT_PKT             = 0x04
};

unsigned int got_echo_sid = 0;
unsigned int got_echo_name = 0;
unsigned char echo_addr[6];

unsigned int connected = 0;
unsigned int connection_handle = 0;

volatile unsigned int comms_up = 0;

/* Mouse variables */
volatile int but;
volatile int mx;
volatile int my;

/* Mouse boundaries */
int mbx1, mby1;
int mbx2, mby2;

void hci_poll2(unsigned char byte)
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

unsigned char *hci_poll()
{
    unsigned int goal = messages_received + 1;

    if (bt_isReadByteReady()) {
       unsigned int run = 0;

       while (run < MAX_READ_RUN && messages_received < goal && bt_isReadByteReady()) {
          unsigned char byte = bt_readByte(); 
	  hci_poll2(byte);
	  run++;
       }
       if (run == MAX_READ_RUN) return 0;
       else return data_buf;
    }
    return 0;
}

void bt_search(void) {
    unsigned char *buf;

    while ( (buf = hci_poll()) ) {
       if (data_len >= 2) {
          if (buf[0] == LE_ADREPORT_CODE) {
             if (buf[1] == 1) { // num_reports
                if (buf[2] == 0) { // event_type
                   int bufindex = 0;
		   unsigned char ad_len = buf[11];

                   for (int c=9;c>=4;c--) echo_addr[9-c] = buf[bufindex + c]; // save the mac address
                   bufindex += 11;

                   got_echo_sid = 0; got_echo_name = 0; // Reset the search state machine
                   do {
                      ad_len = buf[bufindex];
                      unsigned char ad_type = buf[bufindex + 1];
                      bufindex += 2;

                      if (ad_len >= 2) {
                         if (ad_type == 0x03) {
			    unsigned int sid = buf[bufindex] | (buf[bufindex + 1] << 8);
			    if (sid == 0xEC00) {
			       got_echo_sid = 1;
			    }
                         } else if (ad_type == 0x09) {
                            char remote_name[ad_len - 1];
		            unsigned int d=0;

		            while (d<ad_len - 1) {
			       remote_name[d] = buf[bufindex + d];
		               d++;
		            }
			    if (!memcmp(remote_name,"echo",4)) {
			       got_echo_name = 1;
			    }
                         }
                      }

                      bufindex += ad_len - 1;
                   } while (bufindex < data_len);
                }
             }
          }
       }
    }
}

void bt_conn()
{
    unsigned char *buf;

    while ( (buf = hci_poll()) ) {
       if (!connected && data_len >= 2 && buf[0] == LE_CONNECT_CODE) {
          connected = !*(buf+1);
          connection_handle = *(buf+2) | (*(buf+3) << 8);

          if (connected == 0) wait_msec(0x186A);
       }
    }
}

void acl_poll()
{
    while (bt_isReadByteReady()) {
       unsigned char byte = bt_waitReadByte(); 

       if (byte == HCI_EVENT_PKT) {
	  bt_waitReadByte(); // opcode
	  unsigned char length = bt_waitReadByte();
	  for (int i=0;i<length;i++) bt_waitReadByte();
       } else if (byte == HCI_ACL_PKT) {
	  unsigned char h1 = bt_waitReadByte(); // handle1
	  unsigned char h2 = bt_waitReadByte(); // handle2
          unsigned char thandle = h1 | (h2 << 8);

	  unsigned char d1 = bt_waitReadByte();
	  unsigned char d2 = bt_waitReadByte();

	  unsigned int dlen = d1 | (d2 << 8);
	  unsigned char data[dlen];

	  for (int i=0;i<dlen;i++) data[i] = bt_waitReadByte();

	  if (dlen > 7) {
	     unsigned int length = data[0] | (data[1] << 8);
	     unsigned int channel = data[2] | (data[3] << 8);
	     unsigned char opcode = data[4];
 
	     if (thandle == connection_handle && opcode == 0x1b) {
	        if (channel == 4 && data[5] == 0x3b && data[6] == 0x00) {
                   if (length == 7) msetxy(data[7] | (data[8] << 8), data[9] | (data[10] << 8));
                   if (length == 5) msetbut(data[7], data[8]);
                }
	     }
          }
       }
    }
}

void msetbounds (short x1, short y1, short x2, short y2)
{
    if (x1 < WGT_SYS.xres && x1 >= 0) {
       if (x2 < WGT_SYS.xres && x2 >= x1) {
          if (y1 < WGT_SYS.yres && y1 >= 0) {
             if (y2 < WGT_SYS.yres && y2 >= y1) {
                mbx1 = x1;
                mbx2 = x2;
                mby1 = y1;
                mby2 = y2;
             }
          }
       }
    }
}

void msetbut (short event, short bnum) {
    if (event == 1) { // Mouse down
       but |= bnum;
    } else if (event == 2) { // Mouse up
       but ^= bnum;
    }
}

void msetxy (short x, short y)
{
    if (x < mbx1) x = mbx1;
    if (y < mby1) y = mby1;

    if (x > mbx2) x = mbx2;
    if (y > mby2) y = mby2;

    mx = x;
    my = y;
}

void noclick()
{
    if (but) while (but);
}

void mdeinit()
{
    comms_up = 0;
}

void minit()
{
    clear_core2();

    // Initialise the UART
    uart_init();

    // Initialise the Bluetooth
    bt_init();
    bt_reset();
    bt_loadfirmware();
    bt_setbaud();
    bt_setbdaddr();

    // Start scanning
    setLEeventmask(0xff);
    startActiveScanning();

    // Search for the echo
    while (!(got_echo_sid && got_echo_name)) bt_search();
    stopScanning();

    // Connecting to echo
    connect(echo_addr);
    while (!(connected && connection_handle)) bt_conn();

    // Subscribe to updates
    sendACLsubscribe(connection_handle);

    mx = WGT_SYS.xres / 2;
    my = WGT_SYS.yres / 2;
    mbx1 = 0;
    mby1 = 0;
    mbx2 = WGT_SYS.xres;
    mby2 = WGT_SYS.yres;
    but = 0;

    comms_up = 1;
    while (comms_up) acl_poll();
}
