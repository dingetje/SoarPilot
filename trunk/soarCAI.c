/**
* \file soarCAI.c
* \brief SoarPilot Cambridge C302, C302A and GPSNAV support
*/

// tab size: 4

#include <PalmOS.h>	// all the system toolbox headers
#include "soaring.h"
#include "soarCAI.h"
#include "soarComp.h"
#include "soaring.h"
#include "soarIO.h"
#include "soarUtil.h"
#include "soarDB.h"
#include "soarPLst.h"
#include "soarForm.h"
#include "soarUMap.h"
#include "soarMem.h"
#include "soarMath.h"
#include "soarWay.h"
#include "soarWLst.h"

/// CAI command mode prompt
#define CAI_COMMAND_PROMPT "\r\n\ncmd>"
/// CAI download mode prompt
#define CAI_DOWNLOAD_PROMPT "\r\n\ndn>"
/// CAI upload mode prompt
#define CAI_UPLOAD_PROMPT "\r\n\nup>"

#define STR2SHORT(s) ((((s)[0]&255)<<8)+((s)[1]&255))

// variables for list of flights in logger

Int16 cainumLogs = 0;				///< number of flights
Int16 currentCAIFltPage = 0;		///< current flight page number
Int16 selectedCAIFltIndex = -1;		///< selected flight index
Int16 CAInumperpage = 8;			///< number of flights per page
CAILogData *cailogdata = NULL;		///< CAI logger data

Int16 previndex = 1;				///< prev. index
Int16 appnumblocks = 0;				///< ???
CAIGenInfo *caigeninfo=NULL;		///< CAI generic info
Boolean skipmc = false;				///< skip MC
Boolean skipbugs = false;			///< skip bugs
Boolean skipballast = false;		///< skip ballast

// externals
extern UInt16 serial_ref;			///< serial port handle
extern UInt16  WayptsortType;		///< waypoint sort type
extern Int8 xfrdialog;				///< transfer dialog
extern Int8 CompCmdRes;				///< return value flight computer command
extern Int16 CompCmdData;			///< flight computer command data
extern Boolean	menuopen;			///< menu open flag

/**
* \brief output a CAI C302 "G" record
* \param item type of G-record
*/
void Output302GRec(Int8 item)
{
	Char output_char[81];
	Char tempchar[20];
	double tmpdbl;

	if (data.input.basemc >= 100.0) {
		tmpdbl = 99.9;
	} else {
		tmpdbl = data.input.basemc;
	}

	// Xmit C302 G Record
	StrCopy(output_char, "!g,");
	switch (item) {
		case GMC:
			StrCat(output_char, "m");
			StrCat(output_char, leftpad(StrIToA(tempchar, (Int32)(pround(tmpdbl, 1)*10.0)), '0', 3));
			StrCat(output_char, ",");
			break;
		case GBALLAST:
			StrCat(output_char, "b");
			StrCat(output_char, StrIToA(tempchar, (Int32)(data.config.pctwater*10.0)));
			break;
		case GBUGS:
			StrCat(output_char, "u");
			StrCat(output_char, leftpad(StrIToA(tempchar, (Int32)((data.config.bugfactor)*100.0)), '0', 3));
			break;
		case GALL:
			StrCat(output_char, "m");
			StrCat(output_char, leftpad(StrIToA(tempchar, (Int32)(pround(tmpdbl, 1)*10.0)), '0', 3));
			StrCat(output_char, ",");
			StrCat(output_char, "b");
			StrCat(output_char, StrIToA(tempchar, (Int32)(data.config.pctwater*10.0)));
			StrCat(output_char, ",");
			StrCat(output_char, "u");
			StrCat(output_char, leftpad(StrIToA(tempchar, (Int32)((data.config.bugfactor)*100.0)), '0', 3));
			break;	
	}
	StrCat(output_char, "\r\n");

#ifdef NMEALOG
	if (device.VFSCapable) TxData(output_char, USEVFS);
#endif	

	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	return;
}


/**
* \brief function to declare task to CAI flight computer
* \return true - task declaration successful, false - task decl. failed
*/
Boolean DeclareCAITask()
{
	UInt16 TAKEOFFSET=0, LANDOFFSET=0;
	UInt16 numWaypts;
	Boolean retval=true;

	numWaypts = data.task.numwaypts - data.task.numctlpts;

	if (data.task.hastakeoff) {
		TAKEOFFSET = 1;
		numWaypts -= 1;
	}
	if (data.task.haslanding) {
		LANDOFFSET = data.task.numwaypts - 1;
		numWaypts -= 1;
	} else {
		LANDOFFSET = data.task.numwaypts;
	}

	HandleWaitDialog(true);

	// Send the CAI Command to put it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to put it into Download Mode
	if (retval) {
		if (!SendCAIDownloadStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send All Turnpoints
//	HostTraceOutputTL(appErrorClass, "Sending Taskpoints");
	if (retval) {
		if (!SendCAITaskpoints()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the C302 Declare End String 
	if (retval) {
		if (!SendCAIDeclareEnd()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	if (retval) {
		if (data.config.flightcomp == C302COMP ||
			 data.config.flightcomp == C302ACOMP) {
			// Send Pilot Info Data to C302
			if (!SendCAIPilotInfo()) {
				HandleWaitDialog(false);
				retval = false;
			}
		} else {
			// Send Pilot & Config Info Data To GPSNAV
			if (!SendGPSNAVConfigInfo()) {
				HandleWaitDialog(false);
				retval = false;
			}
		}
	}

	// Send Glider Info Data
	// Not necessary for the GPSNAV
	if (retval && (data.config.flightcomp == C302COMP || data.config.flightcomp == C302ACOMP)) {
		if (!SendCAIGliderInfo()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Put the C302/CFR into Flight PNP Mode
	// Always want to do this even if error
	SendCAIFlightModeStart(false);

	if (retval)	HandleWaitDialog(false);
//	HostTraceOutputTL(appErrorClass, "------------------------");
	return(retval);
}

/**
* \brief function to put CAI flight computer in command mode
* \return true - successful, else connection error
*/
Boolean SendCAICommandStart()
{
	Char output_char[6];

	// Put C302/CFR into Command Mode (send Ctrl-C)
	StrCopy(output_char, "\003");
	TxData(output_char, USESER);
	// Put it here so that the Serial Flush will take effect
	ClearSerial();
	TxData(output_char, USESER);
	ClearSerial();
	TxData(output_char, USESER);
	// Wait for the Command Mode Prompt
	if (!(WaitFor(CAI_COMMAND_PROMPT, USESER))) {
		CompCmdRes = CONNECTERR;
		return(false);
	}
	return(true);
}

/**
* \brief function to put CAI flight computer in Pocket NAV mode
* \param dialog - true to show dialog
* \return for now always true
*/
Boolean SendCAIFlightModeStart(Boolean dialog)
{
	Char output_char[6];

	if (dialog) {
		// pop up dialog box
		if (data.config.flightcomp == C302COMP || data.config.flightcomp == C302ACOMP) {
			xfrdialog = XFRC302;
		} else {
			xfrdialog = XFRGPSNAV;
		}
		HandleWaitDialog(true);
	}
	
	// Send the CAI Command it into Command Mode
	// AM: wouldn't it be better to check return value?
	SendCAICommandStart();

	if (data.config.flightcomp == C302COMP ||
		 data.config.flightcomp == C302ACOMP) {
//		HostTraceOutputTL(appErrorClass, "Sending CAI PNP Mode Command");
		// Put C302/CFR into PNP Mode
		StrCopy(output_char, "PNP\r");
	} else {
//		HostTraceOutputTL(appErrorClass, "Sending GPSNAV NMEA Mode Command");
		// Put GPSNAV into NMEA Mode
		if (!SendCAICommandStart()) {
			if (!SendCAICommandStart()) {
				SendCAICommandStart();
			}
		}
		StrCopy(output_char, "NMEA\r");
//		PG : repeat just in case?
//		StrCopy(output_char, "NMEA\r");
	}
	TxData(output_char, USESER);

	// close dialog box
	if (dialog) {
		HandleWaitDialog(false);
		xfrdialog = XFRXFR;
	}
	return(true);
}

/**
* \brief function to send flight download command to CAI flight computer
* \return true if download prompt seen, else false
*/
Boolean SendCAIDownloadStart()
{
	Char output_char[6];

//	HostTraceOutputTL(appErrorClass, "Sending CAI Download Command");
	// Xmit the C302 Download command to put it in Download Mode
	StrCopy(output_char, "DOW 1\r");
	TxData(output_char, USESER);
	if (!(WaitFor(CAI_DOWNLOAD_PROMPT, USESER))) {
		return(false);
	}
	return(true);
}

/**
* \brief function to upload data to CAI flight computer
* \param data pointer to data to upload
* \return true if upload prompt seen, else false
*/
Boolean SendCAIUploadStart(CAIData *data)
{
	Char output_char[6];

//	HostTraceOutputTL(appErrorClass, "Sending CAI Upload Command");
	// Xmit the C302 Upload command to put it in Upload Mode
	StrCopy(output_char, "UPL 1\r");
//	StrCopy(output_char, "UPL\r");
	putString(data, output_char);
	if (!(WaitFor(CAI_UPLOAD_PROMPT, USESER))) {
		return(false);
	}
	data->checksum = 0;
	data->longChecksum = 0;
	return(true);
}

/**
* \brief function to send declare end to CAI flight computer
* \return true if download prompt seen, else false
*/
Boolean SendCAIDeclareEnd()
{
	Char output_char[6];

	if (data.config.flightcomp == C302COMP ||
		 data.config.flightcomp == C302ACOMP) {
//		HostTraceOutputTL(appErrorClass, "Sending 302 Declare End");
		StrCopy(output_char, "D,255\r");
	} else {
//		HostTraceOutputTL(appErrorClass, "Sending GPSNAV Declare End");
		StrCopy(output_char, "D,0\r");
	}
	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor(CAI_DOWNLOAD_PROMPT, USESER))) {
		return(false);
	}
	return(true);

}
/**
* \brief function to send task points to CAI flight computer
* \return true if successful, else false
*/
Boolean SendCAITaskpoints()
{
	Int16 caiidx=1, wayidx=0;
	Boolean retval=true;
	Char output_char[81];
	Char tempchar[20];
	UInt16 TAKEOFFSET=0, LANDOFFSET=0;

	if (data.task.hastakeoff) {
		TAKEOFFSET = 1;
	}
	if (data.task.haslanding) {
		LANDOFFSET = data.task.numwaypts - 1;
	} else {
		LANDOFFSET = data.task.numwaypts;
	}

	if (data.config.flightcomp == C302COMP ||
		 data.config.flightcomp == C302ACOMP) {
		caiidx = 0;
	} else {
		if (WayptsortType != SortByNameA) {
			// Put the waypoint db into 0-9, A-Z order
			DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, SortByNameA);
		}
//		HostTraceOutputTL(appErrorClass, "Sending GPSNAV Task-caiidx|%hu| wayidx|%hu|", caiidx, wayidx);
		// Always puts task into the slot 0 (slot A on the GPSNav Display)
		StrCopy(output_char, "T,0");
	}
	for (wayidx=(TAKEOFFSET); wayidx<LANDOFFSET; wayidx++) {
		if ((data.task.waypttypes[wayidx] & CONTROL) == 0) {
			if (data.config.flightcomp == C302COMP ||
				 data.config.flightcomp == C302ACOMP) {
				if (!SendCAITaskpoint(caiidx, wayidx)) {
					retval = false;
					// This gets us out of the loop
					wayidx = LANDOFFSET;
				}
				caiidx++;
			} else {
				caiidx = FindWayptRecordByName(data.task.wayptnames[wayidx]);
				if (caiidx == CAINOTFOUND) {
					CompCmdRes = WPTNOTFOUND;
					CompCmdData = wayidx;
					retval = false;
					// This gets us out of the loop
					wayidx = LANDOFFSET;
				} else {
					StrCat(output_char, ",");
					StrCat(output_char, StrIToA(tempchar, caiidx+1));
				}
			}
		}
	}

	if (data.config.flightcomp == GPSNAVCOMP) {
		StrCat(output_char, "\r");
		TxData(output_char, USESER);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
		if (!(WaitFor(CAI_DOWNLOAD_PROMPT, USESER))) {
			retval = false;
		}
		if (WayptsortType != SortByNameA) {
			// Put the waypoint db back into selected order
			DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, WayptsortType);
		}
	}

	return(retval);
}

/**
* \brief function to send single task point to CAI flight computer
* \param caiidx CAI index
* \param wayidx waypoint index
* \return true if download prompt seen (data understood and accepted), else false
*/
Boolean SendCAITaskpoint(UInt16 caiidx, UInt16 wayidx)
{
	Char output_char[81];
	Char tempchar[20];

	// This is required for the C302
	caiidx += 128;
//	HostTraceOutputTL(appErrorClass, "Sending CAI Waypoint-caiidx|%hu| wayidx|%hu|", caiidx, wayidx);
	StrCopy(output_char, "D,");
	StrCat(output_char, StrIToA(tempchar, caiidx));
	StrCat(output_char, ",");
	LLToStringDM(data.task.wayptlats[wayidx], tempchar, ISLAT, false, true, 4);
	StrCat(output_char, tempchar);
	StrCat(output_char, ",");
	LLToStringDM(data.task.wayptlons[wayidx], tempchar, ISLON, false, true, 4);
	StrCat(output_char, tempchar);
	StrCat(output_char, ",");
	StrCat(output_char, data.task.wayptnames[wayidx]);
	StrCat(output_char, "\r");

	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor(CAI_DOWNLOAD_PROMPT, USESER))) {
		return(false);
	}
	return(true);
}

/**
* \brief helper function to get unit flags for CAI flight computer
* \param caidata pointer to CAIAddData structure
* \return unit settings data
*/
static Int16 unitFlags(CAIAddData *caidata)
{
	return (data.config.lftunits == METRIC ? CAI_UNIT_VARIO_MS : CAI_UNIT_VARIO_KTS)
		| (data.config.altunits == METRIC ? CAI_UNIT_ALT_METERS : CAI_UNIT_ALT_FEET)
		| (caidata->tempunits == CELCIUS ? CAI_UNIT_TEMP_C : CAI_UNIT_TEMP_F)
		| (caidata->barounits == MILLIBARS ? CAI_UNIT_BARO_MILL : CAI_UNIT_BARO_INHG)
		| (data.config.disunits == METRIC ? CAI_UNIT_DIST_KM : (data.config.disunits == NAUTICAL ? CAI_UNIT_DIST_NM : CAI_UNIT_DIST_SM))
		| (data.config.spdunits == METRIC ? CAI_UNIT_SPEED_KPH : (data.config.spdunits == NAUTICAL ? CAI_UNIT_SPEED_KTS : CAI_UNIT_SPEED_MPH));
}

/**
* \brief function to send pilot info to CAI flight computer, uses IGC header settings
* \return true if download prompt seen (data understood and accepted), else false
*/
Boolean SendCAIPilotInfo()
{
	Char output_char[65];
	Char tempchar[65];
	MemHandle record_handle;
	MemPtr record_ptr;
	CAIAddData *caidata;
	
	AllocMem((void *)&caidata, sizeof(CAIAddData));

	/* Retrieve IGC Header Info */
	OpenDBQueryRecord(config_db, CAIINFO_REC, &record_handle, &record_ptr);
	MemMove(caidata, record_ptr, sizeof(CAIAddData));
	MemHandleUnlock(record_handle);

	if (caidata->pilotinfo) {
		// O,NAME,UNITS,TUNITS,SINK,TEFG,DIFF,DATUM,APP,ARR,SLOW,FAST,GAP,SPD,DB,R,UW,MH
		// Sending Pilot Data
//		HostTraceOutputTL(appErrorClass, "Sending Pilot Data");
		StrCopy(output_char, "O,");
		//Output the Pilot Name (24 chars)
		StrNCopy(tempchar, data.igchinfo.name, 24);
		if (StrLen(data.igchinfo.name) < 24) {
			tempchar[StrLen(data.igchinfo.name)] = '\0';
		} else {
			// IGC pilot name too long, truncate
			tempchar[24] = '\0';
		}
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
		
		// Output a "Zero" to overwrite the current Active Pilot
		StrCat(output_char, "0,");

		// Output the Temperature Units Constant
		StrCat(output_char, StrIToA(tempchar, (Int32)caidata->tempunits));
		StrCat(output_char, ",");

		// Output the Sink Tone On/Off Setting
		StrCat(output_char, StrIToA(tempchar, (Int32)caidata->sinktone));
		StrCat(output_char, ",");

		// Output the Total Energy Final Glide Setting
		StrCat(output_char, StrIToA(tempchar, (Int32)caidata->tefg));
		StrCat(output_char, ",");

		// Output the Final Glide Altitude Mode
		StrCat(output_char, StrIToA(tempchar, (Int32)caidata->dalt));
		StrCat(output_char, ",");

		// Output the Datum Mode
		// Hardcoding it to 100 or WGS84
		// It is ignored by IGC approved units
		StrCat(output_char, "100,");
	
		// Output the Approach Radius in meters
		// Value in Nautical miles converted to Km and then to meters
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(caidata->apprad * DISTKMCONST * 1000.0, 0))));
		StrCat(output_char, ",");
	
		// Output the Arrival Radius in meters
		// Value in Nautical miles converted to Km and then to meters
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(caidata->arvrad * DISTKMCONST * 1000.0, 0))));
		StrCat(output_char, ",");

		// Output the Enroute(Slow) Logging Interval in seconds
		StrCat(output_char, StrIToA(tempchar, data.config.slowlogint));
		StrCat(output_char, ",");

		// Output the Close(Fast) Logging Interval in seconds
		StrCat(output_char, StrIToA(tempchar, data.config.fastlogint));
		StrCat(output_char, ",");

		// Output the Time between Flight Logs (Minutes)
		StrCat(output_char, StrIToA(tempchar, (Int32)caidata->tbtwn));
		StrCat(output_char, ",");

		// Output the Minimum Speed to force flight logging (Knots)
		StrCat(output_char, StrIToA(tempchar, (Int32)data.config.logstartspd));
		StrCat(output_char, ",");

		// Output the STF Deadband in M/S
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(caidata->stfdb * AIRMPSCONST, 0))));
		StrCat(output_char, ",");

		// This field sets the Vario type
		StrCat(output_char, StrIToA(tempchar, (Int32)caidata->variotype));
		StrCat(output_char, ",");
	
		// Output the Unit Flags
		StrCat(output_char, StrIToA(tempchar, (Int32)unitFlags(caidata)));
		StrCat(output_char, ",");
	
		// Output the Margin Height(10ths of Meters)
		// This is Safety Height in SoaringPilot
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(data.config.safealt * ALTMETCONST * 10.0, 0))));

		StrCat(output_char, "\r");

		TxData(output_char, USESER);
// PG moved	FreeMem((void *)&caidata);

//		HostTraceOutputTL(appErrorClass, "%s", output_char);
		if (!(WaitFor(CAI_DOWNLOAD_PROMPT, USESER))) {
			return(false);
		}
	}

	FreeMem((void *)&caidata);

	return(true);
}

/**
* \brief function to send configuration to GPSNAV flight computer
* \return true if download prompt seen (data understood and accepted), else false
*/
Boolean SendGPSNAVConfigInfo()
{
	Char output_char[65];
	Char tempchar[65];
	MemHandle record_handle;
	MemPtr record_ptr;
	CAIAddData *caidata;
	
	AllocMem((void *)&caidata, sizeof(CAIAddData));

	/* Retrieve IGC Header Info */
	OpenDBQueryRecord(config_db, CAIINFO_REC, &record_handle, &record_ptr);
	MemMove(caidata, record_ptr, sizeof(CAIAddData));
	MemHandleUnlock(record_handle);

	// set pilot data flag set?
	if (caidata->pilotinfo) {
		// X,units,PILOTID,ACID,DATUM,APP-R,ARR-R,FAST,SLOW,USERCFG,CONFIG1,CONFIG2,MFN
		// Sending Pilot, Glider & Config Data
//		HostTraceOutputTL(appErrorClass, "Sending GPSNAV Config Data");
		StrCopy(output_char, "X,");
		//Output Units
		//Distance, Height, Vertical Speed
		//If NM, set all
		//0	Nautical miles, feet, knots
		//If KM, check Alt and set based on that
		//1	Kilometers, meters, meters/sec
		//2	Kilometers, feet, knots
		//If SM , set all
		//3	Statute miles, feet, knots
		if (data.config.disunits == NAUTICAL) {
			StrCat(output_char, "0,");
		} else if (data.config.disunits == STATUTE) {
			StrCat(output_char, "3,");
		} else {
			if (data.config.altunits == METRIC) {
				StrCat(output_char, "1,");
			} else {
				StrCat(output_char, "2,");
			}
		}
		//Output the Pilot Name (24 chars)
		// Must Add Padding to 24 Chars
		StrNCopy(tempchar, data.igchinfo.name, 24);
		StrCat(output_char, rightpad(tempchar, ' ', 24));
//		if (StrLen(data.igchinfo.name) < 24) {
//			tempchar[StrLen(data.igchinfo.name)] = '\0';
//		} else {
//			tempchar[24] = '\0';
//		}
//		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
		
		// Output the Glider Type
		StrNCopy(tempchar, data.igchinfo.type, 9);
		StrCat(output_char, rightpad(tempchar, ' ', 9));

		// Output the Glider Contest ID
		// Must Add Padding to 12 Chars
		StrCopy(tempchar, data.igchinfo.cid);
		StrCat(output_char, rightpad(tempchar, ' ', 3));
		StrCat(output_char, ",");

		// Output the Datum Mode
		// Hardcoding it to 100 or WGS84
		// It is ignored by IGC approved units
		StrCat(output_char, "100,");

		// Output the Approach Radius in 10ths of Km
		// Value in Nautical miles converted to Km and then to meters
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(caidata->apprad * DISTKMCONST * 10.0, 0))));
		StrCat(output_char, ",");
	
		// Output the Approach Radius in 10ths of Km
		// Value in Nautical miles converted to Km and then to meters
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(caidata->arvrad * DISTKMCONST * 10.0, 0))));
		StrCat(output_char, ",");

		// Logging values are one numeric value with the two combined
		// Multiply Fast * 256 and then add the Slow value
		// Output the Close(Fast) Logging Interval in seconds
		StrCat(output_char, StrIToA(tempchar, (data.config.fastlogint*256+data.config.slowlogint)));
		StrCat(output_char, "\r");

		TxData(output_char, USESER);

//		HostTraceOutputTL(appErrorClass, "%s", output_char);
		if (!(WaitFor(CAI_DOWNLOAD_PROMPT, USESER))) {
			return(false);
		}
	}

	FreeMem((void *)&caidata);

	return(true);
}

/**
* \brief function to send glider info to CAI flight computer
* \return true if download prompt seen (data understood and accepted), else false
*/
Boolean SendCAIGliderInfo()
{
	Char output_char[65];
	Char tempchar[65];
	MemHandle record_handle;
	MemPtr record_ptr;
	PolarData *polar;
	double a, b, c;
	double V2;
	double twomsinkts;
	double ldspd, ldsnk, ld;
	CAIAddData *caidata;
	Boolean gliderinfo;

	AllocMem((void *)&caidata, sizeof(CAIAddData));

	/* Retrieve IGC Header Info */
	OpenDBQueryRecord(config_db, CAIINFO_REC, &record_handle, &record_ptr);
	MemMove(caidata, record_ptr, sizeof(CAIAddData));
	MemHandleUnlock(record_handle);
	gliderinfo = caidata->gliderinfo;
	FreeMem((void *)&caidata);

	// set glider info flag set?
	if (gliderinfo) {
//		HostTraceOutputTL(appErrorClass, "About to AllocMem");
		AllocMem((void *)&polar, sizeof(PolarData));
		// Retrieve a copy of the active polar
//		HostTraceOutputTL(appErrorClass, "Getting Polar Record");
		OpenDBQueryRecord(polar_db, POLAR_REC, &record_handle, &record_ptr);
		MemMove(polar, record_ptr, sizeof(PolarData));
		MemHandleUnlock(record_handle);

//		HostTraceOutputTL(appErrorClass, "Calling CalcPolarABC");
		CalcPolarABC(polar, 1.0);
		a = polar->a;
//		HostTraceOutputTL(appErrorClass, "a=|%s|", DblToStr(a, 6));
		b = polar->b;
//		HostTraceOutputTL(appErrorClass, "b=|%s|", DblToStr(b, 6));
		c = polar->c;
//		HostTraceOutputTL(appErrorClass, "c=|%s|", DblToStr(c, 6));

		ldspd = Sqrt(c/a);
//		HostTraceOutputTL(appErrorClass, "ldspd=|%s|", DblToStr(ldspd, 6));
		ldsnk = (a*ldspd*ldspd) + (b*ldspd) + c;
//		HostTraceOutputTL(appErrorClass, "ldsnk=|%s|", DblToStr(ldsnk, 6));
		ld = ldspd / ldsnk * -1.0;
//		HostTraceOutputTL(appErrorClass, "ld=|%s|", DblToStr(ld, 6));
		// Must be negative value.  Sink is negative.
		twomsinkts = -2.0 / AIRMPSCONST;
//		HostTraceOutputTL(appErrorClass, "twomsinkts=|%s|", DblToStr(twomsinkts, 6));
		V2 = ((-1.0 * b) - Sqrt(b*b - (4.0*a*(c-twomsinkts)))) / (2.0*a); 
//		HostTraceOutputTL(appErrorClass, "V2=|%s|", DblToStr(V2, 6));

		// G,GliderType,GliderID,L/D,VM,V2,Dry Weight(Kg), Ballast(Liters), Reserved, Config Word
		// Sending Pilot Data
//		HostTraceOutputTL(appErrorClass, "Sending Glider Data");
		StrCopy(output_char, "G,");
		//Output the Glider Type (12 chars)
		StrNCopy(tempchar, polar->name, 12);
		if (StrLen(polar->name) < 12) {
			tempchar[StrLen(polar->name)] = '\0';
		} else {
			tempchar[12] = '\0';
		}
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
	
		// Output the Glider Contest ID
		StrCat(output_char, data.igchinfo.cid);
		StrCat(output_char, ",");

		// Outputting L/D
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(ld, 0))));
		StrCat(output_char, ",");

		// Outputting VM
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(ldspd*SPDKPHCONST, 0))));
		StrCat(output_char, ",");

		// Outputting V2
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(V2*SPDKPHCONST, 0))));
		StrCat(output_char, ",");

		// Outputting Dry Weight(Kg)
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(polar->maxdrywt * WGTKGCONST, 0))));
		StrCat(output_char, ",");

		// Outputting Ballast(Liters)
		StrCat(output_char, StrIToA(tempchar, (Int32)(pround(polar->maxwater * WTRLTRCONST, 0))));
		StrCat(output_char, ",");

		// Outputting zero (0) for the Reserved field
		StrCat(output_char, "0,");

		// Outputting 65535 so that the Polar config is NOT locked
		StrCat(output_char, "65535");

		StrCat(output_char, "\r");

		TxData(output_char, USESER);

		FreeMem((void *)&polar);

//		HostTraceOutputTL(appErrorClass, "%s", output_char);
		if (!(WaitFor(CAI_DOWNLOAD_PROMPT, USESER))) {
			return(false);
		}
	}

	return(true);
}

/**
* \brief function to upload SoarPilot waypoints to CAI flight computer
* \param ShowFinished true to show upload completed dialog
* \return true if successful, else false
*/
Boolean UploadCAIWaypoints(Boolean ShowFinished)
{
	Boolean clearwaypoints = false;
	Boolean retval=true;

	clearwaypoints = true;

	HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, NULL);

	if (WayptsortType != SortByNameA) {
		// Sort the  waypoint database into Name Ascending format
		DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, SortByNameA);
	}

	// Send the CAI Command it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to clear all waypoints from the unit
	if (retval && clearwaypoints) {
		if (!SendCAIClearWaypoints()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to set baud to 9600
/*
	if (retval) {
		if (!SendCAISetSpeed(9600)) {
			HandleWaitDialog(false);
			retval = false;
		}
	}
*/

	// Send the CAI Command to put it into Download Mode
	if (retval) {
		if (!SendCAIDownloadStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send All Waypoints
	if (retval) {
		if (!SendCAIWaypoints()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to end sending waypoints
	if (retval && (data.config.flightcomp==C302COMP || data.config.flightcomp == C302ACOMP)) {
		if (!SendCAIWaypointEnd()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to put it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

/*
	// Send the CAI Command to set baud to 4800
	if (retval) {
		if (!SendCAISetSpeed(4800)) {
			HandleWaitDialog(false);
			retval = false;
		}
	}
*/

	// Put the C302/CFR into Flight PNP Mode
	// Always want to do this even if error
	SendCAIFlightModeStart(false);

	if (WayptsortType != SortByNameA) {
		// Put the waypoint db back into the current sort type
		DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, WayptsortType);
	}

	HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
	if (retval && ShowFinished) {
		if (data.config.flightcomp == C302COMP ||
			 data.config.flightcomp == C302ACOMP) {
			FrmCustomAlert(FinishedAlert, "Sending Waypoints to C302/CFR Succeeded!"," "," ");
		} else {
			FrmCustomAlert(FinishedAlert, "Sending Waypoints to GPSNAV Succeeded!"," "," ");
		}
	}
//	HostTraceOutputTL(appErrorClass, "------------------------");
	return(retval);
}

/**
* \brief function to upload max. possible waypoints to CAI flight computer
* \return true if successful, else false
*/
Boolean SendCAIWaypoints()
{
	Char output_char[81];
	Char tempchar[20];
	Int16 caiwaytype=0;
	Int16 wayindex=0;
	Int16 nrecs;
	MemHandle output_hand;
	MemPtr output_ptr;
	WaypointData *waydata;
	Boolean retval=true;

//	HostTraceOutputTL(appErrorClass, "Sending CAI Waypoints");
	nrecs = OpenDBCountRecords(waypoint_db);

	// max. number of waypoints for each model
	if (data.config.flightcomp == C302COMP ||
		 data.config.flightcomp == C302ACOMP) {
		if (nrecs > 2000) {
			nrecs = 2000;
		}
	} else {
		if (nrecs > 250) {
			nrecs = 250;
		}
	}

	AllocMem((void *)&waydata, sizeof(WaypointData));

	// step through the SoarPilot waypoint database
	for (wayindex=0; wayindex<nrecs; wayindex++) {
		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);

//		HostTraceOutputTL(appErrorClass, "Sending CAI Waypoint-wayindex|%hd|", wayindex);
		StrCopy(output_char, "C,");

		StrIToA(tempchar, (Int32)wayindex+1);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");

		LLToStringDM(waydata->lat, tempchar, ISLAT, false, true, 4);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");

		LLToStringDM(waydata->lon, tempchar, ISLON, false, true, 4);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");

		StrIToA(tempchar, (Int32)(waydata->elevation * ALTMETCONST));
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");

		StrIToA(tempchar, (Int32)wayindex+1);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");

		caiwaytype = 0;
		if (waydata->type & AIRPORT) caiwaytype|= CAIAIRPORT;
		if (waydata->type & TURN)    caiwaytype|= CAITURN;
		if (waydata->type & LAND)    caiwaytype|= CAILAND;
		if (waydata->type & START)   caiwaytype|= CAISTART;
		if (waydata->type & FINISH)  caiwaytype|= CAIFINISH;
		if (waydata->type & MARK)    caiwaytype|= CAIMARK;
		if (waydata->type & HOME)    caiwaytype|= CAIHOME;
		if (waydata->type & THRML)   caiwaytype|= CAITHRML;
		if (waydata->type & AREA)    caiwaytype|= CAITURN;
		StrIToA(tempchar, (Int32)caiwaytype);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");

		StrNCopy(tempchar, waydata->name, 12);
		tempchar[12] = '\0';
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");

		StrNCopy(tempchar, waydata->rmks, 12);
		tempchar[12] = '\0';
		StrCat(output_char, tempchar);
		StrCat(output_char, "\r");

		TxData(output_char, USESER);
		HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, wayindex, "waypoints");
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
		if (!(WaitFor(CAI_DOWNLOAD_PROMPT, USESER))) {
			retval = false;
			// To get out of the for loop
			wayindex = nrecs;
		}
	}
	FreeMem((void *)&waydata);

	return(retval);
}

/**
* \brief function to clear all waypoints of CAI flight computer
* \return true if successful, else false
*/
Boolean SendCAIClearWaypoints()
{
	Char output_char[14];

//	HostTraceOutputTL(appErrorClass, "Sending CAI Clear Waypoints Command");
	// Xmit the C302 command to clear all waypoints
	// Clears just the waypoints for the C302
	// Clears waypoints, tasks and declaration in the GPSNAV
	StrCopy(output_char, "CLEAR POINTS\r");
	TxData(output_char, USESER);
	if (!(WaitFor(CAI_COMMAND_PROMPT, USESER))) {
		return(false);
	}

	return(true);
}

/**
* \brief function to send waypoint end command to CAI flight computer
* \return true if successful, else false
*/
Boolean SendCAIWaypointEnd()
{
	Char output_char[6];

//	HostTraceOutputTL(appErrorClass, "Sending CAI Waypoints End");
	StrCopy(output_char, "C,-1\r");
	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor(CAI_DOWNLOAD_PROMPT, USESER))) {
		return(false);
	}
	return(true);

}

/**
* \brief function to upload SoarPilot glider settings to CAI flight computer
* \return true if successful, else false
*/
Boolean UploadCAIGliderInfo()
{
	Boolean retval=true;

	HandleWaitDialog(true);

	// Send the CAI Command it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to put it into Download Mode
	if (retval) {
		if (!SendCAIDownloadStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send Glider Info Data
	if (retval) {
		if (!SendCAIGliderInfo()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to put it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Put the C302/CFR into Flight PNP Mode
	// Always want to do this even if error
	SendCAIFlightModeStart(false);

	HandleWaitDialog(false);
	if (retval) {
		FrmCustomAlert(FinishedAlert, "Sending Glider Info to C302/CFR Succeeded!"," "," ");
	}
//	HostTraceOutputTL(appErrorClass, "------------------------");
	return(retval);
}

/**
* \brief function to upload SoarPilot pilot info to CAI flight computer
* \return true if successful, else false
*/
Boolean UploadCAIPilotInfo()
{
	Boolean retval=true;

	HandleWaitDialog(true);

	// Send the CAI Command it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to put it into Download Mode
	if (retval) {
		if (!SendCAIDownloadStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send Pilot Info Data
	if (retval) {
		if (data.config.flightcomp == C302COMP ||
			 data.config.flightcomp == C302ACOMP) {
			// Send Pilot Info Data to C302
			if (!SendCAIPilotInfo()) {
				HandleWaitDialog(false);
				retval = false;
			}
		} else {
			// Send Pilot & Config Info Data To GPSNAV
			if (!SendGPSNAVConfigInfo()) {
				HandleWaitDialog(false);
				retval = false;
			}
		}
	}

	// Send the CAI Command to put it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Put the C302/CFR into Flight PNP Mode
	// Always want to do this even if error
	SendCAIFlightModeStart(false);

	HandleWaitDialog(false);
	if (retval) {
		if (data.config.flightcomp == C302COMP ||
			 data.config.flightcomp == C302ACOMP) {
			FrmCustomAlert(FinishedAlert, "Sending Pilot Info to C302/CFR Succeeded!"," "," ");
		} else {
			FrmCustomAlert(FinishedAlert, "Sending Pilot/Config Info to GPSNAV Succeeded!"," "," ");
		}
	}
//	HostTraceOutputTL(appErrorClass, "------------------------");
	return(retval);
}

Boolean ClearCAIWaypoints()
{
	Boolean retval=true;

	HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, NULL);


	// Send the CAI Command it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to clear all waypoints from the unit
	if (retval) {
		if (!SendCAIClearWaypoints()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Put the C302/CFR into Flight PNP Mode
	// Always want to do this even if error
	SendCAIFlightModeStart(false);

	HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
	if (retval) {
		if (data.config.flightcomp == C302COMP ||
			 data.config.flightcomp == C302ACOMP) {
			FrmCustomAlert(FinishedAlert, "Clearing Waypoints in C302/CFR Succeeded!"," "," ");
		} else {
			FrmCustomAlert(FinishedAlert, "Clearing Waypoints in GPSNAV Succeeded!"," "," ");
		}
	} else {
		//Display an Error Dialog
		FrmCustomAlert(WarningAlert, "Clearing Logger Waypoints Failed!"," "," ");
	}
//	HostTraceOutputTL(appErrorClass, "------------------------");
	return(retval);
}

Boolean ClearCAIFlights()
{
	Boolean retval=true;

	HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, NULL);


	// Send the CAI Command it into Command Mode
	if (retval) {
		if (!SendCAICommandStart()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Send the CAI Command to clear all waypoints from the unit
	if (retval) {
		if (!SendCAIClearFlights()) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	// Put the C302/CFR into Flight PNP Mode
	// Always want to do this even if error
	SendCAIFlightModeStart(false);

	HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
	if (retval) {
		if (data.config.flightcomp == C302COMP ||
			 data.config.flightcomp == C302ACOMP) {
			FrmCustomAlert(FinishedAlert, "Clearing Flights in C302/CFR Succeeded!"," "," ");
		} else {
			FrmCustomAlert(FinishedAlert, "Clearing Flights in GPSNAV Succeeded!"," "," ");
		}
	} else {
		//Display an Error Dialog
		FrmCustomAlert(WarningAlert, "Clearing Logger Flights Failed!"," "," ");
	}
//	HostTraceOutputTL(appErrorClass, "------------------------");
	return(retval);
}

Boolean SendCAIClearFlights()
{
	Char output_char[14];

//	HostTraceOutputTL(appErrorClass, "Sending CAI Clear Flight Log Command");
	// Clear Flights Logs
	StrCopy(output_char, "CLEAR LOG\r");
	TxData(output_char, USESER);
	if (!(WaitFor(CAI_COMMAND_PROMPT, USESER))) {
		return(false);
	}

	return(true);
}

static int putChar(CAIData *caidata, Char c)
{
//	Char ec;
	Err error;

	caidata->checksum ^= c;
//	TxData(&c, USESER);
// PG use 
//	serial_out(c);
	SrmSend(serial_ref, &c, 1, &error);
//	return GetData(&ec, 1, USESER)
//		&& ec == c;
	return true;
}

Int16 putString(CAIData *caidata, const Char *str)
{
	while (*str) {
		if (!putChar(caidata, *str++))
			return false;
	}
	return true;
}

static Int16 putDouble(CAIData *caidata, double value, Int16 precision)
{
	Char buffer[13];

	StrNCopy(buffer, (DblToStr2(value, precision, true)), sizeof(buffer));
	return putString(caidata, buffer);
}

static Int16 getString(CAIData *caidata, Char *str, Int16 size)
{
	if (GetData(str, size, USESER)) {
		Int16 i;

		for (i = 0; i < size; i++) {
			caidata->checksum ^= str[i]&255;
			caidata->longChecksum += str[i]&255;
		}

		return true;
	} else
		return false;
}

static Int16 cmdMode(CAIData *caidata)
{
	Int16 i;
	Char output_char[6];

//	HostTraceOutputTL(appErrorClass, "Sending CAI Command Mode Command");
	// Put C302/CFR into Command Mode
	StrCopy(output_char, "\003");
	TxData(output_char, USESER);

	for (i = 0; caidata->state != GL_CAI_CMD && i < GL_CAI_RETRY_CMD; i++) {
		StrCopy(output_char, "\003");
		if (TxData(output_char, USESER)) {
			ClearSerial();
			if (TxData(output_char, USESER)
				&& WaitFor(CAI_COMMAND_PROMPT, USESER)) {
				caidata->state = GL_CAI_CMD;
			} else {
				caidata->state = GL_CAI_UNKNOWN;
			}
		}
	}

	return (caidata->state == GL_CAI_CMD);
}

/*
static Int16 downloadMode(CAIData *caidata)
{
	if (caidata->state != GL_CAI_DOW) {
		if (!cmdMode(caidata)
		  || !putString(caidata, "DOW\r")
		  || !WaitFor(CAI_DOWNLOAD_PROMPT, USESER)) {
			caidata->state = GL_CAI_UNKNOWN;
			return false;
		}
		caidata->state = GL_CAI_DOW;
	}
	caidata->checksum = 0;
	return (true);
}
*/

static Int16 uploadMode(CAIData *caidata)
{
//	HostTraceOutputTL(appErrorClass, "Inside uploadMode");
	if (caidata->state != GL_CAI_UPL) {
		if (!cmdMode(caidata)
		  || !putString(caidata, "UPL 1\r")
		  || !WaitFor(CAI_UPLOAD_PROMPT, USESER)) {
			caidata->state = GL_CAI_UNKNOWN;
			return false;
		}
		caidata->state = GL_CAI_UPL;
	}
	caidata->checksum = 0;
	caidata->longChecksum = 0;
	return true;
}

Int16 CAIFlightMode(CAIData *caidata)
{
	Char outputstr[6];

	if (data.config.flightcomp == C302COMP ||
		 data.config.flightcomp == C302ACOMP) {
		StrCopy(outputstr, "PNP\r");
	} else {
		StrCopy(outputstr, "NMEA\r");
	}
	if (!cmdMode(caidata)
	  || !putString(caidata, outputstr)) {
		caidata->state = GL_CAI_UNKNOWN;
		return false;
	}
	caidata->state = GL_CAI_PNP;
	return true;
}

static Int16 shortChecksum(CAIData *caidata)
{
	Char checksum = 0;
	Int16 i;

	for (i = 3; i < (caidata->buffer[0]&255); i++)
		checksum ^= caidata->buffer[i];

	return(checksum == caidata->buffer[2]);
}

static Int16 longChecksum(CAIData *caidata, Int16 size)
{
	unsigned short i, checksum = 0;

	for (i = 5; i < size; i++)
		checksum += caidata->buffer[i]&255;

	return checksum == (caidata->buffer[3]&255)*256 + (caidata->buffer[4]&255);

}

static Int16 shortReply(CAIData *caidata, const Char *prompt)
{
	Int16 size, promptSize;
	promptSize = StrLen(prompt);
	size = 0;

	return TxData("\r", USESER)
		&& (GetData(caidata->buffer, 1, USESER)
		&& (size = caidata->buffer[0]&255) <= GL_CAI_BUFFER_SIZE
		&& GetData(caidata->buffer + 1, size - 1, USESER)
		&& caidata->buffer[1] == caidata->checksum
		&& shortChecksum(caidata)
		&& StrNCompare(prompt, caidata->buffer + size - promptSize, promptSize) == 0);
}

static Int16 longReply(CAIData *caidata, const Char *prompt)
{
	Int16 size; 
	Int16 promptSize = StrLen(prompt);
//	Int16 retval = false;

	return(TxData("\r", USESER)
		&& GetData(caidata->buffer, 2, USESER)
		&& (size = ((caidata->buffer[0]&255)<<8)+(caidata->buffer[1]&255)) <= GL_CAI_BUFFER_SIZE
		&& GetData(caidata->buffer + 2, size - 2, USESER)
		&& caidata->buffer[2] == caidata->checksum
		&& longChecksum(caidata, size)
		&& StrNCompare(prompt, caidata->buffer + size - promptSize, promptSize) == 0);
/*
	TxData("\r", USESER);
	if (GetData(caidata->buffer, 2, USESER)) {
		size = ((caidata->buffer[0]&255)<<8)+(caidata->buffer[1]&255);
		if (size <= GL_CAI_BUFFER_SIZE) {
			if (GetData(caidata->buffer + 2, size - 2, USESER)) {
				if (caidata->buffer[2] == caidata->checksum) {
					if (longChecksum(caidata, size)) {
						if (StrNCompare(prompt, caidata->buffer + size - promptSize, promptSize) == 0) {
							retval = true;
						} else {
//							HostTraceOutputTL(appErrorClass, "Compare Failed");
						}
					} else {
//						HostTraceOutputTL(appErrorClass, "longChecksum Failed");
					}
				} else {
//					HostTraceOutputTL(appErrorClass, "buffer[2]/checksum Compare Failed");
				}
			} else {
//				HostTraceOutputTL(appErrorClass, "Second GetData Failed");
			}
		} else {
//			HostTraceOutputTL(appErrorClass, "size is too large");
		}
	} else {
//		HostTraceOutputTL(appErrorClass, "First GetData Failed");
	}
//	HostTraceOutputTL(appErrorClass, "longReply returning-|%hd|", retval);
	return(retval);
*/
}
				
// This is used just to get the number of logs in the 302
Boolean GetCAINumLogs(CAIData *caidata, Int16 *numLogs)
{
	caidata->index = 0;
	previndex = -1;
	// Put it in upload mode
	if (uploadMode(caidata)
	// Send Command to get first block of log info
	// Probably should Waitfor the upload prompt here as well
	  && putString(caidata, "B 196")
	// Waitfor the upload prompt to return putting info into caidata->buffer
	  && longReply(caidata, CAI_UPLOAD_PROMPT)) {
		// Save the number of logs
		*numLogs = caidata->buffer[5]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs-|%hd|", *numLogs);
		return(true);
	} else {
		return(false);
	}
}

// This is used just to get the number of logs in the GPSNAV
Boolean GetGPNNumLogs(CAIData *caidata, Int16 *numLogs)
{
	caidata->index = 0;
	previndex = -1;
	// Put it in upload mode
	if (uploadMode(caidata)
	// Send Command to get first block of log info
	// Probably should Waitfor the upload prompt here as well
		&& putString(caidata, "B 129")
	// Waitfor the upload prompt to return putting info into caidata->buffer
		&& shortReply(caidata, CAI_UPLOAD_PROMPT)) {
		// Save the number of logs
		// Value is zero based so must add one
//		*numLogs = (caidata->buffer[6]&255) + 1;
		*numLogs = (caidata->buffer[6]) + 1;
//		HostTraceOutputTL(appErrorClass, "*numLogs6-|%hd|", *numLogs);
/*
		GetData(caidata->buffer, 13, USESER);
		// Save the number of logs
		*numLogs = caidata->buffer[0]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs0-|%hd|", *numLogs);
		*numLogs = caidata->buffer[1]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs1-|%hd|", *numLogs);
		*numLogs = caidata->buffer[2]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs2-|%hd|", *numLogs);
		*numLogs = caidata->buffer[3]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs3-|%hd|", *numLogs);
		*numLogs = caidata->buffer[4]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs4-|%hd|", *numLogs);
		*numLogs = caidata->buffer[5]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs5-|%hd|", *numLogs);
		*numLogs = caidata->buffer[6]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs6-|%hd|", *numLogs);
		*numLogs = caidata->buffer[7]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs7-|%hd|", *numLogs);
		*numLogs = caidata->buffer[8]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs8-|%hd|", *numLogs);
		*numLogs = caidata->buffer[9]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs9-|%hd|", *numLogs);
		*numLogs = caidata->buffer[10]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs10-|%hd|", *numLogs);
		*numLogs = caidata->buffer[11]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs11-|%hd|", *numLogs);
		*numLogs = caidata->buffer[12]&255;
//		HostTraceOutputTL(appErrorClass, "*numLogs12-|%hd|", *numLogs);
*/

		return(true);
	} else {
		return(false);
	}
}

Boolean GetCAILogInfoNext(CAIData *caidata, CAILogData *logData)
{
	Int16 offset = 6 + (caidata->index%8)*36;
//	Char output_char[6];
	Int16 numLogs;
//	Char tempchar[6];

//	caidata->checksum = 0;
//	caidata->longChecksum = 0;

//	HostTraceOutputTL(appErrorClass, "offset-|%hd|", offset);
//	HostTraceOutputTL(appErrorClass, "previndex-|%hd|", previndex);
//	HostTraceOutputTL(appErrorClass, "caidata->index-|%hd|", caidata->index);

	if (caidata->index < (caidata->buffer[5]&255) && (offset != 6 || caidata->index%8 == 0)) {
		if (previndex/8 != caidata->index/8) {
			if (!uploadMode(caidata)
					|| !putString(caidata, "B ")
					|| !putDouble(caidata, (double)(196 + caidata->index/8), 0)
					|| !longReply(caidata, CAI_UPLOAD_PROMPT)) {
				return(false);
			}

/*
			// Xmit the C302 command to get a block of log entries
			// but only if the current block is not the one required
//			HostTraceOutputTL(appErrorClass, "Getting block of log entries");
			StrCopy(output_char, "B ");
		StrIToA(tempchar, (196 + caidata->index/8));
			StrCat(output_char, tempchar);
//			HostTraceOutputTL(appErrorClass, "output_char-|%s|", output_char);
			putString(caidata, output_char);
			if(!longReply(caidata, CAI_UPLOAD_PROMPT)) {
				return(false);
			}
*/
		}
		previndex = caidata->index;
	numLogs = caidata->buffer[5]&255;
//		HostTraceOutputTL(appErrorClass, "numLogs2-|%hd|", numLogs);
		logData->index = caidata->index;
//		HostTraceOutputTL(appErrorClass, "logData->index-|%hd|", logData->index);
//		HostTraceOutputTL(appErrorClass, "caidata->index-|%hd|", caidata->index);
		logData->startDate.year = caidata->buffer[offset];
//		HostTraceOutputTL(appErrorClass, "logData->startDate.year-|%hd|", logData->startDate.year);
		logData->startDate.month = caidata->buffer[offset + 1];
//		HostTraceOutputTL(appErrorClass, "logData->startDate.month-|%hd|", logData->startDate.month);
		logData->startDate.day = caidata->buffer[offset + 2];
//		HostTraceOutputTL(appErrorClass, "logData->startDate.day-|%hu|", logData->startDate.day);
		logData->startTime.hour = caidata->buffer[offset + 3]; 
//		HostTraceOutputTL(appErrorClass, "logData->startTime.hour-|%hd|", logData->startTime.hour);
		logData->startTime.min = caidata->buffer[offset + 4]; 
//		HostTraceOutputTL(appErrorClass, "logData->startTime.min-|%hd|", logData->startTime.min);
		logData->startTime.sec = caidata->buffer[offset + 5]; 
//		HostTraceOutputTL(appErrorClass, "logData->startTime.sec-|%hd|", logData->startTime.sec);
		
		logData->startTime.totalsecs = (UInt32)logData->startTime.hour*3600; 
		logData->startTime.totalsecs += (UInt32)logData->startTime.min*60; 
		logData->startTime.totalsecs += (UInt32)logData->startTime.sec; 
//		HostTraceOutputTL(appErrorClass, "logData->startTime.totalsecs-|%lu|", logData->startTime.totalsecs);

		logData->endDate.year = caidata->buffer[offset + 6];
//		HostTraceOutputTL(appErrorClass, "logData->endData.year-|%hd|", logData->endDate.year);
		logData->endDate.month = caidata->buffer[offset + 7];
//		HostTraceOutputTL(appErrorClass, "logData->endData.month-|%hd|", logData->endDate.month);
		logData->endDate.day = caidata->buffer[offset + 8];
//		HostTraceOutputTL(appErrorClass, "logData->endData.day-|%hd|", logData->endDate.day);
		logData->endTime.hour = caidata->buffer[offset + 9]; 
//		HostTraceOutputTL(appErrorClass, "logData->endTime.hour-|%hd|", logData->endTime.hour);
		logData->endTime.min = caidata->buffer[offset + 10]; 
//		HostTraceOutputTL(appErrorClass, "logData->endTime.min-|%hd|", logData->endTime.min);
		logData->endTime.sec = caidata->buffer[offset + 11]; 
//		HostTraceOutputTL(appErrorClass, "logData->endTime.sec-|%hd|", logData->endTime.sec);

		logData->endTime.totalsecs = (UInt32)logData->endTime.hour*3600; 
		logData->endTime.totalsecs += (UInt32)logData->endTime.min*60; 
		logData->endTime.totalsecs += (UInt32)logData->endTime.sec; 
//		HostTraceOutputTL(appErrorClass, "logData->endTime.totalsecs-|%lu|", logData->endTime.totalsecs);

		StrNCopy(logData->pilotName, caidata->buffer + offset + 12, GL_CAI_NAME_SIZE);
		logData->pilotName[GL_CAI_NAME_SIZE] = '\0';
//		HostTraceOutputTL(appErrorClass, "logData->pilotName-|%s|", logData->pilotName);
		caidata->index += 1;
		return(true);
	}
	return(false);
}

Boolean GetGPNLogInfo(CAIData *caidata, CAILogData *logData)
{
	Int16 tempnum;
	Int16 i;
//	Int16 offset = 6 + (caidata->index%8)*36;
//	Char tempchar[15];

//	Char output_char[6];
//	Int16 numLogs;
//	Char tempchar[6];

//	caidata->checksum = 0;
//	caidata->longChecksum = 0;

//	HostTraceOutputTL(appErrorClass, "offset-|%hd|", offset);
//	HostTraceOutputTL(appErrorClass, "previndex-|%hd|", previndex);
//	HostTraceOutputTL(appErrorClass, "caidata->index-|%hd|", caidata->index);

//	if (caidata->index < (caidata->buffer[5]&255) && (offset != 6 || caidata->index%8 == 0)) {
//		if (previndex/8 != caidata->index/8) {
			if (uploadMode(caidata)
				&& putString(caidata, "B\r")) {
//				&& shortReply(caidata, CAI_UPLOAD_PROMPT)) 

				GetData(caidata->buffer, 20, USESER);
//				HostTraceOutputTL(appErrorClass, "  Hex2Dec2(0012)-|%ld|", Hex2Dec2("0012"));
//				HostTraceOutputTL(appErrorClass, "  Hex2Dec2(0018)-|%ld|", Hex2Dec2("0018"));
//				HostTraceOutputTL(appErrorClass, "  Hex2Dec2(0022)-|%ld|", Hex2Dec2("0022"));
//				HostTraceOutputTL(appErrorClass, "caidata->buffer)-|%s|", caidata->buffer);
				// Get the number of log files 
				for (i=0;i<6;i++) {
//					tempnum = caidata->buffer[i]&255;
					tempnum = caidata->buffer[i];
//					HostTraceOutputT(appErrorClass, "buffer-|%hx|", caidata->buffer[i]);
//					HostTraceOutputT(appErrorClass, "  tempchar-|%s|", StrIToA(tempchar, tempnum));
//					HostTraceOutputT(appErrorClass, "  Hex2Dec2-|%ld|", Hex2Dec2(leftpad(tempchar, '0', 4)));
//					HostTraceOutputT(appErrorClass, "  tempnum-|%hd|", tempnum);
//					HostTraceOutputTL(appErrorClass, "  dec(%hd)-|%lu|", i, B142Dec(tempchar));
				}
			}

/*
			// Xmit the C302 command to get a block of log entries
			// but only if the current block is not the one required
//			HostTraceOutputTL(appErrorClass, "Getting block of log entries");
			StrCopy(output_char, "B ");
		StrIToA(tempchar, (196 + caidata->index/8));
			StrCat(output_char, tempchar);
//			HostTraceOutputTL(appErrorClass, "output_char-|%s|", output_char);
			putString(caidata, output_char);
			if(!longReply(caidata, CAI_UPLOAD_PROMPT)) {
				return(false);
			}
*/
//		}
/*
		previndex = caidata->index;
//	numLogs = caidata->buffer[5]&255;
//		HostTraceOutputTL(appErrorClass, "numLogs2-|%hd|", numLogs);
		logData->index = caidata->index;
//		HostTraceOutputTL(appErrorClass, "logData->index-|%hd|", logData->index);
//		HostTraceOutputTL(appErrorClass, "caidata->index-|%hd|", caidata->index);
		logData->startDate.year = caidata->buffer[offset];
//		HostTraceOutputTL(appErrorClass, "logData->startDate.year-|%hd|", logData->startDate.year);
		logData->startDate.month = caidata->buffer[offset + 1];
//		HostTraceOutputTL(appErrorClass, "logData->startDate.month-|%hd|", logData->startDate.month);
		logData->startDate.day = caidata->buffer[offset + 2];
//		HostTraceOutputTL(appErrorClass, "logData->startDate.day-|%hu|", logData->startDate.day);
		logData->startTime.hour = caidata->buffer[offset + 3]; 
//		HostTraceOutputTL(appErrorClass, "logData->startTime.hour-|%hd|", logData->startTime.hour);
		logData->startTime.min = caidata->buffer[offset + 4]; 
//		HostTraceOutputTL(appErrorClass, "logData->startTime.min-|%hd|", logData->startTime.min);
		logData->startTime.sec = caidata->buffer[offset + 5]; 
//		HostTraceOutputTL(appErrorClass, "logData->startTime.sec-|%hd|", logData->startTime.sec);
		
		logData->startTime.totalsecs = (UInt32)logData->startTime.hour*3600; 
		logData->startTime.totalsecs += (UInt32)logData->startTime.min*60; 
		logData->startTime.totalsecs += (UInt32)logData->startTime.sec; 
//		HostTraceOutputTL(appErrorClass, "logData->startTime.totalsecs-|%lu|", logData->startTime.totalsecs);

		logData->endDate.year = caidata->buffer[offset + 6];
//		HostTraceOutputTL(appErrorClass, "logData->endData.year-|%hd|", logData->endDate.year);
		logData->endDate.month = caidata->buffer[offset + 7];
//		HostTraceOutputTL(appErrorClass, "logData->endData.month-|%hd|", logData->endDate.month);
		logData->endDate.day = caidata->buffer[offset + 8];
//		HostTraceOutputTL(appErrorClass, "logData->endData.day-|%hd|", logData->endDate.day);
		logData->endTime.hour = caidata->buffer[offset + 9]; 
//		HostTraceOutputTL(appErrorClass, "logData->endTime.hour-|%hd|", logData->endTime.hour);
		logData->endTime.min = caidata->buffer[offset + 10]; 
//		HostTraceOutputTL(appErrorClass, "logData->endTime.min-|%hd|", logData->endTime.min);
		logData->endTime.sec = caidata->buffer[offset + 11]; 
//		HostTraceOutputTL(appErrorClass, "logData->endTime.sec-|%hd|", logData->endTime.sec);

		logData->endTime.totalsecs = (UInt32)logData->endTime.hour*3600; 
		logData->endTime.totalsecs += (UInt32)logData->endTime.min*60; 
		logData->endTime.totalsecs += (UInt32)logData->endTime.sec; 
//		HostTraceOutputTL(appErrorClass, "logData->endTime.totalsecs-|%lu|", logData->endTime.totalsecs);

		StrNCopy(logData->pilotName, caidata->buffer + offset + 12, GL_CAI_NAME_SIZE);
		logData->pilotName[GL_CAI_NAME_SIZE] = '\0';
//		HostTraceOutputTL(appErrorClass, "logData->pilotName-|%s|", logData->pilotName);
		caidata->index += 1;
		return(true);
*/
//	}
	return(false);
}

Boolean CAILogStart(CAIData *caidata, Int16 logIndex, Int16 *blockSize)
{
//	HostTraceOutputTL(appErrorClass, "Inside CAILogStart");
	caidata->index = 0;
	if (uploadMode(caidata)
		&& putString(caidata, "B ")
		&& putDouble(caidata, (double)(64 + logIndex), 0)
		&& longReply(caidata, CAI_UPLOAD_PROMPT)
		&& caidata->buffer[5] == 'Y') {
		caidata->blockSize = *blockSize = ((caidata->buffer[6]&255)<<8)+(caidata->buffer[7]&255);
		appnumblocks = ((caidata->buffer[8]&255)<<8)+(caidata->buffer[9]&255);
//		HostTraceOutputTL(appErrorClass, "Approx NumBlocks-|%hd|", ((caidata->buffer[8]&255)<<8)+(caidata->buffer[9]&255));
		return(true);
	} else {
		return(false);
	}
}

static Int16 transferReply(CAIData *caidata, Char *buffer, Int16 size)
{
	Int16 promptSize = StrLen(CAI_UPLOAD_PROMPT);
/*
	Int16 retval = false;
	if (TxData("\r", USESER)) {
		if (GetData(caidata->buffer, 5, USESER)) {
			if (STR2SHORT(caidata->buffer) == size + promptSize + 7) {
				if (caidata->buffer[2] == caidata->checksum) {
					if (getString(caidata, caidata->buffer + 5, 2)) {
						if (getString(caidata, buffer, size)) {
							if (getString(caidata, caidata->buffer + 7, promptSize)) {
								if (STR2SHORT(caidata->buffer + 3) == caidata->longChecksum) {
									if (StrNCompare(caidata->buffer + 7, CAI_UPLOAD_PROMPT, promptSize) == 0) {
										retval = true;
									} else {
//										HostTraceOutputTL(appErrorClass, "StrNCompare Failed");
									}
								} else {
//									HostTraceOutputTL(appErrorClass, "STR2SHORT2 Failed");
								}
							} else {
//								HostTraceOutputTL(appErrorClass, "getString3 Failed");
							}
						} else {
//							HostTraceOutputTL(appErrorClass, "getString2 Failed");
						}
					} else {
//						HostTraceOutputTL(appErrorClass, "getString1 Failed");
					}
				} else {
//					HostTraceOutputTL(appErrorClass, "buffer[2] != checksum");
				}
			} else {
//				HostTraceOutputTL(appErrorClass, "STR2SHORT1 Failed");
			}
		} else {
//			HostTraceOutputTL(appErrorClass, "GetData1 Failed");
		}
	} else {
//		HostTraceOutputTL(appErrorClass, "TxData Failed");
	}

	return(retval);
*/

	return TxData("\r", USESER)
		&& GetData(caidata->buffer, 5, USESER)
		&& STR2SHORT(caidata->buffer) == size + promptSize + 7
		&& caidata->buffer[2] == caidata->checksum
		&& getString(caidata, caidata->buffer + 5, 2)
		&& getString(caidata, buffer, size)
		&& getString(caidata, caidata->buffer + 7, promptSize)
		&& STR2SHORT(caidata->buffer + 3) == caidata->longChecksum
		&& StrNCompare(caidata->buffer + 7, CAI_UPLOAD_PROMPT, promptSize) == 0;
}

Int16 CAILogRead(CAIData *caidata, Char *buffer, Int16 *valid)
{
	Int16 i;
	Char *cmd = "B N";

//	HostTraceOutputTL(appErrorClass, "Inside CAILogRead-caidata->blockSize|%hd|", caidata->blockSize);

	*valid = 0;
	if (caidata->blockSize == 0)
		return true;

	for (i = 0; i < GL_CAI_RETRY_CMD; i++) {
/*
//		HostTraceOutputTL(appErrorClass, "CAILogRead-i|%hd|", i);
		if (putString(caidata, cmd)) {
			if (transferReply(caidata, buffer, caidata->blockSize)) {
				*valid = STR2SHORT(caidata->buffer + 5);
//				HostTraceOutputTL(appErrorClass, "CAILogRead-*valid|%hd|", *valid);
				if (*valid != caidata->blockSize)
					caidata->blockSize = 0;
				return true;
			} else {
//				HostTraceOutputTL(appErrorClass, "CAILogRead-transferReply failed");
			}
		} else {
//			HostTraceOutputTL(appErrorClass, "CAILogRead-putString failed");
		}
*/

		if (uploadMode(caidata)
			&& putString(caidata, cmd)
			&& transferReply(caidata, buffer, caidata->blockSize)) {
			*valid = STR2SHORT(caidata->buffer + 5);
			if (*valid != caidata->blockSize)
				caidata->blockSize = 0;
			return true;
		}
		cmd = "B R";
	}

//	HostTraceOutputTL(appErrorClass, "CAILogRead-buffer|%s|", buffer);
//	HostTraceOutputTL(appErrorClass, "CAILogRead-About to return false");
	return false;
}

Int16 CAILogSignature(CAIData *caidata, Char *buffer, Int16 *valid)
{
	if (uploadMode(caidata)
		&& putString(caidata, "B S")
		&& transferReply(caidata, buffer, CAI_SIGNATURE_SIZE)) {
		*valid = STR2SHORT(caidata->buffer + 5);
		return true;
	}

	return false;
}

Boolean GetCAIGenInfo(CAIData *caidata, CAIGenInfo *geninfo)
{
	// 3 for short reply
	// 15 to step over the Reserved data
	Int16 offset = 3 + 15;

	if (uploadMode(caidata)
		&& putString(caidata, "W")
		&& shortReply(caidata, CAI_UPLOAD_PROMPT)) {
		StrNCopy(geninfo->id, caidata->buffer + offset, CAI_ID_SIZE);
		geninfo->id[CAI_ID_SIZE] = '\0';
//		HostTraceOutputTL(appErrorClass, "geninfo->id-|%s|", geninfo->id);

		StrNCopy(geninfo->ostype, caidata->buffer + offset + CAI_ID_SIZE, CAI_OSTYPE_SIZE);
		geninfo->ostype[CAI_OSTYPE_SIZE] = '\0';
//		HostTraceOutputTL(appErrorClass, "geninfo->ostype-|%s|", geninfo->ostype);

		StrNCopy(geninfo->ver, caidata->buffer + offset + CAI_ID_SIZE + CAI_OSTYPE_SIZE, CAI_VER_SIZE);
		geninfo->ver[CAI_VER_SIZE] = '\0';
//		HostTraceOutputTL(appErrorClass, "geninfo->ver-|%s|", geninfo->ver);
		return(true);
	} else {
		return(false);
	}
}

Boolean DownloadCAILogInfo(Int16 cmd)
{
	Boolean retval=true;
	CAIData *caidata=NULL;
	Int16 i=0;

	if (cmd == CAIFIFREE) {
		if (cailogdata) {
			FreeMem((void *)&cailogdata);
			cailogdata = NULL;
		} 
		if (caigeninfo) {
			FreeMem((void *)&caigeninfo);
			caigeninfo = NULL;
		} 
		return(true);
	}

	AllocMem((void *)&caidata, sizeof(CAIData));
	MemSet(caidata, sizeof(CAIData), 0);

	HandleWaitDialogUpdate(SHOWDIALOG, cainumLogs, i, NULL);

	// Get 302 General Info
	if (retval && (data.config.flightcomp == C302COMP || data.config.flightcomp == C302ACOMP)) {
		AllocMem((void *)&caigeninfo, sizeof(CAIGenInfo));
		MemSet(caigeninfo, sizeof(CAIGenInfo), 0);

		if (data.config.flightcomp == C302COMP ||
			 data.config.flightcomp == C302ACOMP) {
			if (!GetCAIGenInfo(caidata, caigeninfo)) {
				HandleWaitDialogUpdate(STOPDIALOG, cainumLogs, i, NULL);
				retval = false;
				cainumLogs = 0;
			} 
		}
/*
		else {
//			HostTraceOutputTL(appErrorClass, "caigeninfo->id-|%s|", caigeninfo->id);
//			HostTraceOutputTL(appErrorClass, "caigeninfo->ostype-|%s|", caigeninfo->ostype);
//			HostTraceOutputTL(appErrorClass, "caigeninfo->ver-|%s|", caigeninfo->ver);
		}
*/
	}

	// Get Number of Logs In the 302 or GPSNAV
	if (retval) {
		if (data.config.flightcomp == C302COMP ||
			 data.config.flightcomp == C302ACOMP) {
			if (!GetCAINumLogs(caidata, &cainumLogs)) {
				HandleWaitDialogUpdate(STOPDIALOG, cainumLogs, i, NULL);
				retval = false;
				cainumLogs = 0;
			}
		} else {
//			HostTraceOutputTL(appErrorClass, "Calling GetGPNNumLogs");
			if (!GetGPNNumLogs(caidata, &cainumLogs)) {
				HandleWaitDialogUpdate(STOPDIALOG, cainumLogs, i, NULL);
				retval = false;
				cainumLogs = 0;
			}
		}
//		HostTraceOutputTL(appErrorClass, "cainumLogs-|%hd|", cainumLogs);
	}

	// Get Log Entry Info from the 302
	if (retval) {
		AllocMem((void *)&cailogdata, sizeof(CAILogData)*cainumLogs);
		MemSet(cailogdata, cainumLogs * sizeof(CAILogData), 0);
		i = 0;
		if (data.config.flightcomp == C302COMP ||
			 data.config.flightcomp == C302ACOMP) {
			while (GetCAILogInfoNext(caidata, &cailogdata[i])) {
//				HostTraceOutputTL(appErrorClass, "cailogdata[%hd].index-|%hd|", i, cailogdata[i].index);
//				HostTraceOutputTL(appErrorClass, "cailogdata[%hd].pilotName-|%s|", i, cailogdata[i].pilotName);
//				HostTraceOutputTL(appErrorClass, "---------------------------------");
				i++;
				if (!HandleWaitDialogUpdate(UPDATEDIALOG, cainumLogs, i, "entries")) {
					// This will currently never happen
					break;
				}
			}
		} else {
			// GPSNAV code for getting the LogInfo Goes here
			GetGPNLogInfo(caidata, &cailogdata[i]);

//			while (GetCAILogInfoNext(caidata, &cailogdata[i])) {
//			}
		}
//		HostTraceOutputTL(appErrorClass, "cainumLogs-|%hd|", cainumLogs);
	}
 
	// Put the C302/CFR into Flight PNP Mode
	// Always want to do this even if error
	if (caidata && caidata->state != GL_CAI_UNKNOWN && caidata->state != GL_CAI_PNP) {
		CAIFlightMode(caidata);
	}

	if (retval) {
		HandleWaitDialogUpdate(STOPDIALOG, cainumLogs, i, NULL);
	}
	if (caidata) {
		FreeMem((void *)&caidata);
		caidata = NULL;
	}

//	HostTraceOutputTL(appErrorClass, "=================================");
	return(retval);
}

Boolean DownloadCAISelectedLog()
{
	Boolean retval=true;
	CAIData *caidata=NULL;
	Int16 blockSize;
	Char *buffer;
	Int16 valid;
	Int16 count=0;
	Char igcname[15];
	Boolean usercancel=false;
	Char tempchar[7];
	Int16 portspeed;

	AllocMem((void *)&caidata, sizeof(CAIData));
	MemSet(caidata, sizeof(CAIData), 0);
	AllocMem((void *)&buffer, (sizeof(Char)*1025));
	MemSet(buffer, (sizeof(Char)*1025), 0);

	HandleWaitDialogUpdate(SHOWDIALOG, appnumblocks, count, NULL);

	// set C302 to dataspeed from NMEA/Port settings screen, removing unsupported values
	portspeed = data.config.dataspeed;
	if (portspeed < 2) portspeed = 2;
	if ((portspeed == 4) || (portspeed == 6)) portspeed -= 1;
	SysStringByIndex(form_set_port_speed_table, portspeed, tempchar, 7);	
	if (retval) {
//		HostTraceOutputTL(appErrorClass, "Setting speed to %s", tempchar);
		if (!SendCAISetSpeed(caidata, StrAToI(tempchar))) {
			HandleWaitDialog(false);
			retval = false;
		}
	}

	if (retval) {
//		HostTraceOutputTL(appErrorClass, "Going into LogStart mode");
		if (!CAILogStart(caidata, selectedCAIFltIndex, &blockSize)) {
			HandleWaitDialogUpdate(STOPDIALOG, appnumblocks, count, NULL);
			retval = false;
		} else {
//			HostTraceOutputTL(appErrorClass, "LogStart Succeeded");
//			HostTraceOutputTL(appErrorClass, "blockSize-|%hd|", blockSize);
		}
	}

	if (retval) {
		GenerateCAIIGCName(&cailogdata[selectedCAIFltIndex].startDate, 
								 &cailogdata[selectedCAIFltIndex].startTime, igcname);
		XferInit(igcname, IOOPENTRUNC, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "IGC Filename-|%s|", igcname);
		if (!CAILogRead(caidata, buffer, &valid)) {
			HandleWaitDialogUpdate(STOPDIALOG, appnumblocks, count, NULL);
			retval = false;
		} else {
			TxData(buffer, data.config.xfertype);
			count++;
			HandleWaitDialogUpdate(UPDATEDIALOG, appnumblocks, count, "blocks");
//			HostTraceOutputTL(appErrorClass, "buffer-|%s|", buffer);
//			HostTraceOutputTL(appErrorClass, "valid-|%hd|", valid);
//			HostTraceOutputTL(appErrorClass, "count-|%hd|", count);
			while (valid == blockSize && usercancel == false) {
				CAILogRead(caidata, buffer, &valid);
				TxData(buffer, data.config.xfertype);
				count++;
				if (!HandleWaitDialogUpdate(UPDATEDIALOG, appnumblocks, count, "blocks")) {
					usercancel = true;
				}
//				HostTraceOutputTL(appErrorClass, "buffer-|%s|", buffer);
//				HostTraceOutputTL(appErrorClass, "valid-|%hd|", valid);
//				HostTraceOutputTL(appErrorClass, "count-|%hd|", count);
			}
		}
	}

	if (retval) {
		if (usercancel || !CAILogSignature(caidata, buffer, &valid)) {
			HandleWaitDialogUpdate(STOPDIALOG, appnumblocks, count, NULL);
			retval = false;
		} else {
			TxData(buffer, data.config.xfertype);
//			HostTraceOutputTL(appErrorClass, "buffer-|%s|", buffer);
//			HostTraceOutputTL(appErrorClass, "valid-|%hd|", valid);
		}
	}
	XferClose(data.config.xfertype);

	// Send the CAI Command to set baud to 4800
//	if (retval) {
//		HostTraceOutputTL(appErrorClass, "Setting speed to 4800");
		if (!SendCAISetSpeed(caidata, 4800)) {
			HandleWaitDialog(false);
			retval = false;
		}
//	}

	// Put the C302/CFR into Flight PNP Mode
	// Always want to do this even if error
	if (caidata && caidata->state != GL_CAI_UNKNOWN && caidata->state != GL_CAI_PNP) {
//		HostTraceOutputTL(appErrorClass, "Sending Flight Mode Command");
		CAIFlightMode(caidata);
	}

	if (retval) {
		HandleWaitDialogUpdate(STOPDIALOG, appnumblocks, count, NULL);
		FrmCustomAlert(FinishedAlert, "Getting Flight Info From C302/CFR Successful"," "," ");
	}
	if (caidata) {
		FreeMem((void *)&caidata);
		caidata = NULL;
	}
	if (buffer) {
		FreeMem((void *)&buffer);
		buffer = NULL;
	}

//	HostTraceOutputTL(appErrorClass, "=================================");
	return(retval);
}

Boolean SendCAISetSpeed(CAIData *caidata, Int32 newspeed)
{
	Char output_char[14];
	Char tempchar[14];
	Int32 x, baudval;

	if (!cmdMode(caidata)) {
	caidata->state = GL_CAI_UNKNOWN;
	return(false);
	} else {
//		HostTraceOutputTL(appErrorClass, "Sending CAI Set Baud Command");
//		HostTraceOutputTL(appErrorClass, "newspeed-|%ld|", newspeed);
		// Xmit the C302 command to change the baud rate

		// This is used to determine the Cambridge baud value to use
		x = 75;
		baudval = 0;
		while (x < newspeed && x <= 115200) {
			x = x * 2;
			baudval++;
		}
//		HostTraceOutputTL(appErrorClass, "new baudval-|%hd|", baudval);
		StrCopy(output_char, "BAUD ");
		StrIToA(tempchar, (Int32)baudval);
		StrCat(output_char, tempchar);
		StrCat(output_char, "\r");
		TxData(output_char, USESER);
//		HostTraceOutputTL(appErrorClass, "Sending-|%s| to logger", output_char);

		// Have sleep here to allow the data to be sent before
		// closing the port.  Otherwise the data is lost and never sent
		Sleep(2.0);
		XferClose(USESER);
		StrIToA(tempchar, newspeed);
//		HostTraceOutputTL(appErrorClass, "XferInit at new speed-|%ld|", newspeed);
//		HostTraceOutputTL(appErrorClass, "          newspeedstr-|%s|", tempchar);
		XferInit(tempchar, NFC, USESER);

//		HostTraceOutputTL(appErrorClass, "Sending Command Mode Again");
		if (!SendCAICommandStart()) {
//			HostTraceOutputTL(appErrorClass, "Returning False");
			return(false);
		}

//		HostTraceOutputTL(appErrorClass, "Returning True");
		return(true);
	}
}

/**
* \brief find the CAI flight number
* \param fltdate [input] flight date
* \param flttime [input] flight time
* \return flight number
*/
Int8 FindCAIFltNumOfDay(CAIDate *fltdate, CAITime *flttime) 
{
	Int16 x = 0;
	Int16 nrecs;
	Int8 fltnum = 1;

	nrecs = cainumLogs;
//	HostTraceOutputTL(appErrorClass, "nrecs-%hd", nrecs);

//	HostTraceOutputTL(appErrorClass, "fltdate->day-%hu", fltdate->day);
//	HostTraceOutputTL(appErrorClass, "fltdate->month-%hu", fltdate->month);
//	HostTraceOutputTL(appErrorClass, "fltdate->year-%hu", fltdate->year);
//	HostTraceOutputTL(appErrorClass, "flttime->hour-%hd", flttime->hour);
//	HostTraceOutputTL(appErrorClass, "flttime->min-%hd", flttime->min);
//	HostTraceOutputTL(appErrorClass, "flttime->sec-%hd", flttime->sec);
//	HostTraceOutputTL(appErrorClass, "==============================");
	

	for (x=0; x<nrecs; x++) {
//		HostTraceOutputTL(appErrorClass, "cailogdata[x].startDate.day-%hu", cailogdata[x].startDate.day);
//		HostTraceOutputTL(appErrorClass, "cailogdata[x].startDate.month-%hu", cailogdata[x].startDate.month);
//		HostTraceOutputTL(appErrorClass, "cailogdata[x].startDate.year-%hu", cailogdata[x].startDate.year);
//		HostTraceOutputTL(appErrorClass, "cailogdata[x].startTime.hour-%hd", cailogdata[x].startTime.hour);
//		HostTraceOutputTL(appErrorClass, "cailogdata[x].startTime.min-%hd", cailogdata[x].startTime.min);
//		HostTraceOutputTL(appErrorClass, "cailogdata[x].startTime.sec-%hd", cailogdata[x].startTime.sec);
//		HostTraceOutputTL(appErrorClass, "----------------------------");
		if (cailogdata[x].startDate.day == fltdate->day
			 && cailogdata[x].startDate.month == fltdate->month
			 && cailogdata[x].startDate.year == fltdate->year) {
			if (cailogdata[x].startTime.hour < flttime->hour) {
				fltnum++;
			} else if (cailogdata[x].startTime.hour == flttime->hour) {
				if (cailogdata[x].startTime.min < flttime->min) {
					fltnum++;
				} else if (cailogdata[x].startTime.min == flttime->min) {
					if (cailogdata[x].startTime.sec < flttime->sec) {
						fltnum++;
					}
				}
			}
		}
	}
//	HostTraceOutputTL(appErrorClass, "fltnum-%hd", (Int16)fltnum);
//	HostTraceOutputTL(appErrorClass, "----------------------------");
	return(fltnum);
}

/**
* \brief generate a valid CAI IGC file name
* \param fltdate [input] flight date
* \param flttime [input] flight time
* \param igcname [output] pointer to buffer for resulting IGC file name, must be big enough!
*/
void GenerateCAIIGCName(CAIDate *fltdate, CAITime *flttime, Char *igcname)
{
	Char strIgc[15];
	Int32 fltNum=0;
	Char tempchar[10];

	// hour minute second day month year
	// extract the year
	StrIToA(tempchar, fltdate->year);
	strIgc[0] = tempchar[StrLen(tempchar)-1];
	strIgc[1] = '\0';
	// extract the month
	if (fltdate->month <= 9) {
		StrIToA(tempchar, fltdate->month);
	} else {
		tempchar[0] = (fltdate->month+55);
		tempchar[1] = '\0';
	}
	StrCat(strIgc, tempchar);

	// extract the day
	if (fltdate->day <= 9) {
		StrIToA(tempchar, fltdate->day);
	} else {
		tempchar[0] = (fltdate->day+55);
		tempchar[1] = '\0';
	}
	StrCat(strIgc, tempchar);

	// Add the manaufacturer code
	StrCat(strIgc, "C");
	// Add the C302 ID value
	StrCat(strIgc, caigeninfo->id);
	// Add flight number
	fltNum = (Int32)FindCAIFltNumOfDay(fltdate, flttime);
	if (fltNum <= 9) {
		StrIToA(tempchar, fltNum);
	} else {
		tempchar[0] = (fltNum+55);
		tempchar[1] = '\0';
	}
	StrCat(strIgc, tempchar);
	// Add the extension
	StrCat(strIgc, ".igc\0");

	StrCopy(igcname, strIgc);

	return;
}

/**
* \brief form event handler for CAI config form
* \param event pointer to event data
* \return true if event handled, else false
*/
Boolean form_config_caiinst_event_handler(EventPtr event)
{
	Char tempchar[30];
	Boolean handled=false;
	MemHandle record_handle;
	MemPtr record_ptr;
	static CAIAddData *caidata=NULL;

	switch (event->eType) {
		case frmOpenEvent:
			if (caidata == NULL) AllocMem((void *)&caidata, sizeof(CAIAddData));
			// fall through
		case frmUpdateEvent:
			/* Retrieve IGC Header Info */
			OpenDBQueryRecord(config_db, CAIINFO_REC, &record_handle, &record_ptr);
			MemMove(caidata, record_ptr, sizeof(CAIAddData));
			MemHandleUnlock(record_handle);

			// overwrite QNH unit setting
			data.config.QNHunits = caidata->barounits;
			set_unit_constants(),

			FrmDrawForm(FrmGetActiveForm());
			field_set_value(form_config_caiinst_stfdb, print_horizontal_speed2(caidata->stfdb, 2));
			field_set_value(form_config_caiinst_arvrad, print_distance2(caidata->arvrad, 2));
			field_set_value(form_config_caiinst_apprad, print_distance2(caidata->apprad, 2));
			field_set_value(form_config_caiinst_tbtwn, StrIToA(tempchar, (Int32)caidata->tbtwn));
			ctl_set_value(form_config_caiinst_dalt, caidata->dalt);
			ctl_set_value(form_config_caiinst_aalt, !caidata->dalt);
			ctl_set_value(form_config_caiinst_sinktone, caidata->sinktone);
			ctl_set_value(form_config_caiinst_tefg, caidata->tefg);
			ctl_set_value(form_config_caiinst_tempc, (caidata->tempunits==CELCIUS?true:false));
			ctl_set_value(form_config_caiinst_tempf, (caidata->tempunits==FARENHEIT?true:false));
			ctl_set_value(form_config_caiinst_baromill, (caidata->barounits==MILLIBARS?true:false));
			ctl_set_value(form_config_caiinst_baroinhg, (caidata->barounits==INHG?true:false));
			ctl_set_value(form_config_caiinst_variote, (caidata->variotype==CAI_VARIO_TE?true:false));
			ctl_set_value(form_config_caiinst_varionetto, (caidata->variotype==CAI_VARIO_NETTO?true:false));
			ctl_set_value(form_config_caiinst_variosnetto, (caidata->variotype==CAI_VARIO_SNETTO?true:false));
			ctl_set_value(form_config_caiinst_pilotinfo, caidata->pilotinfo);
			ctl_set_value(form_config_caiinst_gliderinfo, caidata->gliderinfo);

			WinDrawChars(data.input.spdtext, 3, 141, 12);
			WinDrawChars(data.input.distext, 2, 141, 24);
			WinDrawChars(data.input.distext, 2, 141, 36);

			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
			if (caidata) {
				FreeMem((void *)&caidata);
				caidata = NULL;
			}
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_config_caiinst_dalt:
					caidata->dalt = true;
					handled = true;
					break;
				case form_config_caiinst_aalt:
					caidata->dalt = false;
					handled = true;
					break;
				case form_config_caiinst_sinktone:
					caidata->sinktone = ctl_get_value(form_config_caiinst_sinktone);
					handled = true;
					break;
				case form_config_caiinst_tefg:
					caidata->tefg = ctl_get_value(form_config_caiinst_tefg);
					handled = true;
					break;
				case form_config_caiinst_tempc:
					caidata->tempunits = CELCIUS;
					handled = true;
					break;
				case form_config_caiinst_tempf:
					caidata->tempunits = FARENHEIT;
					handled = true;
					break;
				case form_config_caiinst_baromill:
					caidata->barounits = MILLIBARS;
					data.config.QNHunits = MILLIBARS;
					set_unit_constants();
					handled = true;
					break;
				case form_config_caiinst_baroinhg:
					caidata->barounits = INHG;
					data.config.QNHunits = INHG;
					set_unit_constants();
					handled = true;
					break;
				case form_config_caiinst_variote:
					caidata->variotype = CAI_VARIO_TE;
					handled = true;
					break;
				case form_config_caiinst_varionetto:
					caidata->variotype = CAI_VARIO_NETTO;
					handled = true;
					break;
				case form_config_caiinst_variosnetto:
					caidata->variotype = CAI_VARIO_SNETTO;
					handled = true;
					break;
				case form_config_caiinst_savebtn:
					caidata->stfdb  = field_get_double(form_config_caiinst_stfdb)/data.input.spdconst;
					caidata->arvrad  = field_get_double(form_config_caiinst_arvrad)/data.input.disconst;
					caidata->apprad  = field_get_double(form_config_caiinst_apprad)/data.input.disconst;
					caidata->tbtwn  = (Int16)field_get_long(form_config_caiinst_tbtwn);
					OpenDBUpdateRecord(config_db, sizeof(CAIAddData), caidata, CAIINFO_REC);
					FrmGotoForm(form_set_logger);
					handled = true;
					break;
				case form_config_caiinst_cancelbtn:
					FrmGotoForm(form_set_logger);
					handled = true;
					break;
				case form_config_caiinst_pilotinfo:
					caidata->pilotinfo = ctl_get_value(form_config_caiinst_pilotinfo);
					handled = true;
					break;
				case form_config_caiinst_gliderinfo:
					caidata->gliderinfo = ctl_get_value(form_config_caiinst_gliderinfo);
					handled = true;
					break;
				default:
					break;
			}
			break;
		default:
			break;
		}
		return(handled);
}
/**
* \brief form event handler for GPSNAV config form
* \param event pointer to event data
* \return true if event handled, else false
*/
Boolean form_config_gpsnavinst_event_handler(EventPtr event)
{
	Boolean handled=false;
	MemHandle record_handle;
	MemPtr record_ptr;
	static CAIAddData *caidata=NULL;

	switch (event->eType) {
		case frmOpenEvent:
			if (caidata == NULL) AllocMem((void *)&caidata, sizeof(CAIAddData));
			// fall through
		case frmUpdateEvent:
			/* Retrieve IGC Header Info */
			OpenDBQueryRecord(config_db, CAIINFO_REC, &record_handle, &record_ptr);
			MemMove(caidata, record_ptr, sizeof(CAIAddData));
			MemHandleUnlock(record_handle);

			FrmDrawForm(FrmGetActiveForm());
			field_set_value(form_config_gpsnavinst_arvrad, print_distance2(caidata->arvrad, 2));
			field_set_value(form_config_gpsnavinst_apprad, print_distance2(caidata->apprad, 2));
			ctl_set_value(form_config_gpsnavinst_gliderinfo, caidata->gliderinfo);

			WinDrawChars(data.input.distext, 2, 141, 20);
			WinDrawChars(data.input.distext, 2, 141, 37);

			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
			if (caidata) {
				FreeMem((void *)&caidata);
				caidata = NULL;
			}
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_config_gpsnavinst_savebtn:
					caidata->arvrad  = field_get_double(form_config_gpsnavinst_arvrad)/data.input.disconst;
					caidata->apprad  = field_get_double(form_config_gpsnavinst_apprad)/data.input.disconst;
					OpenDBUpdateRecord(config_db, sizeof(CAIAddData), caidata, CAIINFO_REC);
					FrmGotoForm(form_set_logger);
					handled = true;
					break;
				case form_config_gpsnavinst_cancelbtn:
					FrmGotoForm(form_set_logger);
					handled = true;
					break;
				case form_config_gpsnavinst_gliderinfo:
					caidata->gliderinfo = ctl_get_value(form_config_gpsnavinst_gliderinfo);
					handled = true;
					break;
				default:
					break;
			}
			break;
		default:
			break;
		}
	return(handled);
}

