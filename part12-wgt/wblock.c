#include "wgt.h"
#include "include/mem.h"

block wloadblock (unsigned char *data)
{
  block ptr;
  block orig = NULL;

  short *shortdata = (short *)data;
  unsigned int width, height, size;

  if (data == NULL) {
    return NULL;
  }

  width = *shortdata;
  shortdata ++;
  data += 2;
  height = *shortdata;
  shortdata ++;
  data += 2;

  size = 4 * (width * height) + 12;

  if (size > 12)
    {
     ptr = malloc (size);
     if (ptr == NULL)
       return NULL;
     orig = ptr;

     /* store width and height */
     *ptr++ = width;
     *ptr++ = height;
     for (int i=0;i<width*height;i++) *ptr++ = vgapal[*data++]; // Ugly - means we have to set the palette correctly before we load the block
    }

  return orig;
}
