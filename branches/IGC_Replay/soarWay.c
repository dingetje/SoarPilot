#include <PalmOS.h>
#include "soaring.h"
#include "soarWay.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarMap.h"
#include "soarUMap.h"
#include "soarIO.h"
#include "soarDB.h"
#include "soarTer.h"
#include "soarMem.h"
#include "Mathlib.h"
#include "soarMath.h"
#include "soarWLst.h"
#include "soarSTF.h"

/*****************************************************************************
 * globals
 *****************************************************************************/
Int16 tempwayidx = -1;

extern UInt16 WayptsortType;
extern Int32 rxrecs;
extern char rxtype[15];
extern double xratio;
extern double yratio;
extern double MCCurVal;
extern Boolean hadGPSdatavalid;
extern Char buf[PARSELEN];
extern WaypointData *TempWpt;

void SaveWaypoint(double lat, double lon, double alt, Boolean thrml)
{
	static Int16 wayidx;
	static Int16 nrecs;
	Char tempchar[30];
	Int16 foundwayptrec = 0;
	Int16 tempint = 0;
	WaypointData waypoint;

	MemSet(&waypoint, sizeof(WaypointData), 0);
	nrecs = DmNumRecords(waypoint_db);

	while ((foundwayptrec > -1) && (tempint <= nrecs)) {
		if (thrml) {
			StrCopy(waypoint.name, " ");
			StrCat(waypoint.name, DblToStr(pround(data.input.thavglift*data.input.lftconst, data.input.lftprec), data.input.lftprec));
//			StrCopy(tempchar, "            ");
//			StrCat(waypoint.name, Left(tempchar, 12-2-StrLen(waypoint.name)));
			StrCat(waypoint.name, " Th");
//			StrCat(waypoint.name, tempchar);
		} else {
			StrCopy(waypoint.name, "_");
			StrCat(waypoint.name, "WP");
			StrIToA(tempchar, tempint);
			StrCat(waypoint.name, tempchar);
		}
		foundwayptrec = FindWayptRecordByName(waypoint.name);
		tempint++;
	}

	waypoint.lat = lat;
	waypoint.lon = lon;
	waypoint.distance = 0.0;
	waypoint.bearing = 0.0;
	waypoint.type= 0x0;
	waypoint.alt = 0.0;
	if (thrml) {
		waypoint.elevation = alt;
		waypoint.type|= THRML;
		StrCopy(waypoint.rmks, "Thermal");
	} else {
		waypoint.elevation = alt;
		waypoint.type|= TURN;
		StrCopy(waypoint.rmks, "Temp Waypt ");
		StrIToA(tempchar, tempint-1);
		StrCat(waypoint.rmks, tempchar);
	}
	if (nrecs == 0) {
		waypoint.type|= HOME;
	}
	waypoint.arearadial1 = 0;
	waypoint.arearadial1mag = true;
	waypoint.arearadial2 = 0;
	waypoint.arearadial2mag = true;
	waypoint.arearadius = 0.0;
	waypoint.arearadius2 = 0.0;
	waypoint.rwdir = 0;
	waypoint.rwlen = 0.0;
//	StrCopy(waypoint.freq, "");
//	StrCopy(waypoint.geninfo, "");

//	HostTraceOutputTL(appErrorClass, "Name:%s",waypoint.name);

	wayidx = OpenDBAddRecord(waypoint_db, sizeof(WaypointData), &waypoint, false);

	return;
}

void waypoint_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	Int16  wayidx = 0;
	UInt16 strnglen;
	static Int16 next = 0;
	Char tempchar[PARSELEN];
	Char tempchar2[10];
	Char *tempcharptr=NULL;
	static WaypointData waypoint;
	static Boolean skip = false;
	static Boolean HOME_set = false;
	Boolean retval;
	Boolean dbempty = true;
	static Boolean lineoverflow = false;
	
	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		HOME_set = false;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		return;
	}

	// Check to see if there are any waypoints already in the database
	if (OpenDBCountRecords(waypoint_db)) {
		dbempty = false;
	}

//	HostTraceOutputTL(appErrorClass, "serinp:|%s|", serinp);
	MemSet(&waypoint, sizeof(WaypointData), 0);
	StrCopy(rxtype, "Waypoints");

	while (cur<length) {
		while( (cur<length) && (next<PARSELEN) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		buf[next] = '\0';
//		HostTraceOutputTL(appErrorClass, "buf:|%s|", buf);

		if (buf[0] == '*') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
		}

		if (next >= PARSELEN) {
//			HostTraceOutputTL(appErrorClass, "Parsing error->PARSELEN Chars");
			/* Parsing error start over */
			lineoverflow = true;
			next=0;
		}
		
		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

			if (cur <= length && skip == false) {

				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else {
					//	Getting waypoint latitude
					GetField(buf, 1, tempchar);
					if ((tempcharptr = StrChr(tempchar, ':')) != NULL) {
						if (StrChr(tempcharptr+1, ':') != NULL) {
							waypoint.lat = DegMinSecColonStringToLatLon(tempchar);
						} else {
							waypoint.lat = DegMinColonStringToLatLon(tempchar);
						}
					} else {
						waypoint.lat = DegMinColonStringToLatLon(tempchar);
					}

					//	Getting waypoint longitude
					GetField(buf, 2, tempchar);
					if ((tempcharptr = StrChr(tempchar, ':')) != NULL) {
						if (StrChr(tempcharptr+1, ':') != NULL) {
							waypoint.lon = DegMinSecColonStringToLatLon(tempchar);
						} else {
							waypoint.lon = DegMinColonStringToLatLon(tempchar);
						}
					} else {
						waypoint.lon = DegMinColonStringToLatLon(tempchar);
					}

					//	Getting waypoint elevation
					GetField(buf, 3, tempchar);
					strnglen = StrLen(tempchar);
					if (strnglen > 1) {
						if (StrChr(tempchar, 'F')) {
							tempchar[strnglen-1] = '\0';
							waypoint.elevation = StrToDbl(tempchar);
						} else {
							tempchar[strnglen-1] = '\0';
							waypoint.elevation = StrToDbl(tempchar)/ALTMETCONST;
						}
					} else {
						waypoint.elevation = 0.0;
					}

					//	Getting waypoint attribute
					GetField(buf, 4, tempchar);
					waypoint.type = NULL;
					if (StrChr(tempchar, 'A')) waypoint.type |= AIRPORT;
					if (StrChr(tempchar, 'T')) waypoint.type |= TURN;
					if (StrChr(tempchar, 'L')) waypoint.type |= LAND;
					if (StrChr(tempchar, 'S')) waypoint.type |= START;
					if (StrChr(tempchar, 'F')) waypoint.type |= FINISH;
					if (StrChr(tempchar, 'M')) waypoint.type |= MARK;
					if (StrChr(tempchar, 'H')) waypoint.type |= HOME;
	/* PG - Deleted, was causing 2 HOME points to be set
								&& (HOME_set==false)) || (dbempty)) {
						// Will only set the HOME attribute on the first waypoint in the file
						// All others will be ignored.
						// This also will clear the HOME status on all others in the database
						// And initialize various variables to make this the one and only one
						// waypoint with the HOME attribute set.
						if (!dbempty) {
							ClearWayptHOMEStatus();
						}
						waypoint.type |= HOME;
	//					HostTraceOutputTL(appErrorClass, "Setting HOME wpt");
						HomeWayptInitialize(&waypoint);
						if (StrChr(tempchar, 'H')) {
							HOME_set = true;
						}
						data.input.terelev = GetTerrainElev(data.input.gpslatdbl, data.input.gpslngdbl);
					}
	*/
					if ((waypoint.type & AIRPORT) || (waypoint.type & LAND)) waypoint.type |= AIRLAND;

					//	Getting waypoint name
	//				GetField(buf, 5, tempchar);
					GetFieldDelim(buf, 5, ',', ',', tempchar);
					StrNCopy(waypoint.name, trim(NoComma(tempchar," "), ' ', true), 12);

					//	Getting waypoint remarks
					GetField(buf, 6, tempchar);
					StrNCopy(waypoint.rmks, trim(tempchar, ' ', true), 12);

					//	Getting the area turnpoint info if it is available
					retval = GetField(buf, 7, tempchar);
					if (retval) {
						MemSet(&tempchar2, sizeof(tempchar2), 0);
	//					HostTraceOutputTL(appErrorClass, "tempchar:|%s|", tempchar);
						StrNCopy(tempchar2, tempchar, 6);
	//					HostTraceOutputTL(appErrorClass, "tempchar2:|%s|", tempchar2);
						waypoint.arearadius = StrToDbl(tempchar2)/1000.0;

						MemSet(&tempchar2, sizeof(tempchar2), 0);
						tempcharptr = &tempchar[6];
	//					HostTraceOutputTL(appErrorClass, "tempchar:|%s|", tempchar);
						StrNCopy(tempchar2, tempcharptr, 6);
	//					HostTraceOutputTL(appErrorClass, "tempchar2:|%s|", tempchar2);
						waypoint.arearadius2 = StrToDbl(tempchar2)/1000.0;

						MemSet(&tempchar2, sizeof(tempchar2), 0);
						tempcharptr = &tempchar[12];
	//					HostTraceOutputTL(appErrorClass, "1-tempcharptr:|%s|", tempcharptr);
						StrNCopy(tempchar2, tempcharptr, 3);
	//					HostTraceOutputTL(appErrorClass, "1-tempchar2:|%s|", tempchar2);
						waypoint.arearadial1 = (Int16)StrAToI(tempchar2);
	//					HostTraceOutputTL(appErrorClass, "1-waypoint.arearadial1:|%hd|", waypoint.arearadial1);

						MemSet(&tempchar2, sizeof(tempchar2), 0);
						tempcharptr = &tempchar[15];
	//					HostTraceOutputTL(appErrorClass, "2-tempcharptr:|%s|", tempcharptr);
						StrNCopy(tempchar2, tempcharptr, 3);
	//					HostTraceOutputTL(appErrorClass, "2-tempchar2:|%s|", tempchar2);
						waypoint.arearadial2 = (Int16)StrAToI(tempchar2);
	//					HostTraceOutputTL(appErrorClass, "2-waypoint.arearadial2:|%hd|", waypoint.arearadial2);

						waypoint.type|= AREA;
					} else {
	//					HostTraceOutputTL(appErrorClass, "arearadius not found, setting to 0.0");
						waypoint.arearadius = 0.0;
						waypoint.arearadius2 = 0.0;
						waypoint.arearadial1 = 0;
						waypoint.arearadial2 = 0;
					}

					// setting the last used time stamp
					waypoint.UsedTime[0] = '\0';

					if ((StrLen(waypoint.name) > 0) && (waypoint.lat != INVALID_LAT_LON) && (waypoint.lon != INVALID_LAT_LON)) {
	//					HostTraceOutputTL(appErrorClass, "Accepting %s %s",waypoint.name, DblToStr(wayidx,0));
						wayidx = OpenDBAddRecord(waypoint_db, sizeof(WaypointData), &waypoint, false);
						rxrecs = wayidx;
						if (FrmGetActiveFormID() == form_transfer) {
							// update record counter on transferscreen
							field_set_value(form_transfer_records, DblToStr(wayidx+1,0));
						}
					} else {
	//					HostTraceOutputTL(appErrorClass, "Rejecting %s %s %s %s",waypoint.name,waypoint.rmks,DblToStr(waypoint.lat,2),DblToStr(waypoint.lon,2));
					}
				}
			}
			next=0;
		}
//		HostTraceOutputTL(appErrorClass, "Setting skip to false");
		skip = false;
//		HostTraceOutputTL(appErrorClass, "============================================================");
	}
//	HostTraceOutputTL(appErrorClass, "-------------------------------------------------------------");
	return;
}

void OutputWayHeader(Int8 wpformat)
{
	Char output_char[81];
	Char dtgstr[15];
	DateType date;

	MemSet(output_char, sizeof(output_char) ,0);

	DateSecondsToDate (TimGetSeconds(), &date);
	DateToAscii (date.month, date.day, date.year+1904, dfDMYLong, dtgstr);

	switch (wpformat) {
	case WPCAI:
		StrCopy(output_char, "** -------------------------------------------------------------");
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);

		StrCopy(output_char, "**      SOARINGPILOT Version ");
		StrCat(output_char, device.appVersion);
		StrCat(output_char, " Waypoints");
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);

		StrCopy(output_char, "**      Date: ");
		StrCat(output_char, dtgstr);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);

		StrCopy(output_char, "** -------------------------------------------------------------");
		StrCatEOL(output_char, data.config.xfertype);
		break;
	case WPCU:
	case WPCUN:
		StrCopy(output_char,"name,code,country,lat,lon,elev,style,rwdir,rwlen,freq,desc");
		StrCatEOL(output_char, data.config.xfertype);
		break;
	case WPGPWPL:
	case WPGPWPLALT:
		// no header
		break;
	case WPOZI:
		StrCopy(output_char, "OziExplorer Waypoint File Version 1.1");
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);

		StrCopy(output_char, "WGS 84");
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);

		StrCopy(output_char,"Reserved 2");
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);

		StrCopy(output_char, "garmin");
		StrCatEOL(output_char, data.config.xfertype);
		break;
	default:
		break;
	}

	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// sort waypoints into ascending name order
	WayptsortType = SortByNameA;
	DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, WayptsortType);

	return;
}

void OutputWayCAI()
{
	Char output_char[81];
	Char *charP=NULL;
	Char tempchar[30];
	Int16 wayindex=0;
	Int16 nrecs;
	MemHandle output_hand;
	MemPtr output_ptr;
	WaypointData *waydata;

	nrecs = OpenDBCountRecords(waypoint_db);

	AllocMem((void *)&waydata , sizeof(WaypointData));

	for (wayindex=0; wayindex<nrecs; wayindex++) {
		if ((wayindex % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, wayindex, "Waypoints");

		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);

		StrCopy(output_char, StrIToA(tempchar, (wayindex+1)));
		StrCat(output_char, ",");

		LLToStringDM(waydata->lat, tempchar, ISLAT, true, true, 3);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");

		LLToStringDM(waydata->lon, tempchar, ISLON, true, true, 3);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");

		StrIToA(tempchar, (Int32)waydata->elevation);
		StrCat(output_char, tempchar);
		StrCat(output_char, "F");
		StrCat(output_char, ",");

		if (waydata->type & AIRPORT) StrCat(output_char, "A");
		if (waydata->type & TURN) StrCat(output_char, "T");
		if (waydata->type & LAND) StrCat(output_char, "L");
		if (waydata->type & START) StrCat(output_char, "S");
		if (waydata->type & FINISH) StrCat(output_char, "F");
		if (waydata->type & MARK) StrCat(output_char, "M");
		if (waydata->type & HOME) StrCat(output_char, "H");
		StrCat(output_char, ",");

		while ((charP=StrChr(waydata->name, ','))) {
			*charP = ' ';
		}
		StrCat(output_char, waydata->name);
		StrCat(output_char, ",");

		while ((charP=StrChr(waydata->rmks, ','))) {
			*charP = ' ';
		}
		StrCat(output_char, waydata->rmks);
		if (waydata->type & AREA) {
			StrCat(output_char, ",");
			StrCat(output_char, leftpad(DblToStr2(waydata->arearadius, 3, false), '0', 6));
			StrCat(output_char, leftpad(DblToStr2(waydata->arearadius2, 3, false), '0', 6));
			StrCat(output_char, leftpad(StrIToA(tempchar, (Int32)waydata->arearadial1), '0', 3));
			StrCat(output_char, leftpad(StrIToA(tempchar, (Int32)waydata->arearadial2), '0', 3));
		}

		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}
	FreeMem((void *)&waydata);
	return;
}

void OutputWayCU()
{
	Char output_char[200];
	Char *charP=NULL;
	Char tempchar[64];
	Int16 wayindex=0;
	Int16 nrecs;
	MemHandle output_hand;
	MemPtr output_ptr;
	WaypointData *waydata;

	nrecs = OpenDBCountRecords(waypoint_db);

	AllocMem((void *)&waydata, sizeof(WaypointData));

	for (wayindex=0; wayindex<nrecs; wayindex++) {
		if ((wayindex % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, wayindex, "Waypoints");

		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);
		// Title
		while ((charP=StrChr(waydata->rmks, ','))) {
			*charP = ' ';
		}
		StrCopy(output_char, "\"");
		StrCat(output_char, waydata->rmks);
		StrCat(output_char, "\",");
		// Code
		while ((charP=StrChr(waydata->name, ','))) {
			*charP = ' ';
		}
		StrCat(output_char, "\"");
		StrCat(output_char, waydata->name);
		StrCat(output_char, "\",");
		// Country (not used)
		StrCat(output_char, "--");
		StrCat(output_char, ",");
		// Lat
		LLToStringDM(waydata->lat, tempchar, ISLAT, false, true, 3);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
		// Lon
		LLToStringDM(waydata->lon, tempchar, ISLON, false, true, 3);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
		// Elev
		StrCat(output_char, DblToStr(pround(waydata->elevation * ALTMETCONST, 1), 1));
		StrCat(output_char, "m");
		StrCat(output_char, ",");
		// Type
		StrCopy(tempchar,"0");
		if ((waydata->type & TURN) || (waydata->type & FINISH) || (waydata->type & START))  StrCopy(tempchar,"1");
		if (waydata->type & LAND) StrCopy(tempchar,"3");
		if (waydata->type & AIRPORT) StrCopy(tempchar,"5");
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
		if ((waydata->rwlen != 0) || (waydata->rwdir != 0)) {
			// Runway Direction
			StrCat(output_char, DblToStr(waydata->rwdir,0));
			StrCat(output_char, ",");
			// Runway Length
			StrCat(output_char, DblToStr(pround(waydata->rwlen * ALTMETCONST, 1), 1));
			StrCat(output_char, "m");
		} else {
			StrCat(output_char, ",");
		}
		StrCat(output_char, ",");	
		if (StrLen(waydata->freq) > 0) {
			// Frequency
			while ((charP=StrChr(waydata->freq, ','))) {
				*charP = ' ';
			}
			StrCat(output_char, "\"");
			StrCat(output_char, waydata->freq);
			StrCat(output_char, "\"");
		}
		StrCat(output_char, ",");
		if (StrLen(waydata->geninfo) > 0) {
			// Description	
			while ((charP=StrChr(waydata->geninfo, ','))) {
				*charP = ' ';
			}	
			StrCat(output_char, "\"");
			StrCat(output_char, waydata->geninfo);
			StrCat(output_char, "\"");
		}
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}
	FreeMem((void *)&waydata);
	return;
}

void OutputWayGPWPL()
{
	Char output_char[80];
	Char tempchar[35];
	Int16 wayindex = 0;
	Int16 nrecs, len, i;
	UInt8 chksum = 0;
	MemHandle output_hand;
	MemPtr output_ptr;
	WaypointData *waydata;
	double deg, alt;

	nrecs = OpenDBCountRecords(waypoint_db);

	AllocMem((void *)&waydata, sizeof(WaypointData));

	for (wayindex=0; wayindex<nrecs; wayindex++) {
		if ((wayindex % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, wayindex, "Waypoints");

		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);
		chksum = 0;

		// NMEA header
		StrCopy(output_char, "$GPWPL,");
		// lat deg
		deg = Floor(Fabs(waydata->lat));
		StrCopy(tempchar,"00");
		StrCat(tempchar,DblToStr(deg,0));
		StrCat(output_char, Right(tempchar,2));
		// lat min
		StrCopy(tempchar,"00");
		StrCat(tempchar,DblToStr(pround(60*(Fabs(waydata->lat)-deg),3),3));
		StrCat(output_char, Right(tempchar,6));
		StrCat(output_char,",");
		// lat hemisphere
		if (waydata->lat >= 0.0) {
			StrCat(output_char,"N,");
		} else {
			StrCat(output_char,"S,");
		}
		// long deg
		deg = Floor(Fabs(waydata->lon));
		StrCopy(tempchar,"000");
		StrCat(tempchar,DblToStr(deg,0));
		StrCat(output_char, Right(tempchar,3));
		// long min
		StrCopy(tempchar,"00");
		StrCat(tempchar,DblToStr(pround(60*(Fabs(waydata->lon)-deg),3),3));
		StrCat(output_char, Right(tempchar,6));
		StrCat(output_char,",");
		// long hemisphere
		if (waydata->lon >= 0.0) {
			StrCat(output_char,"E,");
		} else {
			StrCat(output_char,"W,");
		}
		// ident - 6 chars, uppercase
		ConvertToUpper(waydata->name);
		if (StrLen(waydata->name) < 6) {
			StrCat(waydata->name, "      ");
		}
		if (data.config.wpformat == WPGPWPLALT) {
			// altitude coded as meters/10
			StrCat(output_char, Left(waydata->name,3));
			StrCopy(tempchar,"000");
			alt = waydata->elevation * ALTMETCONST / 10;
			StrCat(tempchar,DblToStr(pround(alt,0),0));
			StrCat(output_char, Right(tempchar,3));
		} else {
			// no altitude
			StrCat(output_char, Left(waydata->name,6));
		}
		// remarks, uppercase only
		ConvertToUpper(waydata->rmks);
		StrCat(output_char, "0");
		StrCat(output_char, waydata->rmks);
		StrCat(output_char, " "); // space after comment included in checksum
		// checksum
		len = StrLen(output_char);
		for (i=1; i < len; i++) {
			chksum ^= output_char[i];
		}
		StrCat(output_char,"*");
		//StrCat(output_char," *");
		StrCopy(tempchar,"00");
		StrCat(tempchar,Dec2Hex(chksum));
		StrCat(output_char,Right(tempchar,2));

		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
		if ((data.config.xfertype != USEVFS) && (data.config.xfertype != USEDOC) && (data.config.xfertype != USEMEMO)) {
			// pause after data transmit for serial connections
			Sleep(0.20);
		}
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	FreeMem((void *)&waydata);
	return;
}

/*
void OutputWayGDN()
{
	Char output_char[81];
	UInt16 wayindex;
	MemHandle output_hand;
	MemPtr output_ptr;
	WaypointData waydata;
	UInt16 nrecs;

	nrecs = OpenDBCountRecords(waypoint_db);

	for (wayindex=0; wayindex<nrecs; wayindex++) {
		if ((wayindex % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, wayindex, "Waypoints");

		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(&waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);

//		tempchar = waydata.gpslng;
//		StrCat(output_char, tempchar);
//		StrCat(output_char, waydata.gpslngdir);
//		StrCat(output_char, waydata.gpsstat);

		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}
	return;
}
*/

Int16 FindWayptRecordByNamePartial(Char* NameString) 
{
	Int16 wayindex = 0;
	Int16 nrecs;
	WaypointData *waydata;
	MemHandle wayhand;
	MemHandle output_hand;
	MemPtr output_ptr;
	Int16 retval=-1;
	Char wayptname[13];
	
	nrecs = OpenDBCountRecords(waypoint_db);

	StrNCopy(wayptname, NameString, 12);
	ConvertToUpper(wayptname);

	wayhand = MemHandleNew(sizeof(WaypointData));
	waydata = MemHandleLock(wayhand);
	for (wayindex=0; wayindex<nrecs; wayindex++) {
		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);
	
		ConvertToUpper(waydata->name);
		if (StrNCompare(wayptname, waydata->name, StrLen(wayptname)) <= 0) {
			retval = wayindex;
			wayindex = nrecs;
		}
	}
	MemHandleUnlock(wayhand);
	MemHandleFree(wayhand);
	return(retval);
}

Int16 FindWayptRecordByName(Char* NameString) 
{
	Int16 wayindex = 0;
	Int16 nrecs;
	WaypointData *waydata;
	MemHandle wayhand;
	MemHandle output_hand;
	MemPtr output_ptr;
	Int16 retval=-1;

	nrecs = OpenDBCountRecords(waypoint_db);

	wayhand = MemHandleNew(sizeof(WaypointData));
	waydata = MemHandleLock(wayhand);
	for (wayindex=0; wayindex<nrecs; wayindex++) {
		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);

		if (StrCompare(NameString, waydata->name) == 0) {
			retval = wayindex;
			wayindex = nrecs;
		}
	}
	MemHandleUnlock(wayhand);
	MemHandleFree(wayhand);
	return(retval);
}

void ClearWayptHOMEStatus() 
{
	Int16 wayindex = 0;
	Int16 nrecs;
	WaypointData *waydata;
	MemHandle wayhand;
	MemHandle output_hand;
	MemPtr output_ptr;

	nrecs = OpenDBCountRecords(waypoint_db);

	wayhand = MemHandleNew(sizeof(WaypointData));
	waydata = MemHandleLock(wayhand);
	for (wayindex=0; wayindex<nrecs; wayindex++) {
		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);

		waydata->type &= ~HOME;
		OpenDBUpdateRecord(waypoint_db, sizeof(WaypointData), waydata, wayindex);
	}
	MemHandleUnlock(wayhand);
	MemHandleFree(wayhand);
	return;
}

void ClearWayptREFStatus() 
{
	Int16 wayindex = 0;
	Int16 nrecs;
	WaypointData *waydata;
	MemHandle wayhand;
	MemHandle output_hand;
	MemPtr output_ptr;

	nrecs = OpenDBCountRecords(waypoint_db);

	wayhand = MemHandleNew(sizeof(WaypointData));
	waydata = MemHandleLock(wayhand);
	for (wayindex=0; wayindex<nrecs; wayindex++) {
		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);

		waydata->type &= ~REFWPT;

		OpenDBUpdateRecord(waypoint_db, sizeof(WaypointData), waydata, wayindex);
	}
	MemHandleUnlock(wayhand);
	MemHandleFree(wayhand);

	data.input.refLat = INVALID_LAT_LON;
	data.input.refLon = INVALID_LAT_LON;
	data.input.refwpttype = REFNONE;

	return;
}

Boolean FindHomeWayptInitialize() 
{
	Int16 wayindex = 0;
	Int16 nrecs;
	WaypointData *waydata;
	MemHandle wayhand;
	MemHandle output_hand;
	MemPtr output_ptr;
	Boolean found=true;

	data.input.true_track.value = 0.0;
	data.input.true_track.valid = VALID;
//	HostTraceOutputTL(appErrorClass, "data.input.true_track.value-|%s|", DblToStr(data.input.true_track.value, 1));
	data.input.deviation.value = 0.0;
	data.input.deviation.valid=VALID;
//	HostTraceOutputTL(appErrorClass, "data.input.deviation.value-|%s|", DblToStr(data.input.deviation.value, 1));

	// do not re-initialise home waypoint if a valid fix has been found
	if (hadGPSdatavalid) return(found);

	nrecs = OpenDBCountRecords(waypoint_db);

	if (nrecs > 0) {
		wayhand = MemHandleNew(sizeof(WaypointData));
		waydata = MemHandleLock(wayhand);
		for (wayindex=0; wayindex<nrecs; wayindex++) {
			OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
			MemMove(waydata, output_ptr, sizeof(WaypointData));
			MemHandleUnlock(output_hand);

			if ((waydata->type & HOME) || wayindex == 0) {
//				HostTraceOutputTL(appErrorClass, "Found HOME -|%s|", waydata->name);
				found = true;
				tempwayidx = wayindex;

				HomeWayptInitialize(waydata);

				// If found the home waypoint, we're all done
				if (waydata->type & HOME) {
					wayindex = nrecs;
				}
			}
		}
		MemHandleUnlock(wayhand);
		MemHandleFree(wayhand);
	} else {
		StrCopy(data.logger.gpslat, "3648.568");
//		HostTraceOutputTL(appErrorClass, "data.logger.gpslat-|%s|", data.logger.gpslat);
		StrCopy(data.logger.gpslatdir, "N");
//		HostTraceOutputTL(appErrorClass, "data.logger.gpslatdir-|%s|", data.logger.gpslatdir);
		data.input.gpslatdbl = 36.80947;
		data.input.coslat = cos(DegreesToRadians(data.input.gpslatdbl));
		data.input.homeLat = 36.80947;

		StrCopy(data.logger.gpslng, "07609.357");
//		HostTraceOutputTL(appErrorClass, "data.logger.gpslng-|%s|", data.logger.gpslng);
		StrCopy(data.logger.gpslngdir, "W");
//		HostTraceOutputTL(appErrorClass, "data.logger.gpslngdir-|%s|", data.logger.gpslngdir);
		data.input.gpslngdbl = -76.15595;
		data.input.homeLon = -76.15595;

		data.input.homeElev = 80.0;
	}

	calcallwaypoints(data.input.homeLat, data.input.homeLon);

	return(found);
}

Boolean FindRefWayptInitialize() 
{
	Int16 wayindex = 0;
	Int16 nrecs;
	WaypointData *waydata;
	MemHandle wayhand;
	MemHandle output_hand;
	MemPtr output_ptr;
	Boolean found=true;

	data.input.refLat = INVALID_LAT_LON;
	data.input.refLon = INVALID_LAT_LON;
	data.input.refwpttype = REFNONE;

	nrecs = OpenDBCountRecords(waypoint_db);

	if (nrecs > 0) {
		wayhand = MemHandleNew(sizeof(WaypointData));
		waydata = MemHandleLock(wayhand);
		for (wayindex=0; wayindex<nrecs; wayindex++) {
			OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
			MemMove(waydata, output_ptr, sizeof(WaypointData));
			MemHandleUnlock(output_hand);

			if (waydata->type & REFWPT) {
//				HostTraceOutputTL(appErrorClass, "Found REF -|%s|", waydata->name);
				found = true;

				data.input.refLat = waydata->lat;
				data.input.refLon = waydata->lon;
				if (data.config.RefWptRadial) {
					data.input.refwpttype = REFRADIAL;
				} else {
					data.input.refwpttype = REFBEARING;
				}
				// found the ref waypoint, we're all done
				wayindex = nrecs;
			}
		}
		MemHandleUnlock(wayhand);
		MemHandleFree(wayhand);
	}

	return(found);
}

// calculates distance and bearing to all waypoints from given position
void calcallwaypoints(double gpslat, double gpslon)
{
	Int16 wayindex = 0;
	Int16 nrecs;
	WaypointData *waydata;
	MemHandle wayhand;
	MemHandle output_hand;
	MemPtr output_ptr;
	double poirange, poibearing;

//	HostTraceOutputTL(appErrorClass, "Calculating all waypoints");
	nrecs = OpenDBCountRecords(waypoint_db);
	data.input.coslat = cos(DegreesToRadians(gpslat));

	wayhand = MemHandleNew(sizeof(WaypointData));
	waydata = MemHandleLock(wayhand);
	for (wayindex=0; wayindex<nrecs; wayindex++) {
		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);

		LatLonToRangeBearing(gpslat, gpslon, waydata->lat, waydata->lon, &poirange, &poibearing);
		// LatLonToRangeBearing returns a True bearing
		// Converted to Magnetic
		poibearing = nice_brg(poibearing + data.input.deviation.value);
		waydata->distance = poirange;
		waydata->bearing = poibearing;

		OpenDBUpdateRecord(waypoint_db, sizeof(WaypointData), waydata, wayindex);
//		HostTraceOutputTL(appErrorClass, "Calc Wpt %s",DblToStr(wayindex,0));
	}
	MemHandleUnlock(wayhand);
	MemHandleFree(wayhand);
	return;
}

// Fills in the apprpriate variables to set the passed in waypoint data
// as the current HOME waypoint
void HomeWayptInitialize(WaypointData *waydata) 
{
	Char nsew[2];
	Char tempchar[12];

	StrPrintF(nsew, "%c", LLToStringDM(waydata->lat, tempchar, ISLAT, false, true, 3));
	tempchar[StrLen(tempchar)-1] = '\0';
	StrCopy(data.logger.gpslat, tempchar);
//	HostTraceOutputTL(appErrorClass, "data.logger.gpslat-|%s|", data.logger.gpslat);
	StrCopy(data.logger.gpslatdir, nsew);
//	HostTraceOutputTL(appErrorClass, "data.logger.gpslatdir-|%s|", data.logger.gpslatdir);
	data.input.gpslatdbl = waydata->lat;
	data.input.coslat = cos(DegreesToRadians(data.input.gpslatdbl));
	data.input.homeLat = waydata->lat;

	StrPrintF(nsew, "%c", LLToStringDM(waydata->lon, tempchar, ISLON, false, true, 3));
	tempchar[StrLen(tempchar)-1] = '\0';
	StrCopy(data.logger.gpslng, tempchar);
//	HostTraceOutputTL(appErrorClass, "data.logger.gpslng-|%s|", data.logger.gpslng);
	StrCopy(data.logger.gpslngdir, nsew);
//	HostTraceOutputTL(appErrorClass, "data.logger.gpslngdir-|%s|", data.logger.gpslngdir);
	data.input.gpslngdbl = waydata->lon;
	data.input.homeLon = waydata->lon;

	data.input.homeElev = waydata->elevation;

	return;
}

//static const CustomPatternType pattern = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};

void DrawFGWayLine(Int16 centerX, Int16 centerY)
{
	Int16 plotX=centerX, plotY=centerY;
	Int16 oppplotX=centerX, oppplotY=centerY;
	Int16 arrowlendX=centerX, arrowlendY=centerY;
	Int16 arrowrendX=centerX, arrowrendY=centerY;
	double xrat=0, yrat=0;
	RectangleType rectP;
	double bearing = data.input.bearing_to_destination.value;
	double sf = 1.6;

	// for current waypoint
	rectP.topLeft.x = centerX - ARROWCENTXOFF;
	rectP.topLeft.y = centerY - ARROWCENTYOFF;
	rectP.extent.x = ARROWAREAWIDTH;
	rectP.extent.y = ARROWAREAWIDTH;
	WinEraseRectangle(&rectP, 0);
	WinDrawRectangleFrame (simpleFrame, &rectP);

	// for next waypoint
	rectP.topLeft.x = (Coord)(152 - (Int16)ARROWCENTXOFF / sf);
	rectP.topLeft.y = (Coord)(137 - (Int16)ARROWCENTYOFF / sf);
	rectP.extent.x = (Coord)(ARROWAREAWIDTH / sf);
	rectP.extent.y = (Coord)(ARROWAREAWIDTH / sf);
	WinEraseRectangle(&rectP, 0);

	// calculate ratios
	xrat = ARROWCENTXOFF / (ARROWCENTXOFF / ARROWAREAWIDTH);
	yrat = ARROWCENTYOFF / (ARROWCENTYOFF / ARROWAREAWIDTH);

	// wind caret
	if (data.config.windarrow) {
//	if (data.config.windarrow && (data.input.wndspd > 2.5)) { // only show wind caret if above 2.5kts / 5kph
		RangeBearingToPixel(xrat, yrat, centerX, centerY, 0.44, data.input.wnddir, NOFORCE, TRACKUP, &plotX, &plotY);
		WinDrawCircle(plotX, plotY, 2, SOLID);
  		if (device.romVersion < SYS_VER_35) {
			WinDrawLine(plotX+1, plotY+1, plotX+1, plotY+1);
			WinDrawLine(plotX-1, plotY-1, plotX-1, plotY-1);
			WinDrawLine(plotX-1, plotY+1, plotX-1, plotY+1);
			WinDrawLine(plotX+1, plotY-1, plotX+1, plotY-1);
		} else {
			WinDrawPixel(plotX+1, plotY+1);
			WinDrawPixel(plotX-1, plotY-1);
			WinDrawPixel(plotX-1, plotY+1);
			WinDrawPixel(plotX+1, plotY-1);
		}
		WinDrawLine(plotX, plotY, centerX, centerY);
	}

	if (data.input.bearing_to_destination.valid==VALID) {
		// draw arrow for bearing to target waypoint
		RangeBearingToPixel(xrat, yrat, centerX, centerY, 0.45, bearing, NOFORCE, TRACKUP, &plotX, &plotY);
		RangeBearingToPixel(xrat, yrat, centerX, centerY, 0.45, RecipCse(bearing), NOFORCE, TRACKUP, &oppplotX, &oppplotY);
		RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(bearing - 150.0), NOFORCE, TRACKUP, &arrowlendX, &arrowlendY);
		RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(bearing + 150.0), NOFORCE, TRACKUP, &arrowrendX, &arrowrendY);
		WinDrawLine(plotX, plotY, oppplotX, oppplotY);
		WinDrawLine(plotX, plotY, arrowlendX, arrowlendY);
		WinDrawLine(plotX, plotY, arrowrendX, arrowrendY);

//		rectP.topLeft.x = plotX-1;
//		rectP.topLeft.y = plotY-1;
//		rectP.extent.x = 2;
//		rectP.extent.y = 2;
//		WinDrawRectangle(&rectP, 0);
//		WinDrawCircle(plotX, plotY, 2, SOLID);
//  		if (device.romVersion < SYS_VER_35) {
//			WinDrawLine(plotX+1, plotY+1, plotX+1, plotY+1);
//			WinDrawLine(plotX-1, plotY-1, plotX-1, plotY-1);
//			WinDrawLine(plotX-1, plotY+1, plotX-1, plotY+1);
//			WinDrawLine(plotX+1, plotY-1, plotX+1, plotY-1);
//		} else {
//			WinDrawPixel(plotX+1, plotY+1);
//			WinDrawPixel(plotX-1, plotY-1);
//			WinDrawPixel(plotX-1, plotY+1);
//			WinDrawPixel(plotX+1, plotY-1);
//		}
//		WinSetPattern(&pattern);
//		WinFillLine(plotX, plotY, oppplotX, oppplotY);
//		StrCopy(tempchar, print_direction2(data.input.bearing_to_destination.value));
//		WinDrawInvertedChars(tempchar, StrLen(tempchar), plotX, plotY);
	}

	// draw second arrow for bearing to next waypoint if distance is valid and meets criteria
	if (((data.config.shownextwpt == NEXTALL) || ((data.config.shownextwpt == NEXTCTL) && data.input.isctlpt)) && (data.input.nextwpt_dist >= 0.0)) {
		bearing = data.input.nextwpt_bear;
		xrat = xrat / sf;
		yrat = yrat / sf;
		centerX = 150;
		centerY = 135;
		RangeBearingToPixel(xrat, yrat, centerX, centerY, 0.45, bearing, NOFORCE, TRACKUP, &plotX, &plotY);
		RangeBearingToPixel(xrat, yrat, centerX, centerY, 0.45, RecipCse(bearing), NOFORCE, TRACKUP, &oppplotX, &oppplotY);
		RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(bearing - 150.0), NOFORCE, TRACKUP, &arrowlendX, &arrowlendY);
		RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(bearing + 150.0), NOFORCE, TRACKUP, &arrowrendX, &arrowrendY);
		WinDrawLine(plotX, plotY, oppplotX, oppplotY);
		WinDrawLine(plotX, plotY, arrowlendX, arrowlendY);
		WinDrawLine(plotX, plotY, arrowrendX, arrowrendY);
	}
}

void cuwaypoint_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	Int16  wayidx = 0;
	UInt16 strnglen;
	static Int16 next = 0;
	Char tempchar[PARSELEN];
	WaypointData waypoint;
	static Boolean skip = false;
	static Boolean keepparsing = true;
	static Boolean HOME_set = false;
	static Boolean lineoverflow = false;
	// Needed for AREA turnpoint parsing
//	Char *tempcharptr=NULL;
//	Boolean retval;

	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		HOME_set = false;
		keepparsing = true;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		return;
	}

	if (keepparsing == false) {
		return;
	}

//	HostTraceOutputTL(appErrorClass, "serinp:|%s|", serinp);
	MemSet(&waypoint, sizeof(WaypointData), 0);
	StrCopy(rxtype, "Waypoints");

	// skip first line (should be header info)
//	skip = true; // now covered by check for quotes as the first character

	while (cur<length) {
		while( (cur<length) && (next<PARSELEN) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		buf[next] = '\0';
//		HostTraceOutputTL(appErrorClass, "buf:|%s|", buf);

		if (buf[0] == '*') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
		}

		if (buf[0] == '-') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
			keepparsing = false;
		}

		if (buf[0] != '"') {
			// Force names to be in quotes
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
//			skip = true;
		}

		if (next >= PARSELEN) {
//			HostTraceOutputTL(appErrorClass, "Parsing error->PARSELEN Chars");
			/* Parsing error start over */
			lineoverflow = true;
			next=0;
		}

		// check first line for field names
		GetField(buf, 0, tempchar);
		ConvertToUpper(tempchar);
		if ((StrCompare(tempchar, "NAME") == 0) || (StrCompare(tempchar, "TITLE") == 0)) {
			skip = true;
		}
		
		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

			if (cur <= length && skip == false) {

				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else {
					//	Getting waypoint remarks
					GetField(buf, 0, tempchar);
	//				HostTraceOutputTL(appErrorClass, "Wpt Rmks 1: %s", tempchar);
					GetFieldDelim(tempchar, 1, '"', '"', tempchar);
	//				HostTraceOutputTL(appErrorClass, "Wpt Rmks 2: %s", tempchar);
					StrNCopy(waypoint.rmks, trim(NoComma(tempchar," "), ' ', true), 12);

					//	Getting waypoint name
					GetField(buf, 1, tempchar);
					GetFieldDelim(tempchar, 1, '"', '"', tempchar);
					StrNCopy(waypoint.name, trim(NoComma(tempchar," "), ' ', true), 12);

					// swap name and remarks
					if (data.config.wpformat == WPCUN) {
						StrCopy(tempchar, waypoint.name);
						StrCopy(waypoint.name, waypoint.rmks);
						StrCopy(waypoint.rmks, tempchar);
					}
					//  if blank name (code), use remarks (name)
					if (StrLen(waypoint.name) == 0) StrCopy(waypoint.name, waypoint.rmks);
	//				HostTraceOutputTL(appErrorClass, "Wpt Name : %s", tempchar);

					//	Getting waypoint latitude
					GetField(buf, 3, tempchar);
					waypoint.lat = DegMinStringToLatLon(tempchar, NULL);
	//				HostTraceOutputTL(appErrorClass, "Lat : %s", tempchar);

					//	Getting waypoint longitude
					GetField(buf, 4, tempchar);
					waypoint.lon = DegMinStringToLatLon(tempchar, NULL);
	//				HostTraceOutputTL(appErrorClass, "Lon : %s", tempchar);

					//	Getting waypoint elevation
					GetField(buf, 5, tempchar);
					strnglen = StrLen(tempchar);
					if (strnglen > 1) {
						if (StrChr(tempchar, 'm')) {
							tempchar[strnglen-1] = '\0';
							waypoint.elevation = StrToDbl(tempchar)/ALTMETCONST;
						} else {
							tempchar[strnglen-1] = '\0';
							tempchar[strnglen-2] = '\0';
							waypoint.elevation = StrToDbl(tempchar);
						}
					} else {
						waypoint.elevation = 0.0;
					}

					//	Getting waypoint attribute
					GetField(buf, 6, tempchar);
					waypoint.type = NULL;
					// Waypoint
					if (StrChr(tempchar, '1')) waypoint.type|= TURN;
					// Airport (Grass)
					else if (StrChr(tempchar, '2')) waypoint.type|= AIRPORT;
					// Outlanding place
					else if (StrChr(tempchar, '3')) waypoint.type|= LAND;
					// Glider site
					else if (StrChr(tempchar, '4')) waypoint.type|= AIRPORT;
					// Airport (Paved)
					else if (StrChr(tempchar, '5')) waypoint.type|= AIRPORT;
					// Mountain Pass
					else if (StrChr(tempchar, '6')) waypoint.type|= MARK;
					// Mountain Top
					else if (StrChr(tempchar, '7')) waypoint.type|= MARK;
					// Others
					else waypoint.type|= TURN;

					if ((waypoint.type & AIRPORT) || (waypoint.type & LAND)) waypoint.type |= AIRLAND;

					//	Getting runway direction
					if (GetField(buf, 7, tempchar)) {
	//					StrNCopy(waypoint.rmks, trim(tempchar, ' ', true), 3);
	//					StrNCat(waypoint.rmks, "/", 13);
	//					waypoint.rmks[4] = '\0';
						waypoint.rwdir = StrAToI(tempchar);
					}

					// Getting runway length (ie 1200 = 12)
					// Gets converted to feet
					if (GetField(buf, 8, tempchar)) {
						strnglen = StrLen(tempchar);
						if (strnglen > 1) {
							if (StrChr(tempchar, 'm')) {
								if (StrChr(tempchar, 'n')) {
									// This would be Nautical Miles (nm)
									tempchar[strnglen-1] = '\0';
									tempchar[strnglen-2] = '\0';
									waypoint.rwlen = StrToDbl(tempchar) * 6076.131;
								} else if (StrChr(tempchar, 'l')) {
									// This would be miles (ml)
									tempchar[strnglen-1] = '\0';
									tempchar[strnglen-2] = '\0';
									waypoint.rwlen = StrToDbl(tempchar) * 5280.0;
								} else {
									// This would be meters(m)
									tempchar[strnglen-1] = '\0';
									waypoint.rwlen = StrToDbl(tempchar)/ALTMETCONST;
								}
							} else {
								// This would be feet (ft)
								tempchar[strnglen-1] = '\0';
								tempchar[strnglen-2] = '\0';
								waypoint.rwlen = StrToDbl(tempchar);
							}
						} else {
							waypoint.rwlen = 0.0;
						}

	//					MemSet(&tempchar2, sizeof(tempchar2), 0);
	//					StrCopy(tempchar2, DblToStr(waypoint.rwlen, 2));
	//					tempchar2[2] = '\0';
	//					if (StrLen(waypoint.rmks) == 0) {
	//						StrNCopy(waypoint.rmks, tempchar2, 12);
	//					} else {
	//						StrNCat(waypoint.rmks, tempchar2, 13);
	//					}
	//					StrNCat(waypoint.rmks, " ", 13);
					}

					//	Getting radio frequency
					if (GetField(buf, 9, tempchar)) {
						GetFieldDelim(tempchar, 1, '"', '"', tempchar);
						StrNCopy(waypoint.freq, trim(tempchar, ' ', true), 10);

	//					GetFieldDelim(tempchar, 1, '"', '"', tempchar);
	//					tempchar[0] = ' ';
	//					StrNCopy(tempchar2, trim(tempchar, ' ', true), 5);
	//					if (StrLen(waypoint.rmks) == 0) {
	//						StrNCopy(waypoint.rmks, tempchar2, 12);
	//					} else {
	//						StrNCat(waypoint.rmks, tempchar2, 13);
	//					}
					}

					//	Getting Description
					if (GetField(buf, 10, tempchar)) {
	//				HostTraceOutputTL(appErrorClass, "Geninfo %s", tempchar);
						GetFieldDelim(tempchar, 1, '"', '"', tempchar);
						StrNCopy(waypoint.geninfo, trim(tempchar, ' ', true), 63);

	//					if (StrLen(waypoint.rmks) < 12) {
	//						if (StrLen(waypoint.rmks) == 0) {
	//							StrNCopy(waypoint.rmks, trim(tempchar, ' ', true), 12);
	//						} else {
	//							StrNCat(waypoint.rmks, trim(tempchar, ' ', true), 13);
	//						}
	//					}
					}

	/*
					//	Getting the area turnpoint info if it is available
					retval = GetField(buf, 7, tempchar);
					if (retval) {
						MemSet(&tempchar2, sizeof(tempchar2), 0);
	//					HostTraceOutputTL(appErrorClass, "tempchar:|%s|", tempchar);
						StrNCopy(tempchar2, tempchar, 4);
	//					HostTraceOutputTL(appErrorClass, "tempchar2:|%s|", tempchar2);
						waypoint.arearadius = StrToDbl(tempchar2)/10.0;
					// need to add arearadius2 if this code is used
						MemSet(&tempchar2, sizeof(tempchar2), 0);
						tempcharptr = &tempchar[12];
	//					HostTraceOutputTL(appErrorClass, "1-tempcharptr:|%s|", tempcharptr);
						StrNCopy(tempchar2, tempcharptr, 3);
	//					HostTraceOutputTL(appErrorClass, "1-tempchar2:|%s|", tempchar2);
						waypoint.arearadial1 = (Int16)StrAToI(tempchar2);
	//					HostTraceOutputTL(appErrorClass, "1-waypoint.arearadial1:|%hd|", waypoint.arearadial1);

						MemSet(&tempchar2, sizeof(tempchar2), 0);
						tempcharptr = &tempchar[15];
	//					HostTraceOutputTL(appErrorClass, "2-tempcharptr:|%s|", tempcharptr);
						StrNCopy(tempchar2, tempcharptr, 3);
	//					HostTraceOutputTL(appErrorClass, "2-tempchar2:|%s|", tempchar2);
						waypoint.arearadial2 = (Int16)StrAToI(tempchar2);
	//					HostTraceOutputTL(appErrorClass, "2-waypoint.arearadial2:|%hd|", waypoint.arearadial2);

						waypoint.type|= AREA;
					} else {
	//					HostTraceOutputTL(appErrorClass, "arearadius not found, setting to 0.0");
						waypoint.arearadius = 0.0;
						waypoint.arearadius2 = 0.0;
						waypoint.arearadial1 = 0;
						waypoint.arearadial2 = 0;
					}
	*/
					// setting the last used time stamp
					waypoint.UsedTime[0] = '\0';

					if ((StrLen(waypoint.name) > 0) && (waypoint.lat != INVALID_LAT_LON) && (waypoint.lon != INVALID_LAT_LON)) {
	//					HostTraceOutputTL(appErrorClass, "Accepting %s", DblToStr(wayidx,0));
						wayidx = OpenDBAddRecord(waypoint_db, sizeof(WaypointData), &waypoint, false);
						rxrecs = wayidx;
						if (FrmGetActiveFormID() == form_transfer) {
							// update record counter on transferscreen
							field_set_value(form_transfer_records, DblToStr(wayidx+1,0));
						}
	//					HostTraceOutputTL(appErrorClass, "Accepting %s", DblToStr(wayidx,0));
					} else {
	//					HostTraceOutputTL(appErrorClass, "Rejecting %s",waypoint.name);
					}
				}
			}
			next=0;
		}
//		HostTraceOutputTL(appErrorClass, "Setting skip to false");
		skip = false;
//		HostTraceOutputTL(appErrorClass, "============================================================");
	}
//	HostTraceOutputTL(appErrorClass, "-------------------------------------------------------------");
	return;
}

void GPWPLwaypoint_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	Int16  wayidx = -1;
	static Int16 next = 0;
	Char tempchar[PARSELEN];
	Char tempchar2[35];
	WaypointData waypoint;
	static Boolean skip = false;
	static Boolean keepparsing = true;
	static char firstwp[13]= "";
	EventType newEvent;
	static Boolean lineoverflow = false;
	
	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		keepparsing = true;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		StrCopy(firstwp, "");
		return;
	}

	if (keepparsing == false) {
		return;
	}

//	HostTraceOutputTL(appErrorClass, "serinp:|%s|", serinp);
	MemSet(&waypoint, sizeof(WaypointData), 0);
	StrCopy(rxtype, "Waypoints");

	while (cur<length) {
		while( (cur<length) && (next<80) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		buf[next] = '\0';
//		HostTraceOutputTL(appErrorClass, "buf:|%s|", buf);

		if (buf[0] == '*') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
		}

		if (buf[0] != '$') {
			// skip if not begining with $
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
		}

		if (next >= PARSELEN) {
//			HostTraceOutputTL(appErrorClass, "Parsing error->PARSELEN Chars");
			/* Parsing error start over */
			lineoverflow = true;
			next=0;
		}
		
		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

			if (cur <= length && skip == false) {

				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else {
					//	Check for $GPWPL
					GetField(buf, 0, tempchar);
	//				HostTraceOutputTL(appErrorClass, "Temp0 %s", tempchar);
					if (StrCompare(tempchar, "$GPWPL") == 0) {

						//	Getting waypoint lat
						GetField(buf, 1, tempchar);
						StrCopy(tempchar2, Right(tempchar, StrLen(tempchar)-2));
	//					HostTraceOutputTL(appErrorClass, "Temp1 %s", tempchar);
						if (StrLen(tempchar) >= 7) {
							//	Getting waypoint lon degrees
							waypoint.lat = StrAToI(Left(tempchar,2));
	//						HostTraceOutputTL(appErrorClass, "Temp1sub %s", tempchar2);
							waypoint.lat += StrToDbl(tempchar2)/60.0;

							//	Getting waypoint lat N/S
							GetField(buf, 2, tempchar);
	//						HostTraceOutputTL(appErrorClass, "Temp2 %s", tempchar);
							if (StrCompare(tempchar, "N") != 0) {
								waypoint.lat = -waypoint.lat;
							}

							// Check latitude
							if (Fabs(waypoint.lat) > 90.0) {
								waypoint.lat = INVALID_LAT_LON;
							}
						} else {
							waypoint.lat = INVALID_LAT_LON;
						}

						//	Getting waypoint lon
						GetField(buf, 3, tempchar);
	//					HostTraceOutputTL(appErrorClass, "Temp3 %s", tempchar);
						StrCopy(tempchar2, Right(tempchar, StrLen(tempchar)-3));
						if (StrLen(tempchar) >= 8) {
							//	Getting waypoint lon degrees
							waypoint.lon = StrAToI(Left(tempchar,3));
	//						HostTraceOutputTL(appErrorClass, "Temp3sub %s", tempchar2);
							waypoint.lon += StrToDbl(tempchar2)/60.0;

							//	Getting Waypoint Lon N/S
							GetField(buf, 4, tempchar);
	//						HostTraceOutputTL(appErrorClass, "Temp4 %s", tempchar);
							if (StrCompare(tempchar, "E") != 0) {
								waypoint.lon = -waypoint.lon;
							}

							// Check longitude
							if (Fabs(waypoint.lon) > 180.0) {
								waypoint.lon = INVALID_LAT_LON;
							}
						} else {
							waypoint.lon = INVALID_LAT_LON;
						}

						//	Getting waypoint name
						GetField(buf, 5, tempchar);
	//					HostTraceOutputTL(appErrorClass, "Temp5 %s", tempchar);
						StrNCopy(waypoint.name, Left(trim(NoComma(tempchar," "),' ',true),6), 12);

						// altitude
						if ((data.config.wpformat == WPGPWPLALT) && StrChr("0123456789",tempchar[4]) && StrChr("0123456789",tempchar[5]) && StrChr("0123456789",tempchar[6])) {
							// decode altitude if in NNNXXX format with XXX = meters / 10
							waypoint.elevation = pround((double)StrAToI(Mid(tempchar, 3, 4)) * 10 / ALTMETCONST, 0);
						}

						// remarks
						if (StrLen(tempchar) > 7) {
	//						HostTraceOutputTL(appErrorClass, "Rmks %s", tempchar);
							StrNCopy(waypoint.rmks, trim(Mid(tempchar, 12, 8),' ',true), 12);
						}
					} else {
						waypoint.lat = INVALID_LAT_LON;
						waypoint.lon = INVALID_LAT_LON;
					}

					// setting the last used time stamp
					waypoint.UsedTime[0] = '\0';

					// Check data
	//				HostTraceOutputTL(appErrorClass, "Name %s", waypoint.name);
	//				HostTraceOutputTL(appErrorClass, "Lat %s", DblToStr(waypoint.lat,3));
	//				HostTraceOutputTL(appErrorClass, "Lon %s", DblToStr(waypoint.lon,3));
					if ((StrLen(firstwp) == 0) && (StrLen(waypoint.name) > 0)) {
						// set first waypoint received
						StrCopy(firstwp, waypoint.name);
					} else if (StrCompare(waypoint.name, firstwp) == 0) {
						// stop parsing when first waypoint repeated
	//					HostTraceOutputTL(appErrorClass, "Repeated Waypoint %s", waypoint.name);
						keepparsing = false;
						if ((data.config.xfertype != USEVFS) && (data.config.xfertype != USEDOC) && (data.config.xfertype != USEMEMO)) {
							// insert event to simulate "Stop" button being pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_transfer_recvbtn;
							EvtAddEventToQueue(&newEvent);
						}
					}
					if ((keepparsing) && (StrLen(waypoint.name) > 0) && (waypoint.lat != INVALID_LAT_LON) && (waypoint.lon != INVALID_LAT_LON)) {
	//					HostTraceOutputTL(appErrorClass, "Accepting %s", DblToStr(wayidx,0));
						wayidx = OpenDBAddRecord(waypoint_db, sizeof(WaypointData), &waypoint, false);
						rxrecs = wayidx;
						if (FrmGetActiveFormID() == form_transfer) {
							// update record counter on transferscreen
							field_set_value(form_transfer_records, DblToStr(wayidx+1,0));
						}
					} else {
	//					HostTraceOutputTL(appErrorClass, "Rejecting %s",waypoint.name);
					}
				}
			}
			next=0;
		}
//		HostTraceOutputTL(appErrorClass, "Setting skip to false");
		skip = false;
//		HostTraceOutputTL(appErrorClass, "============================================================");
	}
//	HostTraceOutputTL(appErrorClass, "-------------------------------------------------------------");
	return;
}

void draw_waypoints(double gliderLat, double gliderLon, double mapscale, double maxdist, Boolean gliderpos)
{
	Int16 wayptidx;
	Boolean querygood;
	Int16 plotX, plotY;
	MemHandle output_hand;
	MemPtr output_ptr;
	WaypointData waypoint;
	Boolean plotgood;
	double poirange, poibearing;
	double WaySTF;
	double tmpalt;

//	HostTraceOutputTL(appErrorClass, "Drawing Waypoints");
	for (wayptidx=0; wayptidx < (Int16)DmNumRecords(waypoint_db); wayptidx++) {
			querygood = OpenDBQueryRecord(waypoint_db, wayptidx, &output_hand, &output_ptr);
			MemMove(&waypoint, output_ptr, sizeof(WaypointData));
			MemHandleUnlock(output_hand);

			plotgood = CalcPlotValues(gliderLat, gliderLon, waypoint.lat, waypoint.lon, xratio, yratio, &plotX, &plotY, &poirange, &poibearing, NOFORCE, data.input.curmaporient, true);

			if (plotgood) {
				//if ((plotX <= SCREEN.WIDTH) && (plotX >= 0) && (plotY <= SCREEN.HEIGHT) && (plotY >= 0)) {
				/* Plot each POI on the screen */
					if (poirange < maxdist) { 
						if (data.config.inrngcalc) {
							if (gliderpos) {
								CalcSTFSpdAlt2(MCCurVal, poirange, poibearing, &WaySTF, &tmpalt);
							} else {
								CalcSTFSpdAlt2(MCCurVal, waypoint.distance, waypoint.bearing, &WaySTF, &tmpalt);
							}
							waypoint.alt = ConvertAltType(waypoint.elevation, data.input.inusealt, true, REQALT, tmpalt);

							if (waypoint.alt <= data.input.inusealt) {
								DrawPOI(plotX, plotY, waypoint.name, (SCREEN.SRES*3), data.config.waymaxlen, true, false, waypoint.type, false, mapscale,
									(waypoint.type & AIRPORT), (waypoint.type & AIRLAND), (waypoint.alt+data.config.safealt > data.input.inusealt));
							} else {
								DrawPOI(plotX, plotY, waypoint.name, (SCREEN.SRES*3), data.config.waymaxlen, false, false, waypoint.type, false, mapscale, false, false, false);
							}
						} else {
							DrawPOI(plotX, plotY, waypoint.name, (SCREEN.SRES*3), data.config.waymaxlen, true, false, waypoint.type,false, mapscale, (waypoint.type & AIRPORT), (waypoint.type & AIRLAND), false);
						}
					} else {
						DrawPOI(plotX, plotY, waypoint.name, (SCREEN.SRES*3), data.config.waymaxlen, false, false, waypoint.type, false, mapscale, false, false, false);
					}
				//}
			}
		//}
	}
	return;
}

void OZIwaypoint_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	Int16  wayidx = 0;
	static Int16 next = 0;
	Char tempchar[PARSELEN];
	static WaypointData waypoint;
	static Boolean skip = false;
	static Boolean HOME_set = false;
	Boolean dbempty = true;
	static Boolean lineoverflow = false;
	
	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		HOME_set = false;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		return;
	}

	// Check to see if there are any waypoints already in the database
	if (OpenDBCountRecords(waypoint_db)) {
		dbempty = false;
	}

//	HostTraceOutputTL(appErrorClass, "serinp:|%s|", serinp);
	MemSet(&waypoint, sizeof(WaypointData), 0);
	StrCopy(rxtype, "Waypoints");

	while (cur<length) {
		while( (cur<length) && (next<PARSELEN) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		buf[next] = '\0';
//		HostTraceOutputTL(appErrorClass, "buf:|%s|", buf);

		if (buf[0] == '*') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
		}

		if (next >= PARSELEN) {
//			HostTraceOutputTL(appErrorClass, "Parsing error->PARSELEN Chars");
			/* Parsing error start over */
			lineoverflow = true;
			next=0;
		}

		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

			// check for header (no comma in line)
			if (StrStr(buf, ",") == NULL) {
//				HostTraceOutputTL(appErrorClass, "Setting skip to true");
				skip = true;
			}

			if (cur <= length && skip == false) {

				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else {
					//	Getting waypoint name
					GetFieldDelim(buf, 1, ',', ',', tempchar);
					StrNCopy(waypoint.name, trim(NoComma(tempchar," "), ' ', true), 12);

					//	Getting waypoint latitude
					GetField(buf, 2, tempchar);
					waypoint.lat = StrToDbl(tempchar);

					//	Getting waypoint longitude
					GetField(buf, 3, tempchar);
					waypoint.lon = StrToDbl(tempchar);

					//	Getting waypoint remarks
					GetField(buf, 10, tempchar);
					StrNCopy(waypoint.rmks, trim(tempchar, ' ', true), 12);

					//	Getting waypoint elevation
					GetField(buf, 14, tempchar);
					waypoint.elevation = StrToDbl(tempchar);

					// setting the last used time stamp
					waypoint.UsedTime[0] = '\0';

					// check waypoint
					if ((StrLen(waypoint.name) > 0) && (waypoint.lat != INVALID_LAT_LON) && (waypoint.lon != INVALID_LAT_LON)) {
	//						HostTraceOutputTL(appErrorClass, "Accepting %s %s",waypoint.name, DblToStr(wayidx,0));
						wayidx = OpenDBAddRecord(waypoint_db, sizeof(WaypointData), &waypoint, false);
						rxrecs = wayidx;
						if (FrmGetActiveFormID() == form_transfer) {
							// update record counter on transferscreen
							field_set_value(form_transfer_records, DblToStr(wayidx+1,0));
						}
					} else {
	//						HostTraceOutputTL(appErrorClass, "Rejecting %s %s %s %s",waypoint.name,waypoint.rmks,DblToStr(waypoint.lat,2),DblToStr(waypoint.lon,2));
					}
				}
			}
			next=0;
		}
//		HostTraceOutputTL(appErrorClass, "Setting skip to false");
		skip = false;
//		HostTraceOutputTL(appErrorClass, "============================================================");
	}
//	HostTraceOutputTL(appErrorClass, "-------------------------------------------------------------");
	return;

	return;
}

void OutputWayOZI()
{
	Char output_char[255];
	Char tempchar[35];
	Int16 wayindex = 0;
	Int16 nrecs;
	MemHandle output_hand;
	MemPtr output_ptr;
	WaypointData *waydata;

	nrecs = OpenDBCountRecords(waypoint_db);

	AllocMem((void *)&waydata, sizeof(WaypointData));

	for (wayindex=0; wayindex<nrecs; wayindex++) {
		if ((wayindex % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, wayindex, "Waypoints");

		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);

		// number
		StrCopy(output_char, leftpad(StrIToA(tempchar, wayindex+1), '0', 4));
		StrCat(output_char, ",");
		// name
		StrCat(output_char, waydata->name);
		StrCat(output_char, ",");
		// lat deg
		StrCat(output_char, DblToStr(waydata->lat,6));
		StrCat(output_char, ",");
		// lon deg
		StrCat(output_char, DblToStr(waydata->lon,6));
		StrCat(output_char, ",");
		// date
		StrCat(output_char, ",");
		// symbols + status + map display format + foreground + background
		StrCat(output_char, "7,1,2,        ,       ,");
		// remarks
		StrCat(output_char, waydata->rmks);
		StrCat(output_char, ",");
		// pointer + garmin display format + proximity
		StrCat(output_char, "0,0,500,");
		// altitude
		StrIToA(tempchar, waydata->elevation);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
		// font size + font style + symbol size + proximity symbol position + proximity time + file attachment + proximity file attachment + proximity symbol name
		StrCat(output_char, "0,0,17,0,10.0,2,,,");

		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	FreeMem((void *)&waydata);
	return;
}

void DeleteThermalWaypts()
{
	UInt16 i;
	MemHandle waypoint_hand;
	MemPtr waypoint_ptr;
	UInt16 nrecs;

 	// clear thermal waypoints
	nrecs = OpenDBCountRecords(waypoint_db);
	if (nrecs > 0)
	for (i = 0; i < nrecs; i++) {
		// get record
		OpenDBQueryRecord(waypoint_db, i, &waypoint_hand, &waypoint_ptr);
		MemMove(TempWpt, waypoint_ptr, sizeof(WaypointData));
		MemHandleUnlock(waypoint_hand);
		// delete if thermal waypoint
		if (TempWpt->type & THRML) {
//			HostTraceOutputTL(appErrorClass, "Delete Thermal Wpt %s", TempWpt->name);
			OpenDBDeleteRecord(waypoint_db, i);
			nrecs--;
			i--;
		}
	}
	return;
}

