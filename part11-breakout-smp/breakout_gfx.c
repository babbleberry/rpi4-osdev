#include "breakout.h"
#include "include/fb.h"
#include "include/multicore.h"

// GRAPHICS ROUTINES

void gfx_drawObject(volatile struct Object *object)
{
    const int zoom = WIDTH/192;
    const int strwidth = 10 * FONT_BPG * zoom;
    const int strheight = FONT_BPG * zoom;

    char tens, ones;

    switch(object->type) {
       case OBJ_BALL:
          drawCircle(object->x + ballradius, object->y + ballradius, ballradius, object->alive ? object->color : 0, 1);
          break;
       case OBJ_SCOREBOARD:
          tens = object->y / 10;
          ones = object->y - (10 * tens);

          drawString((WIDTH/2)-252, MARGIN-25, "Score: 0     Lives:  ", object->color, 3);
          drawChar(tens + 0x30, (WIDTH/2)-252 + (8*8*3), MARGIN-25, object->color, 3);
          drawChar(ones + 0x30, (WIDTH/2)-252 + (8*9*3), MARGIN-25, object->color, 3);
          drawChar((char)object->x + 0x30, (WIDTH/2)-252 + (8*20*3), MARGIN-25, object->color, 3);
          break;
       case OBJ_ENDGAME:
          if (object->alive) {
             if (object->x) {
                drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "Game over!", object->color, zoom);
             } else if (object->y) {
                drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "Well done!", object->color, zoom);
             }
          } else {
             drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "          ", 0, zoom);
          }
          break;
       default:
          drawRect(object->x, object->y, object->x + object->width, object->y + object->height, object->alive ? object->color : 0, 1);
          break;
    }
    object->redraw = 0;
}

void gfx_moveObject(volatile struct Object *object)
{
    if (object->type == OBJ_BALL) {
       drawCircle(object->x + ballradius, object->y + ballradius, ballradius, 0, 1);
       object->redraw = 1;
    } else {
       moveRectAbs(object->x, object->y, object->width, object->height, object->newx, object->newy, 0);
    }

    object->x = object->newx; 
    object->y = object->newy;

    object->move = 0;
    object->newx = 0; 
    object->newy = 0;
}

void gfx_update(void)
{
    for (int i=0;i<numobjs;i++) {
       if (objects[i].move) gfx_moveObject(&objects[i]);
       if (objects[i].redraw) gfx_drawObject(&objects[i]);
    }
}

void gfx_core(void)
{
    clear_core1();
    while (1) gfx_update();
}
