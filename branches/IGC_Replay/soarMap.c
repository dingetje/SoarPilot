#include <PalmOS.h>
#include "soarMap.h"
#include "soarUMap.h"
#include "soaring.h"
#include "soarForm.h"
#include "soarUtil.h"
#include "soarMem.h"
#include "soarDB.h"
#include "soarTask.h"
#include "soarSUA.h"
#include "soarWind.h"
#include "soarWLst.h"
#include "soarSTF.h"
#include "soarMath.h"
#include "soarTer.h"
#include "soarWay.h"

//#define PRINT_FPS // Used to show drawing speed
Boolean updatemap = false;
double truecse;

// screen size & scale
double curmapscale = 20.0;
double actualmapscale = 20.0;
double savmapscale = 20.0;
double xdist, ydistul, ydistll;
double ulRng, llRng;
double xratio, yratio, bigratio;
double mapmaxlat, mapmaxlon;
double mapminlat, mapminlon;
Int16 TextOff = 0;
Boolean firsttime = true;

// map modes
UInt8 mapmode = CRUISE;
Boolean draw_log = false;
Boolean draw_task = false;
Boolean IsMap = false;

// externals
extern IndexedColorType indexGreen, indexRed, indexSector, indexBlack, indexTask;
extern double bigratio;
extern Int16 activetskway;
extern Int16 selectedTaskWayIdx;
extern Int16 selectedFltindex;
extern Int16 profilemaxalt;
extern Int16 profileminalt;
extern WindProfileType *windprofile;
extern double profilestep;
extern Boolean recv_data;
extern double MCCurVal;
extern Boolean taskonhold;
extern Boolean reachedwayppoint;
extern Int32 SUAdrawclasses;
extern TaskData *edittsk;
extern double MCMult;
extern Char timeinfo[15];
extern double calcstfhwind;
extern Boolean taskpreview;

// external variables for terrain
extern Boolean terrainpresent;
extern Boolean terrainvalid;
extern Boolean offterrain;
extern Boolean tskoffterrain;
extern Boolean crash;
extern double crashalt;
extern double crashlat;
extern double crashlon;
extern Boolean crash1;
extern double crashalt1;
extern double crashlat1;
extern double crashlon1;
extern Boolean wptcrash;
extern double wptcrashalt;
extern double wptcrashlat;
extern double wptcrashlon;
extern double *wptterheights;
extern Int32 numwptter;
extern Int32 prevnumwptter;
extern double prevterbear;
extern Boolean tskcrash;
extern double tskcrashalt;
extern double tskcrashlat;
extern double tskcrashlon;
extern double *tskterheights;
extern Int32 numtskter;
extern Int32 prevnumtskter;

// functions
void UpdateMap2(Boolean cleanup, double gliderLat, double gliderLon, double mapscale, Boolean ismap, TaskData *tsk, Boolean gliderpos)
{
	static double scale;
	double poirange, poibearing;
	double WaySTF;
	double startalt;
	Char tempchar[20];
	Int16 plotX, plotY;
	static WinHandle winH, activeWinH;
	UInt16 errW;
	MemHandle output_hand;
	MemPtr output_ptr;
	RectangleType rectP;
	Boolean plotgood;
	Boolean leftturn;
	Char text[30];
	double tmpstf, tmpalt;
	double maxdist;
	double tmpdbl=0.0;
	double fgatpdist=0.0, fgatpbear=0.0;
	double fgatpalt1=0.0;
	double fgatpalt2=0.0;
	double firstaltloss;
	double secondaltloss;
	Int8 fieldadj;
	double boldline = 1.0;
	Int16 TOff;
	FlightData *fltdata;
	LoggerData *logdata;
	UInt32 recindex;
	double pointlat, pointlon;
	Int16 prevplotX = -1, prevplotY = -1;
	Boolean prevplotgood = false;
	Boolean firstpoint;
	Int8 i, lftprec;
	Char fgotchar[10];
	Int16 WfgplotX, WfgplotY;
	Int16 TfgplotX, TfgplotY;
	double refRng, refBrg;
	Int16 posX, posY;
	double ld;
	double temprng, tempbrg;
	Boolean centerglider;
	
#ifdef PRINT_FPS
	UInt32 t=0;
	UInt32 old_t=0;
	Int32 fps;
#endif

//	HostTraceOutputTL(appErrorClass, "Updatemap - Begin");

	IsMap = ismap;
	startalt = data.input.inusealt;

	// Calculate the altitude required for the current distance to the waypoint
	if (data.config.inrngcalc) {
//		HostTraceOutputTL(appErrorClass, "Using Accurate Calculations");
		CalcSTFSpdAlt2(0, 1.0, RecipCse(data.input.wnddir), &tmpstf, &tmpalt);
	} else {
//		HostTraceOutputTL(appErrorClass, "Using Less Accurate Calculations");
		CalcSTFSpdAlt2(0, 1.0, data.input.magnetic_track.value, &tmpstf, &tmpalt);
	}
//	HostTraceOutputTL(appErrorClass, "tmpalt1=|%s|", DblToStr(tmpalt, 1));
	maxdist = (startalt-data.config.safealt) / tmpalt;
//	HostTraceOutputTL(appErrorClass, "maxdist=|%s|", DblToStr(maxdist, 1));

	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(device.HiDensityScrPresent);
	}

	if (cleanup) {
		if (winH) {
			WinDeleteWindow(winH, true);
		}
		return;
	}

	if (device.DIACapable ) {
		TextOff = 46*SCREEN.SRES;
	} else {
		TextOff = 0;
	}

#ifdef PRINT_FPS
	old_t = TimGetTicks();
#endif

	if (firsttime) {
		if (device.DIACapable) {
			// This is large enough to cover the entire screen on an iQue
			// When the screen copying is down down further, it copies different amounts depending
			// on whether this is the normal map or the waypoint sector screen
			winH = WinCreateOffscreenWindow(SCREEN.WIDTH, SCREEN.HEIGHT+TextOff+40, nativeFormat, &errW);
		} else {
			winH = WinCreateOffscreenWindow(SCREEN.WIDTH, SCREEN.HEIGHT, nativeFormat, &errW);
		}
		DrawGlider(data.input.curmaporient, SCREEN.GLIDERX, SCREEN.GLIDERY);

		switch (data.config.lftunits) {
			case STATUTE:
				// fpm
				MCMult = 100;
				break;
			case NAUTICAL:
				// kts
				MCMult = 1;
				break;
			case METRIC:
				// m/s
				MCMult = 0.5;
				break;
			default:
				break;
		}
		firsttime = false;
	}

	/* Set Drawing Window to Offscreen Window */
	activeWinH = WinSetDrawWindow(winH);
	WinEraseWindow();

	if (scale != mapscale && mapscale > 0.0) {
		scale = mapscale;
		/* Find Range to both corners of the screen */

//		HostTraceOutputTL(appErrorClass, "scale =|%s|", DblToStr(scale, 2));
//		HostTraceOutputTL(appErrorClass, "X     =|%s|", DblToStr(SCREEN.GLIDERX, 2));
//		HostTraceOutputTL(appErrorClass, "Y     =|%s|", DblToStr(SCREEN.GLIDERY, 2));

		xdist   = PixelToDist(scale, (double)SCREEN.GLIDERX);
		ydistul = PixelToDist(scale, (double)SCREEN.GLIDERY);
		ydistll = PixelToDist(scale, (double)(SCREEN.HEIGHT - SCREEN.GLIDERY));

//		HostTraceOutputTL(appErrorClass, "X  dis=|%s|", DblToStr(xdist, 2));
//		HostTraceOutputTL(appErrorClass, "YU dis=|%s|", DblToStr(ydistul, 2));
//		HostTraceOutputTL(appErrorClass, "YL dis=|%s|", DblToStr(ydistll, 2));

		ulRng = XYDistToRng(xdist, ydistul);
		llRng = XYDistToRng(xdist, ydistll);

//		HostTraceOutputTL(appErrorClass, "ulRng =|%s|", DblToStr(ulRng, 2));
//		HostTraceOutputTL(appErrorClass, "llRng =|%s|", DblToStr(llRng, 2));

		xratio = (double)SCREEN.GLIDERX / (scale * ((double)SCREEN.GLIDERX / (double)SCREEN.WIDTH));
		yratio = (double)SCREEN.GLIDERY / (scale * ((double)SCREEN.GLIDERY / (double)SCREEN.HEIGHT));

		// used to avoid Int16 overflow in calcplotvalues
		if (xratio > yratio) {
			bigratio = xratio;
		} else {
			bigratio = yratio;
		}
	}

	// force orientation to North Up for flight log display
	// and set task data to task in flight log
	if (draw_log) {
		if (!draw_task) {
			data.input.curmaporient = NORTHUP;
			// get flight record
			AllocMem((void *)&fltdata, sizeof(FlightData));
			OpenDBQueryRecord(flight_db, selectedFltindex, &output_hand, &output_ptr);
			MemMove(fltdata,output_ptr,sizeof(FlightData));
			MemHandleUnlock(output_hand);
			tsk = &fltdata->flttask;
		} else if (!taskpreview) {
			data.input.curmaporient = NORTHUP;
		} else if (data.input.curmaporient == COURSEUP) data.input.curmaporient = TRACKUP;
	}

	// find true course
	if (data.input.curmaporient == TRACKUP) {
		truecse = data.input.true_track.value;
	} else if ((data.input.curmaporient == COURSEUP) && (data.input.bearing_to_destination.valid==VALID)) {
		truecse = nice_brg(data.input.bearing_to_destination.value - data.input.deviation.value);
	} else {
		truecse = nice_brg(-data.input.deviation.value);
	}

	// glider position towards bottom of screen
	centerglider = false;
	SCREEN.GLIDERY = 100*SCREEN.SRES;

	// select to center glider on screen or not
	if (ismap && !draw_log && ((mapmode == THERMAL) || (data.input.curmaporient != TRACKUP))) {
//	if (ismap && !draw_log && (mapmode == THERMAL)) {
		// move glider to center of screen
		if (!device.DIACapable || data.config.btmlabels) {
			SCREEN.GLIDERY = (device.DIACapable?80:(70+(data.config.btmlabels?0:10)))*SCREEN.SRES;
			centerglider = true;
		}
	}

	// initialise max/min lat/lon
	mapmaxlat = -90.0;
	mapminlat = 90.0;
	mapmaxlon = -180.0;
	mapminlon = 180.0;

	// find lat/lon of corners of screen
	poirange = PixelToDist(scale, SCREEN.TOPCORNER * SCREEN.SRES);
	poibearing = SCREEN.TOPANGLE;
	// top right
	RangeBearingToLatLonLinearInterp(gliderLat, gliderLon, poirange, nice_brg(truecse+poibearing), &pointlat, &pointlon);
	if (pointlat > mapmaxlat) mapmaxlat = pointlat;	
	if (pointlat < mapminlat) mapminlat = pointlat;	
	if (pointlon > mapmaxlon) mapmaxlon = pointlon;	
	if (pointlon < mapminlon) mapminlon = pointlon;	
	// top left
	RangeBearingToLatLonLinearInterp(gliderLat, gliderLon, poirange, nice_brg(truecse-poibearing), &pointlat, &pointlon);
	if (pointlat > mapmaxlat) mapmaxlat = pointlat;	
	if (pointlat < mapminlat) mapminlat = pointlat;	
	if (pointlon > mapmaxlon) mapmaxlon = pointlon;	
	if (pointlon < mapminlon) mapminlon = pointlon;	
	// check for bottom labels and DIA capable Palm
	if (!draw_log && ismap && data.config.btmlabels && !centerglider) {
		poirange = PixelToDist(scale, SCREEN.BOTTOMCORNERLBL * SCREEN.SRES);
		poibearing = SCREEN.BOTTOMANGLELBL;
	} else {
		poirange = PixelToDist(scale, SCREEN.BOTTOMCORNER * SCREEN.SRES);
		poibearing = SCREEN.BOTTOMANGLE;
	}
	// bottom right
	RangeBearingToLatLonLinearInterp(gliderLat, gliderLon, poirange, nice_brg(truecse+poibearing), &pointlat, &pointlon);
	if (pointlat > mapmaxlat) mapmaxlat = pointlat;	
	if (pointlat < mapminlat) mapminlat = pointlat;	
	if (pointlon > mapmaxlon) mapmaxlon = pointlon;	
	if (pointlon < mapminlon) mapminlon = pointlon;	
	// bottom left
	RangeBearingToLatLonLinearInterp(gliderLat, gliderLon, poirange, nice_brg(truecse-poibearing), &pointlat, &pointlon);
	if (pointlat > mapmaxlat) mapmaxlat = pointlat;	
	if (pointlat < mapminlat) mapminlat = pointlat;	
	if (pointlon > mapmaxlon) mapmaxlon = pointlon;	
	if (pointlon < mapminlon) mapminlon = pointlon;	

//	HostTraceOutputTL(appErrorClass, "Max Lat %s", DblToStr(mapmaxlat,2));
//	HostTraceOutputTL(appErrorClass, "Min Lat %s", DblToStr(mapminlat,2));
//	HostTraceOutputTL(appErrorClass, "Max Lon %s", DblToStr(mapmaxlon,2));
//	HostTraceOutputTL(appErrorClass, "Min Lon %s", DblToStr(mapminlon,2));

	// Draw the two range circles if active
	if (!draw_log && ismap && (data.config.mapcircrad1 > 0.0)) {
		// Draw the first map range circle
		WinDrawCircle(SCREEN.GLIDERX, SCREEN.GLIDERY, (Int32)(data.config.mapcircrad1*xratio), DASHED);
	}

	if (!draw_log && ismap && (data.config.mapcircrad2 > 0.0)) {
		// Draw the second map range circle
		WinDrawCircle(SCREEN.GLIDERX, SCREEN.GLIDERY, (Int32)(data.config.mapcircrad2*xratio), DASHED);
	}

	// Draw Terrain Coverage Box 
	firstpoint = true;
	if (data.input.showterbox) {
		for (i=0; i<5; i++) {
			if (i == 4 ) {
				// Gotta do this to draw the last line back to the
				// original point
				GetTerrainBounds(&pointlat, &pointlon, 0);
			} else {
				GetTerrainBounds(&pointlat, &pointlon, i);
			}
			plotgood = CalcPlotValues(gliderLat, gliderLon, pointlat, pointlon, xratio, yratio,
					&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);

			// lines
			if (firstpoint) {
				firstpoint = false;
			} else if (plotgood && prevplotgood) {
				WinDrawClippedLine(prevplotX, prevplotY, plotX, plotY, SOLID);
			}
			prevplotX = plotX;
			prevplotY = plotY;
			prevplotgood = plotgood;
		}
	}

	// Draw SUA Data
	if (data.config.suaactivetypes && SUAdrawclasses && ((ismap && ((mapmode == CRUISE) || data.config.keepSUA)) || (!ismap && data.config.keepSUA))) {
		DrawSUA(gliderLat, gliderLon, xratio, yratio, ulRng); 
	}

	// Draw Waypoints
	if (data.config.wayonoff && ((mapmode == CRUISE) || (device.romVersion >= SYS_VER_50))) {
		draw_waypoints(gliderLat, gliderLon, mapscale, maxdist, gliderpos);
	}

	// Draw max and min points for area waypoints
	if (!ismap && (tsk->waypttypes[selectedTaskWayIdx] & AREA)) {

		// plot min point
//		CalcAreaMin(tsk, selectedTaskWayIdx, &minlat, &minlon, true);
		plotgood = CalcPlotValues(gliderLat, gliderLon, data.input.areaminlat, data.input.areaminlon, xratio, yratio,
					&plotX, &plotY, &poirange, &poibearing, NOFORCE, data.input.curmaporient, false);
		if (plotgood) DrawPOI(plotX, plotY, "Min", (SCREEN.SRES*3), 3, false, false, TURN, false, mapscale, false, false, false);

		// plot max point
//		CalcAreaMax(tsk, selectedTaskWayIdx, &maxlat, &maxlon, true);
		plotgood = CalcPlotValues(gliderLat, gliderLon, data.input.areamaxlat, data.input.areamaxlon, xratio, yratio,
					&plotX, &plotY, &poirange, &poibearing, NOFORCE, data.input.curmaporient, false);
		if (plotgood) DrawPOI(plotX, plotY, "Max", (SCREEN.SRES*3), 3, false, false, TURN, false, mapscale, false, false, false);
	}

	// Draw the track trail
	if (!draw_log && ismap && (data.config.trktrail)) {
		DrawTrkTrail(gliderLat, gliderLon, xratio, yratio, ulRng); 
	}

	// Draw Final Glide Over Terrain info
	if (!draw_log && ismap) {
		// plot terrain crashes
		// predicted landing point
		if (crash1) {
			// predicted (crash) landing point
			plotgood = CalcPlotValues(gliderLat, gliderLon, crashlat1, crashlon1, xratio, yratio,
							&WfgplotX, &WfgplotY, &poirange, &poibearing, NOFORCE, data.input.curmaporient, false);
			if (plotgood) {
//				HostTraceOutputTL(appErrorClass, "Plotting 1st Terrain Crash");
				StrCopy(fgotchar, " ");
				DrawPOI(WfgplotX, WfgplotY, fgotchar, (SCREEN.SRES*3), 0, false, true, CRASH, true, mapscale, false, false, false);
			}
		}
		// inusewaypt
		if (wptcrash) {
//			HostTraceOutputTL(appErrorClass, "Plot Inusewaypt Terrain Crash");
			// worst crash point
			plotgood = CalcPlotValues(gliderLat, gliderLon, wptcrashlat, wptcrashlon, xratio, yratio,
							&WfgplotX, &WfgplotY, &poirange, &poibearing, NOFORCE, data.input.curmaporient, false);
			if (plotgood) {
//				HostTraceOutputTL(appErrorClass, "Plotting Worst Waypoint Terrain Crash %s", print_altitude(wptcrashalt));
				StrCopy(fgotchar, print_altitude(wptcrashalt));
				DrawPOI(WfgplotX, WfgplotY, fgotchar, (SCREEN.SRES*3), StrLen(fgotchar), false, true, TERRAIN, true, mapscale, false, false, false);
			}
		}
		// task
		if (tskcrash) {
//			HostTraceOutputTL(appErrorClass, "Plot Task Terrain Crash");
			// worst taskcrash point
			plotgood = CalcPlotValues(gliderLat, gliderLon, tskcrashlat, tskcrashlon, xratio, yratio,
							&TfgplotX, &TfgplotY, &poirange, &poibearing, NOFORCE, data.input.curmaporient, false);
			if (plotgood) {
//				HostTraceOutputTL(appErrorClass, "Plotting Worst Task Terrain Crash %s", print_altitude(tskcrashalt));
				StrCopy(fgotchar, print_altitude(tskcrashalt));
				DrawPOI(TfgplotX, TfgplotY, fgotchar, (SCREEN.SRES*3), StrLen(fgotchar), false, true, TERRAIN, true, mapscale, false, false, false);
			}
		}
	}
	
// Draw the active task
	if (data.config.taskonoff || !ismap) {
		if (!draw_task) {
			DrawTask(gliderLat, gliderLon, xratio, yratio, ulRng, maxdist, startalt, tsk, scale, ismap);
		} else {
			DrawTask(gliderLat, gliderLon, xratio, yratio, ulRng, maxdist, startalt, edittsk, scale, ismap);
		}
	}
	
// Draw the waypoint line on moving map screen
	// Placed here so that I can blank the bottom of the screen
	// where the text goes.
	if (!draw_log) {
		if(ismap && (data.input.bearing_to_destination.valid==VALID)) {
			// CalcPlotValues returns bearing in magnetic
			plotgood = CalcPlotValues(gliderLat, gliderLon, data.input.destination_lat, data.input.destination_lon, xratio, yratio,
					&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, true);

			if (!taskonhold && (activetskway >= 0)) {
				// take task waypoint if task active
				plotgood = CalcPlotValues(gliderLat, gliderLon, data.task.targetlats[activetskway], data.task.targetlons[activetskway], xratio, yratio,
						&posX, &posY, &temprng, &tempbrg, FORCEACTUAL, data.input.curmaporient, true);
			} else {
				// inuseWaypoint position
				posX = plotX;
				posY = plotY;
			}
			// This is for determining if the selected waypoint gets drawn in Bold/Green or
			// Regular/Red.
//			HostTraceOutputTL(appErrorClass, "wayline-poirange=|%s|", DblToStr(poirange, 1));
//			HostTraceOutputTL(appErrorClass, "wayline-maxdist=|%s|", DblToStr(maxdist, 1));
			// using firstaltloss here to be used in the FGA calculations later

			CalcSTFSpdAlt2(MCCurVal, poirange, poibearing, &WaySTF, &firstaltloss);
			data.input.destination_aalt = ConvertAltType(data.input.destination_elev, startalt, true, REQALT, firstaltloss);

//			HostTraceOutputTL(appErrorClass, "wayline-WaySTF=|%s|", DblToStr(WaySTF, 1));
//			HostTraceOutputTL(appErrorClass, "wayline-firstaltloss=|%s|", DblToStr(firstaltloss, 1));
//			HostTraceOutputTL(appErrorClass, "wayline-data.input.destination_aalt=|%s|", DblToStr(data.input.destination_aalt, 1));
//			HostTraceOutputTL(appErrorClass, "wayline-startalt=|%s|", DblToStr(startalt, 1));

			// draw destination symbol and name
			if ((data.input.destination_type & AREA) == 0) {
				if (data.input.destination_aalt <= startalt) {
					DrawPOI(posX, posY, data.input.destination_name, (SCREEN.SRES*3), data.config.waymaxlen, true, true, data.input.destination_type, true, mapscale,
						(data.input.destination_type & AIRPORT), (data.input.destination_type & AIRLAND), (data.input.destination_aalt+data.config.safealt > startalt));
				} else {
					DrawPOI(posX, posY, data.input.destination_name, (SCREEN.SRES*3), data.config.waymaxlen, false, true, data.input.destination_type, true, mapscale, false, false, false);
				}
			}

			// If this is just a single selected waypoint, draw a circle around it
			// at the same radius as the circle radius for a task turnpoint
			if ((tsk->numwaypts <= 0) || (taskonhold) || (!data.config.taskonoff)) {

				// set to colour for sectors
				if (device.colorCapable) {
					WinSetForeColor(indexSector);
				}
				// check for bold line
				if (device.HiDensityScrPresent && data.config.BoldSector) {
					WinSetCoordinateSystem(kCoordinatesStandard);
					boldline = SCREEN.SRES;
				}

				//Check the truncating here and do some rounding instead
				WinDrawCircle(plotX/boldline, plotY/boldline, (Int32)(data.config.turncircrad*xratio)/boldline, SOLID);

				// reset colour to black and normal lines
				if (device.colorCapable) {
					WinSetForeColor(indexBlack);
				}
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(device.HiDensityScrPresent);
					boldline = 1.0;
				}
			}

			// Draw the In Use Waypoint direction
			RctSetRectangle (&rectP, WIDTH_MIN, HEIGHT_MIN, (SCREEN.SRES*25), (SCREEN.SRES*14));
			WinEraseRectangle(&rectP, 0);
			FntSetFont(largeBoldFont);
			StrCopy(tempchar, print_direction2(poibearing));
			StrCat(tempchar, "°");
			WinDrawChars(tempchar, StrLen(tempchar), WIDTH_MIN+(SCREEN.SRES*1), HEIGHT_MIN);

			// Draw the In Use Waypoint distance
			RctSetRectangle (&rectP, SCREEN.WIDTH-(SCREEN.SRES*22), HEIGHT_MIN, (SCREEN.SRES*22), (SCREEN.SRES*14));
			WinEraseRectangle(&rectP, 0);
			if ((poirange*data.input.disconst) < 10.0) {
				StrCopy(tempchar, print_distance2(poirange, 2));
			} else if ((poirange*data.input.disconst) < 100.0) {
				StrCopy(tempchar, print_distance2(poirange, 1));
			} else if ((poirange*data.input.disconst) < 1000.0) {
				StrCopy(tempchar, print_distance2(poirange, 0));
			} else {
				StrCopy(tempchar, "999");
			}
			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*21), HEIGHT_MIN);

			// Draw the turn direction arrows
			tmpdbl = data.input.magnetic_track.value - poibearing;
			if (tmpdbl < 0.0) {
				tmpdbl = 360.0 + tmpdbl;
			}
			if (tmpdbl < 180.0) {
				leftturn = true;
			} else {
				leftturn = false;
				tmpdbl = 360.0 - tmpdbl;
			}
			FntSetFont(symbol11Font);
			if (tmpdbl > 10.0) {
				if (leftturn) {
					WinDrawChars("\002\002", 2, WIDTH_MIN+(SCREEN.SRES*1), HEIGHT_MIN+(SCREEN.SRES*13));
				} else {
					WinDrawChars("\003\003", 2, WIDTH_MIN+(SCREEN.SRES*1), HEIGHT_MIN+(SCREEN.SRES*13));
				}
			} else if (tmpdbl > 5.0) {
				if (leftturn) {
					WinDrawChars("\002", 1, WIDTH_MIN+(SCREEN.SRES*5), HEIGHT_MIN+(SCREEN.SRES*13));
				} else {
					WinDrawChars("\003", 1, WIDTH_MIN+(SCREEN.SRES*5), HEIGHT_MIN+(SCREEN.SRES*13));
				}
			}
			FntSetFont(stdFont);
		} else {
			// Draw the manual distance
			poirange = data.input.distance_to_destination.value;
			FntSetFont(largeBoldFont);
			RctSetRectangle (&rectP, SCREEN.WIDTH-(SCREEN.SRES*22), HEIGHT_MIN, (SCREEN.SRES*22), (SCREEN.SRES*14));
			WinEraseRectangle(&rectP, 0);
			if ((poirange*data.input.disconst) < 10.0) {
				StrCopy(tempchar, print_distance2(poirange, 2));
			} else if ((poirange*data.input.disconst) < 100.0) {
				StrCopy(tempchar, print_distance2(poirange, 1));
			} else if ((poirange*data.input.disconst) < 1000.0) {
				StrCopy(tempchar, print_distance2(poirange, 0));
			} else {
				StrCopy(tempchar, "999");
			}
			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*21), HEIGHT_MIN);
			FntSetFont(stdFont);
		}
	}

	// Draw the next waypoint point bearing and distance if valid and meets criteria
	if (!draw_log && ismap && (((data.config.shownextwpt == NEXTALL) || ((data.config.shownextwpt == NEXTCTL) && data.input.isctlpt)) && (data.input.nextwpt_dist >= 0.0))) {
		FntSetFont(stdFont);
		// bearing
		StrCopy(tempchar, print_direction2(data.input.nextwpt_bear));
		StrCat(tempchar, "°");
		WinDrawChars(tempchar, StrLen(tempchar), WIDTH_MIN+(SCREEN.SRES*25), HEIGHT_MIN+(SCREEN.SRES*3));

		// label
		if (StrLen(data.input.nextwpt_text) == 3) { // only show for Max, Min and Ctl
			WinDrawChars(data.input.nextwpt_text, StrLen(data.input.nextwpt_text), SCREEN.WIDTH-(SCREEN.SRES*18), HEIGHT_MIN+(SCREEN.SRES*12));
		}

		// distance
		if ((data.input.nextwpt_dist*data.input.disconst) < 10.0) {
			StrCopy(tempchar, print_distance2(data.input.nextwpt_dist, 2));
		} else if ((data.input.nextwpt_dist*data.input.disconst) < 100.0) {
			StrCopy(tempchar, print_distance2(data.input.nextwpt_dist, 1));
		} else if ((data.input.nextwpt_dist*data.input.disconst) < 1000.0) {
			StrCopy(tempchar, print_distance2(data.input.nextwpt_dist, 0));
		} else {
			StrCopy(tempchar, "999");
		}
		WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*40), HEIGHT_MIN+(SCREEN.SRES*3));

		FntSetFont(largeBoldFont);
	}
	
	// Draw the line to the inuse waypoint and name
	if (!draw_log && (data.config.wayline) && (data.input.bearing_to_destination.valid==VALID)) {
		DrawWayLine(plotX, plotY, gliderLat, gliderLon, xratio, yratio, ismap);
	}

	// Draw the reference point or current waypoint name on map screen
	StrCopy(tempchar,"");
	if (!draw_log && ismap) {
		if ((data.input.refwpttype != REFNONE) && (data.input.refLat != INVALID_LAT_LON) && (data.input.refLon != INVALID_LAT_LON)) {
			// display range and bearing from/to ref point
			LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.input.refLat, data.input.refLon, &refRng, &refBrg);
			refBrg = nice_brg(refBrg + data.input.deviation.value);
			StrCopy(tempchar, "  ");
			if (data.config.RefWptRadial) {
				refBrg = RecipCse(refBrg);
				StrCat(tempchar, print_direction2(refBrg));
				StrCat(tempchar, "R");
			} else {
				StrCat(tempchar, print_direction2(refBrg));
				StrCat(tempchar, "°");
			}
			if (pround(refRng*data.input.disconst,0) > 999.0) {
				StrCat(tempchar, "999");
			} else {
				StrCat(tempchar, print_distance2(refRng,0));
			}
			StrCopy(tempchar, Right(tempchar,7));
		} else {
			// display current waypoint name
			if ((StrLen(data.input.destination_name) == 0) || (StrCompare(data.input.destination_name, ",") == 0)) {
				StrCopy(tempchar, "Not Selected");
			} else {
				StrCopy(tempchar, " ");
				StrCat(tempchar, data.input.destination_name);
				StrCat(tempchar, " ");
			}
		}
	}
	FntSetFont(stdFont);
	if (StrLen(tempchar) > 0) {
		if (!device.DIACapable) {
			if (data.config.btmlabels) {
	 			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH/2-FntCharsWidth(tempchar,StrLen(tempchar))/2, SCREEN.HEIGHT-(SCREEN.SRES*32));
			} else {
	 			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH/2-FntCharsWidth(tempchar,StrLen(tempchar))/2, SCREEN.HEIGHT-(SCREEN.SRES*10));
			}
		} else {
			if (data.config.btmlabels) {
	 			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH/2-FntCharsWidth(tempchar,StrLen(tempchar))/2, SCREEN.HEIGHT-(SCREEN.SRES*10));
			} else {
	 			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH/2-FntCharsWidth(tempchar,StrLen(tempchar))/2, SCREEN.HEIGHT+(SCREEN.SRES*36));
			}
		}
	}
	FntSetFont(largeBoldFont);

	// Draw Wind Data 	
	if (!draw_log && data.config.windarrow && ismap) {
		DrawWind(SCREEN.GLIDERX, SCREEN.GLIDERY, xratio, yratio, scale); 
	}

	// Draw thermal strength with height profile
	if (!draw_log && data.config.thermalprofile && ismap && (mapmode == THERMAL)) {
		DrawThermalProfile();
	}

	// if required show time to go to start opens, TOT info or temp waypoint
	if (StrLen(timeinfo) > 0) {
		StrCopy(timeinfo, trim(timeinfo, ' ', true));
		WinDrawChars(timeinfo, StrLen(timeinfo), SCREEN.SRES*80-FntCharsWidth(timeinfo,StrLen(timeinfo))/2, 0);
	}

/* Draw Bottom Labels */
	// poirange and poibear must not change from above
	if (!draw_log && ismap && (data.config.btmlabels)) {
		// If drawing the bottom labels or the FG labels
		// clears the area where the text goes
		if (device.DIACapable) {
			RctSetRectangle (&rectP, WIDTH_MIN+(SCREEN.SRES), SCREEN.HEIGHT-(SCREEN.SRES*45)+TextOff,SCREEN.WIDTH, SCREEN.SRES*45);
		} else {
			RctSetRectangle (&rectP, WIDTH_MIN+(SCREEN.SRES), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff,SCREEN.WIDTH, SCREEN.SRES*22);
		}
		WinEraseRectangle(&rectP, 0);

		// Clear the area for the MC (cruise) or TLft (thermal) value 
		if ((mapmode == THERMAL) && !device.DIACapable && (data.config.mapRHfield != MAPTLFT)) {
			tmpdbl = pround(data.input.thavglift*data.input.lftconst, data.input.lftprec+1);
		} else {
			tmpdbl = pround(data.input.basemc*data.input.lftconst, data.input.lftprec+1);
		}
		StrCopy(tempchar, DblToStr(tmpdbl, data.input.lftprec));
		fieldadj = 19;
		if (StrLen(tempchar) > 3) {
			fieldadj = fieldadj + 6;
		}
		if (!device.DIACapable) {
			RctSetRectangle (&rectP, WIDTH_MIN, SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff, (SCREEN.SRES*fieldadj), (SCREEN.SRES*22));
			WinEraseRectangle(&rectP, 0);
		}
		if ((mapmode == THERMAL) && !device.DIACapable && (data.config.mapRHfield != MAPTLFT)) {
			StrCopy(text, "Tlft");
		} else {
			StrCopy(text, "MC: ");
		}
		FntSetFont(largeBoldFont);
		WinDrawChars(tempchar, StrLen(tempchar), WIDTH_MIN+(SCREEN.SRES*1), SCREEN.HEIGHT-(SCREEN.SRES*35)+TextOff);
		FntSetFont(stdFont);
		WinDrawChars(text, StrLen(text), WIDTH_MIN+(SCREEN.SRES*1), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff);

		// calculate STF and arrival attide
		if (data.input.destination_valid) {
			CalcSTFSpdAlt2(MCCurVal, data.inuseWaypoint.distance, data.inuseWaypoint.bearing, &WaySTF, &tmpalt);
		} else {
			CalcSTFSpdAlt2(MCCurVal, data.input.distance_to_destination.value, data.input.magnetic_track.value, &WaySTF, &firstaltloss);
			data.inuseWaypoint.elevation = 0.0;
			data.input.destination_aalt = ConvertAltType(data.inuseWaypoint.elevation, startalt, true, REQALT, firstaltloss);;
		}

		// speed to fly or required ground speed value
		FntSetFont(stdFont);
		if (data.config.showrgs) {
			StrCopy(text, "RGS");
			WaySTF = WaySTF - calcstfhwind;
		} else {
			StrCopy(text, "STF");
		}
		WinDrawChars(text, StrLen(text), WIDTH_MIN+(SCREEN.SRES*46), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);
		FntSetFont(largeBoldFont);
		StrCopy(text, print_horizontal_speed2(WaySTF, 0));
		WinDrawChars(text, StrLen(text), WIDTH_MIN+(SCREEN.SRES*46), SCREEN.HEIGHT-(SCREEN.SRES*13)+TextOff);
		FntSetFont(stdFont);

		// Veritcal Line to the left of the STF value
		WinDrawLine(WIDTH_MIN+(SCREEN.SRES*44), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff,
				WIDTH_MIN+(SCREEN.SRES*44), SCREEN.HEIGHT+TextOff);
		// Draw Vertical Line to the left of the Altitude
		WinDrawLine(WIDTH_MIN+(SCREEN.SRES*65), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff,
				WIDTH_MIN+(SCREEN.SRES*65), SCREEN.HEIGHT+TextOff);
		if (device.DIACapable) {
			// Veritcal Line to the right of the MC value
			WinDrawLine(WIDTH_MIN+(SCREEN.SRES*23), SCREEN.HEIGHT-(SCREEN.SRES*46)+TextOff,
					WIDTH_MIN+(SCREEN.SRES*23), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);
			// Veritcal Line to the left of the FGA value
			WinDrawLine(WIDTH_MIN+(SCREEN.SRES*44), SCREEN.HEIGHT-(SCREEN.SRES*46)+TextOff,
					WIDTH_MIN+(SCREEN.SRES*44), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);
			// Setting the placment values for the above/below glideslope arrows
			plotX = WIDTH_MIN+(SCREEN.SRES*24);
			plotY = SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff;
		} else {
			// Setting the placment values for the above/below glideslope arrows
			plotX = SCREEN.WIDTH-(SCREEN.SRES*20);
			plotY = SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff;
		}

		// Clear the area for the D.Alt arrows or average LD or FGA
		if (!device.DIACapable) {
			RctSetRectangle (&rectP, SCREEN.WIDTH-(SCREEN.SRES*20), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff, (SCREEN.SRES*20), (SCREEN.SRES*22));
			WinEraseRectangle(&rectP, 0);
		}

		// Draw the average LD
		FntSetFont(largeBoldFont);
		if (device.DIACapable || (data.config.mapRHfield == MAPGLIDE)) {
			if ((data.input.distdone != 0.0) && (data.input.avglift != 0.0)) {
				ld = -data.input.distdone*2*60 / data.input.avglift; // assuming 30 sec average
				lftprec = 0;
				if (ld < -99) ld = -99;
				if (ld > 199) ld = 199;
				if ((ld > 0) && (ld < 10)) lftprec = 1;
				StrCopy(tempchar, DblToStr(ld, lftprec));
				WinDrawChars(tempchar, StrLen(tempchar), plotX+(SCREEN.SRES*2), plotY+(SCREEN.SRES*9));
			} else {
				WinDrawChars("XX", 2, plotX+(SCREEN.SRES*2), plotY+(SCREEN.SRES*9));
			}
		}
		
		// Draw the D.ALT Arrows
		FntSetFont(symbolFont);
		if (device.DIACapable || (data.config.mapRHfield == MAPGLIDE)) {
			tmpdbl = ConvertAltType(data.inuseWaypoint.elevation, startalt, true, ARVALT, firstaltloss);
			if (tmpdbl > (2.0*data.config.safealt)) {
				WinDrawChars("\005\005", 2, plotX, plotY);
			} else if (tmpdbl >= data.config.safealt && tmpdbl <= (2.0*data.config.safealt)) {
				WinDrawChars("\005", 1, plotX, plotY);
			} else if (tmpdbl < data.config.safealt && tmpdbl >= 0.0) {
				WinDrawChars("\006", 1, plotX, plotY);
			} else if (tmpdbl < 0.0) {
				WinDrawChars("\006\006", 2, plotX, plotY);
			}
		}

		
		// Have to save this to fgatpalt1 since data.input.destination_aalt could change below
		fgatpalt1 = data.input.destination_aalt;

		// Only need to call this if in mode other than REQALT since it is calculated above.
		if (data.config.alttype != REQALT) {
			data.input.destination_aalt = ConvertAltType(data.inuseWaypoint.elevation, startalt, true, data.config.alttype, firstaltloss);
		}

		// convert the displayed altitudes taking into account the terrain clearance
		tmpalt = data.input.destination_aalt;
//		HostTraceOutputTL(appErrorClass, "In Use Alt %s", DblToStr(tmpalt*ALTMETCONST,0));
//		HostTraceOutputTL(appErrorClass, "Wpt Crash Alt %s", DblToStr(wptcrashalt*ALTMETCONST,0));
		switch (data.config.alttype) {
			case REQALT:
				StrCopy(text, "RAlt");
				if (wptcrash && !offterrain) {
					tmpalt = startalt - wptcrashalt;
				}
				break;
			case ARVALT:
				StrCopy(text, "AAlt");
				if (wptcrash && !offterrain) {
					// if crash is more than safety height, adjust AAlt figure
					if (wptcrashalt + data.config.safealt < 0) {
						tmpalt = wptcrashalt + data.config.safealt;
					}
				}
				break;
			case DLTALT:
				StrCopy(text, "DAlt");
				if (wptcrash && !offterrain) {
					tmpalt = wptcrashalt;
				}
				break;
			default:
				break;
		}

		// display the altitude
		FntSetFont(stdFont);
		WinDrawChars(text, StrLen(text), WIDTH_MIN+(SCREEN.SRES*67), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);
		FntSetFont(largeBoldFont);

		if (tmpalt >= 99999.5) {
			StrCopy(text, "99999");
		} else if (tmpalt <= -99999.5) {
			StrCopy(text, "-99999");
		} else {
			StrCopy(text, print_altitude(tmpalt));
		}
		WinDrawChars(text, StrLen(text), WIDTH_MIN+(SCREEN.SRES*67), SCREEN.HEIGHT-(SCREEN.SRES*13)+TextOff);
		FntSetFont(stdFont);

		if (data.input.destination_valid) {

			if (device.DIACapable || (data.config.mapRHfield == MAPFGA)) {
				// Find the STF altitude from current waypoint to:
				//      The Finish Point if a task is active or
				//      The HOME waypoint if no task is active
				if (tsk->numwaypts > 0) {
					// get FGA altitude
					fgatpalt2 = data.input.FGAdispalt;
				} else {
//					HostTraceOutputTL(appErrorClass, "Inside GOTO Waypoint FGA Case");
					// Setting fgatpalt2 to fgatpalt1 to cover the case where
					// the current waypoint/turnpoint is the HOME/Finish waypoint/turnpoint
					tskcrash = false;
			 		if (data.config.alttype == REQALT) {
						fgatpalt2 = fgatpalt1;
					} else {
						fgatpalt2 = ConvertAltType(data.input.destination_elev, startalt, true, data.config.alttype, firstaltloss);
					}

					if (data.input.destination_lat != data.input.homeLat || data.input.destination_lon != data.input.homeLon) {
						LatLonToRangeBearing(data.input.destination_lat, data.input.destination_lon, data.input.homeLat, data.input.homeLon, &fgatpdist, &fgatpbear);

						// Convert to Magnetic
						fgatpbear = nice_brg(fgatpbear + data.input.deviation.value);

						// CalcSTFSpdAlt expects the first parameter to be in knots
						CalcSTFSpdAlt2(MCCurVal, fgatpdist, fgatpbear, &WaySTF, &secondaltloss);
						fgatpalt2 = ConvertAltType(data.input.homeElev, startalt, true, REQALT, (firstaltloss+secondaltloss));
						if (data.config.alttype != REQALT) {
							fgatpalt2 = ConvertAltType(data.input.homeElev, startalt, true, data.config.alttype,  (firstaltloss+secondaltloss));
						}
					}
				}
				tmpalt = fgatpalt2;
				// display the result
				if (tmpalt >= 99999.5) {
					StrCopy(text, "99999");
				} else if (tmpalt <= -99999.5) {
					StrCopy(text, "-99999");
				} else {
					StrCopy(text, print_altitude(tmpalt));
				}
				FntSetFont(stdFont);
				if (device.DIACapable) {
					WinDrawChars("FGA:", 4, WIDTH_MIN+(SCREEN.SRES*46), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff);
					FntSetFont(largeBoldFont);
					WinDrawChars(text, StrLen(text), WIDTH_MIN+(SCREEN.SRES*46), SCREEN.HEIGHT-(SCREEN.SRES*35)+TextOff);
					// Draw Vertical Line to the left of the Altitude
					// Drawn here to fix problem where the line isn't getting drawn all the way to
					// the top because the FGA value is masking it off.
					WinDrawLine(WIDTH_MIN+(SCREEN.SRES*65), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff,
							WIDTH_MIN+(SCREEN.SRES*65), SCREEN.HEIGHT+TextOff);
				} else if (data.config.mapRHfield == MAPFGA) {
					WinDrawChars("FGA:", 4, SCREEN.WIDTH-(SCREEN.SRES*17), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff);
					FntSetFont(largeBoldFont);
					WinDrawChars(text, StrLen(text), plotX+(SCREEN.SRES*(15-StrLen(text)*5)), plotY+(SCREEN.SRES*9));
				}
			}
		}

		// display course and speed
		FntSetFont(stdFont);
		StrCopy(tempchar, "Cse");
		WinDrawChars(tempchar, StrLen(tempchar), WIDTH_MIN, SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);
		StrCopy(tempchar, "Spd");
		WinDrawChars(tempchar, StrLen(tempchar), WIDTH_MIN+(SCREEN.SRES*25), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);

		FntSetFont(largeBoldFont);
		StrCopy(tempchar, print_direction(data.input.magnetic_track));
//		StrCat(tempchar, "°");
		WinDrawChars(tempchar, StrLen(tempchar), WIDTH_MIN, SCREEN.HEIGHT-(SCREEN.SRES*13)+TextOff);
		StrCopy(tempchar, print_horizontal_speed(data.input.ground_speed));

		WinDrawChars(tempchar, StrLen(tempchar), WIDTH_MIN+(SCREEN.SRES*25), SCREEN.HEIGHT-(SCREEN.SRES*13)+TextOff);
		if (data.config.altreftype == AGL) {
			if (terrainvalid) {
				StrCopy(tempchar, print_altitude(data.input.inusealt-data.input.terelev));
			} else {
				StrCopy(tempchar, "N/A");
			}
		} else if (data.config.altreftype == QFE) {
			StrCopy(tempchar, print_altitude(data.input.inusealt-data.config.qfealt));
		} else if (data.config.altreftype == PALT) {
			// FL always in ft
			StrCopy(tempchar, DblToStr(pround(data.logger.pressalt,0),0));
		} else {
			StrCopy(tempchar, print_altitude(data.input.inusealt));
		}

		WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*55), SCREEN.HEIGHT-(SCREEN.SRES*13)+TextOff);

		// Placed this here to make the Q plot correctly
		FntSetFont(stdFont);
		switch ( data.config.altreftype ) {
			case MSL:
				StrCopy(tempchar, "MSL:");
				StrCat(tempchar,data.input.alttext);
				break;
			case QFE:
				StrCopy(tempchar, "QFE:");
				StrCat(tempchar,data.input.alttext);
				break;
			case AGL:
				StrCopy(tempchar, "AGL:");
				StrCat(tempchar,data.input.alttext);
				break;
			case PALT:
				// FL always in ft
				StrCopy(tempchar, " FL:ft");
				break;				
			default:
				break;
		}
		WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*55), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);

		FntSetFont(largeBoldFont);
		if (device.DIACapable) {
			// This is for the Current Lift Value
			lftprec = data.input.lftprec;
			if ((data.input.lftprec == 2) && (data.input.curlift < 0.0)) lftprec = 1;
			StrCopy(tempchar, DblToStr(pround(data.input.curlift*data.input.lftconst, lftprec), lftprec));
			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*78), SCREEN.HEIGHT-(SCREEN.SRES*35)+TextOff);

			// This is for the Average Lift Value
			StrCopy(tempchar, DblToStr(pround(data.input.avglift*data.input.lftconst, data.input.lftprec), data.input.lftprec));
			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*55), SCREEN.HEIGHT-(SCREEN.SRES*35)+TextOff);

			// This is for the Thermal Average Lift Value
			lftprec = data.input.lftprec;
			if ((data.input.lftprec == 2) && (data.input.thavglift < 0)) lftprec = 1;
			StrCopy(tempchar, DblToStr(pround(data.input.thavglift*data.input.lftconst, lftprec), lftprec));
			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*23), SCREEN.HEIGHT-(SCREEN.SRES*35)+TextOff);

			FntSetFont(stdFont);
			StrCopy(tempchar, "Lft");
			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*78), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff);
			StrCopy(tempchar, "Avg");
			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*55), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff);
			StrCopy(tempchar, "Tlft");
			WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*23), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff);

			// Veritcal Line to the left of the Lft value
			WinDrawLine(SCREEN.WIDTH-(SCREEN.SRES*80), SCREEN.HEIGHT-(SCREEN.SRES*46)+TextOff, 
							SCREEN.WIDTH-(SCREEN.SRES*80), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);
			// Veritcal Line to the left of the AVG value
			WinDrawLine(SCREEN.WIDTH-(SCREEN.SRES*57), SCREEN.HEIGHT-(SCREEN.SRES*46)+TextOff, 
							SCREEN.WIDTH-(SCREEN.SRES*57), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);
			// Veritcal Line to the left of the Tlft value
			WinDrawLine(SCREEN.WIDTH-(SCREEN.SRES*25), SCREEN.HEIGHT-(SCREEN.SRES*46)+TextOff, 
							SCREEN.WIDTH-(SCREEN.SRES*25), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);
		} else if (data.config.mapRHfield == MAPLFT) {
			// This is for the Current Lift Value
			lftprec = data.input.lftprec;
			if ((data.input.lftprec == 2) && (data.input.curlift < 0.0)) lftprec = 1;
			StrCopy(tempchar, DblToStr(pround(data.input.curlift*data.input.lftconst, lftprec), lftprec));
			WinDrawChars(tempchar, StrLen(tempchar), plotX+(SCREEN.SRES*(15-StrLen(text)*5)), plotY+(SCREEN.SRES*9));
			FntSetFont(stdFont);
			WinDrawChars("Lft:", 4, SCREEN.WIDTH-(SCREEN.SRES*17), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff);
		} else if (data.config.mapRHfield == MAPAVG) {
			// This is for the Average Lift Value
			StrCopy(tempchar, DblToStr(pround(data.input.avglift*data.input.lftconst, data.input.lftprec), data.input.lftprec));
			WinDrawChars(tempchar, StrLen(tempchar), plotX+(SCREEN.SRES*(15-StrLen(text)*5)), plotY+(SCREEN.SRES*9));
			FntSetFont(stdFont);
			WinDrawChars("Avg:", 4, SCREEN.WIDTH-(SCREEN.SRES*17), SCREEN.HEIGHT-(SCREEN.SRES*45)+TextOff);
		} else if (data.config.mapRHfield == MAPTLFT) {
 			// This is for the Thermal Average Lift Value
			lftprec = data.input.lftprec;
			if ((data.input.lftprec == 2) && (data.input.thavglift < 0)) lftprec = 1;
			StrCopy(tempchar, DblToStr(pround(data.input.thavglift*data.input.lftconst, lftprec), lftprec));
			WinDrawChars(tempchar, StrLen(tempchar), plotX+(SCREEN.SRES*(20-StrLen(text)*5)), plotY+(SCREEN.SRES*9));
			FntSetFont(stdFont);
			WinDrawChars("Tlft:", 5, SCREEN.WIDTH-(SCREEN.SRES*18), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff);
		}
		FntSetFont(stdFont);

		// show warning triangles for terrain crash or if off terrain map
		if (wptcrash || (offterrain && data.config.usefgterrain)) {
			FntSetFont((FontID)WAYSYMB11);
			if (device.colorCapable) {
				WinSetTextColor(indexRed);
			}

			if (offterrain) {
				tempchar[0] = 11; // fell off terrain map!
			} else {
				tempchar[0] = 9;
			}
			tempchar[1] = '\0';
			// in altitude box
			WinDrawChars(tempchar, 1, WIDTH_MIN+(SCREEN.SRES*89), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);	

			if (device.colorCapable) {
				WinSetTextColor(indexBlack);
			}
			FntSetFont(stdFont);
		}

		if (tskcrash || (tskoffterrain && data.config.usefgterrain)) {
			FntSetFont((FontID)WAYSYMB11);
			if (device.colorCapable) {
				WinSetTextColor(indexRed);
			}

			if (tskoffterrain) {
				tempchar[0] = 11; // fell off terrain map!
			} else {
				tempchar[0] = 9;
			}
			tempchar[1] = '\0';
			// in FGA box
			if (device.DIACapable) {
				WinDrawChars(tempchar, 1, WIDTH_MIN+(SCREEN.SRES*66), SCREEN.HEIGHT-(SCREEN.SRES*44)+TextOff);				
			}

			if (device.colorCapable) {
				WinSetTextColor(indexBlack);
			}
			FntSetFont(stdFont);
		}

		// Line for the Speed Value
		WinDrawLine(WIDTH_MIN+(SCREEN.SRES*23), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff, 
						WIDTH_MIN+(SCREEN.SRES*23), SCREEN.HEIGHT+TextOff);

		// Vertical Line to the right of the Altitude Value
		WinDrawLine(SCREEN.WIDTH-(SCREEN.SRES*57), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff, 
						SCREEN.WIDTH-(SCREEN.SRES*57), SCREEN.HEIGHT+TextOff);

		// Drawing a line above the text
		WinDrawLine(WIDTH_MIN, SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff, 
						SCREEN.WIDTH, SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff);

		// Drawing a line below the text
		WinDrawLine(WIDTH_MIN, SCREEN.HEIGHT+TextOff, 
						SCREEN.WIDTH, SCREEN.HEIGHT+TextOff);

	} else {
		// clear rectangle for Scale box only
		RctSetRectangle (&rectP, SCREEN.WIDTH-(SCREEN.SRES*25), SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff, SCREEN.SRES*25, SCREEN.SRES*22);
		WinEraseRectangle(&rectP, 0);
	}

	// draw MC on waypoint sector view
	if (!draw_log && !ismap) {
		tmpdbl = pround(data.input.basemc*data.input.lftconst, data.input.lftprec+1);
		StrCopy(tempchar, DblToStr(tmpdbl, data.input.lftprec));
		fieldadj = 19;
		if (StrLen(tempchar) > 3) {
			fieldadj = fieldadj + 6;
		}
		RctSetRectangle (&rectP, WIDTH_MIN, SCREEN.HEIGHT-(SCREEN.SRES*44)+(SCREEN.SRES*22), (SCREEN.SRES*fieldadj), (SCREEN.SRES*22));
		WinEraseRectangle(&rectP, 0);
		FntSetFont(largeBoldFont);	
		WinDrawChars(tempchar, StrLen(tempchar), WIDTH_MIN+(SCREEN.SRES*1), SCREEN.HEIGHT-(SCREEN.SRES*35)+(SCREEN.SRES*22));
		FntSetFont(stdFont);
	}

	// Build the map orientation character to add to the map scale
	if (data.input.curmaporient == TRACKUP) {
		StrCopy(text, "T");
	} else if (data.input.curmaporient == COURSEUP) {
		StrCopy(text, "C");
	} else {
		StrCopy(text, "N");
	}

	// adjust scale position for moving map or waypoint sector
	if (ismap) {
//		TOff = TextOff;
		if (device.DIACapable && !draw_log) {
			TOff = 46*SCREEN.SRES;
		} else {
			TOff = 0;
		}
	} else {
		TOff = 0;
	}

	// Need to Always Draw the Scale 
	FntSetFont(stdFont);
	StrCopy(tempchar, "Scale");
	WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*23), SCREEN.HEIGHT-(SCREEN.SRES*22)+TOff);
	FntSetFont(largeBoldFont);
	tmpdbl = pround(mapscale * data.input.disconst, 0);

	if (pround(mapscale * data.input.disconst, 1) < 1.0) {
		StrCopy(tempchar, DblToStr(pround(mapscale * data.input.disconst, 1), 1));
		StrCat(tempchar, text);
		WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*22), SCREEN.HEIGHT-(SCREEN.SRES*13)+TOff);
	} else if (tmpdbl < 10.0) {
		StrCopy(tempchar, DblToStr(tmpdbl, 0));
		StrCat(tempchar, text);
		WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*13), SCREEN.HEIGHT-(SCREEN.SRES*13)+TOff);
	} else if (tmpdbl < 100.0) {
		StrCopy(tempchar, DblToStr(tmpdbl, 0));
		StrCat(tempchar, text);
		WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*19), SCREEN.HEIGHT-(SCREEN.SRES*13)+TOff);
	} else if (tmpdbl < 1000.0) {
		StrCopy(tempchar, DblToStr(tmpdbl, 0));
		StrCat(tempchar, text);
		WinDrawChars(tempchar, StrLen(tempchar), SCREEN.WIDTH-(SCREEN.SRES*25), SCREEN.HEIGHT-(SCREEN.SRES*13)+TOff);
	}

	if (ismap) {
		// Drawing line to the left of the scale value
		WinDrawLine(SCREEN.WIDTH-(SCREEN.SRES*25), SCREEN.HEIGHT-(SCREEN.SRES*22)+TOff, SCREEN.WIDTH-(SCREEN.SRES*25), SCREEN.HEIGHT+TOff);
		// Drawing line on top of the scale value
		WinDrawLine(SCREEN.WIDTH-(SCREEN.SRES*25), SCREEN.HEIGHT-(SCREEN.SRES*22)+TOff, SCREEN.WIDTH, SCREEN.HEIGHT-(SCREEN.SRES*22)+TOff);
		// Drawing line on bottom of the scale value
		WinDrawLine(SCREEN.WIDTH-(SCREEN.SRES*25), SCREEN.HEIGHT+TOff, SCREEN.WIDTH, SCREEN.HEIGHT+TOff);
	}					
	FntSetFont(stdFont);

	// draw panning arrows if no GPS data being received
	if (ismap && (!recv_data || draw_log || (recv_data && (StrCompare(data.logger.gpsstat, "A") != 0)))) {
		FntSetFont(symbolFont);
		// Left Arrow
		WinDrawChars("\003", 1, WIDTH_MIN+(SCREEN.SRES*1), SCREEN.HEIGHT/2-(SCREEN.SRES*6));
		// Right Arrow
		WinDrawChars("\004", 1, SCREEN.WIDTH-(SCREEN.SRES*11), SCREEN.HEIGHT/2-(SCREEN.SRES*6));
		// Up Arrow
		WinDrawChars("\005", 1, SCREEN.WIDTH/2-(SCREEN.SRES*5), HEIGHT_MIN+(SCREEN.SRES*1));
		// Down Arrow
		if (data.config.btmlabels && !draw_log) {
 			WinDrawChars("\006", 1, SCREEN.WIDTH/2-(SCREEN.SRES*5), SCREEN.HEIGHT-(SCREEN.SRES*32));
		} else {
 			WinDrawChars("\006", 1, SCREEN.WIDTH/2-(SCREEN.SRES*5), SCREEN.HEIGHT-(SCREEN.SRES*10));
		}
		FntSetFont(largeBoldFont);
		WinDrawChars("RESET", 5, SCREEN.WIDTH/2+(SCREEN.SRES*15), HEIGHT_MIN);
		FntSetFont(stdFont);
	}

	// Drawing a line below the map area
	if (device.DIACapable && data.config.btmlabels) WinDrawLine(WIDTH_MIN, SCREEN.HEIGHT, SCREEN.WIDTH, SCREEN.HEIGHT);

	// Plot the reference glider on the screen
	if (!draw_task) {
		if (ismap) {
			// centre of screen                   
			DrawGlider(data.input.curmaporient, SCREEN.GLIDERX, SCREEN.GLIDERY);
		} else if (CalcPlotValues(gliderLat, gliderLon, data.input.gpslatdbl, data.input.gpslngdbl, xratio, yratio, &plotX, &plotY, &poirange, &poibearing, NOFORCE, data.input.curmaporient, false)) {
			// actual position
			DrawGlider(data.input.curmaporient, plotX, plotY);
		}
	}

#ifdef PRINT_FPS
	t = TimGetTicks();
	if (t - old_t > 0) {
		fps = (t - old_t)*1000/SysTicksPerSecond();
		StrIToA(text, fps);
		StrCat(text, " mspf");
//		FntSetFont(largeBoldFont);
//		WinDrawChars(text, StrLen(text), WIDTH_MIN, HEIGHT_MIN+(SCREEN.SRES*49));
//		FntSetFont(stdFont);
		old_t = t;
		HostTraceOutputTL(appErrorClass, "Ticks: %s",text);
	}
#endif

	// clear rectangle for other information if not drawing the moving map
	// call routine to update waypoint sector fields
	if (!ismap) {
		if (device.DIACapable) {
			RctSetRectangle (&rectP, WIDTH_MIN, SCREEN.HEIGHT-(SCREEN.SRES*45)+TextOff, SCREEN.WIDTH, SCREEN.SRES*113);
		} else {
			RctSetRectangle (&rectP, WIDTH_MIN, HEIGHT_MIN, SCREEN.WIDTH, 43*SCREEN.SRES);
		}
		WinEraseRectangle(&rectP, 0);
		UpdateSectorInfo(xratio, yratio);
	}

	// Copy the contents of the offscreen window to the active window 
	// When the screen copying is done, if it is a DIACapable display, it copies different amounts depending
	// on whether this is the normal map or the waypoint sector screen
	if (device.DIACapable) {
		if (!ismap) {
			// Waypoint Sector Screen
			RctSetRectangle (&rectP, 0, 0, SCREEN.WIDTH, SCREEN.HEIGHT+TextOff+40);
		} else { 
			// Normal Moving Map 
			RctSetRectangle (&rectP, 0, 0, SCREEN.WIDTH, SCREEN.HEIGHT+TextOff+2);
		}
	} else {
		RctSetRectangle (&rectP, 0, 0, SCREEN.WIDTH, SCREEN.HEIGHT);
	}
	WinCopyRectangle (winH, activeWinH, &rectP, 0, 0, winPaint);

	/* Reset everything */
	WinSetActiveWindow(activeWinH);  // sets Draw Window also
	activeWinH = WinSetDrawWindow(activeWinH);

	// Plot the selected flight log
	if (draw_log && !draw_task) {
		AllocMem((void *)&logdata, sizeof(LoggerData));
		firstpoint = true;

//		HostTraceOutputTL(appErrorClass, "Flight %s", DblToStr(selectedFltindex,0));
//		HostTraceOutputTL(appErrorClass, "1st Point %s", DblToStr(fltdata->startidx,0));
//		HostTraceOutputTL(appErrorClass, "last Point %s", DblToStr(fltdata->stopidx,0));

		// step thru log points
		for (recindex = fltdata->startidx; recindex < fltdata->stopidx; recindex += GetMapDownsampleScale(data.config.mapscaleidx)) {
			// get logged point record
			OpenDBQueryRecord(logger_db, recindex, &output_hand, &output_ptr);
			MemMove(logdata, output_ptr, sizeof(LoggerData));
			MemHandleUnlock(output_hand);

			// convert lat/lon
			pointlat = DegMinStringToLatLon(logdata->gpslat, *logdata->gpslatdir);
			pointlon = DegMinStringToLatLon(logdata->gpslng, *logdata->gpslngdir);

//			HostTraceOutputTL(appErrorClass, "Point no. %s", DblToStr(recindex,0));
//			HostTraceOutputTL(appErrorClass, "Lat %s %s", logdata->gpslat, logdata->gpslatdir);
//			HostTraceOutputTL(appErrorClass, "Lat %s", DblToStr(pointlat,4));
//			HostTraceOutputTL(appErrorClass, "Lon %s %s", logdata->gpslng, logdata->gpslngdir);
//			HostTraceOutputTL(appErrorClass, "Lon %s", DblToStr(pointlon,4));
//			HostTraceOutputTL(appErrorClass, "---");

			// plot point
			plotgood = CalcPlotValues(gliderLat, gliderLon, pointlat, pointlon, xratio, yratio,
						&plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);

			// points
//			if (plotgood) {
//				if (device.romVersion < SYS_VER_35) {
//					WinDrawLine(plotX, plotY, plotX, plotY;
//				} else {
//					WinDrawPixel(plotX, plotY);
//				}
//			}

			// lines
			if (firstpoint) {
				firstpoint = false;
			} else if (plotgood && prevplotgood) {
				WinDrawClippedLine(prevplotX, prevplotY, plotX, plotY, SOLID);
			}
			prevplotX = plotX;
			prevplotY = plotY;
			prevplotgood = plotgood;
		}

		// free memory
		FreeMem((void *)&logdata);
		FreeMem((void *)&fltdata);
	}

	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(kCoordinatesStandard);
		boldline = 1.0;
	}

	if (draw_log) {
		// show back button if plotting flight log or previewing task
		ctl_set_visible(form_moving_map_backbtn, true);
		// show mode
		if (draw_task) {
			if (taskpreview) {
				WinDrawChars("Fly", 3, 3, 1);
			} else {
				WinDrawChars("Pan", 3, 3, 1);
			}   	
			WinDrawChars("Task", 4, 138, 1);
		} else {
			WinDrawChars("Pan", 3, 3, 1);
			WinDrawChars("Log", 3, 143, 1);
		}		              	
	}

//	HostTraceOutputTL(appErrorClass, "Updatemap - Done");
}

