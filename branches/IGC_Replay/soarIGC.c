#include <PalmOS.h>
#include <MemoryMgr.h>
#include "soaring.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarDB.h"
#include "soarIO.h"
#include "soarIGC.h"
#include "soarMath.h"
#include "soarUtil.h"
#include "soarMem.h"
#include "Mathlib.h"

extern Int32  rxrecs;
extern char rxtype[15];
extern Char buf[PARSELEN];
static Int16  igcidx = 0;

/*
* convert IGC string into numeric latitude or longitude
*/
static double getlatlon(char *str)
{
	double latlon;
	str[7] = '\0';
	latlon = StrToDbl(&(str[2]))/(double)60000.0;
	str[2] = '\0';
	latlon += StrToDbl(str);
	return latlon;
}

static double deg2rad(double deg)
{
   return (deg * 0.01745329251 /*(PI / 180.0)*/);
}

static double get_heading(double lat1, double lon1, double lat2, double lon2)
{
	// from
	double fLat = deg2rad(lat2);
	double fLon = deg2rad(lon2);
	// to
	double tLat = deg2rad(lat1);
	double tLon = deg2rad(lon1);
	double degree = radToDeg * (atan2(sin(tLon-fLon)*cos(tLat), cos(fLat)*sin(tLat)-sin(fLat)*cos(tLat)*cos(tLon-fLon)));
	if (degree < 0)
	{
		degree += 360.0;
	}
	return degree;
} 

/*
* Calculate haversine distance between points in meters
*/
static double calcdistance(double lat1, double lon1, double lat2, double lon2)
{
	/* FAI Earth radius in meters */
	static double d_fak = 6371000.0;
	double sinlon, sinlat, latx, laty, lonx, lony;
	latx = deg2rad(lat1); lonx = deg2rad(lon1);
	laty = deg2rad(lat2); lony = deg2rad(lon2);

	sinlat = sin((latx-laty)/2.0);
	sinlon = sin((lonx-lony)/2.0);
	
	return 2.0*asin(Sqrt( sinlat*sinlat + sinlon*sinlon * cos(latx)*cos(laty)))*d_fak;
} 

void igc_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	static Int16 next = 0;
//	Char tempchar[PARSELEN];
	static Boolean skip = false;
	static Boolean lineoverflow = false;
	double	signlat = 1.0,
			signlon = 1.0,
			dist=0.0;
	static double lastlat = 0.0,
				  lastlon = 0.0;
	static UInt32 lastseconds=0;
	UInt32	deltaseconds=0;
	Int32	hours=0,
			minutes=0,
			seconds=0;
	
	SimPoint simpoint;
	MemSet(&simpoint, sizeof(SimPoint), 0);

	if (reset) {
//		HostTraceOutputTL(appErrorClass, " IGC resetting");
		next = 0;
		skip = false;
		igcidx = 0;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		// for the progress dialog
		StrCopy(rxtype, "B-Records");
		return;
	}
	if (igcidx == 0) {
		// slooooow...when database contains several hundred records
		OpenDBEmpty(sim_db);
	}
	
	while (cur < length) {
		while( (cur < length) && (next < PARSELEN) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		if (next >= PARSELEN) {
			/* Parsing error start over */
			lineoverflow = true;
			next=0;
		}
		buf[next] = '\0';
	
		if (buf[0] != 'B') {
//			HostTraceOutputTL(appErrorClass, "Skipping non B-record");
			skip = true;
		}
		
		if (StrLen(buf) == 0) { // empty line
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			if ((cur <= length) && (skip == false)) {
				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else {				
					// parse B records
					//              1111 1 111 11222 2 2 22222 33333
					// 0 123456 78 90123 4 567 89012 3 4 56789 01234
					// B HHMMSS DD MMMMM N DDD MMMMM E V PPPPP GGGGG CR LF
					//
					// HHMMSS is time stamp
					// DD MMMMM is latitude and longitude
					// PPPPP is pressure altitude
					// GGGGG is GPS altitude
					if (next < 35) continue; // too short
					if (buf[15] != '0') continue;
					if ((buf[14] != 'N') && (buf[14] != 'S')) continue;
					if ((buf[23] != 'E') && (buf[23] != 'W')) continue;
					if (buf[14]=='N') signlat = (double)1.0;
					if (buf[14]=='S') signlat = (double)(-1.0);
					if (buf[23]=='W') signlon = (double)(-1.0);
					if (buf[23]=='E') signlon = (double)1.0;
					// split buffer up in separate strings
					buf[14] = '\0'; buf[23] = '\0'; buf[35] = '\0';
					// get GPS altitude in meter
					simpoint.alt=StrToDbl(&buf[30]); 
					// latitude and longitude
					simpoint.lat = signlat * getlatlon(&buf[7]);
					simpoint.lon = signlon * getlatlon(&buf[16]);
					// get timestamp
					buf[7] = '\0';	seconds = StrAToI(&buf[5]);
					buf[5] = '\0';	minutes = StrAToI(&buf[3]);
					buf[3] = '\0';	hours = StrAToI(&buf[1]);
					simpoint.seconds = (seconds + 60*minutes + 3600*hours);
//					HostTraceOutputTL(appErrorClass, "h=%ld, m=%ld, s=%ld, simpoint.seconds=%lu",hours,minutes,seconds,simpoint.seconds);

					// first time here?
					if (!lastseconds)
					{
						lastseconds = simpoint.seconds;
						lastlat = simpoint.lat; lastlon = simpoint.lon;
					}					
					dist = calcdistance(lastlat,lastlon,simpoint.lat,simpoint.lon);
					deltaseconds = simpoint.seconds-lastseconds;
					simpoint.speed = (deltaseconds!=0) ? (dist*3.6)/deltaseconds : 0.0; /* in km/h */
					simpoint.heading = get_heading(simpoint.lat,simpoint.lon,lastlat,lastlon);
					
//					HostTraceOutputTL(appErrorClass, "lat=%s",DblToStr(simpoint.lat,5));
//					HostTraceOutputTL(appErrorClass, "lon=%s",DblToStr(simpoint.lon,5));
//					HostTraceOutputTL(appErrorClass, "dist=%s, deltaseconds=%lu, lastseconds=%lu",DblToStr(dist,5),deltaseconds,lastseconds);
//					HostTraceOutputTL(appErrorClass, "speed=%s",DblToStr(simpoint.speed,5));
					
					rxrecs++;
					// save in database
					igcidx = OpenDBAddRecord(sim_db, sizeof(SimPoint), &simpoint, false);
					if (FrmGetActiveFormID() == form_transfer) {
						// update record counter on transfer screen
						field_set_value(form_transfer_records, DblToStr(igcidx+1,0));
					}
					lastseconds = simpoint.seconds;
					lastlat = simpoint.lat; lastlon = simpoint.lon;
//					HostTraceOutputTL(appErrorClass, "igcidx |%hd|", igcidx);
				}
			}
			next=0;
		}
		skip = false;
	}
	return;
}
