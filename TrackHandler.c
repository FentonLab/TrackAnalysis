#include	<stdio.h>		/* This program reads data files from working directory and process them using	*/
#include	<string.h>		/* the function track_analysis	*/
#include	"Track.h"
#include	"Postscript.h"

int	main(int argc, char *argv[])
{
	int     join = 0, num = 2, k = 8;
	FILE    *sfp;
	char    **FileNames;    /* text buffer          */
	char	DirString[10];
	OPTIONS	Options;
	DIR_PATH	DirPath;
	TRACK	*Track;
	int		i, NumberOfFiles = 0;
	int	DirChar;

	FILEINFO	*FileInfo;
	
	// initialize options
	Options.IgnoreOnlineState = 0;
	Options.DoNotFilterTrack = 0;
	Options.TargetAnalysis = 0;
	Options.OverlappingTargets = 0;
	Options.Watermaze = 0;
	Options.OpenField = 0;
	Options.BsgV1 = 0;
	Options.TimeSeries = 0;
	Options.ZoneAnalysis = 0;
	Options.DwellMap = 0;

	// initialize directory paths
#ifdef UNIX_MACHINE
	DirChar = '/';
	strcpy(DirString, "/");
#else
	DirChar = '\\';
	strcpy(DirString, "\\");
#endif
	strcpy(DirPath.dat, DirString);
	strcpy(DirPath.tbl, DirString);
	strcpy(DirPath.sum, DirString);
	strcpy(DirPath.ps, DirString);
	strcpy(DirPath.ta, DirString);
	strcpy(DirPath.tms, DirString); // time series of various values
	strcpy(DirPath.map, DirString); // Dwell map 

	system(CLEAR_SCREEN);
	fprintf(stderr, "%s\n", TRACKANALYSIS_VERSION);
	fprintf(stderr, "Optional command line parameters are:\n");
	fprintf(stderr, "%s\tTo use TrackAnalysis with the Bio-Signal Group graphical user interface.\n", BSGV1);
	fprintf(stderr, "%s\n", DO_NOT_FILTER_TRACK);
	fprintf(stderr, "\tDisable the normal filtering of the track to correct\n\tno detects and detects that are too far to be realistic.\n");
	fprintf(stderr, "%s\n", IGNORE_ONLINE_STATE);
	fprintf(stderr, "\tIgnore the online determination of behavioral states\n\twhile analysing the data. It would be unusual to ignore this info.\n");
	fprintf(stderr, "%s argument\n", OPENFIELD);
	fprintf(stderr, "\targument=Total number of square areas in the 256x256 pixel tracked area for calculating line crossings. The suggested value is 16.\n");
	fprintf(stderr, "\tTreats the data as if were from an open field experiment.\n");
	fprintf(stderr, "%s\n", OVERLAPPING_TARGETS);
	fprintf(stderr, "\tIf the targets used for %s are large enough\n\tthen they may overlap and the same events\n\twill be counted multiple times. This is typically not allowed.\n", TARGET_ANALYSIS);
	fprintf(stderr, "%s\n", TARGET_ANALYSIS);
	fprintf(stderr, "\tProduces a .tbl file in directory TAfiles. The file counts\n\tentrances and time for different target diameters in each quadrant.\n");
	fprintf(stderr, "%s\n", TIMESERIES);
	fprintf(stderr, "\tProduces .tms file in directory TMSfiles. The file lists the timeseries of various parameters like the distance from the shock zone.\n");
	fprintf(stderr, "%s argument\n", WATERMAZE);
	fprintf(stderr, "\targument=trial time limit in sec.\n");
	fprintf(stderr, "\tAlso invokes %s. %s causes the initial no detects to be excluded\n", TARGET_ANALYSIS, WATERMAZE);
	fprintf(stderr, "\tfrom analysis and uses only the specified subsequent number of seconds for analysis.\n");
	fprintf(stderr, "%s\n", VERSION);
	fprintf(stderr, "\tThis option will print the version number of TrackAnalysis and then wait for a keypress before quitting.\n");
	fprintf(stderr, "%s\n", ZONE_ANALYSIS);
        fprintf(stderr, "\tProduces a .tbl file in directory ZAfiles. The file counts\n\ttime for different avoidance zones in each quadrant.\n");
	fprintf(stderr, "%s\n", DWELL_MAP);
	fprintf(stderr, "\tGenerate a dwell time map of the space in MAPfiles. The map is a linear array of 256x256 values corresponding to time in seconds.\n"); 
	fprintf(stderr, "\tThe array represents positions x0,y0, x1, y0,...x255, y255. x=position\%255; y=position/255.\n"); 

	Options.TimeLimit = 0;
	i = 1;
	while(argc - i){
		if(!strcmp(argv[i], IGNORE_ONLINE_STATE))	{
			Options.IgnoreOnlineState = 1;
			fprintf(stderr, "\nUsing option: %s.\n", IGNORE_ONLINE_STATE);
			i++;
			continue;
		}
		if(!strcmp(argv[i], DO_NOT_FILTER_TRACK))	{
			Options.DoNotFilterTrack = 1;
			fprintf(stderr, "\nUsing option: %s.\n", DO_NOT_FILTER_TRACK);
			i++;
			continue;
		}
		if(!strcmp(argv[i], TARGET_ANALYSIS))	{
			Options.TargetAnalysis= 1;
			fprintf(stderr, "\nUsing option: %s.\n", TARGET_ANALYSIS);
			i++;
			continue;
		}
		if(!strcmp(argv[i], OVERLAPPING_TARGETS))	{
			Options.OverlappingTargets= 1;
			fprintf(stderr, "\nUsing option: %s.\n", OVERLAPPING_TARGETS);
			i++;
			continue;
		}
		if(!strcmp(argv[i], OPENFIELD))	{
			Options.OpenField = 1;
			i++;
			if(i < argc){
				if(sscanf(argv[i],"%lf",&(Options.OpenFieldDimension)) != 1){
					fprintf(stderr, "With the OpenField option you must specify the total number of square regions that will fit into the 256x256 pixel tracked area.\n");
					return (-1);
				}
				Options.OpenFieldDimension = sqrt(Options.OpenFieldDimension);
			}else{
				fprintf(stderr, "With the OpenField option you must specify the total number of square regions that will fit into the 256x256 pixel tracked area.\n");
				return (-1);
			}
			i++;
			fprintf(stderr, "\nUsing option: %s.\n", OPENFIELD);
			continue;
		}
		if(!strcmp(argv[i], WATERMAZE))	{
			i++;
			if(i < argc){
				if(sscanf(argv[i],"%d",&(Options.TimeLimit)) != 1){
					fprintf(stderr, "Need to specify the duration of the analysis with the watermaze option\n");
					return (-1);
				}
			}else{
				fprintf(stderr, "Need to specify the duration of the analysis with the watermaze option\n");
				return (-1);
			}

			Options.TimeLimit *= 1000; // convert to ms
			Options.Watermaze = 1;
			Options.TargetAnalysis = 1;
			fprintf(stderr, "\nUsing options: %s %s.\n", WATERMAZE, TARGET_ANALYSIS);
			i++;
			continue;
		}
		if(!strcmp(argv[i], BSGV1))	{
			Options.BsgV1 = 1;
			fprintf(stderr, "\nUsing option: %s.\n", BSGV1);
			i++;
			continue;
		}
		if(!strcmp(argv[i], TIMESERIES))	{
			Options.TimeSeries = 1;
			fprintf(stderr, "\nUsing option: %s.\n", TIMESERIES);
			i++;
			continue;
		}
		if(!strcmp(argv[i], VERSION))	{
			system(CLEAR_SCREEN);
			fprintf(stderr, "\n\n\n%s", TRACKANALYSIS_VERSION);
			fprintf(stderr, "\nHit return/enter to quit.\n");
			while (fgetc(stdin) != "")
			exit(-1);	
		}
		if(!strcmp(argv[i], ZONE_ANALYSIS))	{
			Options.ZoneAnalysis= 1;
			fprintf(stderr, "\nUsing option: %s.\n", ZONE_ANALYSIS);
			i++;
			continue;
		}
		if(!strcmp(argv[i], DWELL_MAP))	{
			Options.DwellMap= 1;
			fprintf(stderr, "\nUsing option: %s.\n", DWELL_MAP);
			i++;
			continue;
		}

		fprintf(stderr, "Don't recognize the command line parameter %s.\n", argv[i]);
		fprintf(stderr, "The optional parameters are:\n");
		fprintf(stderr, "%s\n", IGNORE_ONLINE_STATE);
		fprintf(stderr, "%s\n", DO_NOT_FILTER_TRACK);
		fprintf(stderr, "%s argument=trial time limit in sec\n", WATERMAZE);
		fprintf(stderr, "%s\n", TARGET_ANALYSIS);
		fprintf(stderr, "%s\n", ZONE_ANALYSIS);
		fprintf(stderr, "%s\n", OVERLAPPING_TARGETS);
		fprintf(stderr, "%s\n", OPENFIELD);
		fprintf(stderr, "%s\n", TIMESERIES);
		fprintf(stderr, "%s\n", BSGV1);
		fprintf(stderr, "%s\n", VERSION);
		fprintf(stderr, "%s\n", DWELL_MAP);
		exit(-1);
	}
	fprintf(stderr, "\n");
 
	while(1){
		if(Options.BsgV1 == 1)
			NumberOfFiles = GetBSGv1FileNames(&DirPath, &FileNames, &FileInfo, Options);
		else
			NumberOfFiles = GetFileNames(&DirPath, &FileNames, &FileInfo, Options);

		if(!NumberOfFiles){
			fprintf(MyStdErr,"%c\nHit a key.\t", 7);
			if(getchar())
				return(-1);
		}

		sfp = fopen(FileNames[0], "w");	// a text summary file
		if(sfp == NULL){
			fprintf(MyStdOut,"Can't open %s.\nMake sure the following directories exist in the working path.\n", FileNames[0]);
			fprintf(MyStdOut,"DATfiles, SUMfiles, TBLfiles, PSfiles and TAfiles\n");
			return(-1);
		}

		fprintf (sfp, "Summary of the data in this analysis\n");
		fprintf (sfp, "Locomotory distances calculated at 1 sec resolution.\n");
		fclose (sfp);

		Track = NULL; // just to make sure. as TrackAnalysis tests for this before allocating memory to it 
		if(!TrackAnalysis(NumberOfFiles, DirPath, FileNames, FileInfo, Options)){
			fprintf(MyStdOut,"\nCouldn't complete analysis.\n");
		}

		fprintf(MyStdOut,"\n"); // This is so that the clear character is not written on the same line as
		fflush(MyStdOut);	// a file name when the output messages are redirected to a file.
		fprintf(MyStdErr,"%c\nHit a key. q to quit\t", 7);
		if(getchar() == 'q')
			break;
		system(CLEAR_SCREEN);
	}
	return (1);
}

int	GetFileNames(DIR_PATH *DirPath, char ***F, FILEINFO **FileInfo, OPTIONS Options)
{
	static 	char **DataFileNames;
	static	char AnalysisPath[256];
	static	FILEINFO	*I;
	static char	DATpath[256], OutputPath[256];
	static NumberOfFiles;
		
	char	TempFileName[256], DirString[10];
	char	AnalysisName[256], AnalysisFile[512], pwd[256], Line[256];
	int		DirChar, LineIndex, Index, Start, Stop, i, file;

	FILE	*fp;

	// do this to allow batch mode processing
        if(DataFileNames != NULL){
       	        for(i = 0; i < NumberOfFiles; i++)
       	        	free(DataFileNames[i]);
		free(DataFileNames);
		// DEBUGs("free DATA FILE NAMES")
	}

	fprintf(MyStdErr, "Enter '?' for the overview instructions\n\n");
	fprintf(MyStdErr, "\nThe input data files MUST be in a directory called DATfiles.\n");
	fprintf(MyStdErr, "The output will go to the corresponding directories\n");

	fprintf(MyStdErr, "(TBLfiles, SUMfiles, PSfiles, TAfiles)\n");
	fprintf(MyStdErr, "The 'Analysis' file MUST be in the path with the Output directories.\n");

	fprintf(MyStdErr, "DATfiles TBLfiles SUMfiles PSfiles & TAfiles must exist in the appropriate path.\n");
	fprintf(MyStdErr, "\nOn the same line, enter the paths to the\n");
	fprintf(MyStdErr, "Input (DATfiles) and the Output (TBLfiles, SUMfiles, PSfiles)\n");
	fprintf(MyStdErr, "%s\n%s\n\n",PATH_EXAMPLES, PWD_EXAMPLE);

#ifdef UNIX_MACHINE
	system("pwd > TAtemp");
	DirChar = '/';
	strcpy(DirString, "/");
#else
	system("cd > TAtemp");
	DirChar = '\\';
	strcpy(DirString, "\\");
#endif

	fp = fopen("TAtemp", "r");
	if(fp != NULL){
		GetLine(fp, pwd);
		pwd[strlen(pwd) - 1] = '\0';
		fclose(fp);

		system(DELETE_TAtemp);
	}
	fprintf(MyStdOut, "SHORT CUT-> Hit enter to use %s:\t", pwd);

	for(i = 0; i < 256; i++)
		AnalysisPath[i]='\0';

	GetChars(AnalysisPath);
	if(*AnalysisPath == '\0'){
		strcpy(DATpath, DEFAULT_ANALYSIS_PATH);
		strcpy(OutputPath, DEFAULT_ANALYSIS_PATH);
	} else if(*AnalysisPath == '?'){
			instruct();
	}else{
		if(sscanf(AnalysisPath,"%s%s",DATpath, OutputPath) != 2){
			fprintf(MyStdOut,"Must type the path to BOTH the DATfiles and the Output files\nQuitting.\n\n");
			return(0);
		}
	}

	if(DATpath[strlen(DATpath) -1] != DirChar)
		strcat(DATpath, DirString);
	if(OutputPath[strlen(OutputPath) -1] != DirChar)
		strcat(OutputPath, DirString);

	sprintf(DirPath->lst, "%s", DATpath);
	sprintf(DirPath->dat, "%s%s%s", DATpath, DEFAULT_DATDIR, DirString);
	sprintf(DirPath->tbl, "%s%s%s", OutputPath, DEFAULT_TBLDIR, DirString);
	sprintf(DirPath->sum, "%s%s%s", OutputPath, DEFAULT_SUMDIR, DirString);
	sprintf(DirPath->ps, "%s%s%s", OutputPath, DEFAULT_PSDIR, DirString);
	sprintf(DirPath->ta, "%s%s%s", OutputPath, DEFAULT_TADIR, DirString);
	sprintf(DirPath->tms, "%s%s%s", OutputPath, DEFAULT_TMSDIR, DirString);
	sprintf(DirPath->za, "%s%s%s", OutputPath, DEFAULT_ZADIR, DirString);
	sprintf(DirPath->map, "%s%s%s", OutputPath, DEFAULT_MAPDIR, DirString);

	fprintf(MyStdOut, "\nDirectories:   Analysis file = %s\n", DirPath->lst);
	fprintf(MyStdOut, "                   TBL files = %s\n", DirPath->tbl);
	fprintf(MyStdOut, "                   SUM files = %s\n", DirPath->sum);
	fprintf(MyStdOut, "                    PS files = %s\n", DirPath->ps);
	if(Options.TargetAnalysis)
		fprintf(MyStdOut, "                    TA files = %s\n", DirPath->ta);
	if(Options.TimeSeries)
		fprintf(MyStdOut, "                    TS files = %s\n", DirPath->tms);
	if(Options.ZoneAnalysis)
		fprintf(MyStdOut, "                    ZA files = %s\n", DirPath->za);
	if(Options.DwellMap)
		fprintf(MyStdOut, "                    MAP files = %s\n", DirPath->map);

	while (1){
		fprintf(MyStdErr, "\nEnter the .txt file listing the data files to analyse:\n");
		fprintf(MyStdErr, "[Filenames must not contain white space or they must be enclosed in double quotes]\n");
		fprintf(MyStdErr, "[i.e. FileName.dat and \"File Name.dat\" are o.k. but File Name.dat is incorrect]\n");
		fprintf(MyStdErr, "[Explicitly name the Room and Arena Frame files (_Room or _Arena)\nor else priority is given to _Room]\n\n");
		fprintf(MyStdErr, "\tAnalysis:  ");
		GetChars(AnalysisName);
		if(*AnalysisName == '?')
			instruct();
		if(*AnalysisName == '\0')
			break;

		sprintf(AnalysisFile, "%s%s.txt", DirPath->lst, AnalysisName);
		fp = fopen(AnalysisFile, "r");
		if(fp == NULL){
			sprintf(AnalysisFile, "%s%s", DirPath->lst, AnalysisName);
            fp = fopen(AnalysisFile, "r");
            if(fp == NULL){
				fprintf(MyStdOut,"Can't open %s or %s.txt. Try again.\n", AnalysisFile, AnalysisFile);
                continue;
            }
        }
		break;
	}
	NumberOfFiles = 0;

	while (1){
		GetLine(fp, Line);
		if(*Line == '\0'){
			break;
		}
			
		// format File1 [Start Stop] File2 [Start Stop] ... where subsequent files and start/stop indices are optional
		// or
		// format "File name1" [Start Stop] "File name2" [Start Stop] ... where subsequent files and start/stop indices are optional
		NumberOfFiles += CountFilesInLine(Line);
	}

	NumberOfFiles += 2;		// including the .sum and .tbl files
	DataFileNames = (char **)calloc((size_t)(NumberOfFiles + 1), sizeof(char *));
	if(DataFileNames == NULL){
		fprintf(MyStdOut, "Can't calloc FileInfo\n");
		return(-1);
	}

	for(i = 0; i < NumberOfFiles; i++)
		DataFileNames[i] = (char *)calloc((size_t)256, sizeof(char));

	I = (FILEINFO *)calloc((size_t)(NumberOfFiles + 1), sizeof(FILEINFO));
	if(I == NULL){
		fprintf(MyStdOut, "Can't calloc FileInfo\n");
		return(-1);
	}

	sprintf(DataFileNames[0], "%s%s%s%s%s.sum", OutputPath, DirString, DEFAULT_SUMDIR, DirString, AnalysisName);
	sprintf(DataFileNames[1], "%s%s%s%s%s.tbl", OutputPath, DirString, DEFAULT_TBLDIR, DirString, AnalysisName);

	rewind(fp);

	for (i = 2; i < NumberOfFiles; i++){
		GetLine(fp, Line);
		// format File1 [Start Stop] File2 [Start Stop] ... where subsequent files and start/stop indices are optional
		// or
		// format "File name1" [Start Stop] "File name2" [Start Stop] ... where subsequent files and start/stop indices are optional
		file = I[i].NFiles = LineIndex = 0;
		while(1){
			I[i].StartTime[file] = 0;
			I[i].StopTime[file] = (MAX_SECONDS * 1000);

			Index = 0;

			if(GetStringInQuotes(Line+LineIndex, TempFileName, &Index) == EOF){
				file--; 
				break;
			}else if(!Index){
				file--; 
				break;
			}
				
			sprintf(DataFileNames[i + file],"%s%s", DirPath->dat, TempFileName);
			strcpy(I[i].ShortFileName[file], TempFileName);

			// Construct PS, TA, ZA and MAP filenames: first remove the DATA FILE SUFFIX
			sprintf(I[i].PSFileName,"%s%s%s%s", OutputPath, DEFAULT_PSDIR, DirString, TempFileName);
			sprintf(I[i].TAFileName,"%s%s%s%s", OutputPath, DEFAULT_TADIR, DirString, TempFileName);
			sprintf(I[i].TMSFileName,"%s%s%s%s", OutputPath, DEFAULT_TMSDIR, DirString, TempFileName);
			sprintf(I[i].ZAFileName,"%s%s%s%s", OutputPath, DEFAULT_ZADIR, DirString, TempFileName);
			sprintf(I[i].MAPFileName,"%s%s%s%s", OutputPath, DEFAULT_MAPDIR, DirString, TempFileName);
			sprintf(I[i].CompositeFileName,"%s", TempFileName);

			if(strstr(I[i].PSFileName, DATA_FILE_SUFFIX) != NULL)
				sprintf(strstr(I[i].PSFileName, DATA_FILE_SUFFIX),"%s","");
			if(strstr(I[i].TAFileName, DATA_FILE_SUFFIX) != NULL)
				sprintf(strstr(I[i].TAFileName, DATA_FILE_SUFFIX),"%s","");
			if(strstr(I[i].ZAFileName, DATA_FILE_SUFFIX) != NULL)
				sprintf(strstr(I[i].ZAFileName, DATA_FILE_SUFFIX),"%s","");
			if(strstr(I[i].MAPFileName, DATA_FILE_SUFFIX) != NULL)
				sprintf(strstr(I[i].MAPFileName, DATA_FILE_SUFFIX),"%s","");
			

			if(Index){
				(I[i].NFiles)++;
				LineIndex += Index;
				Index = 0;
			}

			if(sscanf(Line+LineIndex, " [%d%d]%n", &Start, &Stop, &Index) == EOF){
				break;
			}
			if(Index > 5){
				if(Stop > 0){			// if Stop == 0 then take all the track
					I[i].StartTime[file] = (Start * 1000);
					I[i].StopTime[file] = (Stop * 1000);

					// Construct PS filename: add start-stop times
					sprintf(TempFileName, "[%d-%d]",Start, Stop);
					strcat(I[i].PSFileName, TempFileName);

					// Construct TA filename: add start-stop times
					sprintf(TempFileName, "[%d-%d]",Start, Stop);
					strcat(I[i].TAFileName, TempFileName);

					// Construct Composite filename: add start-stop times
					sprintf(TempFileName, "[%d-%d]",Start, Stop);
					strcat(I[i].CompositeFileName, TempFileName);
				}
			}
			file++;
			LineIndex += Index;
				
		}

		i += (file);		// increment for the additional files 
	}
	fclose(fp);


	*F = (char **)DataFileNames;
	*FileInfo = I;

	if(NumberOfFiles)
		fprintf(MyStdOut,"\n\n%s\nDoing Analysis %s, ",TRACKANALYSIS_VERSION, AnalysisName);
	return NumberOfFiles;
}

int	GetBSGv1FileNames(DIR_PATH *DirPath, char ***F, FILEINFO **FileInfo, OPTIONS Options)
{
	static 	char **DataFileNames;
	static	char AnalysisPath[256];
	static	FILEINFO	*I;
	static char	DATpath[256], OutputPath[256];
	static NumberOfFiles;
		
	char	TempFileName[256], FileName[256], *pFile, DirString[10];
	char	AnalysisName[256], AnalysisFile[512], Line[256];
	int		LineIndex, Index, Start, Stop, i, file;
	int	DirChar, length;

	FILE	*fp;

	// do this to allow batch mode processing
        if(DataFileNames != NULL){
       	        for(i = 0; i < NumberOfFiles; i++)
       	        	free(DataFileNames[i]);
		free(DataFileNames);
		// DEBUGs("free DATA FILE NAMES")
	}

	fprintf(MyStdErr, "Enter '?' for the overview instructions\n\n");
	fprintf(MyStdErr, "\nThe input data files MUST be in a directory called DATfiles.\n");
	fprintf(MyStdErr, "The output will go to the corresponding directories\n");

	fprintf(MyStdErr, "(TBLfiles, SUMfiles, PSfiles, TAfiles)\n");

	fprintf(MyStdErr, "\nEnter the path to the LSTfiles and the output directories\n");
	fprintf(MyStdErr, "TBLfiles SUMfiles PSfiles (optionally TAfiles).\n");

	*OutputPath ='\0';

	GetChars(OutputPath);
	if(*OutputPath == '?'){
		instruct();
	}

#ifdef UNIX_MACHINE
	DirChar = '/';
	strcpy(DirString, "/");
#else
	DirChar = '\\';
	strcpy(DirString, "\\");
#endif

	if(OutputPath[strlen(OutputPath) -1] != DirChar)
		strcat(OutputPath, DirString);

	sprintf(DirPath->lst, "%s%s%s", OutputPath, DEFAULT_LSTDIR, DirString);
	sprintf(DirPath->tbl, "%s%s%s", OutputPath, DEFAULT_TBLDIR, DirString);
	sprintf(DirPath->sum, "%s%s%s", OutputPath, DEFAULT_SUMDIR, DirString);
	sprintf(DirPath->ps, "%s%s%s", OutputPath, DEFAULT_PSDIR, DirString);
	sprintf(DirPath->ta, "%s%s%s", OutputPath, DEFAULT_TADIR, DirString);
	sprintf(DirPath->tms, "%s%s%s", OutputPath, DEFAULT_TMSDIR, DirString);
	sprintf(DirPath->za, "%s%s%s", OutputPath, DEFAULT_ZADIR, DirString);

	fprintf(MyStdOut, "\nDirectories:   Analysis file = %s\n", DirPath->lst);
	fprintf(MyStdOut, "                   TBL files = %s\n", DirPath->tbl);
	fprintf(MyStdOut, "                   SUM files = %s\n", DirPath->sum);
	fprintf(MyStdOut, "                    PS files = %s\n", DirPath->ps);
	if(Options.TargetAnalysis)
		fprintf(MyStdOut, "                    TA files = %s\n", DirPath->ta);
	if(Options.TimeSeries)
		fprintf(MyStdOut, "                    TMS files = %s\n", DirPath->tms);
	if(Options.ZoneAnalysis)
		fprintf(MyStdOut, "                    ZA files = %s\n", DirPath->za);

	while (1){
		fprintf(MyStdErr, "\nEnter the .txt file listing the data files to analyse:\n");
		fprintf(MyStdErr, "\tAnalysis:  ");
		GetChars(AnalysisName);
		if(*AnalysisName == '?')
			instruct();
		if(*AnalysisName == '\0')
			break;

		// remove the terminating '.txt' if it is there
		pFile = strstr(AnalysisName,".txt");
		if (pFile != NULL)
			*pFile = '\0';
			
		sprintf(AnalysisFile, "%s%s.txt", DirPath->lst, AnalysisName);
		fp = fopen(AnalysisFile, "r");
		if(fp == NULL){
                        sprintf(AnalysisFile, "%s%s", DirPath->lst, AnalysisName);
                        fp = fopen(AnalysisFile, "r");
                        if(fp == NULL){
                                fprintf(MyStdOut,"Can't open %s%s or %s%s.txt. Try again.\n", DirPath->lst, AnalysisName, DirPath->lst, AnalysisName);
                                continue;
                        }
                }
		break;
	}
	NumberOfFiles = 0;
	while (1){
		GetLine(fp, Line);
		if(*Line == '\0'){
			break;
		}

		NumberOfFiles += CountFilesInLine(Line);
		// NumberOfFiles += CountFiles(Line, FILE_TERMINATING_STRING);
	}
	NumberOfFiles += 2;		// including the .sum and .tbl files
	DataFileNames = (char **)calloc((size_t)(NumberOfFiles + 1), sizeof(char *));
	if(DataFileNames == NULL){
		fprintf(MyStdOut, "Can't calloc FileInfo\n");
		return(-1);
	}

	for(i = 0; i < NumberOfFiles; i++){
		DataFileNames[i] = (char *)calloc((size_t)256, sizeof(char));
	}

	I = (FILEINFO *)calloc((size_t)(NumberOfFiles + 1), sizeof(FILEINFO));
	if(I == NULL){
		fprintf(MyStdOut, "Can't calloc FileInfo\n");
		return(-1);
	}

	sprintf(DataFileNames[0], "%s%s%s", DirPath->sum, AnalysisName, ".sum");
	sprintf(DataFileNames[1], "%s%s%s", DirPath->tbl, AnalysisName, ".tbl");
	
	rewind(fp);

	for (i = 2; i < NumberOfFiles; i++){
		GetLine(fp, Line);
                // format File1 [Start Stop] File2 [Start Stop] ... where subsequent files and start/stop indices are optional
                // or
                // format "File name1" [Start Stop] "File name2" [Start Stop] ... where subsequent files and start/stop indices are optional

		file = I[i].NFiles = LineIndex = 0;
		while(1){
			I[i].StartTime[file] = 0;
			I[i].StopTime[file] = (MAX_SECONDS * 1000);

			Index = 0;

			strcpy(FileName, Line+LineIndex);
			if(GetStringInQuotes(Line+LineIndex, FileName, &Index) == EOF){
				file--;
				break;
			}else if(!Index){
				file--; 
				break;
			}

			strcpy(DataFileNames[i + file], FileName); // store the full path
			strcpy(I[i].FullFileName[file], FileName); // store the full path

			// now find the start of filename without the path
			length = strlen(FileName);
			while(FileName[length] != DirChar){
				length--;
				if(!length)
					break;
			}
			if(length)
				strcpy(FileName, FileName+length+1);

			strcpy(I[i].ShortFileName[file], FileName); // store the short file name

            		// Construct PS and TA filenames: first remove the DATA FILE SUFFIX
            		sprintf(I[i].PSFileName,"%s%s%s%s", OutputPath, DEFAULT_PSDIR, DirString, FileName);
            		sprintf(I[i].TAFileName,"%s%s%s%s", OutputPath, DEFAULT_TADIR, DirString, FileName);
            		sprintf(I[i].ZAFileName,"%s%s%s%s", OutputPath, DEFAULT_ZADIR, DirString, FileName);
			sprintf(I[i].CompositeFileName,"%s", FileName);

			if(strstr(I[i].PSFileName, FILE_TERMINATING_STRING) != NULL){
				sprintf(strstr(I[i].PSFileName, FILE_TERMINATING_STRING),"%s","");
				sprintf(strstr(I[i].TAFileName, FILE_TERMINATING_STRING),"%s","");
				sprintf(strstr(I[i].ZAFileName, FILE_TERMINATING_STRING),"%s","");
			}

			if(Index){
				(I[i].NFiles)++;
				LineIndex += Index;
				Index = 0;
			}

			if(sscanf(Line+LineIndex, " [%d%d]%n", &Start, &Stop, &Index) == EOF){
				break;
			}
			if(Index > 5){
				if(Stop > 0){
					I[i].StartTime[file] = (Start * 1000);
					I[i].StopTime[file] = (Stop * 1000);

					// Construct PS filename: add start-stop times
					sprintf(TempFileName, "[%d-%d]",Start, Stop);
					strcat(I[i].PSFileName, TempFileName);

					// Construct TA filename: add start-stop times
					sprintf(TempFileName, "[%d-%d]",Start, Stop);
					strcat(I[i].TAFileName, TempFileName);

					// Construct Composite filename: add start-stop times
					sprintf(TempFileName, "[%d-%d]",Start, Stop);
					strcat(I[i].CompositeFileName, TempFileName);
				}
			}
			file++;
			LineIndex += Index;
		}

		i += (file);		// increment for the additional files 
	}
	fclose(fp);


	*F = (char **)DataFileNames;
	*FileInfo = I;

	if(NumberOfFiles)
		fprintf(MyStdOut,"\n\nDoing Analysis %s, ",AnalysisName);
	return NumberOfFiles;
}

int	CountFilesInLine(char *Line){
	int	LineIndex = 0, Index, Start, Stop;
	int	NumberOfFiles = 0;
	char	Filename[256];

	// format File1 [Start Stop] File2 [Start Stop] ... where subsequent files and start/stop indices are optional
	// or
	// format "File name1" [Start Stop] "File name2" [Start Stop] ... where subsequent files and start/stop indices are optional
	while(1){
                        if(GetStringInQuotes(Line+LineIndex, Filename, &Index) == EOF)
                                break;
                        if(!Index)
                                break;
                        LineIndex += Index;
                        NumberOfFiles++;

                        Index = 0;
                        if(sscanf(Line+LineIndex, " [%d %d]%n", &Start, &Stop, &Index) == EOF)
                                break;

                        LineIndex += Index;
                }
/*
	while(1){
		 pLine = Line+LineIndex;

		// search for the first occurence of the string that marks the end part of a filename
		if((pFile = strstr(pLine, TerminatingString)) == NULL){
			break;
		}
		NumberOfFiles++;
		LineIndex = (pFile - Line) + strlen(TerminatingString);
	}
*/
	return NumberOfFiles;
}
