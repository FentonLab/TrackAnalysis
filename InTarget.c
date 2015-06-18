#include "SimuPP.h"
#include <math.h>

BOOL InTarget(double dx, double dy, double Radius)
{	
	return (hypot(dx, dy) < Radius);
}
