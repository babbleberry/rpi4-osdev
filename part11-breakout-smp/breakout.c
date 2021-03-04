#include "breakout.h"
#include "include/fb.h"
#include "include/multicore.h"

// OBJECT INITIALISERS

void initBricks(void)
{
    int brickwidth = 32;
    int brickheight = 8;
    int brickspacer = 20;
    const int brickcols[] = { 0x11, 0x22, 0xEE, 0x44, 0x66 };

    int ybrick = MARGIN + brickheight;

    for (int i=0; i<ROWS; i++) {
       int xbrick = MARGIN + (VIRTWIDTH/COLS/2) - (brickwidth/2);
        
       for (int j = 0; j<COLS; j++) {
	  objects[numobjs].type = OBJ_BRICK;
	  objects[numobjs].x = xbrick;
	  objects[numobjs].y = ybrick;
	  objects[numobjs].width = brickwidth;
	  objects[numobjs].height = brickheight;
	  objects[numobjs].color = brickcols[i];
	  objects[numobjs].alive = 1;
	  objects[numobjs].redraw = 1;
	  objects[numobjs].move = 0;
	  numobjs++;

          xbrick += (VIRTWIDTH/COLS);
        }
        ybrick = ybrick + brickspacer;
     }
}

void initBall(void)
{
    objects[numobjs].type = OBJ_BALL;
    objects[numobjs].x = (WIDTH/2) - ballradius;
    objects[numobjs].y = (HEIGHT/2) - ballradius;
    objects[numobjs].width = ballradius * 2;
    objects[numobjs].height = ballradius * 2;
    objects[numobjs].color = 0x55;
    objects[numobjs].alive = 1;
    objects[numobjs].redraw = 1;
    objects[numobjs].move = 0;
    ball = &objects[numobjs];
    numobjs++;
}

void initPaddle(void)
{
    int paddleheight = 20;
    int startx = MARGIN + (dir * ((VIRTWIDTH - paddlewidth + MARGIN)/100));

    objects[numobjs].type = OBJ_PADDLE;
    objects[numobjs].x = startx;
    objects[numobjs].y = (HEIGHT-MARGIN-paddleheight);
    objects[numobjs].width = paddlewidth;
    objects[numobjs].height = paddleheight;
    objects[numobjs].color = 0x11;
    objects[numobjs].alive = 1;
    objects[numobjs].redraw = 1;
    objects[numobjs].move = 0;
    paddle = &objects[numobjs];
    numobjs++;
}

void initScoreboard(int points, int lives)
{
    objects[numobjs].type = OBJ_SCOREBOARD;
    objects[numobjs].x = lives;
    objects[numobjs].y = points;
    objects[numobjs].width = 0;  // unused
    objects[numobjs].height = 0; // unused
    objects[numobjs].color = 0x0f;
    objects[numobjs].alive = 1;
    objects[numobjs].redraw = 1;
    objects[numobjs].move = 0;
    scoreboard = &objects[numobjs];
    numobjs++;
}

void updateScoreboard(int points, int lives)
{
    while (scoreboard->move || scoreboard->redraw);   // Wait for any current move/redraw to finish

    scoreboard->x = lives;
    scoreboard->y = points;
    scoreboard->redraw = 1;
}

void initEndgame(void)
{
    objects[numobjs].type = OBJ_ENDGAME;
    objects[numobjs].x = 0;
    objects[numobjs].y = 0;
    objects[numobjs].width = 0;  // unused
    objects[numobjs].height = 0; // unused
    objects[numobjs].color = 0;
    objects[numobjs].alive = 0;
    objects[numobjs].redraw = 0;
    objects[numobjs].move = 0;
    endgame = &objects[numobjs];
    numobjs++;
}

void updateEndgame(int gameover, int welldone) 
{
    while (endgame->move || endgame->redraw);   // Wait for any current move/redraw to finish

    endgame->x = gameover;
    endgame->y = welldone;
    endgame->color = welldone ? 0x02 : 0x04;
    endgame->alive = gameover | welldone;
    endgame->redraw = 1;
}

void removeObject(volatile struct Object *object)
{
    while (object->move || object->redraw);   // Wait for any current move/redraw to finish

    object->alive = 0;
    object->redraw = 1;
}

volatile struct Object *detectObjectCollision(volatile struct Object *with, int xoff, int yoff)
{
    for (int i=0; i<numobjs;i++) {
	if (&objects[i] != with && objects[i].alive == 1) {
	   if (with->x + xoff > objects[i].x + objects[i].width || objects[i].x > with->x + xoff + with->width) {
              // with is too far left or right to collide
	   } else if (with->y + yoff > objects[i].y + objects[i].height || objects[i].y > with->y + yoff + with->height) {
              // with is too far up or down to collide
	   } else {
	      // Collision!
	      return &objects[i];
	   }
        }
    }
    return 0;
}

void moveObject(volatile struct Object *object, int x, int y)
{
    while (object->move || object->redraw);   // Wait for any current move/redraw to finish

    object->newx = x;
    object->newy = y;
    object->move = 1;
}

// THE GAME

const int ballradius = 15;
const int paddlewidth = 80;

volatile unsigned char dir = 50;
volatile unsigned int numobjs;

volatile struct Object *objects = (struct Object *)HEAP_ADDRESS;
volatile struct Object *ball;
volatile struct Object *paddle;
volatile struct Object *scoreboard;
volatile struct Object *endgame;

void breakout_init()
{
    numobjs = 0;

    initBricks();
    initBall();
    initPaddle();
    initScoreboard(0, NUM_LIVES);
    initEndgame();
}

void breakout()
{
    volatile struct Object *foundObject;

    int bricks = ROWS * COLS;
    int lives = NUM_LIVES;
    int points = 0;

    int velocity_x = 1;
    int velocity_y = 3;

    while (lives > 0 && bricks > 0) {
       // Are we going to hit anything?
       foundObject = detectObjectCollision(ball, velocity_x, velocity_y);

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
             updateScoreboard(points, lives);
          }
       }

       moveObject(ball, ball->x + velocity_x, ball->y + velocity_y);
       wait_msec(0x186A);

       // Check we're in the game arena still
       if (ball->x + ball->width >= WIDTH-MARGIN) {
          velocity_x = -velocity_x;
       } else if (ball->x <= MARGIN) {
          velocity_x = -velocity_x;
       } else if (ball->y + ball->height >= HEIGHT-MARGIN) {
          lives--;

	  removeObject(ball);
	  removeObject(paddle);

          updateScoreboard(points, lives);
	  if (lives) {
             initBall();
             initPaddle();
	  }
       } else if (ball->y <= MARGIN) {
          velocity_y = -velocity_y;
       }
    }

    // Show endgame state
    if (bricks == 0) {
       removeObject(ball);
       removeObject(paddle);
       updateEndgame(0, 1);
    } else {
       updateEndgame(1, 0);
    }

    wait_msec(0x500000); // Wait 5 seconds

    // Clear endgame state
    updateEndgame(0, 0);

    wait_msec(0x100000); // Wait 1 second
}

void main()
{
    fb_init();                  // Init the screen for debug

    start_core3(comms_core);    // Start the comms engine (core 3)
    while (!comms_up);          // Wait for comms up

    breakout_init();            // Initialise the game engine (core 0)
    start_core1(gfx_core);      // Start the graphics engine (core 1)
    start_core2(snd_core);      // Start the sound engine (core 2)

    while (1) {               
       breakout();              // Run the game
       breakout_init();         // Re-initialise the game engine
    }
}
