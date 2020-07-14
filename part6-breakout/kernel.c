#include "io.h"
#include "fb.h"
#include "breakout.h"

void main()
{
    fb_init();
    game();
    while (1);
}
