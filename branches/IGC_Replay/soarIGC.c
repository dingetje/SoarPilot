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

static t_index pnts=0;	/* no of points in track */
static t_point *pointlist = 0;	/* start point of track list */

// cleanup old list if we have one
static void cleanUpList()
{
	struct _point *pCleanup = 0;
	while(pointlist)
	{
		pCleanup = pointlist;
		pointlist = pointlist->next;
		FreeMem((void *) pCleanup);
	}
	pnts = 0;
}

// Add a point to the list
/* static void addPoint(double lat, double lon, double alt, double speed, double dist, int seconds)
{
	static t_point *last = 0;
	t_point *neu = 0;
	AllocMem((void *)&neu, sizeof(t_point));
	MemSet(neu, sizeof(t_point), 0);
	neu->lat = lat;
	neu->lon = lon;
	neu->alt = alt;
	neu->speed = speed;
	neu->dist = dist;
	neu->seconds = seconds;
	neu->next = 0;
	// 1st time here?
	if (!pointlist)
	{
		// remember start of the points list
		pointlist = neu;
	}
	else
	{
		last->next = neu;
	}
	last = neu;
	pnts++;
} 
 */
void igc_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	Int16  igcidx = 0;
	static Int16 next = 0;
//	Char tempchar[PARSELEN];
	static Boolean skip = false;
	static Boolean lineoverflow = false;
	
	if (reset) {
		HostTraceOutputTL(appErrorClass, " IGC resetting");
		next = 0;
		skip = false;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		cleanUpList();
		return;
	}

	StrCopy(rxtype, "Flights");

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
				//	TODO parse B record			
					HostTraceOutputTL(appErrorClass, buf);
					igcidx++;
					if (FrmGetActiveFormID() == form_transfer) {
						// update record counter on transferscreen
						field_set_value(form_transfer_records, DblToStr(igcidx+1,0));
					}
//					HostTraceOutputTL(appErrorClass, "polaridx |%hd|", polaridx);
				}
			}
			next=0;
		}
		skip = false;
	}
	return;
}
