#include "wgt.h"
#include "include/mem.h"

extern short bx,by,tx,ty;

void wfreeblock (block ptr)
{
  free (ptr);
}

short wgetblockwidth (block ptr)
{
  return *ptr;                  /* Width is first 2 bytes of data */
}

short wgetblockheight (block ptr)
{
  ptr ++;                       /* Skip width */
  return *ptr;                  /* Height is second 2 bytes of data */
}
