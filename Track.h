#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<string.h>
#include	"Header.h"
#include	"Postscript.h"

//	********************************************
// Define what machine the code will be compiled for
// since Unix and Windows directory path conventions differ
//	********************************************

#define UNIX_MACHINE
// #define WINDOWS_MACHINE

#define TRACKANALYSIS_VERSION	"TrackAnalysis Version 2.52\nRelease date 21.03.2014\n"

#define MyStdErr	stderr
#define MyStdOut	stdout

//Data file formats
#define TRACKER_FORMAT	3 // used after October 18, 2007
#define ITRACK_FORMAT	2 // used after March 15, 2003
#define FGX_FORMAT	1 // used after May 1, 2001
#define DTtracker_FORMAT 0 // used prior to May 1, 2001

#define	DEBUGi(x) printf("\n%d\n", x); fflush(stdout);
#define	DEBUGs(x) printf("\n%s\n", x); fflush(stdout);
#define	DEBUGf(x) printf("\n%f\n", x); fflush(stdout);
#define	DEBUG(x) printf("\n%d\n", x); fflush(stdout);


#ifdef WINDOWS_MACHINE
	#define M_PI			3.1415926535897932384626433832795
	#define CLEAR_SCREEN		"cls"
	#define	DELETE_TAtemp	"del TAtemp"
	#define	DELETE_ZAtemp	"del ZAtemp"
	#define PATH_EXAMPLES	"e.g. drive:\\directory\\withDATfiles\\ drive:\\directory\\withPS-SUM-TBLfiles\\"
	#define PWD_EXAMPLE		"'.\\' is used to specify the working directory"
	#define DEFAULT_ANALYSIS_PATH ".\\"
#endif

#ifdef UNIX_MACHINE
	#define CLEAR_SCREEN		"clear"
	#define	DELETE_TAtemp		"rm TAtemp"
	#define	DELETE_ZAtemp		"rm ZAtemp"
	#define PATH_EXAMPLES		"e.g. /directory/withDATfiles/ /directory/withPS-SUM-TBLfiles/"
	#define PWD_EXAMPLE			"'./' is used to specify the working directory"
	#define DEFAULT_ANALYSIS_PATH "./"
#endif

#define	FILE_TERMINATING_STRING	".dat"
#define	DEG_PER_RAD		(180.0 / M_PI)
#define MAX_INT			0xFFFFFFFFFFFFFFFF
#define MAX_MINUTES		(300)
#define	MSperMINUTE		(60000)
#define MAX_SECONDS		(MAX_MINUTES * 60)
#define	DEFAULT_ARENA_CENTER_X		128
#define	DEFAULT_ARENA_CENTER_Y		128
#define	DEFAULT_ARENA_RADIUS		128
#define	DEFAULT_SMALL_ARENA_RADIUS	40
#define	DEFAULT_PIXELS_PER_CM		2.07
#define MAXIMUM_COORDINATE		255
#define TRACKER_XY_RESOLUTION		256
#define MAX_FILES_TO_CONCATENATE	10
#define POSITION_SAMPLING_INTERVAL 	100 // in ms

// Commandline options
#define IGNORE_ONLINE_STATE 	"IgnoreOnlineState"
#define DO_NOT_FILTER_TRACK 	"DoNotFilterTrack"
#define TARGET_ANALYSIS 	"TargetAnalysis"
#define OVERLAPPING_TARGETS	"OverlappingTargets"
#define WATERMAZE		"Watermaze"
#define OPENFIELD		"OpenField"
#define BSGV1			"BioSignalGroupV1"
#define TIMESERIES		"TimeSeries"
#define VERSION			"Version"
#define ZONE_ANALYSIS 		"ZoneAnalysis"
#define DWELL_MAP		"DwellMap"

// Experimental Paradigms
#define PLACE_AVOIDANCE_STRING	"PlaceAvoidance"
#define PLACE_PREFERENCE_STRING	"PlacePreference"
#define TRACKER_STRING		"Tracker"
#define OPEN_FIELD_STRING	"OpenField"
#define WATERMAZE_STRING	"Watermaze"
#define PLACE_AVOIDANCE		1
#define PLACE_PREFERENCE	2
#define TRACKER			3
#define OPEN_FIELD		4
#define WATER_MAZE		5

// Frame Types in the header e.g. %Frame.0 ( RoomFrame )
#define	HEADER_ROOM_FRAME_WORD		"RoomFrame"
#define	HEADER_ARENA_FRAME_WORD		"ArenaFrame"
#define	HEADER_CAMERA_FRAME_WORD	"CameraFrame"

// Reinforced area types
#define	REINFORCED_CIRCLE	1
#define	REINFORCED_SECTOR	2
#define	REINFORCED_POLYGON	3

// Feeder Mode definitions
#define POSITION_STRING		"Position"
#define TIME_STRING		"Time"
#define POSITION_MODE		1
#define TIME_MODE		2

// Quadrant definitions
#define	TARGET_QUADRANT_ANG	0.0
#define	CCW_QUADRANT_ANG	(M_PI / 2.0)
#define	OPP_QUADRANT_ANG	M_PI
#define	CW_QUADRANT_ANG		(3.0 * M_PI / 2.0)

#define	TARGET_QUADRANT	1
#define	CCW_QUADRANT	2
#define	OPP_QUADRANT	3
#define	CW_QUADRANT	4

#define	TARGET				1
#define	COUNTER_CLOCKWISE		2
#define	OPPOSITE			3
#define	CLOCKWISE			4

#define DEFAULT_DATDIR	"DATfiles"
#define DEFAULT_LSTDIR	"LSTfiles"
#define DEFAULT_TBLDIR	"TBLfiles"
#define DEFAULT_SUMDIR	"SUMfiles"
#define DEFAULT_PSDIR	"PSfiles"
#define DEFAULT_TADIR	"TAfiles"
#define DEFAULT_TMSDIR	"TMSfiles"
#define DEFAULT_ZADIR	"ZAfiles"
#define DEFAULT_MAPDIR	"MAPfiles"
#define DATA_FILE_SUFFIX	".dat"
#define ROOM_FRAME_IN_FILENAME "_Room"
#define ARENA_FRAME_IN_FILENAME "_Arena"
#define ANGLE 				1
#define RADIUS				2
#define ROOM_FRAME_INCREASE		5.0


#define NO_FILE_TEXT	"-NoFile(s)"

// Arena types
#define	ARENA_TYPE_CIRCLE	1

// Frame types
#define	UNKNOWN_FRAME				-1
#define	ROOM_FRAME				0
#define	ARENA_FRAME				1
#define	ROOM_AND_ARENA_FRAME			2
#define	SINGLE_FRAME				ROOM_FRAME
#define	CAMERA_FRAME				ROOM_FRAME

// Sector values
#define	ROOM_SECTOR				1
#define	ARENA_SECTOR				2
#define	ROOM_AND_ARENA_SECTOR			3

// Analysis Parameters
#define	DEFAULT_SAMPLING_INTERVAL	1000 // 1 sec in ms
#define RESET_INTERVAL_DEFAULT		1000 // 1 sec in ms
#define LINEARITY_SCALE_FACTOR		2	// sets the time scale for measuring linearity by multiplying the timestep for distance
#define	SHOCK_STATE			2	// When Event state is 2 the rat is shocked 
#define	BAD_SPOT_STATE			5	// Tracker introduced this state. When Event state is 5 the track was lost
#define	PA_STATE_Undefined		-1 // iTrack defines 5 Place avoidance states as... iTrack defined a new state BadSpot
#define PA_STATE_OutsideSector 		0	
#define PA_STATE_EntranceLatency 	1
#define PA_STATE_Shock			2
#define PA_STATE_InterShockLatency	3
#define PA_STATE_Refractory		4
#define PA_STATE_BadSpot		5
#define PP_STATE_OutsideSector 		0	
#define PP_STATE_Entering 		1	// iTrack defines 4 Preference states as...iTrack defined a new state BadSpot
#define PP_STATE_Feed			2
#define PP_STATE_refractory		3
#define PP_STATE_BadSpot		4

#define ANGULAR_BIN_SIZE_IN_DEGREES	10 // determines the resolution for angular distribution calculations
#define N_ANGULAR_BINS 			(360 / ANGULAR_BIN_SIZE_IN_DEGREES)
#define N_ANNULAR_BINS			8 // determines the number of annuli for annular distribution calculations	

#define MAX_DISTANCE_CM_PER_MS				(200.0 / 1000.0) // 200 cm/sec this is for apparent head movements -- they can be very fast

#define TIME_TOO_LONG_TO_INTERPOLATE_MS		3000
#define TIME_FOR_SPEED_ESTIMATE_MS		1000

#define DISTANCE_TOO_FAR_TO_INTERPOLATE_CM	15.0

// Analysis method definitions
	// necessary to accomodate change from time based to frame sample based calculations
#define	UNREASONABLY_LONG_SHOCK_IN_FRAMES 50
#define	TIME_BASED		1
#define	SAMPLE_BASED		2
#define STATE_BASED		3

// Typedefs

typedef enum {
        false,
        true,
}BOOL;

typedef struct {
	int	IgnoreOnlineState;
	int	DoNotFilterTrack;
	int	DwellMap;
	int	TargetAnalysis;
	int	OverlappingTargets;
	int	Watermaze;
	int	TimeLimit;
	int	TimeSeries;
	int	OpenField;
	double	OpenFieldDimension;
	int	BsgV1;
	int	ZoneAnalysis;
} OPTIONS;

typedef struct {
	char 	dat[256];
	char 	lst[256];
	char	ps[256];
	char	tbl[256];
	char	sum[256];
	char	ta[256];
	char	tms[256];
	char	map[256];
	char	za[256];
}DIR_PATH;

typedef struct {
	int		NFiles;
	int		NSamples[MAX_FILES_TO_CONCATENATE];
	int		StartTime[MAX_FILES_TO_CONCATENATE];
	int		StopTime[MAX_FILES_TO_CONCATENATE];
	char	PSFileName[256];
	char    TAFileName[256];
	char    TMSFileName[256];
	char    ZAFileName[256];
	char    MAPFileName[256];
	char	CompositeFileName[256];
	char	FilePath[256];
	char	FullFileName[MAX_FILES_TO_CONCATENATE][256];
	char	ShortFileName[MAX_FILES_TO_CONCATENATE][256];
	char	FileFrameName[MAX_FILES_TO_CONCATENATE][10];
	char	FileNameSuffix[MAX_FILES_TO_CONCATENATE][10];
	int	FileFormat;
	long	DataEndOffset;
} FILEINFO;

typedef struct{
	int	n;
	int	*StartSample;
	int	*EndSample;
} TRIALS;

typedef struct {
	int	framecount;
	int	time;
	int	x;
	int	y;
	int	x2;
	int	y2;
	float	angle;
	double rad;
	double ang;
	int	Sector;
	int event;
	int Feeder;
	int Motor;
	int	State;
	int	ShockLevel;
	int	Flag;
	int	FrameInfo;
	TRIALS	Trials;
} TRACK;

typedef struct{
	int	CalculationMethod;
	int	Latency;
	int	Duration;
	int	MinISI;
	int	ResetInterval;
} SHOCK;

typedef struct{
	int Mode;
	float	Trigger;
	float	Refractory;
} FEEDER;

typedef struct{
	float	Ang;
	float	Width;
	float	InRad;
	float	OutRad;
} SECTOR;

typedef struct{
	float	X;
	float	Y;
	float	Rad;
} CIRCLE;

typedef struct {
	char			Show;
	char			Type;
	unsigned char	CoordFrame;
	SECTOR			Sector;
	CIRCLE			Circle;
} REINFORCED_PLACE;

typedef struct {
	unsigned char	Type;
	float				Left;
	float				Top;
	float				Right;
	float				Bottom;
	float				CenterX;
	float				CenterY;
	float				RadiusPix;
	float				Radius;
} ARENA;

typedef struct {
	int	Day;
	int	Month;
	int	Year;
} DATE;

typedef struct {
	char	ControlFile[256];
	char	ParametricFile[256];
	char	Paradigm[256];
	char	Frame[256];
	int			ReinforcedAreaDefined;
	int			FrameTypeOfData;
	REINFORCED_PLACE	Target[2];	// Room frame = 0, Arena Frame = 1
	ARENA			Arena;
	unsigned char		NumberOfCoordFrames;
	int			NumberOfSamps;
	int			NSamps[MAX_FILES_TO_CONCATENATE];
	int			TimeStep;
	double			PixelsPerCM;
	SHOCK			Shock;
	FEEDER			Feeder[2];	// Room frame = 0, Arena Frame = 1
	int			FileFormat;  // DTtracker = 0, FGX = 1, iTrack = 2, Tracker = 3
	float			FileFormatVersion;
	char			FileFormatReleaseDate[12];
	int			FileParameters;  // # of columns in data file
	DATE			Date;
	int			ParadigmType;
	OPTIONS			Options;
	int			NumberOfInitialSampsExcluded;
	int			TrialNumber;
	int			TrialSubjectId;
	char			TrialStartLabel[256];

} INFO;

typedef struct {
        double Bin[N_ANGULAR_BINS];
        double Limit;
	double	Min;
	double	MinNonZero;
	double 	Avg;
	double	Max;
	double	SD;
	double	Skewness;
	double	Kurtosis;
	double	BinSzDeg;
	double	RayleighLength;
	double	RayleighAng;
        int     NBins;
        int     MinBin;
        int     MinNonZeroBin;
        int     LoMinBin;
        int     HiMinBin;
        int     MaxBin; 
        int     LoMaxBin;
        int     HiMaxBin;
}POLAR; 

typedef struct {
    	double 	Bin[N_ANNULAR_BINS];
	double	BinAvgCM[N_ANNULAR_BINS];
	double	Min;
	double	MinNonZero;
	double 	Avg;
	double	Max;
	double	SD;
	double	Skewness;
	double	Kurtosis;
	double 	BinSzPix;
	double	BinSzCM;
	double	MinRad;
	double	MaxRad;
 	int     NBins;
    	double  MinBin;
    	double  MinNonZeroBin;
	double  MaxBin; 
}ANNULAR;

typedef struct {
	int 	LineCrossings;
	int 	TimeStep;
	double	Xbins;
	double	Ybins;
}OFIELD;

typedef struct {
	char	FileName[50];
	char	FrameName[50];
	int	frame;
	double	PathLength;
	double	Linearity;
	double	Speed;
	double	sdSpeed;
	double	PathToFirstEntrance;
	float	TimeToFirstEntrance;
	double	PathToFirstShock;
	float	TimeToFirstShock;
	double	PathToSecondEntrance;
	float	TimeToSecondEntrance;
	double	PathToSecondShock;
	float	TimeToSecondShock;
	double	SpeedAtFirstEntrance;
	double	SpeedAtSecondEntrance;
	int	NumberOfShocks;
	int	Entrances;
	float	Duration;
	float	DwellMap[TRACKER_XY_RESOLUTION * TRACKER_XY_RESOLUTION];
	int	Misses;
	int	NumberOfSamples;
	float	MaxTimeAvoided;
	double	MaxPathAvoided;
	int	EntrancesTARG;
	int   	EntrancesCW;
	int   	EntrancesCCW;
	int   	EntrancesOPP;
	double	TimeEntranceTARG;
	double	TimeEntranceCW;
	double	TimeEntranceCCW;
	double	TimeEntranceOPP;
	double	pTimeEntranceTARG;
	double	pTimeEntranceCW;
	double	pTimeEntranceCCW;
	double	pTimeEntranceOPP;
	double	QuadrantTimeTARG;
	double	QuadrantTimeCW;
	double	QuadrantTimeCCW;
	double	QuadrantTimeOPP;
	double	pQuadrantTimeTARG;
	double	pQuadrantTimeCW;
	double	pQuadrantTimeCCW;
	double	pQuadrantTimeOPP;
	POLAR	Polar;
	ANNULAR	Annular;
	double	pTargetSize;
	double	pZoneSize;
	double	EntranceTARG;
	double	EntranceCW;
	double	EntranceCCW;
	double	EntranceOPP;
	OFIELD	OField;
} MEASURES;

typedef enum {
        out,
        in_wait,
        in_feed,
        in_fed,
        out_refractory,
} FEEDER_STATE;

typedef struct{
        unsigned int m_NumberOfEntrances;
        unsigned long m_StopWait;
        unsigned long m_FeederEntranceLatency;
        unsigned long m_StopRefractory;
        unsigned long m_FeederExitLatency;
        FEEDER_STATE m_Food;
} PPInfo;

typedef struct{
	int	FrameCount;
	int	msTimeStamp;
	int	RoomX;
	int	RoomY;
	int	ArenaX;
	int	ArenaY;
	int	Angle;
	int	Sectors;
	int	State;
	int	AvoidanceState;
	int	PreferenceState;
	int	DecideState;
	int	CurrentLevel;
	int	FrameInfo;
	int	Flags;
	int	MotorState;
	int	FeederState;
} PARAMETER_INDEX;


// Globals
extern char     *optarg;
extern int      optind;


// function prototypes
		
char	*GetLine(FILE *f, char *s);

FILE	*openfile(char *, char *);
FILE	*OpenPSFile(char *, float);

int		TrackAnalysis(int, DIR_PATH, char **, FILEINFO *, OPTIONS);
int		GetFileNames(DIR_PATH *, char ***, FILEINFO **, OPTIONS);
int		GetBSGv1FileNames(DIR_PATH *, char ***, FILEINFO **, OPTIONS);
int		GetBSGv2FileNames(DIR_PATH *, char ***, FILEINFO **, OPTIONS);
int		GetChars(char *);
int     	GetNextTrackSize(FILEINFO *, INFO *, int);
int		GetTrackSize(char *, char **, int, FILEINFO *, INFO *, int);
int		GetTrack(char *, char **, int, FILEINFO *, TRACK **, INFO *, int, OPTIONS);
int		GetTrackerVersion(FILE *, INFO *, long);
int		FindKeyWord(FILE *, long, char *, char *, char *);
int		Comment(FILE *, char *);
int		SectionEnd(char *);
int		GetType(char *);
int		GetASCIIEncoding(char *, char ***);
int 		CalculateDwellMap(TRACK *, int, int, MEASURES *);
int 		CalculatePolarTrack(TRACK *, int, int, INFO);
int		CountFilesInLine(char *);
int		GetStringInQuotes(char *, char *, int *);

long	ReadHeader(FILE *, INFO *, PARAMETER_INDEX *);
long	ReadNextHeader(FILE *, INFO *, PARAMETER_INDEX *);
long	ReadHeaderTracker(FILE *, INFO *, long, PARAMETER_INDEX *);
long	ReadHeaderITrack(FILE *, INFO *, long, PARAMETER_INDEX *);
long	ReadHeaderFGX(FILE *, INFO *, long, PARAMETER_INDEX *);
long	ReadHeaderDTtracker(FILE *, INFO *, long);
long	FindNextHeaderStart(FILE *);
long	FindHeaderStart(FILE *);
long	FindHeaderEnd(FILE *);
long	FindSection(FILE *, long, char *);
long	FindSectionEnd(FILE *);

void	instruct();
void	DrawTrack(FILE *, TRACK *, char *, int, int, int, INFO, int, int, FILEINFO);
void	DrawWatermazeTrack(FILE *, TRACK *, char *, int, int, int, INFO, int, int, FILEINFO, float);
void	DrawPolarAvoidanceHistogram(FILE *, INFO, POLAR, int, int, int);
void	DrawPolarHistogram(FILE *, INFO, POLAR, int, int, int, float, float);
void    DrawAnnularHistogram(FILE *, INFO, ANNULAR, int, int, float);
void	DrawAnEvent(FILE *, float, float, RGBCOLOR *, int, int);
void	PathLengthMeasures(TRACK *, int, int, INFO, MEASURES *);
void	PrintTargetQuadrantAnalysis(FILE *, MEASURES, INFO);
void	PrintZoneQuadrantAnalysis(FILE *, MEASURES, INFO);
void	PrintTargetAnalysisHeadings(FILE *);
void	PrintTargetAnalysis(FILE *, MEASURES, INFO);
void	PrintZoneAnalysisHeadings(FILE *);
void	PrintZoneAnalysis(FILE *, MEASURES, INFO);
void	SetRGBColors (RGBCOLOR *);
void	PP_StateEntranceMeasures(TRACK *, int, int, INFO, unsigned char **, MEASURES *, int);
void	PP_TimeBasedEntranceMeasures(TRACK *, int, int, INFO, unsigned char **, MEASURES *, int);
void	PA_TimeBasedEntranceMeasures(TRACK *, int, int, INFO, unsigned char **, MEASURES *, int);
void	PA_SampleBasedEntranceMeasures(TRACK *, int, int, INFO, unsigned char **, MEASURES *, int);
void	PA_StateBasedEntranceMeasures(TRACK *, int, int, INFO, unsigned char **, MEASURES *, int);
void	GetParameterIndex(char **, int, PARAMETER_INDEX *);

void	CalculateDistanceFromShockZone(FILE *, TRACK *, int, int, INFO, unsigned char **);
void 	CalculatePolarDistribution(TRACK *, int, int, POLAR *, float);
void	CalculateRayleighVector(TRACK *, int, int, POLAR *, float);
void 	CalculateAnnularDistribution(TRACK *, int, int, INFO, ANNULAR *);
void    CharacterizeDistribution(TRACK *, int, int, int, double *, double *, double *, double *);
void	CalculateLineCrossings(TRACK *, int, int, INFO, OFIELD *);
void    CalculateTimePerArea(TRACK *, int, int, unsigned char **, MEASURES *);
void	MakeAvoidCircleMap(unsigned char ***, INFO, CIRCLE, int);
void	MakeAvoidSectorMap(unsigned char ***, INFO, SECTOR);
void	MakePreferenceMap(unsigned char ***, INFO, CIRCLE, int);
void	MakeNullSectorMap(unsigned char ***);
void	MakeQuadrantMap(unsigned char ***, INFO, double);
void	PrintPlaceAvoidanceTextReport(FILE *, MEASURES, INFO);
void	PrintPlaceAvoidancePSReport(FILE *, MEASURES, INFO);
void	PrintPlaceAvoidanceTableHeadings(FILE *);
void	PrintPlaceAvoidanceTableReport(FILE *, MEASURES);
void	PrintPlacePreferenceTextReport(FILE *, MEASURES, INFO);
void	PrintPlacePreferencePSReport(FILE *, MEASURES, INFO);
void	PrintPlacePreferenceTableHeadings(FILE *);
void	PrintPlacePreferenceTableReport(FILE *, MEASURES);
void	PrintWatermazeTextReport(FILE *, MEASURES, INFO);
void	PrintWatermazePSReport(FILE *, MEASURES, INFO, float);
void	PrintWatermazePSLegend(FILE *, MEASURES, INFO, float);
void	PrintWatermazeTableHeadings(FILE *);
void	PrintWatermazeTableReport(FILE *, MEASURES, INFO);
void	PrintTrackerTextReport(FILE *, MEASURES, INFO);
void	PrintTrackerPSReport(FILE *, MEASURES, INFO);
void	PrintTrackerTableHeadings(FILE *);
void	PrintTrackerTableReport(FILE *, MEASURES);
void	PrintOpenFieldTextReport(FILE *, MEASURES, INFO);
void	PrintOpenFieldPSReport(FILE *, MEASURES, INFO);
void    PrintOpenFieldTableHeadings(FILE *);
void	PrintOpenFieldTableReport(FILE *, MEASURES);

void	ExcludeFirstNoDetects(TRACK **, INFO *);
void 	ExcludeExtraSamples(TRACK **, INFO *);
void	FilterTrack(TRACK **, INFO *);
void    GetMinimumPolarLimits(POLAR *, float);
void    GetMaximumPolarLimits(POLAR *, float);
void 	QuadrantTargetAnalysis(TRACK *, INFO *, MEASURES *, int, OPTIONS);
void 	QuadrantZoneAnalysis(TRACK *, INFO *, MEASURES *, int, OPTIONS);
void 	TargetAnalysis(TRACK *, INFO *, MEASURES *, int, OPTIONS);
void 	ZoneAnalysis(TRACK *, INFO *, MEASURES *, int, OPTIONS);

FEEDER_STATE DecideFood(unsigned long, BOOL, PPInfo *);

