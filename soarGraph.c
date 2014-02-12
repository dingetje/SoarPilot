#include <PalmOS.h>
#include "soaring.h"
#include "soarDB.h"
#include "soarUtil.h"
#include "soarGraph.h"
#include "soarUGraph.h"
#include "soarWind.h"
#include "soarMem.h"

void DrawAltTimeGraph(Int16 fltindex, UInt8 graphtype)
{
	Graph_t *appgr;
	Graph_t *appgr2;
	DateTimeType strtdt, stopdt;
	UInt32 startsecs, stopsecs, lengthsecs;
	UInt32 prevsecs=0;
	UInt32 graphsecs=0;
	UInt32 maxscalevalue=0;
	UInt32 minscalevalue=0;
	double prevalt=0.0;
	double prevterelev=0.0;
	MemHandle output_hand;
	MemPtr output_ptr;
	FlightData *fltdata;
	Int32 recindex;
	LoggerData *logdata;
	Boolean firsttime=true;
	Char tempchar[10];
	Coord ulX, ulY, widthX, heightY;
	Boolean altlabeldrawn = false;
	Int16 prevtaskleg = -1;
	Int16 prevhour = -1;
	Int16 prevtasklegsecs = -1;

	if(graphtype == 0) {

		ulX = 5;
		ulY = 25;
		widthX = 150;
		heightY = 100;

		AllocMem((void *)&fltdata, sizeof(FlightData));
		AllocMem((void *)&logdata, sizeof(LoggerData));
		AllocMem((void *)&appgr, sizeof(Graph_t));
		AllocMem((void *)&appgr2, sizeof(Graph_t));

		//must call this. This initializes the variables in the graph and ensures
		//there are no spurious values. The values of initialization can be changed
		//by changing respective values in the grlib.h header files

		grLibInitializeGraphStruct(appgr);	
		grLibInitializeGraphStruct(appgr2);	

		OpenDBQueryRecord(flight_db, fltindex, &output_hand, &output_ptr);
		MemMove(fltdata,output_ptr,sizeof(FlightData));
		MemHandleUnlock(output_hand);
		// Copy the flight start and stop DTG(MMDDYY) into the Start & Stop DateTime Structure
		// Copy the flight start and stop UTC(HHMMSS) into the Start & Stop DateTime Structure
		StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &strtdt);
		StringToDateAndTime(fltdata->stopdtg, fltdata->stoputc, &stopdt);

		// Get the start and stop times converted into seconds
		startsecs = TimDateTimeToSeconds(&strtdt);
		stopsecs = TimDateTimeToSeconds(&stopdt);

		// Calculate flt length 
		if (startsecs > stopsecs) lengthsecs = 0; else lengthsecs=stopsecs-startsecs; 

		//set the graph perimeters and the Type (LINE or LINE_FILL) and specify if draw in
		//color or not
		grLibSetGraphParams(appgr, ulX, ulY, widthX, heightY, LINE, device.colorCapable);
		grLibSetGraphParams(appgr2, ulX, ulY, widthX, heightY, LINE_FILL_HASH, device.colorCapable);
//		grLibSetGraphParams(appgr2, ulX, ulY, widthX, heightY, LINE, device.colorCapable);

		maxscalevalue = (UInt32)(pround(fltdata->maxalt*data.input.altconst, 0));
		if (maxscalevalue < 10) {
			maxscalevalue = 10;
		} else if (maxscalevalue < 100) {
			maxscalevalue = 100;
		} else if (maxscalevalue < 1000) {
			maxscalevalue = ((maxscalevalue / 100) + 1) * 100;
		} else if (maxscalevalue < 10000) {
			maxscalevalue = ((maxscalevalue / 1000) + 1) * 1000;
		} else if (maxscalevalue < 100000) {
			maxscalevalue = ((maxscalevalue / 10000) + 1) * 10000;
		}

		minscalevalue = (UInt32)(pround(fltdata->minalt*data.input.altconst, 0));
		//specify the scales (max and min X and Y) - these are the data ranges
		grLibSetMaxMinScales(appgr, 0, 0, lengthsecs, maxscalevalue); 
		grLibSetMaxMinScales(appgr2, 0, 0, lengthsecs, maxscalevalue); 
//		grLibSetMaxMinScales(&appgr, 0, minscalevalue, lengthsecs, maxscalevalue); 

		//draw an outer frame for the graph. This routine will use the frame
		//parameters set in grLibSetGraphParams
		grLibDrawGraphFrame(appgr,FRAME);

		grLibDrawGraphGrids(appgr,YGRID,YTEXT,5); 

		for (recindex = fltdata->startidx; recindex < fltdata->stopidx; recindex++) {
			OpenDBQueryRecord(logger_db, recindex, &output_hand, &output_ptr);
			MemMove(logdata, output_ptr, sizeof(LoggerData));
			MemHandleUnlock(output_hand);
			// Copy the current data points DTG(MMDDYY) into the Start & Stop DateTime Structure
			// Copy the Stop time UTC (HHMMSS) into the Stop DateTime Structure
			StringToDateAndTime(logdata->gpsdtg, logdata->gpsutc, &stopdt);
			graphsecs=TimDateTimeToSeconds(&stopdt) - startsecs;

			if (firsttime) {
				// Output the start time with seconds
				SecondsToDateOrTimeString(startsecs, tempchar, 1, (Int32)data.config.timezone);
				grLibDrawGraphText(appgr, graphsecs, 12, 0, -2, "|");
				grLibDrawGraphText(appgr, graphsecs, 12, 0, 7, "|");
				grLibDrawGraphText(appgr, graphsecs, 0, -4, 15, tempchar);
				prevtaskleg = logdata->taskleg;
				prevhour = (Int16)((graphsecs+startsecs)/3600);
				prevtasklegsecs = graphsecs;
			} else {
				appgr->numDataPts = 0;
				appgr2->numDataPts = 0;
				if ((Int16)((graphsecs+startsecs)/3600) > prevhour) {
					// draw tick marks at hourly intervals
					grLibDrawGraphText(appgr, graphsecs, 12, 0, -2, "|");
					prevhour = (Int16)((graphsecs+startsecs)/3600);
				}
				if (logdata->taskleg != prevtaskleg) {
					// draw line for task leg change
					grLibInputGraphData(appgr, prevsecs, minscalevalue, 0, NULL);	
					grLibInputGraphData(appgr, graphsecs,  maxscalevalue, 0, NULL);
					if (prevtaskleg >= 0) {
						// label task leg
						grLibDrawGraphText(appgr, (graphsecs+prevtasklegsecs)/2, 12, -2, -10, DblToStr(prevtaskleg,0));
					}
					prevtaskleg = logdata->taskleg;
					prevtasklegsecs = graphsecs;
				} else {
					// draw normal altitude
					grLibInputGraphData(appgr, prevsecs,(UInt32)(pround(prevalt*data.input.altconst, 0)), 0, NULL);
					grLibInputGraphData(appgr, graphsecs, (UInt32)(pround(logdata->gpsalt*data.input.altconst, 0)), 0, NULL);
//					grLibInputGraphData(appgr, graphsecs, (UInt32)(pround(logdata->pressalt*data.input.altconst, 0)), 0, NULL);
				}

				// draw ground
				grLibInputGraphData(appgr2, prevsecs, (UInt32)(pround(prevterelev*data.input.altconst, 0)), 0, NULL);
				grLibInputGraphData(appgr2, graphsecs,  (UInt32)(pround(logdata->terelev*data.input.altconst, 0)), 0, NULL);

				if (logdata->gpsalt >= fltdata->maxalt && altlabeldrawn == false) {
					// Output the max altitude when it is reached
					grLibDrawGraphText(appgr, graphsecs, (UInt32)(pround(logdata->gpsalt*data.input.altconst, 0)),   5, -10, print_altitude(fltdata->maxalt));
//					grLibDrawGraphText(appgr, graphsecs, (UInt32)(pround(logdata->pressalt*data.input.altconst, 0)), 5, -10, print_altitude(fltdata->maxalt));
					altlabeldrawn = true;
				}

				// draw the graphs
				grLibDrawGraph(appgr,0);
//				grLibDrawGraph(appgr,1);
				grLibDrawGraph(appgr2,0);
			}
			firsttime = false;
			prevsecs = graphsecs;
			prevalt = logdata->gpsalt;
			prevterelev = logdata->terelev;
		}
		// Output the end time
		SecondsToDateOrTimeString(stopsecs, tempchar, 1, (Int32)data.config.timezone);
		grLibDrawGraphText(appgr, graphsecs, 12, 0, -2, "|");
		grLibDrawGraphText(appgr, graphsecs, 12, 0, 7, "|");
		grLibDrawGraphText(appgr, graphsecs, 0, -30, 15, tempchar);

		FreeMem((void *)&fltdata);
		FreeMem((void *)&logdata);
		grLibDrawGraphTitle(appgr,"Altitude vs. Time");
		grLibReleaseResources(appgr);
		grLibReleaseResources(appgr2);
		FreeMem((void *)&appgr);
		FreeMem((void *)&appgr2);
	}
}

void DrawPctThermalGraph(Int16 fltindex, UInt8 graphtype)
{
	Graph_t *appgr;
	DateTimeType strtdt, stopdt;
	UInt32 startsecs, stopsecs, lengthsecs;
	UInt32 graphsecs=0;
	UInt32 prevcursecs=0;
	UInt32 intervalsecs = 0;
	UInt32 plotsecs = 0;
	UInt32 maxscalevalue=0;
	UInt32 minscalevalue=0;
	MemHandle output_hand;
	MemPtr output_ptr;
	FlightData *fltdata;
	Int32 recindex;
	LoggerData *logdata;
	Boolean firsttime=true;
	Char tempchar[10];
	Coord ulX, ulY, widthX, heightY;
	UInt16 thermalcount=0;
	UInt32 pctthermal;
	UInt16 x=1;
	UInt16 plotcount=1;
	UInt16 maxplotcount;
	Int16 prevtaskleg=0;
	Boolean taskstarted = false;

	//must call this. This initializes the variables in the graph and ensures
	//there are no spurious values. The values of initialization can be changed
	//by changing respective values in the grlib.h header files

	AllocMem((void *)&appgr, sizeof(Graph_t));

	grLibInitializeGraphStruct(appgr);	

	ulX = 5;
	ulY = 25;
	widthX = 150;
	heightY = 100;

	AllocMem((void *)&fltdata, sizeof(FlightData));
	AllocMem((void *)&logdata, sizeof(LoggerData));

	OpenDBQueryRecord(flight_db, fltindex, &output_hand, &output_ptr);
	MemMove(fltdata, output_ptr, sizeof(FlightData));
	MemHandleUnlock(output_hand);

//	HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-fltdata->flttask.numwaypts-|%hu|", fltdata->flttask.numwaypts);
//	HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-fltdata->tskstartutc-|%s|", fltdata->tskstartutc);
//	HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-fltdata->tskstoputc-|%s|", fltdata->tskstoputc);
	if (StrCompare(fltdata->tskstoputc, "NT")==0 || fltdata->flttask.numwaypts <= 0) {
		// no task started so plot by time
		maxplotcount = 7;
		// Copy the flight start and stop DTG(MMDDYY) into the Start & Stop DateTime Structure
		// Copy the flight start and stop UTC(HHMMSS) into the Start & Stop DateTime Structure
		StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &strtdt);
		StringToDateAndTime(fltdata->stopdtg, fltdata->stoputc, &stopdt);

		// Get the start and stop times converted into seconds
		startsecs = TimDateTimeToSeconds(&strtdt);
//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-startsecs-|%lu|", startsecs);
		stopsecs = TimDateTimeToSeconds(&stopdt);
//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-stopsecs-|%lu|", stopsecs);

		// Calculate flt length 
		if (startsecs > stopsecs) lengthsecs = 0; else lengthsecs=stopsecs-startsecs; lengthsecs=stopsecs-startsecs; 
//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-lengthsecs-|%lu|", lengthsecs);
		intervalsecs = lengthsecs / (UInt32)(maxplotcount - 1);
//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-intervalsecs-|%lu|", intervalsecs);

		//set the graph perimeters and the Type (BAR, LINE, LINE_FILL, STACK) and specify if draw in
		//color or not
		grLibSetGraphParams(appgr,ulX, ulY, widthX, heightY, BAR, device.colorCapable);

		maxscalevalue = 125;

		minscalevalue = 0;
		//specify the scales (max and min X and Y) - these are the data ranges
		grLibSetMaxMinScales(appgr, 0, 0, (UInt32)maxplotcount, maxscalevalue); 

		//draw an outer frame for the graph. This routine will use the frame
		//parameters set in grLibSetGraphParams
		grLibDrawGraphFrame(appgr,FRAME);

		grLibDrawGraphGrids(appgr,YGRID,YTEXT,5); 

//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-startidx-|%hd|", fltdata->startidx);
//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-stopidx-|%hd|", fltdata->stopidx);
		for (recindex = fltdata->startidx; recindex < fltdata->stopidx; recindex++) {
//			HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-recindex-|%hd|", recindex);
			OpenDBQueryRecord(logger_db, recindex, &output_hand, &output_ptr);
//			HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-OpenDBQueryRecord-|%hd|", recindex);
			MemMove(logdata, output_ptr, sizeof(LoggerData));
//			HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-MemMove");
			MemHandleUnlock(output_hand);
//			HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-MemHandleUnlock");
			// Copy the current data points DTG(MMDDYY) into the Start & Stop DateTime Structure
			// Copy the Stop time UTC (HHMMSS) into the Stop DateTime Structure
			StringToDateAndTime(logdata->gpsdtg, logdata->gpsutc, &stopdt);
//			HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-StringToDateAndTime");
			graphsecs=TimDateTimeToSeconds(&stopdt) - startsecs;
//			HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-graphsecs-|%lu|", graphsecs);
			if (firsttime) {
				firsttime = false;
				// Output the start time with seconds
				SecondsToDateOrTimeString(startsecs, tempchar, 1, (Int32)data.config.timezone);
				grLibDrawGraphText(appgr, (UInt32)plotcount, 0, -10, 3, tempchar); 
				prevcursecs = graphsecs;
			} 
			// If in THERMAL mode, increment the thermal posit count to allow for determining
			// the thermal vs. cruise percentage
			if (logdata->thermal == THERMAL) {
				thermalcount++;
			}

//			HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-plotsecs-|%lu|", plotsecs);
			if ((prevcursecs != graphsecs) && ((graphsecs >= plotsecs+intervalsecs) || (recindex == (fltdata->stopidx-1)))) {
				plotsecs += intervalsecs;
//				HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-x-|%hu|", x);
				pctthermal = (UInt32)pround(((double)thermalcount / (double)x * 100.0), 0);
//				HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-pctthermal-|%lu|", pctthermal);

				grLibInputGraphData(appgr, (UInt32)plotcount, pctthermal, 1, NULL);

				if (pctthermal >= 0) {
					grLibDrawGraphText(appgr, (UInt32)plotcount, pctthermal, -4, -12, StrIToA(tempchar, pctthermal));
				}
				thermalcount = 0;
				x = 0; // x is the number of logger records being used to find the percentage
				plotcount++;
//				HostTraceOutputTL(appErrorClass, "=======================================");
			}
			x++;
			prevcursecs = graphsecs;
//			HostTraceOutputTL(appErrorClass, "---------------------------------------");
		}
		grLibDrawGraph(appgr,1);
		// Output the end time
		SecondsToDateOrTimeString(stopsecs, tempchar, 1, (Int32)data.config.timezone);
		grLibDrawGraphText(appgr, (UInt32)plotcount, 0, -30, 3, tempchar); 

		grLibDrawGraphTitle(appgr,"%Thermal vs. Time");
	} else {
		// plot % thermal by task leg
		taskstarted = false;
		maxplotcount = fltdata->flttask.numwaypts;
		if (fltdata->flttask.hastakeoff) maxplotcount--;
		if (fltdata->flttask.haslanding) maxplotcount--;
//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-taskleg-maxplotcount-|%hu|", maxplotcount);
		//set the graph perimeters and the Type (BAR, LINE, LINE_FILL, STACK) and specify if draw in
		//color or not
		grLibSetGraphParams(appgr,ulX, ulY, widthX, heightY, BAR, device.colorCapable);

		maxscalevalue = 125;

		minscalevalue = 0;
		//specify the scales (max and min X and Y) - these are the data ranges
		grLibSetMaxMinScales(appgr, 0, 0, (UInt32)maxplotcount, maxscalevalue); 

		//draw an outer frame for the graph. This routine will use the frame
		//parameters set in grLibSetGraphParams
		grLibDrawGraphFrame(appgr,FRAME);

		grLibDrawGraphGrids(appgr,YGRID,YTEXT,5); 

//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-startidx-|%lu|", fltdata->startidx);
//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-stopidx-|%lu|", fltdata->stopidx);
//		HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-records-|%lu|", fltdata->stopidx-fltdata->startidx+1);
//		HostTraceOutputTL(appErrorClass, "-----");
		for (recindex = fltdata->startidx; recindex < fltdata->stopidx; recindex++) {
//			HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-recindex-|%ld|", recindex);
			OpenDBQueryRecord(logger_db, recindex, &output_hand, &output_ptr);
			MemMove(logdata, output_ptr, sizeof(LoggerData));
			MemHandleUnlock(output_hand);

			if ((logdata->taskleg != -1) || (taskstarted)) {
				taskstarted = true;

				// If in THERMAL mode, increment the thermal posit count to allow for determining
				// the thermal vs. cruise percentage
				if (logdata->thermal == THERMAL) {
//				if (logdata->thermal == CRUISE) {
					thermalcount++;
				}
	
//				HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-logdata->taskleg-|%hd|", logdata->taskleg);
				if (((prevtaskleg > 0) && (prevtaskleg != logdata->taskleg)) || (recindex == (fltdata->stopidx-1))) {
//					HostTraceOutputTL(appErrorClass, "-");
//					HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-prevleg -|%hu|", prevtaskleg);
//					HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-x -|%hu|", x);
//					HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-th-|%hu|", thermalcount);
					pctthermal = (UInt32)pround(((double)thermalcount / (double)x * 100.0), 0);
//					HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-pctthermal-|%ld|", pctthermal);
//					HostTraceOutputTL(appErrorClass, "DrawPctThermalGraph-recindex-|%ld|", recindex-fltdata->startidx);
//					HostTraceOutputTL(appErrorClass, "-");

					grLibInputGraphData(appgr, (UInt32)prevtaskleg, pctthermal, 0, NULL);

					if (pctthermal >= 0) {
						grLibDrawGraphText(appgr, (UInt32)prevtaskleg, pctthermal, -4, -12, StrIToA(tempchar, pctthermal));
						grLibDrawGraphText(appgr, (UInt32)prevtaskleg, 0, 0, 3, StrIToA(tempchar, prevtaskleg)); 
					}
					thermalcount = 0;
					x = 0; // x is the number of logger records being used to find the percentage
				} 
				x++;
				prevtaskleg = logdata->taskleg;
				if (prevtaskleg == -1) taskstarted = false;
			}
		}
		grLibDrawGraph(appgr,0);

		grLibDrawGraphTitle(appgr,"%Thermal vs. Task Leg");

	}
	grLibReleaseResources(appgr);
	FreeMem((void *)&fltdata);
	FreeMem((void *)&logdata);
	FreeMem((void *)&appgr);
}
