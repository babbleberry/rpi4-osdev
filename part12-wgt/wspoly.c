#include "wgt.h"

extern short *pinit_array;
extern short *pstartx;
extern short *pendx;
extern short polygon_buffer_size;

static void wspolyline (short x1, short y1, short x2, short y2)
{
short tmx, tmy, y;
long x, m;

 if (y2 != y1)
   {
    if (y2 < y1)
      {
       tmy = y1;
       y1 = y2;
       y2 = tmy;
       tmx = x1;
       x1 = x2;
       x2 = tmx;
      }

    x = (long)x1 << 8;
    m = ((long)(x2 - x1) << 8) / ((long)(y2 - y1));
    x++;
    y1++;

    for (y = y1; y <= y2; y++)
      {
       if ((y >= 0) && (y < WGT_SYS.yres)) {
	 if (pstartx[y] == -16000) {
	   pstartx[y] = x >> 8;
	 } else {
	   pendx[y] = x >> 8;
         }
       }
       x += m;
      }
   }
}

void wsolidpoly (tpolypoint *vertexlist, short numvertex, short x, short y, void (*customline)(short, short, short))
{
short i;
tpolypoint *curpt, *nextpt;

  if (customline == NULL)
    customline = whline;

  curpt = vertexlist;
  nextpt = vertexlist + 1;

  memcpy (pstartx, pinit_array, polygon_buffer_size << 1);
  memcpy (pendx, pinit_array, polygon_buffer_size << 1);

  for (i = 0; i < numvertex - 1; i++)
    {
     wspolyline (curpt->x + x, curpt->y + y, nextpt->x + x, nextpt->y + y);
     curpt++;
     nextpt++;
    }

  nextpt = vertexlist;
  wspolyline (curpt->x + x, curpt->y + y, nextpt->x + x, nextpt->y + y);

  for (i = 0; i < WGT_SYS.yres; i++)
    {
     if (pstartx[i] != -16000)
       {
	if (pendx[i] == -16000)
	  pendx[i] = pstartx[i];
	(*customline)(pstartx[i], pendx[i], i);
    }
  }
}


void whollowpoly(tpolypoint *vertexlist, short numvertex, short x, short y, short closemode)
{
short i;
tpolypoint *curpt, *nextpt;

  curpt = vertexlist;
  nextpt = vertexlist + 1;

  for (i = 0; i < numvertex - 1; i++)
    {
     wline (curpt->x + x, curpt->y + y, nextpt->x + x, nextpt->y + y);
     curpt++;
     nextpt++;
    }

  if (closemode == CLOSED_POLY)
    {
     nextpt = vertexlist;
     wline (curpt->x + x, curpt->y + y, nextpt->x + x, nextpt->y + y);
    }
}
