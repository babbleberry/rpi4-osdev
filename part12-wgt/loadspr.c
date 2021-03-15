#include "wgt.h"
#include "include/mem.h"

short wloadsprites (color *pal, unsigned char *file, block *image_array, short start,
		    short end)
{
short maxsprite; /* maximum # of sprites in the file */
short startingsprite;  /* First sprite in the file. Version <=3 = 1
						      Version 4   = 0 */
unsigned int a, b, i, spritemade;
char buf[14];
int size;

  a = (unsigned int)*file;
  file += 2;

  /* Get the version number, and change the startingsprite accordingly.
     If version <= 3, maxsprite contains the maximum number of sprites
     that can be stored in a file.  If version > 4, maxsprite contains
     the number of the highest sprite in the file. (empty sprites at
     the end are not kept in the file. */
  if (a <= 3)
    startingsprite = 1;
  else
    startingsprite = 0;

  memcpy_char(&buf, file, 13);
  file += 13;

  if (0 == memcmp (" Sprite File ", &buf[0], 13))
  /* see if it is a sprite file */
    {
     for (int k=0;k<256;k++) {
       pal[k].r = 4 * *file++;
       pal[k].g = 4 * *file++;
       pal[k].b = 4 * *file++;
     }
     wsetpalette (0, 255, pal);

     maxsprite = (short)*file; /* maximum sprites in this file */
     file += 2;

     for (i = start; i <= end; i++) /* make all sprites null
				       (still need to call wfreesprites) */
       image_array[i] = NULL;

     for (i = startingsprite; i <= maxsprite; i++) /* load them in */
       {
        spritemade = (unsigned int)*file; /* flag to see if sprite exists */
        file += 2;
	if (spritemade == 1)
	  {
           a = (unsigned int)*file; /* get width and height */
           file += 2;
           b = (unsigned int)*file;
           file += 2;

	   if ((i >= start) && (i <= end)) /* Load this one */
	     {
	      size = a * b;

	      image_array[i] = (block)malloc ((size * 4) + 12);
              block arr = image_array[i];
 
              *arr++ = a;
              *arr++ = b;

              for (int k=0;k<size;k++) *arr++ = vgapal[*file++];
	     }
	   else /* Don't load this one */
             file += size;
	  }
       }
    }
  return (0);
}

void wfreesprites (block *image_array, short start, short end)
{
short i;

  for (i = start; i <= end; i++)
    {
     if (image_array[i] != NULL)
       {
	free (image_array[i]);
	image_array[i] = NULL;
       }
    }
}
