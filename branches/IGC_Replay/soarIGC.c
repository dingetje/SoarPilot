#include <PalmOS.h>
#include "soaring.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarDB.h"
#include "soarIO.h"
#include "soarIGC.h"
#include "soarMath.h"

extern Int32  rxrecs;
extern char rxtype[15];
extern Char buf[PARSELEN];

void igc_parser(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	Int16  igcidx = 0;
	static Int16 next = 0;
//	Char tempchar[PARSELEN];
	static Boolean skip = false;
	static Boolean lineoverflow = false;
	
	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		MemSet(&buf,sizeof(buf),0);
		rxrecs = 0;
		StrCopy(rxtype, "Records");
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

		if (buf[0] == '*') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
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
