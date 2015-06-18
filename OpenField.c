#include	"Track.h"

void  CalculateLineCrossings(TRACK *T, int StartSamp, int NumberOfSamps, INFO I, OFIELD *of){
// superimposes a line grid on the space and calculates the number of line crossings
	int	i, x, y, x0, y0, t0, TimeStep;
	double	XbinSize, YbinSize, dist;

	// int	Area[MAXIMUM_COORDINATE][MAXIMUM_COORDINATE];

	XbinSize = (int)(TRACKER_XY_RESOLUTION/of->Xbins);
	YbinSize = (int)(TRACKER_XY_RESOLUTION/of->Ybins);
	of->LineCrossings = 0;
	TimeStep = of->TimeStep; 

	// build a map of the pixels 
	/*
	for(y = 0; y < MAXIMUM_COORDINATE; y++){
		row = (int)(y / YbinSize);
		for(x = 0; x < MAXIMUM_COORDINATE; x++){
			col = (int)(x / XbinSize);
			Area[y][x] = row * col + col;
		}
	}
	*/
			
	// find the first good sample
	for(i = StartSamp; i < NumberOfSamps; i++){
		x0 = T[i].x;
		y0 = T[i].y;
		if(x0 || y0){	 //if good point
			t0 = T[i].time;
			break;
		}
	}
	x0 = (int)(x0 / XbinSize);
	y0 = (int)(y0 / YbinSize);
		
	// increment line crossings as a change of rescaled pixel
	for(; i < NumberOfSamps; i++){
		if(T[i].time < (t0 + TimeStep))
			continue;
		t0 += TimeStep;

		x = T[i].x;
		y = T[i].y;
		if(!x && !y)	 //if bad point
			continue;
		x = (int)(x / XbinSize);
		y = (int)(y / YbinSize);
		dist = hypot((double)(y-y0), (double)(x-x0));

		of->LineCrossings+= (int)(dist + 0.6); // round up enough to account for sqrt(2)
		x0 = x;
		y0 = y;
/*
		if(dist > 1.0){
			of->LineCrossings+=2;
			x0 = x;
			y0 = y;
		}else if(dist > 0.0){
			of->LineCrossings++;
			x0 = x;
			y0 = y;
		}
*/
	}


	return;
}
