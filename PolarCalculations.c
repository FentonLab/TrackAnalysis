#include	"Track.h"

int CalculatePolarTrack(TRACK *T, int StartSamp, int NumberOfSamps, INFO Info){
	double CartesianX, CartesianY, rad, ArenaRadiusInPixels;
	int	i, n;

	ArenaRadiusInPixels = (MAXIMUM_COORDINATE / 2);
	
	for(i = 0, n = 0; i < NumberOfSamps; i++){
		if(T[i].x && T[i].y){
			CartesianX = (double)((float)T[i].x - Info.Arena.CenterX);
			CartesianY = (double)(Info.Arena.CenterY - (float)T[i].y);
			if((CartesianX != 0.0) && (CartesianY != 0.0)){
				// ang and rad are defined
				// Compute radius
				rad = hypot(CartesianX, CartesianY);
				// if(rad <= (double)(Info.Arena.Radius * Info.PixelsPerCM * 100.0)){
				if(rad <= ArenaRadiusInPixels){
					T[i].rad = rad;
				}else{
					T[i].rad = 0.0;
				}


				// Compute angle	atan2(y/x) returns 0 if x = 0 
				T[i].ang = atan2(CartesianY, CartesianX);
				if(CartesianX == 0.0)
					T[i].ang = ((Info.Arena.CenterY - T[i].y) > 0) ? (M_PI / 2.0) : (-1 * M_PI / 2.0);
				if(T[i].ang < 0.0)			// convert -rads to +rads
					T[i].ang += (2.0 * M_PI);
				T[i].ang *= DEG_PER_RAD; // convert radians to degrees
				n++;
			}else{ //ang and rad undefined 
				T[i].ang = 0.0;
				T[i].rad = 0.0;
			}
		}else{
			//track and therefore ang and rad undefined 
			T[i].ang = 0.0;
			T[i].rad = 0.0;
		}
	}
	return(n);
}

void  CalculatePolarDistribution(TRACK *T, int StartSamp, int NumberOfSamps, POLAR *P, float ReferenceAngDeg){
// calculates the distribution relative to a reference angle so measures can be standardized e.g. to the to-be-avoided area 
	int	i, n, b;
	double	ang;
	double	angtobin;

	P->BinSzDeg = ANGULAR_BIN_SIZE_IN_DEGREES;
	P->NBins = (int)(360 / P->BinSzDeg);
	for(i = 0; i < P->NBins; i++)
		P->Bin[i] = 0;

	for(i = 0, n = 0; i < NumberOfSamps; i++){
		if((T[i].ang != 0.0) && (T[i].rad != 0.0)){ //if good point
			ang = T[i].ang - (double)ReferenceAngDeg;
			if(ang < 0.0)
				ang += 360.0;

			angtobin = ang + (P->BinSzDeg / 2.0); // bins are centered on bin steps
			if(angtobin > 360.0)
				angtobin -= 360.0;
			b = (int)(angtobin / P->BinSzDeg); // bins are centered on bin steps
			P->Bin[b]++;
			n++;
		}
	}

	P->Min = P->MinNonZero = 1.0;
	P->Max = 0.0;

	for(i = 0; i < P->NBins; i++){
		P->Bin[i] /= n;

		if(P->Min > P->Bin[i]){
			P->Min = P->Bin[i];
			P->MinBin = i;
		}
		if(P->Max < P->Bin[i]){
			P->Max = P->Bin[i];
			P->MaxBin = i;
		}
		if(P->Bin[i] != 0.0){
			if(P->MinNonZero > P->Bin[i]){
				P->MinNonZero = P->Bin[i];
				P->MinNonZeroBin = i;
			}
		}
	}

	CharacterizeDistribution(T, StartSamp, NumberOfSamps,  ANGLE, &(P->Avg), &(P->SD), &(P->Skewness), &(P->Kurtosis));
	GetMinimumPolarLimits(P, (float) 0.5);
	GetMaximumPolarLimits(P, (float) 0.5);

	return;
}

void CalculateAnnularDistribution(TRACK *T, int StartSamp, int NumberOfSamps, INFO Info, ANNULAR *A){
	int	 i, n, bin;
	double CurrentMaxRad, PreviousMaxRad, Constant;

	A->NBins = N_ANNULAR_BINS;
	A->BinSzCM = (double)(Info.Arena.Radius / (double)A->NBins);
	A->BinSzPix = A->BinSzCM * Info.PixelsPerCM;


	for(i = 0; i < A->NBins; i++)
		A->Bin[i] = 0;
	
	// calculate limits of equal area bins in cm
	PreviousMaxRad = (double)(Info.Arena.Radius);
	Constant = (PreviousMaxRad * PreviousMaxRad / A->NBins);
	for(bin = A->NBins -1; bin >0; bin--){
		CurrentMaxRad = sqrt(PreviousMaxRad * PreviousMaxRad - Constant);
		if(bin == A->NBins - 1)
			A->BinAvgCM[bin] = (Info.Arena.Radius + CurrentMaxRad) / 2.0;
		else
			A->BinAvgCM[bin] = (PreviousMaxRad + CurrentMaxRad) / 2.0;	
		PreviousMaxRad = CurrentMaxRad;
	}
	A->BinAvgCM[0] = CurrentMaxRad / 2.0;
	
//	for(i = 0; i < A->NBins; i++)
//		printf("XX %d %lf\n", i, A->BinAvgCM[i]);
//	getchar();

	A->MaxRad = 0.0;
	A->MinRad = (double)(Info.Arena.Radius * Info.PixelsPerCM);

	for(i = 0, n = 0; i < NumberOfSamps; i++){
		if((T[i].ang != 0.0) && (T[i].rad != 0.0)){ //if good point
			// make bins equal in area
			PreviousMaxRad = (double)(Info.Arena.Radius * Info.PixelsPerCM);
			Constant = (PreviousMaxRad * PreviousMaxRad / A->NBins);
			for(bin = A->NBins -1; bin >0; bin--){
				CurrentMaxRad = sqrt(PreviousMaxRad * PreviousMaxRad - Constant);
				if(T[i].rad >= CurrentMaxRad)
					break;

				// printf("\n%d %lf %lf %lf %lf %lf", bin, PreviousMaxRad, T[i].rad, CurrentMaxRad, (M_PI * PreviousMaxRad * PreviousMaxRad) - (M_PI * CurrentMaxRad * CurrentMaxRad), ((M_PI * PreviousMaxRad * PreviousMaxRad) - (M_PI * CurrentMaxRad * CurrentMaxRad)) / (M_PI * 128.0 * 128.0));
				PreviousMaxRad = CurrentMaxRad;
			}

//			equal width bins
//			bin = (int)(T[i].rad / (double)A->BinSzPix);

			if(bin < A->NBins){ // this should never fail but what the hell
				A->Bin[bin]++;
				n++;
			}
			// get the min and max Rad values
			if(A->MinRad > T[i].rad){
				A->MinRad = T[i].rad;
			}
			if(A->MaxRad < T[i].rad){
				A->MaxRad = T[i].rad;
			}
				
		}
	}

	A->Min = A->MinNonZero = 1.0;
	A->Max = 0.0;

	for(i = 0; i < A->NBins; i++){
		A->Bin[i] /= (double)n;

		if(A->Min > A->Bin[i]){
			A->Min = A->Bin[i];
			A->MinBin = A->BinAvgCM[i];
		}
		if(A->Max < A->Bin[i]){
			A->Max = A->Bin[i];
			A->MaxBin = A->BinAvgCM[i];

		}
		if(A->Bin[i] != 0.0){
			if(A->MinNonZero > A->Bin[i]){
				A->MinNonZero = A->Bin[i];
				A->MinNonZeroBin = A->BinAvgCM[i];

			}
		}
	}

	CharacterizeDistribution(T, StartSamp, NumberOfSamps, RADIUS, &(A->Avg), &(A->SD), &(A->Skewness), &(A->Kurtosis));
	A->Avg /= Info.PixelsPerCM;

	return;
}			

void CharacterizeDistribution(TRACK *T, int StartSamp, int NumberOfSamps, int  Measure, double *Avg, double *SD, double *Skewness, double *Kurtosis){
	int i, n;
	double X, dX, Sum, XX, SS, SSS, SSSS;

	n = 0;
	Sum = XX = dX =  0.0;
	for(i = StartSamp; i < NumberOfSamps; i++){
		switch (Measure){
			case ANGLE:
				if(T[i].ang == 0.0 && (T[i].rad == 0.0 ))
					continue;
				X = T[i].ang;
				break;
			case RADIUS: 
				if(T[i].ang == 0.0 && (T[i].rad == 0.0))
					continue;
				X = T[i].rad;
				break;
			default:
				break;
		}
		Sum += X;
		XX += X * X;
		n++;
	}

	*Avg  = Sum / (double)n;

	n = 0;
	SS = SSS = SSSS = 0.0;
	for(i = StartSamp; i < NumberOfSamps; i++){
		switch (Measure){
			case ANGLE:
				if(T[i].ang == 0.0 && (T[i].rad == 0.0 ))
					continue;
				X = T[i].ang;
				break;
			case RADIUS: 
				if(T[i].ang == 0.0 && (T[i].rad == 0.0))
					continue;
				X = T[i].rad;
				break;
			default:
				break;
		}
	
		dX = (*Avg - X);
		if(Measure == ANGLE){
			// necessary to preserve the sign for skewness
			if(dX > 180.0)
				dX -= 360.0;
			if(dX < -180.0)
				dX += 360.0;
		}
		SS += (dX * dX);
		SSS += (dX * dX * dX);
		SSSS += (dX * dX * dX * dX);
		n++;
	}


	*SD = sqrt(SS / (double)(n-1));

	*Skewness = SSS / (double)((n-1) * (*SD) * (*SD) * (*SD));
	*Kurtosis = SSSS / (double)((n-1) * (*SD) * (*SD) * (*SD) * (*SD));
	return;
}

void	GetMinimumPolarLimits(POLAR *P, float Limit){
	int	LoBin, HiBin, LargestBin;
	double Sum;

	Sum = P->Bin[P->MinBin];
	LargestBin = P->NBins -1;
	LoBin = P->MinBin - 1;
	if(LoBin < 0)
		LoBin = LargestBin;
	HiBin = P->MinBin + 1;
	if(HiBin > LargestBin)
		HiBin = 0;

	while(Sum < Limit){
		if(P->Bin[LoBin] < P->Bin[HiBin]){
			Sum += P->Bin[LoBin];
			if(LoBin > 0)
				LoBin--;
			else
				LoBin = LargestBin;		 
		}else{
			Sum += P->Bin[HiBin];
			if(HiBin < LargestBin)
				HiBin++;
			else
				HiBin = 0;
		}
	}

	P->LoMinBin = LoBin;
	P->HiMinBin = HiBin;
	return;
}

void	GetMaximumPolarLimits(POLAR *P, float Limit){
	int	LoBin, HiBin, LargestBin;
	double Sum;

	Sum = P->Bin[P->MaxBin];
	LargestBin = P->NBins -1;
	LoBin = P->MaxBin - 1;
	if(LoBin < 0)
		LoBin = LargestBin;
	HiBin = P->MaxBin + 1;
	if(HiBin > LargestBin)
		HiBin = 0;

	while(Sum < Limit){
		if(P->Bin[LoBin] > P->Bin[HiBin]){
			Sum += P->Bin[LoBin];
			if(LoBin > 0)
				LoBin--;
			else
				LoBin = LargestBin;		 
		}else{
			Sum += P->Bin[HiBin];
			if(HiBin < LargestBin)
				HiBin++;
			else
				HiBin = 0;
		}
	}

	P->LoMaxBin = LoBin;
	P->HiMaxBin = HiBin;
	return;
}
void CalculateRayleighVector(TRACK *T, int StartSamp, int NumberOfSamps, POLAR *P, float ReferenceAngDeg){
	int i, n;
	double X, Y, ang;
	double  hypot(),sin(),cos(), atan2();

	n = 0;
	X = Y =  0.0;
	for(i = StartSamp; i < NumberOfSamps; i++){
		if(T[i].ang == 0.0 && (T[i].rad == 0.0 ))
			continue;
		ang = T[i].ang - (double)ReferenceAngDeg;
                if(ang < 0.0)
                	ang += 360.0;

		ang /= DEG_PER_RAD;
		X += cos(ang);
		Y += sin(ang);
		n++;
	}

	X /= n;
	Y /= n;

	P->RayleighLength = hypot(X,Y);
	P->RayleighAng  = atan2(Y,X);
	if(P->RayleighAng < 0.0)
                P->RayleighAng += (2.0 * M_PI);
	P->RayleighAng *= DEG_PER_RAD;

	return;
}
