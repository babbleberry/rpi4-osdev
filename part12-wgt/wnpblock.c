#include "wgt.h"
#include "include/mem.h"

// We should replace this with faster version (and xray needs to do the right thing - not sure memcpy is a good replacement)
#define fastcopy memcpy
#define putxray memcpy

block wnewblock (short x, short y, short x2, short y2)
{ 
  block ptr,orig;
  int temp,width,height;
  int dispofs;
  int size;
  int ctr;
  
  width = abs (x - x2) + 1;
  height = abs (y - y2) + 1;
  size = 4 * ((int)width * (int)height) + 12;
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
  *ptr = width;   /* store the width */
  ptr ++;              /* and height */
  *ptr = height;
  ptr ++;
  
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

  size = 4 * ((int)width * (int)height) + 12;
  ptr = malloc (size);
  if  (ptr == NULL)
    return NULL;
  orig = ptr;
  *ptr = width;   /* store the width */
  ptr ++;
  *ptr = height;       /* and height */
  ptr ++;

  memset (ptr, 0, (size - 12) / 4);
  return orig;
}

void wputblock (short x, short y, block src, short method)
{
  short width, height, display, maxy;
  short ctr;
  block dst;

  if (src == NULL)
    return;
  width = *src;
  src ++;
  height = *src;
  src ++;

  if  (x + width > bx)
    display = (bx + 1) - x;
  else display = width;
  if  (x < tx)
  {
    src += tx - x;
    display -= tx - x;   
    x = tx;
  }                                    /* clip x */
  if (display <= 0)
    return;
  
  maxy = y + height - 1;
  if  (maxy > by) 
    maxy = by;
  if  (y < ty)
  {
    src += (ty - y)*width;   
    y = ty;
  }                                    /* clip y */
  
  dst = &abuf[y * WGT_SYS.xres + x];
  if (method == 0)
    for  (ctr = y; ctr <= maxy; ctr++)
    {
      fastcopy (dst, src, display);
      src += width;
      dst += WGT_SYS.xres;
    } 
  else
    for  (ctr = y; ctr <= maxy; ctr++)
    {
      putxray (dst, src, display);
      src += width;
      dst += WGT_SYS.xres;
    } 
}
