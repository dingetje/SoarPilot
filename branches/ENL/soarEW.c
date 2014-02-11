#include <PalmOS.h>	// all the system toolbox headers
#include "soarEW.h"
#include "soarComp.h"
#include "soaring.h"
#include "soarIO.h"
#include "soarUtil.h"
#include "soarDB.h"
#include "soarPLst.h"
#include "soarForm.h"
#include "soarUMap.h"
#include "soarWay.h"
#include "soarMem.h"

extern Boolean recv_data;
extern Boolean	menuopen;

Int8 EWMRxfertype = USESER;
EWMRData *ewmrdata;

/****************************************************************************/
/* Code for EW Model D                                                      */
/****************************************************************************/

Boolean SendEWPreamble()
{
	Char output_char[6];

//	HostTraceOutputTL(appErrorClass, "Sending Preamble");
	// Xmit EW Preamble to put in IO Mode
	output_char[0] = 0x18;
	output_char[1] = '\0';
	TxData(output_char, USESER);
	output_char[0] = 0x18;
	output_char[1] = '\0';
	TxData(output_char, USESER);
	StrCat(output_char, "##\r\n");
	TxData(output_char, USESER);

	if (!(WaitFor("IO Mode.\r\n", USESER)))
		return(false);

	return(true);
}

Boolean SendEWSPI(Boolean blankdate)
{
	Char output_char[81];
	Char tempchar[81];
	DateTimeType tempdt;
	UInt32 tempsecs;

	// Sending SPI Data
//	HostTraceOutputTL(appErrorClass, "Setting SPI Data");
	StrCopy(output_char, "#SPI4A\r\n");
	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	Sleep(0.5);

	//Output the Pilot Name (12 chars padded with spaces)
	StrNCopy(tempchar, data.igchinfo.name, 12);
	tempchar[StrLen(data.igchinfo.name)] = '\0';
	if (StrLen(tempchar) < 12) {
		StrCopy(tempchar, rightpad(tempchar, ' ', 12));
	}
	StrCopy(output_char, tempchar);

	//Output the Glider Type (8 chars padded with spaces)
	StrNCopy(tempchar, data.igchinfo.type, 8);
	tempchar[StrLen(data.igchinfo.type)] = '\0';
	if (StrLen(tempchar) < 8) {
		StrCopy(tempchar, rightpad(tempchar, ' ', 8));
	}
	StrCat(output_char, tempchar);

	//Output the Glider ID (8 chars padded with spaces)
	StrNCopy(tempchar, data.igchinfo.cid, 8);
	tempchar[StrLen(data.igchinfo.cid)] = '\0';
	if (StrLen(tempchar) < 8) {
		StrCopy(tempchar, rightpad(tempchar, ' ', 8));
	}
	StrCat(output_char, tempchar);
	
	//Output the GPS Model (12 chars padded with spaces)
	StrNCopy(tempchar, data.igchinfo.gpsmodel, 12);
	tempchar[StrLen(data.igchinfo.gpsmodel)] = '\0';
	if (StrLen(tempchar) < 12) {
		StrCopy(tempchar, rightpad(tempchar, ' ', 12));
	}
	StrCat(output_char, tempchar);

	//Output the GPS Serial (12 chars padded with spaces)
	StrNCopy(tempchar, data.igchinfo.gpsser, 12);
	tempchar[StrLen(data.igchinfo.gpsser)] = '\0';
	if (StrLen(tempchar) < 12) {
		StrCopy(tempchar, rightpad(tempchar, ' ', 12));
	}
	StrCat(output_char, tempchar);

	//Output the Flight Date (6 chars yymmdd)
	if (blankdate) {
		StrCopy(tempchar, "      ");
	} else {
		if (recv_data) {
			StringToDateAndTime(data.logger.gpsdtg, data.logger.gpsutc, &tempdt);
			tempsecs=TimDateTimeToSeconds(&tempdt);
			SecondsToDateOrTimeString(tempsecs, tempchar, 3, 0);
		} else {
			StrCopy(tempchar,
			SecondsToDateOrTimeString(TimGetSeconds(), tempchar, 3, data.config.timezone*-1));
		}
	}
	StrCat(output_char, tempchar);

	StrCat(output_char, "\r\n");

	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor("OK\r\n", USESER)))
		return(false);

	return(true);
}

Boolean ClearEWWaypoints()
{
	Char output_char[11];
	Char tempchar[11];

	// Clear Current Task Declaration
//	HostTraceOutputTL(appErrorClass, "Clearing Task Declaration");
	StrCopy(tempchar, "CTP00");
	StrCopy(output_char, "#");
	StrCat(output_char, tempchar);
	StrCat(output_char, CalcChkSum(tempchar));
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor("OK\r\n", USESER)))
		return(false);

	StrCopy(tempchar, "CTP01");
	StrCopy(output_char, "#");
	StrCat(output_char, tempchar);
	StrCat(output_char, CalcChkSum(tempchar));
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor("OK\r\n", USESER)))
		return(false);

	StrCopy(tempchar, "CTP02");
	StrCopy(output_char, "#");
	StrCat(output_char, tempchar);
	StrCat(output_char, CalcChkSum(tempchar));
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor("OK\r\n", USESER)))
		return(false);

	StrCopy(tempchar, "CTP03");
	StrCopy(output_char, "#");
	StrCat(output_char, tempchar);
	StrCat(output_char, CalcChkSum(tempchar));
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor("OK\r\n", USESER)))
		return(false);

	StrCopy(tempchar, "CTP04");
	StrCopy(output_char, "#");
	StrCat(output_char, tempchar);
	StrCat(output_char, CalcChkSum(tempchar));
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor("OK\r\n", USESER)))
		return(false);

	StrCopy(tempchar, "CTP05");
	StrCopy(output_char, "#");
	StrCat(output_char, tempchar);
	StrCat(output_char, CalcChkSum(tempchar));
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor("OK\r\n", USESER)))
		return(false);

	return(true);
}

Boolean SendEWWaypoint(UInt16 ewidx, UInt16 wayidx)
{
	Char output_char[81];
	Char tempout[81];
	Char tempchar[20];
	Char tempchar2[20];
	UInt16 LatLonFlag=0;

//	HostTraceOutputTL(appErrorClass, "Sending EW Waypoint-EWidx|%hu| Wayidx|%hu|", ewidx, wayidx);
	StrCopy(tempout, "STP0");
	StrCat(tempout, StrIToA(tempchar, ewidx));
	StrCopy(output_char, "#");
	StrNCopy(tempchar, data.task.wayptnames[wayidx], 6);
	tempchar[6] = '\0';
	if (StrLen(tempchar) < 6) {
		StrCopy(tempchar, rightpad(tempchar, ' ', 6));
	}
	ConvertToUpper(tempchar);
//	HostTraceOutputTL(appErrorClass, "Waypoint Name Before Hex Convert|%s|", tempchar);
	StrCat(tempout, Str2Hex(tempchar));
//	HostTraceOutputTL(appErrorClass, "Converted to Hex|%s|", tempout);

	//=IF(O6="N",1,2)+IF(O7="E",4,8)
	if (data.task.wayptlats[wayidx] >= 0) {
		LatLonFlag = 1;
	} else {
		LatLonFlag = 2;
	}
	if (data.task.wayptlons[wayidx] >= 0) {
		LatLonFlag += 4;
	} else {
		LatLonFlag += 8;
	}
	StrCat(tempout, Dec2Hex(LatLonFlag));
//	HostTraceOutputTL(appErrorClass, "Add LatLon Flag|%s|", tempout);

	//This will output the lat and lon into a string with a colon and two decimal places of minutes
	LLToStringDM(data.task.wayptlats[wayidx], tempchar, ISLAT, true, false, 2);
	tempchar[StrLen(tempchar)-1] = '\0';
	GetFieldDelim(tempchar, 0, ' ', ':', tempchar2);
//	HostTraceOutputTL(appErrorClass, "Lat Degs|%s|", tempchar2);
	StrCat(tempout, Dec2Hex(StrAToI(tempchar2)));
//	HostTraceOutputTL(appErrorClass, "Lat Degs Output|%s|", Dec2Hex(StrAToI(tempchar2)));
	GetFieldDelim(tempchar, 1, ':', ' ', tempchar2);
//	HostTraceOutputTL(appErrorClass, "Lat Mins|%s|", tempchar2);
	StrCat(tempout, leftpad(Dec2Hex(StrAToI(tempchar2)), '0', StrLen(tempchar2)));
//	HostTraceOutputTL(appErrorClass, "Lat Mins Output|%s|", 
//								leftpad(Dec2Hex(StrAToI(tempchar2)), '0', StrLen(tempchar2)));
//	HostTraceOutputTL(appErrorClass, " ");

	LLToStringDM(data.task.wayptlons[wayidx], tempchar, ISLON, true, false, 2);
	tempchar[StrLen(tempchar)-1] = '\0';
	GetFieldDelim(tempchar, 0, ' ', ':', tempchar2);
//	HostTraceOutputTL(appErrorClass, "Lon Degs|%s|", tempchar2);
	StrCat(tempout, Dec2Hex(StrAToI(tempchar2)));
//	HostTraceOutputTL(appErrorClass, "Lon Degs Output|%s|", Dec2Hex(StrAToI(tempchar2)));
	GetFieldDelim(tempchar, 1, ':', ' ', tempchar2);
//	HostTraceOutputTL(appErrorClass, "Lon Mins|%s|", tempchar2);
	StrCat(tempout, leftpad(Dec2Hex(StrAToI(tempchar2)), '0', StrLen(tempchar2)));
//	HostTraceOutputTL(appErrorClass, "Lon Mins Output|%s|", 
//								leftpad(Dec2Hex(StrAToI(tempchar2)), '0', StrLen(tempchar2)));
//	HostTraceOutputTL(appErrorClass, " ");
//	HostTraceOutputTL(appErrorClass, "Add LatLon|%s|", tempout);
	StrCat(tempout, CalcChkSum(tempout));
//	HostTraceOutputTL(appErrorClass, "Add Checksum|%s|", tempout);

	StrCat(output_char, tempout);
//	HostTraceOutputTL(appErrorClass, "Add # To Front|%s|", output_char);

	StrCat(output_char, "\r\n");

	TxData(output_char, USESER);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);
	if (!(WaitFor("OK\r\n", USESER)))
		return(false);

	return(true);
}

Boolean DeclareTaskToEW()
{
	UInt16 TAKEOFFSET=0, LANDOFFSET=0;
	UInt16 numWaypts;
	UInt16 ewidx=1, wayidx=0;

//	HostTraceOutputTL(appErrorClass, "Num Way Pts %s", DblToStr(data.task.numwaypts,0));
//	HostTraceOutputTL(appErrorClass, "Num Ctl Pts %s", DblToStr(data.task.numctlpts,0));
	numWaypts = data.task.numwaypts - data.task.numctlpts;
//	HostTraceOutputTL(appErrorClass, "Num Pts to EW %s", DblToStr(numWaypts,0));

	if (data.task.hastakeoff) {
		TAKEOFFSET = 1;
		numWaypts -= 1;
	}

	if (data.task.haslanding) {
		LANDOFFSET = data.task.numwaypts - 2;
		numWaypts -= 1;
	} else {
		LANDOFFSET = data.task.numwaypts - 1;
	}

	HandleWaitDialog(true);

	// Send the EW Preamble to wakeup the EW
	if (!(SendEWPreamble())) {
		HandleWaitDialog(false);
		return(false);
	}

	// Clear all of the current Waypoints
	if (!(ClearEWWaypoints())) {
		HandleWaitDialog(false);
		return(false);
	}

	// Send the start turnpoint
	if (!(SendEWWaypoint(0, TAKEOFFSET))) {
		HandleWaitDialog(false);
		return(false);
	}

	// Send the finish turnpoint
	if (!(SendEWWaypoint(5, LANDOFFSET))) {
		HandleWaitDialog(false);
		return(false);
	}

	// Send All Turnpoints, ignoring control points
	ewidx = 1;
	for (wayidx=(TAKEOFFSET+1); wayidx<LANDOFFSET; wayidx++) {
		if ((data.task.waypttypes[wayidx] & CONTROL) == 0) {
			if (!(SendEWWaypoint(ewidx, wayidx))) {
				HandleWaitDialog(false);
				return(false);
			}
		ewidx++;
		}
	}

	// Send SPI Data
	if (!(SendEWSPI(false))) {
		HandleWaitDialog(false);
		return(false);
	}

//	HostTraceOutputTL(appErrorClass, "------------------------");
	HandleWaitDialog(false);
	return(true);
}

Boolean ClearEWDeclaration()
{
	HandleWaitDialog(true);

	// Clear all of the current Waypoints
	if (!(ClearEWWaypoints())) {
		HandleWaitDialog(false);
		return(false);
	}

	// Send SPI Data and Clear the Flight Date
	if (!(SendEWSPI(true))) {
		HandleWaitDialog(false);
		return(false);
	}

	HandleWaitDialog(false);
	return(true);
}

/****************************************************************************/
/* Code for EW Micro Recorder                                               */
/****************************************************************************/

Boolean form_config_ewmrinst_event_handler(EventPtr event)
{
	Boolean handled=false;
	Char tempchar[15];
	MemHandle record_handle;
	MemPtr record_ptr;

	switch (event->eType) {
		case frmOpenEvent:
			if (!data.config.declaretoSD || (data.config.flightcomp == EWMRCOMP)) {
				GetEWMRConfig();
			}
		case frmUpdateEvent:
			// Retrieve EWMR Config Data 
			// Using it to store the glider and pilot output toggels
			OpenDBQueryRecord(config_db, EWMRINFO_REC, &record_handle, &record_ptr);
			MemMove(ewmrdata, record_ptr, sizeof(EWMRData));
			MemHandleUnlock(record_handle);
/*
			// Test Values
			StrCopy(ewmrdata->modelname, "MicroRecorder");
			StrCopy(ewmrdata->serialnumber, "560");
			StrCopy(ewmrdata->firmwarever, "6.60");
			StrCopy(ewmrdata->hardwarever, "5.1");
			StrCopy(ewmrdata->secstatus, "NONSECURE");
			StrCopy(ewmrdata->enlfitted, "N");
			ewmrdata->updaterate = 4;
			ewmrdata->datarate = 38400;
			ewmrdata->aotime = 90;
			ewmrdata->aoaltchg = 50;
			ewmrdata->aospdchg = 10;
			ewmrdata->startfilespd = 20;
			ewmrdata->startfilepaltchg = 60;
			ewmrdata->filesync = EWSYNCLAST;
*/
			FrmDrawForm(FrmGetActiveForm());

			if (data.config.flightcomp == EWMRCOMP) {
				frm_set_title("Configure EWMR");
				ctl_set_visible(form_config_ewmrinst_settingslabel, true);
				data.config.declaretoSD = false;
			} else {
				frm_set_title("Configure EWMR-SD");
				ctl_set_visible(form_config_ewmrinst_synclabel, true);
				ctl_set_visible(form_config_ewmrinst_lastbtn, true);
				ctl_set_visible(form_config_ewmrinst_allbtn, true);
				if (ewmrdata->filesync==EWSYNCLAST) { 
					ctl_set_value(form_config_ewmrinst_allbtn, false);
					ctl_set_value(form_config_ewmrinst_lastbtn, true);
				} else {
					ctl_set_value(form_config_ewmrinst_allbtn, true);
					ctl_set_value(form_config_ewmrinst_lastbtn, false);
				}
			}

			ctl_set_value(form_config_ewmrinst_pilotoutbtn, ewmrdata->pilotinfo);
			ctl_set_value(form_config_ewmrinst_glideroutbtn, ewmrdata->gliderinfo);

			//Unchangeable Values
			field_set_value(form_config_ewmrinst_modelname, ewmrdata->modelname);
			field_set_enable(form_config_ewmrinst_modelname, false);
			field_set_value(form_config_ewmrinst_serialnumber, ewmrdata->serialnumber);
			field_set_enable(form_config_ewmrinst_serialnumber, false);
			field_set_value(form_config_ewmrinst_firmwarever, ewmrdata->firmwarever);
			field_set_enable(form_config_ewmrinst_firmwarever, false);
			field_set_value(form_config_ewmrinst_hardwarever, ewmrdata->hardwarever);
			field_set_enable(form_config_ewmrinst_hardwarever, false);
			field_set_value(form_config_ewmrinst_secstatus, ewmrdata->secstatus);
			field_set_enable(form_config_ewmrinst_secstatus, false);
			field_set_value(form_config_ewmrinst_enlfitted, ewmrdata->enlfitted);
			field_set_enable(form_config_ewmrinst_enlfitted, false);

			// Changeable Values
//			field_set_value(form_config_ewmrinst_datarate, StrIToA(tempchar, (Int32)ewmrdata->datarate));
			field_set_value(form_config_ewmrinst_updaterate, StrIToA(tempchar, (Int32)ewmrdata->updaterate));
			field_set_value(form_config_ewmrinst_aotime, StrIToA(tempchar, (Int32)ewmrdata->aotime));
			field_set_value(form_config_ewmrinst_aoaltchg, print_altitude((double)ewmrdata->aoaltchg));
			field_set_value(form_config_ewmrinst_aospdchg, print_horizontal_speed2((double)ewmrdata->aospdchg, 0));
			field_set_value(form_config_ewmrinst_startfilespd, print_horizontal_speed2((double)ewmrdata->startfilespd, 0));
			field_set_value(form_config_ewmrinst_startfilepaltchg, print_altitude((double)ewmrdata->startfilepaltchg));

			WinDrawChars("sec", 3, 67, 59);
			WinDrawChars("min", 3, 67, 81);
			WinDrawChars(data.input.alttext, 2, 147, 81);
			WinDrawChars(data.input.spdtext, 3, 85, 92);
			WinDrawChars(data.input.spdtext, 3, 67, 114);
			WinDrawChars(data.input.alttext, 2, 147, 114);

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
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_config_ewmrinst_savebtn:
//					ewmrdata->datarate = (Int16)field_get_long(form_config_ewmrinst_datarate);
					ewmrdata->updaterate = (Int16)field_get_long(form_config_ewmrinst_updaterate);
//					HostTraceOutputTL(appErrorClass, "updaterate-|%hd|", ewmrdata->updaterate);
					ewmrdata->aotime = (Int16)field_get_long(form_config_ewmrinst_aotime);
//					HostTraceOutputTL(appErrorClass, "aotime-|%hd|", ewmrdata->aotime);
					ewmrdata->aoaltchg = (Int16)(pround((field_get_long(form_config_ewmrinst_aoaltchg)/data.input.altconst), 0));
//					HostTraceOutputTL(appErrorClass, "aoaltchg raw-|%s|", DblToStr(field_get_long(form_config_ewmrinst_aoaltchg)/data.input.altconst, 3));
//					HostTraceOutputTL(appErrorClass, "aoaltchg-|%hd|", ewmrdata->aoaltchg);
					ewmrdata->aospdchg = (Int16)(pround((field_get_long(form_config_ewmrinst_aospdchg)/data.input.spdconst), 0));
//					HostTraceOutputTL(appErrorClass, "aospdchg raw-|%s|", DblToStr(field_get_long(form_config_ewmrinst_aospdchg)/data.input.spdconst, 3));
//					HostTraceOutputTL(appErrorClass, "aospdchg-|%hd|", ewmrdata->aospdchg);
					ewmrdata->startfilespd = (Int16)(pround((field_get_long(form_config_ewmrinst_startfilespd)/data.input.spdconst), 0));
//					HostTraceOutputTL(appErrorClass, "startfilespd raw-|%s|", DblToStr(field_get_long(form_config_ewmrinst_startfilespd)/data.input.spdconst, 3));
//					HostTraceOutputTL(appErrorClass, "startfilespd-|%hd|", ewmrdata->startfilespd);
					ewmrdata->startfilepaltchg = (Int16)(pround((field_get_long(form_config_ewmrinst_startfilepaltchg)/data.input.altconst), 0));
//					HostTraceOutputTL(appErrorClass, "startfilepaltchg raw-|%s|", DblToStr(field_get_long(form_config_ewmrinst_startfilepaltchg)/data.input.altconst, 3));
//					HostTraceOutputTL(appErrorClass, "startfilepaltchg-|%hd|", ewmrdata->startfilepaltchg);
					ewmrdata->filesync = (Boolean)ctl_get_value(form_config_ewmrinst_allbtn);
//					HostTraceOutputTL(appErrorClass, "filesync-|%hd|", ewmrdata->filesync);
					ewmrdata->pilotinfo = (Boolean)ctl_get_value(form_config_ewmrinst_pilotoutbtn);
//					HostTraceOutputTL(appErrorClass, "pilotinfo-|%hd|", ewmrdata->pilotinfo);
					ewmrdata->gliderinfo = (Boolean)ctl_get_value(form_config_ewmrinst_glideroutbtn);
//					HostTraceOutputTL(appErrorClass, "gliderinfo-|%hd|", ewmrdata->gliderinfo);

					OpenDBUpdateRecord(config_db, sizeof(EWMRData), ewmrdata, EWMRINFO_REC);

//					DeclareTaskToEWMR();
					if (!data.config.declaretoSD) SendConfigToEWMR();

					FrmGotoForm(form_set_logger);
					handled = true;
					break;
				case form_config_ewmrinst_cancelbtn:
					FrmGotoForm(form_set_logger);
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

// These functions are used to put the EWMR into it's various modes 
// Config mode is entered by sending a CTRL-B (0x02).
// The EWMR will send the entire current config then send a CTRL-C (0x03).
// Then SP will send a CTRL-X (0x18) to switch the EWMR into download mode.
// SP will then start sending the config.
// SP will send a CTRL-C (0x03) signify it has completed sending the config.
// The EWMR will then reboot with the new config info.
// 
Boolean SendEWMRCommsStart(Boolean skipctrlc)
{
	Char output_char[6];

//	HostTraceOutputTL(appErrorClass, "Starting EWMR Communication");
	// Xmit EW Preamble to put into Download Mode
//	HostTraceOutputTL(appErrorClass, "Sending CTRL-B");
	output_char[0] = 0x02;
	output_char[1] = '\0';
	TxData(output_char, EWMRxfertype);

	if (!skipctrlc) {
//		HostTraceOutputTL(appErrorClass, "Waitfor CTRL-C");
		if (!(WaitForC(0x03, EWMRxfertype)))
			return(false);
	}

	return(true);
}

Boolean SendEWMRDownloadStart()
{
	Char output_char[6];

//	HostTraceOutputTL(appErrorClass, "Sending CTRL-X to start download mode");
	output_char[0] = 0x18;
	output_char[1] = '\0';
	TxData(output_char, EWMRxfertype);

	return(true);
}

Boolean SendEWMRDownloadAbort()
{
	Char output_char[6];

	// Xmit CTRL-B to abort download mode
	// Must be sent after CTRL-X
//	HostTraceOutputTL(appErrorClass, "Sending CTRL-B to cancel download mode");
	output_char[0] = 0x02;
	output_char[1] = '\0';
	TxData(output_char, EWMRxfertype);

//	HostTraceOutputTL(appErrorClass, "Waitfor OK");
	if (!(WaitFor("OK", EWMRxfertype)))
		return(false);

	return(true);
}

Boolean SendEWMRCommsEnd()
{
	Char output_char[6];

//	HostTraceOutputTL(appErrorClass, "Ending EWMR Communication");
//	HostTraceOutputTL(appErrorClass, "Sending CTRL-C");
	// Xmit EW Postamble to put into Normal Mode
	output_char[0] = 0x03;
	output_char[1] = '\0';
	TxData(output_char, EWMRxfertype);

//	HostTraceOutputTL(appErrorClass, "Waitfor OK");
	if (!(WaitFor("OK", EWMRxfertype)))
		return(false);

	return(true);
}

Boolean GetEWMRConfig()
{
	Char tempchar[81];
	MemHandle record_handle;
	MemPtr record_ptr;
	Boolean retval=true;

	// Retrieve EWMR Config Data
	// Using it to store the glider and pilot output toggels
	OpenDBQueryRecord(config_db, EWMRINFO_REC, &record_handle, &record_ptr);
	MemMove(ewmrdata, record_ptr, sizeof(EWMRData));
	MemHandleUnlock(record_handle);

	HandleWaitDialog(true);

//	HostTraceOutputTL(appErrorClass, "Getting Config from EWMR");
	// Send the EWMR Comms Start command
	if (retval && !(SendEWMRCommsStart(true))) {
		retval=false;
	}

//	HostTraceOutputT(appErrorClass, "About To Waitfor Model Name");
	// Get various configuration data from EWMR
	if (retval && !(WaitFor("Model Name:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Model Name-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->modelname,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Serial Number:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Serial Number-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->serialnumber,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Firmware Version:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Firmware Version-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->firmwarever,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Hardware Version:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Hardware Version-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->hardwarever,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Security Status:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Security Status-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->secstatus,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Engine Noise Fitted:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Engine Noise Fitted-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->enlfitted,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (data.config.flightcomp == EWMRSDCOMP ||
		 data.config.flightcomp == EWMRSDTASCOMP) {
		if (retval && !(WaitFor("Sync Flights to SD:", EWMRxfertype)))
			retval=(false);
//		HostTraceOutputT(appErrorClass, "Sync Flights to SD-");
		if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
			retval=(false);
		trim(tempchar, ' ', true);
		ConvertToUpper(tempchar);
		if (StrCompare(tempchar, "LAST") == 0) {
			ewmrdata->filesync = EWSYNCLAST;
		} else {
			ewmrdata->filesync = EWSYNCALL;
		}
//		HostTraceOutputTL(appErrorClass, "|%s|", tempchar);
	}

//	if (retval && !(WaitFor("Debug Level:", EWMRxfertype)))
//		retval=(false);
//	HostTraceOutputT(appErrorClass, "Debug Level-");
//	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
//		retval=(false);
//	StrCopy(ewmrdata->debuglevel,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("GPS Update Rate (secs):", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "GPS Update Rate (secs)-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	ewmrdata->updaterate = (Int16)StrAToI(trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("GPS Baud Rate:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "GPS Baud Rate-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	ewmrdata->datarate = (Int16)StrAToI(trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Auto-Off Time (mins):", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Auto-Off Time (mins)-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	ewmrdata->aotime = (Int16)StrAToI(trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Auto-Off Altitude Change (meters):", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Auto-Off Altitude Change (meters)-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	ewmrdata->aoaltchg = (Int16)(pround((StrAToI(trim(tempchar, ' ', true))/ALTMETCONST), 0));
//	HostTraceOutputTL(appErrorClass, "tempchar-|%s| aoaltchg-|%hd|", trim(tempchar, ' ', true), ewmrdata->aoaltchg);

	if (retval && !(WaitFor("Auto-Off Velocity (kph):", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Auto-Off Velocity (kph)-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	ewmrdata->aospdchg = (Int16)(pround((StrAToI(trim(tempchar, ' ', true))/SPDKPHCONST), 0));
//	HostTraceOutputTL(appErrorClass, "tempchar-|%s| aospdchg-|%hd|", trim(tempchar, ' ', true), ewmrdata->aospdchg);

	if (retval && !(WaitFor("Start File Speed (kph):", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Start File Speed (kph)-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	ewmrdata->startfilespd = (Int16)(pround((StrAToI(trim(tempchar, ' ', true))/SPDKPHCONST), 0));
//	HostTraceOutputTL(appErrorClass, "tempchar-|%s| startfilespd-|%hd|", trim(tempchar, ' ', true), ewmrdata->startfilespd);

	if (retval && !(WaitFor("Start File Pressure Change (meters):", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Start File Pressure Change (meters)-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	ewmrdata->startfilepaltchg = (Int16)(pround((StrAToI(trim(tempchar, ' ', true))/ALTMETCONST), 0));
//	HostTraceOutputTL(appErrorClass, "tempchar-|%s| startfilepaltchg-|%hd|", trim(tempchar, ' ', true), ewmrdata->startfilepaltchg);

	if (retval && !(WaitFor("Pilot Name:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Pilot Name-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->pilotname,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Competition ID:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Competition ID-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->cid,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Aircraft Type:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Aircraft Type-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->atype,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

	if (retval && !(WaitFor("Aircraft ID:", EWMRxfertype)))
		retval=(false);
//	HostTraceOutputT(appErrorClass, "Aircraft ID-");
	if (retval && !(GetDataEOL(tempchar, EWMRxfertype)))
		retval=(false);
	StrCopy(ewmrdata->gid,trim(tempchar, ' ', true));
//	HostTraceOutputTL(appErrorClass, "|%s|",trim(tempchar, ' ', true));

/*
//	HostTraceOutputTL(appErrorClass, "Waitfor CTRL-C");
	if (retval && !(WaitForC(0x03, EWMRxfertype))) {
//		HostTraceOutputTL(appErrorClass, "About to set retval to false");
		retval=(false);
	}
*/

	Sleep(3.0);
	// Send the EWMR Download Start command
//	HostTraceOutputT(appErrorClass, "Sending EWMR Download Start");
	if (retval && !(SendEWMRDownloadStart())) {
		HandleWaitDialog(false);
		retval=(false);
	}

	// Send the EWMR Download Abort command
//	HostTraceOutputT(appErrorClass, "Sending EWMR Download Abort");
	if (retval && !(SendEWMRDownloadAbort())) {
		HandleWaitDialog(false);
		retval=(false);
	}

	if (retval) OpenDBUpdateRecord(config_db, sizeof(EWMRData), ewmrdata, EWMRINFO_REC);

	
	// This sleep is to let the unit reload the config and start ending NMEA data again.
	Sleep(10);

	HandleWaitDialog(false);

	return(retval);
}

Boolean SendConfigToEWMR()
{
	HandleWaitDialog(true);

//	HostTraceOutputTL(appErrorClass, "Starting To Send Config To EWMR");
	// Send the EWMR Comms Download Start Command
//	HostTraceOutputTL(appErrorClass, "Sending Download Start");
	if (!(SendEWMRDownloadStart())) {
		HandleWaitDialog(false);
		return(false);
	}

	Sleep(1);
	// Send the EWMR Config info
//	HostTraceOutputTL(appErrorClass, "Sending Config");
	if (!(SendEWMRConfig(false))) {
		HandleWaitDialog(false);
		return(false);
	}

/*
	// Send the EWMR Task info
//	HostTraceOutputTL(appErrorClass, "Sending Empty Task");
	if (!(SendEWMRTask(true))) {
		HandleWaitDialog(false);
		return(false);
	}
*/

	Sleep(1);
	// Send the EWMR Comms End Command to put the unit back not normal mode
//	HostTraceOutputTL(appErrorClass, "Sending Comms End");
	if (!(SendEWMRCommsEnd())) {
		HandleWaitDialog(false);
		return(false);
	}

	// This sleep is to let the unit reload the config and start ending NMEA data again.
	Sleep(10);

//	HostTraceOutputTL(appErrorClass, "Sending Config To EWMR Completed");
//	HostTraceOutputTL(appErrorClass, "------------------------");
	HandleWaitDialog(false);
	return(true);
}

Boolean SendEWMRConfig(Boolean clearlogs)
{
	Char output_char[81];
	Char tempchar[81];

//	HostTraceOutputTL(appErrorClass, "SendEWMRConfig-About to QueryRecord");
//	if (ewmrdata == NULL) AllocMem((void *)&ewmrdata, sizeof(EWMRData));
	// Retrieve EWMR Data
	// Using it to store the glider and pilot output toggels
//	OpenDBQueryRecord(config_db, EWMRINFO_REC, &record_handle, &record_ptr);
//	MemMove(ewmrdata, record_ptr, sizeof(EWMRData));
//	MemHandleUnlock(record_handle);

//	HostTraceOutputTL(appErrorClass, "Sending EWMR Header Info");
	StrCopy(output_char, "---------------------\r\n");
	TxData(output_char, EWMRxfertype);
	StrCopy(output_char, "EW FLIGHT RECORDER\r\n");
	TxData(output_char, EWMRxfertype);
	StrCopy(output_char, "---------------------\r\n\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "Model Name:       ");
	StrCat(output_char, ewmrdata->modelname);
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "Serial Number:    ");
	StrCat(output_char, ewmrdata->serialnumber);
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "Firmware Version: ");
	StrCat(output_char, ewmrdata->firmwarever);
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "Hardware Version: ");
	StrCat(output_char, ewmrdata->hardwarever);
	StrCat(output_char, "\r\n\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "SECURITY\r\n");
	TxData(output_char, EWMRxfertype);
	StrCopy(output_char, "---------------\r\n\r\n");
	TxData(output_char, EWMRxfertype);
	StrCopy(output_char, "Security Status: ");
	StrCat(output_char, ewmrdata->secstatus);
	StrCat(output_char, "\r\n\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "LOGGER SETTINGS\r\n");
	TxData(output_char, EWMRxfertype);
	StrCopy(output_char, "---------------\r\n\r\n");
	TxData(output_char, EWMRxfertype);

	if (clearlogs) {
//		HostTraceOutputTL(appErrorClass, "EWMR - Deleting Files");
		StrCopy(output_char, "Delete All Files:                    Y\r\n");
	} else {
//		HostTraceOutputTL(appErrorClass, "EWMR - NOT Deleting Files");
		StrCopy(output_char, "Delete All Files:                    N\r\n");
	}
	TxData(output_char, EWMRxfertype);

//	HostTraceOutputTL(appErrorClass, "EWMR - General Settings");
	StrCopy(output_char, "Engine Noise Fitted:                 ");
	StrCat(output_char, ewmrdata->enlfitted);
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	if (data.config.flightcomp == EWMRSDCOMP ||
		 data.config.flightcomp == EWMRSDTASCOMP) {
		StrCopy(output_char, "Sync Flights to SD:                  ");
		if (ewmrdata->filesync==EWSYNCLAST) { 
			StrCat(output_char, "Last");
		} else {
			StrCat(output_char, "All");
		}
		StrCat(output_char, "\r\n");
		TxData(output_char, EWMRxfertype);
	}

//	StrCopy(output_char, "Debug Level:                         ");
//	StrCat(output_char, ewmrdata->debuglevel);
//	StrCat(output_char, "\r\n\r\n");

	StrCopy(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "USER SETTINGS\r\n");
	TxData(output_char, EWMRxfertype);
	StrCopy(output_char, "---------------\r\n\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "GPS Update Rate (secs):              ");
	StrCat(output_char, StrIToA(tempchar, (Int32)ewmrdata->updaterate));
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "GPS Baud Rate:                       ");
	StrCat(output_char, StrIToA(tempchar, (Int32)ewmrdata->datarate));
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "Auto-Off Time (mins):                ");
	StrCat(output_char, StrIToA(tempchar, (Int32)ewmrdata->aotime));
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "Auto-Off Altitude Change (meters):   ");
	StrCat(output_char, StrIToA(tempchar, (Int32)(pround((ewmrdata->aoaltchg * ALTMETCONST), 0))));
	StrCat(output_char, "\r\n");
//	HostTraceOutputTL(appErrorClass, "aoaltchg - |%s|", output_char);
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "Auto-Off Velocity (kph):             ");
	StrCat(output_char, StrIToA(tempchar, (Int32)(pround((ewmrdata->aospdchg * SPDKPHCONST), 0))));
	StrCat(output_char, "\r\n");
//	HostTraceOutputTL(appErrorClass, "aospdchg - |%s|", output_char);
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "Start File Speed (kph):              ");
	StrCat(output_char, StrIToA(tempchar, (Int32)(pround((ewmrdata->startfilespd * SPDKPHCONST), 0))));
	StrCat(output_char, "\r\n");
//	HostTraceOutputTL(appErrorClass, "startfilespd - |%s|", output_char);
	TxData(output_char, EWMRxfertype);

	StrCopy(output_char, "Start File Pressure Change (meters): ");
	StrCat(output_char, StrIToA(tempchar, (Int32)(pround((ewmrdata->startfilepaltchg * ALTMETCONST), 0))));
	StrCat(output_char, "\r\n\r\n");
//	HostTraceOutputTL(appErrorClass, "startfilealtchg - |%s|", output_char);
	TxData(output_char, EWMRxfertype);

//	HostTraceOutputTL(appErrorClass, "EWMR - User Details");
	StrCopy(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);
	StrCopy(output_char, "USER DETAILS\r\n");
	TxData(output_char, EWMRxfertype);
	StrCopy(output_char, "--------------\r\n\r\n");
	TxData(output_char, EWMRxfertype);

	//Output the Pilot Name 
	StrCopy(output_char, "Pilot Name:     ");
	if (ewmrdata->pilotinfo) { 
		StrCopy(tempchar, data.igchinfo.name);
	} else {
		StrCopy(tempchar, ewmrdata->pilotname);
	}
	StrCat(output_char, tempchar);
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	//Output the Glider Contest ID
	StrCopy(output_char, "Competition ID: ");
	if (ewmrdata->pilotinfo) { 
		StrCopy(tempchar, data.igchinfo.cid);
	} else {
		StrCopy(tempchar, ewmrdata->cid);
	}
	StrCat(output_char, tempchar);
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	//Output the Glider Type
	StrCopy(output_char, "Aircraft Type:  ");
	if (ewmrdata->gliderinfo) { 
		StrCopy(tempchar, data.igchinfo.type);
	} else {
		StrCopy(tempchar, ewmrdata->atype);
	}
	StrCat(output_char, tempchar);
	StrCat(output_char, "\r\n");
	TxData(output_char, EWMRxfertype);

	//Output the Glider Registration
	StrCopy(output_char, "Aircraft ID:    ");
	if (ewmrdata->gliderinfo) { 
		StrCopy(tempchar, data.igchinfo.gid);
	} else {
		StrCopy(tempchar, ewmrdata->gid);
	}
	StrCat(output_char, tempchar);
	StrCat(output_char, "\r\n\r\n");
	TxData(output_char, EWMRxfertype);

//	HostTraceOutputTL(appErrorClass, "SendEWMRConfig - About to return");
	return(true);
}

Boolean SendEWMRTask(Boolean cleartask)
{
	Char output_char[256];
	UInt16 TAKEOFFSET=0, LANDOFFSET=0;
	UInt16 turnpts;
	Int16 wayidx=0;

//	HostTraceOutputTL(appErrorClass, "EWMR - Flight Declaration");
	// All turnpoints in the task.
	// This includes any takeoff and landing points
	// and the start and finish points.
	// Have to get rid of those.
	turnpts = data.task.numwaypts-data.task.numctlpts;
	turnpts--; // For Start point
	turnpts--; // For Finish point
	
	if (data.task.hastakeoff) {
		TAKEOFFSET = 1;
		turnpts--; // For takeoff point
	}
	if (data.task.haslanding) {
		LANDOFFSET = data.task.numwaypts - 1;
		turnpts--; // For landing point
	} else {
		LANDOFFSET = data.task.numwaypts;
	}

	StrCopy(output_char, "FLIGHT DECLARATION\r\n");
	TxData(output_char, EWMRxfertype);
	StrCopy(output_char, "-------------------\r\n\r\n");

	if (!cleartask) {
//		HostTraceOutputTL(appErrorClass, "EWMR - Sending Task");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "Description:      ");
		StrCat(output_char, data.task.name);
		StrCat(output_char, "\r\n");
		TxData(output_char, EWMRxfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);

		if (data.task.hastakeoff) {
			StrCopy(output_char, "Take Off LatLong: ");
			SendEWMRTaskpoint(0, output_char); 
		} else {
			StrCopy(output_char, "Take Off LatLong: 0000000N00000000E TAKE OFF\r\n");
			TxData(output_char, EWMRxfertype);
//			HostTraceOutputTL(appErrorClass, "%s", output_char);
		}

		StrCopy(output_char, "Start LatLon:     ");
		SendEWMRTaskpoint(TAKEOFFSET, output_char); 

		// This gets the declared turnpoints
		for (wayidx=(TAKEOFFSET+1); wayidx<LANDOFFSET-1; wayidx++) {
			if ((data.task.waypttypes[wayidx] & CONTROL) == 0) {
				StrCopy(output_char, "TP LatLon:        ");
				if (!SendEWMRTaskpoint(wayidx, output_char)) {
					return(false);
					// This gets us out of the loop
					wayidx = LANDOFFSET;
				}
			}
		}

		// This fills in the rest of the turnpoint declaration with blank turnpoints
		for (wayidx=(10-turnpts); wayidx>0; wayidx--) {
			StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
			TxData(output_char, EWMRxfertype);
//			HostTraceOutputTL(appErrorClass, "%s", output_char);
		}
		StrCopy(output_char, "Finish LatLon:    ");
		SendEWMRTaskpoint(LANDOFFSET-1, output_char); 

		if (data.task.haslanding) {
			StrCopy(output_char, "Land LatLon:      ");
			SendEWMRTaskpoint(LANDOFFSET, output_char); 
		} else {
			StrCopy(output_char, "Land LatLon:      0000000N00000000W LAND\r\n");
			TxData(output_char, EWMRxfertype);
//			HostTraceOutputTL(appErrorClass, "%s", output_char);
		}
	} else {
//		HostTraceOutputTL(appErrorClass, "EWMR - Clear Task");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "Description:      Empty Task\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "Take Off LatLong: 0000000N00000000E TAKE OFF\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "Start LatLon:     0000000N00000000E START\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "TP LatLon:        0000000N00000000E TURN POINT\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "Finish LatLon:    0000000N00000000E FINISH\r\n");
		TxData(output_char, EWMRxfertype);
		StrCopy(output_char, "Land LatLon:      0000000N00000000E LAND\r\n");
		TxData(output_char, EWMRxfertype);
	}

	return(true);
}

Boolean SendEWMRTaskpoint(UInt16 wayidx, Char *output_char)
{
	Char tempchar[20];

//	HostTraceOutputTL(appErrorClass, "Sending EWMR Waypoint-wayidx|%hu|", wayidx);
	LLToStringDM(data.task.wayptlats[wayidx], tempchar, ISLAT, false, false, 3);
	StrCat(output_char, tempchar);
	LLToStringDM(data.task.wayptlons[wayidx], tempchar, ISLON, false, false, 3);
	StrCat(output_char, tempchar);
	StrCat(output_char, " ");
	StrCat(output_char, data.task.wayptnames[wayidx]);
	StrCat(output_char, "\r\n");

	TxData(output_char, EWMRxfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	return(true);
}

Boolean DeclareTaskToEWMR(Boolean declaretoSD)
{
	HandleWaitDialog(true);

//	HostTraceOutputTL(appErrorClass, "Starting to Declare To EWMR");
	if (!declaretoSD) {
		// Send the EWMR Comms Download Start Command
		if (!(SendEWMRDownloadStart())) {
			HandleWaitDialog(false);
			return(false);
		}
		Sleep(1);

	} else {
		if (!device.CardRW) {
			HandleWaitDialog(false);
			return(false);
		}
		// declaring to SD card
		EWMRxfertype = USEVFS;
		XferInit("EW-USER.TXT", IOOPENTRUNC, EWMRxfertype);
	}

	// Send the EWMR Config info
	if (!(SendEWMRConfig(false))) {
		HandleWaitDialog(false);
		return(false);
	}


	// Send the EWMR Task info
	if (!(SendEWMRTask(false))) {
		HandleWaitDialog(false);
		return(false);
	}

	if (!declaretoSD) {
		Sleep(1);
		// Send the EWMR Comms End Command to put the unit back not normal mode
		if (!(SendEWMRCommsEnd())) {
			HandleWaitDialog(false);
			return(false);
		}
		// This sleep is to let the unit reload the config and start ending NMEA data again.
		Sleep(10);
	} else {
		// close SD file
		XferClose(EWMRxfertype);
		if (!VFSFileCopy("/PALM/Programs/SoarPilot/EW-USER.TXT", "/EW-USER.TXT")) {
 			HandleWaitDialog(false);
			return(false);
		}
		EWMRxfertype = USESER;
	}

//	HostTraceOutputTL(appErrorClass, "EWMR Declaration Completed");
//	HostTraceOutputTL(appErrorClass, "------------------------");
	HandleWaitDialog(false);
	return(true);
}

Boolean ClearEWMRTask(Boolean declaretoSD)
{
	HandleWaitDialog(true);

//	HostTraceOutputTL(appErrorClass, "Starting to Clear EWMR Task");
	if (!declaretoSD) {
		// Send the EWMR Comms Download Start Command
		if (!(SendEWMRDownloadStart())) {
			HandleWaitDialog(false);
			return(false);
		}
		Sleep(1);

	} else {
		if (!device.CardRW) {
			HandleWaitDialog(false);
			return(false);
		}
		EWMRxfertype = USEVFS;
		XferInit("EW-USER.TXT", IOOPENTRUNC, EWMRxfertype);
	}

	// Send the EWMR Config & Task info
	// Have to send config otherwise task
	// does not get cleared properly
	if (!(SendEWMRConfig(false))) {
		HandleWaitDialog(false);
		return(false);
	}

	// Send the EWMR Task info clearing declaration
	if (!(SendEWMRTask(true))) {
		HandleWaitDialog(false);
		return(false);
	}

	if (!declaretoSD) {
		Sleep(1);
		// Send the EWMR Comms End Command to put the unit back not normal mode
		if (!(SendEWMRCommsEnd())) {
			HandleWaitDialog(false);
			return(false);
		}
		// This sleep is to let the unit reload the config and start ending NMEA data again.
		Sleep(10);
	} else {
		// close SD file
		XferClose(EWMRxfertype);
		if (!VFSFileCopy("/PALM/Programs/SoarPilot/EW-USER.TXT", "/EW-USER.TXT")) {
 			HandleWaitDialog(false);
			return(false);
		}
		EWMRxfertype = USESER;
	}

//	HostTraceOutputTL(appErrorClass, "EWMR Declaration Cleared");
//	HostTraceOutputTL(appErrorClass, "------------------------");
	HandleWaitDialog(false);
	return(true);
}

Boolean ClearEWMRLogs()
{
	HandleWaitDialog(true);

//	HostTraceOutputTL(appErrorClass, "Starting to Clear EWMR Logs");
	// Send the EWMR Comms Download Start Command
	if (!(SendEWMRDownloadStart())) {
		HandleWaitDialog(false);
		return(false);
	}

	Sleep(1);
	// Send the EWMR Config info clearing all flight logs
	if (!(SendEWMRConfig(true))) {
		HandleWaitDialog(false);
		return(false);
	}

	Sleep(1);
	// Send the EWMR Comms End Command to put the unit back not normal mode
	if (!(SendEWMRCommsEnd())) {
		HandleWaitDialog(false);
		return(false);
	}

	// This sleep is to let the unit reload the config and start ending NMEA data again.
	Sleep(10);

//	HostTraceOutputTL(appErrorClass, "EWMR Logs Cleared");
//	HostTraceOutputTL(appErrorClass, "------------------------");
	HandleWaitDialog(false);
	return(true);
}

