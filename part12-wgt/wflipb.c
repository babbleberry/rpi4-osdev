#include "wgt.h"
#include "include/mem.h"

#define fastcopy memcpy

void wflipblock (block image, short direction)
{
  int width,height;
  int ctr,ctr2;
  block temp, temp2;
  unsigned int *pr2;

  width = wgetblockwidth (image);         /* Find width and height of block */
  height = wgetblockheight (image);
  temp = malloc (width*4);
  temp2 = malloc (width*4);
  image += 4;

  if  (direction == 1)                       /* Horizontal flip */
  {
    for  (ctr = 1; ctr <= height; ctr++)
      /* Use each row */
    {
      fastcopy (temp, image, width);
      /* Copy row to buffer */

      for  (ctr2 = 0; ctr2 <= width - 1; ctr2++)
	temp2[width - 1 - ctr2] = temp[ctr2];
      /* Reverse elements
      to another buffer */

      fastcopy (image, temp2, width);       /* Copy back to original block */
      image += width;                     /* Advance pr to next row */
    }
  }
  else if  (direction == 0)                  /* Vertical flip */
  {
    pr2 = image + (height*width) - width; /* pr2 points to last row */
    for  (ctr = 1; ctr <= (height / 2); ctr++)
    /* Only flip halfway */
    {
      fastcopy (temp, image, width);        /* Get row of image data */
      fastcopy (image, pr2, width);         /* Copy last row to first */
      fastcopy (pr2, temp, width);       /* Put first row on last */
      image += width;                     /* Move to next row */
      pr2 -= width;                    /* Decrement last row */
    }
  }
  free (temp);
  free (temp2);
}
