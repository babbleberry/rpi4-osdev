#include "../include/wgt.h"
#include "../include/mem.h"

static short start, end;            /* Start and end x coords for filled line */

extern short bx, by, tx, ty;        /* Clipping variables */

typedef struct {
	short x, y, start, finish;  /* values for filled scan line */
	void *next;                 /* Points to next node in linked list */
} node;

static node *push (node *s, short x, short y)
{
  node *newnode;

  s->start = start;                    /* Store start of line */
  s->finish = end;                     /* Store end of line */

  newnode = (node *)malloc (sizeof (node));
  /* Allocate new node */

  newnode->next = (node *)s;           /* Make it point to previous node */
  newnode->x = x;                      /* Store current x and y */
  newnode->y = y;
  return newnode;                      /* Return pointer to new head node */
}

static node *pop (node *s, short *x, short *y)
{
  node *headnode;

  headnode = s;                        /* Store pointer to head node */
  s = (node *)s->next;                 /* Go back in the chain */
  *x = s->x;
  /* Find out where we were last (in that row) */
  *y = s->y;
  start = s->start;                    /* Restore start and end of line */
  end = s->finish;
  free (headnode);                     /* Free head node, no longer needed */
  return s;                            /* Return pointer to new head node */
}

void wregionfill (short x, short y)
{
  short color2fill;                    /* Color to change */
  node *s;                             /* Pointer to head of linked list */
  block ptr;                           /* Used to scan through screen data */

  if  ((x >= tx) && (y >= ty) && (x <= bx) && (y <= by) &&
      (abuf[WGT_SYS.xres * y + x] != currentcolor))
    {
     /* Only fill if within clipping region and color to fill isn't same as
	color we're filling with */
     color2fill = abuf[WGT_SYS.xres * y + x];     /* Find out color to fill */
     s = (node *)malloc (sizeof (node));
     /* Allocate head node */
     s->next = NULL;                    /* No previous nodes */
     s->x = x;                          /* Store current coords */
     s->y = y;

     filllabel1:
     ;
     start = x;
     /* Start of line will be our x position */
     ptr = &abuf[WGT_SYS.xres * y + start - 1];    /* Make pointer to screen data */

     while  ((start > tx) && (*ptr == color2fill))
       {
	/* Now scan left until we hit a different color */
	ptr--;
	start--;
       }
    end = x;

    /* End of line will be our y position */
    ptr = &abuf[WGT_SYS.xres * y + end + 1];

    while  ((end < bx) && (*ptr == color2fill))
      {
       /* Scan right until we hit a different color */
       end++;
       ptr++;
      }

    memset (&abuf[WGT_SYS.xres * y + start], currentcolor, end - start + 1);
    /* Fill line segment */
    filllabel2:
    ;
    if  (y < by)
      {
       /* Proceed to check below every pixel in current line segment to see
	  if we can advance down a row and continue the fill */

       ptr = &abuf[(y + 1) * WGT_SYS.xres + start];
       for  (x = start; x <= end; x++)
	 {
	  if  (*ptr == color2fill)
	    {
	     /* We've found a way to 'leak' into next row, store our
		position */
	     s = push (s, x, y + 1);

	     y++;
	     /* Now move down a row */

	     goto filllabel1;
	    }
	  ptr++;
	 }
      }

    /* The next area of code does the same thing, except checking upwards */
    if  (y > ty)
      {
       ptr = &abuf[(y - 1) * WGT_SYS.xres + start];
       for  (x = start; x <= end; x++)
	 {
	  if (*ptr == color2fill)
	    {
	     s = push (s, x, y - 1);
	     y--;
	     goto filllabel1;
	    }
	  ptr++;
	 }
      }

    /* Now we've filled all we can in one direction, move to previous node */
    if  (s->next != NULL)
      {
       s = pop (s, &x, &y);
       /* Get data for previous node so we can continue scanning */
       goto filllabel2;
      }

    /* Free the node now that we're done with it */
    free (s);
  }
}
