#include "../include/wgt.h"
#include "../include/mem.h"

short *pinit_array;
short *pstartx;
short *pendx;
short *pinten1;
short *pinten2;
short *polyx2;
short *polyy2;
short polygon_buffer_size;

void winitpoly (short maxrows)
{
  int bytes;
  int ctr;

  polygon_buffer_size = maxrows;

  bytes = 2 * maxrows;
  pstartx = malloc (bytes);
  pendx = malloc (bytes);
  pinten1 = malloc (bytes);
  pinten2 = malloc (bytes);
  polyx2 = malloc (bytes);
  polyy2 = malloc (bytes);
  pinit_array = malloc (bytes);
  for (ctr = 0; ctr < maxrows; ctr++)
    pinit_array[ctr] = -16000;
}

void wdeinitpoly (void)
{
  free (pstartx);
  free (pendx);
  free (pinten1);
  free (pinten2);
  free (polyx2);
  free (polyy2);
  free (pinit_array);
}
