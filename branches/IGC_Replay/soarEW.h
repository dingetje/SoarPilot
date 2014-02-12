#ifndef SOAREW_H
#define SOAREW_H

#define SCMP2 __attribute__ ((section ("scmp2")))
#define SCMP3 __attribute__ ((section ("scmp3")))

#define EWspeed 3

#define EWSYNCLAST 0
#define EWSYNCALL 1

typedef struct EWMRData {
	Char	modelname[26];
	Char	serialnumber[11];
	Char	firmwarever[11];
	Char	hardwarever[11];
	Char	secstatus[11];
	Char	enlfitted[2];
	Char    debuglevel[11];
	Int16	updaterate; //seconds
	UInt16	datarate; //baud - readonly
	Int16	aotime; //Time in minutes before unit shuts off if no alt or pos chg
	Int16	aoaltchg; //Altitude change in meters to keep unit from shutting off
	Int16	aospdchg; //Velocity change in kph to keep unit from shutting off
	Int16	startfilespd; //Speed in kph at which logging starts
	Int16	startfilepaltchg; //Pressure altitude change in meters at which logging starts
	Boolean	filesync; //Transfer ALL files (0) or LAST file (1)
	Char	pilotname[26];
	Char	cid[6];
	Char	atype[16];
	Char	gid[11];
	Boolean	pilotinfo;
	Boolean	gliderinfo;
} EWMRData;

/*****************************************************************************
 * prototypes
 *****************************************************************************/
Boolean SendEWPreamble() SCMP2;
Boolean SendEWSPI(Boolean blankdate) SCMP2;
Boolean ClearEWWaypoints() SCMP2;
Boolean SendEWWaypoint(UInt16 ewidx, UInt16 wayidx) SCMP2;
Boolean DeclareTaskToEW() SCMP2;
Boolean ClearEWDeclaration() SCMP2;
Boolean form_config_ewmrinst_event_handler(EventPtr event) SCMP2;
Boolean SendEWMRCommsStart(Boolean skipctrlc) SCMP3;
Boolean SendEWMRDownloadStart() SCMP3;
Boolean SendEWMRDownloadAbort() SCMP3;
Boolean SendEWMRCommsEnd() SCMP3;
Boolean GetEWMRConfig() SCMP3;
Boolean SendConfigToEWMR() SCMP3;
Boolean SendEWMRConfig(Boolean clearlogs) SCMP3;
Boolean SendEWMRTask(Boolean cleartask) SCMP3;
Boolean SendEWMRTaskpoint(UInt16 wayidx, Char *output_char) SCMP3;
Boolean DeclareTaskToEWMR(Boolean declaretoSD) SCMP3;
Boolean ClearEWMRTask(Boolean declaretoSD) SCMP3;
Boolean ClearEWMRLogs() SCMP3;

#endif
