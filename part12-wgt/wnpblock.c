#include "wgt.h"
#include "include/mem.h"

block wnewblock (short x, short y, short x2, short y2)
{ 
  block ptr,orig;
  int temp,width,height;
  int dispofs;
  int size;
  int ctr;
  
  width = abs (x - x2) + 1;
  height = abs (y - y2) + 1;
  size = (int)width * (int)height + 5;
  if  (x2 < x)
  { 
    temp = x; x = x2; x2 = temp;
  }
  if  (y2 < y)
  { 
    temp = y; y = y2; y2 = temp;
  }
  ptr = malloc (size);
  if  (ptr == NULL)
    return NULL;
  orig = ptr;
  *(short *)ptr = width;   /* store the width */
  ptr += 2;              /* and height */
  *(short *)ptr = height;
  ptr += 2;
  
  dispofs = y * WGT_SYS.xres + x;
  temp = width;
  if (temp > WGT_SYS.xres)
    temp = WGT_SYS.xres;
  for  (ctr = y; ctr <= y2; ctr++)
  { 
    memcpy (ptr, &abuf[dispofs], temp);
    /* read off the screen */
    ptr += width;
    dispofs += WGT_SYS.xres;
    /* into the new ptr */
  }
  return orig;
}

block wallocblock (short width, short height)
{
  block ptr,orig;
  int size;

  size = (int)width * (int)height + 5;
  ptr = malloc (size);
  if  (ptr == NULL)
    return NULL;
  orig = ptr;
  *(short *)ptr = width;   /* store the width */
  ptr += 2;
  *(short *)ptr = height;       /* and height */
  ptr += 2;

  memset (ptr, 0, size - 4);
  return orig;
}
