#include <PalmOS.h>
#include "soaring.h"
#include "soarTer.h"
#include "soarDB.h"
#include "soarUtil.h"
#include "soarUMap.h"
#include "soarMem.h"
#include "soarMath.h"
#include "soarSTF.h"
#include "soarIO.h"
#include "soarWay.h"

// used for terrain detection - hold the highest point on the terrain obstructing the glide
double crashlat, crashlon;
double crashlat1, crashlon1;
double crashalt = 0.0;
double crashalt1 = 0.0;
Boolean crash = false;
Boolean crash1 = false;
// variables for waypoint crash
Boolean wptcrash = false;
double wptcrashalt = 0.0;
double wptcrashlat = 0.0;
double wptcrashlon = 0.0;
// variables for task crash
Boolean tskcrash = false;
double tskcrashalt = 0.0;
double tskcrashlat = 0.0;
double tskcrashlon = 0.0;

// dynamic array for holding terrain height from current position to inuseWaypoint and task
double *wptterheights = NULL;
double *tskterheights = NULL;
Int32 prevnumwptter = 0;
Int32 numwptter = 0;
Int32 prevnumtskter = 0;
Int32 numtskter = 0;
double prevterbear = -999.9;
Boolean terrainpresent = false;
Boolean terrainvalid = false;
Boolean offterrain = false;
Boolean tskoffterrain = false;

// internal variables
double ullat, ullon;
double lrlat, lrlon;
Int16 numrows, numcols;
Int16 *terline = NULL;
MemHandle record_handle;
MemPtr record_ptr;

void InitTerrainData()
{
	TerHeader terhead;
	if (OpenDBCountRecords(terrain_db) > 0) {

		OpenDBQueryRecord(terrain_db, 0, &record_handle, &record_ptr);
		MemMove(&terhead, record_ptr, sizeof(TerHeader));
		MemHandleUnlock(record_handle);
		ullat = (double)terhead.ullat;
//		HostTraceOutputTL(appErrorClass, "terhead ullat=|%s|", DblToStr(ullat, 1));
		ullon = (double)terhead.ullon;
//		HostTraceOutputTL(appErrorClass, "terhead ullon=|%s|", DblToStr(ullon, 1));
		lrlat = (double)terhead.ullat-(terhead.numrows*GCONST);
//		HostTraceOutputTL(appErrorClass, "terhead lrlat=|%s|", DblToStr(lrlat, 1));
		lrlon = (double)terhead.ullon+(terhead.numcols*GCONST);
//		HostTraceOutputTL(appErrorClass, "terhead lrlon=|%s|", DblToStr(lrlon, 1));
		numrows = terhead.numrows;
//		HostTraceOutputTL(appErrorClass, "terhead numrows=|%hd|", terhead.numrows);
		numcols = terhead.numcols;
//		HostTraceOutputTL(appErrorClass, "terhead numcols=|%hd|", terhead.numcols);

//		HostTraceOutputTL(appErrorClass, "About to allocate terline");
		AllocMem((void *)&terline, (numcols * sizeof(Int16)));
//		HostTraceOutputTL(appErrorClass, "Finished allocating terline");

		terrainpresent = true;
	}
	return;
}

double GetTerrainElev(double curlat, double curlon)
{
	Int16 targetcol, targetrow;
	Int16 terrainelevation=0;
	static Int16 prevrow=-1, prevcol=-1;
	
	if (terrainpresent) {
//		HostTraceOutputTL(appErrorClass, "Get ullat=|%s|", DblToStr(ullat, 1));
//		HostTraceOutputTL(appErrorClass, "Get ullon=|%s|", DblToStr(ullon, 1));
//		HostTraceOutputTL(appErrorClass, "Get lrlat=|%s|", DblToStr(lrlat, 1));
//		HostTraceOutputTL(appErrorClass, "Get lrlon=|%s|", DblToStr(lrlon, 1));
//		HostTraceOutputTL(appErrorClass, "Get curlat=|%s|", DblToStr(curlat, 1));
//		HostTraceOutputTL(appErrorClass, "Get curlon=|%s|", DblToStr(curlon, 1));
		if (!PointOutOfBounds (ullat, ullon, lrlat, lrlon, curlat, curlon)) {
			// Have to add one to targetrow to account for first record
			// being a header record
			targetrow = (Int16)(Fabs(((ullat - curlat) / GCONST))) + 1;
//			HostTraceOutputTL(appErrorClass, "targetrow=|%hd|", targetrow);
			targetcol = (Int16)(Fabs(((ullon - curlon) / GCONST)));
			// This is the handle out-of-bounds cases which should not occur
			if (targetrow > numrows || targetcol > numcols) {
				terrainelevation = 0;
				terrainvalid = false;
				prevrow = -1;
				prevcol = -1;
				return((double)terrainelevation);
			}
//			HostTraceOutputTL(appErrorClass, "targetcol=|%hd|", targetcol);
			if (targetrow != prevrow) {
				OpenDBQueryRecord(terrain_db, targetrow, &record_handle, &record_ptr);
				MemMove(terline, record_ptr, (numcols * sizeof(Int16)));
				MemHandleUnlock(record_handle);
			}
			terrainelevation = terline[targetcol];
			terrainvalid = true;
			if (terrainelevation == -9999) {
//				HostTraceOutputTL(appErrorClass, "Terrain elevation -9999");
				terrainelevation = 0;
				terrainvalid = true;
			}
			prevrow = targetrow;
			prevcol = targetcol;
			terrainelevation = (Int16)(terrainelevation / ALTMETCONST); 
		} else {
//			HostTraceOutputTL(appErrorClass, "Terrain outofbounds");
			terrainelevation = 0;
			terrainvalid = false;
			prevrow = -1;
			prevcol = -1;
		}
	} else {
//		HostTraceOutputTL(appErrorClass, "Terrain not available");
		terrainelevation = 0;
		terrainvalid = false;
		prevrow = -1;
		prevcol = -1;
	}
//	HostTraceOutputTL(appErrorClass, "Terrain Elevation=|%hd|", terrainelevation);
	return((double)terrainelevation);
}

void CleanUpTerrainData()
{
	if (terrainpresent) {
		FreeMem((void *)&terline);
	}
}

Boolean loadterrain(double *terheights, Int32 numter, double origlat, double origlon, double origelev, double tgtlat, double tgtlon, double tgtelev)
{
// load terrain heights into pre-declared array of numter items

	Int32 i;
	double terlat, terlon;
	double ratio;
	Boolean onterrain = true;	

	// load terrain heights
	if (numter > 0) {
//		HostTraceOutputTL(appErrorClass, "Loading Terrain.....");
		for (i = 0; i <= numter; i++) {
			ratio  = (double)i / (double)numter;
			terlat = origlat + (tgtlat - origlat) * ratio;
			terlon = origlon + (tgtlon - origlon) * ratio;
			terheights[i] = GetTerrainElev(terlat, terlon);
			if ((i == 0)      && (terheights[i] < origelev)) terheights[i] = origelev;
			if ((i == numter) && (terheights[i] < tgtelev )) terheights[i] = tgtelev;
//			HostTraceOutputTL(appErrorClass, "Terrain Height %s", print_altitude(terheights[i]));
			if (!terrainvalid) {
				 // mark as invalid in array
				terheights[i] = -9999;
				onterrain = false;
			}
		}
	}

	return(onterrain);
}

Boolean terraincrash(double *terheights, Int32 numter, double origlat, double origlon, double tgtlat, double tgtlon, 
			double origalt, double tgtaalt, Boolean reset)
{
// check for the terrain height vs. predicted altitude on glide

	Int32 i;
	double ratio;
	double glidealt;
	double terclear=0.0;
	double terelev;
	Boolean markedcrash = false;
	Boolean newcrash = false;
	double tempalt;
	double templat, templon;

	if (reset) {
		crash = false;
		crash1 = false;
		crashalt = 99999.9;
		crashlat = INVALID_LAT_LON;
		crashlon = INVALID_LAT_LON;
	}

	if (offterrain) return(false);

	tempalt = crashalt;
	templat = crashlat;
	templon = crashlon;

	// initialise globals
	if (!crash1) {
		crashlat1 = INVALID_LAT_LON;
		crashlon1 = INVALID_LAT_LON;
	}

//	HostTraceOutputTL(appErrorClass, "Terrain Check.....");
	if (terrainpresent && terrainvalid && (numter > 0)) {
		for (i = 0; i <= numter; i++) {
			// calculate glide altitude and get terrain height
			// from current position to inuseWaypoint	
			ratio = (double)i / (double)numter;
			glidealt = origalt + (tgtaalt - origalt) * ratio;
			terelev = terheights[i];
//			HostTraceOutputTL(appErrorClass, "Terrain Height Check %s", print_altitude(terelev));
//			HostTraceOutputTL(appErrorClass, "Current Alt Check %s", print_altitude(glidealt));

			if (terelev == -9999) {
				// reached edge of terrain coverage so stop
//				HostTraceOutputTL(appErrorClass, "Reached invalid terrain");
				i = -1;
				break;
			}

			// calculate terrain clearance
			terclear = glidealt - (terelev + data.config.safealt);
//			HostTraceOutputTL(appErrorClass, "Glider altitude %s", DblToStr(glidealt*ALTMETCONST,0));
//			HostTraceOutputTL(appErrorClass, "Terrain height %s", DblToStr(terelev*ALTMETCONST,0));
//			HostTraceOutputTL(appErrorClass, "Terrain Clearance %s", DblToStr((glidealt-terelev)*ALTMETCONST,0));

			// check for terrain crash
			if (terclear < 0.0) {
				if (!crash1) {
					// save 1st crash point - predicted landing point
					crash1 = true;
					crashlat1 = origlat + (tgtlat - origlat) * ratio;
					crashlon1 = origlon + (tgtlon - origlon) * ratio;
				}
				// set terrain crash flag
				crash = true;
				newcrash = true;
			} else if (crash) {
				// no longer in terrain conflict, record peak crash position and altitude
//				markedcrash = true;
				crashalt = tempalt;
				crashlat = templat;
				crashlon = templon;
			}

			// check to see if this is the worst crash point
			if (terclear < (tempalt+1)) { // small fudge factor to prevent terrain crash on zero distance case
				if (i > numter-1) {
					// worst crash point is end point, so don't count as crash
//					HostTraceOutputTL(appErrorClass, "Highest Terrain is end point");
					crash = false;
					newcrash = false;
					crashalt = 0.0;
				}
				// mark worst crash point
				tempalt = terclear;
				templat = origlat + (tgtlat - origlat) * ratio;
				templon = origlon + (tgtlon - origlon) * ratio;
			}
		}
	} else {
		// terrain not present, so zero results
		newcrash = false;
		crash = false;
		crash1 = false;
		crashalt = 0.0;
	}

	if (!markedcrash) { 
		// record peak crash position and altitude if not done so already
		crashalt = tempalt;
		crashlat = templat;
		crashlon = templon;
	}

	return(newcrash);
}

Boolean tskterraincrash(double *terheights, Int16 activetskway, double origalt)
{
	Int16 i,j;
	Boolean newcrash = false;

	if (tskoffterrain) return(false);

	if (activetskway >= data.task.numwaypts-1) {
		// last waypoint in task, so return current waypoint crash
		newcrash = wptcrash;
	} else
	// check remaining task waypoints
	for (i = activetskway; i < data.task.numwaypts-1; i++) {
		while (data.task.waypttypes[i] & CONTROL) i++;
		j = i + 1;
		while (data.task.waypttypes[j] & CONTROL) j++;
//		HostTraceOutputTL(appErrorClass, "TP %s", tsk->wayptnames[i]);
//		HostTraceOutputTL(appErrorClass, "Index %s", DblToStr(tsk->terrainidx[i],0));
//		HostTraceOutputTL(appErrorClass, "Number %s", DblToStr(tsk->terrainidx[j]-tsk->terrainidx[i],0));
//		HostTraceOutputTL(appErrorClass, "Start Alt %s", print_altitude(origalt));
//		HostTraceOutputTL(appErrorClass, "Alt loss %s",print_altitude(tsk->alts[j]));
//		HostTraceOutputTL(appErrorClass, "Finish Alt %s", print_altitude(origalt - tsk->alts[j]));
//		HostTraceOutputTL(appErrorClass, "From Lat %s", DblToStr(tsk->wayptlats[i],3));
//		HostTraceOutputTL(appErrorClass, "To   Lat %s", DblToStr(tsk->wayptlats[j],3));
		newcrash |= terraincrash(&terheights[data.task.terrainidx[i]], data.task.terrainidx[j]-data.task.terrainidx[i],
					data.task.targetlats[i],data.task.targetlons[i], data.task.targetlats[j],data.task.targetlons[j],
					origalt, origalt - data.activetask.alts[j], false);
		origalt -= data.activetask.alts[j];
	}
	return(newcrash);
}

// Get the corners of the defined terrain.
// 0 - Upper left
// 1 - Upper right
// 2 - Lower right
// 3 - Lower left
Boolean GetTerrainBounds(double *pointlat, double *pointlon, Int8 corner)
{
	switch (corner) {
		case 0:
			*pointlat = ullat;
			*pointlon = ullon;
			break;
		case 1:
			*pointlat = ullat;
			*pointlon = lrlon;
			break;
		case 2:
			*pointlat = lrlat;
			*pointlon = lrlon;
			break;
		case 3:
			*pointlat = lrlat;
			*pointlon = ullon;
			break;
	}

	if (terrainpresent) {
		return(true);
	} else {
		return(false);
	}
}

Boolean loadtskterrain(TaskData *tertsk)
{
	Int16 x,z;
	double origelev, tgtelev;
	double tersum;

	tskoffterrain = false;

	// clear terrain array
	if ((prevnumtskter > 0) && (tskterheights != NULL)) {
		// free previous array
		FreeMem((void *)&tskterheights);
		tskterheights = NULL;
	}

	// load terrain on task
	if (data.config.usefgterrain) {
//		HostTraceOutputTL(appErrorClass, "Load Task Terrain....");

		// calculate array size
		numtskter = 0;
		for (x = 0; x < tertsk->numwaypts; x++) {
//			HostTraceOutputTL(appErrorClass, "Task Terrain Index %s", DblToStr(numtskter,0));
			if ((tertsk->waypttypes[x] & CONTROL) == 0) numtskter += (Int32)(tertsk->distances[x] * TERGRID)+1;
//			HostTraceOutputTL(appErrorClass, "Task Terrain Points %s", DblToStr(numtskter,0));
			tertsk->terrainidx[x] = numtskter;
		}
		tertsk->terrainidx[x] = numtskter;

		// allocate new size array
		if (AllocMem((void *)&tskterheights, (numtskter+1)*sizeof(double))) {
			MemSet(tskterheights, (numtskter+1)*sizeof(double), 0);
			prevnumtskter = numtskter;

			// load terrain array for each leg
//			HostTraceOutputTL(appErrorClass, "Loading Task Terrain.....");
			tskoffterrain = false;
			for (x = 0; x < tertsk->numwaypts-1; x++) {
				while (tertsk->waypttypes[x] & CONTROL) x++;
				z = x + 1;
				while (tertsk->waypttypes[z] & CONTROL) z++;
//				HostTraceOutputTL(appErrorClass, "Load Terrain From %s", tertsk->wayptnames[x]);
//				HostTraceOutputTL(appErrorClass, "Load Terrain To %s", tertsk->wayptnames[z]);
//				HostTraceOutputTL(appErrorClass, "start Terrain Points %s", DblToStr((tertsk->terrainidx[x]),0));
//				HostTraceOutputTL(appErrorClass, "Task Terrain Points %s", DblToStr((tertsk->terrainidx[z]-tertsk->terrainidx[x]),0));
				if ((tertsk->targetlats[x] == tertsk->wayptlats[x]) && (tertsk->targetlons[x] == tertsk->wayptlons[x])) {
					origelev = tertsk->elevations[x];
				} else {
					origelev = 0.0;
				}
				if ((tertsk->targetlats[z] == tertsk->wayptlats[z]) && (tertsk->targetlons[z] == tertsk->wayptlons[z])) {
					tgtelev = tertsk->elevations[z];
				} else {
					tgtelev = 0.0;
				}
				tskoffterrain = tskoffterrain | !loadterrain(&tskterheights[tertsk->terrainidx[x]], (tertsk->terrainidx[z]-tertsk->terrainidx[x]),
										tertsk->targetlats[x], tertsk->targetlons[x], origelev, tertsk->targetlats[z], tertsk->targetlons[z], tgtelev);
			}

			tersum = 0.0;
			for (x = 0; x < numtskter; x++) {
//					HostTraceOutputTL(appErrorClass, "Task Terrain Height %s", DblToStr(tskterheights[x],0));
					tersum += (Int16)tskterheights[x];
			}
//			HostTraceOutputT(appErrorClass, "Total Task Terrain Height %s", DblToStr(numtskter,0));
//			HostTraceOutputTL(appErrorClass, " : %s", DblToStr(tersum,0));

			// check full task for terrain crash
			tskcrash = false;
		} else {
			// AllocMeme failed
			tskoffterrain = true;
		}
	} else {
		tskcrash = false;
		return(true);
	}

	return(!tskoffterrain);
}
