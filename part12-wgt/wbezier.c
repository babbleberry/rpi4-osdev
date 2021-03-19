#include "wgt.h"
#include "include/mem.h"

void wbezier (tpolypoint *rawpts, short numraw, tpolypoint *curvepts, short numcurve)
/* Bezier curve algorithm submitted to us by Claude Morais */
{
  double *combi;
  double deltat, temp;
  double prod,quot;
  double sumx,sumy;
  short  tmp, count;
  
  combi = malloc (sizeof(double)*numraw);

  numraw--;
  deltat = 1.0 / (numcurve - 1);
  combi[0] = combi[numraw] = 1.0;

  for  (tmp = 0; tmp <= (numraw - 2); tmp++)
    combi[tmp + 1] = (combi[tmp] * (numraw - tmp)) / (tmp + 1);
  
  for  (count = 0; count < numcurve; count++)
    {
     if ((temp = count*deltat) <= 0.5)
       {
	quot = prod = 1.0 - temp;
	for  (tmp = 1; tmp < numraw; tmp++)
	  prod *= quot;

	quot = temp / quot;
	sumx = rawpts[numraw].x;
	sumy = rawpts[numraw].y;
	for  (tmp = numraw - 1; tmp >= 0; tmp--)
	  {
	   sumx = combi[tmp]* rawpts[tmp].x + quot* sumx;
	   sumy = combi[tmp]* rawpts[tmp].y + quot* sumy;
	  }
       }
     else
       {
	quot = prod = temp;
	for (tmp = 1; tmp < numraw; tmp++)
	  prod *= quot;

	quot = (1.0 - temp) / quot;
	sumx = rawpts[0].x;
	sumy = rawpts[0].y;
	for (tmp = 1; tmp <= numraw; tmp++)
	  {
	   sumx = combi[tmp]* rawpts[tmp].x + quot* sumx;
	   sumy = combi[tmp]* rawpts[tmp].y + quot* sumy;
	  }
       }
     curvepts[count].x = (sumx*prod);
     curvepts[count].y = (sumy*prod);
    }
  
  free (combi);
}
