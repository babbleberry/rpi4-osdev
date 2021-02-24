// Heap memory allocation
#define SAFE_ADDRESS  0x00400000 // Somewhere safe to store a lot of data

// The screen
#define WIDTH         1920
#define HEIGHT        1080
#define MARGIN        30
#define VIRTWIDTH     (WIDTH-(2*MARGIN))
#define FONT_BPG      8

// For the bricks
#define ROWS          5
#define COLS          10

// Gameplay
#define NUM_LIVES     3

extern const int ballradius;
extern const int paddlewidth;

// Object tracking

struct Object
{
    unsigned int type;

    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    unsigned char color;

    unsigned char alive;
    unsigned char redraw;

    unsigned int newx;
    unsigned int newy;
    unsigned char move;
};

enum {
    OBJ_NONE         = 0,
    OBJ_BRICK        = 1,
    OBJ_PADDLE       = 2,
    OBJ_BALL         = 3,
    OBJ_SCOREBOARD   = 4,
    OBJ_ENDGAME      = 5
};

extern volatile unsigned int numobjs;
extern volatile struct Object *objects;
extern volatile struct Object *ball;
extern volatile struct Object *paddle;
extern volatile struct Object *scoreboard;
extern volatile struct Object *endgame;

extern volatile unsigned int comms_up;
extern volatile unsigned char dir;

// Shared functions

void gfx_core(void);
void snd_core(void);
void comms_core(void);
void moveObject(volatile struct Object *object, int x, int y);
