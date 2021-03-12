#include "wgt.h"

void resize_horizontal_line (block src, block dest, unsigned int whole,
	   unsigned int xfrac, unsigned short step, short length)
/* Where:
   src  : is a pointer to the source bitmap
   dest : is a pointer to the destination screen
   whole: is the amount of pixels to increase the source pointer for every
          destination pixel across
   xfrac: is the initial fractional portion
   step : is the fractional pixel amount to increase the source pointer for
          every destination pixel across.  1 step = 1/256 of a pixel, so
          every 256 fractional steps, the source pointer is increased by 1.
   length : is the number of pixels to draw
*/
{
    unsigned int px;

    for (int i=0; i<length; i++) {
       px = *src;
       xfrac += step;
       if (xfrac > 65535) {
         xfrac %= 65536;
         src++;
       }
       src += whole;
       *dest = px;
       dest++;
    }
}

void resize_horizontal_line_xray (block src, block dest, unsigned int whole,
	   unsigned int xfrac, unsigned short step, short length)
/* Where:
   src  : is a pointer to the source bitmap
   dest : is a pointer to the destination screen
   whole: is the amount of pixels to increase the source pointer for every
          destination pixel across
   xfrac: is the initial fractional portion
   step : is the fractional pixel amount to increase the source pointer for
          every destination pixel across.  1 step = 1/256 of a pixel, so
          every 256 fractional steps, the source pointer is increased by 1.
   length : is the number of pixels to draw
*/
{
    unsigned int px;

    for (int i=0; i<length; i++) {
       px = *src;
       xfrac += step;
       if (xfrac > 65535) {
         xfrac %= 65536;
         src++;
       }
       src += whole;
       if (px != 0) *dest = px;
       dest++;
    }
}

void wresize (short x, short y, short x2, short y2, block image, short mode)
{
long xdif, ydif;
long finalwidth, finalheight, endwidth, endheight;
long origwidth,origheight;
int i;
long endx;

block to, from;
block fromt;
unsigned short whole;
unsigned short step;
unsigned short ywhole;
unsigned short ystep;

unsigned long xstepper, ystepper;
unsigned long yfrac;
unsigned long xfrac;

  origwidth = wgetblockwidth (image);
  origheight = wgetblockheight (image);
  /* Get the original width and height of the block */

  finalwidth = abs (x2 - x) + 1;
  finalheight = abs (y2 - y) + 1;
  /* Find the new width and height */

  xstepper = ((long)(origwidth) << 16) / ((long)(finalwidth));
  /* Calculate the amount to add to the source bitmap for every pixel across.
  This is done using a fixed point number by multiplying it by 65536 (<<16)
  and using 0-65535 as a fractional amount. */

  whole = xstepper >> 16;               /* Keep the whole amount */
  step = xstepper - whole * 65536;
  /* and the fractional amount (1/65536th of a pixel) */

  ystepper = ((long)(origheight) << 16) / ((long)(finalheight));
  ywhole = ystepper >> 16;
  ystep = ystepper - ywhole * 65536;
  /* Do the same with the vertical height */


  to = &abuf[y*WGT_SYS.xres + x];
  /* Make an initial pointer to the destination */
  from = &image[2];
  /* Skip over the width/height integers */

  endwidth = finalwidth;
  endheight = finalheight;

  xdif = 0;
  /* Differences between actual coordinates and */
  ydif = 0;                            /* the clipped coordinates. */
  yfrac = 0;                              /* Reset step to 0 */
  xfrac = 0;                              /* Reset step to 0 */

  /* Do the clipping */
  if  (x < tx)
  {
    xdif = tx - x;
    /* Find the difference between   x<--->tx */
    /* if we know that tx is the greater number */
    to += xdif;
    /* Make sure dest is within left clipping */
    endwidth -= xdif;
    /* Decrease the horizontal number of pixels drawn */
    
    xfrac = (xdif * step) % 65536;

    from += ((int)xdif * xstepper) >> 16;
    /* Now adjust the source pointer according to the whole and step values.
    We multiply the number of pixels and the rate and add this to our
    pointer.  The fractional part is divided by 65536 because we're only
    concerned with the whole pixel amounts. */
  }

  if  (y < ty)
  {
    ydif = ty - y;
    endheight = finalheight - (ydif);
    to += ydif * WGT_SYS.xres;
    yfrac = (ydif * ystep) % 65536;

    /* Do the same for the height */
    from += (((long)ydif * (long)ystepper)>>16) * (long)origwidth;
    /* Same as above but also multiply be the width of the source since we're
       going down a row instead of one pixel across. */
  }

  if  (x + finalwidth - 1 > bx)
     endwidth -= ((x + finalwidth - 1) - bx);
  if  (y + finalheight - 1 > by)
     endheight -= ((y + finalheight - 1) - by);
  /* Clip the right and bottom edges simply by decreasing the number of
  pixels drawn. */


  if  ((x2 > tx) && (y2 > ty) && (endwidth > 0))
  /* If it is even on the screen at all */
  {
    endx = endwidth;

    if  (mode == NORMAL)               /* Plain and simple */
      for  (i = 1; i <= endheight; i++) /* Vertical loop */
    {
      fromt = from;
      resize_horizontal_line (fromt, to, whole, xfrac, step, endx);
      /* Resize one line using our assembly routine */

      to += endx;

      yfrac += ystep;
      if  (yfrac > 65535)
      {
	yfrac -= 65535;
	from += origwidth;
      }

      from += ywhole * origwidth;
      to += WGT_SYS.xres - endwidth;
    }
    else                               /* Use XRAY mode */
      for  (i = 1; i <= endheight; i++)
    {
      fromt = from;
      resize_horizontal_line_xray (fromt, to, whole, xfrac, step, endx);
      to += endx;

      yfrac += ystep;
      if  (yfrac > 65535)
      {
	yfrac -= 65535;
	from += origwidth;
      }

      from += ywhole*origwidth;
      to += WGT_SYS.xres - endwidth;
    }
  }
}
