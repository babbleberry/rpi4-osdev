#include "fb.h"
#include "io.h"
#include "bt.h"

// The BLE stuff
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
unsigned char dir = 1;

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

void bt_search()
{
    unsigned char *buf;

    while ( (buf = hci_poll()) ) {
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

    while ( (buf = hci_poll()) ) {
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
	     if (length == 4) {
		dir = data[7];
		uart_byte(dir);
             }
	     uart_writeText("\n");
	  }
       }
    }
}

// The screen
#define WIDTH         1920
#define HEIGHT        1080
#define MARGIN        30
#define VIRTWIDTH     (WIDTH-(2*MARGIN))
#define FONT_BPG      8

// For the bricks
#define ROWS          5
#define COLS          10
unsigned int bricks = ROWS * COLS;

// Gameplay
#define NUM_LIVES     3

// OBJECT TRACKING

struct Object
{
    unsigned int type;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    unsigned char alive;
};

enum {
    OBJ_NONE   = 0,
    OBJ_BRICK  = 1,
    OBJ_PADDLE = 2,
    OBJ_BALL   = 3
};

unsigned int numobjs = 0;
struct Object objects[(ROWS * COLS) + (2 * NUM_LIVES)];
struct Object *ball;
struct Object *paddle;

void removeObject(struct Object *object)
{
    drawRect(object->x, object->y, object->x + object->width, object->y + object->height, 0, 1);
    object->alive = 0;
}

void moveObject(struct Object *object, int xoff, int yoff)
{
    moveRect(object->x, object->y, object->width, object->height, xoff, yoff, 0x00);
    object->x = object->x + xoff;
    object->y = object->y + yoff;
}

struct Object *detectCollision(struct Object *with, int xoff, int yoff)
{
    for (int i=0; i<numobjs;i++) {
	if (&objects[i] != with && objects[i].alive == 1) {
	   if (with->x + xoff > objects[i].x + objects[i].width || objects[i].x > with->x + xoff + with->width) {
              // with is too far left or right to ocllide
	   } else if (with->y + yoff > objects[i].y + objects[i].height || objects[i].y > with->y + yoff + with->height) {
              // with is too far up or down to ocllide
	   } else {
	      // Collision!
	      return &objects[i];
	   }
        }
    }
    return 0;
}

// KEY HANDLER

unsigned char getUart()
{
    unsigned char ch = 0;

    if (uart_isReadByteReady()) ch = uart_readByte();
    return ch;
}

// OBJECT INITIALISERS

void initBricks()
{
    int brickwidth = 32;
    int brickheight = 8;
    int brickspacer = 20;
    int brickcols[5] = { 0x11, 0x22, 0xEE, 0x44, 0x66 };

    int ybrick = MARGIN + brickheight;

    for (int i=0; i<ROWS; i++) {
       int xbrick = MARGIN + (VIRTWIDTH/COLS/2) - (brickwidth/2);
        
       for (int j = 0; j<COLS; j++) {
          drawRect(xbrick, ybrick, xbrick+brickwidth, ybrick+brickheight, brickcols[i], 1);

	  objects[numobjs].type = OBJ_BRICK;
	  objects[numobjs].x = xbrick;
	  objects[numobjs].y = ybrick;
	  objects[numobjs].width = brickwidth;
	  objects[numobjs].height = brickheight;
	  objects[numobjs].alive = 1;
	  numobjs++;

          xbrick += (VIRTWIDTH/COLS);
        }
        ybrick = ybrick + brickspacer;
     }
}

void initBall()
{
    int ballradius = 15;

    drawCircle(WIDTH/2, HEIGHT/2, ballradius, 0x55, 1);

    objects[numobjs].type = OBJ_BALL;
    objects[numobjs].x = (WIDTH/2) - ballradius;
    objects[numobjs].y = (HEIGHT/2) - ballradius;
    objects[numobjs].width = ballradius * 2;
    objects[numobjs].height = ballradius * 2;
    objects[numobjs].alive = 1;
    ball = &objects[numobjs];
    numobjs++;
}

void initPaddle()
{
    int paddlewidth = 80;
    int paddleheight = 20;

    drawRect((WIDTH-paddlewidth)/2, (HEIGHT-MARGIN-paddleheight), (WIDTH-paddlewidth)/2 + paddlewidth, (HEIGHT-MARGIN), 0x11, 1);

    objects[numobjs].type = OBJ_PADDLE;
    objects[numobjs].x = (WIDTH-paddlewidth)/2;
    objects[numobjs].y = (HEIGHT-MARGIN-paddleheight);
    objects[numobjs].width = paddlewidth;
    objects[numobjs].height = paddleheight;
    objects[numobjs].alive = 1;
    paddle = &objects[numobjs];
    numobjs++;
}

void drawScoreboard(int score, int lives)
{
    char tens = score / 10; score -= (10 * tens);
    char ones = score;

    char string[] = "Score: 0xx   Lives: x\0\0";

    string[8] = tens + 0x30;
    string[9] = ones + 0x30;
    string[20] = (char)lives + 0x30;

    drawString((WIDTH/2)-252, MARGIN-25, string, 0x0f, 3);
}

void main()
{
    struct Object *foundObject;

    int lives = NUM_LIVES;
    int points = 0;
    
    int velocity_x = 1;
    int velocity_y = 3;

    uart_init();

    bt_init();
    uart_writeText("Initialising Bluetooth: ");
    bt_reset();
    bt_loadfirmware();
    bt_setbaud();
    bt_setbdaddr();

    fb_init();
    initBricks();
    initBall();
    initPaddle();
    drawScoreboard(points, lives);

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

    // Subscribe to updates
    uart_writeText("Sending subscribe request: ");
    uart_hex(connection_handle); uart_writeText("\n");
    sendACLsubscribe(connection_handle);

    // Begin the game
    uart_writeText("Let the game commence...\n");

    while (lives > 0 && bricks > 0) {
       acl_poll();

       // Get any waiting input and flush the buffer
       if (dir != 1) {
          if (dir == 2) if (paddle->x + paddle->width + (paddle->width / 2) <= WIDTH-MARGIN) moveObject(paddle, paddle->width / 2, 0);
          if (dir == 0) if (paddle->x >= MARGIN+(paddle->width / 2)) moveObject(paddle, -(paddle->width / 2), 0);
       }
       uart_loadOutputFifo();

       // Are we going to hit anything?
       foundObject = detectCollision(ball, velocity_x, velocity_y);

       if (foundObject) {
          if (foundObject == paddle) {
             velocity_y = -velocity_y;
	     // Are we going to hit the side of the paddle
	     if (ball->x + ball->width + velocity_x == paddle->x || ball->x + velocity_x == paddle->x + paddle->width) velocity_x = -velocity_x;
          } else if (foundObject->type == OBJ_BRICK) {
             removeObject(foundObject);
             velocity_y = -velocity_y;
             bricks--;
             points++;
             drawScoreboard(points, lives);
          }
       }

       wait_msec(4000); // Wait a little...
       moveObject(ball, velocity_x, velocity_y);

       // Check we're in the game arena still
       if (ball->x + ball->width >= WIDTH-MARGIN) {
          velocity_x = -velocity_x;
       } else if (ball->x <= MARGIN) {
          velocity_x = -velocity_x;
       } else if (ball->y + ball->height >= HEIGHT-MARGIN) {
          lives--;

	  removeObject(ball);
	  removeObject(paddle);

          initBall();
          initPaddle();
          drawScoreboard(points, lives);
       } else if (ball->y <= MARGIN) {
          velocity_y = -velocity_y;
       }
    }

    int zoom = WIDTH/192;
    int strwidth = 10 * FONT_BPG * zoom;
    int strheight = FONT_BPG * zoom;

    if (bricks == 0) drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "Well done!", 0x02, zoom);
    else drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "Game over!", 0x04, zoom);

    while (1) acl_poll();
}
