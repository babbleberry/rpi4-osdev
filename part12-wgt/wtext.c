#include "wgt.h"

short xc, yc, cstart, cend, curspeed;
short grid, trans, fore, back;
short wfontsize;

void wouttextxy (short x, short y, wgtfont font, char *string)
{
  short ctr,wd;

  xc = x;
  if  ((grid == 0) | (font != NULL))    /* custom font */
    for  (ctr = 0; ctr < strlen (string); ctr++)
  {
    wd = woutchar (string[ctr], xc, y, font);
    /* output one char */
    xc += wd;                          /* x coord increases by width */
  }
  else
    for  (ctr = 0; ctr < strlen (string); ctr++)
    woutchar (string[ctr], x + ctr - 1, y, font);
  /* output one char */
}

short woutchar (short asc, short xc, short yc, wgtfont font)
{
  short asc_8, i, j, msk,g;
  short maskon, letteron, pixon;
  short col, clp;
  wgtfont orig;
  int ofs, width = 8, height;
  short xclip, yclip;                    /* Clipping */
  short xadd = 0,yadd = 0;
  block temp;                          /* Temporary block to write to abuf */
  wgtfont FONTROM;

  FONTROM = (unsigned char *)&vgafont;

  if  ((asc < 256) && (asc >= 0))
  {
    if  (font == NULL)                   /* default font */
    {
      width = 8;
      if  (grid == 1)
      {
	xc = xc * 8;                     /* Multiply by grid size */
	yc = yc * 8;
      }

      if  (xc + 7 > bx)                /* Check clipping */
	xclip = bx - xc;
      else xclip = 7;
      if  (yc + 7 > by)
	yclip = by - yc;
      else yclip = 7;

      if  (xc < tx)
      {
	xadd = tx - xc;
	xclip -= tx - xc;
	xc = tx;
      }
      if  (yc < ty)
      {
	yadd = ty - yc;
	yclip -= (ty - yc);
	yc = ty;
      }


      if  ((xclip >= 0) && (yclip >= 0))
      {
	maskon = (trans != 0);         /* setup colour transparency flags */
	letteron = (trans != 1);
	asc_8 = asc << 3 ;
	temp = &abuf[yc*WGT_SYS.xres + xc];     /* For quick access */
	
	for  (j = 0; j <= yclip; j++)
	{
	  msk = *((unsigned char *)(FONTROM + asc_8 + j + yadd));
	  /* read font from ROM */
	  msk = msk << xadd;
	  for  (i = 0; i <= xclip; i++)
	  {
	    pixon = ((msk & 1) > 0);
	    if  (letteron & pixon)     /* Put the right colour */
	      *temp = fore;
	    else if  (maskon & !(pixon))
	      *temp = back;
	    msk = msk >> 1;
	    temp++;
	  }
	  temp += WGT_SYS.xres - 1 - xclip;
	}
      }
    }
    else if  (asc < 128)                /* using a custom font */
    {
      orig = font;                      /* start at font ptr */
      font += 15;                       /* skip over header */
      ofs = *(short *)font;             /* read integer offset of char table */
      font = orig + ofs + (2 * asc);    /* reset ptr, move to offset of char table */
      ofs = *(short *)font;             /* read integer offset of char data */
      font = orig + ofs;                /* reset ptr, move to offset of char data */

      width = *(short *)font;           /* read width and height */
      height = *(short *)(font + 2);
      font += 4;

      for  (i = 8; i <= 64; i += 8)    /* make a multiple of 8 */
      {
	/* eg. if width is 7, make it 8 (round up) */
	if  (i >= width)
	  break;
      }
      clp = i;

      if  (xc + width > bx)            /* Check clipping */
	xclip = bx - xc + 1;
      else xclip = width;
      if  (yc + height > by)
	yclip = by - yc + 1;
      else yclip = height;

      if  (yc < ty)
      {
	yadd = ty - yc;
	yclip -= (ty - yc);
	font += (ty - yc)*clp / 8;
	yc = ty;
      }

      if  ((xclip >= 0) && (yclip >= 0))
      {
	if  ((xclip / 8) != clp / 8)
	  clp = (clp - xclip) / 8;
	else clp = 0;

	maskon = (trans != 0);
	letteron = (trans != 1);
	temp = &abuf[yc*WGT_SYS.xres + xc];     /* For quick access */
	col = - 1;
	for  (j = 0; j < yclip; j++)
	{
	  msk = *font;
	  for  (g = 0; g < xclip; g++)
	  {
	    col++;
	    if  (col > 7)
	    {
	      font++;
	      msk = *font;
	      col = 0;
	    }
	    if  (xc + g >= tx)
	    {
	      pixon = ((msk & 128) > 0);
	      if  (letteron && pixon)
		*temp = fore;
	      else if  (maskon && !(pixon))
		*temp = back;
	    }
	    msk = msk << 1;
	    temp++;
	  }
	  temp += WGT_SYS.xres - xclip;
	  font += clp;
	  col = 8;
	}
      }
    }
  }
  return width;
}

void wtextcolor (unsigned int col)
{
  fore = col;
}

void wtextbackground (unsigned int col)
{
  back = col;
}

void wtexttransparent (short transparent)
{
  trans = transparent;
}

void wtextgrid (short onoff)
{
  if ((onoff != 0) & (onoff != 1))
    onoff = 1;
  grid = onoff;
}
