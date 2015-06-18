# macro definitions

PROG = /usr/local/bin/TrackAnalysis
LIBS = -lm
OPTIM = -O2

OBJS =\
DecideFood.o\
DwellMap.o\
DrawTrack.o\
EntranceCalculations.o\
GetLine.o\
GetTrack.o\
Instruct.o\
OpenField.o\
OpenPSFile.o\
PolarCalculations.o\
ReadHeader.o\
TrackAnalysis.o\
TrackHandler.o\
Utilities.o

# program dependencies

$(PROG):$(OBJS)
	cc $(OPTIM) -o $(PROG) $(OBJS) $(LIBS)


# inference rule

.c.o:
	cc -c $(OPTIM) $*.c
	chmod a+rw $*.o


# function dependencies

DecideFood.o:		Track.h
DwellMap.o:		Track.h
DrawTrack.o:		Track.h Postscript.h
EntranceCalculations.o:	Track.h Header.h
GetLine.o:		
GetTrack.o:		Track.h Header.h
Instruct.o:		Track.h Header.h
LineCrossings.o:	Track.h
OpenField.o:		Track.h
OpenPSFile.o:		Track.h Postscript.h
PolarCalculations.o:	Track.h
ReadHeader.o:		Track.h Header.h
TrackAnalysis.o:	Postscript.h Track.h Header.h
TrackHandler.o:		Postscript.h Track.h Header.h
Utilities.o:		Track.h Header.h
