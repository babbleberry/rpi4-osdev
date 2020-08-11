#include "io.h"
#include "bt.h"

#define memcmp         __builtin_memcmp

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

void bt_search()
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
	              for (int c=9;c>=4;c--) echo_addr[9-c] = buf[c];
	              buf += 11;

		      got_echo_sid = 0; got_echo_name = 0; // Reset the search state machine
		      do {
	                 ad_len = buf[0];
	                 unsigned char ad_type = buf[1];
	                 buf += 2;

   		         if (ad_len >= 2) {
			    if (ad_type == 0x03) {
			       unsigned int sid=0;

			       for (int d=0;d<ad_len - 1;d+=2) {
			          sid = buf[d] | (buf[d+1] << 8);
				  if (sid == 0xEC00) {
			             uart_hex(sid); uart_writeText(" ");
			             got_echo_sid = 1;
				  }
			       }
			    } else if (ad_type == 0x09) {
			       char remote_name[ad_len - 1];
		               unsigned int d=0;

		               while (d<ad_len - 1) {
				  remote_name[d] = buf[d];
		                  d++;
		               }
			       if (!memcmp(remote_name,"echo",4)) {
			          uart_writeText(remote_name); uart_writeText(" ");
				  got_echo_name = 1;
			       }
	                    }
	                 }

		         buf += ad_len - 1;
		      } while (buf[1]);
	           }
	        }
             }
          }
       }
    }
}

void bt_conn()
{
    unsigned char *buf;

    while ( (buf = poll()) ) {
       if (data_len >= 2) {
          if (buf[0] == LE_CONNECT_CODE && !connected) {
             connected = !buf[1];
	     uart_hex(connected); uart_writeText(" ");
	     connection_handle = buf[2] | (buf[3] << 8);
	     uart_hex(connection_handle); uart_writeText(" ");
	  }
       }
    }
}

void acl_poll()
{
    while (bt_isReadByteReady()) {
       unsigned char byte = bt_readByte(); 

       if (byte == HCI_EVENT_PKT) {
	  unsigned char opcode = bt_waitReadByte();
	  unsigned char length = bt_waitReadByte();
	  for (int i=0;i<length;i++) bt_waitReadByte();
       } else if (byte == HCI_ACL_PKT) {
	  unsigned char h1 = bt_waitReadByte();
	  unsigned char h2 = bt_waitReadByte();

	  unsigned int handle = h1 | (h2 & 0x0f);
	  unsigned char flags = (h2 & 0xf0) >> 4;

	  h1 = bt_waitReadByte();
	  h2 = bt_waitReadByte();

	  unsigned int length = h1 | (h2 << 8);
	  unsigned char data[length];

	  for (int i=0;i<length;i++) data[i] = bt_waitReadByte();

	  length = data[0] | (data[1] << 8);

	  unsigned int channel = data[2] | (data[3] << 8);
	  unsigned char opcode = data[4];

	  if (opcode == 0x1b) {
             unsigned int from_handle = data[5] | (data[6] << 8);
	     
             for (int c=0;c<length-3;c++) uart_byte(data[7+c]);
	     uart_writeText("\n");
	  }
       }
    }
}

void main()
{
    uart_init();
    bt_init();

    uart_writeText("Initialising Bluetooth: ");
    bt_reset();
    bt_loadfirmware();
    bt_setbaud();
    bt_setbdaddr();

    // Print the BD_ADDR
    unsigned char local_addr[6];
    bt_getbdaddr(local_addr);
    for (int c=5;c>=0;c--) uart_byte(local_addr[c]);
    uart_writeText("\n");

    // Start scanning for echo
    setLEeventmask(0xff);
    startActiveScanning();
    uart_writeText("Waiting for echo: ");
    while (!(got_echo_sid && got_echo_name)) bt_search();
    stopScanning();
    for (int c=0;c<=5;c++) uart_byte(echo_addr[c]);
    uart_writeText("\n");

    // Ask to connect to the echo
    uart_writeText("Connecting to echo: ");
    connect(echo_addr);
    while (!connected) bt_conn();
    uart_writeText("\n");

    // Get the characteristic value
    uart_writeText("Sending read request: ");
    uart_hex(connection_handle); uart_writeText("\n");
    sendACLsubscribe(connection_handle);

    // Into the main infinite loop
    uart_writeText("Waiting for input...\n");
    while (1) {
       acl_poll();
       uart_update();
    }
}
