#include "../include/wgt.h"

short wgettextwidth (char *string, wgtfont font)
{
  char asc;
  unsigned short ofs,width;
  short ctr;
  unsigned short totwidth;
  wgtfont orig;

  totwidth = 0;

  for  (ctr = 0; ctr < strlen (string); ctr++)
    /* step through all letters */
  {
    asc = string[ctr];
    /* get ascii code of letter to count */

    if  ((font == NULL) & (string != NULL))
      /* default font is 8x8 */
      totwidth += 8;
    else
    {
      orig = font;                     /* start at font pr */
      font += 15;                      /* skip over header */
      ofs = *(short *)font;            /* read integer offset of char table */
      font = orig;                     /* reset to ptr */
      font += ofs;                     /* move to offset of char table */
      font += 2*asc;
      /* go to offset in table of ascii code */
      ofs = *(short *)font;            /* read integer offset (letter data) */
      font = orig;                     /* reset to ptr */
      font += ofs;                     /* move to offset of letter data */
      width = *(short *)font;          /* read width and height of letter */
      totwidth += width;               /* add the width to the total */
      font = orig;
    }
  }
  return totwidth;
}

short wgettextheight (char *string, wgtfont font)
{
  short asc;
  unsigned short ofs, height;
  short ctr;
  short maxheight;
  wgtfont orig;

  maxheight = 0;
  orig = font;

  if  ((font == NULL) & (string != NULL))
    /* all chars in default font are 8x8 */
    maxheight = 8;
  else
  {
    for  (ctr = 0; ctr < strlen (string); ctr++)
    {
      asc = string[ctr];
      font += 15;                      /* header */
      ofs = *(short *)font;            /* read integer ofs */
      font = orig;                     /* reset */
      font += ofs;                     /* move to ofs */
      font += 2*asc;                   /* go to ofs of data */
      ofs = *(short *)font;            /* read integer ofs */
      font = orig;                     /* reset */
      font += ofs;                     /* move to ofs */

      font += 2;                       /* skip width */
      height = *(short *)font;

      if  (height > maxheight)         /* return the maximum letter height */
	maxheight = height;
      font = orig;
    }
  }
  return maxheight;
}
