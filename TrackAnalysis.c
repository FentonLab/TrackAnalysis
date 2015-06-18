// 23-02-09 added the option to do ZoneAnalyses with Place Avoidance data 
// 11-08-07 added the possibility that the Place Avoidance target is a circle 
// 10-06-12 added calculations of speed at times of first and second entrances to PA_StateBasedEntranceMeasures()
// 14.08.12 completed calculations of speed at times of first and second entrances to PA_StateBasedEntranceMeasures()
// 01.05.13 added calculation of Rayleigh vector function: CalculateRayleighVector()
// 25-06-13 modified GetTrack to accomodate Angle data in the Data samples, which are floats. Modifucation will accomodate any data that are represented as real nuumbers.
// 13-08-13 Added the option and function DwellMap to generate a 1-D map of where the subject spent time. 
// 23.08.13 I->Target[Frame].Type = REINFORCED_SECTOR; // 23.08.13 added this because ReinforcedSector is used subsequently even if not positions are defined
// 23.08.13 also changed I->Target[0].Type = REINFORCED_SECTOR; to I->Target[Frame].Type = REINFORCED_SECTOR;  in several places that are flagged with the 23.08.13 data ena a comment
// 21-03-14 Corrected bug in handling concatenated .dat files. Added ability to count number of samples in each file by storing to FILEINFO NSamples[FileNumber]. Modified GetTrackSize() in GetTrack.c Also modified GetTrack.c to count data samples form the current file i.e. FileSamples and count data samples for the curent analysis DataSamples.


#include "Track.h"
#include "Postscript.h"

int	TrackAnalysis(int NumberOfFiles, DIR_PATH DirPath, char **FileNames, FILEINFO *FileInfo, OPTIONS Options)
{
	int	i, j, p, frame;
	FILE	*sfp, *tbl, *psfp, *tafp, *dfp, *zafp, *mapfp;

	char	File[256];	/* text buffer		*/
	int	DrawEvents, PointsPerPixel = 1, NFilesRead, FirstPlaceAvoidanceFile, FirstPlacePreferenceFile, FirstOpenFieldFile, FirstWatermazeFile, FirstTrackerFile;
	int	NumberOfCoordFrames;

	INFO		*I;
	MEASURES	*m, m_quad;
	TRACK 		*Track;
	unsigned char	**ReinforcedMap;
	float		DrawTrackScale = 1.0;
	int		Page, FilesDrawn, FilesDrawnOnPage, FilesInRow, FilesInColumn, MaxFilesInRow, MaxFilesInColumn, n_pixels;
	int		PSx, PSy, GraphicX, GraphicY;
	float		ArenaRadiusInPix = DEFAULT_ARENA_RADIUS;

	sfp = tbl = psfp = tafp = zafp = mapfp = NULL;	// initialize

	sfp = openfile(FileNames[0], "a");	/* opens the non-table output file	*/
	tbl = openfile(FileNames[1], "w");	/* opens the table output file		*/
	if((sfp == NULL) || (tbl == NULL)){
		fprintf(MyStdOut,"Output file is probably opened by another program.\nClose it and rerun TrackAnalysis. Quitting\n");
		return(0);
	}
	fprintf (MyStdOut, "processing %d data files...", NumberOfFiles - 2);
	NFilesRead = 0;
	FirstPlaceAvoidanceFile = FirstPlacePreferenceFile = FirstOpenFieldFile = FirstWatermazeFile = FirstTrackerFile = 1;

	Track = (TRACK *)malloc(sizeof(TRACK));

	for (i = 2; i < NumberOfFiles; i++){
		fprintf (MyStdOut, "\nFile %d ... %s", i - 1, FileNames[i]);	// Console feedback
		for(j = 1; j < FileInfo[i].NFiles; j++){
			fprintf(MyStdOut, " %s", FileNames[i + j]);
		}
		fflush(MyStdOut);

		// Make postscript graphics

		sprintf(File, "%s.ps", FileInfo[i].PSFileName);
		psfp = OpenPSFile(File, (float)PointsPerPixel);
		if(psfp == NULL){
			fprintf(MyStdOut, "\nCan't open PostScript file: %s\n", File);
		}
			
		if(Options.TargetAnalysis){
			sprintf(File, "%s.tbl", FileInfo[i].TAFileName);
			tafp = fopen(File, "w");
			if(tafp == NULL){
				fprintf(MyStdOut, "\nCan't open Target Analysis file: %s\n", File);
			}
		}
		if(Options.TimeSeries){
			sprintf(File, "%s.tms", FileInfo[i].TMSFileName);
			dfp = fopen(File, "w");
			if(dfp == NULL){
				fprintf(MyStdOut, "\nCan't open Distance to Shock output file: %s\n", File);
			}
		}

		if(Options.DwellMap){
			sprintf(File, "%s.map", FileInfo[i].MAPFileName);
			dfp = fopen(File, "w");
			if(dfp == NULL){
				fprintf(MyStdOut, "\nCan't open Dwell Map output file: %s\n", File);
			}
		}

		NumberOfCoordFrames = 1;	// set to 1 to start

		// Load the track data into memory
		I = (INFO *)calloc(1, sizeof(INFO));
		if(I == NULL){
			fprintf(MyStdOut,"\nCan't calloc Info\n"); fflush(MyStdOut);
			return(0);
		}
		I->Options = Options;
		for(frame = 0; frame < NumberOfCoordFrames; frame++){
			FileInfo[i].DataEndOffset = 0;
			NFilesRead = GetTrackSize(DirPath.dat, FileNames, i, &(FileInfo[i]), I, frame);
			if(NFilesRead == 0){
				fprintf(MyStdOut,"\nGetTrackSize returned 0 files.\n");
				continue;
				return(0);
			}
			NumberOfCoordFrames = I->NumberOfCoordFrames;

			if(Options.TargetAnalysis)
				PrintTargetAnalysisHeadings(tafp);

			if(Options.ZoneAnalysis){
				if(I->Target[frame].Type != REINFORCED_SECTOR){
					fprintf(MyStdOut, "\nThe ZoneAnalysis option is only available for Sector-defined shock zones. Skipping the ZoneAnalysis for this file: %s\n", File);
				}else{
					sprintf(File, "%s.tbl", FileInfo[i].ZAFileName);
					zafp = fopen(File, "w");
					if(zafp == NULL){
						fprintf(MyStdOut, "\nCan't open Zone Analysis file: %s\n", File);
					}
					PrintZoneAnalysisHeadings(zafp);
				}
			}

			// GetTrackSize reads the file header and thus the Paradigm Type of the data file
	        	switch(I->ParadigmType){
				case PLACE_AVOIDANCE:
					if(FirstPlaceAvoidanceFile){
						PrintPlaceAvoidanceTableHeadings(tbl);
						FirstPlaceAvoidanceFile = 0;
						MaxFilesInRow = 1;
						MaxFilesInColumn = 1;
					}
					break;

				case PLACE_PREFERENCE:
					if(FirstPlacePreferenceFile){
						PrintPlacePreferenceTableHeadings(tbl);
						FirstPlacePreferenceFile = 0;
						MaxFilesInRow = 1;
						MaxFilesInColumn = 1;
					}
					break;

				case OPEN_FIELD:
					if(FirstOpenFieldFile){
						PrintOpenFieldTableHeadings(tbl);
						FirstOpenFieldFile = 0;
						MaxFilesInRow = 1;
						MaxFilesInColumn = 1;
					}
					break;

				case WATER_MAZE:
                    			if(FirstWatermazeFile){
						PrintWatermazeTableHeadings(tbl);
                        			FirstWatermazeFile = 0;
						MaxFilesInRow = 4;
						MaxFilesInColumn = 2;
                    			}
                    			break;

				case TRACKER:
					if(FirstTrackerFile){
						PrintTrackerTableHeadings(tbl);
						FirstTrackerFile = 0;
						MaxFilesInRow = 1;
						MaxFilesInColumn = 1;
					}
					break;
				default:
               				break;
			}
			
			// data files may be organized as concatenated trials
			// loop and do this stuff for each trial
			// go through the loop at least once since there will be data for at least one track

			FilesDrawn = FilesDrawnOnPage = FilesInRow = FilesInColumn = 0;
			Page = 1;
			PSx = (int)PS_X_MIN;
			PSy = (int)PS_Y_MAX; 
			do{
				Track = (TRACK *)realloc(Track, (size_t)I->NumberOfSamps * sizeof(TRACK));
				if (Track == NULL){
					fprintf(MyStdOut, "\nCannot allocate memory for the track data\n");
					exit(-1);
				}
			
				m = (MEASURES *)calloc(1, sizeof(MEASURES));
	       			if(m == NULL){
					fprintf(MyStdOut,"\nCan't calloc measures\n"); fflush(MyStdOut);
					return(0);
				}
			
				if(NFilesRead == FileInfo[i].NFiles){
					m->frame = frame;
					// sprintf(m->FileName, "%s", FileNames[i]);
					sprintf(m->FileName, "%s", FileInfo[i].ShortFileName[0]);
					for(j = 1; j < FileInfo[i].NFiles; j++){
						strcat(m->FileName, "-");
						// strcat(m->FileName, FileNames[i + j]);
						strcat(m->FileName, FileInfo[i].ShortFileName[j]);
					}
					fprintf(sfp, "\t\t--------------------------------\nFile: %s\n",m->FileName);

				} else{
					//sprintf(m->FileName,"%s", FileNames[i]);
					sprintf(m->FileName, "%s", FileInfo[i].ShortFileName[0]);
					for(j = 1; j < FileInfo[i].NFiles; j++){
						strcat(m->FileName, "-");
						//strcat(m->FileName, FileNames[i + j]);
						strcat(m->FileName, FileInfo[i].ShortFileName[j]);
					}
					strcat(m->FileName, NO_FILE_TEXT);
					fprintf(sfp, "\t\t--------------------------------\nFile: %s\n",m->FileName);
					fprintf (MyStdOut, "%s", NO_FILE_TEXT);	// Console feedback
	
					goto PRINT_RESULTS;
				}
				if(!GetTrack(DirPath.dat, FileNames, i, &(FileInfo[i]), &Track, I, frame, Options)){
					fprintf(MyStdOut,"\nCan't get data from data file: %s. Skipping.\n", m->FileName);
					break;
				}
	
				if(Options.Watermaze){
					ExcludeFirstNoDetects(&Track, I);
					ExcludeExtraSamples(&Track, I);
				}
	
				FileInfo[i].FileFormat = I->FileFormat;	// these two structures hold similar info 
		
				if(!Options.DoNotFilterTrack)
					FilterTrack(&Track, I);
	
				if(Options.DwellMap){
					CalculateDwellMap(Track, 0, I->NumberOfSamps, m);
					sprintf(File, "%s.map", FileInfo[i].MAPFileName);
                                        mapfp = fopen(File, "w");
                                        if(mapfp == NULL){
                                                fprintf(MyStdOut, "\nCan't open Dwell Map file: %s\n", File);
                                        }
					n_pixels = TRACKER_XY_RESOLUTION * TRACKER_XY_RESOLUTION;
					for(p = 0; p < n_pixels; p++){
						fprintf(mapfp,"%0.3f\n",m->DwellMap[p]);	
					}
					fclose(mapfp);
				}

				if(CalculatePolarTrack(Track, 0, I->NumberOfSamps, *I)){
		
					CalculatePolarDistribution(Track, 0, I->NumberOfSamps, &(m->Polar), I->Target[frame].Sector.Ang);

					CalculateRayleighVector(Track, 0, I->NumberOfSamps, &(m->Polar), I->Target[frame].Sector.Ang);
	
				 	CalculateAnnularDistribution(Track, 0, I->NumberOfSamps, *I, &(m->Annular));
				}
				if(I->Options.OpenField){
					m->OField.Xbins = m->OField.Ybins = I->Options.OpenFieldDimension;
					m->OField.TimeStep = DEFAULT_SAMPLING_INTERVAL;
					CalculateLineCrossings(Track, 0, I->NumberOfSamps, *I, &(m->OField));
				}
	
				// Calculate measures of movement 
				PathLengthMeasures(Track, 0, I->NumberOfSamps, *I, m);
	
				// Calculate entrances etc.
				switch(I->ParadigmType){
					case PLACE_AVOIDANCE:
						if(I->Target[frame].Type == REINFORCED_SECTOR){
							MakeAvoidSectorMap(&ReinforcedMap, *I, I->Target[frame].Sector);
						}else if(I->Target[frame].Type == REINFORCED_CIRCLE){
							MakeAvoidCircleMap(&ReinforcedMap, *I, I->Target[frame].Circle, 1);
						}
	
						if(Options.IgnoreOnlineState){
							PA_TimeBasedEntranceMeasures(Track, 0, I->NumberOfSamps, *I, ReinforcedMap, m, frame);
							break;
						}
						// This is necessary because online time based calculations were changed to frame based calculations
						if(I->Shock.CalculationMethod == TIME_BASED){
							PA_TimeBasedEntranceMeasures(Track, 0, I->NumberOfSamps, *I, ReinforcedMap, m, frame);
						}else if(I->Shock.CalculationMethod == STATE_BASED){
						// 	if((I->FileFormat == FGX_FORMAT) || (I->FileFormat == ITRACK_FORMAT) || (I->FileFormat == TRACKER_FORMAT)){
							PA_StateBasedEntranceMeasures(Track, 0, I->NumberOfSamps, *I, ReinforcedMap, m, frame);
						}else{	// THIS IS THE DEFAULT METHOD FOR WHEN ONLINE STATES CAN'T BE TRUSTED
							PA_SampleBasedEntranceMeasures(Track, 0, I->NumberOfSamps, *I, ReinforcedMap, m, I->FrameTypeOfData);
						}
						if(Options.TimeSeries){
							CalculateDistanceFromShockZone(dfp, Track, 0, I->NumberOfSamps, *I, ReinforcedMap);
							fclose(dfp);
						}
						if(Options.ZoneAnalysis){
							if(I->Target[frame].Type == REINFORCED_SECTOR){
								for(m->pZoneSize = 0.0; m->pZoneSize < 1.0; m->pZoneSize += 0.25){
									ZoneAnalysis(Track, I, m, frame, Options);
									PrintZoneAnalysis(zafp, *m, *I);
								}
								QuadrantZoneAnalysis(Track, I, m, frame, Options);
	
								m->pZoneSize = -1.0;
								PrintZoneQuadrantAnalysis(zafp, *m, *I);
								m_quad = *m;

								fclose(zafp);
							}
						}
	
						break;
					case PLACE_PREFERENCE:
					case WATER_MAZE:
						if(Options.TargetAnalysis){
							for(m->pTargetSize = 0.5; m->pTargetSize <= 3.0; m->pTargetSize += 0.5){
								TargetAnalysis(Track, I, m, frame, Options);
								PrintTargetAnalysis(tafp, *m, *I);
							}

							QuadrantTargetAnalysis(Track, I, m, frame, Options);
	
							m->pTargetSize = -1.0;
							PrintTargetQuadrantAnalysis(tafp, *m, *I);
							m_quad = *m;

							// fclose(tafp);
						}
	
						// count entrances to each quadrant's target area
						m->pTargetSize = 1.0;
						TargetAnalysis(Track, I, m, frame, Options);
	
						MakePreferenceMap(&ReinforcedMap, *I, I->Target[frame].Circle, Options.OverlappingTargets);
	
						if(Options.IgnoreOnlineState){
							PP_TimeBasedEntranceMeasures(Track, 0, I->NumberOfSamps, *I, ReinforcedMap, m, frame);
						}else{
							PP_StateEntranceMeasures(Track, 0, I->NumberOfSamps, *I, ReinforcedMap, m, frame);
						}
						break;
					default:
						break;
				}
	
				switch(I->ParadigmType){
					case PLACE_AVOIDANCE:
						if(I->Target[frame].Type == REINFORCED_SECTOR)
							MakeAvoidSectorMap(&ReinforcedMap, *I, I->Target[frame].Sector);
						else if(I->Target[frame].Type == REINFORCED_CIRCLE){
							MakeAvoidCircleMap(&ReinforcedMap, *I, I->Target[frame].Circle, 1);
						}else if(!I->ReinforcedAreaDefined){
							MakeNullSectorMap(&ReinforcedMap);
						}
						break;
					
					case OPEN_FIELD:
					case TRACKER:
						MakeAvoidSectorMap(&ReinforcedMap, *I, I->Target[frame].Sector);
						break;
	
					case PLACE_PREFERENCE:
					case WATER_MAZE:
						MakePreferenceMap(&ReinforcedMap, *I, I->Target[frame].Circle, 1);
						break;
					default:
						break;
				}
	
				CalculateTimePerArea(Track, 0, I->NumberOfSamps, ReinforcedMap, m);
	
				if(psfp != NULL){
					char	Label[256];
					sprintf(Label,"File: %s", FileInfo[i].CompositeFileName);

					// Draw track and graphs
					// Note coordinate system for each draw function is relative to the page
					switch(I->ParadigmType){
       	            			case PLACE_AVOIDANCE:
						DrawTrackScale = 1.0;
						PSx = (int)PS_X_MIN;
						PSy = (int)PS_Y_MAX; 
						GraphicX = PSx;
						GraphicY = PSy;
						DrawEvents = 1; // draw a mark for each event
						DrawTrack(psfp, Track, Label, PointsPerPixel, GraphicX, GraphicY, *I, DrawEvents, frame, FileInfo[i]);
						GraphicX += ((PS_X_OFFSET + I->Arena.CenterX * DrawTrackScale + 20) * PointsPerPixel);
						GraphicY -= (I->Arena.CenterX / DrawTrackScale * PointsPerPixel);
						DrawPolarAvoidanceHistogram(psfp, *I, m->Polar, GraphicX, GraphicY, PointsPerPixel);
						GraphicX -=  (I->Arena.CenterX / DrawTrackScale - 30 / DrawTrackScale);
						GraphicY -=  (3.6 * I->Arena.CenterX / DrawTrackScale);
						DrawAnnularHistogram(psfp, *I, m->Annular, GraphicX, GraphicY, 1.0);

						// set coordinate system for report text
						GraphicX = PSx;
						GraphicY = PSy - (2.4 * I->Arena.CenterX / DrawTrackScale);
						fprintf(psfp,"gsave\n");
						(void) fprintf(psfp, "%d %d translate\n", GraphicX, GraphicY); 
						PrintPlaceAvoidancePSReport(psfp, *m, *I);
						fprintf(psfp,"grestore\n");
						break;
					case PLACE_PREFERENCE:
						DrawTrackScale = 1.0;
						PSx = (int)PS_X_MIN;
						PSy = (int)PS_Y_MAX; 
						GraphicX = PSx;
						GraphicY = PSy;
						DrawEvents = 1; // draw a mark for each event
						DrawTrack(psfp, Track, Label, PointsPerPixel, GraphicX, GraphicY, *I, DrawEvents, frame, FileInfo[i]);
						GraphicX += ((PS_X_OFFSET + I->Arena.CenterX * DrawTrackScale + 20) * PointsPerPixel);
                                                GraphicY -= (I->Arena.CenterX / DrawTrackScale * PointsPerPixel);
						DrawPolarHistogram(psfp, *I, m->Polar, GraphicX, GraphicY, PointsPerPixel, DrawTrackScale, 1.0);
						GraphicX -=  (I->Arena.CenterX / DrawTrackScale - 30 / DrawTrackScale);
                                                GraphicY -=  (3.6 * I->Arena.CenterX / DrawTrackScale);
						DrawAnnularHistogram(psfp, *I, m->Annular, GraphicX, GraphicY, 1.0);

						// set coordinate system for report text
						GraphicX = PSx;
						GraphicY = PSy - (2.4 * I->Arena.CenterX / DrawTrackScale);
						fprintf(psfp,"gsave\n");
						(void) fprintf(psfp, "%d %d translate\n", GraphicX, GraphicY); 
						PrintPlacePreferencePSReport(psfp, *m, *I);
						fprintf(psfp,"grestore\n");
						break;
					case WATER_MAZE:
						DrawTrackScale = 4.0;
						PSx = (int)(PS_X_MIN + FilesInRow * (3 * ArenaRadiusInPix / DrawTrackScale)); 
						PSy = PS_Y_MAX - FilesInColumn * (10.0 * ArenaRadiusInPix / DrawTrackScale);  
						GraphicX = PSx + I->Arena.CenterX * PointsPerPixel;
						GraphicY = PSy;
						DrawEvents = 1; // draw a mark for each event
						if(!FilesInRow && !FilesInColumn){
							sprintf(Label,"File: %s (page %d)", FileInfo[i].CompositeFileName, Page);
						}else{
							sprintf(Label,"%s","");
						}
						DrawWatermazeTrack(psfp, Track, Label, PointsPerPixel, GraphicX, GraphicY, *I, DrawEvents, frame, FileInfo[i], DrawTrackScale);
						GraphicY -= (int)(6.3 * ArenaRadiusInPix / DrawTrackScale);
						DrawPolarHistogram(psfp, *I, m->Polar, GraphicX, GraphicY, PointsPerPixel, DrawTrackScale, 1.0);

						GraphicY -= (int)(4 * ArenaRadiusInPix / DrawTrackScale);
						DrawAnnularHistogram(psfp, *I, m->Annular, (GraphicX - 1.0 * ArenaRadiusInPix/DrawTrackScale), GraphicY, DrawTrackScale);

						// set coordinate system for report text
						fprintf(psfp,"gsave\n");
						(void) fprintf(psfp, "%d %d translate\n", GraphicX - 15, GraphicY);
						if(!FilesInRow)
							PrintWatermazePSLegend(psfp, *m, *I, DrawTrackScale);
						PrintWatermazePSReport(psfp, *m, *I, DrawTrackScale);
						fprintf(psfp,"grestore\n");
						break;
					case TRACKER:
						DrawTrackScale = 1.0;
						PSx = (int)PS_X_MIN;
						PSy = (int)PS_Y_MAX; 
						GraphicX = PSx;
						GraphicY = PSy;
						DrawEvents = 1; // draw a mark for each event
						DrawTrack(psfp, Track, Label, PointsPerPixel, GraphicX, GraphicY, *I, DrawEvents, frame, FileInfo[i]);
						GraphicX += ((PS_X_OFFSET + I->Arena.CenterX * DrawTrackScale + 20) * PointsPerPixel);
                                                GraphicY -= (I->Arena.CenterX / DrawTrackScale * PointsPerPixel);
						DrawPolarHistogram(psfp, *I, m->Polar, GraphicX, GraphicY, PointsPerPixel, DrawTrackScale, 3.0);
						GraphicX -=  (I->Arena.CenterX / DrawTrackScale - 30 / DrawTrackScale);
                                                GraphicY -=  (3.6 * I->Arena.CenterX / DrawTrackScale);
						DrawAnnularHistogram(psfp, *I, m->Annular, GraphicX, GraphicY, 1.0);

						// set coordinate system for report text
						GraphicX = PSx;
						GraphicY = PSy - (2.4 * I->Arena.CenterX / DrawTrackScale);
						fprintf(psfp,"gsave\n");
						(void) fprintf(psfp, "%d %d translate\n", GraphicX, GraphicY); 
						PrintTrackerPSReport(psfp, *m, *I);
						fprintf(psfp,"grestore\n");
						break;
					case OPEN_FIELD:
						DrawTrackScale = 1.0;
						PSx = (int)PS_X_MIN;
						PSy = (int)PS_Y_MAX; 
						GraphicX = PSx;
						GraphicY = PSy;
						DrawEvents = 1; // draw a mark for each event
						DrawTrack(psfp, Track, Label, PointsPerPixel, GraphicX, GraphicY, *I, DrawEvents, frame, FileInfo[i]);
						GraphicX += ((PS_X_OFFSET + I->Arena.CenterX * DrawTrackScale + 20) * PointsPerPixel);
                                                GraphicY -= (I->Arena.CenterX / DrawTrackScale * PointsPerPixel);
						DrawPolarHistogram(psfp, *I, m->Polar, GraphicX, GraphicY, PointsPerPixel, DrawTrackScale, 3.0);
						GraphicX -=  (I->Arena.CenterX / DrawTrackScale - 30 / DrawTrackScale);
                                                GraphicY -=  (3.6 * I->Arena.CenterX / DrawTrackScale);
						DrawAnnularHistogram(psfp, *I, m->Annular, GraphicX, GraphicY, 1.0);

						// set coordinate system for report text
						GraphicX = PSx;
						GraphicY = PSy - (2.4 * I->Arena.CenterX / DrawTrackScale);
						fprintf(psfp,"gsave\n");
						(void) fprintf(psfp, "%d %d translate\n", GraphicX, GraphicY); 
						PrintOpenFieldPSReport(psfp, *m, *I);
						fprintf(psfp,"grestore\n");
						break;
					default:
						break;
					}
					// count the files so that multiple files can be arranged on a page
					FilesDrawn++;
					FilesDrawnOnPage++;
					FilesInRow = FilesDrawnOnPage % MaxFilesInRow;
					FilesInColumn = FilesDrawnOnPage / MaxFilesInRow;
					if(FilesInColumn == MaxFilesInColumn){
						FilesDrawnOnPage = 0;
						FilesInColumn = FilesInRow = 0;
						fprintf(psfp,"showpage\n");
						Page++;
					}
				}
	
				PRINT_RESULTS:
				switch(I->ParadigmType){
       	            			case PLACE_AVOIDANCE:
						PrintPlaceAvoidanceTextReport(sfp, *m, *I);
						fflush(sfp);
						PrintPlaceAvoidanceTableReport(tbl, *m);
						fflush(tbl);
						break;
					case PLACE_PREFERENCE:
						PrintPlacePreferenceTextReport(sfp, *m, *I);
						fflush(sfp);
						PrintPlacePreferenceTableReport(tbl, *m);
						fflush(tbl);
						break;
					case WATER_MAZE:
						PrintWatermazeTextReport(sfp, *m, *I);
						fflush(sfp);
						PrintWatermazeTableReport(tbl, *m, *I);
						fflush(tbl);
						break;
       	            			case TRACKER:
						PrintTrackerTextReport(sfp, *m, *I);
						fflush(sfp);
						PrintTrackerTableReport(tbl, *m);
						fflush(tbl);
						break;
       	            			case OPEN_FIELD:
						PrintOpenFieldTextReport(sfp, *m, *I);
						fflush(sfp);
						PrintOpenFieldTableReport(tbl, *m);
						fflush(tbl);
						break;
					default:
						break;
				}
	
				Track -= I->NumberOfInitialSampsExcluded;	// Some of the initial samples may have been excluded from analysis 

				// free and allocate these structures so the values are reinitialized for each trial
				free((void *)m);
				free((void *)I);

				I = (INFO *)calloc(1, sizeof(INFO));
				if(I == NULL){
					fprintf(MyStdOut,"\nCan't calloc Info\n"); fflush(MyStdOut);
					return(0);
				}
				I->Options = Options;

			}while(GetNextTrackSize(&(FileInfo[i]), I, 0)); // only consider single file analyses organized as trials i.e. NFiles = 1
			if(FilesInColumn)	// The ps file should end with a showpage
				fprintf(psfp,"showpage\n");
	
		} // end of frame loop
		fclose(psfp);
		if(tafp != NULL)	// user can call it for a paradigm for which it doesn't make sense e.g Avoidance
			fclose(tafp);

		if(FileInfo[i].NFiles > 1)
			i+= (FileInfo[i].NFiles - 1);	// only increment by the additional files in the list
	}

	fclose(sfp);
	fclose(tbl);

	return(1);
}

void	PrintPlaceAvoidanceTextReport(FILE *p, MEASURES m, INFO I)
{
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\t\t********************************\n\n");
		return;
	}
	
	fprintf(p, "Coordinate Frame:\t%s\n", m.FrameName);
	if(m.NumberOfSamples)
		fprintf(p, "Proportion of samples missed (no detects):\t%0.3f (%d)\n", (float)m.Misses / (float)m.NumberOfSamples, m.Misses); 
	else
		fprintf(p, "Proportion of samples missed (no detects):\t%0.3f (%d)\n", (float)(1.00), m.Misses);

	if(I.Target[m.frame].Type == REINFORCED_SECTOR){
		fprintf(p, "Reinforced Area (ang, width, inner radius, outer radius)\n");
		fprintf(p, "(%0.2f, %0.2f, %0.2f, %0.2f)\n", I.Target[m.frame].Sector.Ang, I.Target[m.frame].Sector.Width,
			I.Target[m.frame].Sector.InRad, I.Target[m.frame].Sector.OutRad );
	} else if(I.Target[m.frame].Type == REINFORCED_CIRCLE){
		fprintf(p, "Reinforced Area (CenterX, CenterY, Radius)\n");
		fprintf(p, "(%0.2f, %0.2f, %0.2f)\n", I.Target[m.frame].Circle.X, I.Target[m.frame].Circle.Y,
			I.Target[m.frame].Circle.Rad);
	}
	fprintf(p, "Shock Parameters (latency, duration, Min ISI in ms)\n");
	fprintf(p, "(%d, %d, %d)\n\n", I.Shock.Latency, I.Shock.Duration, I.Shock.MinISI);

	fprintf(p, "Total time:                        \t%d sec\n", (int)(m.Duration + 0.5));
	fprintf(p, "Total pathlength:                  \t%0.2f m\n", (float)m.PathLength);
	fprintf(p, "Number of entrances:               \t%d\n", m.Entrances);
	fprintf(p, "Time to the first entrance:        \t%0.2f sec\n", m.TimeToFirstEntrance);
	fprintf(p, "Time to the second entrance:       \t%0.2f sec\n", m.TimeToSecondEntrance);
	fprintf(p, "Pathlength to the first entrance:  \t%0.2f m\n", (float)m.PathToFirstEntrance);
	fprintf(p, "Pathlength to the second entrance: \t%0.2f m\n", (float)m.PathToSecondEntrance);
	fprintf(p, "Speed at the first entrance:       \t%0.2f cm/s\n", (float)m.SpeedAtFirstEntrance);
	fprintf(p, "Speed at the second entrance:      \t%0.2f cm/s\n", (float)m.SpeedAtSecondEntrance);
	fprintf(p, "Entrances per distance moved:      \t%0.2f 1/m\n", (float)(m.PathLength == 0.0) ? 0.0 : (m.Entrances / m.PathLength));
	fprintf(p, "Number of shocks:                  \t%d\n", m.NumberOfShocks);
	fprintf(p, "Time to the first shock:           \t%0.2f sec\n", m.TimeToFirstShock);
	fprintf(p, "Pathlength to the first shock:     \t%0.2f m\n", (float)m.PathToFirstShock);
	fprintf(p, "Speed: mean (s.d)                  \t%0.2f (%0.2f) cm/s\n", (float)m.Speed, (float)m.sdSpeed);
	fprintf(p, "Linearity (linear/integral dist)   \t%0.4f (timestep = %ds)\n", (float)m.Linearity, (int)(I.TimeStep * LINEARITY_SCALE_FACTOR/1000));
	fprintf(p, "Time (s) in Target area (T CCW OPP CW)\t(%0.3lf %0.3lf %0.3lf %0.3lf)\n", m.TimeEntranceTARG, m.TimeEntranceCCW, m.TimeEntranceOPP, m.TimeEntranceCW);
	fprintf(p, "p Time in Quadrant (T CCW OPP CW)  \t(%0.4lf %0.4lf %0.4lf %0.4lf)\n", m.pTimeEntranceTARG, m.pTimeEntranceCCW, m.pTimeEntranceOPP, m.pTimeEntranceCW);
	fprintf(p, "Max time of avoidance               \t%d sec\n", (int)m.MaxTimeAvoided);
	fprintf(p, "Max path of avoidance               \t%0.2f m\n", m.MaxPathAvoided);
	fprintf(p, "Rayleigh vector length [ang]:       \t%0.2f [%0.2f]\n", m.Polar.RayleighLength, m.Polar.RayleighAng);
	fprintf(p, "Polar histogram measures (%d deg bins)\n", (int)m.Polar.BinSzDeg);
	fprintf(p, "Average [sd]:                       \t%0.2f [%0.2f]\n", m.Polar.Avg, m.Polar.SD);
	fprintf(p, "Minimum value [0-1]:                \t%0.4f\n", m.Polar.Min);
	fprintf(p, "Minimum bin (0 = shock sector):     \t%d deg\n", (int)(m.Polar.MinBin * m.Polar.BinSzDeg));
	fprintf(p, "50%% time in the range (incl min bin):\t[%d %d] degs\n", (int)(m.Polar.LoMinBin * m.Polar.BinSzDeg), (int)(m.Polar.HiMinBin * m.Polar.BinSzDeg));
	fprintf(p, "Maximum value [0-1]:                \t%0.4f\n", m.Polar.Max);
	fprintf(p, "Maximum bin (0 = shock sector):     \t%d deg\n", (int)(m.Polar.MaxBin * m.Polar.BinSzDeg));
	fprintf(p, "50%% time in the range (incl max bin):\t[%d %d] degs\n", (int)(m.Polar.LoMaxBin * m.Polar.BinSzDeg), (int)(m.Polar.HiMaxBin * m.Polar.BinSzDeg));
	fprintf(p, "Annular histogram measures (%d equal areas)\n", m.Annular.NBins);
	fprintf(p, "Average [sd]:                       \t%0.2f [%0.2f]\n", m.Annular.Avg, m.Annular.SD);
	fprintf(p, "Skewness and Kurtosis:              \t%0.2f %0.2f\n", (float)(m.Annular.Skewness), (float)(m.Annular.Kurtosis));
	fprintf(p, "Minimum non-zero value [0-1]:       \t%0.4f\n", m.Annular.MinNonZero);
	fprintf(p, "Minimum non-zero bin (center=%.1f edge=%.1f):\t%0.1f cm\n", (float)m.Annular.BinAvgCM[0], (float)m.Annular.BinAvgCM[m.Annular.NBins - 1], (float)(m.Annular.MinNonZeroBin));
	fprintf(p, "Maximum value [0-1]:                \t%0.4f\n", m.Annular.Max);
	fprintf(p, "Maximum bin (center=%.1f, edge=%.1f):\t%0.1f cm\n", (float)m.Annular.BinAvgCM[0], (float)m.Annular.BinAvgCM[m.Annular.NBins - 1], (float)(m.Annular.MaxBin));

	fprintf(p, "\t\t********************************\n\n");
	return;
}

void	PrintPlacePreferenceTextReport(FILE *p, MEASURES m, INFO I)
{
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\t\t********************************\n\n");
		return;
	}
	
	fprintf(p, "Coordinate Frame:\t%s\n", m.FrameName);
	if(m.NumberOfSamples)
		fprintf(p, "Proportion of samples missed (no detects):\t%0.3f (%d)\n", (float)m.Misses / (float)m.NumberOfSamples, m.Misses); 
	else
		fprintf(p, "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)(1.00), m.Misses);

	fprintf(p, "Reinforced Area (centerX, centerY, radius)\n");
	fprintf(p, "(%0.2f, %0.2f, %0.2f)\n",
                     I.Target[m.frame].Circle.X, I.Target[m.frame].Circle.Y, I.Target[m.frame].Circle.Rad);
        fprintf(p, "Reward Parameters (dwell criterion, refractory time in ms)\n"); 
        fprintf(p, "(%0.1f, %0.1f)\n", I.Feeder[m.frame].Trigger, I.Feeder[m.frame].Refractory);                   

	fprintf(p, "Total time:                        \t%d sec\n", (int)(m.Duration + 0.5));
	fprintf(p, "Total pathlength:                  \t%0.2f m\n", (float)m.PathLength);
	fprintf(p, "Number of rewards:                  \t%d\n", m.Entrances);
	fprintf(p, "Time to the first reward:          \t%0.2f sec\n", m.TimeToFirstEntrance);
	fprintf(p, "Time to the second reward:         \t%0.2f sec\n", m.TimeToSecondEntrance);
	// fprintf(p, "Pathlength to the first entrance:  \t%0.2f m\n", (float)m.PathToFirstEntrance);
	// fprintf(p, "Pathlength to the second entrance: \t%0.2f m\n", (float)m.PathToSecondEntrance);
	// fprintf(p, "Entrances per distance moved:      \t%0.2f 1/m\n", (float)(m.PathLength == 0.0) ? 0.0 : (m.Entrances / m.PathLength));
	// fprintf(p, "Number of shocks:                  \t%d\n", m.NumberOfShocks);
	// fprintf(p, "Time to the first shock:           \t%0.2f sec\n", m.TimeToFirstShock);
	// fprintf(p, "Pathlength to the first shock:     \t%0.2f m\n", (float)m.PathToFirstShock);
	fprintf(p, "Speed: mean (s.d)                  \t%0.2f (%0.2f) cm/s\n", (float)m.Speed, (float)m.sdSpeed);
	// fprintf(p, "Linearity (linear/integral dist)   \t%0.4f (timestep = %ds)\n", (float)m.Linearity, (int)(I.TimeStep * LINEARITY_SCALE_FACTOR/1000));
	fprintf(p, "Entrances in Quadrant  (T CCW OPP CW)  \t(%d %d %d %d)\n", m.EntrancesTARG, m.EntrancesCCW, m.EntrancesOPP, m.EntrancesCW);
	fprintf(p, "Time in Quadrant (T CCW OPP CW)  \t(%0.3lf %0.3lf %0.3lf %0.3lf)\n", m.TimeEntranceTARG, m.TimeEntranceCCW, m.TimeEntranceOPP, m.TimeEntranceCW);
	fprintf(p, "prop Time in Quadrant (T CCW OPP CW)  \t(%0.3lf %0.3lf %0.3lf %0.3lf)\n", m.pTimeEntranceTARG, m.pTimeEntranceCCW, m.pTimeEntranceOPP, m.pTimeEntranceCW);
	fprintf(p, "Max time without reward                \t%d sec\n", (int)m.MaxTimeAvoided);
	// fprintf(p, "Max path of avoidance               \t%0.2f m\n", m.MaxPathAvoided);
	// fprintf(p, "Polar histogram measures (%d deg bins)\n", (int)m.Polar.BinSzDeg);
	// fprintf(p, "Rayleigh vector length [ang]:       \t%0.2f [%0.2f]\n", m.Polar.RayleighLength, m.Polar.RayleighAng);
	// fprintf(p, "Average [sd]:                       \t%0.2f [%0.2f]\n", m.Polar.Avg, m.Polar.SD);
	// fprintf(p, "Minimum value [0-1]:                \t%0.4f\n", m.Polar.Min);
	// fprintf(p, "Minimum bin (0 = shock sector):     \t%d deg\n", (int)(m.Polar.MinBin * m.Polar.BinSzDeg));
	// fprintf(p, "50%% time in the range (incl min bin):\t[%d %d] degs\n", (int)(m.Polar.LoMinBin * m.Polar.BinSzDeg), (int)(m.Polar.HiMinBin * m.Polar.BinSzDeg));
	// fprintf(p, "Maximum value [0-1]:                \t%0.4f\n", m.Polar.Max);
	// fprintf(p, "Maximum bin (0 = shock sector):     \t%d deg\n", (int)(m.Polar.MaxBin * m.Polar.BinSzDeg));
	// fprintf(p, "50%% time in the range (incl max bin):\t[%d %d] degs\n", (int)(m.Polar.LoMaxBin * m.Polar.BinSzDeg), (int)(m.Polar.HiMaxBin * m.Polar.BinSzDeg));
	// fprintf(p, "Annular histogram measures (%d equal areas)\n", m.Annular.NBins);
	// fprintf(p, "Average [sd]:                       \t%0.2f [%0.2f]\n", m.Annular.Avg, m.Annular.SD);
	// fprintf(p, "Skewness and Kurtosis:              \t%0.2f %0.2f\n", (float)(m.Annular.Skewness), (float)(m.Annular.Kurtosis));
	// fprintf(p, "Minimum non-zero value [0-1]:       \t%0.4f\n", m.Annular.MinNonZero);
	// fprintf(p, "Minimum non-zero bin (center=%.1f edge=%.1f):\t%0.1f cm\n", (float)m.Annular.BinAvgCM[0], (float)m.Annular.BinAvgCM[m.Annular.NBins - 1], (float)(m.Annular.MinNonZeroBin));
	// fprintf(p, "Maximum value [0-1]:                \t%0.4f\n", m.Annular.Max);
	// fprintf(p, "Maximum bin (center=%.1f, edge=%.1f):\t%0.1f cm\n", (float)m.Annular.BinAvgCM[0], (float)m.Annular.BinAvgCM[m.Annular.NBins - 1], (float)(m.Annular.MaxBin));

	fprintf(p, "\t\t********************************\n\n");
	return;
}

void	PrintWatermazeTextReport(FILE *p, MEASURES m, INFO I)
{
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\t\t********************************\n\n");
		return;
	}
	
	if(m.NumberOfSamples)
		fprintf(p, "Proportion of samples missed (no detects):\t%0.3f (%d)\n", (float)m.Misses / (float)m.NumberOfSamples, m.Misses); 
	else
		fprintf(p, "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)(1.00), m.Misses);

	fprintf(p, "Target (centerX, centerY, radius)\n");
	fprintf(p, "(%0.2f, %0.2f, %0.2f)\n",
                     I.Target[m.frame].Circle.X, I.Target[m.frame].Circle.Y, I.Target[m.frame].Circle.Rad);
        fprintf(p, "Escape Parameters (dwell criterion in ms)\n"); 
        fprintf(p, "(%0.1f)\n", I.Feeder[m.frame].Trigger);                   

	fprintf(p, "Trial number                       \t%d\n", I.TrialNumber);
	fprintf(p, "Trial subject                      \t%d\n", I.TrialSubjectId);
	fprintf(p, "Trial start                        \t%s\n", I.TrialStartLabel);
	fprintf(p, "Total time:                        \t%0.1f sec\n", m.Duration);
	fprintf(p, "Total pathlength:                  \t%0.2f m\n", (float)m.PathLength);
	fprintf(p, "Number of rewards:                 \t%d\n", m.Entrances);
	fprintf(p, "Time to the first crossing:        \t%0.2f sec\n", m.TimeToFirstEntrance);
	fprintf(p, "Time to the second crossing:       \t%0.2f sec\n", m.TimeToSecondEntrance);
	fprintf(p, "Pathlength to the first crossing:  \t%0.2f m\n", (float)m.PathToFirstEntrance);
	fprintf(p, "Speed: mean (s.d)                  \t%0.2f (%0.2f) cm/s\n", (float)m.Speed, (float)m.sdSpeed);
	fprintf(p, "Linearity (linear/integral dist)   \t%0.4f (timestep = %ds)\n", (float)m.Linearity, (int)(I.TimeStep * LINEARITY_SCALE_FACTOR/1000));
	fprintf(p, "Annulus Crossings (T CCW OPP CW)   \t(%d %d %d %d)\n", m.EntrancesTARG, m.EntrancesCCW, m.EntrancesOPP, m.EntrancesCW);
	fprintf(p, "Time in Quadrant (T CCW OPP CW)    \t(%0.3lf %0.3lf %0.3lf %0.3lf)\n", m.QuadrantTimeTARG, m.QuadrantTimeCCW, m.QuadrantTimeOPP, m.QuadrantTimeCW);
	fprintf(p, "p Time in Quadrant (T CCW OPP CW)  \t(%0.3lf %0.3lf %0.3lf %0.3lf)\n", m.pQuadrantTimeTARG, m.pQuadrantTimeCCW, m.pQuadrantTimeOPP, m.pQuadrantTimeCW);
	fprintf(p, "Max time between crossings	       \t%0.1f sec\n", m.MaxTimeAvoided);
	// fprintf(p, "Polar histogram measures (%d deg bins)\n", (int)m.Polar.BinSzDeg);
	// fprintf(p, "Average [sd]:                       \t%0.2f [%0.2f]\n", m.Polar.Avg, m.Polar.SD);
	// fprintf(p, "Minimum value [0-1]:                \t%0.4f\n", m.Polar.Min);
	// fprintf(p, "Minimum bin (0 = shock sector):     \t%d deg\n", (int)(m.Polar.MinBin * m.Polar.BinSzDeg));
	// fprintf(p, "50%% time in the range (incl min bin):\t[%d %d] degs\n", (int)(m.Polar.LoMinBin * m.Polar.BinSzDeg), (int)(m.Polar.HiMinBin * m.Polar.BinSzDeg));
	// fprintf(p, "Maximum value [0-1]:                \t%0.4f\n", m.Polar.Max);
	// fprintf(p, "Maximum bin (0 = shock sector):     \t%d deg\n", (int)(m.Polar.MaxBin * m.Polar.BinSzDeg));
	// fprintf(p, "50%% time in the range (incl max bin):\t[%d %d] degs\n", (int)(m.Polar.LoMaxBin * m.Polar.BinSzDeg), (int)(m.Polar.HiMaxBin * m.Polar.BinSzDeg));
	// fprintf(p, "Annular histogram measures (%d equal areas)\n", m.Annular.NBins);
	// fprintf(p, "Average [sd]:                       \t%0.2f [%0.2f]\n", m.Annular.Avg, m.Annular.SD);
	// fprintf(p, "Skewness and Kurtosis:              \t%0.2f %0.2f\n", (float)(m.Annular.Skewness), (float)(m.Annular.Kurtosis));
	// fprintf(p, "Minimum non-zero value [0-1]:       \t%0.4f\n", m.Annular.MinNonZero);
	// fprintf(p, "Minimum non-zero bin (center=%.1f edge=%.1f):\t%0.1f cm\n", (float)m.Annular.BinAvgCM[0], (float)m.Annular.BinAvgCM[m.Annular.NBins - 1], (float)(m.Annular.MinNonZeroBin));
	// fprintf(p, "Maximum value [0-1]:                \t%0.4f\n", m.Annular.Max);
	// fprintf(p, "Maximum bin (center=%.1f, edge=%.1f):\t%0.1f cm\n", (float)m.Annular.BinAvgCM[0], (float)m.Annular.BinAvgCM[m.Annular.NBins - 1], (float)(m.Annular.MaxBin));

	fprintf(p, "\t\t********************************\n\n");
	return;
}
void	PrintTrackerTextReport(FILE *p, MEASURES m, INFO I)
{
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\t\t********************************\n\n");
		return;
	}
	
	// fprintf(p, "Coordinate Frame:\t%s\n", m.FrameName);
	if(m.NumberOfSamples)
		fprintf(p, "Proportion missed samples (no detects):\t%0.3f (%d)\n", (float)m.Misses / (float)m.NumberOfSamples, m.Misses); 
	else
		fprintf(p, "Proportion missed samples (no detects):\t%0.3f (%d)", (float)(1.00), m.Misses);

	fprintf(p, "Total time:                        \t%0.1f sec\n", m.Duration);
	fprintf(p, "Total pathlength:                  \t%0.2f m\n", (float)m.PathLength);
	fprintf(p, "Speed: mean (s.d)                  \t%0.2f (%0.2f) cm/s\n", (float)m.Speed, (float)m.sdSpeed);
	fprintf(p, "Linearity (linear/integral dist)   \t%0.4f (timestep = %ds)\n", (float)m.Linearity, (int)(I.TimeStep * LINEARITY_SCALE_FACTOR/1000));
	fprintf(p, "Time in Quadrant (E N W S)         \t(%0.3lf %0.3lf %0.3lf %0.3lf)\n", m.TimeEntranceTARG, m.TimeEntranceCCW, m.TimeEntranceOPP, m.TimeEntranceCW);
	fprintf(p, "p Time in Quadrant (E N W S)       \t(%0.3lf %0.3lf %0.3lf %0.3lf)\n", m.pTimeEntranceTARG, m.pTimeEntranceCCW, m.pTimeEntranceOPP, m.pTimeEntranceCW);
	// fprintf(p, "Annular histogram measures (%d equal areas)\n", m.Annular.NBins);
	fprintf(p, "\t\t********************************\n\n");
	return;
}

void	PrintOpenFieldTextReport(FILE *p, MEASURES m, INFO I)
{
	int i;

	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\t\t********************************\n\n");
		return;
	}
	
	// fprintf(p, "Coordinate Frame:\t%s\n", m.FrameName);
	if(m.NumberOfSamples)
		fprintf(p, "Proportion missed samples (no detects):\t%0.3f (%d)\n", (float)m.Misses / (float)m.NumberOfSamples, m.Misses); 
	else
		fprintf(p, "Proportion missed samples (no detects):\t%0.3f (%d)", (float)(1.00), m.Misses);

	fprintf(p, "Total time:                        \t%0.1f sec\n", m.Duration);
	fprintf(p, "Total pathlength:                  \t%0.2f m\n", (float)m.PathLength);
	fprintf(p, "Grid for Line Crossings:		   \t%0.1f x %0.1f pixels\n", m.OField.Xbins, m.OField.Ybins);
	fprintf(p, "Number of Line Crossings:		   \t%d\n", m.OField.LineCrossings);
	fprintf(p, "Speed: mean (s.d)                  \t%0.2f (%0.2f) cm/s\n", (float)m.Speed, (float)m.sdSpeed);
	fprintf(p, "Linearity (linear/integral dist)   \t%0.4f (timestep = %ds)\n", (float)m.Linearity, (int)(I.TimeStep * LINEARITY_SCALE_FACTOR/1000));
	fprintf(p, "Time in Quadrant (E N W S)         \t(%0.3lf %0.3lf %0.3lf %0.3lf)\n", m.TimeEntranceTARG, m.TimeEntranceCCW, m.TimeEntranceOPP, m.TimeEntranceCW);
	fprintf(p, "p Time in Quadrant (E N W S)       \t(%0.3lf %0.3lf %0.3lf %0.3lf)\n", m.pTimeEntranceTARG, m.pTimeEntranceCCW, m.pTimeEntranceOPP, m.pTimeEntranceCW);
	fprintf(p, "p Time in %d equal area annuli starting from center:\n", m.Annular.NBins);
	for(i=0; i < m.Annular.NBins; i++)
		fprintf(p, " %0.3f", m.Annular.Bin[i]);
	fprintf(p, "\n");

	fprintf(p, "\t\t********************************\n\n");
	return;
}

#define	PA_PS_LINES_TO_PRINT	36
void	PrintPlaceAvoidancePSReport(FILE *psfp, MEASURES m, INFO I)
{
	char	p[PA_PS_LINES_TO_PRINT][100];
	int		i, x, y, LineOffset;
	float	ArenaRadiusInPix;

	ArenaRadiusInPix = (float)DEFAULT_ARENA_RADIUS;   // iTrack sets the radius to be 127

	sprintf(p[0], I.Frame);
	if(m.NumberOfSamples)
		sprintf(p[1], "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)m.Misses / (float)m.NumberOfSamples, m.Misses); 
	else
		sprintf(p[1], "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)(1.00), m.Misses); 

	if(I.Target[m.frame].Type == REINFORCED_SECTOR){
		sprintf(p[2], "Reinforced Area (ang, width, inner radius, outer radius)");
		sprintf(p[3], "(%0.2f, %0.2f, %0.2f, %0.2f)",
			I.Target[m.frame].Sector.Ang, I.Target[m.frame].Sector.Width,
			I.Target[m.frame].Sector.InRad, I.Target[m.frame].Sector.OutRad );
	}else if(I.Target[m.frame].Type == REINFORCED_CIRCLE){
		sprintf(p[2], "Reinforced Area (centerX, centerY, radius)");
		sprintf(p[3], "(%0.2f, %0.2f, %0.2f)",
			I.Target[m.frame].Circle.X, I.Target[m.frame].Circle.Y, I.Target[m.frame].Circle.Rad);
	}
	sprintf(p[4], "Shock Parameters (latency, duration, Min ISI in ms)");
	sprintf(p[5], "(%d, %d, %d)", I.Shock.Latency, I.Shock.Duration, I.Shock.MinISI);

	sprintf(p[6], "");
	sprintf(p[7], "Total time:");
	sprintf(p[8], "Total pathlength:");
	sprintf(p[9], "Number of entrances:");
	sprintf(p[10], "Time to the first entrance:");
	sprintf(p[11], "Time to the second entrance:");
	sprintf(p[12], "Speed at the first entrance:");
	sprintf(p[13], "Speed at the second entrance:");
    	sprintf(p[14], "Pathlength to the first entrance:");
    	sprintf(p[15], "Pathlength to the second entrance:");
    	sprintf(p[16], "Number of shocks:");
    	sprintf(p[17], "Time to the first shock:");
    	sprintf(p[18], "Pathlength to the first shock:");
    	sprintf(p[19], "Entrances per distance moved:");
	sprintf(p[20], "Speed: mean (s.d.)");
	sprintf(p[21], "Linearity" );
	sprintf(p[22], "Target time (s) (T CCW OPP CW)");
	sprintf(p[23], "p Quad time (T CCW OPP CW)");
	sprintf(p[24], "Max time of avoidance: ");
    	sprintf(p[25], "Max path of avoidance: ");
    	sprintf(p[26], "Rayleigh Length [Angle]: ");
    	sprintf(p[27], "Polar histogram (deg/bin):");
    	sprintf(p[28], "Average [sd]: ");
    	sprintf(p[29], "Min & 50%% range [lo min hi]: ");
    	sprintf(p[30], "Max & 50%% range [lo max hi]: ");
    	sprintf(p[31], "Annular histogram (n equal areas)");
    	sprintf(p[32], "Average [sd]: ");
    	sprintf(p[33], "[Skewness Kurtosis]: ");
    	sprintf(p[34], "Min non-zero [min bin]: ");
    	sprintf(p[35], "Max [max bin]: ");

	(void) fprintf(psfp, "/Times-Roman findfont\n");
	(void) fprintf(psfp, "0.35 cm scalefont\n");
	(void) fprintf(psfp, "setfont\n");
	(void) fprintf(psfp, "0 0 0 setrgbcolor\n");

	// coordinate system set before calling function
	x = 0;
	y = 0;
	LineOffset = -10;

	for(i = 0; i < PA_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset)); 
		fprintf(psfp, "( %s ) show\n", p[i]);
	}

	sprintf(p[0], ""); 
	sprintf(p[1], "");

	sprintf(p[2], "");
	sprintf(p[3], "");
	sprintf(p[4], "");

	sprintf(p[5], "");
	sprintf(p[6], "");
	sprintf(p[7], "%d sec", (int)(m.Duration + 0.5));
	sprintf(p[8], "%0.2f m", (float)m.PathLength);
	sprintf(p[9], "%d", m.Entrances);
	sprintf(p[10], "%0.2f sec", m.TimeToFirstEntrance);
	sprintf(p[11], "%0.2f sec", m.TimeToSecondEntrance);
	sprintf(p[12], "%0.2f cm/s", (float)m.SpeedAtFirstEntrance);
	sprintf(p[13], "%0.2f cm/s", (float)m.SpeedAtSecondEntrance);
	sprintf(p[14], "%0.2f m", (float)m.PathToFirstEntrance);
	sprintf(p[15], "%0.2f m", (float)m.PathToSecondEntrance);
	sprintf(p[16],"%d", m.NumberOfShocks);
	sprintf(p[17], "%0.2f sec", m.TimeToFirstShock);
	sprintf(p[18], "%0.2f m", (float)m.PathToFirstShock);
	sprintf(p[19],"%0.2f 1/m", (float)(m.PathLength == 0.0) ? 0.0 : (m.Entrances / m.PathLength));
	sprintf(p[20],"%0.2f (%0.2f) cm/s", (float)m.Speed, (float)m.sdSpeed);
	sprintf(p[21],"%0.4f (timestep = %ds)", (float)m.Linearity, (int)(I.TimeStep * LINEARITY_SCALE_FACTOR / 1000));
	sprintf(p[22],"(%0.3f %0.3f %0.3f %0.3f)", (float)m.TimeEntranceTARG,
				(float)m.TimeEntranceCCW, (float)m.TimeEntranceOPP, (float)m.TimeEntranceCW);
	sprintf(p[23],"(%0.3f %0.3f %0.3f %0.3f)", (float)m.pTimeEntranceTARG,
				(float)m.pTimeEntranceCCW, (float)m.pTimeEntranceOPP, (float)m.pTimeEntranceCW);
	sprintf(p[24],"%d sec", (int)m.MaxTimeAvoided);
	sprintf(p[25],"%.2f m", m.MaxPathAvoided);
	sprintf(p[26],"%.2f [%.2f] deg", m.Polar.RayleighLength, m.Polar.RayleighAng);
	sprintf(p[27],"%d deg", (int)(m.Polar.BinSzDeg));
	sprintf(p[28],"%.2f [%.2f] deg", m.Polar.Avg, m.Polar.SD);
	sprintf(p[29],"%.4f [%d %d %d] deg", m.Polar.Min, (int)(m.Polar.LoMinBin * m.Polar.BinSzDeg), (int)(m.Polar.MinBin * m.Polar.BinSzDeg), (int)(m.Polar.HiMinBin * m.Polar.BinSzDeg));
	sprintf(p[30],"%.4f [%d %d %d] deg", m.Polar.Max, (int)(m.Polar.LoMaxBin * m.Polar.BinSzDeg), (int)(m.Polar.MaxBin * m.Polar.BinSzDeg), (int)(m.Polar.HiMaxBin * m.Polar.BinSzDeg));
	sprintf(p[31],"%d", m.Annular.NBins);
	sprintf(p[32],"%.2lf [%.2lf] cm", m.Annular.Avg, m.Annular.SD);
	sprintf(p[33],"[%.2lf %.2lf]", m.Annular.Skewness, m.Annular.Kurtosis);
	sprintf(p[34],"%.4lf [%0.1lf] cm", m.Annular.MinNonZero, m.Annular.MinNonZeroBin);
	sprintf(p[35],"%.4lf [%0.1lf] cm", m.Annular.Max, m.Annular.MaxBin);

	// coordinate system was set before when writing legend
	x += 140;
	y += 0;
	LineOffset = -10;

	for(i = 0; i < PA_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset));
		fprintf(psfp, "( %s ) show\n", p[i]);

	}
	fprintf(psfp, "grestore\n");

	return;
}

#define	PP_PS_LINES_TO_PRINT	16
void	PrintPlacePreferencePSReport(FILE *psfp, MEASURES m, INFO I)
{
	char	p[PP_PS_LINES_TO_PRINT][100];
	int		i, x, y, LineOffset;
	float	ArenaRadiusInPix;

	ArenaRadiusInPix = (float)DEFAULT_ARENA_RADIUS;   // iTrack sets the radius to be 127

	sprintf(p[0], I.Frame);
	if(m.NumberOfSamples)
		sprintf(p[1], "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)m.Misses / (float)m.NumberOfSamples, m.Misses); 
	else
		sprintf(p[1], "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)(1.00), m.Misses); 
	sprintf(p[2], "Reinforced Area (centerX, centerY, radius)");
	sprintf(p[3], "(%0.2f, %0.2f, %0.2f)",
		I.Target[m.frame].Circle.X, I.Target[m.frame].Circle.Y, I.Target[m.frame].Circle.Rad);
	sprintf(p[4], "Reward Parameters (dwell criterion, refractory time in ms)");
	sprintf(p[5], "(%0.1f, %0.1f)",
		I.Feeder[m.frame].Trigger, I.Feeder[m.frame].Refractory);
	sprintf(p[6], "");
	sprintf(p[7], "Total time:");
	sprintf(p[8], "Total pathlength:");
	sprintf(p[9], "Number of rewards:");
	sprintf(p[10], "Time to the first reward:");
	sprintf(p[11], "Speed: mean (s.d.)");
	sprintf(p[12], "Quad Entries (T CCW OPP CW)");
	sprintf(p[13], "Quad time (T CCW OPP CW)");
	sprintf(p[14], "p Quad time (T CCW OPP CW)");
	sprintf(p[15], "Max time without reward: ");

	(void) fprintf(psfp, "/Times-Roman findfont\n");
	(void) fprintf(psfp, "0.35 cm scalefont\n");
	(void) fprintf(psfp, "setfont\n");
	(void) fprintf(psfp, "0 0 0 setrgbcolor\n");
	
	// coordinates set before calling the function
	x = 0;
	y = 0;
	LineOffset = -10;

	for(i = 0; i < PP_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset)); 
		fprintf(psfp, "( %s ) show\n", p[i]);
	}

	sprintf(p[0], ""); 
	sprintf(p[1], "");

	sprintf(p[2], "");
	sprintf(p[3], "");
	sprintf(p[4], "");

	sprintf(p[5], "");
	sprintf(p[6], "");
	sprintf(p[7], "%d sec", (int)(m.Duration + 0.5));
	sprintf(p[8], "%0.2f m", (float)m.PathLength);
	sprintf(p[9], "%d", m.Entrances);
	sprintf(p[10], "%0.2f sec", m.TimeToFirstEntrance);
	sprintf(p[11],"%0.2f (%0.2f) cm/s", (float)m.Speed, (float)m.sdSpeed);
	sprintf(p[12],"(%d %d %d %d)", m.EntrancesTARG,
				m.EntrancesCCW, m.EntrancesOPP, m.EntrancesCW);
	sprintf(p[13],"(%.0lf %.0lf %.0lf %.0lf) sec", m.TimeEntranceTARG,
				m.TimeEntranceCCW, m.TimeEntranceOPP, m.TimeEntranceCW);
	sprintf(p[14],"(%0.3lf %0.3lf %0.3lf %0.3lf)", m.pTimeEntranceTARG,
				m.pTimeEntranceCCW, m.pTimeEntranceOPP, m.pTimeEntranceCW);
	sprintf(p[15],"%d sec", (int)m.MaxTimeAvoided);

	// coordinates set when legends were printed
	x += 140;
	y += 0;

	LineOffset = -10;

	for(i = 0; i < PP_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset));
		fprintf(psfp, "( %s ) show\n", p[i]);

	}
	fprintf(psfp, "grestore\n");

	return;
}


#define	TR_PS_LINES_TO_PRINT	8
void	PrintTrackerPSReport(FILE *psfp, MEASURES m, INFO I)
{
	char	p[PP_PS_LINES_TO_PRINT][100];
	int		i, x, y, LineOffset;
	float	ArenaRadiusInPix;

        ArenaRadiusInPix = (float)DEFAULT_ARENA_RADIUS;   // iTrack sets the radius to be 127

	sprintf(p[0], I.Frame);
	if(m.NumberOfSamples)
		sprintf(p[1], "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)m.Misses / (float)m.NumberOfSamples, m.Misses); 
	else
		sprintf(p[1], "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)(1.00), m.Misses); 
	sprintf(p[2], "");
	sprintf(p[3], "Total time:");
	sprintf(p[4], "Total pathlength:");
	sprintf(p[5], "Speed: mean (s.d.)");
	sprintf(p[6], "Quad time (E N W S)");
	sprintf(p[7], "p Quad time (E N W S)");

	(void) fprintf(psfp, "/Times-Roman findfont\n");
	(void) fprintf(psfp, "0.35 cm scalefont\n");
	(void) fprintf(psfp, "setfont\n");
	(void) fprintf(psfp, "0 0 0 setrgbcolor\n");

	// coordinates set when function was called
	x = 0;
	y = 0;
	LineOffset = -10;

	for(i = 0; i < TR_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset)); 
		fprintf(psfp, "( %s ) show\n", p[i]);
	}

	sprintf(p[0], ""); 
	sprintf(p[1], "");

	sprintf(p[2], "");
	sprintf(p[3], "%d sec", (int)(m.Duration + 0.5));
	sprintf(p[4], "%0.2f m", (float)m.PathLength);
	sprintf(p[5],"%0.2f (%0.2f) cm/s", (float)m.Speed, (float)m.sdSpeed);
	sprintf(p[6],"(%.0lf %.0lf %.0lf %.0lf) sec", m.TimeEntranceTARG,
				m.TimeEntranceCCW, m.TimeEntranceOPP, m.TimeEntranceCW);
	sprintf(p[7],"(%0.3lf %0.3lf %0.3lf %0.3lf)", m.pTimeEntranceTARG,
				m.pTimeEntranceCCW, m.pTimeEntranceOPP, m.pTimeEntranceCW);

	// coordinates set when legends were printed
	x += 140;
	y += 0;
	LineOffset = -10;

	for(i = 0; i < TR_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset));
		fprintf(psfp, "( %s ) show\n", p[i]);
	}
	fprintf(psfp, "grestore\n");

	return;
}

#define	OF_PS_LINES_TO_PRINT 10	
void	PrintOpenFieldPSReport(FILE *psfp, MEASURES m, INFO I)
{
	char	p[OF_PS_LINES_TO_PRINT][100];
	int		i, x, y, LineOffset;
	float	ArenaRadiusInPix;

    	ArenaRadiusInPix = (float)DEFAULT_ARENA_RADIUS;   // iTrack sets the radius to be 127

	sprintf(p[0], I.Frame);
	if(m.NumberOfSamples)
		sprintf(p[1], "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)m.Misses / (float)m.NumberOfSamples, m.Misses); 
	else
		sprintf(p[1], "Proportion of samples missed (no detects):\t%0.3f (%d)", (float)(1.00), m.Misses); 
	sprintf(p[2], "");
	sprintf(p[3], "Total time:");
	sprintf(p[4], "Total pathlength:");
	sprintf(p[5], "Speed: mean (s.d.)");
	sprintf(p[6], "Quad time (E N W S)");
	sprintf(p[7], "p Quad time (E N W S)");
	sprintf(p[8], "(X,Y) pixels for Line Crossings");
	sprintf(p[9], "# Line crossings");

	(void) fprintf(psfp, "/Times-Roman findfont\n");
	(void) fprintf(psfp, "0.35 cm scalefont\n");
	(void) fprintf(psfp, "setfont\n");
	(void) fprintf(psfp, "0 0 0 setrgbcolor\n");

	// coordinates set when function called
	x = 0;
	y = 0;
	LineOffset = -10;

	for(i = 0; i < OF_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset)); 
		fprintf(psfp, "( %s ) show\n", p[i]);
	}

	sprintf(p[0], ""); 
	sprintf(p[1], "");

	sprintf(p[2], "");
	sprintf(p[3], "%d sec", (int)(m.Duration + 0.5));
	sprintf(p[4], "%0.2f m", (float)m.PathLength);
	sprintf(p[5],"%0.2f (%0.2f) cm/s", (float)m.Speed, (float)m.sdSpeed);
	sprintf(p[6],"(%.0lf %.0lf %.0lf %.0lf) sec", m.TimeEntranceTARG,
				m.TimeEntranceCCW, m.TimeEntranceOPP, m.TimeEntranceCW);
	sprintf(p[7],"(%0.3lf %0.3lf %0.3lf %0.3lf)", m.pTimeEntranceTARG,
				m.pTimeEntranceCCW, m.pTimeEntranceOPP, m.pTimeEntranceCW);
	sprintf(p[8],"(%0.1lf, %0.1lf)", m.OField.Xbins, m.OField.Ybins);
	sprintf(p[9],"%d", m.OField.LineCrossings);

	// coordinates set when legends were printed
	x += 140;
	y += 0;
	LineOffset = -10;

	for(i = 0; i < OF_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset));
		fprintf(psfp, "( %s ) show\n", p[i]);
	}
	fprintf(psfp, "grestore\n");

	return;
}

#define	WM_PS_LINES_TO_PRINT	8
void	PrintWatermazePSLegend(FILE *psfp, MEASURES m, INFO I, float TrackScale)
{
	char	p[WM_PS_LINES_TO_PRINT][100];
	int		i, x, y, LineOffset;
	float   ArenaRadiusInPix;

        ArenaRadiusInPix = (float)DEFAULT_ARENA_RADIUS / TrackScale;   // iTrack sets the radius to be 127

	// sprintf(p[1], "Target (centerX, centerY, radius)");
	// sprintf(p[2], "(%0.2f, %0.2f, %0.2f)",
	//	I.Target[m.frame].Circle.X, I.Target[m.frame].Circle.Y, I.Target[m.frame].Circle.Rad);
	// sprintf(p[3], "Escape Parameters (dwell criterion in ms)");
	// sprintf(p[4], "(%0.1f)", I.Feeder[m.frame].Trigger);
	// sprintf(p[5], "");
	sprintf(p[0], "%% no detects:");
	sprintf(p[1], "Latency to target:");
	sprintf(p[2], "Total pathlength:");
	sprintf(p[3], "Speed: mean (s.d.)");
	sprintf(p[4], "Annuli Crosses (t ccw opp cw)");
	sprintf(p[5], "Quad time (t ccw opp cw)");
	sprintf(p[6], "p Quad time (t ccw opp cw)");
	sprintf(p[7], "Max time between crossings: ");

	(void) fprintf(psfp, "/Times-Roman findfont\n");
	(void) fprintf(psfp, "0.25 cm scalefont\n");
	(void) fprintf(psfp, "setfont\n");
	(void) fprintf(psfp, "0 0 0 setrgbcolor\n");

	x = -100;	// coordinates are relative to origin of last plot
	y = -1 * (int)(0.4 * ArenaRadiusInPix);
	LineOffset = -8;

	for(i = 0; i < WM_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset)); 
		fprintf(psfp, "( %s ) show\n", p[i]);
	}
	return;
}

#define	WM_PS_LINES_TO_PRINT	8
void	PrintWatermazePSReport(FILE *psfp, MEASURES m, INFO I, float TrackScale)
{
	char	p[WM_PS_LINES_TO_PRINT][100];
	int		i, x, y, LineOffset;
	float   ArenaRadiusInPix;

        ArenaRadiusInPix = (float)DEFAULT_ARENA_RADIUS / TrackScale;   // iTrack sets the radius to be 127

	// sprintf(p[1], "Target (centerX, centerY, radius)");
	// sprintf(p[2], "(%0.2f, %0.2f, %0.2f)",
	//	I.Target[m.frame].Circle.X, I.Target[m.frame].Circle.Y, I.Target[m.frame].Circle.Rad);
	// sprintf(p[3], "Escape Parameters (dwell criterion in ms)");
	// sprintf(p[4], "(%0.1f)", I.Feeder[m.frame].Trigger);
	// sprintf(p[5], "");
	/* these are printed in the PrintWatermazePSLegend() 
	sprintf(p[0], "%% no detects:");
	sprintf(p[1], "Latency to target:");
	sprintf(p[2], "Total pathlength:");
	sprintf(p[3], "Speed: mean (s.d.)");
	sprintf(p[4], "Annuli Crosses (t ccw opp cw)");
	sprintf(p[5], "Quad time (t ccw opp cw)");
	sprintf(p[6], "p Quad time (t ccw opp cw)");
	sprintf(p[7], "Max time between crossings: ");
	*/

	if(m.NumberOfSamples)
		sprintf(p[0], "%0.3f (%d)", (float)(100.0 * m.Misses) / (float)m.NumberOfSamples, m.Misses);
	else
		sprintf(p[0], "%0.3f (%d)", (float)(100), m.Misses); 

	sprintf(p[1], "%0.2f sec", m.TimeToFirstEntrance);
	sprintf(p[2], "%0.2f m", (float)m.PathLength);
	sprintf(p[3],"%0.2f (%0.2f) cm/s", (float)m.Speed, (float)m.sdSpeed);
	sprintf(p[4],"(%d %d %d %d)", m.EntrancesTARG,
				m.EntrancesCCW, m.EntrancesOPP, m.EntrancesCW);
	sprintf(p[5],"(%.2lf %.2lf %.2lf %.2lf) sec", m.QuadrantTimeTARG,
				m.QuadrantTimeCCW, m.QuadrantTimeOPP, m.QuadrantTimeCW);
	sprintf(p[6],"(%0.3lf %0.3lf %0.3lf %0.3lf)", m.pQuadrantTimeTARG,
				m.pQuadrantTimeCCW, m.pQuadrantTimeOPP, m.pQuadrantTimeCW);
	sprintf(p[7],"%0.1f sec", m.MaxTimeAvoided);

	(void) fprintf(psfp, "/Times-Roman findfont\n");
	(void) fprintf(psfp, "0.25 cm scalefont\n");
	(void) fprintf(psfp, "setfont\n");
	(void) fprintf(psfp, "0 0 0 setrgbcolor\n");

	x = -10;	// coordinates are relative to origin of last plot
	y = -1 * (int)(0.4 * ArenaRadiusInPix);
	LineOffset = -8;

	for(i = 0; i < WM_PS_LINES_TO_PRINT; i++){
		if(!strcmp(p[i],""))
			continue;
		(void) fprintf(psfp, "%d pix %d pix mt\n", x, y + (i * LineOffset));
		fprintf(psfp, "( %s ) show\n", p[i]);

	}
	fprintf(psfp, "grestore\n"); // this will restore the coordinate system to the top left of the page

	return;
}

void	PrintPlaceAvoidanceTableHeadings(FILE *tblfp)
{
	fprintf (tblfp, "       filename\tp-miss \ttotal  \ttotal  \t# of  \ttime to\tpath to\tspeed  \tentr.  \t# of   \ttime to\tpath to\tspeed  \tsd of  \tline- \tmax.  \tmax.  \ttime to\tpath to\tspeed  \tTime   \tpTime  \tpTime  \tpTime  \tpTime  \tRayleig\tRayleig\tPolar  \tPolar  \tPolar  \tPolar  \tMin    \tMin    \tPolar  \tPolar  \tMax    \tMax    \tAnnular\tAnnular\tAnnular\tAnnular\tAnnular\tAnnular\tAnnular\tAnnular\tnext   \n");
	fprintf (tblfp, "               \t       \ttime   \tpath   \tentr. \tfirst  \tfirst  \tfirst  \tper    \tshocks \tfirst  \tfirst  \t       \tspeed  \tarity \ttime  \tpath  \tsecond \tsecond \tsecond \tTARG   \tTARG   \tCCW    \tOPP    \tCW     \tlength \tangle  \tavg    \tsd     \tmin    \tmin bin\t50%% rng\t50%% rng\tmax    \tmax bin\t50%% rng\t50%% rng\tmin+   \tmin+bin\tmax    \tmax bin\tavg    \tsd     \tSkewnes\tKurtosi\tparam  \n");
	fprintf (tblfp, "               \t       \t       \t       \t      \tentr.  \tentr.  \tentr. \tdist.   \t       \tshock  \tshock  \t       \t       \t      \tavoid.\tavoid.\tentr.  \tentr.  \tentr.  \t       \t       \t       \t       \t       \t       \t       \tval    \tval    \tval    \t       \tlo bin \thi bin \tval    \t       \tlo bin \thi bin \tval    \t       \tval    \t       \t       \t       \t       \t       \t       \n");
	fprintf (tblfp, "               \t       \t[s]    \t[m]    \t      \t[s]    \t[m]    \t[cm/s] \t[1/m]  \t       \t[s]    \t[m]    \t[cm/s] \t[cm/s] \t[0-1] \t[s]   \t[m]   \t[s]    \t[m]    \t[cm/s] \t[s]    \t       \t       \t       \t       \t[0-1]  \t[deg]  \t[deg]  \t[deg]  \t[0-1]  \t[deg]  \t[deg]  \t[deg]  \t[0-1]  \t[deg]  \t[deg]  \t[deg]  \t[0-1]  \t[cm]   \t[0-1]  \t[cm]   \t[cm]   \t[cm]   \t       \t       \t[]     \n");

	return;
}

void	PrintPlacePreferenceTableHeadings(FILE *tblfp)
{
	fprintf (tblfp, "       filename\tframe  \ttotal  \ttotal  \t# of  \ttime to\tspeed  \tmax.  \tEntr   \tEntr   \tEntr   \tEntr   \tTime   \tTime  \tTime  \tTime  \tnext   \n");
	fprintf (tblfp, "               \t       \ttime   \tpath   \treward\tfirst  \t       \ttime  \tTARG   \tCCW    \tOPP    \tCW     \tTARG   \tCCW   \tOPP   \tCW    \tparam  \n");
	fprintf (tblfp, "               \t       \t       \t       \t      \treward \t       \tno ent\t       \t       \t       \t       \t       \t      \t      \t      \t       \n");
	fprintf (tblfp, "               \t       \t[s]    \t[m]    \t      \t[s]    \t[cm/s] \t[s]   \t       \t       \t       \t       \t[s]    \t[s]   \t[s]   \t[s]   \t[]     \n");

	return;
}

void	PrintWatermazeTableHeadings(FILE *tblfp)
{
	fprintf (tblfp, "       filename\tsubject \ttrial # \tstart   \ttotal  \ttotal  \t# of  \ttime to\tspeed  \tmax.  \tEntr   \tEntr   \tEntr   \tEntr   \tTime   \tTime  \tTime  \tTime  \tTime  \tTime  \tTime  \tTime  \tnext   \n");
	fprintf (tblfp, "               \t        \t        \t        \ttime   \tpath   \treward\tfirst  \t       \ttime  \tTARG   \tCCW    \tOPP    \tCW     \tTARG   \tCCW   \tOPP   \tCW    \tQuad  \tQuad  \tQuad  \tQuad  \tparam  \n");
	fprintf (tblfp, "               \t        \t        \t        \t       \t       \t      \treward \t       \tno ent\t       \t       \t       \t       \t       \t      \t      \t      \tTARG  \tCCW   \tOPP   \tCW    \t       \n");
	fprintf (tblfp, "               \t        \t        \t        \t[s]    \t[m]    \t      \t[s]    \t[cm/s] \t[s]   \t       \t       \t       \t       \t[s]    \t[s]   \t[s]   \t[s]   \t[s]   \t[s]   \t[s]   \t[s]   \t[]     \n");

	return;
}

void	PrintOpenFieldTableHeadings(FILE *tblfp)
{
	fprintf (tblfp, "       filename\ttotal  \ttotal  \t# of  \tline-  \tspeed  \tpTime  \tpTime  \tpTime  \tpTime  \tpTime  \tpTime  \tpTime  \tpTime  \tnext   \n");
	fprintf (tblfp, "               \ttime   \tpath   \tline  \tarity  \t       \tann1   \tann2   \tann3   \tann4   \tann5   \tann6   \tann7   \tann8   \tparam  \n");
	fprintf (tblfp, "               \t       \t       \tcross \t       \t       \t       \t       \t       \t       \t       \t       \t       \t       \t       \n");
	fprintf (tblfp, "               \t[s]    \t[m]    \t      \t       \t[cm/s] \t       \t       \t       \t       \t       \t       \t       \t       \t[]     \n");

	return;
}

void	PrintTrackerTableHeadings(FILE *tblfp)
{
	fprintf (tblfp, "       filename\ttotal  \ttotal  \tline-  \tspeed  \tsd of  \tTime   \tTime   \tTime   \tTime   \tpTime  \tpTime  \tpTime  \tpTime  \tnext   \n");
	fprintf (tblfp, "               \ttime   \tpath   \tarity  \t       \tspeed  \tquadE  \tquadN  \tquadW  \tquadS  \tquadE  \tquadN  \tquadW  \tquadS  \tparam  \n");
	fprintf (tblfp, "               \t       \t       \t       \t       \t       \t       \t       \t       \t       \t       \t       \t       \t       \t       \n");
	fprintf (tblfp, "               \t[s]    \t[m]    \t       \t[cm/s] \t       \t[s]    \t[s]    \t[s]    \t[s]    \t       \t       \t       \t       \t[]     \n");

	return;
}

void	PrintPlaceAvoidanceTableReport(FILE *p, MEASURES m)
{

	fprintf(p, "%30s\t",	m.FileName);
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\n");
		return;
	}
	fprintf(p, "%0.5f\t",		(float)m.Misses / (float)m.NumberOfSamples);
	fprintf(p, "%d\t",		(int)(m.Duration + 0.5));		
	fprintf(p, "%0.2f\t",		(float)m.PathLength);	
	fprintf(p, "%d\t",		m.Entrances);	
	fprintf(p, "%0.2f\t",		m.TimeToFirstEntrance);	
	fprintf(p, "%0.2f\t",	(float)m.PathToFirstEntrance);	
	fprintf(p, "%0.2f\t",	(float)m.SpeedAtFirstEntrance);	
	fprintf(p, "%0.2f\t",	(float)(m.PathLength == 0.0) ? 0.0: (m.Entrances / m.PathLength));
	fprintf(p, "%d\t",		m.NumberOfShocks);
	fprintf(p, "%0.2f\t",		m.TimeToFirstShock);	
	fprintf(p, "%0.2f\t",	(float)m.PathToFirstShock);	
	fprintf(p, "%0.2f\t",	(float)m.Speed);
	fprintf(p, "%0.2f\t",	(float)m.sdSpeed);
	fprintf(p, "%0.4f\t",	(float)m.Linearity);
	fprintf(p, "%d\t",  	(int)m.MaxTimeAvoided); 
	fprintf(p, "%0.2f\t",	m.MaxPathAvoided);
	fprintf(p, "%0.2f\t",  	m.TimeToSecondEntrance);
	fprintf(p, "%0.2f\t",  	(float)m.PathToSecondEntrance);
	fprintf(p, "%0.2f\t",  	(float)m.SpeedAtSecondEntrance);
	fprintf(p, "%0.4lf\t",	m.TimeEntranceTARG);
	fprintf(p, "%0.4lf\t",	m.pTimeEntranceTARG);
	fprintf(p, "%0.4lf\t",	m.pTimeEntranceCCW);
	fprintf(p, "%0.4lf\t",	m.pTimeEntranceOPP);
	fprintf(p, "%0.4lf\t",	m.pTimeEntranceCW);
	fprintf(p, "%0.2f\t",	(float)(m.Polar.RayleighLength));
	fprintf(p, "%0.2f\t",	(float)(m.Polar.RayleighAng));
	fprintf(p, "%0.2f\t",	(float)(m.Polar.Avg));
	fprintf(p, "%0.2f\t",	(float)(m.Polar.SD));
	fprintf(p, "%0.4f\t",	(float)(m.Polar.Min));
	fprintf(p, "%d\t",	(int)(m.Polar.MinBin * m.Polar.BinSzDeg));
	fprintf(p, "%d\t",	(int)(m.Polar.LoMinBin * m.Polar.BinSzDeg));
	fprintf(p, "%d\t",	(int)(m.Polar.HiMinBin * m.Polar.BinSzDeg));
	fprintf(p, "%0.4f\t",	(float)(m.Polar.Max));
	fprintf(p, "%d\t",	(int)(m.Polar.MaxBin * m.Polar.BinSzDeg));
	fprintf(p, "%d\t",	(int)(m.Polar.LoMaxBin * m.Polar.BinSzDeg));
	fprintf(p, "%d\t",	(int)(m.Polar.HiMaxBin * m.Polar.BinSzDeg));
	fprintf(p, "%0.4f\t",	(float)(m.Annular.MinNonZero));
	fprintf(p, "%0.1f\t",	(float)(m.Annular.MinNonZeroBin));
	fprintf(p, "%0.4f\t",	(float)(m.Annular.Max));
	fprintf(p, "%0.1f\t",	(float)(m.Annular.MaxBin));
	fprintf(p, "%0.2f\t",	(float)(m.Annular.Avg));
	fprintf(p, "%0.2f\t",	(float)(m.Annular.SD));
	fprintf(p, "%0.2f\t",	(float)(m.Annular.Skewness));
	fprintf(p, "%0.2f\t",	(float)(m.Annular.Kurtosis));
	
	fprintf(p, "\n");

	return;
}

void	PrintPlacePreferenceTableReport(FILE *p, MEASURES m)
{

	fprintf(p, "%30s\t",	m.FileName);
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\n");
		return;
	}
	fprintf(p, "%s\t",		m.FrameName);
	fprintf(p, "%d\t",		(int)(m.Duration + 0.5));		
	fprintf(p, "%.2f\t",		(float)m.PathLength);	
	fprintf(p, "%d\t",		m.Entrances);	
	fprintf(p, "%0.2f\t",		m.TimeToFirstEntrance);	
	fprintf(p, "%.2f\t",	(float)m.Speed);
	fprintf(p, "%d\t",  	(int)m.MaxTimeAvoided); 
	fprintf(p, "%d\t",	m.EntrancesTARG);
	fprintf(p, "%d\t",	m.EntrancesCCW);
	fprintf(p, "%d\t",	m.EntrancesOPP);
	fprintf(p, "%d\t",	m.EntrancesCW);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceTARG);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceCCW);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceOPP);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceCW);
	
	fprintf(p, "\n");

	return;
}

void	PrintWatermazeTableReport(FILE *p, MEASURES m, INFO I)
{

	fprintf(p, "%30s\t",	m.FileName);
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\n");
		return;
	}
	fprintf(p, "%d\t",	I.TrialSubjectId);		
	fprintf(p, "%d\t",	I.TrialNumber);		
	fprintf(p, "%s\t",	I.TrialStartLabel);		
	fprintf(p, "%0.1f\t",	m.Duration);		
	fprintf(p, "%.2f\t",	(float)m.PathLength);	
	fprintf(p, "%d\t",	m.Entrances);	
	fprintf(p, "%0.2f\t",	m.TimeToFirstEntrance);	
	fprintf(p, "%.2f\t",	(float)m.Speed);
	fprintf(p, "%0.1f\t",  	m.MaxTimeAvoided); 
	fprintf(p, "%d\t",	m.EntrancesTARG);
	fprintf(p, "%d\t",	m.EntrancesCCW);
	fprintf(p, "%d\t",	m.EntrancesOPP);
	fprintf(p, "%d\t",	m.EntrancesCW);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceTARG);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceCCW);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceOPP);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceCW);
	fprintf(p, "%0.3lf\t",	m.QuadrantTimeTARG);
	fprintf(p, "%0.3lf\t",	m.QuadrantTimeCCW);
	fprintf(p, "%0.3lf\t",	m.QuadrantTimeOPP);
	fprintf(p, "%0.3lf\t",	m.QuadrantTimeCW);
	
	fprintf(p, "\n");

	return;
}

void	PrintTrackerTableReport(FILE *p, MEASURES m)
{

	fprintf(p, "%30s\t",	m.FileName);
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\n");
		return;
	}
	fprintf(p, "%d\t",		(int)(m.Duration + 0.5));		
	fprintf(p, "%.2f\t",		(float)m.PathLength);	
	fprintf(p, "%.4f\t",	(float)m.Linearity);
	fprintf(p, "%.2f\t",	(float)m.Speed);
	fprintf(p, "%.2f\t",	(float)m.sdSpeed);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceTARG);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceCCW);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceOPP);
	fprintf(p, "%0.3lf\t",	m.TimeEntranceCW);
	fprintf(p, "%0.3lf\t",	m.pTimeEntranceTARG);
	fprintf(p, "%0.3lf\t",	m.pTimeEntranceCCW);
	fprintf(p, "%0.3lf\t",	m.pTimeEntranceOPP);
	fprintf(p, "%0.3lf\t",	m.pTimeEntranceCW);
	
	fprintf(p, "\n");

	return;
} 

void	PrintOpenFieldTableReport(FILE *p, MEASURES m)
{
	int	i;

	fprintf(p, "%30s\t",	m.FileName);
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\n");
		return;
	}
	fprintf(p, "%d\t",	(int)(m.Duration + 0.5));		
	fprintf(p, "%.2f\t",	(float)m.PathLength);	
	fprintf(p, "%d\t",	(int)m.OField.LineCrossings);	
	fprintf(p, "%.4f\t",	(float)m.Linearity);
	fprintf(p, "%.2f\t",	(float)m.Speed);
	for(i=0; i < m.Annular.NBins; i++)
		fprintf(p, "%0.3lf\t",	m.Annular.Bin[i]);	
	fprintf(p, "\n");

	return;
} 

void	PrintTargetAnalysisHeadings(FILE *tafp)
{
	fprintf (tafp, "       filename\tTrial # \tSubject \tStart   \tpTarget\tEntr   \tEntr   \tEntr   \tEntr   \tTime  \tTime  \tTime  \tTime  \tnext   \n");
	fprintf (tafp, "               \t        \t        \t        \tsize   \tTARG   \tCCW    \tOPP    \tCW     \tTARG  \tCCW   \tOPP   \tCW    \tparam  \n");

	return;
}

void	PrintTargetAnalysis(FILE *p, MEASURES m, INFO I)
{

	fprintf(p, "%15s\t",	m.FileName);
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\n");
		return;
	}

	fprintf(p, "%d\t",	I.TrialNumber);	
	fprintf(p, "%d\t",	I.TrialSubjectId);	
	fprintf(p, "%s\t",	I.TrialStartLabel);	
	fprintf(p, "%0.2lf\t",	m.pTargetSize);	
	fprintf(p, "%3d\t",	m.EntrancesTARG);
	fprintf(p, "%3d\t",	m.EntrancesCCW);
	fprintf(p, "%3d\t",	m.EntrancesOPP);
	fprintf(p, "%3d\t",	m.EntrancesCW);
	fprintf(p, "%.4lf\t",	m.TimeEntranceTARG);
	fprintf(p, "%.4lf\t",	m.TimeEntranceCCW);
	fprintf(p, "%.4lf\t",	m.TimeEntranceOPP);
	fprintf(p, "%.4lf\t",	m.TimeEntranceCW);
	
	fprintf(p, "\n");

	return;
}
void	PrintZoneAnalysisHeadings(FILE *zafp)
{
	fprintf (zafp, "       filename\tInRad  \tEntr   \tEntr   \tEntr   \tEntr   \tTime  \tTime  \tTime  \tTime  \tnext   \n");
	fprintf (zafp, "               \tsize   \tTARG   \tCCW    \tOPP    \tCW     \tTARG  \tCCW   \tOPP   \tCW    \tparam  \n");

	return;
}

void	PrintZoneAnalysis(FILE *p, MEASURES m, INFO I)
{

	fprintf(p, "%15s\t",	m.FileName);
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\n");
		return;
	}

	fprintf(p, "%0.2lf\t",	m.pZoneSize);	
	fprintf(p, "%3d\t",	m.EntrancesTARG);
	fprintf(p, "%3d\t",	m.EntrancesCCW);
	fprintf(p, "%3d\t",	m.EntrancesOPP);
	fprintf(p, "%3d\t",	m.EntrancesCW);
	fprintf(p, "%.4lf\t",	m.TimeEntranceTARG);
	fprintf(p, "%.4lf\t",	m.TimeEntranceCCW);
	fprintf(p, "%.4lf\t",	m.TimeEntranceOPP);
	fprintf(p, "%.4lf\t",	m.TimeEntranceCW);
	
	fprintf(p, "\n");

	return;
}
void	PrintTargetQuadrantAnalysis(FILE *p, MEASURES m, INFO I)
{
	fprintf(p, "%15s\t",	m.FileName);
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\n");
		return;
	}
	fprintf(p, "%d\t",	I.TrialNumber);	
	fprintf(p, "%d\t",	I.TrialSubjectId);	
	fprintf(p, "%s\t",	I.TrialStartLabel);	
	fprintf(p, "quad\t");	
	fprintf(p, "%3d\t",	-1);
	fprintf(p, "%3d\t",	-1);
	fprintf(p, "%3d\t",	-1);
	fprintf(p, "%3d\t",	-1);
	fprintf(p, "%.4lf\t",	m.QuadrantTimeTARG);
	fprintf(p, "%.4lf\t",	m.QuadrantTimeCCW);
	fprintf(p, "%.4lf\t",	m.QuadrantTimeOPP);
	fprintf(p, "%.4lf\t",	m.QuadrantTimeCW);
	
	fprintf(p, "\n");

	return;
}
void	PrintZoneQuadrantAnalysis(FILE *p, MEASURES m, INFO I)
{
	fprintf(p, "%15s\t",	m.FileName);
	if(strstr(m.FileName, NO_FILE_TEXT) != NULL){
		fprintf(p, "\n");
		return;
	}
	fprintf(p, "quad\t");	
	fprintf(p, "%3d\t",	-1);
	fprintf(p, "%3d\t",	-1);
	fprintf(p, "%3d\t",	-1);
	fprintf(p, "%3d\t",	-1);
	fprintf(p, "%.4lf\t",	m.QuadrantTimeTARG);
	fprintf(p, "%.4lf\t",	m.QuadrantTimeCCW);
	fprintf(p, "%.4lf\t",	m.QuadrantTimeOPP);
	fprintf(p, "%.4lf\t",	m.QuadrantTimeCW);
	
	fprintf(p, "\n");

	return;
}

void TargetAnalysis(TRACK *Track, INFO *I, MEASURES *m, int frame, OPTIONS Options){

	unsigned char	**ReinforcedMap;
	CIRCLE		Circle;
	MEASURES	TAm;
	double		Rad, CartesianX, CartesianY, Ang, OriginalAng, ThisAng;
	int		TriggerLatency;


	TAm = *m;
	Circle = I->Target[frame].Circle;
	Circle.Rad = (float)(I->Target[frame].Circle.Rad * m->pTargetSize);

	CartesianY = I->Arena.CenterY - Circle.Y;
	CartesianX = Circle.X - I->Arena.CenterX;
	OriginalAng = atan2(CartesianY, CartesianX);
	Rad = hypot(CartesianX, CartesianY);


	// count entrances for the 4 target areas
	for(ThisAng = 0.0; ThisAng < (2*M_PI); ThisAng += (M_PI/2.0)){
		Ang = OriginalAng + ThisAng;
		CartesianX = cos(Ang) * Rad;
		CartesianY = sin(Ang) * Rad; 
		Circle.X = (float)(CartesianX + I->Arena.CenterX);
		Circle.Y = (float)(I->Arena.CenterY - CartesianY);
	

		MakePreferenceMap(&ReinforcedMap, *I, Circle, Options.OverlappingTargets);
		// do these calculations for each definition of the target because the areas can be sometimes defined so they overlap. 

		// Count a crossing even if a single sample is detected in the target so adjust this in the I structure 
		TriggerLatency = I->Feeder[frame].Trigger;
		I->Feeder[frame].Trigger = 10;	// set so 10 ms is sufficient to register an entrance
		PP_TimeBasedEntranceMeasures(Track, 0, I->NumberOfSamps, *I, ReinforcedMap, &TAm, frame);
		I->Feeder[frame].Trigger = TriggerLatency;

		if(ThisAng == 0.0){
			m->EntrancesTARG = TAm.Entrances;
		}else if(ThisAng ==  M_PI/2.0){
			m->EntrancesCCW = TAm.Entrances;
		}else if(ThisAng == M_PI){
			m->EntrancesOPP = TAm.Entrances;
		}else if(ThisAng == (3.0 * M_PI/2.0)){
			m->EntrancesCW = TAm.Entrances;
		}

		CalculateTimePerArea(Track, 0, I->NumberOfSamps, ReinforcedMap, &TAm);
		if(ThisAng == 0.0){
			m->TimeEntranceTARG = TAm.TimeEntranceTARG;
			m->pTimeEntranceTARG = TAm.pTimeEntranceTARG;
		}else if(ThisAng ==  M_PI/2.0){
			m->TimeEntranceCCW = TAm.TimeEntranceTARG;
			m->pTimeEntranceCCW = TAm.pTimeEntranceTARG;
		}else if(ThisAng == M_PI){
			m->TimeEntranceOPP = TAm.TimeEntranceTARG;
			m->pTimeEntranceOPP = TAm.pTimeEntranceTARG;
		}else if(ThisAng == (3.0 * M_PI/2.0)){
			m->TimeEntranceCW = TAm.TimeEntranceTARG;
			m->pTimeEntranceCW = TAm.pTimeEntranceTARG;
		}
	}

	return;
}
void QuadrantTargetAnalysis(TRACK *Track, INFO *I, MEASURES *m, int frame, OPTIONS Options){

	unsigned char	**ReinforcedMap;
	CIRCLE		Circle;
	MEASURES	TAm;
	double		CartesianX, CartesianY, OriginalAng;

	TAm = *m;
	Circle = I->Target[frame].Circle;
	Circle.Rad = (float)(I->Target[frame].Circle.Rad * m->pTargetSize);

	CartesianY = I->Arena.CenterY - Circle.Y;
	CartesianX = Circle.X - I->Arena.CenterX;
	OriginalAng = atan2(CartesianY, CartesianX);
	MakeQuadrantMap(&ReinforcedMap, *I, OriginalAng);
	CalculateTimePerArea(Track, 0, I->NumberOfSamps, ReinforcedMap, &TAm);
        m->QuadrantTimeTARG = TAm.TimeEntranceTARG;
        m->pQuadrantTimeTARG = TAm.pTimeEntranceTARG;
        m->QuadrantTimeCCW = TAm.TimeEntranceCCW;
        m->pQuadrantTimeCCW = TAm.pTimeEntranceCCW;
        m->QuadrantTimeOPP = TAm.TimeEntranceOPP;
        m->pQuadrantTimeOPP = TAm.pTimeEntranceOPP;
        m->QuadrantTimeCW = TAm.TimeEntranceCW;
        m->pQuadrantTimeCW = TAm.pTimeEntranceCW;

	return;
}

void QuadrantZoneAnalysis(TRACK *Track, INFO *I, MEASURES *m, int frame, OPTIONS Options){

	unsigned char	**ReinforcedMap;
	SECTOR		Sector;
	MEASURES	ZAm;
	double		CartesianX, CartesianY, OriginalAng;

	OriginalAng = I->Target[frame].Sector.Ang;
	OriginalAng /= DEG_PER_RAD;
	MakeQuadrantMap(&ReinforcedMap, *I, OriginalAng);

	CalculateTimePerArea(Track, 0, I->NumberOfSamps, ReinforcedMap, &ZAm);
        m->QuadrantTimeTARG = ZAm.TimeEntranceTARG;
        m->pQuadrantTimeTARG = ZAm.pTimeEntranceTARG;
        m->QuadrantTimeCCW = ZAm.TimeEntranceCCW;
        m->pQuadrantTimeCCW = ZAm.pTimeEntranceCCW;
        m->QuadrantTimeOPP = ZAm.TimeEntranceOPP;
        m->pQuadrantTimeOPP = ZAm.pTimeEntranceOPP;
        m->QuadrantTimeCW = ZAm.TimeEntranceCW;
        m->pQuadrantTimeCW = ZAm.pTimeEntranceCW;

	return;
}

void ZoneAnalysis(TRACK *Track, INFO *I, MEASURES *m, int frame, OPTIONS Options){

	unsigned char	**ReinforcedMap;
	SECTOR		Sector;
	MEASURES	ZAm;
	double		Rad, CartesianX, CartesianY, Ang, OriginalAng, ThisAng;

	ZAm = *m;
	Sector = I->Target[frame].Sector;
      	Sector.OutRad = I->Arena.CenterX + 0.5;
        Sector.InRad = m->pZoneSize * Sector.OutRad;
	OriginalAng = I->Target[frame].Sector.Ang;

        MakeAvoidSectorMap(&ReinforcedMap, *I, Sector);
	// do these calculations for each definition of the target because the areas can be sometimes defined so they overlap. 

	for(ThisAng = 0.0; ThisAng < 360.0; ThisAng += 90.0){
	        Sector.Ang = OriginalAng + ThisAng;
		if(Sector.Ang >= 360.0)
			Sector.Ang -= 360.0;

               	MakeAvoidSectorMap(&ReinforcedMap, *I, Sector);

		PA_TimeBasedEntranceMeasures(Track, 0, I->NumberOfSamps, *I, ReinforcedMap, &ZAm, frame);

		if(ThisAng == 0.0){
			m->EntrancesTARG = ZAm.Entrances;
		}else if(ThisAng ==  90.0){
			m->EntrancesCCW = ZAm.Entrances;
		}else if(ThisAng == 180.0){
			m->EntrancesOPP = ZAm.Entrances;
		}else if(ThisAng == 270.0){
			m->EntrancesCW = ZAm.Entrances;
		}

		CalculateTimePerArea(Track, 0, I->NumberOfSamps, ReinforcedMap, &ZAm);
		if(ThisAng == 0.0){
			m->TimeEntranceTARG = ZAm.TimeEntranceTARG;
			m->pTimeEntranceTARG = ZAm.pTimeEntranceTARG;
		}else if(ThisAng == 90.0){
			m->TimeEntranceCCW = ZAm.TimeEntranceTARG;
			m->pTimeEntranceCCW = ZAm.pTimeEntranceTARG;
		}else if(ThisAng == 180.0){
			m->TimeEntranceOPP = ZAm.TimeEntranceTARG;
			m->pTimeEntranceOPP = ZAm.pTimeEntranceTARG;
		}else if(ThisAng == 270.0){
			m->TimeEntranceCW = ZAm.TimeEntranceTARG;
			m->pTimeEntranceCW = ZAm.pTimeEntranceTARG;
		}
	}

/*
	CalculateTimePerArea(Track, 0, I->NumberOfSamps, ReinforcedMap, &ZAm);
        m->TimeEntranceTARG = ZAm.TimeEntranceTARG;
        m->pTimeEntranceTARG = ZAm.pTimeEntranceTARG;
        m->TimeEntranceCCW = ZAm.TimeEntranceCCW;
        m->pTimeEntranceCCW = ZAm.pTimeEntranceCCW;
        m->TimeEntranceOPP = ZAm.TimeEntranceOPP;
        m->pTimeEntranceOPP = ZAm.pTimeEntranceOPP;
        m->TimeEntranceCW = ZAm.TimeEntranceCW;
        m->pTimeEntranceCW = ZAm.pTimeEntranceCW;
*/

	return;
}
