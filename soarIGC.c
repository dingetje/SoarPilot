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

void igc_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	static Int16 next = 0;
//	Char tempchar[PARSELEN];
	static Boolean skip = false;
	static Boolean lineoverflow = false;
	double	signlat = 1.0,
			signlon = 1.0;
	int		hours=0,
			minutes=0,
			seconds=0;
	
	SimPoint simpoint;
	MemSet(&simpoint, sizeof(SimPoint), 0);

	if (reset) {
		HostTraceOutputTL(appErrorClass, " IGC resetting");
		next = 0;
		skip = false;
		igcidx = 0;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		OpenDBEmpty(sim_db);
		return;
	}

	StrCopy(rxtype, "B-Records");

	while (cur < length) {
		while( (cur < length) && (next < PARSELEN) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		buf[next] = '\0';

		if (buf[0] != 'B') {
			HostTraceOutputTL(appErrorClass, "Skipping non B-record");
			skip = true;
		}

		if (next >= PARSELEN) {
			/* Parsing error start over */
			lineoverflow = true;
			next=0;
		}
		
		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

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
					if (next < 35) continue;
					if (buf[15] != '0') continue;
					if ((buf[14] != 'N') && (buf[14] != 'S')) continue;
					if ((buf[23] != 'E') && (buf[23] != 'W')) continue;
					if (buf[14]=='N') signlat = (double)1.0;
					if (buf[14]=='S') signlat = (double)(-1.0);
					if (buf[23]=='W') signlon = (double)(-1.0);
					if (buf[23]=='E') signlon = (double)1.0;
					buf[14] = '\0'; buf[23] = '\0'; buf[35] = '\0';
					// use GPS alt
					simpoint.alt=StrToDbl(&buf[30]); 
					simpoint.lat = signlat * getlatlon(&buf[7]);
					simpoint.lon = signlon * getlatlon(&buf[16]);
					buf[7] = '\0';	seconds = StrAToI(&buf[5]);
					buf[5] = '\0';	minutes = StrAToI(&buf[3]);
					buf[3] = '\0';	hours = StrAToI(&buf[1]);
					simpoint.seconds = seconds + 60*minutes + 3600*hours;
					HostTraceOutputTL(appErrorClass, "lat=%s",DblToStr(simpoint.lat,2));
					HostTraceOutputTL(appErrorClass, "lon=%s",DblToStr(simpoint.lon,2));
					HostTraceOutputTL(appErrorClass, "alt=%s",DblToStr(simpoint.alt,2));
					
					rxrecs++;
					igcidx = OpenDBAddRecord(sim_db, sizeof(SimPoint), &simpoint, false);
					if (FrmGetActiveFormID() == form_transfer) {
						// update record counter on transfer screen
						field_set_value(form_transfer_records, DblToStr(igcidx+1,0));
					}
//					HostTraceOutputTL(appErrorClass, "igcidx |%hd|", igcidx);
				}
			}
			next=0;
		}
		skip = false;
	}
	return;
}
