// 20.11.2007 Corrected error associated with Tracker format change.
// Tracker reports the Sector radius parameters in %radius. TrackAnalysis needs these valuse in pixels.
// Added a condition to do the conversion if the file format is tracker
// 23.08.13 I->Target[Frame].Type = REINFORCED_SECTOR; // 23.08.13 added this because ReinforcedSector is used subsequently even if not positions are defined
// 23.08.13 also changed I->Target[0].Type = REINFORCED_SECTOR; to I->Target[Frame].Type = REINFORCED_SECTOR;  in several places that are flagged with the 23.08.13 data ena a comment


#include	"Track.h"
		
// Prepared for the
// Tracker
// Place Avoidance
// Place Preference
// Open Field
// Paradigms

// These are the Labels that are searched for in the Header file
#define	HEADER_BEGIN_LABEL				"%%BEGIN_HEADER"
#define	HEADER_END_LABEL				"%%END_HEADER"
#define	SECTION_BEGIN_LABEL				"%%BEGIN"
#define	SECTION_END_LABEL				"%%END"
#define	COMMENT						"//"
#define	TYPE_PREFIX					'.'

#define	DTTRACKER_HEADER_TYPE				"DTtracker"
#define	FGX_HEADER_TYPE					"FGX"
#define	ITRACK_HEADER_TYPE				"iTrack"
#define	TRACKER_HEADER_TYPE				"Tracker"

#define MAX_NUMBER_OF_FORMAT_PARAMETERS			20

#define TRACKER_VERSION_LABEL				"%TrackerVersion."
#define DATE_LABEL					"%Date."
#define TIME_LABEL					"%Time."
#define TRIALS_START_LABEL				"%Start."
#define TRIALS_OLD_START_LABEL                              "%Start"
#define TRACKER_RESOLUTION_LABEL			"%TrackerResolution_PixPerCM."
#define PARADIGM_LABEL					"%Paradigm."
#define REINFORCED_SECTOR_LABEL				"%ReinforcedSector."
#define ROOM_REINFORCED_SECTOR_LABEL			"%RoomReinforcedSector."
#define ARENA_REINFORCED_SECTOR_LABEL			"%ArenaReinforcedSector."
#define SHOCK_PARAMETERS_LABEL				"%ShockParameters."
#define SAMPLE_FORMAT_LABEL				"%Sample."
#define ARENA_CENTER_XY_LABEL				"%ArenaCenterXY."
#define ARENA_DIAMETER_M_LABEL				"%ArenaDiameter_m."
#define FRAME_LABEL					"%Frame."
#define FEEDER_MODE_LABEL				"%FeederMode."
#define FEEDER_PARAMETERS_LABEL				"%FeederParameters."

long	ReadHeader(FILE *fp, INFO *I, PARAMETER_INDEX *PI)
{
	long	HeaderStart, HeaderEnd;
	char	KeyWordString[256];
	int	ReinforcedArea = 0;

        // These are currently set to default values
        I->Arena.Type = ARENA_TYPE_CIRCLE;
        I->Arena.Bottom = DEFAULT_ARENA_CENTER_Y * 2 - 1;
        I->Arena.Right = DEFAULT_ARENA_CENTER_X * 2 - 1;
        I->Arena.Top = I->Arena.Left = 0;
        I->Arena.Radius = DEFAULT_SMALL_ARENA_RADIUS;
        I->Arena.CenterX = DEFAULT_ARENA_CENTER_X;
        I->Arena.CenterY = DEFAULT_ARENA_CENTER_Y;
        I->TimeStep = DEFAULT_SAMPLING_INTERVAL;
        I->FileFormat = ITRACK_FORMAT;

        // this is the preferred method
        // I->Shock.CalculationMethod = SAMPLE_BASED;
        I->Shock.CalculationMethod = STATE_BASED;

	// Look for header beginning
	HeaderStart = FindNextHeaderStart(fp);
	if(!HeaderStart){
		fprintf(MyStdOut,"\nCan't find start of header (%s).\nWrong File Format.\n", HEADER_BEGIN_LABEL);
		return(0);
	}

	if(I->Options.OpenField){	// the option overrides whatever the header indicates the paradigm was
					// setting before reading the header will determine what ReadHeader will return
		I->ParadigmType = OPEN_FIELD;
		strcpy(I->Paradigm, OPEN_FIELD_STRING);
	}

	// Check to see what tracker format it is: Tracker, iTrack, FGX or DTtracker
	fscanf(fp,"%s", KeyWordString);
	if(!strcmp(KeyWordString, ITRACK_HEADER_TYPE)){
		I->FileFormat = ITRACK_FORMAT;
		HeaderEnd = ReadHeaderITrack(fp, I, HeaderStart, PI);
	}else if(!strcmp(KeyWordString, TRACKER_HEADER_TYPE)){
		I->FileFormat = TRACKER_FORMAT;
		HeaderEnd = ReadHeaderTracker(fp, I, HeaderStart, PI);
	}else if(!strcmp(KeyWordString, FGX_HEADER_TYPE)){
		I->FileFormat = FGX_FORMAT;
		HeaderEnd = ReadHeaderFGX(fp, I, HeaderStart, PI);
	}else if(!strcmp(KeyWordString, DTTRACKER_HEADER_TYPE)){
		I->FileFormat = DTtracker_FORMAT;
		HeaderEnd = ReadHeaderDTtracker(fp, I, HeaderStart);
	}else if(GetTrackerVersion(fp, I, HeaderStart) == ITRACK_FORMAT){ // Itrack has a line in the Header that indicates the tracker type
		I->FileFormat = ITRACK_FORMAT;
		HeaderEnd = ReadHeaderITrack(fp, I, HeaderStart, PI);
	}else if(GetTrackerVersion(fp, I, HeaderStart) == TRACKER_FORMAT){ // Tracker has a line in the Header that indicates the tracker type
		I->FileFormat = TRACKER_FORMAT;
		HeaderEnd = ReadHeaderTracker(fp, I, HeaderStart, PI);
	}else{	// HACK FOR iTRACK
		fprintf(MyStdOut,"\nCan't determine data format type.\nWrong File Format.\n");
		return(0);
	}

	if(I->Options.OpenField){	// the option overrides whatever the header indicates the paradigm was
		I->ParadigmType = OPEN_FIELD;
		strcpy(I->Paradigm, OPEN_FIELD_STRING);
	}else if(I->Options.Watermaze){	// the option overrides whatever the header indicates the paradigm was
		I->ParadigmType = WATER_MAZE;
		strcpy(I->Paradigm, WATERMAZE_STRING);
	}

	if(HeaderEnd){
		return(HeaderEnd);
	}else{
		fprintf(MyStdOut,"Can't determine Header type\nWrong File Format.\n");
		return(0);
	}
}

long	ReadNextHeader(FILE *fp, INFO *I, PARAMETER_INDEX *PI)
{
	long	HeaderStart, HeaderEnd;
	char	KeyWordString[256];
	int	ReinforcedArea = 0;

        // These are currently set to default values
        I->Arena.Type = ARENA_TYPE_CIRCLE;
        I->Arena.Bottom = DEFAULT_ARENA_CENTER_Y * 2 - 1;
        I->Arena.Right = DEFAULT_ARENA_CENTER_X * 2 - 1;
        I->Arena.Top = I->Arena.Left = 0;
        I->Arena.Radius = DEFAULT_SMALL_ARENA_RADIUS;
        I->Arena.CenterX = DEFAULT_ARENA_CENTER_X;
        I->Arena.CenterY = DEFAULT_ARENA_CENTER_Y;
        I->TimeStep = DEFAULT_SAMPLING_INTERVAL;
        I->FileFormat = ITRACK_FORMAT;

        // this is the preferred method
        // I->Shock.CalculationMethod = SAMPLE_BASED;
        I->Shock.CalculationMethod = STATE_BASED;

	// Look for header beginning
	HeaderStart = FindNextHeaderStart(fp);
	if(!HeaderStart){
	//	fprintf(MyStdOut,"\nCan't find start of header (%s).\nWrong File Format.\n", HEADER_BEGIN_LABEL);
		return(0);
	}

	if(I->Options.OpenField){	// the option overrides whatever the header indicates the paradigm was
					// setting before reading the header will determine what ReadHeader will return
		I->ParadigmType = OPEN_FIELD;
		strcpy(I->Paradigm, OPEN_FIELD_STRING);
	}

	// Check to see what tracker format it is: iTrack, FGX or DTtracker
	fscanf(fp,"%s", KeyWordString);
	if(!strcmp(KeyWordString, ITRACK_HEADER_TYPE)){
		I->FileFormat = ITRACK_FORMAT;
		HeaderEnd = ReadHeaderITrack(fp, I, HeaderStart, PI);
	}else if(!strcmp(KeyWordString, TRACKER_HEADER_TYPE)){
		I->FileFormat = TRACKER_FORMAT;
		HeaderEnd = ReadHeaderTracker(fp, I, HeaderStart, PI);
	}else if(!strcmp(KeyWordString, FGX_HEADER_TYPE)){
		I->FileFormat = FGX_FORMAT;
		HeaderEnd = ReadHeaderFGX(fp, I, HeaderStart, PI);
	}else if(!strcmp(KeyWordString, DTTRACKER_HEADER_TYPE)){
		I->FileFormat = DTtracker_FORMAT;
		HeaderEnd = ReadHeaderDTtracker(fp, I, HeaderStart);
	}else if(GetTrackerVersion(fp, I, HeaderStart) == ITRACK_FORMAT){ // Itrack has a line in the Header that indicates the tracker type
		I->FileFormat = ITRACK_FORMAT;
		HeaderEnd = ReadHeaderITrack(fp, I, HeaderStart, PI);
	}else if(GetTrackerVersion(fp, I, HeaderStart) == TRACKER_FORMAT){ // Tracker has a line in the Header that indicates the tracker type
		I->FileFormat = TRACKER_FORMAT;
		HeaderEnd = ReadHeaderTracker(fp, I, HeaderStart, PI);
	}else{	// HACK FOR iTRACK
		fprintf(MyStdOut,"\nCan't determine data format type.\nWrong File Format.\n");
		return(0);
	}
	if(I->Options.OpenField){	// the option overrides whatever the header indicates the paradigm was
		I->ParadigmType = OPEN_FIELD;
		strcpy(I->Paradigm, OPEN_FIELD_STRING);
	}else if(I->Options.Watermaze){	// the option overrides whatever the header indicates the paradigm was
		I->ParadigmType = WATER_MAZE;
		strcpy(I->Paradigm, WATERMAZE_STRING);
	}

	if(HeaderEnd){
		return(HeaderEnd);
	}else{
	//	fprintf(MyStdOut,"Can't determine Header type\nWrong File Format.\n");
		return(0);
	}
}

int	GetTrackerVersion(FILE *fp, INFO *I, long HeaderStart)
{
	char	KeyWordString[256], Encoding[512], TrackerVersion[256], VersionText[256], ReleaseText[256], ReleaseDate[12], **Parameters = NULL;
	int		Type, NumberOfParameters;
	long	SectionStart;
	float	VersionNumber;

	SectionStart = FindSection(fp, HeaderStart, "SETUP_INFORMATION");
	if(SectionStart){
		// TrackerVersion
		if(FindKeyWord(fp, SectionStart, TRACKER_VERSION_LABEL, KeyWordString, Encoding)){
			Type = GetType(KeyWordString);
			switch(Type){
			case -1:
				fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
				return(0);

			case 0:	// ASCII encoded
				NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
				if(!NumberOfParameters){
					fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRACKER_VERSION_LABEL);
					return 0;
				}
				if(NumberOfParameters >= 5){
					sscanf(Parameters[0],"%s", TrackerVersion);
					sscanf(Parameters[2],"%f", &VersionNumber);
					sscanf(Parameters[4],"%s", ReleaseDate);
					I->FileFormatVersion = VersionNumber;
					strcpy(I->FileFormatReleaseDate, ReleaseDate);
					if(!strcmp(TrackerVersion, ITRACK_HEADER_TYPE)){
						I->FileFormat = ITRACK_FORMAT;
						return(ITRACK_FORMAT);
					}else if (!strcmp(TrackerVersion, TRACKER_HEADER_TYPE)){
						I->FileFormat = TRACKER_FORMAT;
						return(TRACKER_FORMAT);
					}else{
						fprintf(MyStdOut,"(%s).\nTracker version not recognized.\n",Parameters[0]);
						return 0;
					}
				}
				break;
			default:
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRACKER_VERSION_LABEL);
				return 0;
			}
		}
	}

	return(0);
}

long	ReadHeaderTracker(FILE *fp, INFO *I, long HeaderStart, PARAMETER_INDEX *PI)
{
	long	Offset, HeaderEnd, SectionStart;
	char	KeyWordString[256], Encoding[512], **Parameters;
	int		Type, NumberOfParameters, Frame, ReinforcedArea = 0;
	int		ArenaRadius_pix;


	// Section: DATABASE_INFORMATION
	SectionStart = FindSection(fp, HeaderStart, "DATABASE_INFORMATION");
	if(SectionStart){
		// Date
		if(!FindKeyWord(fp, SectionStart, DATE_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", DATE_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", DATE_LABEL);
				return 0;
			}
			if(NumberOfParameters == 1){
				sscanf(Parameters[0],"%d.%d.%d", &(I->Date.Day), &(I->Date.Month), &(I->Date.Year));
			}else if(NumberOfParameters == 3){
				sscanf(Parameters[0],"%d. %d. %d", &(I->Date.Day), &(I->Date.Month), &(I->Date.Year));
			}else{
				fprintf(MyStdOut,"%s format must be dd.mm.yyyy\nWrong File Format.\n", DATE_LABEL);
				return(0);
			}
			if((I->Date.Year > 2001) || ((I->Date.Year >=2001) && (I->Date.Month >= 5))){
				I->Shock.CalculationMethod = STATE_BASED; // SAMPLE_BASED;
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", DATE_LABEL);
			return(0);
		}

		Offset = FindSectionEnd(fp);
		if(!Offset)
			fprintf(MyStdOut,"\nCan't find end of Header Section (%s).\nWrong File Format.\n", "DATA_BASE_INFORMATION");
	}

	// Section: SETUP_INFORMATION
	SectionStart = FindSection(fp, HeaderStart, "SETUP_INFORMATION");
	if(SectionStart){
		// Do stuff ...

		// Paradigm
		if(I->ParadigmType == OPEN_FIELD){	// this is determined by an option and thus the Header info should be ignored
		}else{
			if(!FindKeyWord(fp, SectionStart, PARADIGM_LABEL, KeyWordString, Encoding)){
				// if paradigm is not specified then assume it is TRACKER which is basic
				// fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", PARADIGM_LABEL);
				strcpy(I->Paradigm, TRACKER_STRING);
                     	  	 I->ParadigmType = TRACKER;
				// return 0;
			}else{
				Type = GetType(KeyWordString);
				switch(Type){
				case -1:
					fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
					return(0);

				case 0:	// ASCII encoded
					NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
					if(!NumberOfParameters){
						fprintf(MyStdOut,"(%s).\nWrong File Format.\n", PARADIGM_LABEL);
						return 0;
					}
					if(NumberOfParameters == 1){
						strcpy(I->Paradigm, Parameters[0]);
						if(!strcmp(I->Paradigm, PLACE_AVOIDANCE_STRING))
       	                		                I->ParadigmType = PLACE_AVOIDANCE;
       	                		        else if(!strcmp(I->Paradigm, PLACE_PREFERENCE_STRING))
       	                		                I->ParadigmType = PLACE_PREFERENCE;
       	                		        else if(!strcmp(I->Paradigm, TRACKER_STRING))
       		                 			I->ParadigmType = TRACKER;
       	                		        else if(!strcmp(I->Paradigm, OPEN_FIELD_STRING))
       		                 			I->ParadigmType = OPEN_FIELD;
						else{
       	                		                fprintf(MyStdOut,"TrackAnalysis is not prepared for this paradigm (%s)\n", I->Paradigm);
       	                		                return(0);
       	                		         }
					}else{
						fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", PARADIGM_LABEL);
						return(0);
					}
					break;
	
				default:
					fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", PARADIGM_LABEL);
					return(0);
				}
			}
		} // end of dealing with if optional paradigm is openfield

		// Tracker resolution PixelsPer cm
		if(!FindKeyWord(fp, SectionStart, TRACKER_RESOLUTION_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(-1);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
				return(0);
			}
			if(NumberOfParameters == 1){
				I->PixelsPerCM = atof(Parameters[0]);
			}else{
				fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
			return(0);
		}
					
		// Frame
		
		I->FrameTypeOfData = UNKNOWN_FRAME;
		if(!FindKeyWord(fp, SectionStart, FRAME_LABEL, KeyWordString, Encoding)){
			I->NumberOfCoordFrames = 1;
                        Frame = 0;
                        I->Target[Frame].CoordFrame = SINGLE_FRAME;
			I->FrameTypeOfData = UNKNOWN_FRAME;

			// fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", FRAME_LABEL);
			// return 0;
		}else{
			Type = GetType(KeyWordString);
			switch(Type){
			case -1:
				fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
				return(0);

			case 0:	// ASCII encoded
				NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
				if(!NumberOfParameters){
					fprintf(MyStdOut,"(%s).\nWrong File Format.\n", FRAME_LABEL);
					return 0;
				}
				if(NumberOfParameters == 1){
					strcpy(I->Frame, Parameters[0]);
					I->NumberOfCoordFrames = 1;
					Frame = SINGLE_FRAME;
					I->Target[Frame].CoordFrame = SINGLE_FRAME;

					// use this opportunity to set the data type because Tracker <=2.2 did not keep the frame independent
					// if this is not set here there is a second opportunity in GetTrack based on if the filename ends with _Room or _Arena
					if(strcasestr(I->Frame, HEADER_ROOM_FRAME_WORD) != NULL)
						I->FrameTypeOfData = ROOM_FRAME;
					else if(strcasestr(I->Frame, HEADER_ARENA_FRAME_WORD) != NULL)
						I->FrameTypeOfData = ARENA_FRAME;
					else if(strcasestr(I->Frame, HEADER_CAMERA_FRAME_WORD) != NULL)
						I->FrameTypeOfData = ROOM_FRAME;
				}else{
					fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", FRAME_LABEL);
					return(0);
				}
				break;

			default:
				fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", FRAME_LABEL);
				return(0);
			}
		}

		// ArenaCenterXY
		if(!FindKeyWord(fp, SectionStart, ARENA_CENTER_XY_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
				return 0;
			}
			if(NumberOfParameters == 2){
				I->Arena.CenterX = (float)atof(Parameters[0]) + 0.5 ; // account for the half pixel
				I->Arena.CenterY = (float)atof(Parameters[1]) + 0.5 ;
				I->Arena.RadiusPix = (float)(int)((I->Arena.CenterX + I->Arena.CenterY)/2.0 + 0.5);
			}else{
				fprintf(MyStdOut,"Must only be 2 values for %s\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
				return(0);
			}

			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
			return(0);
		}

		// ArenaDiameter_m
		if(!FindKeyWord(fp, SectionStart, ARENA_DIAMETER_M_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
			return 0;
		}

		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded

			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);

			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
				return 0;
			}
			if(NumberOfParameters == 1){
				// Value in header is in m convert to cm
				I->Arena.Radius = (float)(atof(Parameters[0]) * 100.0 / 2.0);
				if(I->Arena.Radius <= 0.0) { // user did not give dimensions of arena
					fprintf(MyStdOut,"%c\nTHE DATA FILE HEADER INDICATES THE ARENA DIAMETER IS <= 0 m\n", 7);
					fprintf(MyStdOut,"%cSOMEONE NEGLECTED TO ENTER A PROPER VALUE WHEN THE EXPERIMENT WAS CONFIGURED\n", 7);
					fprintf(MyStdOut,"%cNO DISTANCE MEASURES CAN BE CALCULATED UNLESS THIS VALUE IS DEFINED\n", 7);
					fprintf(MyStdOut,"%cFOR FURTHER PROCESSING THE VALUE IS ASSUMED TO BE 0.82 m\n", 7);
					fprintf(MyStdOut,"The PixelsPerCM value is also unlikely to be correct.\n");
					fprintf(MyStdOut,"%cYou should repair these values in the file headers and configure the experiment properly\n", 7);
					I->Arena.Radius =  0.82 / 2.0;
				}
					
			}else{
				fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
			return(0);
		}

		// Trials label. If data are organized as trials then each will have a %Start.0 label
		// Older versions of iTrack had a %Start for the label so repeat this search for the old version and assume ASCII encoding
		if(FindKeyWord(fp, SectionStart, TRIALS_START_LABEL, KeyWordString, Encoding)){
			// this is an optional field of the Header
			Type = GetType(KeyWordString);
			switch(Type){
			case -1:
				fprintf(MyStdOut,"\nKeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
				return(-1);

			case 0:	// ASCII encoded
				NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
				if(!NumberOfParameters){
					fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRIALS_START_LABEL);
					return(0);
				}
				if(NumberOfParameters == 3){
					I->TrialNumber = atoi(Parameters[0]);
					I->TrialSubjectId = atoi(Parameters[1]);
					strcpy(I->TrialStartLabel, Parameters[2]);
				}else{
					fprintf(MyStdOut,"Must only be 3 values for %s\nWrong File Format.\n", TRIALS_START_LABEL);
					return(0);
				}
				break;

			default:
				fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", TRIALS_START_LABEL);
				return(0);
			}
		}else if(FindKeyWord(fp, SectionStart, TRIALS_OLD_START_LABEL, KeyWordString, Encoding)){
			char temp[256];
			Type = 0; // assume ASCII encoding because GetType(KeyWordString) will return -1;
			strcpy(temp,Encoding);
			sprintf(Encoding,"(%s", temp); // do this because the leading '(' is missing in the Encoding

			switch(Type){
			case -1:
				fprintf(MyStdOut,"\nKeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
				return(-1);

			case 0:	// ASCII encoded
				NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
				if(!NumberOfParameters){
					fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRIALS_OLD_START_LABEL);
					return(0);
				}
				if(NumberOfParameters == 3){
					I->TrialNumber = atoi(Parameters[0]);
					I->TrialSubjectId = atoi(Parameters[1]);
					strcpy(I->TrialStartLabel, Parameters[2]);
				}else{
					fprintf(MyStdOut,"Must only be 3 values for %s\nWrong File Format.\n", TRIALS_OLD_START_LABEL);
					return(0);
				}
				break;

			default:
				fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", TRIALS_OLD_START_LABEL);
				return(0);
			}
		}
		
		switch (I->ParadigmType){
			case TRACKER:
			case OPEN_FIELD:
			// Reinforced Sector
				// define a sector as the E quadrant so quadrant analysis can be performed
				// note that this overrides and sector info in the header
				I->Target[0].CoordFrame = 0;
				Frame = 0;
				I->Target[0].CoordFrame = 0;
				I->Target[Frame].Sector.Ang = 0.0;
				I->Target[Frame].Sector.Width = 90.0;
				I->Target[Frame].Sector.InRad = 0.0;
				I->Target[Frame].Sector.OutRad = (float)(DEFAULT_ARENA_RADIUS - 1); //default is 128 and max is 1 pixel less
				I->Target[Frame].Show =	0;
				break; // TRACKER and OPEN_FIELD

			case PLACE_AVOIDANCE:
			// Reinforced Sector
				if(!FindKeyWord(fp,	SectionStart, REINFORCED_SECTOR_LABEL, KeyWordString, Encoding)){
					// if the keyword is missing assume that there was no reinforced area in this frame and continue
					// fprintf(MyStdOut,"Can't	find keyWord (%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
					I->Target[0].CoordFrame = 0;
					Frame = 0;

					I->Target[Frame].Sector.Ang = 0.0;
					I->Target[Frame].Sector.Width = 0.0;
					I->Target[Frame].Sector.InRad = 0.0;
					I->Target[Frame].Sector.OutRad = 0.0;

					I->Target[Frame].Circle.X = 0.0;
					I->Target[Frame].Circle.Y = 0.0;
					I->Target[Frame].Circle.Rad = 0.0; 

					I->ReinforcedAreaDefined = 0;
                                        I->Target[Frame].Type = REINFORCED_SECTOR; // 23.08.13 added this because it triggers the allocation of ReinforcedSector, which is used subsequently even if not positions are defined  
					ReinforcedArea	= 0;
					break;
				}
				Type = GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
						return(0);

					case 0:	// ASCII encoded
						NumberOfParameters = GetASCIIEncoding(Encoding,	&Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File	Format.\n",	REINFORCED_SECTOR_LABEL);
							return(0);
						}
						if(NumberOfParameters == 4){	// ANNULUS SECTOR TYPE
							I->Target[Frame].Sector.Ang	= (float)atof(Parameters[0]);
							I->Target[Frame].Sector.Width =	(float)atof(Parameters[1]);
							I->Target[Frame].Sector.InRad =	(float)atof(Parameters[2]);
							I->Target[Frame].Sector.OutRad = (float)atof(Parameters[3]);

							if((I->Target[Frame].Sector.Ang) < 0.0)
								I->Target[Frame].Sector.Ang	+= 360.0;
							if(I->Target[Frame].Sector.Width !=0.0)
								I->Target[Frame].Show =	1;
							else
								I->Target[Frame].Show =	0;
							if(I->Target[Frame].Sector.OutRad == 0.0)
								I->Target[Frame].Show =	0;

							
							I->Target[Frame].Type = REINFORCED_SECTOR; // 23.08.13 changed I->Target[Frame].Type from I->Target[0].Type
							I->ReinforcedAreaDefined = 1;
							ReinforcedArea	= 1;
							// Tracker v2.10 and lower reports the Radius data in % of the arena radius but TrackAnalysis calculates in pixels, so convert to Pixels
							// A separate bug caused some Tracker files to be designated iTrack in the header
							if(((I->FileFormat == TRACKER_FORMAT) || (I->FileFormat == ITRACK_FORMAT)) && (I->FileFormatVersion <= 2.1)){
								I->Target[Frame].Sector.InRad *= (I->Arena.RadiusPix / 100.0); 
								I->Target[Frame].Sector.OutRad *= (I->Arena.RadiusPix / 100.0);
							}
						}else if (NumberOfParameters == 3){	// CIRCLE TYPE
							I->Target[Frame].Circle.X	= (float)atof(Parameters[0]);
							I->Target[Frame].Circle.Y=	(float)atof(Parameters[1]);
							I->Target[Frame].Circle.Rad =	(float)atof(Parameters[2]);
							I->Target[0].Type = REINFORCED_CIRCLE;
							I->ReinforcedAreaDefined = 1;
							ReinforcedArea = 1;
							I->Target[Frame].Show =	1;
						}else{
							fprintf(MyStdOut,"Must be 3 values for a circle or 4 values for an annulus-sector  type of %s\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
							return(0);
						}
						break;

					default:
						fprintf(MyStdOut,"%s	must be	ASCII encoded\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
						return(0);
				}

				//	Shock Parameters
				if(!FindKeyWord(fp, SectionStart, SHOCK_PARAMETERS_LABEL, KeyWordString, Encoding)){
					fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File	Format.\n",	SHOCK_PARAMETERS_LABEL);
					return	(0);
				}
				Type =	GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format	not	respected in: %s\nWrong	File Format.\n", KeyWordString);
						return (0);

					case 0:	// ASCII encoded
						NumberOfParameters =	GetASCIIEncoding(Encoding, &Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
							return (0);
						}

						if(NumberOfParameters >=	3){
							I->Shock.Latency	= atoi(Parameters[0]);
							I->Shock.Duration = atoi(Parameters[1]);
							I->Shock.MinISI = atoi(Parameters[2]);

							if(NumberOfParameters ==	4)
								I->Shock.ResetInterval =	atoi(Parameters[3]);
							else
								I->Shock.ResetInterval =	RESET_INTERVAL_DEFAULT;

						}else{
							fprintf(MyStdOut,"Must be 3 or 4	values for %s\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
							return (0);
						}
						if((I->Date.Year	<= 2001) &&	(I->Date.Month < 5)){
							I->Shock.CalculationMethod = TIME_BASED;
						}
						break;

					default:
						fprintf(MyStdOut,"%s	must be	ASCII encoded\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
						return (0);
				}
			break;	// PLACE_AVOIDANCE

			case PLACE_PREFERENCE:
				// Reinforced Place.
				if(!FindKeyWord(fp, SectionStart, REINFORCED_SECTOR_LABEL, KeyWordString, Encoding)){
					fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
					return 0;
				}
				I->Target[0].CoordFrame = 0;

				Type = GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
						return(0);

					case 0: // ASCII encoded
						NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
							return(0);
						}
						if(NumberOfParameters == 3){
							I->Target[Frame].Circle.X = (float)atof(Parameters[0]);
							I->Target[Frame].Circle.Y = (float)atof(Parameters[1]);
							I->Target[Frame].Circle.Rad = (float)atof(Parameters[2]);
							if((I->Target[Frame].Circle.Rad) > 0.0)
								I->Target[Frame].Show = 1;
							else
								I->Target[Frame].Show = 0;
						}else{
							fprintf(MyStdOut,"Must be 3 values for %s\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
							return(0);
						}
						break;

					default:
						fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
						return(0);
				}
				// This is the only type that has been used
				I->Target[0].Type = REINFORCED_CIRCLE;
				ReinforcedArea = 1;

				/* Not currently used
				// Feeder Mode
				if(!FindKeyWord(fp, SectionStart, FEEDER_MODE_LABEL, KeyWordString, Encoding)){
					fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", FEEDER_MODE_LABEL);
					return (0);
				}
				Type = GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
						return (0);

					case 0: // ASCII encoded
						NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File Format.\n", FEEDER_MODE_LABEL);
							return (0);
						}else if(NumberOfParameters == 1){
							if(!strcmp(Parameters[0], POSITION_STRING)){
								I->Feeder[0].Mode = POSITION_MODE;
							}else if(!strcmp(Parameters[0], TIME_STRING)){
								I->Feeder[0].Mode = TIME_MODE;
							}
						}else{
							fprintf(MyStdOut,"%s must be either %s or %s\nWrong File Format.\n", FEEDER_MODE_LABEL, POSITION_STRING, TIME_STRING);
							return (0);
						}
						break;

					default:
						fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", FEEDER_MODE_LABEL);
						return (0);
						break;
				}
				*/

				// FeederParameters
				if(!FindKeyWord(fp, SectionStart, FEEDER_PARAMETERS_LABEL, KeyWordString, Encoding)){
					fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
					return (0);
				}
				Type = GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
						return (0);

					case 0: // ASCII encoded
						NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
							return (0);
						}

						if(NumberOfParameters == 2){
							I->Feeder[Frame].Trigger = (float)atof(Parameters[0]);
							I->Feeder[Frame].Refractory = (float)atof(Parameters[1]);
						}else{
							fprintf(MyStdOut,"Must be 2 values for %s\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
							return (0);
						}
						break;

					default:
						fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
						return (0);
				}

				break; // PLACE_PREFERENCE
			default:
				fprintf(MyStdOut,"TrackAnalysis is not prepared for this paradigm (%s)\n", I->Paradigm);
				return(0);
			break;
		}

		// Calculate Arena boundaries in pixels
		ArenaRadius_pix = (int)(I->Arena.Radius * I->PixelsPerCM);
		I->Arena.Bottom = I->Arena.CenterY + ArenaRadius_pix;
		if(I->Arena.Bottom > 255)
			I->Arena.Bottom = 255;
		I->Arena.Right = I->Arena.CenterX + ArenaRadius_pix;
		if(I->Arena.Right > 255)
			I->Arena.Right = 255;
		I->Arena.Top = I->Arena.CenterY - ArenaRadius_pix;
		if(I->Arena.Top < 0)
			I->Arena.Top = 0;
		I->Arena.Left = I->Arena.CenterX - ArenaRadius_pix;
		if(I->Arena.Left < 0)
			I->Arena.Left = 0;
	}else{
		fprintf(MyStdOut, "\nCan't find Header section %s.\nWrong File Format.\n", "SETUP_INFORMATION");
		return (0);
	}

	// Section: RECORD_FORMAT
	SectionStart = FindSection(fp, HeaderStart, "RECORD_FORMAT");
	if(Offset){ 
		// Do stuff ...
		// Sample
		if(!FindKeyWord(fp, SectionStart, SAMPLE_FORMAT_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", SAMPLE_FORMAT_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return 0;

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"%s\nWrong File Format.\n", SAMPLE_FORMAT_LABEL);
				return 0;
			}
			I->FileParameters = NumberOfParameters;
			GetParameterIndex(Parameters, NumberOfParameters, PI);	
		}

		Offset = FindSectionEnd(fp);
		if(!Offset)
			fprintf(MyStdOut,"\nCan't find end of Header Section (%s).\nWrong File Format.\n", "RECORD_FORMAT");
	}

	if(!(HeaderEnd = FindHeaderEnd(fp))){
		fprintf(MyStdOut, "\nCan't find end of header. Wrong data format\n");
		return (0);
	}
	return(HeaderEnd);
}

long	ReadHeaderITrack(FILE *fp, INFO *I, long HeaderStart, PARAMETER_INDEX *PI)
{
	long	Offset, HeaderEnd, SectionStart;
	char	KeyWordString[256], Encoding[512], **Parameters;
	int		Type, NumberOfParameters, Frame, ReinforcedArea = 0;
	int		ArenaRadius_pix;


	// Section: DATABASE_INFORMATION
	SectionStart = FindSection(fp, HeaderStart, "DATABASE_INFORMATION");
	if(SectionStart){
		// Date
		if(!FindKeyWord(fp, SectionStart, DATE_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", DATE_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", DATE_LABEL);
				return 0;
			}
			if(NumberOfParameters == 1){
				sscanf(Parameters[0],"%d.%d.%d", &(I->Date.Day), &(I->Date.Month), &(I->Date.Year));
			}else if(NumberOfParameters == 3){
				sscanf(Parameters[0],"%d. %d. %d", &(I->Date.Day), &(I->Date.Month), &(I->Date.Year));
			}else{
				fprintf(MyStdOut,"%s format must be dd.mm.yyyy\nWrong File Format.\n", DATE_LABEL);
				return(0);
			}
			if((I->Date.Year > 2001) || ((I->Date.Year >=2001) && (I->Date.Month >= 5))){
				I->Shock.CalculationMethod = STATE_BASED; // SAMPLE_BASED;
				// I->Shock.CalculationMethod = SAMPLE_BASED; // STATE_BASED;
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", DATE_LABEL);
			return(0);
		}

		Offset = FindSectionEnd(fp);
		if(!Offset)
			fprintf(MyStdOut,"\nCan't find end of Header Section (%s).\nWrong File Format.\n", "DATA_BASE_INFORMATION");
	}

	// Section: SETUP_INFORMATION
	SectionStart = FindSection(fp, HeaderStart, "SETUP_INFORMATION");
	if(SectionStart){
		// Do stuff ...

		// Paradigm
		if(I->ParadigmType == OPEN_FIELD){	// this is determined by an option and thus the Header info should be ignored
		}else{
			if(!FindKeyWord(fp, SectionStart, PARADIGM_LABEL, KeyWordString, Encoding)){
				// if paradigm is not specified then assume it is TRACKER which is basic
				// fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", PARADIGM_LABEL);
				strcpy(I->Paradigm, TRACKER_STRING);
                     	  	 I->ParadigmType = TRACKER;
				// return 0;
			}else{
				Type = GetType(KeyWordString);
				switch(Type){
				case -1:
					fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
					return(0);

				case 0:	// ASCII encoded
					NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
					if(!NumberOfParameters){
						fprintf(MyStdOut,"(%s).\nWrong File Format.\n", PARADIGM_LABEL);
						return 0;
					}
					if(NumberOfParameters == 1){
						strcpy(I->Paradigm, Parameters[0]);
						if(!strcmp(I->Paradigm, PLACE_AVOIDANCE_STRING))
       	                		                I->ParadigmType = PLACE_AVOIDANCE;
       	                		        else if(!strcmp(I->Paradigm, PLACE_PREFERENCE_STRING))
       	                		                I->ParadigmType = PLACE_PREFERENCE;
       	                		        else if(!strcmp(I->Paradigm, TRACKER_STRING))
       		                 			I->ParadigmType = TRACKER;
       	                		        else if(!strcmp(I->Paradigm, OPEN_FIELD_STRING))
       		                 			I->ParadigmType = OPEN_FIELD;
						else{
       	                		                fprintf(MyStdOut,"TrackAnalysis is not prepared for this paradigm (%s)\n", I->Paradigm);
       	                		                return(0);
       	                		         }
					}else{
						fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", PARADIGM_LABEL);
						return(0);
					}
					break;
	
				default:
					fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", PARADIGM_LABEL);
					return(0);
				}
			}
		} // end of dealing with if optional paradigm is openfield

		// Tracker resolution PixelsPer cm
		if(!FindKeyWord(fp, SectionStart, TRACKER_RESOLUTION_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(-1);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
				return(0);
			}
			if(NumberOfParameters == 1){
				I->PixelsPerCM = atof(Parameters[0]);
			}else{
				fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
			return(0);
		}
					
		// Frame
		if(!FindKeyWord(fp, SectionStart, FRAME_LABEL, KeyWordString, Encoding)){
			I->NumberOfCoordFrames = 1;
                        Frame = 0;
                        I->Target[Frame].CoordFrame = SINGLE_FRAME;

			// fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", FRAME_LABEL);
			// return 0;
		}else{
			Type = GetType(KeyWordString);
			switch(Type){
			case -1:
				fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
				return(0);

			case 0:	// ASCII encoded
				NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
				if(!NumberOfParameters){
					fprintf(MyStdOut,"(%s).\nWrong File Format.\n", FRAME_LABEL);
					return 0;
				}
				if(NumberOfParameters == 1){
					strcpy(I->Frame, Parameters[0]);
					I->NumberOfCoordFrames = 1;
					Frame = SINGLE_FRAME;
					I->Target[Frame].CoordFrame = SINGLE_FRAME;
				}else{
					fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", FRAME_LABEL);
					return(0);
				}
				break;

			default:
				fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", FRAME_LABEL);
				return(0);
			}
		}

		// ArenaCenterXY
		if(!FindKeyWord(fp, SectionStart, ARENA_CENTER_XY_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
				return 0;
			}
			if(NumberOfParameters == 2){
				I->Arena.CenterX = (float)atof(Parameters[0]) + 0.5 ; // account for the half pixel
				I->Arena.CenterY = (float)atof(Parameters[1]) + 0.5 ;
			}else{
				fprintf(MyStdOut,"Must only be 2 values for %s\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
				return(0);
			}

			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
			return(0);
		}

		// ArenaDiameter_m
		if(!FindKeyWord(fp, SectionStart, ARENA_DIAMETER_M_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
			return 0;
		}

		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded

			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);

			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
				return 0;
			}
			if(NumberOfParameters == 1){
				// Value in header is in m convert to cm
				I->Arena.Radius = (float)(atof(Parameters[0]) * 100.0 / 2.0);
				if(I->Arena.Radius <= 0.0) { // user did not give dimensions of arena
					fprintf(MyStdOut,"%c\nTHE DATA FILE HEADER INDICATES THE ARENA DIAMETER IS <= 0 m\n", 7);
					fprintf(MyStdOut,"%cSOMEONE NEGLECTED TO ENTER A PROPER VALUE WHEN THE EXPERIMENT WAS CONFIGURED\n", 7);
					fprintf(MyStdOut,"%cNO DISTANCE MEASURES CAN BE CALCULATED UNLESS THIS VALUE IS DEFINED\n", 7);
					fprintf(MyStdOut,"%cFOR FURTHER PROCESSING THE VALUE IS ASSUMED TO BE 0.82 m\n", 7);
					fprintf(MyStdOut,"The PixelsPerCM value is also unlikely to be correct.\n");
					fprintf(MyStdOut,"%cYou should repair these values in the file headers and configure the experiment properly\n", 7);
					I->Arena.Radius =  0.82 / 2.0;
				}
					
			}else{
				fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
			return(0);
		}

		// Trials label. If data are organized as trials then each will have a %Start.0 label
		// Older versions of iTrack had a %Start for the label so repeat this search for the old version and assume ASCII encoding
		if(FindKeyWord(fp, SectionStart, TRIALS_START_LABEL, KeyWordString, Encoding)){
			// this is an optional field of the Header
			Type = GetType(KeyWordString);
			switch(Type){
			case -1:
				fprintf(MyStdOut,"\nKeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
				return(-1);

			case 0:	// ASCII encoded
				NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
				if(!NumberOfParameters){
					fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRIALS_START_LABEL);
					return(0);
				}
				if(NumberOfParameters == 3){
					I->TrialNumber = atoi(Parameters[0]);
					I->TrialSubjectId = atoi(Parameters[1]);
					strcpy(I->TrialStartLabel, Parameters[2]);
				}else{
					fprintf(MyStdOut,"Must only be 3 values for %s\nWrong File Format.\n", TRIALS_START_LABEL);
					return(0);
				}
				break;

			default:
				fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", TRIALS_START_LABEL);
				return(0);
			}
		}else if(FindKeyWord(fp, SectionStart, TRIALS_OLD_START_LABEL, KeyWordString, Encoding)){
			char temp[256];
			Type = 0; // assume ASCII encoding because GetType(KeyWordString) will return -1;
			strcpy(temp,Encoding);
			sprintf(Encoding,"(%s", temp); // do this because the leading '(' is missing in the Encoding

			switch(Type){
			case -1:
				fprintf(MyStdOut,"\nKeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
				return(-1);

			case 0:	// ASCII encoded
				NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
				if(!NumberOfParameters){
					fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRIALS_OLD_START_LABEL);
					return(0);
				}
				if(NumberOfParameters == 3){
					I->TrialNumber = atoi(Parameters[0]);
					I->TrialSubjectId = atoi(Parameters[1]);
					strcpy(I->TrialStartLabel, Parameters[2]);
				}else{
					fprintf(MyStdOut,"Must only be 3 values for %s\nWrong File Format.\n", TRIALS_OLD_START_LABEL);
					return(0);
				}
				break;

			default:
				fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", TRIALS_OLD_START_LABEL);
				return(0);
			}
		}
		
		switch (I->ParadigmType){
			case TRACKER:
			case OPEN_FIELD:
			// Reinforced Sector
				// define a sector as the E quadrant so quadrant analysis can be performed
				// note that this overrides and sectort info in the header
				I->Target[0].CoordFrame = 0;
				Frame = 0;
				I->Target[0].CoordFrame = 0;
				I->Target[Frame].Sector.Ang = 0.0;
				I->Target[Frame].Sector.Width = 90.0;
				I->Target[Frame].Sector.InRad = 0.0;
				I->Target[Frame].Sector.OutRad = (float)(DEFAULT_ARENA_RADIUS - 1); //default is 128 and max is 1 pixel less
				I->Target[Frame].Show =	0;
				break; // TRACKER and OPEN_FIELD

			case PLACE_AVOIDANCE:
			// Reinforced Sector
				if(!FindKeyWord(fp,	SectionStart, REINFORCED_SECTOR_LABEL, KeyWordString, Encoding)){
					// if the keyword is missing assume that there was no reinforced area in this frame and continue
					//fprintf(MyStdOut,"Can't	find keyWord (%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
					I->Target[0].CoordFrame = 0;
					Frame = 0;

					I->Target[Frame].Sector.Ang = 0.0;
					I->Target[Frame].Sector.Width = 0.0;
					I->Target[Frame].Sector.InRad = 0.0;
					I->Target[Frame].Sector.OutRad = 0.0;

					I->Target[Frame].Circle.X = 0.0;
					I->Target[Frame].Circle.Y = 0.0;
					I->Target[Frame].Circle.Rad = 0.0; 

					I->ReinforcedAreaDefined = 0;
                                        I->Target[Frame].Type = REINFORCED_SECTOR; // 23.08.13 added this because it triggers the allocation of ReinforcedSector, which is used subsequently even if not positions are defined  
					ReinforcedArea	= 0;
					break;
				}
				Type = GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
						return(0);

					case 0:	// ASCII encoded
						NumberOfParameters = GetASCIIEncoding(Encoding,	&Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File	Format.\n",	REINFORCED_SECTOR_LABEL);
							return(0);
						}
						if(NumberOfParameters == 4){	// ANNULUS SECTOR TYPE
							I->Target[Frame].Sector.Ang	= (float)atof(Parameters[0]);
							I->Target[Frame].Sector.Width =	(float)atof(Parameters[1]);
							I->Target[Frame].Sector.InRad =	(float)atof(Parameters[2]);
							I->Target[Frame].Sector.OutRad = (float)atof(Parameters[3]);

							if((I->Target[Frame].Sector.Ang) < 0.0)
								I->Target[Frame].Sector.Ang	+= 360.0;
							if(I->Target[Frame].Sector.Width !=0.0)
								I->Target[Frame].Show =	1;
							else
								I->Target[Frame].Show =	0;
							if(I->Target[Frame].Sector.OutRad == 0.0)
								I->Target[Frame].Show =	0;

							I->Target[Frame].Type = REINFORCED_SECTOR; // 23.08.13 changed I->Target[Frame].Type from I->Target[0].Type
							I->ReinforcedAreaDefined = 1;
							ReinforcedArea	= 1;
						}else if (NumberOfParameters == 3){	// CIRCLE TYPE
							I->Target[Frame].Circle.X	= (float)atof(Parameters[0]);
							I->Target[Frame].Circle.Y=	(float)atof(Parameters[1]);
							I->Target[Frame].Circle.Rad =	(float)atof(Parameters[2]);
							I->Target[0].Type = REINFORCED_CIRCLE;
							I->ReinforcedAreaDefined = 1;
							ReinforcedArea = 1;
							I->Target[Frame].Show =	1;
						}else{
							fprintf(MyStdOut,"Must be 3 values for a circle or 4 values for an annulus-sector  type of %s\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
							return(0);
						}
						break;

					default:
						fprintf(MyStdOut,"%s	must be	ASCII encoded\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
						return(0);
				}

				//	Shock Parameters
				if(!FindKeyWord(fp, SectionStart, SHOCK_PARAMETERS_LABEL, KeyWordString, Encoding)){
					fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File	Format.\n",	SHOCK_PARAMETERS_LABEL);
					return	(0);
				}
				Type =	GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format	not	respected in: %s\nWrong	File Format.\n", KeyWordString);
						return (0);

					case 0:	// ASCII encoded
						NumberOfParameters =	GetASCIIEncoding(Encoding, &Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
							return (0);
						}

						if(NumberOfParameters >=	3){
							I->Shock.Latency	= atoi(Parameters[0]);
							I->Shock.Duration = atoi(Parameters[1]);
							I->Shock.MinISI = atoi(Parameters[2]);

							if(NumberOfParameters ==	4)
								I->Shock.ResetInterval =	atoi(Parameters[3]);
							else
								I->Shock.ResetInterval =	RESET_INTERVAL_DEFAULT;

						}else{
							fprintf(MyStdOut,"Must be 3 or 4	values for %s\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
							return (0);
						}
						if((I->Date.Year	<= 2001) &&	(I->Date.Month < 5)){
							I->Shock.CalculationMethod = TIME_BASED;
						}
						break;

					default:
						fprintf(MyStdOut,"%s	must be	ASCII encoded\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
						return (0);
				}
			break;	// PLACE_AVOIDANCE

			case PLACE_PREFERENCE:
				// Reinforced Place.
				if(!FindKeyWord(fp, SectionStart, REINFORCED_SECTOR_LABEL, KeyWordString, Encoding)){
					fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
					return 0;
				}
				I->Target[0].CoordFrame = 0;

				Type = GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
						return(0);

					case 0: // ASCII encoded
						NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
							return(0);
						}
						if(NumberOfParameters == 3){
							I->Target[Frame].Circle.X = (float)atof(Parameters[0]);
							I->Target[Frame].Circle.Y = (float)atof(Parameters[1]);
							I->Target[Frame].Circle.Rad = (float)atof(Parameters[2]);
							if((I->Target[Frame].Circle.Rad) > 0.0)
								I->Target[Frame].Show = 1;
							else
								I->Target[Frame].Show = 0;
						}else{
							fprintf(MyStdOut,"Must be 3 values for %s\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
							return(0);
						}
						break;

					default:
						fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
						return(0);
				}
				// This is the only type that has been used
				I->Target[0].Type = REINFORCED_CIRCLE;
				ReinforcedArea = 1;

				/* Not currently used
				// Feeder Mode
				if(!FindKeyWord(fp, SectionStart, FEEDER_MODE_LABEL, KeyWordString, Encoding)){
					fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", FEEDER_MODE_LABEL);
					return (0);
				}
				Type = GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
						return (0);

					case 0: // ASCII encoded
						NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File Format.\n", FEEDER_MODE_LABEL);
							return (0);
						}else if(NumberOfParameters == 1){
							if(!strcmp(Parameters[0], POSITION_STRING)){
								I->Feeder[0].Mode = POSITION_MODE;
							}else if(!strcmp(Parameters[0], TIME_STRING)){
								I->Feeder[0].Mode = TIME_MODE;
							}
						}else{
							fprintf(MyStdOut,"%s must be either %s or %s\nWrong File Format.\n", FEEDER_MODE_LABEL, POSITION_STRING, TIME_STRING);
							return (0);
						}
						break;

					default:
						fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", FEEDER_MODE_LABEL);
						return (0);
						break;
				}
				*/

				// FeederParameters
				if(!FindKeyWord(fp, SectionStart, FEEDER_PARAMETERS_LABEL, KeyWordString, Encoding)){
					fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
					return (0);
				}
				Type = GetType(KeyWordString);
				switch(Type){
					case -1:
						fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
						return (0);

					case 0: // ASCII encoded
						NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
						if(!NumberOfParameters){
							fprintf(MyStdOut,"(%s).\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
							return (0);
						}

						if(NumberOfParameters == 2){
							I->Feeder[Frame].Trigger = (float)atof(Parameters[0]);
							I->Feeder[Frame].Refractory = (float)atof(Parameters[1]);
						}else{
							fprintf(MyStdOut,"Must be 2 values for %s\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
							return (0);
						}
						break;

					default:
						fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
						return (0);
				}

				break; // PLACE_PREFERENCE
			default:
				fprintf(MyStdOut,"TrackAnalysis is not prepared for this paradigm (%s)\n", I->Paradigm);
				return(0);
			break;
		}


		// Calculate Arena boundaries in pixels
		ArenaRadius_pix = (int)(I->Arena.Radius * I->PixelsPerCM);
		I->Arena.Bottom = I->Arena.CenterY + ArenaRadius_pix;
		if(I->Arena.Bottom > 255)
			I->Arena.Bottom = 255;
		I->Arena.Right = I->Arena.CenterX + ArenaRadius_pix;
		if(I->Arena.Right > 255)
			I->Arena.Right = 255;
		I->Arena.Top = I->Arena.CenterY - ArenaRadius_pix;
		if(I->Arena.Top < 0)
			I->Arena.Top = 0;
		I->Arena.Left = I->Arena.CenterX - ArenaRadius_pix;
		if(I->Arena.Left < 0)
			I->Arena.Left = 0;
	}else{
		fprintf(MyStdOut, "\nCan't find Header section %s.\nWrong File Format.\n", "SETUP_INFORMATION");
		return (0);
	}

	// Section: RECORD_FORMAT
	SectionStart = FindSection(fp, HeaderStart, "RECORD_FORMAT");
	if(Offset){ 
		// Do stuff ...
		// Sample
		if(!FindKeyWord(fp, SectionStart, SAMPLE_FORMAT_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", SAMPLE_FORMAT_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return 0;

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"%s\nWrong File Format.\n", SAMPLE_FORMAT_LABEL);
				return 0;
			}
			I->FileParameters = NumberOfParameters;
			GetParameterIndex(Parameters, NumberOfParameters, PI);	
		}

		Offset = FindSectionEnd(fp);
		if(!Offset)
			fprintf(MyStdOut,"\nCan't find end of Header Section (%s).\nWrong File Format.\n", "RECORD_FORMAT");
	}

	if(!(HeaderEnd = FindHeaderEnd(fp))){
		fprintf(MyStdOut, "\nCan't find end of header. Wrong data format\n");
		return (0);
	}
	return(HeaderEnd);
}

long	ReadHeaderFGX(FILE *fp, INFO *I, long HeaderStart, PARAMETER_INDEX *PI)
{
	long	Offset, HeaderEnd, SectionStart;
	char	KeyWordString[256], Encoding[512], **Parameters;
	int		Type, NumberOfParameters, Frame, ReinforcedArea = 0;
	int		ArenaRadius_pix;

	// Section: DATABASE_INFORMATION
	SectionStart = FindSection(fp, HeaderStart, "DATABASE_INFORMATION");
	if(SectionStart){
		// Date
		if(!FindKeyWord(fp, SectionStart, DATE_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", DATE_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", DATE_LABEL);
				return 0;
			}
			if(NumberOfParameters == 1){
				sscanf(Parameters[0],"%d.%d.%d", &(I->Date.Day), &(I->Date.Month), &(I->Date.Year));
			}else if(NumberOfParameters == 3){
				sscanf(Parameters[0],"%d. %d. %d", &(I->Date.Day), &(I->Date.Month), &(I->Date.Year));
			}else{
				fprintf(MyStdOut,"%s format must be dd.mm.yyyy\nWrong File Format.\n", DATE_LABEL);
				return(0);
			}
			if((I->Date.Year > 2001) || ((I->Date.Year >=2001) && (I->Date.Month >= 5))){
				I->Shock.CalculationMethod = SAMPLE_BASED; // STATE_BASED;
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", DATE_LABEL);
			return(0);
		}

		Offset = FindSectionEnd(fp);
		if(!Offset)
			fprintf(MyStdOut,"\nCan't find end of Header Section (%s).\nWrong File Format.\n", "DATA_BASE_INFORMATION");
	}

	// Section: SETUP_INFORMATION
	SectionStart = FindSection(fp, HeaderStart, "SETUP_INFORMATION");
	if(SectionStart){
		// Do stuff ...

		// Paradigm
		if(!FindKeyWord(fp, SectionStart, PARADIGM_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", PARADIGM_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", PARADIGM_LABEL);
				return 0;
			}
			if(NumberOfParameters == 1){
				strcpy(I->Paradigm, Parameters[0]);
				if(!strcmp(I->Paradigm, PLACE_AVOIDANCE_STRING))
                                       I->ParadigmType = PLACE_AVOIDANCE;
                                else if(!strcmp(I->Paradigm, PLACE_PREFERENCE_STRING))
                                       I->ParadigmType = PLACE_PREFERENCE;
                                else {
                                       fprintf(MyStdOut,"TrackAnalysis is not prepared for this paradigm (%s)\n", I->Paradigm);
                                       return(0);
                                }
			}else{
				fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", PARADIGM_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", PARADIGM_LABEL);
			return(0);
		}

		// Tracker resolution PixelsPer cm
		if(!FindKeyWord(fp, SectionStart, TRACKER_RESOLUTION_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(-1);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
				return(0);
			}
			if(NumberOfParameters == 1)
				I->PixelsPerCM = atof(Parameters[0]);
			else{
				fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
			return(0);
		}
					
		// Frame
		if(!FindKeyWord(fp, SectionStart, FRAME_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", FRAME_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", FRAME_LABEL);
				return 0;
			}
			if(NumberOfParameters == 1){
				strcpy(I->Frame, Parameters[0]);
				I->NumberOfCoordFrames = 1;
				Frame = SINGLE_FRAME;
				I->Target[Frame].CoordFrame = SINGLE_FRAME;
			}else{
				fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", FRAME_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", FRAME_LABEL);
			return(0);
		}

		// ArenaCenterXY
		if(!FindKeyWord(fp, SectionStart, ARENA_CENTER_XY_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
				return 0;
			}
			if(NumberOfParameters == 2){
				I->Arena.CenterX = (float)atof(Parameters[0]) + 0.5;
				I->Arena.CenterY = (float)atof(Parameters[1]) + 0.5;
			}else{
				fprintf(MyStdOut,"Must only be 2 values for %s\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
				return(0);
			}

			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", ARENA_CENTER_XY_LABEL);
			return(0);
		}

		// ArenaDiameter_m
		if(!FindKeyWord(fp, SectionStart, ARENA_DIAMETER_M_LABEL, KeyWordString, Encoding)){
			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
			return 0;
		}
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
				return 0;
			}
			if(NumberOfParameters == 1){
				// Value in header is in m convert to cm
				I->Arena.Radius = (float)(atof(Parameters[0]) * 100.0 / 2.0);
			}else{
				fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
				return(0);
			}

			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", ARENA_DIAMETER_M_LABEL);
			return(0);
		}
		
                       switch (I->ParadigmType){
                       case PLACE_AVOIDANCE:
                               // Reinforced Sector.
                               if(!FindKeyWord(fp, SectionStart, REINFORCED_SECTOR_LABEL, KeyWordString, Encoding)){
                                       fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
                                       return 0;
                               }
                               I->Target[0].CoordFrame = 0;


                               Type = GetType(KeyWordString);
                               switch(Type){
                               case -1:
                                       fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
                                       return(0);

                               case 0: // ASCII encoded
                                       NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
                                       if(!NumberOfParameters){
                                               fprintf(MyStdOut,"(%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
                                               return(0);
                                       }
                                       if(NumberOfParameters == 4){
                                               I->Target[Frame].Sector.Ang = (float)atof(Parameters[0]);
                                               I->Target[Frame].Sector.Width = (float)atof(Parameters[1]);
                                               I->Target[Frame].Sector.InRad = (float)atof(Parameters[2]);
                                               I->Target[Frame].Sector.OutRad = (float)atof(Parameters[3]);

                                               if((I->Target[Frame].Sector.Ang) < 0.0)
                                                       I->Target[Frame].Sector.Ang += 360.0;
                                               if(I->Target[Frame].Sector.Width != 0.0)
                                                       I->Target[Frame].Show = 1;
                                               else
                                                       I->Target[Frame].Show = 0;

                                               if(I->Target[Frame].Sector.OutRad == 0.0)
                                                       I->Target[Frame].Show = 0;


                                       }else{
                                               fprintf(MyStdOut,"Must be 4 values for %s\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
                                               return(0);
                                       }
                                       break;

                               default:
                                       fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
                                       return(0);
                               }
                               // This is the only type that has been used
				I->Target[Frame].Type = REINFORCED_SECTOR; // 23.08.13 changed I->Target[Frame].Type from I->Target[0].Type
                               ReinforcedArea = 1;

                               // Shock Parameters
                               if(!FindKeyWord(fp, SectionStart, SHOCK_PARAMETERS_LABEL, KeyWordString, Encoding)){
                                       fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
                                       return (0);
                               }
                               Type = GetType(KeyWordString);
                               switch(Type){
                               case -1:
                                       fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
                                       return (0);

                               case 0: // ASCII encoded
                                       NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
                                       if(!NumberOfParameters){
                                               fprintf(MyStdOut,"(%s).\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
                                               return (0);
                                       }

                                       if(NumberOfParameters >= 3){
                                               I->Shock.Latency = atoi(Parameters[0]);
                                               I->Shock.Duration = atoi(Parameters[1]);
                                               I->Shock.MinISI = atoi(Parameters[2]);

                                               if(NumberOfParameters == 4)
                                                       I->Shock.ResetInterval = atoi(Parameters[3]);
                                               else
                                                       I->Shock.ResetInterval = RESET_INTERVAL_DEFAULT;

                                       }else{
                                               fprintf(MyStdOut,"Must be 4 values for %s\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
                                               return (0);
                                       }
                                       if((I->Date.Year <= 2001) && (I->Date.Month < 5)){
                                              	I->Shock.CalculationMethod = TIME_BASED;
                                       }
                                       break;

                               default:
                                       fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
                                       return (0);
                               	break;
			}

			break;

                       case PLACE_PREFERENCE:
                               // Reinforced Place.

                               if(!FindKeyWord(fp, SectionStart, REINFORCED_SECTOR_LABEL, KeyWordString, Encoding)){
                                       fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
                                       return 0;
                               }
                               I->Target[0].CoordFrame = 0;


                               Type = GetType(KeyWordString);
                               switch(Type){
                               case -1:
                                       fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
                                       return(0);

                               case 0: // ASCII encoded
                                       NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
                                       if(!NumberOfParameters){
                                               fprintf(MyStdOut,"(%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
                                               return(0);
                                       }
                                       if(NumberOfParameters == 3){
                                               I->Target[Frame].Circle.X = (float)atof(Parameters[0]);
                                               I->Target[Frame].Circle.Y = (float)atof(Parameters[1]);
                                               I->Target[Frame].Circle.Rad = (float)atof(Parameters[2]);

                                               if((I->Target[Frame].Circle.Rad) > 0.0)
                                                       I->Target[Frame].Show = 1;
                                               else
                                                       I->Target[Frame].Show = 0;

                                       }else{
                                               fprintf(MyStdOut,"Must be 3 values for %s\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
                                               return(0);
                                       }
                                       break;

                               default:
                                       fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
                                       return(0);
                               }
                               // This is the only type that has been used
                               I->Target[0].Type = REINFORCED_CIRCLE;
                               ReinforcedArea = 1;

			// Feeder Mode
				// Feeder Mode is not necessary to determine for water maze experiments
                               	if(!FindKeyWord(fp, SectionStart, FEEDER_MODE_LABEL, KeyWordString, Encoding)){
					if(!I->Options.Watermaze){
                                       		fprintf(MyStdOut,"\nCan't find keyWord (%s). Potentially wrong File Format. ", FEEDER_MODE_LABEL);
                                       		return (0);
					}
                               }else{ 
                               		Type = GetType(KeyWordString);
                               		switch(Type){
                               		case -1:
                                       		fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
                                       		return (0);

                               		case 0: // ASCII encoded
                         				NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
                                     			if(!NumberOfParameters){
                                             		fprintf(MyStdOut,"(%s).\nWrong File Format.\n", FEEDER_MODE_LABEL);
                                             		return (0);
                                    	 		}else if(NumberOfParameters == 1){
								if(!strcmp(Parameters[0], POSITION_STRING)){
									I->Feeder[0].Mode = POSITION_MODE;
								}else if(!strcmp(Parameters[0], TIME_STRING)){
									I->Feeder[0].Mode = TIME_MODE;
								}
							}else{
                                                		fprintf(MyStdOut,"%s must be either %s or %s\nWrong File Format.\n", FEEDER_MODE_LABEL, POSITION_STRING, TIME_STRING);
                                                		return (0);
							}
							break;

						default:
                                        		fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", FEEDER_MODE_LABEL);
                                        		return (0);
							break;
					}
				}
							
                        // FeederParameters
                                if(!FindKeyWord(fp, SectionStart, FEEDER_PARAMETERS_LABEL, KeyWordString, Encoding)){
                                        fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
                                        return (0);
                                }
                                Type = GetType(KeyWordString);
                                switch(Type){
                                case -1:
                                        fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
                                        return (0);

                                case 0: // ASCII encoded
                                        NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
                                        if(!NumberOfParameters){
                                                fprintf(MyStdOut,"(%s).\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
                                                return (0);
                                        }

                                        if(NumberOfParameters == 2){
                                                I->Feeder[Frame].Trigger = (float)atof(Parameters[0]);
                                                I->Feeder[Frame].Refractory = (float)atof(Parameters[1]);
                                        }else{
                                                fprintf(MyStdOut,"Must be 2 values for %s\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
                                                return (0);
                                        }

                                        break;

                                default:
                                        fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", FEEDER_PARAMETERS_LABEL);
                                        return (0);
                                	break;
				}

				break;
                        default:
                                fprintf(MyStdOut,"TrackAnalysis is not prepared for this paradigm (%s)\n", I->Paradigm);
                                return(0);
                                break;
                        }
		
			// Calculate Arena boundaries in pixels
			ArenaRadius_pix = (int)(I->Arena.Radius * I->PixelsPerCM);
			I->Arena.Bottom = I->Arena.CenterY + ArenaRadius_pix;
			if(I->Arena.Bottom > 255)
				I->Arena.Bottom = 255;
			I->Arena.Right = I->Arena.CenterX + ArenaRadius_pix;
			if(I->Arena.Right > 255)
				I->Arena.Right = 255;
			I->Arena.Top = I->Arena.CenterY - ArenaRadius_pix;
			if(I->Arena.Top < 0)
				I->Arena.Top = 0;
			I->Arena.Left = I->Arena.CenterX - ArenaRadius_pix;
			if(I->Arena.Left < 0)
				I->Arena.Left = 0;

		}else{ 
		fprintf(MyStdOut, "\nCan't find Header section %s.\nWrong File Format.\n", "SETUP_INFORMATION");
		return (0);
	}

        // Section: RECORD_FORMAT
        SectionStart = FindSection(fp, HeaderStart, "RECORD_FORMAT");
        if(Offset){
                // Do stuff ...
                // Sample
                if(!FindKeyWord(fp, SectionStart, SAMPLE_FORMAT_LABEL, KeyWordString, Encoding)){
                        fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", SAMPLE_FORMAT_LABEL);
                        return 0;
                }
                Type = GetType(KeyWordString);
                switch(Type){
                case -1:
                        fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
                        return 0;

                case 0: // ASCII encoded
                        NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
                        if(!NumberOfParameters){
                                fprintf(MyStdOut,"%s\nWrong File Format.\n", SAMPLE_FORMAT_LABEL);
                                return 0;
                        }
                        I->FileParameters = NumberOfParameters;
                        GetParameterIndex(Parameters, NumberOfParameters, PI);
                }

                Offset = FindSectionEnd(fp);
                if(!Offset)
                        fprintf(MyStdOut,"\nCan't find end of Header Section (%s).\nWrong File Format.\n", "RECORD_FORMAT");
        }
	if(!(HeaderEnd = FindHeaderEnd(fp))){
		fprintf(MyStdOut, "\nCan't find end of header. Wrong data format\n");
		return (0);
	}
	return(HeaderEnd);
}

long	ReadHeaderDTtracker(FILE *fp, INFO *I, long HeaderStart)
{
	long    Offset, HeaderEnd, SectionStart;
        char    KeyWordString[256], Encoding[512], **Parameters;
        int             Type, NumberOfParameters, Frame, ReinforcedArea = 0;

	// The DTtracker file format used prior to about May 1, 2001
	I->Arena.Radius = DEFAULT_SMALL_ARENA_RADIUS;

	// Section:  SETUP_INFORMATION
        SectionStart = FindSection(fp, HeaderStart, "SETUP_INFORMATION");
	if(SectionStart == 0){
		fprintf(MyStdOut,"\nCan't find setction: %s\nWrong File Format.\n", "SETUP_INFORMATION");
		return(0);
	}
		

	// Tracker resolution PixelsPer cm
	if(!FindKeyWord(fp, SectionStart, TRACKER_RESOLUTION_LABEL, KeyWordString, Encoding)){
		I->PixelsPerCM = DEFAULT_PIXELS_PER_CM;
	}else{
		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(-1);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
				return(0);
			}
			if(NumberOfParameters == 1)
				I->PixelsPerCM = atof(Parameters[0]);
			else{
				fprintf(MyStdOut,"Must only be 1 value for %s\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", TRACKER_RESOLUTION_LABEL);
			return(0);
		}
	}
	// Paradigm
	if(!FindKeyWord(fp, SectionStart, PARADIGM_LABEL, KeyWordString, Encoding)){
		fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", PARADIGM_LABEL);
		return 0;
	}
	Type = GetType(KeyWordString);
	switch(Type){
	case -1:
		fprintf(MyStdOut,"/nKeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
		return(0);

	case 0:	// ASCII encoded
		NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
		if(!NumberOfParameters){
			fprintf(MyStdOut,"(%s).\nWrong File Format.\n", PARADIGM_LABEL);
			return 0;
		}
		if(NumberOfParameters == 1){
			strcpy(I->Paradigm, Parameters[0]);
			if(!strcmp(I->Paradigm, PLACE_AVOIDANCE_STRING))
                                      I->ParadigmType = PLACE_AVOIDANCE;
                        else if(!strcmp(I->Paradigm, PLACE_PREFERENCE_STRING))
                               I->ParadigmType = PLACE_PREFERENCE;
                        else {
                               fprintf(MyStdOut,"/nTrackAnalysis is not prepared for this paradigm (%s)\n", I->Paradigm);
                               return(0);
                        }
		}else{
			fprintf(MyStdOut,"/nMust only be 1 value for %s\nWrong File Format.\n", PARADIGM_LABEL);
			return(0);
		}
		break;

	default:
		fprintf(MyStdOut,"/n%s must be ASCII encoded\nWrong File Format.\n", PARADIGM_LABEL);
		return(0);
	}

				
	ReinforcedArea = 0; // At least 1 of the possible reinforced area labels must be found

	// (Room) Reinforced Sector.
	// This was the default for early versions when only the room frame was tracked
	if(FindKeyWord(fp, SectionStart, REINFORCED_SECTOR_LABEL, KeyWordString, Encoding)){

		Frame = ROOM_FRAME;
		I->Target[Frame].CoordFrame = ROOM_FRAME;

		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
				return(0);
			}
			if(NumberOfParameters == 4){
				I->Target[Frame].Sector.Ang = (float)atof(Parameters[0]);
				I->Target[Frame].Sector.Width = (float)atof(Parameters[1]);
				I->Target[Frame].Sector.InRad = (float)atof(Parameters[2]);
				I->Target[Frame].Sector.OutRad = (float)atof(Parameters[3]);

				if((I->Target[Frame].Sector.Ang) < 0.0)
					I->Target[Frame].Sector.Ang += 360.0;
				if(I->Target[Frame].Sector.Width != 0.0)
					I->Target[Frame].Show = 1;
				else
					I->Target[Frame].Show = 0;	
				
				if(I->Target[Frame].Sector.OutRad == 0.0)
					I->Target[Frame].Show = 0;	

			}else{
				fprintf(MyStdOut,"Must be 4 values for %s\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
				return(0);
			}

			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", REINFORCED_SECTOR_LABEL);
			return(0);
		}
		// This is the only type that has been used
		I->Target[Frame].Type = REINFORCED_SECTOR;
		ReinforcedArea = 1;
	}
		
	//Room Frame Reinforced Sector

	if(FindKeyWord(fp, SectionStart, ROOM_REINFORCED_SECTOR_LABEL, KeyWordString, Encoding)){
	//	fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", ROOM_REINFORCED_SECTOR_LABEL);
	//	return(0);

		Frame = ROOM_FRAME;
		I->Target[Frame].CoordFrame = ROOM_FRAME;

		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", ROOM_REINFORCED_SECTOR_LABEL);
				return(0);
			}
			if(NumberOfParameters == 4){
				I->Target[Frame].Sector.Ang = (float)atof(Parameters[0]);
				I->Target[Frame].Sector.Width = (float)atof(Parameters[1]);
				I->Target[Frame].Sector.InRad = (float)atof(Parameters[2]);
				I->Target[Frame].Sector.OutRad = (float)atof(Parameters[3]);

				if((I->Target[Frame].Sector.Ang) < 0.0)
					I->Target[Frame].Sector.Ang += 360.0;
				if(I->Target[Frame].Sector.Width != 0.0)
					I->Target[Frame].Show = 1;
				else
					I->Target[Frame].Show = 0;

				if(I->Target[Frame].Sector.OutRad == 0.0)
					I->Target[Frame].Show = 0;	

			}else{
				fprintf(MyStdOut,"Must be 4 values for %s\nWrong File Format.\n", ROOM_REINFORCED_SECTOR_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", ROOM_REINFORCED_SECTOR_LABEL);
			return(0);
		}
		// This is the only type that has been used
		I->Target[Frame].Type = REINFORCED_SECTOR;			
		ReinforcedArea = 1;
	}

	//Arena Frame Reinforced Sector
	if(FindKeyWord(fp, SectionStart, ARENA_REINFORCED_SECTOR_LABEL, KeyWordString, Encoding)){
	//			fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", ARENA_REINFORCED_SECTOR_LABEL);
	//			return(0);
		Frame = ARENA_FRAME;
		I->Target[Frame].CoordFrame = ARENA_FRAME;

		Type = GetType(KeyWordString);
		switch(Type){
		case -1:
			fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
			return(0);

		case 0:	// ASCII encoded
			NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
			if(!NumberOfParameters){
				fprintf(MyStdOut,"(%s).\nWrong File Format.\n", ARENA_REINFORCED_SECTOR_LABEL);
				return(0);
			}
			I->Target[Frame].CoordFrame = 1; // Arena Frame
			if(NumberOfParameters == 4){
				I->Target[Frame].Sector.Ang = (float)atof(Parameters[0]);
				I->Target[Frame].Sector.Width = (float)atof(Parameters[1]);
				I->Target[Frame].Sector.InRad = (float)atof(Parameters[2]);
				I->Target[Frame].Sector.OutRad = (float)atof(Parameters[3]);

				if((I->Target[Frame].Sector.Ang) < 0.0)
				I->Target[Frame].Sector.Ang += 360.0;
				if(I->Target[Frame].Sector.Width != 0.0)
					I->Target[Frame].Show = 1;
				else
					I->Target[Frame].Show = 0;

				if(I->Target[Frame].Sector.OutRad == 0.0)
					I->Target[Frame].Show = 0;	

			}else{
				fprintf(MyStdOut,"Must be 4 values for %s\nWrong File Format.\n", ARENA_REINFORCED_SECTOR_LABEL);
				return(0);
			}
			break;

		default:
			fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", ARENA_REINFORCED_SECTOR_LABEL);
			return(0);
		}
		// This is the only type that has been used
		I->Target[Frame].Type = REINFORCED_SECTOR;
		ReinforcedArea = 1;
	}

	if(!ReinforcedArea){
		fprintf(MyStdOut,"\nCan't find any of keyWords (%s %s %s).\nWrong File Format.\n",
			REINFORCED_SECTOR_LABEL, ROOM_REINFORCED_SECTOR_LABEL, ARENA_REINFORCED_SECTOR_LABEL);
		return(0);
	}

	// Shock Parameters
	if(!FindKeyWord(fp, SectionStart, SHOCK_PARAMETERS_LABEL, KeyWordString, Encoding)){
		fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
		return (0);
	}
	Type = GetType(KeyWordString);
	switch(Type){
	case -1:
		fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
		return (0);

	case 0:	// ASCII encoded
		NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
		if(!NumberOfParameters){
			fprintf(MyStdOut,"(%s).\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
			return (0);
		}

		if(NumberOfParameters >= 3){
			I->Shock.Latency = atoi(Parameters[0]);
			I->Shock.Duration = atoi(Parameters[1]);
			I->Shock.MinISI = atoi(Parameters[2]);

			if(NumberOfParameters == 4)
				I->Shock.ResetInterval = atoi(Parameters[3]);
			else
				I->Shock.ResetInterval = RESET_INTERVAL_DEFAULT;

		}else{
			fprintf(MyStdOut,"Must be 4 values for %s\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
			return (0);
		}

		// This is necessary for deciding how to make entrance/shock calculations. They were time
		// based but Martin changed them to frame based.
		if(I->Shock.Duration > UNREASONABLY_LONG_SHOCK_IN_FRAMES)
			I->Shock.CalculationMethod = TIME_BASED;
		else{
			I->Shock.CalculationMethod = SAMPLE_BASED; // SAMPLE_BASED;
		}
		break;

	default:
		fprintf(MyStdOut,"%s must be ASCII encoded\nWrong File Format.\n", SHOCK_PARAMETERS_LABEL);
		return (0);
	}
	Offset = FindSectionEnd(fp);
	if(!Offset){
		fprintf(MyStdOut,"\nCan't find end of Header Section %s.\nWrong File Format.\n", "SETUP_INFORMATION");
		return(0);
	}

        // Section: RECORD_FORMAT
        SectionStart = FindSection(fp, HeaderStart, "RECORD_FORMAT");
        if(SectionStart){
                // Do stuff ...                         
                // Sample
                if(!FindKeyWord(fp, SectionStart, SAMPLE_FORMAT_LABEL, KeyWordString, Encoding)){
                        fprintf(MyStdOut,"\nCan't find keyWord (%s).\nWrong File Format.\n", SAMPLE_FORMAT_LABEL);
                        return 0;           
                }
                Type = GetType(KeyWordString);
                switch(Type){
                case -1:
                        fprintf(MyStdOut,"KeyWord.integer format not respected in: %s\nWrong File Format.\n", KeyWordString);
                        return 0;

                case 0: // ASCII encoded
                        NumberOfParameters = GetASCIIEncoding(Encoding, &Parameters);
                        if(!NumberOfParameters){
                                fprintf(MyStdOut,"%s\nWrong File Format.\n", SAMPLE_FORMAT_LABEL);
                                return 0;
                        }
        
                        I->FileParameters = NumberOfParameters;
                        switch (NumberOfParameters){
			case 5:
				if(strcmp("FrameCount", Parameters[0]) != 0){
					fprintf(MyStdOut,"%s\nWrong File Format. Expecting: FrameCount 1msTimeStamp X Y State\n", Parameters[0]);
					return 0;
				}
				if(strcmp("1msTimeStamp",Parameters[1])){
					fprintf(MyStdOut,"%s\nWrong File Format. Expecting: 1msTimeStamp X Y State\n", Parameters[1]);
					return 0;
				}
				if(!((!strcmp("RedX",Parameters[2])) && (!strcmp("RedY",Parameters[3]))
					|| (!strcmp("RoomX",Parameters[2])) && (!strcmp("RoomY",Parameters[3])) )){
					fprintf(MyStdOut,"%s\nWrong File Format. Expecting: Red/Room/ArenaX Red/Room/ArenaY\n", SAMPLE_FORMAT_LABEL);
					return 0;
				}
				if(!strcmp("State",Parameters[4]) && !strcmp("FeederState",Parameters[4])){
					fprintf(MyStdOut,"%s\nWrong File Format. Expecting: State or FeederState\n", Parameters[4]);
					return (0);
				}
				I->NumberOfCoordFrames = 1;
				break;
			case 7:
				if(strcmp("FrameCount", Parameters[0]) != 0){
					fprintf(MyStdOut,"%s\nWrong File Format. Expecting: FrameCount 1msTimeStamp X Y State\n", Parameters[0]);
					return 0;
				}
				if(strcmp("1msTimeStamp",Parameters[1])){
					fprintf(MyStdOut,"%s\nWrong File Format. Expecting: 1msTimeStamp X Y State\n", Parameters[1]);
					return 0;
				}
				if(!((!strcmp("RedX",Parameters[2])) && (!strcmp("RedY",Parameters[3]))
					|| (!strcmp("RoomX",Parameters[2])) && (!strcmp("RoomY",Parameters[3])) )){
					fprintf(MyStdOut,"%s\nWrong File Format. Expecting: Red/Room/ArenaX Red/Room/ArenaY\n", SAMPLE_FORMAT_LABEL);
					return 0;
				}
				if(!(!strcmp("ArenaX",Parameters[4]) && !strcmp("ArenaY",Parameters[5]))){
					fprintf(MyStdOut,"%s\nWrong File Format. Expecting: ArenaX ArenaY\n", SAMPLE_FORMAT_LABEL);
					return (0);
				}
				if(!strcmp("State",Parameters[6]) && !strcmp("FeederState",Parameters[6])){
					fprintf(MyStdOut,"%s\nWrong File Format. Expecting: State or FeederState\n", Parameters[6]);
					return (0);
				}
				I->NumberOfCoordFrames = 2;
				break;

			default:
				fprintf(MyStdOut,"Must be 5 or 7 values for %s\nWrong File Format.\n", SAMPLE_FORMAT_LABEL);
				return (0);
			}
		}
	}

	Offset = FindSectionEnd(fp);
	if(!Offset)
		fprintf(MyStdOut,"\nCan't find end of Header Section (%s).\nWrong File Format.\n", "RECORD_FORMAT");

	if(!(HeaderEnd = FindHeaderEnd(fp))){
		fprintf(MyStdOut, "\nCan't find end of header. Wrong data format\n");
		return (0);
	}
/*
	for(i = 0; i < MAX_NUMBER_OF_FORMAT_PARAMETERS; i++)
                free(Parameters[i]);
        free(Parameters);
*/

	return(HeaderEnd);
}
long	FindHeaderStart(FILE *fp)
{
	char String[256];
	
	rewind(fp);
	while(fscanf(fp, "%s", String) != EOF){
		if(!strcmp(String, HEADER_BEGIN_LABEL)){
			return(ftell(fp));
		}
	}
	
	return (0);
}

long    FindNextHeaderStart(FILE *fp)
{
        char String[256];

        while(fscanf(fp, "%s", String) != EOF){
                if(!strcmp(String, HEADER_BEGIN_LABEL)){
                        return(ftell(fp));
                }
        }

        return (0);
}

long	FindHeaderEnd(FILE *fp)
{
	char String[256];
	
	while(fscanf(fp, "%s", String) != EOF){

		if(!strcmp(String, HEADER_END_LABEL)){
			return(ftell(fp));
		}
	}
	return (0);
}

long	FindSection(FILE *fp, long Offset, char *Section)
{
	char	Flag[256];

	fseek(fp, Offset, SEEK_SET);

	while (fscanf(fp,"%s", Flag) != EOF){
		if(Comment(fp, Flag)) continue;

		if(!strcmp(Flag, SECTION_BEGIN_LABEL)){
			fscanf(fp,"%s", Flag);

			if(!strcmp(Flag, Section))
				return(ftell(fp));
		}
	}
	return 0;
}

int	FindKeyWord(FILE *fp, long Offset, char *KeyWord, char *String, char *Line)
{

	fseek(fp, Offset, SEEK_SET);

	while (fscanf(fp,"%s", String) != EOF){
		if(Comment(fp, String)) continue;
		if(SectionEnd(String)) break;

		if(strstr(String, KeyWord) != NULL){
			(void)GetLine(fp, Line);
			return(1);
		}
	}
	return 0;
}

long	FindSectionEnd(FILE *fp)
{
	char	String[256];

	while (fscanf(fp,"%s", String) != EOF)
		if(SectionEnd(String))
			return(ftell(fp));
	return(0);
}

int	Comment(FILE *fp, char *String)
{
	if(!strcmp(String, COMMENT)){
		while(getc(fp) != '\n');
		return(1);
	}
	return(0);
}

int	SectionEnd(char *String)
{
	if(!strcmp(String, SECTION_END_LABEL))
		return(1);
	return(0);
}

int	GetType(char *KeyWord)
{
	char *TypeString;
	int	Type = -1;
	
	TypeString = strchr(KeyWord, TYPE_PREFIX);
	if(TypeString == NULL)
		return(-1);
	
	sscanf(TypeString+1,"%d", &Type);	// +1 to step past the '.'
	return(Type);
}

int	GetASCIIEncoding(char *Encoding, char ***P)
{
	int n; 
	char String[256], *TerminateString, *Line;
	static char **Parameters;

	if(Parameters != NULL)
		for(n=0; n < MAX_NUMBER_OF_FORMAT_PARAMETERS; n++)
			free (Parameters[n]);
	free(Parameters);
	
	Parameters = (char **)calloc(MAX_NUMBER_OF_FORMAT_PARAMETERS, sizeof(char *));
	if(Parameters == NULL){
		fprintf(MyStdOut, "\nCan't allocate Parameters\n");
		return (0);
	}

	for(n=0; n < MAX_NUMBER_OF_FORMAT_PARAMETERS; n++){
		Parameters[n] = (char *)calloc((size_t)256, sizeof(char));
		if(Parameters[n] == NULL){
			fprintf(MyStdOut, "\nCan't allocate Parameters\n");
			return (0);
		}
	}
	*P = Parameters;

	// a '(' must start the ASCII Encoding
	if((Line = strchr(Encoding, '(' )) == NULL){
		fprintf(MyStdOut,"a '(' must start ASCII encodings.\n");
		return (0);
	}
	Line++;

	if(sscanf(Line, "%s", String) != 1){
		fprintf(MyStdOut,"No ASCII string to decode\n");
		return (0);
	}

//	Parameters[0] = (char *)calloc(strlen(String + 1), sizeof(char));
	strcpy(Parameters[0], String);
	if((TerminateString = strchr(Parameters[0], ')' )) != NULL){	// a ')' ends the encoding
		*TerminateString = '\0';
		return(1);
	}

	n = 1;

	Line += (strlen(String) + 1);
	while((*Line == ' ') || (*Line == '\t')){	// move to start of next string
		Line ++;
	}

	while(sscanf(Line, "%s", String) != EOF){
			if(!strcmp(String, ")"))
			return (n);
//		Parameters[n] = (char *)calloc(strlen(String) + 1, sizeof(char));
		strcpy(Parameters[n], String);
		if((TerminateString = strchr(Parameters[n], ')')) != NULL){ // a ')' indicates the end of the ASCII encoding
			*TerminateString = '\0' ;
			return(++n);
		}
		Line += (strlen(String) + 1);
		while((*Line == ' ') || (*Line == '\t')){	// move to start of next string
			Line ++;
		}
		n++;
		if(n == MAX_NUMBER_OF_FORMAT_PARAMETERS){
			fprintf(MyStdOut,"Too many Parameters in Encoding of:\t");
			return(0);
		}
	}
		
	return (n);
}

void	GetParameterIndex(char **Params, int NParams, PARAMETER_INDEX *PI){
int	i;

	// initialize to -1;
	PI->FrameCount = -1;
	PI->msTimeStamp = -1;
	PI->RoomX = -1;
	PI->RoomY = -1;
	PI->ArenaX = -1;
	PI->ArenaY = -1;
	PI->Angle = -1;
	PI->Sectors = -1;
	PI->AvoidanceState = -1;
	PI->PreferenceState = -1;
	PI->DecideState = -1;
	PI->CurrentLevel = -1;
	PI->FrameInfo = -1;
	PI->Flags = -1;
	PI->MotorState = -1;
	PI->FeederState = -1;

	for(i = 0; i < NParams; i++){
		if(!strcmp(Params[i], "FrameCount"))
			PI->FrameCount = i;
		else if(!strcmp(Params[i], "1msTimeStamp"))
			PI->msTimeStamp = i;
		else if(!strcmp(Params[i],"RoomX"))
			PI->RoomX = i;
		else if(!strcmp(Params[i],"RoomY"))
			PI->RoomY = i;
		else if(!strcmp(Params[i],"ArenaX"))
			PI->ArenaX = i;
		else if(!strcmp(Params[i],"ArenaY"))
			PI->ArenaY = i;
		else if(!strcmp(Params[i],"Angle"))
			PI->Angle = i;
		else if(!strcmp(Params[i],"Sectors"))
			PI->Sectors = i;
		else if(!strcmp(Params[i],"State"))
			PI->State = i;
		else if(!strcmp(Params[i],"AvoidanceState"))
			PI->AvoidanceState = i;
		else if(!strcmp(Params[i],"PreferenceState"))
			PI->PreferenceState = i;
		else if(!strcmp(Params[i],"DecideState"))
			PI->DecideState = i;
		else if(!strcmp(Params[i],"CurrentLevel"))
			PI->CurrentLevel = i;
		else if(!strcmp(Params[i],"ShockLevel")) // backwards compatability
			PI->CurrentLevel = i;
		else if(!strcmp(Params[i],"FrameInfo"))
			PI->FrameInfo = i;
		else if(!strcmp(Params[i],"Flags"))
			PI->Flags = i;
		else if(!strcmp(Params[i],"MotorState"))
			PI->MotorState = i;
		else if(!strcmp(Params[i],"FeederState"))
			PI->FeederState = i;
		else
			fprintf(MyStdErr, "\nWarning: Parameter %s not matched while parsing the Record Format Header.\n", Params[i]);
	}
	return;
}
