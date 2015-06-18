#include <stdlib.h>
#include <stdio.h>
#include "Track.h"
#ifdef WINDOWS_MACHINE
#include "strcasestr.h"
#endif

int	GetTrack(char *Path, char **FileNames, int FileIndex, FILEINFO *FileInfo, TRACK **Track, INFO *Info, int Frame, OPTIONS Options)
{
	char	File[256], Line[256];
	FILE	*fp;
	TRACK	*t;
	long	DataStartOffset	= 0, FirstEligibleSampleOffset;
	int	FileNumber,	i, Time, FileSample, DataSample;
	int	LastTime, LastTimeInPreviousFile;
	float	**Data;
	int	iFrameCount, iTime, iX,	iY, iX2, iY2, iAngle, iSectors,	iState,	iFeeder, iCurrent, iMotor, iFlags, iFrameInfo;
	PARAMETER_INDEX	PI;
	int	 vals, LineIndex, n, j;

	t = *Track;
	Data = NULL;
	
	FileSample = DataSample = LastTimeInPreviousFile = 0;
	for(FileNumber = 0; FileNumber < FileInfo->NFiles; FileNumber++){
		sprintf(File,"%s",FileInfo->FullFileName[FileNumber]);
		fp = fopen(File, "r");
	        if(fp == NULL){
                	fprintf(MyStdErr,"Can't open data file %s in GetTrack\n", File);
                	return(0);
        	}

	        // set fp to start of next trial data
	        // because ReadHeader looks for the data starting at fp
		// this info was set in the last call of GetTrack
       		// fseek(fp, FileInfo->DataEndOffset, SEEK_SET);

		DataStartOffset	= ReadHeader(fp, Info, &PI);

		fseek(fp, DataStartOffset, SEEK_SET);

		if(Data == NULL){ // if this is the first file of an analysis
			Data = (float **)calloc((size_t)Info->NumberOfSamps, sizeof(float *));
			if (Data == NULL){
				fprintf(MyStdOut, "Cannot allocate memory for the data\n");
				return(0);
			}
			for(i =	0; i < Info->NumberOfSamps; i++){
				Data[i]	= (float *)calloc((size_t)Info->FileParameters, sizeof(float));
				if(Data[i] == NULL){
					fprintf(MyStdOut, "Cannot allocate memory for the data\n");
					return(0);
				}
			}
		}

		FirstEligibleSampleOffset = ftell(fp);
		
		// Initialize indices to the data
		iFrameCount = -1;
		iTime =	-1;
		iX = -1;
		iY = -1;
		iX2 = -1;
		iY2 = -1;
		iAngle = -1;
		iSectors = -1;
		iFlags = -1;
		iCurrent = -1;
		iFeeder	= -1;
		iMotor = -1;
		iFrameInfo = -1;
		iState = -1;
					   
		// Determine the indices to the	data
		// Notice that more than one parameter can be stored in	a variable
		// because it is assumed that no data file will	have more than one candidate parameter for a variable
		for(i =	0; i < Info->FileParameters; i++){
			if (PI.FrameCount == i){
				if(iFrameCount == -1)
					iFrameCount = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of FrameCount in	GetTrack\n");
					return(0);
				}
			}else if(PI.msTimeStamp	== i){
				if(iTime == -1)
					iTime =	i;
				else{ 
					fprintf(MyStdErr,"Duplicate assignment of msTimeStamp in GetTrack\n");
					return(0);
				}
			}else if (PI.RoomX == i){
				if(iX == -1)
					iX = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of X in GetTrack\n");
					return(0);
				}
			}else if (PI.RoomY == i){
				if(iY == -1)
					iY = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of Y in GetTrack\n");
					return(0);
				}
			}else if (PI.ArenaX == i){
				if(iX == -1)
					iX = i;
				else if	(iX2 ==	-1)
					iX2 = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of X/X2 in GetTrack\n");
					return(0);
				}
			}else if (PI.ArenaY == i){
				if(iY == -1)
					iY = i;
				else if	(iY2 ==	-1)
					iY2 = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of Y/Y2 in GetTrack\n");
					return(0);
				}
			}else if (PI.Sectors ==	i){
				if(iSectors == -1)
					iSectors = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of Sectors in GetTrack\n");
					return(0);
				}
			}else if (PI.Flags == i){
				if(iFlags == -1)
					iFlags = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of Flags	in GetTrack\n");
					return(0);
				}
			}else if (PI.CurrentLevel == i){
				if(iCurrent == -1)
					iCurrent = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of Current in GetTrack\n");
					return(0);
				}
			}else if (PI.FeederState == i){
				if(iFeeder == -1)
					iFeeder	= i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of Feeder in GetTrack\n");
					return(0);
				}
			}else if (PI.MotorState	== i){
				if(iMotor == -1)
					iMotor = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of Motor in GetTrack\n");
					return(0);
				}
			}else if (PI.FrameInfo == i){
				if(iFrameInfo == -1)
					iFrameInfo = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of FrameInfo in GetTrack\n");
					return(0);
				}
			}else if (PI.State == i){
				if(iState == -1)
					iState = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of State in GetTrack\n");
					return(0);
				}
			}else if (PI.AvoidanceState == i){
				if(iState == -1)
					iState = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of State	in GetTrack\n");
					return(0);
				}
			}else if (PI.PreferenceState ==	i){
				if(iState == -1)
					iState = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of State	in GetTrack\n");
					return(0);
				}
			}else if (PI.DecideState == i){
				if(iState == -1)
					iState = i;
				else{
					fprintf(MyStdErr,"Duplicate assignment of State	in GetTrack\n");
					return(0);
				}
			}else
			   fprintf(MyStdErr,"\nWarning:	Did not assign parameter in column %d to a variable", i+1);
		}

		// Check whether the miminimum necessary info is available
		if(Info->ParadigmType == TRACKER){
			// if((iTime == -1) || (iX	== -1) || (iY == -1) ||	(iFrameInfo == -1)){
			if((iTime == -1) || (iX	== -1) || (iY == -1)){
				fprintf(MyStdErr, "\nInsufficient set of parameters parsed for paradigm: %s\n",	Info->Paradigm);
				return(0);
			}
		}	
		if(Info->ParadigmType == PLACE_AVOIDANCE){
		//	if((iTime == -1) || (iX	== -1) || (iY == -1) ||	(iSectors == -1) || (iCurrent == -1) ||	(iState	== -1) || (iFrameInfo == -1)){
			if((iTime == -1) || (iX	== -1) || (iY == -1) ||	(iSectors == -1) || (iState == -1) || (iFrameInfo == -1)){
				fprintf(MyStdErr, "\nInsufficient set of parameters parsed for paradigm: %s\n",	Info->Paradigm);
				return(0);
			}
		}	
		if(Info->ParadigmType == PLACE_PREFERENCE){
			if((iTime == -1) || (iX	== -1) || (iY == -1) ||	(iSectors == -1) || (iFeeder ==	-1) || (iState == -1) || (iFrameInfo ==	-1)){
				fprintf(MyStdErr, "\nInsufficient set of parameters parsed for paradigm: %s\n", Info->Paradigm);
				return(0);
			}
		}	
		if(Info->ParadigmType == WATER_MAZE){
			if((iTime == -1) || (iX	== -1) || (iY == -1) ||	(iState == -1) || (iFrameInfo == -1)){
				fprintf(MyStdErr, "\nInsufficient set of parameters parsed for paradigm: %s\n",	Info->Paradigm);
				return(0);
			}
		}

		if(Info->ParadigmType == OPEN_FIELD){
			if((iTime == -1) || (iX	== -1) || (iY == -1)){
				fprintf(MyStdErr, "\nInsufficient set of parameters parsed for paradigm: %s\n",	Info->Paradigm);
				return(0);
			}
		}	
		
		// Get the data. Parse it from a line to ensure 
		// that the formatting	is correct
		// First find the first	sample to save
		fseek(fp, DataStartOffset, SEEK_SET);

		// don't use for(s =	0; s < Info->NumberOfSamps;	s++){
		// Already determined that the data are presentin GetTrackSize
		// but NumberOfSamps refers to the number of samples to be analysed not the number of samples in the file
		// for example may want to analyse 100 samples starting at the 1000th sample
		
		FileSample = 0;	// indexes data samples in the file whereas DataSample indexes data samples in memory
		while(1){
			while (strlen(GetLine(fp, Line)) <= 2); // account for the newline and line feed chars
			for(i =	0, vals = 0, LineIndex = 0; i < Info->FileParameters; i++){
				vals += sscanf(Line+LineIndex, "%f%n", &(Data[DataSample][i]), &n);
				// Although vals can count if a value was missing, it is of no consequence
				LineIndex += n;
			}
			if((long)(Data[DataSample][iTime]) >= FileInfo->StartTime[FileNumber]){
				FileSample++;
				DataSample++; // advance index to the next data sample
				break;
			}
		}
		// fill	in the Data array
		if(!FileSample){
			fprintf(MyStdErr,"There are no data with timestamps after the analysis start time of %d\n", (int)(FileInfo->StartTime[FileNumber]/1000));
			return -1;
		}
		for(; FileSample < FileInfo->NSamples[FileNumber]; FileSample++, DataSample++){
//		for(; DataSample < Info->NumberOfSamps; FileSample, DataSample++){
			while (strlen(GetLine(fp, Line)) <= 2); // account for the newline and line feed chars after
			for(i =	0, vals = 0, LineIndex = 0; i < Info->FileParameters; i++){
				vals += sscanf(Line+LineIndex, "%f%n", &(Data[DataSample][i]), &n);
				LineIndex += n;
			}
//HERE IS THE ISSUE
			if(vals < (i-1)){ // If a value was missed it was because the Flag was not defined in this sample
printf("HERE IS THE ISSUE vals=%d FileParameters=%d i=%d\n", vals, Info->FileParameters, i);
					  // shift all subsequent values accordingly
					for(j = iFlags; j < Info->FileParameters; j++)
						Data[DataSample][j+1] = Data[DataSample][j];
					Data[DataSample][iFlags] = 0;
			}

			Time = (long) (Data[DataSample][iTime]);
	
			if(Time	> FileInfo->StopTime[FileNumber])
				break;

			if(Time){	// check if Time is not	zero
				Data[DataSample][iTime] = Time + LastTimeInPreviousFile;
				LastTime = (long)Data[DataSample][iTime];
			}
		}
		// store the file offest at which the last trial's data ends
        	FileInfo->DataEndOffset = ftell(fp);

		fclose(fp);
		LastTimeInPreviousFile = LastTime + POSITION_SAMPLING_INTERVAL;	 // add	time for one sample
	}

	for(DataSample = 0; DataSample < Info->NumberOfSamps; DataSample++){
		if(iFrameCount != -1) t[DataSample].framecount = (long)Data[DataSample][iFrameCount];
		if(iTime != -1) t[DataSample].time = (long)Data[DataSample][iTime];
		if(iX != -1) t[DataSample].x = (long)Data[DataSample][iX];
		if(iY != -1) t[DataSample].y = (long)Data[DataSample][iY];
		if(iX2 != -1) t[DataSample].x2 = (long)Data[DataSample][iX2];
		if(iY2 != -1) t[DataSample].y2 = (long)Data[DataSample][iY2];
		if(iAngle != -1) t[DataSample].angle = (long)Data[DataSample][iAngle];
		if(iSectors != -1) t[DataSample].Sector = (long)Data[DataSample][iSectors];
		if(iState != -1){
			t[DataSample].State	= (long)Data[DataSample][iState];
			t[DataSample].event	= (long)Data[DataSample][iState];	// for backward	compatability
		}
		if(iFeeder != -1) t[DataSample].Feeder = (long)Data[DataSample][iFeeder];
		if(iMotor != -1) t[DataSample].Motor = (long)Data[DataSample][iMotor];
		if(iCurrent != -1) t[DataSample].ShockLevel	= (long)Data[DataSample][iCurrent];
		if(iFlags != -1) t[DataSample].Flag	= (long)Data[DataSample][iFlags];
		if(iFrameInfo != -1) t[DataSample].FrameInfo = (long)Data[DataSample][iFrameInfo];

	}
	// Make sure the x,y samples are good numbers between 0 and 255
	for(DataSample = 0;	DataSample < Info->NumberOfSamps; DataSample++){
		if((t[DataSample].x > MAXIMUM_COORDINATE) || (t[DataSample].x < 0)) t[DataSample].x = 0;
		if((t[DataSample].y > MAXIMUM_COORDINATE) || (t[DataSample].y < 0)) t[DataSample].y = 0;
		if((t[DataSample].x2 > MAXIMUM_COORDINATE) || (t[DataSample].x2 < 0)) t[DataSample].x2 = 0;
		if((t[DataSample].y2 > MAXIMUM_COORDINATE) || (t[DataSample].y2 < 0)) t[DataSample].y2 = 0;
	}

	// FGX recorded	missed detects as 127, 127 rather than 0,0 until 20.05.2001
	if((Info->FileFormat ==	FGX_FORMAT) && (Info->Date.Year	<= 2001) &&
		 (Info->Date.Month <= 5) && (Info->Date.Day < 21)){
		for(DataSample = 0;	DataSample < Info->NumberOfSamps; DataSample++){
			if((t[DataSample].x	== 127)	&& (t[DataSample].y	== 127)){
				t[DataSample].x = t[DataSample].y = 0;
			}
		}
	}

	// it is an error to free the Data array if it is local memory
	for(DataSample = 0;	DataSample < Info->NumberOfSamps; DataSample++)
		free(Data[DataSample]);
	free(Data);

	return(1);
}

int	GetTrackSize(char *Path, char **FileNames, int FileIndex, FILEINFO *FileInfo, INFO *Info, int Frame)
{
	char	File[256], Line[256], string[256];
	FILE	*fp;
	long	DataStartOffset = 0, DataOffset;
	int	FileNumber, NumberOfSamples, dummy, FrameNumber, Time;
	PARAMETER_INDEX PI;

	Info->NumberOfSamps = 0;

	strcpy(FileInfo->FilePath, Path);	
	
	for(FileNumber = 0; FileNumber < FileInfo->NFiles; FileNumber++){
		// Try file naming format: filename.dat
		// try to open file with and without .dat extension
			// file_name.dat
		sprintf(File, "%s%s", FileNames[FileIndex + FileNumber], DATA_FILE_SUFFIX);
		fp = fopen(File, "r");
		if(fp == NULL){ //file_name.dat not found
				// try file_name
			sprintf(File, "%s", FileNames[FileIndex + FileNumber]);
			fp = fopen(File, "r");
			if(fp == NULL){ // filename not found. 
					// Try filename_Room.dat
				sprintf(File, "%s%s%s", FileNames[FileIndex + FileNumber], ROOM_FRAME_IN_FILENAME, DATA_FILE_SUFFIX);
				fp = fopen(File, "r");
				if(fp == NULL){ // filename_Room.dat not found
						// try filename_Room
					sprintf(File, "%s%s", FileNames[FileIndex + FileNumber], ROOM_FRAME_IN_FILENAME);
					fp = fopen(File, "r");
					if(fp == NULL){ // filename_Room not found.
							// try filename_Arena.dat
						sprintf(File, "%s%s%s", FileNames[FileIndex + FileNumber], ARENA_FRAME_IN_FILENAME, DATA_FILE_SUFFIX);
						fp = fopen(File, "r");
						if(fp == NULL){ // filename_Arena.dat not found
								// try filename_Arena
							sprintf(File, "%s%s", FileNames[FileIndex + FileNumber], ARENA_FRAME_IN_FILENAME);
							fp = fopen(File, "r");
							if(fp == NULL){ // File not found
								return(0);
							}
						}
					}
				}
			}
		}
		strcpy(FileInfo->FullFileName[FileNumber], File);

		DataStartOffset = ReadHeader(fp, Info, &PI);

		if(Info->FrameTypeOfData == UNKNOWN_FRAME){
			if(strcasestr(File, ROOM_FRAME_IN_FILENAME) != NULL){ // assume this file contains Room frame data
				Info->FrameTypeOfData = ROOM_FRAME;
			}else if(strcasestr(File, ARENA_FRAME_IN_FILENAME) != NULL){ // assume this file contains Arena frame data
				Info->FrameTypeOfData = ARENA_FRAME;
			}
		}
		if(Info->FrameTypeOfData == UNKNOWN_FRAME){	// warn the user
			 //assume this file contains Room frame data as the default
			Info->FrameTypeOfData = ROOM_FRAME;
			fprintf(MyStdOut, "%d\nCoordinate Frame of the track data is unknown. Assuming Room or Camera Frame data.\n", 7); 
			fprintf(MyStdOut, "This information should be specified in the .dat file's Header by the line %%Frame.0 ( TypeOfFrame ).\n");
			fprintf(MyStdOut, "Or by ending the filename with either _Room or _Arena.\n");
			fprintf(MyStdOut, "If the data are from the Arena Frame then the analyses are incorrect and the .dat file must somehow specify which frame is being used.\n");
		}

		fseek(fp, DataStartOffset, SEEK_SET);

		NumberOfSamples = 0;

		while((DataOffset = ftell(fp)) != -1){
			if(*GetLine(fp, Line) == '\0') // end of data
				break;

			// to check for start of a new trial and to tolerate blank lines in unix and dos formats
			if(sscanf(Line, "%d", &dummy) != 1){
				sscanf(Line, "%s", &string);
				if(!strcmp(string, "%%BEGIN_HEADER"))
					break;
				else
					continue;
			}
			if(sscanf(Line, "%d%d", &FrameNumber, &Time) != 2){
				fprintf(MyStdOut,"\nExpected FrameNumber and Time data not present. File corrupted.\n");
				fprintf(MyStdOut,"Data line %d. Corrupt data to read are: %s\n", NumberOfSamples, Line);
				return(0);
			}
			if(Time >= FileInfo->StartTime[FileNumber]){
				if(Time < FileInfo->StopTime[FileNumber]){
					NumberOfSamples++;
				}else{
					break;
				}
			}
		}
		Info->NumberOfSamps += NumberOfSamples;
		FileInfo->NSamples[FileNumber] = NumberOfSamples;

		if(!NumberOfSamples){
			fprintf(MyStdOut, "\nData file does not have samples with time stamps in the range you selected ([%d %d])\n", (int)(FileInfo->StartTime[FileNumber]/1000), (int)(FileInfo->StopTime[FileNumber]/1000));
			return(0);
		}
		fclose(fp);
	}
	return(FileNumber);
}

int	GetNextTrackSize(FILEINFO *FileInfo, INFO *Info, int FileNumber)
{
	char	Line[256], string[256];
	FILE	*fp;
	long	DataStartOffset = 0, DataOffset;
	int	NumberOfSamples, dummy, FrameNumber, Time;
	PARAMETER_INDEX PI;

	Info->NumberOfSamps = 0;
	fp = fopen(FileInfo->FullFileName[FileNumber], "r");
	if(fp == NULL){
		fprintf(MyStdErr,"\nCan't open data file %s in GetNextTrackSize\n", FileInfo->FullFileName[FileNumber]);
		return(0);
	}

	// set fp to start of next trial data
	fseek(fp, FileInfo->DataEndOffset, SEEK_SET);

	DataStartOffset = ReadNextHeader(fp, Info, &PI);

	if(DataStartOffset == 0) // no data to read
		return 0;

	fseek(fp, DataStartOffset, SEEK_SET);

	NumberOfSamples = 0;

	while((DataOffset = ftell(fp)) != -1){
		if(*GetLine(fp, Line) == '\0') // end of data
			break;

		// to check for start of a new trial and to tolerate blank lines in unix and dos formats
		if(sscanf(Line, "%d", &dummy) != 1){
			sscanf(Line, "%s", &string);
			if(!strcmp(string, "%%BEGIN_HEADER"))
				break;
			else
				continue;
		}

		if(sscanf(Line, "%d%d", &FrameNumber, &Time) != 2){
			fprintf(MyStdOut,"\nExpected FrameNumber and Time data not present. File corrupted.\n");
			fprintf(MyStdOut,"Data line %d. Corrupt data to read are: %s\n", NumberOfSamples, Line);
			return(0);
		}

		if(Time >= FileInfo->StartTime[FileNumber]){
			if(Time < FileInfo->StopTime[FileNumber]){
				NumberOfSamples++;
			}else{
				break;
			}
		}
	}
	// store the file offest at which the last trial's data ends
	// in GetTrack after the data are actually read into memory
	// FileInfo->DataEndOffset = DataOffset;

	Info->NumberOfSamps = NumberOfSamples;
	if(!NumberOfSamples){
		fprintf(MyStdOut, "\nData file does not have samples with time stamps in the range you selected ([%d %d])\n", (int)(FileInfo->StartTime[FileNumber]/1000), (int)(FileInfo->StopTime[FileNumber]/1000));
                return(0);
        }
	fclose(fp);
	return(1);
}

void FilterTrack(TRACK **Track, INFO *Info){
	int		i, Sample, FirstGoodSample, LastGoodSample, PreviousSampleGood;
	int		dTimeStep_ms, temp;
	double	dTime_ms, dDist_pix, dDist_x, dDist_y;
	TRACK	*t;

	t = *Track;

	// exclude points that are too far to be plausible

	for(FirstGoodSample = 0; FirstGoodSample < Info->NumberOfSamps; FirstGoodSample++){
		if(t[FirstGoodSample].x  && t[FirstGoodSample].y) // find a good sample
			break;
	}

	for(Sample = FirstGoodSample + 1; Sample < Info->NumberOfSamps; Sample++){
		if((t[Sample].x == 0) && (t[Sample].y == 0)){ // missed detect
			continue;
		}

		if((t[Sample - 1].x == 0) && (t[Sample - 1].y == 0)){ // last sample was missed
			continue;
		}

		dDist_pix = hypot(t[Sample - 1].x - t[Sample].x, t[Sample - 1].y - t[Sample].y);

		if((dDist_pix / Info->PixelsPerCM) >
				(MAX_DISTANCE_CM_PER_MS * (t[Sample].time - t[Sample - 1].time))){

			t[Sample].x = t[Sample].y = 0;
		}
	}

	// try to fix missed detects

	PreviousSampleGood = 1;

	for(Sample = FirstGoodSample + 1; Sample < Info->NumberOfSamps; Sample++){
		if((t[Sample].x == 0) && (t[Sample].y == 0)){ // missed detect
			if(PreviousSampleGood){
				LastGoodSample = Sample - 1;
				PreviousSampleGood = 0;
				continue;
			}

		}else if (!PreviousSampleGood){ // have a good sample, so try to fix previous
			dTime_ms = (double)(t[Sample].time - t[LastGoodSample].time);
			if (dTime_ms && (dTime_ms < TIME_TOO_LONG_TO_INTERPOLATE_MS)){
				dDist_x = t[Sample].x - t[LastGoodSample].x;
				dDist_y = t[Sample].y - t[LastGoodSample].y;
				dDist_pix = hypot(dDist_x, dDist_y);

				if ((dDist_pix / Info->PixelsPerCM) < DISTANCE_TOO_FAR_TO_INTERPOLATE_CM) {
					// o.k. to interpolate so do it

					dDist_x /= dTime_ms;
					dDist_y /= dTime_ms;
					dTimeStep_ms = (int)(dTime_ms / (Sample - LastGoodSample));

					for (i = LastGoodSample + 1; i < Sample; i++){
						if(!t[i].time)
							t[i].time = t[i-1].time + dTimeStep_ms;
						dTime_ms = t[i].time - t[i-1].time;
						temp = t[i].x;
						t[i].x = t[i-1].x + (int)(dDist_x * dTime_ms + 0.5);
						if(t[i].x > 255)
							t[i].x = temp;

						temp = t[i].y;
						t[i].y = t[i-1].y + (int)(dDist_y * dTime_ms + 0.5);
						if(t[i].y > 255)
							t[i].y = temp;
					}
					PreviousSampleGood = 1;
				}
			}
			PreviousSampleGood = 1;		
		}
	}

	return;

}
#define MINIMUM_GOOD_SAMPS	8
#define NUMBER_OF_SAMPLES_TO_CHECK 10

void ExcludeFirstNoDetects(TRACK **Track, INFO *Info){

	int		i, FirstGoodSample, GoodSamps;
	int		MinimumGoodSamples = MINIMUM_GOOD_SAMPS;
	TRACK	*t;

	t = *Track;

	// exclude the first set of no detects

	for(FirstGoodSample = 0; FirstGoodSample < Info->NumberOfSamps; FirstGoodSample++){
		if(t[FirstGoodSample].x  || t[FirstGoodSample].y){ // find a good sample
			GoodSamps = 0;
			for(i = 0; i < NUMBER_OF_SAMPLES_TO_CHECK; i++){
				GoodSamps += (t[FirstGoodSample + i].x  || t[FirstGoodSample + i].y); // sum good samples
			}
			if(GoodSamps >= MinimumGoodSamples)
				break;
		}
	}
	if(FirstGoodSample == Info->NumberOfSamps){
		// fprintf(MyStdOut,"\nExcluding First No Detects. Too many samples to exclude. Using all the track.\n");
		FirstGoodSample = 0;
	}

	//  set the track data to start with the first good detect
	*Track = (t+FirstGoodSample);	
	Info->NumberOfSamps -= FirstGoodSample;
	Info->NumberOfInitialSampsExcluded = FirstGoodSample;

	if(FirstGoodSample)
		fprintf(MyStdOut, "\n\tWatermaze: Excluded initial %d No detect samples", FirstGoodSample);

	return;
}

void ExcludeExtraSamples(TRACK **Track, INFO *Info){

	int		i;
	TRACK	*t;

	t = *Track;


	for(i = 0; i < Info->NumberOfSamps; i++){
		if((t[i].time - t[0].time) > Info->Options.TimeLimit){
			Info->NumberOfSamps = i;
			break;
		}
	}

	return;
}
