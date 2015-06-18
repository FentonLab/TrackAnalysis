#include	"Track.h"

int CalculateDwellMap(TRACK *T, int StartSamp, int NumberOfSamps, MEASURES *m){
	int	i, pixel, n_pixels;
	float	dt, Time = 0.0;
	
	n_pixels = TRACKER_XY_RESOLUTION * TRACKER_XY_RESOLUTION;
	for(i = 0; i < n_pixels; i++)
		m->DwellMap[i] = 0.0;	//initialize the map

	if(T[0].x && T[0].y){	//do the first position
		pixel = (T[0].y * TRACKER_XY_RESOLUTION)+ T[0].x;
		dt = (float)(T[0].time/1000.0); //time increment in seconds
		m->DwellMap[pixel] += dt;
		Time = dt; 
	}

	for(i = 1; i < NumberOfSamps; i++){
		if(T[i].x && T[i].y){
			pixel = (T[i].y * TRACKER_XY_RESOLUTION)+ T[i].x;
			dt = (float)(T[i].time - T[i-1].time)/1000.0;	//time increment in seconds
			m->DwellMap[pixel] += dt;
			Time += dt;
		}
	}
	return (1);
}

