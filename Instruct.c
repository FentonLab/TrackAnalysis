#include	"Track.h"

void	instruct()
{
	fprintf(MyStdErr, "Usage: TrackAnalysis\n\n");
	fprintf(MyStdErr, "This program works with standard track data files with the extension .dat\n");
	fprintf(MyStdErr, "It prompts for the name of a text file (AnalysisName) containing the data\nfilenames to be analysed.\n");
	fprintf(MyStdErr, "The format of the file requires 1 analysis per line.\n");
	fprintf(MyStdErr, "Multiple files on a line causes them to be concatenated\n");
	fprintf(MyStdErr, "Specific parts of files can be selected for analysis by specifying\n");
	fprintf(MyStdErr, "within '[]' the start time and stop time in seconds\n");
	fprintf(MyStdErr, "Here is an example of a file specifying 3 analyses:\n");
	fprintf(MyStdErr, "1. All of file1, 2.  Seconds 0 to 600 of file2,\n");
	fprintf(MyStdErr, "and  3. Seconds 100 to 1200 of file3 concatenated with all of file2.\n");
	fprintf(MyStdErr, "file1 \n");
	fprintf(MyStdErr, "file2 [0 600]\n");
	fprintf(MyStdErr, "file3 [0 1200] file2\n\n");

	fprintf(MyStdErr, "Hit any key to continue\n");
	getchar();

	fprintf(MyStdErr, "TrackAnalysis calculates a variety of parameters and produces the following files:\n");
	fprintf(MyStdErr,	"AnalysisName.sum - A summary text file of each data file processed\n");
	fprintf(MyStdErr, "AnalysisName.tbl - A tabular file to be directly imported to Excel.\n");
	fprintf(MyStdErr, "DataFileName.ps  - A commercial software compatible Postscript graphics file.\n");
	fprintf(MyStdErr, "The following 4 directories must be in the working directory:\n");
	fprintf(MyStdErr, "DATfiles, TBLfiles, SUMfiles, PSfiles\n");
	fprintf(MyStdErr, "Data files MUST be in DATfiles.\n");
	fprintf(MyStdErr, "\nTrackAnalysis will continue to develop. More analyses will be added.\n");
	fprintf(MyStdErr, "It's use is organized with this development explicitly in mind.\n");
	fprintf(MyStdErr, "Organize analyses according to the data files and save each analysis file\n");
	fprintf(MyStdErr, "so it can be run later when TrackAnalysis has new functions.\n");

	fprintf(MyStdErr,"%c\nHit any key", 7);
	getchar();
	return;
}

