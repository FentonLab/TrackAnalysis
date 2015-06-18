// 03.02.2001
// added p Time in sector calculations i.e. CalculateTimePerArea()
// added correction for "pre-mature shock". i.e. if Entrance has not been counted but shock delivered
//  	increment the Entrance count. This is because if the rat is shocked it usually leaves the shock sector
//	and thus doesn't meet the criterion for an Entrance. this corrects for an old error in the shocking function
//	of the Place Avoidance acquisition software.
// 08.03.2001
// Corrections for the encoding of shock that was introduced by Martin. Originally only the start of a shock was
// encoded, however he has marked each frame in which there was a shock.
// Also corrected the methods for calculating entrances. It was originally based on time, now it is based on samples.
// 13.02.2008
// Correction for failure of Tracker 2.2 and lower to set the OutsideRefractory state correctly in PlaceAvoidance
// if((((I.FileFormat == TRACKER_FORMAT) || (I.FileFormat == ITRACK_FORMAT)) && (I.FileFormatVersion <= 2.2)) && ((TimeOfLastEntrance < (t - I.Shock.MinISI)) || (TimeOfLastEntrance < (t - I.Shock.ResetInterval)))){
// 10.06.12 added calculations of speed at times of first and second entrances to PA_StateBasedEntranceMeasures()
// 14.08.12 completed calculations of speed at times of first and second entrances to PA_StateBasedEntranceMeasures()
 
#include	"Track.h"
#define RAD_PER_DEG (M_PI/180.0)

void	PathLengthMeasures(TRACK *T, int StartSamp, int NumberOfSamps, INFO Info, MEASURES *m)  
{
	double d, x0, y0, x1, y1;
	double LinearityX0, LinearityY0, LinearityX1, LinearityY1;
	double Integrated, PixelsPerCM;
	double  SS = 0.0;
	int	i, n, previous, t0, LinearityN, LinearityT0;
	int	TimeStep;

	TimeStep = Info.TimeStep;
	PixelsPerCM = Info.PixelsPerCM;
	Integrated = 0.0;
	m->Linearity = 0.0;
	m->PathLength = 0.0;
	m->Speed = 0.0;
	m->sdSpeed = 0.0;
	m->Duration = 0.0;
	m->Misses = 0;
	m->NumberOfShocks = 0;

	strcpy(m->FrameName, "?");
	if((Info.FileFormat == TRACKER_FORMAT) || (Info.FileFormat == ITRACK_FORMAT) || (Info.FileFormat == FGX_FORMAT)){
		if(!strcmp("RoomFrame", Info.Frame)){
			strcpy(m->FrameName, "room");
		}else if (!strcmp("ArenaFrame", Info.Frame)){
			strcpy(m->FrameName, "arena");
		}
	}else{
		if(m->frame == ROOM_FRAME){
			strcpy(m->FrameName, "room");
		}else if (m->frame == ARENA_FRAME){
			strcpy(m->FrameName, "arena");
		}
	}

	n = 0;
	LinearityN = 0;
	// get first samples
	for(i = 0; i < NumberOfSamps; i++){
		if(T[i].x && T[i].y){
			x0 = LinearityX0 = (double)T[i].x;
			y0 = LinearityY0 = (double)T[i].y;
			LinearityT0 = t0 = T[i].time;
			previous = i;
			break;
		}
		m->Misses++;
	}
	for(; i < NumberOfSamps; i++){
		// for computing tortuosity
		if(!T[i].x && !T[i].y){
			m->Misses++;
			continue;
		}

		Integrated += hypot(T[i].x - T[previous].x, T[i].y - T[previous].y);
		previous = i;
		
		if((T[i].time - t0) >= TimeStep){
			if(T[i].x && T[i].y){
				x1 = (double)T[i].x;
				y1 = (double)T[i].y;
			
				// estimate movement
				d = hypot(x1 - x0, y1 - y0);
				n++;
				m->PathLength += d;
				SS += d * d;

				t0 = T[i].time;
				x0 = x1;
				y0 = y1;
			}
		}

		// calculate linearity at LINEARITY_SCALE_FACTOR * time step i.e. 2 sec
		if((T[i].time - LinearityT0) >= (TimeStep * LINEARITY_SCALE_FACTOR)){
			if(T[i].x && T[i].y){
				LinearityX1 = (double)T[i].x;
				LinearityY1 = (double)T[i].y;
			
				// estimate movement
				d = hypot(LinearityX1 - LinearityX0, LinearityY1 - LinearityY0);
				LinearityN++;
				if(Integrated > 0.0)
					m->Linearity += (d / Integrated);

				LinearityT0 = T[i].time;
				LinearityX0 = LinearityX1;
				LinearityY0 = LinearityY1;
				Integrated = 0.0;
			}
		}
	}
	if(NumberOfSamps == 0)
		return;

	// if(m->PathLength == 0.0)
	// 	return;

	if(LinearityN)
		m->Linearity /= LinearityN;
	else
		m->Linearity = -1.0;

	// find the last good sample to get the recording duration
	i = NumberOfSamps;
	while(i > 0){
		if(T[--i].time > 0)
			break;
	}
	m->Duration = (float)(T[i].time - T[0].time);	// in msec
	m->PathLength /= (double)(PixelsPerCM * 100.0);
	m->Speed = m->PathLength / (float)m->Duration * 1000.0 * 100.0;
	m->Duration = (m->Duration / 1000.0);	// in sec
	if(m->PathLength == 0){
		m->sdSpeed = -1.0;
	}else{
		m->sdSpeed = (n * SS) - (m->PathLength * m->PathLength) / (n * (n - 1));
		m->sdSpeed = sqrt((double)m->sdSpeed);
		m->sdSpeed /= (PixelsPerCM * 1000.0);
	}
	m->NumberOfSamples = NumberOfSamps;

	return;
}

void	PA_TimeBasedEntranceMeasures(TRACK *T, int	StartSamp, int NumberOfSamps, INFO I, unsigned char **ReinforcedMap, MEASURES *m, int frame)  
{
	int TimeStep;
	double PixelsPerCM;
	MEASURES	temp;
	int		SampleOfLastEntrance, TimeOfLastEntrance;
	int 	FirstEntranceSample = 0, FirstShockSample = 0;
	int 	SecondEntranceSample = 0, SecondShockSample = 0;
	int		MaxAvoidedIntervalSample1, MaxAvoidedIntervalSample2;
	int	i, x, y, t, TimeOfEntrance, TimeOfLoss, TimeOfLeaving, Entrances;
	int	MinISI, ResetInterval;
	int	ShockedInThisSample, TimeOfLastShock;
	int	EntranceState;	// 0 Out, 1 Entering, 2 Entrance, 3 lostEntrance, 4 Leaving
	int	Latency;	//			*			 *
				//   ---0---*-1--2- 3 --2*-4----0--->
				//			*---shocked--*	

	// FGX definition of state in .dat file
	// 0 Out, 1 Entering (prior to shock), 2 Shock, 3 Intershock interval during an entrance

	Latency = I.Shock.Latency;
	ResetInterval = I.Shock.ResetInterval;
	MinISI = I.Shock.MinISI;
	TimeStep = I.TimeStep;
	PixelsPerCM = I.PixelsPerCM;
	Entrances = 0;
	TimeOfLastEntrance = T[0].time;
	SampleOfLastEntrance = 0;
	MaxAvoidedIntervalSample1 = MaxAvoidedIntervalSample2 = 0;


	m->NumberOfShocks = 0;
	m->PathToFirstEntrance = m->PathToSecondEntrance = m->PathLength;
	m->TimeToFirstEntrance = m->TimeToSecondEntrance = m->Duration;
	m->PathToFirstShock = m->PathToSecondShock = m->PathLength;
	m->TimeToFirstShock = m->TimeToSecondShock = m->Duration;
	m->MaxTimeAvoided = 0.0;
	m->MaxPathAvoided = 0.0;

	switch(I.FileFormat){
	case TRACKER_FORMAT:
	case ITRACK_FORMAT:
	case FGX_FORMAT:
		EntranceState = 0;
		TimeOfLastShock = - MinISI;
		for(i = 0; i < NumberOfSamps; i++){
			ShockedInThisSample = 0;
			
			x = T[i].x;
			y = T[i].y;
			t = T[i].time;

			if(T[i].event == 2){
				ShockedInThisSample = 1;
				if((t - TimeOfLastShock) > MinISI){	//check that this is a 'new' shock
					m->NumberOfShocks ++;
					TimeOfLastShock = t;
					if(m->NumberOfShocks == 1){
						m->TimeToFirstShock = ((t - T[0].time) / 1000.0);
						FirstShockSample = i;
					}
					if(m->NumberOfShocks == 2){
						m->TimeToSecondShock = ((t - T[0].time) / 1000.0);
						SecondShockSample = i;
					}
				}
	
			}
			
			if(!x && !y){	// Lost
				if(EntranceState == 0)  // Out
					continue;
				if(EntranceState == 1)	// Entering
					continue;
				if (EntranceState == 2){ // Entrance but now lost
					TimeOfLoss = t;
					EntranceState = 3;
					continue;
				}
				if (EntranceState == 3) // lostEntrance
					continue;
				if (EntranceState == 4) // Leaving
					continue;
			}

	
			if(ReinforcedMap[y][x] == TARGET){  // Rat is in shocked area
	
				if (EntranceState == 0) { // Out -> In
					EntranceState = 1;
					TimeOfEntrance = t;
					if(!ShockedInThisSample)
						continue;
				}
	
				ENTERING_STATE_FGX:
				if (EntranceState == 1) {				// Entering
										// if shocked then count as an entrance 
					if((ShockedInThisSample) || ((t - TimeOfEntrance) >= Latency)){ // A new Entrance
						EntranceState = 2;
						Entrances++;

						if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
							m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
							MaxAvoidedIntervalSample1 = SampleOfLastEntrance;
							MaxAvoidedIntervalSample2 = i;
						}
						TimeOfLastEntrance = t;
						SampleOfLastEntrance = i;

						if(Entrances == 1){
							m->TimeToFirstEntrance = ((t - T[0].time) / 1000.0);
							FirstEntranceSample = i;
						}
						if(Entrances == 2){
							m->TimeToSecondEntrance = ((t - T[0].time) / 1000.0);
							SecondEntranceSample = i;
						}
						continue;
					}else{
						continue;
					}
				}
				if (EntranceState == 2)	// Entrance
					continue;
	
				if (EntranceState == 3){ // lostEntrance
					if((t - TimeOfLoss) >= ResetInterval){  // Lost for too long:
						EntranceState = 1;						//  Consider this a new entering 
						TimeOfEntrance = t;
						if(ShockedInThisSample)		// if shocked then count as an entrance
							goto ENTERING_STATE_FGX;
						continue;
					}else{									// Lost only briefly
						EntranceState = 2;						// Reinstate the ongoing Entrance
						continue;							
					}
				}
				if (EntranceState == 4){					// Leaving
					if((t - TimeOfLeaving) >= MinISI){	// Away for too long:
						EntranceState = 1;				// Consider this a new entering
						TimeOfEntrance = t;				// This should never happen but just in case...
					}else{
						EntranceState = 2;				// Still part of the last Entrance
						continue;
					}
				}
			}else{	
	
				if (EntranceState == 0) { // Out
					continue;
				}
				if (EntranceState == 1) {	// Entering
					if((t - TimeOfEntrance) >= MinISI){ 
						EntranceState = 0;
					}
					continue;							
				}
				if (EntranceState == 2)	{	// Entrance
					EntranceState = 4;		// now leaving
					TimeOfLeaving = t;
					continue;
				}
				if (EntranceState == 3){	// lostEntrance
					if((t - TimeOfLoss) >= ResetInterval){  // Lost for too long:
						EntranceState = 0;					//  Out 
						continue;
					}else{									// Lost only briefly
						EntranceState = 4;						// a new Leaving
						TimeOfLeaving = t;
						continue;							
					}
				}
				if (EntranceState == 4){				// Leaving
					if((t - TimeOfLeaving) >= MinISI){	// Away for too long:
						EntranceState = 0;				// Out	
					}else{								// Still leaving
						continue;
					}
				}
			}
		}
		break;

	case DTtracker_FORMAT:
		EntranceState = 0;
		TimeOfLastShock = - MinISI;
		for(i = 0; i < NumberOfSamps; i++){
			ShockedInThisSample = 0;
			
			x = T[i].x;
			y = T[i].y;
			t = T[i].time;
			
			if((frame == ROOM_FRAME) && ((T[i].event == 1) || (T[i].event == 3))){
				ShockedInThisSample = 1;
				if((t - TimeOfLastShock) > MinISI){	//check that this is a 'new' shock
					m->NumberOfShocks ++;
					TimeOfLastShock = t;
					if(m->NumberOfShocks == 1){
						m->TimeToFirstShock = ((t - T[0].time) / 1000.0);
						FirstShockSample = i;
					}
					if(m->NumberOfShocks == 2){
						m->TimeToSecondShock = ((t - T[0].time) / 1000.0);
						SecondShockSample = i;
					}
				}
	
			}else if((frame == ARENA_FRAME) && ((T[i].event == 2) || (T[i].event == 3))){
					ShockedInThisSample = 1;
					if((T[i].time - TimeOfLastShock) > MinISI){	//check that this is a 'new' shock
						m->NumberOfShocks ++;
						TimeOfLastShock = t;
						if(m->NumberOfShocks == 1){
							m->TimeToFirstShock = ((t - T[0].time) / 1000.0);
							FirstShockSample = i;
						}
						if(m->NumberOfShocks == 2){
							m->TimeToSecondShock = ((t - T[0].time) / 1000.0);
							SecondShockSample = i;
						}
					}
			}
			
			if(!x && !y){	// Lost
				if(EntranceState == 0)  // Out
					continue;
				if(EntranceState == 1)	// Entering
					continue;
				if (EntranceState == 2){ // Entrance but now lost
					TimeOfLoss = t;
					EntranceState = 3;
					continue;
				}
				if (EntranceState == 3) // lostEntrance
					continue;
				if (EntranceState == 4) // Leaving
					continue;
			}

	
			if(ReinforcedMap[y][x] == TARGET){  // Rat is in shocked area
	
				if (EntranceState == 0) { // Out -> In
					EntranceState = 1;
					TimeOfEntrance = t;
					if(!ShockedInThisSample)
						continue;
				}
	
				ENTERING_STATE_DT:
				if (EntranceState == 1) {				// Entering
										// if shocked then count as an entrance 
					if((ShockedInThisSample) || ((t - TimeOfEntrance) >= Latency)){ // A new Entrance
						EntranceState = 2;
						Entrances++;

						if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
							m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
							MaxAvoidedIntervalSample1 = SampleOfLastEntrance;
							MaxAvoidedIntervalSample2 = i;
						}
						TimeOfLastEntrance = t;
						SampleOfLastEntrance = i;
						
						if(Entrances == 1){
							m->TimeToFirstEntrance = ((t - T[0].time) / 1000.0);
							FirstEntranceSample = i;
						}
						if(Entrances == 2){
							m->TimeToSecondEntrance = ((t - T[0].time) / 1000.0);
							SecondEntranceSample = i;
						}
						
						continue;
					}else{
						continue;
					}
				}
				if (EntranceState == 2)	// Entrance
					continue;
	
				if (EntranceState == 3){ // lostEntrance
					if((t - TimeOfLoss) >= ResetInterval){  // Lost for too long:
						EntranceState = 1;						//  Consider this a new entering 
						TimeOfEntrance = t;
						if(ShockedInThisSample)		// if shocked then count as an entrance
							goto ENTERING_STATE_DT;
						continue;
					}else{									// Lost only briefly
						EntranceState = 2;						// Reinstate the ongoing Entrance
						continue;							
					}
				}
				if (EntranceState == 4){					// Leaving
					if((t - TimeOfLeaving) >= MinISI){	// Away for too long:
						EntranceState = 1;				// Consider this a new entering
						TimeOfEntrance = t;				// This should never happen but just in case...
					}else{
						EntranceState = 2;				// Still part of the last Entrance
						continue;
					}
				}
			}else{	
	
				if (EntranceState == 0) { // Out
					continue;
				}
				if (EntranceState == 1) {	// Entering
					if((t - TimeOfEntrance) >= MinISI){ 
						EntranceState = 0;
					}
					continue;							
				}
				if (EntranceState == 2)	{	// Entrance
					EntranceState = 4;		// now leaving
					TimeOfLeaving = t;
					continue;
				}
				if (EntranceState == 3){	// lostEntrance
					if((t - TimeOfLoss) >= ResetInterval){  // Lost for too long:
						EntranceState = 0;					//  Out 
						continue;
					}else{									// Lost only briefly
						EntranceState = 4;						// a new Leaving
						TimeOfLeaving = t;
						continue;							
					}
				}
				if (EntranceState == 4){				// Leaving
					if((t - TimeOfLeaving) >= MinISI){	// Away for too long:
						EntranceState = 0;				// Out	
					}else{								// Still leaving
						continue;
					}
				}
			}
		}
		break;
	}
	m->Entrances = Entrances;

	if(m->NumberOfShocks){
		PathLengthMeasures(T, 0, FirstShockSample, I,  &temp); // get the path to first shock
		m->PathToFirstShock = temp.PathLength;
	}else{
		m->PathToFirstShock = m->PathLength;
	}

	if(m->NumberOfShocks >= 2){
		PathLengthMeasures(T, 0, SecondShockSample, I,  &temp); // get the path to first shock
		m->PathToSecondShock = temp.PathLength;
	}else{
		m->PathToSecondShock = m->PathLength;
	}

	if(m->Entrances){
		PathLengthMeasures(T, 0, FirstEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToFirstEntrance = temp.PathLength;
		PathLengthMeasures(T, MaxAvoidedIntervalSample1, (MaxAvoidedIntervalSample2 - MaxAvoidedIntervalSample1), I,  &temp); 
		m->MaxPathAvoided = temp.PathLength;
	}else{
		m->PathToFirstEntrance = m->PathLength;
		m->MaxPathAvoided = m->PathLength;
		m->MaxTimeAvoided = (m->Duration * 1000.0);
	}

	if(m->Entrances >= 2){
		PathLengthMeasures(T, 0, SecondEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToSecondEntrance = temp.PathLength;
	}else{
		m->PathToSecondEntrance = m->PathLength;
	}

	if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
		m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
		MaxAvoidedIntervalSample1 = SampleOfLastEntrance;
		MaxAvoidedIntervalSample2 = i;
	}
	m->MaxTimeAvoided = (m->MaxTimeAvoided / 1000.0);

		
	return;
}

void	PP_TimeBasedEntranceMeasures(TRACK *T, int StartSamp, int NumberOfSamps, INFO I, unsigned char **ReinforcedMap, MEASURES *m, int Frame)  
{
	double PixelsPerCM;
	MEASURES	temp;
	PPInfo		pp;
	FEEDER_STATE	FeederState;
	int     LastInOut, dataInOut, InOut;
	unsigned long Time, dataTime, LastGoodSampleTime, StartTime;
	int	SampleOfLastEntrance, TimeOfLastEntrance;
	int 	FirstEntranceSample = 0, FirstShockSample = 0;
	int 	SecondEntranceSample = 0, SecondShockSample = 0;
	int	MaxNoRewardIntervalSample1, MaxNoRewardIntervalSample2;
	int	i, x, y;
	int	InCriterion, OutCriterion;	

	InCriterion = (int)(I.Feeder[Frame].Trigger);
	OutCriterion = (int)(I.Feeder[Frame].Refractory);
	PixelsPerCM = I.PixelsPerCM;
	TimeOfLastEntrance = T[0].time;
	SampleOfLastEntrance = 0;
	MaxNoRewardIntervalSample1 = MaxNoRewardIntervalSample2 = 0;

	m->PathToFirstEntrance = m->PathToSecondEntrance = m->PathLength;
	m->TimeToFirstEntrance = m->TimeToSecondEntrance = m->Duration;
	m->PathToFirstShock = m->PathToSecondShock = m->PathLength;
	m->TimeToFirstShock = m->TimeToSecondShock = m->Duration;
	m->MaxTimeAvoided = 0.0;
	m->MaxPathAvoided = 0.0;
	m->Entrances = 0;

	switch(I.FileFormat){
		case TRACKER_FORMAT:
		case ITRACK_FORMAT:
		case FGX_FORMAT:
			// initialize values in	pp structure
			pp.m_NumberOfEntrances = 0;
			pp.m_StopWait =	0;		
			pp.m_StopRefractory = 0;		
			pp.m_FeederEntranceLatency = InCriterion; 
			pp.m_FeederExitLatency = OutCriterion;
			pp.m_Food = FeederState	= out;

			LastInOut = InOut =	dataInOut =	0;

			// used	to decide when to call DecideFood
			Time = StartTime = 0;

			// The values used by FGX for defining the target were doubles determined from polar coordinates (angle	and	radius)
			// The values in the header	are	rounded	to the nearest pixel (int) and so ocassionally the FGX and TA differ in
			m->TimeToFirstShock = m->TimeToSecondShock = m->Duration;
			m->MaxTimeAvoided = 0.0;
			m->MaxPathAvoided = 0.0;
			m->Entrances = 0;

			for(i =	0; i < NumberOfSamps; i++){
				x =	T[i].x;
				y =	T[i].y;
				dataTime  =	T[i].time;

				if(x &&	y){

					LastInOut =	(ReinforcedMap[y][x] ==	TARGET)	? 1:0;	// is Rat is in	reinforced area?

					LastGoodSampleTime = dataTime;
				}else if((dataTime - LastGoodSampleTime) > POSITION_SAMPLING_INTERVAL){
					LastInOut =	0;
				}

				// set Time. It	is needed for DecideFood.
				// The control thread's	timing is independent of the occurence of a	frame event
				// and it was done by telling windows to create	an Event each 100 ms (POSITION_SAMPLING_INTERVAL)
				// FGX didn't Decide what too do until intervals of	100	ms elapsed
				// This	is an approximation	since the Timing that was used was not 'additive' i.e. it was based	on the Windows
				// timer creating an event each	100	ms (+/-	10 ms)
				// The exact time of this Event	was	not	stored in the data file. The clock started at "StartTime" which	I assume to	be 0 but could be
				// up to 30	ms depending on	when FGX finished all the housekeeping stuff before	beginning tracking.

				if(dataTime >= Time){
					 //	InOut is actually from the previous	sample
					InOut =	LastInOut;
					FeederState = DecideFood(Time, InOut, &pp);
					Time +=	POSITION_SAMPLING_INTERVAL;
					//Time =	dataTime;

					switch (FeederState){
						case 0:	 //	Out	
							break;
						case 1:	// InWait
							break;
						case 2:	// Feed
							switch(m->Entrances){
								case 0:
									m->TimeToFirstEntrance = ((dataTime - T[0].time) / 1000.0);
									FirstEntranceSample	= i;
									break;
								case 1:
									m->TimeToSecondEntrance	= ((dataTime - T[0].time) / 1000.0);
									SecondEntranceSample = i;
									break;
								default:
									break;
							}
							m->Entrances++;
	
							if(m->MaxTimeAvoided < (float)(dataTime - TimeOfLastEntrance)){
								m->MaxTimeAvoided = (float)(dataTime - TimeOfLastEntrance);
								MaxNoRewardIntervalSample1 = SampleOfLastEntrance;
								MaxNoRewardIntervalSample2 = i;
							}
							TimeOfLastEntrance = dataTime;
							SampleOfLastEntrance = i;
							break;
						case 3:	// In
							break;
						case 4:	// OutWait
							break;
						default:	// error in	algorithm
							fprintf(stderr,"Error in PP_TimeBasedEntrances\nTerminating. Fix the algorithm\n");
							fprintf(stderr,"sample=%d, State=%d InTarget=%d\n", i, FeederState, InOut);
							exit(-1);
					}
				}
			}
		default:
			break;
	} // end switch FileFormat

	if(m->Entrances){
		PathLengthMeasures(T, 0, FirstEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToFirstEntrance = temp.PathLength;
		PathLengthMeasures(T, MaxNoRewardIntervalSample1, (MaxNoRewardIntervalSample2 - MaxNoRewardIntervalSample1), I,  &temp); 
		m->MaxPathAvoided = temp.PathLength;
	}else{
		m->PathToFirstEntrance = m->PathLength;
		m->MaxPathAvoided = m->PathLength;
		m->MaxTimeAvoided = (m->Duration * 1000.0);
	}

	if(m->Entrances >= 2){
		PathLengthMeasures(T, 0, SecondEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToSecondEntrance = temp.PathLength;
	}else{
		m->PathToSecondEntrance = m->PathLength;
	}

	if(m->MaxTimeAvoided < (float)(dataTime - TimeOfLastEntrance)){
		m->MaxTimeAvoided = (float)(dataTime - TimeOfLastEntrance);
		MaxNoRewardIntervalSample1 = SampleOfLastEntrance;
		MaxNoRewardIntervalSample2 = i;
	}
	m->MaxTimeAvoided = (m->MaxTimeAvoided / 1000.0);

	return;
}

void	PA_SampleBasedEntranceMeasures(TRACK *T, int	StartSamp, int NumberOfSamps, INFO I, unsigned char **ReinforcedMap, MEASURES *m, int frame)  
{
	int TimeStep;
	double PixelsPerCM;
	MEASURES	temp;
	int	ThisSector, InSector;
	int	TimeOfLastEntrance;
	int 	FirstEntranceSample = 0, FirstShockSample = 0;
	int 	SecondEntranceSample = 0, SecondShockSample = 0;
	int	i, x, y, t, SampleOfLoss, SampleOfLeaving, Entrances;
	int	MinISI, ResetInterval;
	int	EntranceInThisSample, ShockInThisSample, SampleWithLastShock;
	int	EntranceState, StateBeforeLoss;
	int	Latency;
	int	SampleOfEntering, SampleOfShock, SampleOfLastEntrance, MaxAvoidedIntervalSample1, MaxAvoidedIntervalSample2;

	

	// DTtracker definition of state
	// 0 Out, 1 Latency, 2 Entrance, 3 Entered, 4 Refractory
	//			*			 *
	//   ---0---*-1--2- 3 --2*-4----0--->
	//			*---shocked--*	


	Latency = I.Shock.Latency;
	ResetInterval = I.Shock.ResetInterval;
	MinISI = I.Shock.MinISI;
	TimeStep = I.TimeStep;
	PixelsPerCM = I.PixelsPerCM;
	Entrances = 0;
	TimeOfLastEntrance = T[0].time;
	SampleOfLastEntrance = 0;
	MaxAvoidedIntervalSample1 = MaxAvoidedIntervalSample2 = 0;
	
	m->NumberOfShocks = 0;
	m->PathToFirstEntrance = m->PathToSecondEntrance = m->PathLength;
	m->TimeToFirstEntrance = m->TimeToSecondEntrance = m->Duration;
	m->PathToFirstShock = m->PathToSecondShock = m->PathLength;
	m->TimeToFirstShock = m->TimeToSecondShock = m->Duration;
	m->MaxTimeAvoided = 0.0;
	m->MaxPathAvoided = 0;

	EntranceState = 0;
	SampleWithLastShock = - MinISI;

	switch(frame){
		case ROOM_FRAME:
			ThisSector = 1;
			break;
		case ARENA_FRAME:
			ThisSector = 2;
			break;
		default:
			fprintf(MyStdErr,"Undefined frame in PA_SampleBasedEntranceCalculations. Coding Error\n");
			exit(-1);
	}

	for(i = 0; i < NumberOfSamps; i++){
		x = T[i].x;
		y = T[i].y;
		t = T[i].time;

		EntranceInThisSample = ShockInThisSample = 0;	
		if(!x && !y){	// Lost rat
			if(EntranceState != PA_STATE_Undefined)
				StateBeforeLoss = EntranceState;
			EntranceState = PA_STATE_Undefined;
			SampleOfLoss = i;
			continue;
		}
		InSector = (T[i].Sector == ThisSector) ? 1 : 0;
		if(!InSector){
			switch(EntranceState){
				case PA_STATE_Undefined: // Previously Lost so first Restore State
					if((t - T[SampleOfLoss].time) < ResetInterval)
						EntranceState = StateBeforeLoss;
					else
						EntranceState = PA_STATE_OutsideSector;
				// now update state
				case PA_STATE_OutsideSector:
				case PA_STATE_EntranceLatency:
					EntranceState = PA_STATE_OutsideSector;
					break;
				case PA_STATE_Shock: 
				case PA_STATE_InterShockLatency: 
					EntranceState = PA_STATE_Refractory;
					SampleOfLeaving = i;
					break;
				case PA_STATE_Refractory:
					if((t - T[SampleOfLeaving].time) >= ResetInterval) 
						EntranceState = PA_STATE_OutsideSector;
				default:
					break;
			}
			// T[i].event = PA_STATE_OutsideSector;
			continue;
		}

		// rat is InSector
		switch(EntranceState){
			case PA_STATE_Undefined: // Previously Lost so first Restore State
				if((t - T[SampleOfLoss].time) < ResetInterval)
					EntranceState = StateBeforeLoss;
				else
					EntranceState = PA_STATE_OutsideSector;
			// now update state
			case PA_STATE_OutsideSector:	// Out
				EntranceState = PA_STATE_EntranceLatency;
				SampleOfEntering = i;
				break;
			case PA_STATE_EntranceLatency:
				if((t - T[SampleOfEntering].time) >= Latency){
					EntranceState = PA_STATE_Shock;
					SampleOfShock = i;
					EntranceInThisSample = ShockInThisSample = 1;
				}else
					EntranceState = 1;
				break;
			case PA_STATE_Shock:
				if((t - T[SampleOfShock].time) >= MinISI){
					EntranceState = PA_STATE_Shock;
					SampleOfShock = i;
					ShockInThisSample = 1;
				}else
					EntranceState = PA_STATE_InterShockLatency;
				break;
			case PA_STATE_InterShockLatency:
				if((t - T[SampleOfShock].time) >= MinISI){
					EntranceState = PA_STATE_Shock;
					SampleOfShock = i;
					ShockInThisSample = 1;
				}else
					EntranceState = PA_STATE_InterShockLatency;
				break;
			case PA_STATE_Refractory:
				EntranceState = PA_STATE_InterShockLatency;
			default:
				break;
		}

		if(EntranceInThisSample){
			Entrances++;
			if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
				m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
				MaxAvoidedIntervalSample1 = SampleOfLastEntrance;
				MaxAvoidedIntervalSample2 = i;
			}
			TimeOfLastEntrance = t;
			SampleOfLastEntrance = i;

			if(Entrances == 1){
				m->TimeToFirstEntrance = ((t - T[0].time) / 1000.0);
				FirstEntranceSample = i;
			}
			if(Entrances == 2){
				m->TimeToSecondEntrance = ((t - T[0].time) / 1000.0);
				SecondEntranceSample = i;
			}
		}
		if(ShockInThisSample){
			m->NumberOfShocks++;
			if(m->NumberOfShocks == 1){
				m->TimeToFirstShock = ((t - T[0].time) / 1000.0);
				FirstShockSample = i;
			}
			if(m->NumberOfShocks == 2){
				m->TimeToSecondShock = ((t - T[0].time) / 1000.0);
				SecondShockSample = i;
			}
		}
		T[i].event = EntranceState;
	}

	m->Entrances = Entrances;
	if(m->NumberOfShocks){
		PathLengthMeasures(T, 0, FirstShockSample, I,  &temp); // get the path to first shock
		m->PathToFirstShock = temp.PathLength;
	}else{
		m->PathToFirstShock = m->PathLength;
		m->PathToSecondShock = m->PathLength;
	}

	if(m->NumberOfShocks >= 2){
		PathLengthMeasures(T, 0, SecondShockSample, I,  &temp); // get the path to first shock
		m->PathToSecondShock = temp.PathLength;
	}else{
		m->PathToSecondShock = m->PathLength;
	}


	if(m->Entrances){
		PathLengthMeasures(T, 0, FirstEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToFirstEntrance = temp.PathLength;
		PathLengthMeasures(T, MaxAvoidedIntervalSample1, (MaxAvoidedIntervalSample2 - MaxAvoidedIntervalSample1), I,  &temp); 
		m->MaxPathAvoided = temp.PathLength;
	}else{
		m->PathToFirstEntrance = m->PathLength;
		m->MaxPathAvoided = m->PathLength;
		m->MaxTimeAvoided = (m->Duration * 1000.0);
	}

	if(m->Entrances >= 2){
		PathLengthMeasures(T, 0, SecondEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToSecondEntrance = temp.PathLength;
	}else{
		m->PathToSecondEntrance = m->PathLength;
	}

	if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
		m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
		MaxAvoidedIntervalSample1 = SampleOfLastEntrance;
		MaxAvoidedIntervalSample2 = i;
	}
	m->MaxTimeAvoided = (m->MaxTimeAvoided / 1000.0);

	return;
}

void	PA_StateBasedEntranceMeasures(TRACK *T, int StartSamp, int NumberOfSamps, INFO I, unsigned char **ReinforcedMap, MEASURES *m, int frame)  
{
	MEASURES	temp;
	int	TimeOfLastEntrance;
	int 	FirstEntranceSample = 0, FirstShockSample = 0;
	int 	SecondEntranceSample = 0, SecondShockSample = 0;
	int	i, x, y, t, f, Entrances, sample;
	int	PreviousState, SectorMask;
	int	SampleOfLastEntrance, MaxAvoidedIntervalSample1, MaxAvoidedIntervalSample2;
	double 	dx, dy, dt, distance, PixelsPerCM;

	// FGX  definition of shock state (stored in the TRACK.event variable)
	// OutsideSector = 0, EntranceLatency = 1, Shock = 2, InterShockLatency = 3
	if(I.FrameTypeOfData == ROOM_FRAME) 
		SectorMask = ROOM_SECTOR;
	else if(I.FrameTypeOfData == ARENA_FRAME) 
		SectorMask = ARENA_SECTOR;
	else if(I.FrameTypeOfData == CAMERA_FRAME) 
		SectorMask = ROOM_SECTOR;
	else	// assume Room Frame
		SectorMask = ROOM_SECTOR;

	Entrances = 0;
	TimeOfLastEntrance = T[0].time;
	SampleOfLastEntrance = 0;
	MaxAvoidedIntervalSample1 = MaxAvoidedIntervalSample2 = 0;
	PixelsPerCM = I.PixelsPerCM;
	
	m->NumberOfShocks = 0;
	m->PathToFirstEntrance = m->PathToSecondEntrance = m->PathLength;
	m->TimeToFirstEntrance = m->TimeToSecondEntrance = m->Duration;
	m->PathToFirstShock = m->PathToSecondShock = m->PathLength;
	m->TimeToFirstShock = m->TimeToSecondShock = m->Duration;
	m->MaxTimeAvoided = 0.0;
	m ->SpeedAtFirstEntrance = m->SpeedAtSecondEntrance = -1.0;

	PreviousState = PA_STATE_OutsideSector;
	for(i = 0; i < NumberOfSamps; i++){

		x = T[i].x;
		y = T[i].y;

		if((!x) && (!y)){	// skip if this is a bad sample
			continue;
		}
		t = T[i].time;
		f = i;

		if(T[i].event == PA_STATE_Shock){
			// if(((PreviousState == PA_STATE_OutsideSector) || (PreviousState == PA_STATE_EntranceLatency) || (PreviousState == PA_STATE_InterShockLatency)) && (ReinforcedMap[y][x] == TARGET)){	//check that this is a 'new' shock and the rat is in the target
			if((PreviousState != PA_STATE_BadSpot) && (PreviousState != PA_STATE_Shock) && (ReinforcedMap[y][x] == TARGET) && (T[i].Sector & SectorMask)){	//check that this is a 'new' shock and the rat is in the target
			// if((PreviousState != PA_STATE_BadSpot) && (PreviousState != PA_STATE_Shock) && (T[i].Sector & SectorMask)){	//check that this is a 'new' shock and the rat is in the target
				// note that if the rat entered or left before shock but returned before RefractoryLatency it got shocked
				m->NumberOfShocks ++;
				// DEBUG(i)
				if(m->NumberOfShocks == 1){
					m->TimeToFirstShock = ((t - T[0].time) / 1000.0);
					FirstShockSample = i;
					// calculate speed at time of the entrance
					// find the sample TIME_FOR_SPEED_ESTIMATE_MS ms before
					for(sample = i-1; sample >= 0; sample--){
						dt = (double)(t - T[sample].time);
						if(dt > TIME_FOR_SPEED_ESTIMATE_MS){
							dt = (double)((t - T[sample+1].time) / 1000.0);
							dx = (double)(x - T[sample+1].x);
							dy = (double)(y - T[sample+1].y);
							distance = hypot(dx,dy);
							m->SpeedAtFirstEntrance = distance / dt;
							m->SpeedAtFirstEntrance /= (double)(PixelsPerCM);
							break;
						}
					}
					if(sample == 0){
						dt = (double)((t - T[0].time) / 1000.0);
						dx = (double)(x - T[0].x);
						dy = (double)(y - T[0].y);
						distance = hypot(dx,dy);
						m->SpeedAtFirstEntrance = distance / dt;
						m->SpeedAtFirstEntrance /= (double)(PixelsPerCM);
					}
					
				}else if(m->NumberOfShocks == 2){
					m->TimeToSecondShock = ((t - T[0].time) / 1000.0);
					SecondShockSample = i;
					// calculate speed at time of the entrance
					// find the sample TIME_FOR_SPEED_ESTIMATE_MS ms before
					for(sample = i-1; sample >= 0; sample--){
						dt = (double)(t - T[sample].time);
						if(dt > TIME_FOR_SPEED_ESTIMATE_MS){
							dt = (double)((t - T[sample+1].time) / 1000.0);
							dx = (double)(x - T[sample+1].x);
							dy = (double)(y - T[sample+1].y);
							distance = hypot(dx,dy);
							m ->SpeedAtSecondEntrance = distance / dt;
							m->SpeedAtSecondEntrance /= (double)(PixelsPerCM);
							break;
						}
					}
					if(sample == 0){
						dt = (double)((t - T[0].time) / 1000.0);
						dx = (double)(x - T[0].x);
						dy = (double)(y - T[0].y);
						distance = hypot(dx,dy);
						m->SpeedAtSecondEntrance = distance / dt;
						m->SpeedAtSecondEntrance /= (double)(PixelsPerCM);
					}
				}
			}
			// if((PreviousState == PA_STATE_EntranceLatency) && (ReinforcedMap[y][x] == TARGET)){ // this is a new entrance
			// Tracker <= 2.2 bug allows Shock without prior EntranceLatency
			if((PreviousState < PA_STATE_Shock) && (ReinforcedMap[y][x] == TARGET) && (T[i].Sector & SectorMask)){ // this is a new entrance
				// note that if the rat entered or left before shock but returned before RefractoryLatency it got shocked
				// if((I.FileFormatVersion <= 2.1) && ((TimeOfLastEntrance < (t - I.Shock.MinISI)) || (TimeOfLastEntrance < (t - I.Shock.ResetInterval)))){
				// (I.FileFormat == TRACKER_FORMAT) || (I.FileFormat == ITRACK_FORMAT)) && (I.FileFormatVersion <= 2.2) because Tracker 2.1 Header indicated ITrack format
				if(((I.FileFormat == TRACKER_FORMAT) || (I.FileFormat == ITRACK_FORMAT)) && (I.FileFormatVersion < 2.201) && (TimeOfLastEntrance < (t - I.Shock.MinISI)) && (TimeOfLastEntrance < (t - I.Shock.ResetInterval))){
					// A bug in Tracker version < 2.2 is that InterShockLatency and OutsideRefractory states were not set correctly.
					// A separate bug caused some files to have Format iTrack instead of Tracker 
					Entrances++;
					if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
						m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
						MaxAvoidedIntervalSample1 = SampleOfLastEntrance;
						MaxAvoidedIntervalSample2 = i;
					}
					TimeOfLastEntrance = t;
					SampleOfLastEntrance = i;
					if(Entrances == 1){
						m->TimeToFirstEntrance = ((t - T[0].time) / 1000.0);
						FirstEntranceSample = i;
						// calculate speed at time of the entrance
						// find the sample TIME_FOR_SPEED_ESTIMATE_MS ms before
						for(sample = i-1; sample >= 0; sample--){
							dt = (double)(t - T[sample].time);
							if(dt > TIME_FOR_SPEED_ESTIMATE_MS){
								dt = (double)((t - T[sample+1].time) / 1000.0);
								dx = (double)(x - T[sample+1].x);
								dy = (double)(y - T[sample+1].y);
								distance = hypot(dx,dy);
								m->SpeedAtFirstEntrance = distance / dt;
								m->SpeedAtFirstEntrance /= (double)(PixelsPerCM);
								break;
							}
						}
						if(sample == 0){
							dt = (double)((t - T[0].time) / 1000.0);
							dx = (double)(x - T[0].x);
							dy = (double)(y - T[0].y);
							distance = hypot(dx,dy);
							m->SpeedAtFirstEntrance = distance / dt;
							m->SpeedAtFirstEntrance /= (double)(PixelsPerCM);
						}
					}else if(Entrances == 2){
						m->TimeToSecondEntrance = ((t - T[0].time) / 1000.0);
						SecondEntranceSample = i;
						// calculate speed at time of the entrance
						// find the sample TIME_FOR_SPEED_ESTIMATE_MS ms before
						for(sample = i-1; sample >= 0; sample--){
							dt = (double)(t - T[sample].time);
							if(dt > TIME_FOR_SPEED_ESTIMATE_MS){
								dt = (double)((t - T[sample+1].time) / 1000.0);
								dx = (double)(x - T[sample+1].x);
								dy = (double)(y - T[sample+1].y);
								distance = hypot(dx,dy);
								m->SpeedAtSecondEntrance = distance / dt;
								m->SpeedAtSecondEntrance /= (double)(PixelsPerCM);
								break;
							}
						}
						if(sample == 0){
							dt = (double)((t - T[0].time) / 1000.0);
							dx = (double)(x - T[0].x);
							dy = (double)(y - T[0].y);
							distance = hypot(dx,dy);
							m->SpeedAtSecondEntrance = distance / dt;
							m->SpeedAtSecondEntrance /= (double)(PixelsPerCM);
						}
					}
				} else if((I.FileFormat == TRACKER_FORMAT) && (I.FileFormatVersion > 2.201)){
					// Tracker version > 2.20 sets the InterShockLatency and OutsideRefractory states correctly.
					Entrances++;
					if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
						m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
						MaxAvoidedIntervalSample1 = SampleOfLastEntrance;
						MaxAvoidedIntervalSample2 = i;
					}
					TimeOfLastEntrance = t;
					SampleOfLastEntrance = i;
					if(Entrances == 1){
						m->TimeToFirstEntrance = ((t - T[0].time) / 1000.0);
						FirstEntranceSample = i;
						// calculate speed at time of the entrance
                                                // find the sample TIME_FOR_SPEED_ESTIMATE_MS ms before
                                                for(sample = i-1; sample >= 0; sample--){
                                                        dt = (double)(t - T[sample].time);
                                                        if(dt > TIME_FOR_SPEED_ESTIMATE_MS){
                                                                dt = (double)((t - T[sample+1].time) / 1000.0);
                                                                dx = (double)(x - T[sample+1].x);
                                                                dy = (double)(y - T[sample+1].y);
                                                                distance = hypot(dx,dy);
                                                                m->SpeedAtFirstEntrance = distance / dt;
								m->SpeedAtFirstEntrance /= (double)(PixelsPerCM);
                                                                break;
                                                        }
                                                }
                                                if(sample == 0){
                                                        dt = (double)((t - T[0].time) / 1000.0);
                                                        dx = (double)(x - T[0].x);
                                                        dy = (double)(y - T[0].y);
                                                        distance = hypot(dx,dy);
                                                        m->SpeedAtFirstEntrance = distance / dt;
							m->SpeedAtFirstEntrance /= (double)(PixelsPerCM);
                                                }
					}else if(Entrances == 2){
						m->TimeToSecondEntrance = ((t - T[0].time) / 1000.0);
						SecondEntranceSample = i;
						// calculate speed at time of the entrance
                                                // find the sample TIME_FOR_SPEED_ESTIMATE_MS ms before
                                                for(sample = i-1; sample >= 0; sample--){
                                                        dt = (double)(t - T[sample].time);
                                                        if(dt > TIME_FOR_SPEED_ESTIMATE_MS){
                                                                dt = (double)((t - T[sample+1].time) / 1000.0);
                                                                dx = (double)(x - T[sample+1].x);
                                                                dy = (double)(y - T[sample+1].y);
                                                                distance = hypot(dx,dy);
                                                                m->SpeedAtSecondEntrance = distance / dt;
								m->SpeedAtSecondEntrance /= (double)(PixelsPerCM);
                                                                break;
                                                        }
                                                }
                                                if(sample == 0){
                                                        dt = (double)((t - T[0].time) / 1000.0);
                                                        dx = (double)(x - T[0].x);
                                                        dy = (double)(y - T[0].y);
                                                        distance = hypot(dx,dy);
                                                        m->SpeedAtSecondEntrance = distance / dt;
							m->SpeedAtFirstEntrance /= (double)(PixelsPerCM);
						}
					}
				}
			}
		}
		PreviousState = T[i].event; 
	}
	m->Entrances = Entrances;

	if(m->NumberOfShocks){
		PathLengthMeasures(T, 0, FirstShockSample, I,  &temp); // get the path to first shock
		m->PathToFirstShock = temp.PathLength;
	}else{
		m->PathToFirstShock = m->PathLength;
	}
	if(m->NumberOfShocks >= 2){
		PathLengthMeasures(T, 0, SecondShockSample, I,  &temp); // get the path to first shock
		m->PathToSecondShock = temp.PathLength;
	}else{
		m->PathToSecondShock = m->PathLength;
	}
	if(m->Entrances){
		PathLengthMeasures(T, 0, FirstEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToFirstEntrance = temp.PathLength;
		PathLengthMeasures(T, MaxAvoidedIntervalSample1, (MaxAvoidedIntervalSample2 - MaxAvoidedIntervalSample1), I,  &temp); 
		m->MaxPathAvoided = temp.PathLength;
	}else{
		m->PathToFirstEntrance = m->PathLength;
		m->MaxPathAvoided = m->PathLength;
		m->MaxTimeAvoided = (m->Duration * 1000.0);
	}
	if(m->Entrances >= 2){
		PathLengthMeasures(T, 0, SecondEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToSecondEntrance = temp.PathLength;
	}else{
		m->PathToSecondEntrance = m->PathLength;
	}
	if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
		m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
		MaxAvoidedIntervalSample1 = SampleOfLastEntrance;
		MaxAvoidedIntervalSample2 = i;
	}
	m->MaxTimeAvoided = (m->MaxTimeAvoided / 1000.0);

	return;
}

void	PP_StateEntranceMeasures(TRACK *T, int StartSamp, int NumberOfSamps, INFO I, unsigned char **ReinforcedMap, MEASURES *m, int frame)  
{
	MEASURES	temp;
	int	TimeOfLastEntrance;
	int 	FirstEntranceSample = 0, FirstShockSample = 0;
	int 	SecondEntranceSample = 0, SecondShockSample = 0;
	int	i, x, y, t, f, Entrances;
	int	PreviousState;
	int	SampleOfLastEntrance, MaxNoRewardIntervalSample1, MaxNoRewardIntervalSample2;

	// FGX  definition of Place preference state (stored in the TRACK.State variable)
	// OutsideSector = 0, Entering = 1, Entrance/Feed = 2, Refractory = 3

	Entrances = 0;
	TimeOfLastEntrance = T[0].time;
	SampleOfLastEntrance = 0;
	MaxNoRewardIntervalSample1 = MaxNoRewardIntervalSample2 = 0;
	
	m->NumberOfShocks = 0;
	m->PathToFirstEntrance = m->PathToSecondEntrance = m->PathLength;
	m->TimeToFirstEntrance = m->TimeToSecondEntrance = m->Duration;
	m->PathToFirstShock = m->PathToSecondShock = m->PathLength;
	m->TimeToFirstShock = m->TimeToSecondShock = m->Duration;
	m->MaxTimeAvoided = 0.0;
	m->MaxPathAvoided = 0;

	PreviousState = PP_STATE_OutsideSector;
	for(i = 0; i < NumberOfSamps; i++){

		x = T[i].x;
		y = T[i].y;

		if(x && y){
			t = T[i].time;
			f = i;

			if(T[i].State == PP_STATE_Feed){
				if((PreviousState == PP_STATE_Entering) && (ReinforcedMap[y][x] == TARGET)){ // this is a new entrance
					Entrances++;
					if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
						m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
						MaxNoRewardIntervalSample1 = SampleOfLastEntrance;
						MaxNoRewardIntervalSample2 = i;
					}
					TimeOfLastEntrance = t;
					SampleOfLastEntrance = i;
					if(Entrances == 1){
						m->TimeToFirstEntrance = ((t - T[0].time) / 1000.0);
						FirstEntranceSample = i;
					}else if(Entrances == 2){
						m->TimeToSecondEntrance = ((t - T[0].time) / 1000.0);
						SecondEntranceSample = i;
					}
				}
			}
		}
		PreviousState = T[i].State; 
	}
	m->Entrances = Entrances;

	if(m->Entrances){
		PathLengthMeasures(T, 0, FirstEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToFirstEntrance = temp.PathLength;
		PathLengthMeasures(T, MaxNoRewardIntervalSample1, (MaxNoRewardIntervalSample2 - MaxNoRewardIntervalSample1), I,  &temp); 
		m->MaxPathAvoided = temp.PathLength;
	}else{
		m->PathToFirstEntrance = m->PathLength;
		m->MaxPathAvoided = m->PathLength;
		m->MaxTimeAvoided = (float)(m->Duration * 1000.0);
	}
	if(m->Entrances >= 2){
		PathLengthMeasures(T, 0, SecondEntranceSample, I,  &temp); // get the path to first entrance
		m->PathToSecondEntrance = temp.PathLength;
	}else{
		m->PathToSecondEntrance = m->PathLength;
	}
	if(m->MaxTimeAvoided < (float)(t - TimeOfLastEntrance)){
		m->MaxTimeAvoided = (float)(t - TimeOfLastEntrance);
		MaxNoRewardIntervalSample1 = SampleOfLastEntrance;
		MaxNoRewardIntervalSample2 = i;
	}
	m->MaxTimeAvoided = (m->MaxTimeAvoided / 1000.0);

	return;
}

void	MakeAvoidCircleMap(unsigned char ***map, INFO Info, CIRCLE Circle, int OverlappingTargets){
	int	i, x, y;
	double	atan2(), hypot();
	double	X, Y, TargetAng, Ang, Rad, CartesianX, CartesianY;
	static unsigned char	**AvoidMap;

	if(AvoidMap != NULL){
		for(i = 0; i < TRACKER_XY_RESOLUTION; i++)
			free(AvoidMap[i]);
		free(AvoidMap);
	}
	AvoidMap = (unsigned char **)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char *));
	if(AvoidMap == NULL){
		fprintf(MyStdOut,"Can't allocate AvoidMap\n");
		return;
	}
	for(i = 0; i < TRACKER_XY_RESOLUTION; i++){
		AvoidMap[i] = (unsigned char *)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char));
		if(AvoidMap[i] == NULL){
			fprintf(MyStdOut,"Can't allocate AvoidMap\n");
			return;
		}
	}
	*map = AvoidMap;
	
	if(Circle.Rad == 0.0){
		for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
			for (x = 0; x < TRACKER_XY_RESOLUTION; x++){
				AvoidMap[y][x] = 0;
			}
		}
		return;
	}

	// TARGET
	X = (double)Circle.X;
	Y = (double)Circle.Y;

	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){
			AvoidMap[y][x] = 0;
			// Compute Distance from center of target and decide if it is in the target
			if (Circle.Rad > hypot(x - X, y - Y)){
				AvoidMap[y][x] = TARGET_QUADRANT;
			}
		}
	}
	
	if(!OverlappingTargets)
		return;

	// prepare for other quadrants
	CartesianX = Circle.X - Info.Arena.CenterX;
	CartesianY = Info.Arena.CenterY - Circle.Y;
	if((CartesianX != 0.0) || (CartesianY != 0.0)){ // Ang is defined
		// Compute angle	atan2(y/x) returns 0 if x = 0 
		TargetAng = atan2(CartesianY, CartesianX);
					
		if(CartesianX == 0.0)
			TargetAng = ((CartesianY) > 0) ? (M_PI / 2.0) : (-1 * M_PI / 2.0);
		if(TargetAng < 0.0)			// convert -rads to +rads
			TargetAng += (2.0 * M_PI);
	}
	Rad = hypot(Circle.X - Info.Arena.CenterX, Circle.Y - Info.Arena.CenterY);

	// CCW quadrant
	Ang = TargetAng + CCW_QUADRANT_ANG;
 	if(Ang > (2.0 * M_PI))
 		Ang -= (2.0 * M_PI);

	CartesianX = cos(Ang) * Rad;
	CartesianY = sin(Ang) * Rad;
	X = CartesianX + Info.Arena.CenterX;	
	Y = Info.Arena.CenterY - CartesianY;	

	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){

			// Compute Distance from center of target and decide if it is in the target
			if (Circle.Rad > hypot(x - X, y - Y)){
				if(AvoidMap[y][x] == 0)	//in case the areas are overlapping
					AvoidMap[y][x] = CCW_QUADRANT;
				else
					AvoidMap[y][x] = -1;
			}
		}
	}
	// OPP quadrant
	Ang = TargetAng + OPP_QUADRANT_ANG;
 	if(Ang > (2.0 * M_PI))
	 	Ang -= (2.0 * M_PI);

	CartesianX = cos(Ang) * Rad;
	CartesianY = sin(Ang) * Rad;
	X = CartesianX + Info.Arena.CenterX;	
	Y = Info.Arena.CenterY - CartesianY;	

	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){

			// Compute Distance from center of target and decide if it is in the target
			if (Circle.Rad > hypot(x - X, y - Y)){
				if(AvoidMap[y][x] == 0){	//in case the areas are overlapping
					AvoidMap[y][x] = OPP_QUADRANT;
				}else
					AvoidMap[y][x] = -1;
			}
		}
	}
	// CW quadrant
	Ang = TargetAng + CW_QUADRANT_ANG;
 	if(Ang > (2.0 * M_PI))
 		Ang -= (2.0 * M_PI);

	CartesianX = cos(Ang) * Rad;
	CartesianY = sin(Ang) * Rad;
	X = CartesianX + Info.Arena.CenterX;	
	Y = Info.Arena.CenterY - CartesianY;	

	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){

			// Compute Distance from center of target and decide if it is in the target
			if (Circle.Rad > hypot(x - X, y - Y)){
				if(AvoidMap[y][x] == 0)	//in case the areas are overlapping
					AvoidMap[y][x] = CW_QUADRANT;
				else
					AvoidMap[y][x] = -1;
			}
		}
	}
	return;
}

void	MakeNullSectorMap(unsigned char ***map){
	int	i, x, y;
	static unsigned char	**NullMap;

	if(NullMap != NULL){
		for(i = 0; i < TRACKER_XY_RESOLUTION; i++)
			free(NullMap[i]);
		free(NullMap);
	}
	NullMap = (unsigned char **)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char *));
	if(NullMap == NULL){
		fprintf(MyStdOut,"Can't allocate NullMap\n");
		return;
	}
	for(i = 0; i < TRACKER_XY_RESOLUTION; i++){
		NullMap[i] = (unsigned char *)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char));
		if(NullMap[i] == NULL){
			fprintf(MyStdOut,"Can't allocate NullMap\n");
			return;
		}
	}
	*map = NullMap;
	
	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){
			NullMap[y][x] = 0;
		}
	}
	return;
}

void	MakeAvoidSectorMap(unsigned char ***map, INFO Info, SECTOR Sector){
	int	i, x, y;
	double	atan2(), hypot();
	double	CartesianX, CartesianY, Ang, Rad, TestAng;
	double	TransformAng, MinShockAng, MaxShockAng, MinShockRad, MaxShockRad;
	static unsigned char	**AvoidMap;

	if(AvoidMap != NULL){
		for(i = 0; i < TRACKER_XY_RESOLUTION; i++)
			free(AvoidMap[i]);
		free(AvoidMap);
	}
	AvoidMap = (unsigned char **)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char *));
	if(AvoidMap == NULL){
		fprintf(MyStdOut,"Can't allocate AvoidMap\n");
		return;
	}
	for(i = 0; i < TRACKER_XY_RESOLUTION; i++){
		AvoidMap[i] = (unsigned char *)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char));
		if(AvoidMap[i] == NULL){
			fprintf(MyStdOut,"Can't allocate AvoidMap\n");
			return;
		}
	}
	*map = AvoidMap;
	
	if(Sector.Width == 0.0){
		for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
			for (x = 0; x < TRACKER_XY_RESOLUTION; x++){
				AvoidMap[y][x] = 0;
			}
		}
		return;
	}

	MinShockRad = (double)(Sector.InRad);
	MaxShockRad = (double)(Sector.OutRad);

	// Transform angles so that the min is 0 and the max is the width
	MinShockAng = 0.0;
	MaxShockAng = (double)(Sector.Width / DEG_PER_RAD);

	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
			for (x = 0; x < TRACKER_XY_RESOLUTION; x++){
				AvoidMap[y][x] = 0;
				CartesianX = (double)(x - Info.Arena.CenterX);
				CartesianY = (double)(Info.Arena.CenterY - y);

				// Compute radius
				Rad = hypot(CartesianX, CartesianY);

				// decide if point is in target area
				if((Rad >= MinShockRad) && (Rad <= MaxShockRad)){

					// Compute angle	atan2(y/x) returns 0 if x = 0 
					Ang = atan2(CartesianY, CartesianX);
					if((CartesianX == 0.0) && (CartesianY == 0.0))
						// Ang is undefined
						continue;
					
					if(CartesianX == 0.0)
						Ang = ((Info.Arena.CenterY - y) > 0) ? (M_PI / 2.0) : (-1 * M_PI / 2.0);
					if(Ang < 0.0)			// convert -rads to +rads
						Ang += (2.0 * M_PI);
						
					// transform ang into the coordinate system with MinShockAng = 0
					TransformAng = (double)(((Sector.Width / 2.0) - Sector.Ang) / DEG_PER_RAD);
					TestAng = Ang + TransformAng;
					if(TestAng < 0.0)			// convert -rads to +rads
						TestAng += (2.0 * M_PI);
					if(TestAng >= (2.0 * M_PI))
						TestAng -= (2.0 * M_PI);

					if(TestAng <= MaxShockAng){
						// point is in target area
						AvoidMap[y][x] = TARGET_QUADRANT;
						continue;
					}

					// transform ang into the coordinate system with MinShockAng = 0
					TransformAng = (double)((Sector.Width / 2.0 / DEG_PER_RAD) - ((Sector.Ang / DEG_PER_RAD) + CCW_QUADRANT_ANG));
					TestAng = Ang + TransformAng;
					if(TestAng < 0.0)			// convert -rads to +rads
						TestAng += (2.0 * M_PI);
					if(TestAng >= (2.0 * M_PI))
						TestAng -= (2.0 * M_PI);

					if(TestAng <= MaxShockAng){
						AvoidMap[y][x] = CCW_QUADRANT;
						continue;
					}

					// transform ang into the coordinate system with MinShockAng = 0
					TransformAng = (double)((Sector.Width / 2.0 / DEG_PER_RAD) - ((Sector.Ang / DEG_PER_RAD) + OPP_QUADRANT_ANG));
					TestAng = Ang + TransformAng;
					if(TestAng < 0.0)			// convert -rads to +rads
						TestAng += (2.0 * M_PI);
					if(TestAng >= (2.0 * M_PI))
						TestAng -= (2.0 * M_PI);

					if(TestAng <= MaxShockAng){
						AvoidMap[y][x] = OPP_QUADRANT;
						continue;
					}

					// transform ang into the coordinate system with MinShockAng = 0
					TransformAng = (double)((Sector.Width / 2.0 / DEG_PER_RAD) - ((Sector.Ang / DEG_PER_RAD) + CW_QUADRANT_ANG));
					TestAng = Ang + TransformAng;
					if(TestAng < 0.0)			// convert -rads to +rads
						TestAng += (2.0 * M_PI);
					if(TestAng >= (2.0 * M_PI))
						TestAng -= (2.0 * M_PI);

					if(TestAng <= MaxShockAng){
						AvoidMap[y][x] = CW_QUADRANT;
						continue;
					}
				}
			
			}
	}
	return;
}

void	MakePreferenceMap(unsigned char ***map, INFO Info, CIRCLE Circle, int OverlappingTargets){
	int	i, x, y;
	double	atan2(), hypot();
	double	X, Y, TargetAng, Ang, Rad, CartesianX, CartesianY;
	static unsigned char	**PreferenceMap;

 	if(PreferenceMap != NULL){
		for(i = 0; i < TRACKER_XY_RESOLUTION; i++)
			free(PreferenceMap[i]);
		free(PreferenceMap);
	}
	PreferenceMap = (unsigned char **)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char *));
	if(PreferenceMap == NULL){
		fprintf(MyStdOut,"Can't allocate PreferenceMap\n");
		return;
	}
	for(i = 0; i < TRACKER_XY_RESOLUTION; i++){
		PreferenceMap[i] = (unsigned char *)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char));
		if(PreferenceMap[i] == NULL){
			fprintf(MyStdOut,"Can't allocate PreferenceMap\n");
			return;
		}
	}
	*map = PreferenceMap;
	if(Circle.Rad == 0.0){
		for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
			for (x = 0; x < TRACKER_XY_RESOLUTION; x++){
				PreferenceMap[y][x] = 0;
			}
		}
		return;
	}

	// TARGET
	X = (double)Circle.X;
	Y = (double)Circle.Y;

	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){
			PreferenceMap[y][x] = 0;
			// Compute Distance from center of target and decide if it is in the target
			if (Circle.Rad > hypot(x - X, y - Y)){
				PreferenceMap[y][x] = TARGET_QUADRANT;
			}
		}
	}
	
	if(!OverlappingTargets)
		return;

	// prepare for other quadrants
	CartesianX = Circle.X - Info.Arena.CenterX;
	CartesianY = Info.Arena.CenterY - Circle.Y;
	if((CartesianX != 0.0) || (CartesianY != 0.0)){ // Ang is defined
		// Compute angle	atan2(y/x) returns 0 if x = 0 
		TargetAng = atan2(CartesianY, CartesianX);
					
		if(CartesianX == 0.0)
			TargetAng = ((CartesianY) > 0) ? (M_PI / 2.0) : (-1 * M_PI / 2.0);
		if(TargetAng < 0.0)			// convert -rads to +rads
			TargetAng += (2.0 * M_PI);
	}
	Rad = hypot(Circle.X - Info.Arena.CenterX, Circle.Y - Info.Arena.CenterY);

	// CCW quadrant
	Ang = TargetAng + CCW_QUADRANT_ANG;
 	if(Ang > (2.0 * M_PI))
 		Ang -= (2.0 * M_PI);

	CartesianX = cos(Ang) * Rad;
	CartesianY = sin(Ang) * Rad;
	X = CartesianX + Info.Arena.CenterX;	
	Y = Info.Arena.CenterY - CartesianY;	

	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){

			// Compute Distance from center of target and decide if it is in the target
			if (Circle.Rad > hypot(x - X, y - Y)){
				if(PreferenceMap[y][x] == 0)	//in case the areas are overlapping
					PreferenceMap[y][x] = CCW_QUADRANT;
				else
					PreferenceMap[y][x] = -1;
			}
		}
	}
	// OPP quadrant
	Ang = TargetAng + OPP_QUADRANT_ANG;
 	if(Ang > (2.0 * M_PI))
	 	Ang -= (2.0 * M_PI);

	CartesianX = cos(Ang) * Rad;
	CartesianY = sin(Ang) * Rad;
	X = CartesianX + Info.Arena.CenterX;	
	Y = Info.Arena.CenterY - CartesianY;	

	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){

			// Compute Distance from center of target and decide if it is in the target
			if (Circle.Rad > hypot(x - X, y - Y)){
				if(PreferenceMap[y][x] == 0){	//in case the areas are overlapping
					PreferenceMap[y][x] = OPP_QUADRANT;
				}else
					PreferenceMap[y][x] = -1;
			}
		}
	}
	// CW quadrant
	Ang = TargetAng + CW_QUADRANT_ANG;
 	if(Ang > (2.0 * M_PI))
 		Ang -= (2.0 * M_PI);

	CartesianX = cos(Ang) * Rad;
	CartesianY = sin(Ang) * Rad;
	X = CartesianX + Info.Arena.CenterX;	
	Y = Info.Arena.CenterY - CartesianY;	

	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){

			// Compute Distance from center of target and decide if it is in the target
			if (Circle.Rad > hypot(x - X, y - Y)){
				if(PreferenceMap[y][x] == 0)	//in case the areas are overlapping
					PreferenceMap[y][x] = CW_QUADRANT;
				else
					PreferenceMap[y][x] = -1;
			}
		}
	}
	return;
}

void	CalculateTimePerArea(TRACK *T, int StartSamp, int NumberOfSamps, unsigned char **ReinforcedMap, MEASURES *m)  
{
	int	i, x, y; 
	double	Total, dt, Other;

	dt = Total = Other = 0.0;
	m->TimeEntranceTARG = 0.0;
	m->TimeEntranceCW = 0.0;
	m->TimeEntranceCCW = 0.0;
	m->TimeEntranceOPP = 0.0;
	m->pTimeEntranceTARG = 0.0;
	m->pTimeEntranceCW = 0.0;
	m->pTimeEntranceCCW = 0.0;
	m->pTimeEntranceOPP = 0.0;

/*
        for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
                for (x = 0; x < TRACKER_XY_RESOLUTION; x++){
                        printf("%d ", ReinforcedMap[y][x]);
		}
		printf("\n");
	}
*/

	for(i = StartSamp + 1; i < NumberOfSamps; i++){
		if(T[i].x && T[i].y){
			x = T[i].x;
			y = T[i].y;
			// dt = (i) ? (double)(T[i].time - T[i-1].time)/1000.0: (double)(T[i].time/1000.0);
			dt = (double)(T[i].time - T[i-1].time)/1000.0;

			switch(ReinforcedMap[y][x]) {
			case TARGET:	
				(m->TimeEntranceTARG) += dt;
				Total += dt;
				break;
			case CLOCKWISE:	
				(m->TimeEntranceCW) += dt;
				Total += dt;
				break;
			case COUNTER_CLOCKWISE:	
				(m->TimeEntranceCCW) += dt;
				Total += dt;
				break;
			case OPPOSITE:	
				(m->TimeEntranceOPP) += dt;
				Total += dt;
				break;
			default:
				Other += dt;
				break;
			}
		}
	}
	if(Total > 0.0){
		m->pTimeEntranceTARG = m->TimeEntranceTARG / Total;
		m->pTimeEntranceCW = m->TimeEntranceCW / Total;
		m->pTimeEntranceCCW = m->TimeEntranceCCW / Total;
		m->pTimeEntranceOPP = m->TimeEntranceOPP / Total;
	}

	return;
}


void	MakeQuadrantMap(unsigned char ***map, INFO Info, double TargetAng){
	int	i, x, y;
	double	atan2(), hypot();
	double	X, Y, Ang, Radius, CartesianX, CartesianY;
	double	TargAngMax, TargAngMin, CCWAngMin, CCWAngMax, OPPAngMin, OPPAngMax, CWAngMin, CWAngMax;
	static unsigned char	**QuadrantMap;

	if(QuadrantMap != NULL){
		for(i = 0; i < TRACKER_XY_RESOLUTION; i++)
			free(QuadrantMap[i]);
		free(QuadrantMap);
	}
	QuadrantMap = (unsigned char **)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char *));
	if(QuadrantMap == NULL){
		fprintf(MyStdOut,"Can't allocate QuadrantMap\n");
		return;
	}

	for(i = 0; i < TRACKER_XY_RESOLUTION; i++){
		QuadrantMap[i] = (unsigned char *)calloc(TRACKER_XY_RESOLUTION, sizeof(unsigned char));
		if(QuadrantMap[i] == NULL){
			fprintf(MyStdOut,"Can't allocate QuadrantMap\n");
			return;
		}
	}

	*map = QuadrantMap;

	TargAngMin = TargetAng - (M_PI/4.0);
	if(TargAngMin < 0.0)			// convert -rads to +rads
		TargAngMin += (2.0 * M_PI);
	TargAngMax = TargAngMin + (M_PI/2.0);
	if(TargAngMax > (2.0 * M_PI))	// convert > 360 to < 360
		TargAngMax -= (2.0 * M_PI);
	if(TargAngMin > TargAngMax)
		TargAngMax += (2.0 * M_PI);

	CCWAngMin = TargAngMin + (M_PI/2.0);
	if(CCWAngMin > (2.0 * M_PI))	// convert > 360 to < 360
		CCWAngMin -= (2.0 * M_PI);
	CCWAngMax = CCWAngMin + (M_PI/2.0);
	if(CCWAngMax > (2.0 * M_PI))	// convert > 360 to < 360
		CCWAngMax -= (2.0 * M_PI);
	if(CCWAngMin > CCWAngMax)
		CCWAngMax += (2.0 * M_PI);

	OPPAngMin = TargAngMin + M_PI;
	if(OPPAngMin > (2.0 * M_PI))	// convert > 360 to < 360
		OPPAngMin -= (2.0 * M_PI);
	OPPAngMax = OPPAngMin + (M_PI/2.0);
	if(OPPAngMax > (2.0 * M_PI))	// convert > 360 to < 360
		OPPAngMax -= (2.0 * M_PI);
	if(OPPAngMin > OPPAngMax)
		OPPAngMax += (2.0 * M_PI);

	CWAngMin = TargAngMin + (M_PI * 1.5);
	if(CWAngMin > (2.0 * M_PI))	// convert > 360 to < 360
		CWAngMin -= (2.0 * M_PI);
	CWAngMax = CWAngMin + (M_PI/2.0);
	if(CWAngMax > (2.0 * M_PI))	// convert > 360 to < 360
		CWAngMax -= (2.0 * M_PI);
	if(CWAngMin > CWAngMax)
		CWAngMax += (2.0 * M_PI);

	Radius = (Info.Arena.CenterX + Info.Arena.CenterY)/ 2.0 + 1.0; // add the one to account for half pixels
	for (y = 0; y < TRACKER_XY_RESOLUTION; y++){
		Y = (double)y;
		for (x = 0; x < TRACKER_XY_RESOLUTION; x++){
			X = (double)x;
			CartesianX = X - Info.Arena.CenterX;
			CartesianY = Info.Arena.CenterY - Y;
			if(hypot(CartesianY, CartesianX) > Radius){	// set point outside the arena to 0
				QuadrantMap[y][x] = 0;
	//			printf("%d ",QuadrantMap[y][x]);
				continue;
			}
				
			if((CartesianX == 0.0) && (CartesianY == 0.0)){ // Ang is undefined
				QuadrantMap[y][x] = 0;
	//			printf("%d ",QuadrantMap[y][x]);
				continue;
			}
			// Compute angle	atan2(y/x) returns 0 if x = 0 
			Ang = atan2(CartesianY, CartesianX);
			if(CartesianX == 0.0)
				Ang = ((CartesianY) > 0) ? (M_PI / 2.0) : (-1 * M_PI / 2.0);
			if(Ang < 0.0)			// convert -rads to +rads
				Ang += (2.0 * M_PI);

			if((Ang >= TargAngMin) && (Ang < TargAngMax))
				QuadrantMap[y][x] = TARGET_QUADRANT;
			else if((Ang >= CCWAngMin) && (Ang < CCWAngMax))
				QuadrantMap[y][x] = CCW_QUADRANT;
			else if((Ang >= OPPAngMin) && (Ang < OPPAngMax))
				QuadrantMap[y][x] = OPP_QUADRANT;
			else if((Ang >= CWAngMin) && (Ang < CWAngMax))
				QuadrantMap[y][x] = CW_QUADRANT;
			else if((Ang + (M_PI * 2.0)  >= TargAngMin) && (Ang + (M_PI * 2.0) < TargAngMax))
				QuadrantMap[y][x] = TARGET_QUADRANT;
			else if((Ang + (M_PI * 2.0)  >= CCWAngMin) && (Ang + (M_PI * 2.0)  < CCWAngMax))
				QuadrantMap[y][x] = CCW_QUADRANT;
			else if((Ang + (M_PI * 2.0)  >= OPPAngMin) && (Ang + (M_PI * 2.0)  < OPPAngMax))
				QuadrantMap[y][x] = OPP_QUADRANT;
			else if((Ang + (M_PI * 2.0)  >= CWAngMin) && (Ang + (M_PI * 2.0)  < CWAngMax))
				QuadrantMap[y][x] = CW_QUADRANT;

	//		printf("%d ",QuadrantMap[y][x]);
		}
	//	printf("\n");
	}
	return;
}

void	CalculateDistanceFromShockZone(FILE *dfp, TRACK *T, int StartSamp, int NumberOfSamps, INFO Info, unsigned char **ReinforcedMap)  
{
	int	i, fc; 
	double	dAng1, dAng2, RatAng, ShockMinAng, ShockMaxAng, RatRad, ShockInRad;
	double	ShockMinInX, ShockMinInY, ShockMaxInX, ShockMaxInY, ShockAng, ShockInX, ShockInY;
	double	temp, StdAng, x, y, X, Y, ArenaRad, ArenaCenterX, ArenaCenterY, InShockZone, Dist, DistIn, DistInArc, DistInPoint, DistLateral;
	
	fprintf(dfp,"%%%%BEGIN_HEADER\n");
	fprintf(dfp,"\t%%BEGIN RECORD_FORMAT\n");
	fprintf(dfp,"\t%%Sample.0 (FrameCount LineNumber Xcoord Ycoord DistanceFromShockZoneInPixels DistanceFromeShockZoneInCm\n");
	fprintf(dfp,"\t\t//LineNumber refers to the line in the .dat file and not to the frame or field number.\n");
	fprintf(dfp,"\t\t//When the rat is not detected DistanceFromShockZoneInPixels and DistanceFromeShockZoneInCm are set to 1000.0.\n");
	fprintf(dfp,"\t\t//When the rat is inside the Shock Zone the Distance values are negative.\n");
	fprintf(dfp,"\t%%END RECORD_FORMAT\n");
	fprintf(dfp,"%%%%END_HEADER\n");
        ArenaCenterX = Info.Arena.CenterX; // in pixels
        ArenaCenterY = Info.Arena.CenterY; // in pixels
	ArenaRad = (ArenaCenterX + ArenaCenterY)/2.0;

	// define the shock zone. assume the Outer edge of the zone is at the arena border
	// Standardize the shock zone so it is centered at 0 deg.
	StdAng = -1.0 * Info.Target[0].Sector.Ang;
	if(StdAng < -180.0)
		StdAng += 360.0;
	ShockMinAng =  0.0 - (Info.Target[0].Sector.Width)/2.0;
	ShockMaxAng = ShockMinAng + Info.Target[0].Sector.Width;
	// convert to radians
	StdAng *= RAD_PER_DEG;
	ShockMinAng *= RAD_PER_DEG;
	ShockMaxAng *= RAD_PER_DEG;
        ShockInRad = Info.Target[0].Sector.InRad; // radius in pixels
	
	// coordinates of the intersection of the shock borders with the inner radius of the shock zone
	ShockMinInX = cos(ShockMinAng) * ShockInRad;
	ShockMinInY = sin(ShockMinAng) * ShockInRad;
	ShockMaxInX = cos(ShockMaxAng) * ShockInRad;
	ShockMaxInY = sin(ShockMaxAng) * ShockInRad;

	for(i = StartSamp; i < NumberOfSamps; i++){
		if(!T[i].x && !T[i].y){	// 0,0 is an undetected sample
			fprintf(dfp,"%d\t%d\t%d\t%d\t%0.3lf\t%0.1lf\n", T[i].framecount, i+1, T[i].x, T[i].y, 1000.0, 1000.0);
			continue;
		}

		x = (double)T[i].x;
		y = (double)T[i].y;
		fc = T[i].framecount;
		
		if(ReinforcedMap[(int)y][(int)x] == TARGET) // rat is in shock zone
			InShockZone = -1.0;
		else
			InShockZone = 1.0;

		// convert from tracker to Cartesian coordinates
                X = x - ArenaCenterX;
                Y = ArenaCenterY - y;

		// RAT IN CENTER -- closest border is the inner radius of the shock zone
                if((X == 0.0) && (Y == 0.0)){
			Dist = ShockInRad; // distance in pixels
			fprintf(dfp,"%d\t%d\t%d\t%d\t%0.3lf\t%0.1lf\n", fc, i+1, T[i].x, T[i].y, Dist, Dist / Info.PixelsPerCM);
		}else{ // RAT NOT IN CENTER -- the closest point is either on the inner radius or on the lateral borders
			// convert to polar coordinates
			RatRad = hypot(X, Y);
			RatAng = atan2(Y, X);
			RatAng += StdAng; 	// convert to standardized angular coordinate
			X = cos(RatAng) * RatRad;
			Y = sin(RatAng) * RatRad;

               		if(RatAng < 0.0) // convert -rads to +rads
				RatAng += (2.0 * M_PI);
			if(RatAng > (2.0 * M_PI))
				RatAng -= (2.0 * M_PI);

			// Calculate the two angular distances
			dAng1 = (RatAng - ShockMinAng);
			dAng2 = (RatAng - ShockMaxAng);
			if(dAng1 > M_PI) dAng1 -= (2.0 * M_PI);
			if(dAng2 > M_PI) dAng2 -= (2.0 * M_PI);

			// choose the angular border that is closer
			if(fabs(dAng1) < fabs(dAng2)){
				ShockAng = ShockMinAng;
				ShockInX = ShockMinInX;
				ShockInY = ShockMinInY;
			}else{
				ShockAng = ShockMaxAng;
				ShockInX = ShockMaxInX;
				ShockInY = ShockMaxInY;
			}

			// DISTANCE ALONG RADIUS TO THE INNER BORDER
			if((RatAng > ShockMinAng) && (RatAng < ShockMaxAng)){
				x = cos(RatAng) * ShockInRad;
				y = sin(RatAng) * ShockInRad;
				DistInArc = hypot(Y-y, X-x);
			}else{
				temp = RatAng + M_PI;
				if(temp > (2.0 * M_PI))
					temp -= (2.0 * M_PI);
				if((temp > ShockMinAng) && (temp < ShockMaxAng)){
					x = cos(temp) * ShockInRad;
					y = sin(temp) * ShockInRad;
					DistInArc = hypot(Y-y, X-x);
				}else{
					DistInArc = 5000.0;
				}
			}

			// DISTANCE TO THE INTERSECTION OF THE SHOCK ANGLE WITH THE INNER RADIUS
			DistInPoint = hypot(Y-ShockInY, X-ShockInX);
	
			// choose the shorter distance to the inner border
			Dist = DistIn = (DistInPoint < DistInArc) ? DistInPoint: DistInArc;

			// DISTANCE TO THE LATERAL BORDER
			x = cos(ShockAng) * RatRad;
			y = sin(ShockAng) * RatRad;
			DistLateral = hypot(Y-y, X-x);

			// choose the shorter distance of the one to the lateral or inner borders
			Dist = (DistIn < DistLateral) ? DistIn: DistLateral;

			// If rat is in shock zone make the distance negative
			Dist *= InShockZone;

			fprintf(dfp,"%d\t%d\t%d\t%d\t%0.3lf\t%0.1lf\n", fc, i+1, T[i].x, T[i].y, Dist, Dist / Info.PixelsPerCM);
		}
	}
	return;
}
