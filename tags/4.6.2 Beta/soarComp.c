#include <PalmOS.h>	// all the system toolbox headers
#include "soarComp.h"
#include "soaring.h"
#include "soarIO.h"
#include "soarForm.h"
#include "soarMath.h"
#include "soarLog.h"
#include "soarUtil.h"
#include "soarEW.h"
#include "soarCAI.h"
#include "soarVolk.h"
#include "soarCOL.h"
#include "soarFlrm.h"

Boolean BinaryFileXfr = false;
Int8    CompCmd;
Int8	CompCmdRes;
Int16   CompCmdData;

extern Boolean menuopen;
extern Int8 xfrdialog;
extern Int16 cainumLogs;
extern Int16 currentCAIFltPage;
extern Int16 selectedCAIFltIndex;
extern Int16 CAInumperpage;
extern CAILogData *cailogdata;
extern UInt32 cursecs;

// Flight List Event Handler
Boolean form_list_flts_event_handler(EventPtr event)
{
	Boolean handled = false;
	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst;
	Char tempchar[7];
	static UInt32 pollsecs = 0;
	
	lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_flts_list));

	switch (event->eType) {
		case frmOpenEvent:
//		case frmUpdateEvent:
			FrmDrawForm(pfrm);
			menuopen = false;
			selectedCAIFltIndex = -1;
			currentCAIFltPage = 0;
			BinaryFileXfr = false;
			ctl_set_value(form_list_flts_binaryfile, false); 
			switch (data.config.flightcomp) {
				case C302COMP:
				case C302ACOMP:
					if (!DownloadCAILogInfo(CAIFISTART)) {
						//Display an Error Dialog
						FrmCustomAlert(WarningAlert, "Getting Flight Info From C302/CFR Failed"," "," ");
						FrmGotoForm(form_transfer);
					} 
					break;
				case VOLKSCOMP:
				case B50VLKCOMP:
					if (!VLKgetloginfo(CAIFISTART)) {
						//Display an Error Dialog
						FrmCustomAlert(WarningAlert, "Getting Flight Info From Volkslogger Failed"," "," ");
						FrmGotoForm(form_transfer);
					}
					break;
				case LXCOMP:
				case FILSERCOMP:
				case B50LXCOMP:
					if (!COLGetFlightList(CAIFISTART)) {
						//Display an Error Dialog
						FrmCustomAlert(WarningAlert, "Getting Flight Info From LX logger Failed"," "," ");
						FrmGotoForm(form_transfer);
					} 
					break;
				case GPSNAVCOMP:
					if (!DownloadCAILogInfo(CAIFISTART)) {
						//Display an Error Dialog
						FrmCustomAlert(WarningAlert, "Getting Flight Info From GPSNAV Failed"," "," ");
						FrmGotoForm(form_transfer);
					} 
					break;
				case FLARMCOMP:
					if (!FlarmGetFlightList(CAIFISTART)) {
						//Display an Error Dialog
						FrmCustomAlert(WarningAlert, "Getting Flight Info From Flarm logger Failed"," "," ");
						FrmGotoForm(form_transfer);
					} 
					break;
				default:
					break;
			}
			refresh_flts_list(0);
			pollsecs = cursecs;
			handled=true;
			break;
		case ctlSelectEvent:
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_list_flts_getbtn:
					if (data.config.xfertype == USEVFS || data.config.xfertype == USEDOC) {
						switch (data.config.flightcomp) {
							case C302COMP:
							case C302ACOMP:
								if (!(DownloadCAISelectedLog())) {
									//Display an Error Dialog
									FrmCustomAlert(WarningAlert, "Getting Flight From C302/CFR Failed"," "," ");
									FrmGotoForm(form_transfer);
								}
								break;
							case VOLKSCOMP:
							case B50VLKCOMP:
								if (!(VLKDownloadSelectedLog())) {
									//Display an Error Dialog
									FrmCustomAlert(WarningAlert, "Getting Flight From Volkslogger Failed"," "," ");
									FrmGotoForm(form_transfer);
								}
								break;
							case LXCOMP:
							case FILSERCOMP:
							case B50LXCOMP:
								if ((data.config.xfertype != USEVFS) && BinaryFileXfr) {
									FrmCustomAlert(WarningAlert, "Binary Data Xfer Type Selected\n","Select Card From\n","NMEA/Port Config Screen");
								} else if (!(COLDownloadFlight())) {
									//Display an Error Dialog
									FrmCustomAlert(WarningAlert, "Getting Flight From LX logger Failed"," "," ");
									FrmGotoForm(form_transfer);
								}
								break;
							case GPSNAVCOMP:
								if (!(DownloadCAISelectedLog())) {
									//Display an Error Dialog
									FrmCustomAlert(WarningAlert, "Getting Flight From GPSNAV Failed"," "," ");
									FrmGotoForm(form_transfer);
								}
								break;
							case FLARMCOMP:
								if (!(FlarmDownloadFlight())) {
									//Display an Error Dialog
									FrmCustomAlert(WarningAlert, "Getting Flight From Flarm Failed"," "," ");
									FrmGotoForm(form_transfer);
								}
								break;
							default:
								break;
						}
					} else {
						FrmCustomAlert(WarningAlert, "Unsupported Data Xfer Type Selected\n","Select DOC or Card From\n","NMEA/Port Config Screen");
					}
					break;
				case form_list_flts_closebtn:
					FrmGotoForm(form_transfer);
					break;
				case form_list_flts_binaryfile:
					BinaryFileXfr = ctl_get_value(form_list_flts_binaryfile);
					break;
				default:
					break;
			}
			handled=true;
			break;
		case lstSelectEvent:
		{
			switch (data.config.flightcomp) {
				case C302COMP:
				case C302ACOMP:
				case VOLKSCOMP:
				case B50VLKCOMP:
				case LXCOMP:
				case B50LXCOMP:
				case FILSERCOMP:
				case GPSNAVCOMP:
				case FLARMCOMP:
					if (event->data.lstSelect.listID == form_list_flts_list) {
						if (SetSelectedFlt(LstGetSelection(lst))) {
							PlayKeySound();
							ctl_set_visible(form_list_flts_getbtn, true);
							if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == FILSERCOMP) || (data.config.flightcomp == B50LXCOMP)) {
								ctl_set_visible(form_list_flts_binaryfile, true);
							}
						}
					}
					break;
				default:
					break;
			}
			DrawHorizListLines(8, 28, 14);
			handled=true;
			break;
		}
		case nilEvent:
			if (cursecs > pollsecs) {
				pollsecs = cursecs;
				switch (data.config.flightcomp) {
					case LXCOMP:
					case FILSERCOMP:
					case B50LXCOMP:
						// keep LX loggers "alive"
						COLPing();
					break;
				default:
				        break;
				}
			}
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "list_waypt-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "list_waypt-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "list_caiflts-frmCloseEvent");
			switch (data.config.flightcomp) {
				case C302COMP:
				case C302ACOMP:
					DownloadCAILogInfo(CAIFIFREE);
					break;
				case VOLKSCOMP:
				case B50VLKCOMP:
					VLKgetloginfo(CAIFIFREE);
					break;
				case LXCOMP:
				case FILSERCOMP:
				case B50LXCOMP:
					COLGetFlightList(CAIFIFREE);
					break;
				case GPSNAVCOMP:
					DownloadCAILogInfo(CAIFIFREE);
					break;
				case FLARMCOMP:
					FlarmGetFlightList(CAIFIFREE);
					break;
				default:
					break;
			}
			refresh_flts_list(CAIFIFREE);
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
			XferInit(tempchar, NFC, data.config.nmeaxfertype);
			handled=false;
			break;
		default:
			break;
	}
	return(handled);
}

void DeclareTaskToLogger(Int8 CompDecCmd)
{
	Char tempchar[7];
	Int16 tempnumwaypts=0;

	xfrdialog = XFRXFR;
	CompCmdRes = CompDecCmd;

	if (CompDecCmd == DECNOCMD) {
		return;
	}

	if (CompDecCmd == DECSEND) {
		// check minimum number of turnpoints
		if (data.task.numwaypts-data.task.numctlpts < 2) {
			compmsg(TOOFEWWPTS, false);
		}
	}

	switch (data.config.flightcomp) {
	
	// EW Model D logger
	case EWCOMP:
		if (data.config.nmeaspeed != EWspeed) {
			// Don't close port and reopen port if it is already at 9600
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, EWspeed, tempchar, 7);
			XferInit(tempchar, NFC, USESER);
		}
		switch(CompDecCmd) {
			case DECCLEAR:
				compmsg(DECCLEAR, ClearEWDeclaration());
				break;
			case DECSEND:
				if (data.task.numwaypts-data.task.numctlpts <= 6) {
					compmsg(DECSEND, DeclareTaskToEW());
				} else {
					compmsg(TOOMANYWPTS, false);
				}
				break;
			default:
				break;
		}
		if (data.config.nmeaspeed != EWspeed) {
			// Don't close port and reopen port if it is already at the correct speed
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
			XferInit(tempchar, NFC, USESER);
		}
		break;

	// EW MicroRecorder logger
	case EWMRCOMP:
	case EWMRSDCOMP:
	case EWMRSDTASCOMP:
/*
		if (data.config.nmeaspeed != EWspeed) {
			// Don't close port and reopen port if it is already at 9600
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, EWspeed, tempchar, 7);
			XferInit(tempchar, NFC, USESER);
		}
*/
		switch(CompDecCmd) {
			case DECCLEAR:
				compmsg(DECCLEAR, ClearEWMRTask(data.config.declaretoSD));
				break;
			case DECSEND:
				tempnumwaypts = data.task.numwaypts-data.task.numctlpts;
				if (data.task.hastakeoff) {
					tempnumwaypts--;
				}
				if (data.task.haslanding) {
					tempnumwaypts--;
				}

				if (tempnumwaypts <= 12) {
					compmsg(DECSEND, DeclareTaskToEWMR(data.config.declaretoSD));
				} else {
					compmsg(TOOMANYWPTS, false);
				}
				break;
			default:
				break;
		}
		if (data.config.nmeaspeed != EWspeed) {
			// Don't close port and reopen port if it is already at the correct speed
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
			XferInit(tempchar, NFC, USESER);
		}
		break;

	// Cambridge C302/C302A
	case C302COMP:
	case C302ACOMP:
		if (data.config.nmeaspeed != C302speed) {
			// Don't close port and reopen port if it is already at the data xfer speed
			XferClose(data.config.nmeaxfertype);
			SysStringByIndex(form_set_port_speed_table, C302speed, tempchar, 7);
			XferInit(tempchar, NFC, USESER);
		}
		switch(CompDecCmd) {
			case DECSEND:
				compmsg(DECSEND, DeclareCAITask());
			default:
				break;
		}
		if (data.config.nmeaspeed != C302speed) {
			// Don't close port and reopen port if it is already at the correct speed
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
			XferInit(tempchar, NFC, data.config.nmeaxfertype);
		}
		break;

	// Cambridge GPSNAV
	case GPSNAVCOMP:
		if (data.config.nmeaspeed != C302speed) {
			// Don't close port and reopen port if it is already at the data xfer speed
			XferClose(data.config.nmeaxfertype);
			SysStringByIndex(form_set_port_speed_table, C302speed, tempchar, 7);
			XferInit(tempchar, NFC, USESER);
		}
		switch(CompDecCmd) {
			case DECSEND:
				if (data.task.numwaypts-data.task.numctlpts <= 9 ) {
					if (compmsg(SENDWPTS, UploadCAIWaypoints(false))) {
						compmsg(DECSEND, DeclareCAITask());
					}
				} else {
					compmsg(TOOMANYWPTS, false);
				}
				break;
			default:
				break;
		}
		if (data.config.nmeaspeed != C302speed) {
			// Don't close port and reopen port if it is already at the correct speed
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
			XferInit(tempchar, NFC, data.config.nmeaxfertype);
		}
		break;

	// Garrecht Volkslogger
	case VOLKSCOMP:
	case B50VLKCOMP:
		if (data.config.nmeaspeed != VLKspeed) {
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, VLKspeed, tempchar, 7);
			XferInit(tempchar, NFC, USESER);
		}
		switch(CompDecCmd) {
			case DECCLEAR:
				compmsg(DECCLEAR, ClearVLKDeclaration());
				break;
			case DECSEND:
				compmsg(DECSEND, DeclareVLKTask());
				break;
			default:
				break;
		}
		if (data.config.nmeaspeed != VLKspeed) {
			// Don't close port and reopen port if it is already at the correct speed
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
			XferInit(tempchar, NFC, USESER);
		}
		break;

	// LX / Colibri / Filser
	case LXCOMP:
	case FILSERCOMP:
	case B50LXCOMP:
		if (data.config.dataspeed > 7) {
			compmsg(DATASPDERR, false);
			break;
		}
		if (data.config.nmeaspeed != data.config.dataspeed) {
			// Don't close port and reopen port if it is already at the data xfer speed
			XferClose(data.config.nmeaxfertype);
			SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
			XferInit(tempchar, NFC, USESER);
		}
		switch(CompDecCmd) {
			case DECCLEAR:
//				compmsg(DECCLEAR, ClearColibriDeclaration());
				break;
			case DECSEND:
				if (data.task.numwaypts-data.task.numctlpts <= 10 ) {
					compmsg(DECSEND, COLDeclareTask());
				} else {
					compmsg(TOOMANYWPTS, false);
				}
				break;
			default:
				break;
		}
		if (data.config.nmeaspeed != data.config.dataspeed) {
			// Don't close port and reopen port if it is already at the correct speed
			XferClose(USESER);
			SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
			XferInit(tempchar, NFC, data.config.nmeaxfertype);
		}
		break;

	// FLARM IGC
	case FLARMCOMP:
		switch(CompDecCmd) {
			case DECCLEAR:
				compmsg(DECCLEAR, ClearFlarmDeclaration(data.config.declaretoSD));
				break;
			case DECSEND:
				compmsg(DECSEND, DeclareFlarmTask(data.config.declaretoSD));
				break;
			default:
				break;
		}
		break;

	// logger not supported
	default:
		break;
	}

	return;
}

Boolean compmsg(Int8 msgtype, Boolean success)
{
	Char loggername[15];
	
	// check for returned result
	if (CompCmdRes != msgtype) msgtype = CompCmdRes;

	// find logger name
 	if ((data.config.flightcomp == EWMRCOMP) || 
		 (data.config.flightcomp == EWMRSDCOMP) ||
		 (data.config.flightcomp == EWMRSDTASCOMP)) {
		// special case for EWMR or EWMR-SD logger
		SysStringByIndex(form_comp_table, EWMRCOMP, loggername, 15);
		if (data.config.declaretoSD) StrCat(loggername, " SD");
	} if (data.config.flightcomp == FLARMCOMP) {
		SysStringByIndex(form_comp_table, FLARMCOMP, loggername, 15);
		if (data.config.declaretoSD) StrCat(loggername, " SD");
	} else {
		SysStringByIndex(form_comp_table, data.config.flightcomp, loggername, 15);
	}

	// build warning, error or finished message
	switch (msgtype) {
		case DECSEND:
			StrCopy(warning->line1, "Declaration to ");
			StrCat(warning->line1, loggername);
			break;
		case DECCLEAR:
			StrCopy(warning->line1, "Clear ");
			StrCat(warning->line1, loggername);
			StrCat(warning->line1, " Declarion");
			break;
		case SENDWPTS:
			StrCopy(warning->line1, "Sending Waypoints to ");
			StrCat(warning->line1, loggername);
			break;

		// error messages
		case TOOFEWWPTS:
			success = false;
			StrCopy(warning->line1, "Need at least 2 Waypoints");
			break;
		case TOOMANYWPTS:
			success = false;
			StrCopy(warning->line1, "More than ");
			switch (data.config.flightcomp) {
				case EWCOMP:
					StrCat(warning->line1, "6");
					break;
				case EWMRCOMP:
				case EWMRSDCOMP:
				case EWMRSDTASCOMP:
					StrCat(warning->line1, "12");
					break;
				case GPSNAVCOMP:
					StrCat(warning->line1, "9");
					break;
				case LXCOMP:
				case FILSERCOMP:
				case B50LXCOMP:
					StrCat(warning->line1, "10");
					break;
				default:
					break;
			}
			StrCat(warning->line1, " Waypoints");
			break;
		case DATASPDERR:
			success = false;
			StrCopy(warning->line1, "Data Speed Over ");
			SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, loggername, 15);
			StrCat(warning->line1, loggername);
			StrCat(warning->line1, "bps");
			break;
		case CONNECTERR:
			success = false;
			StrCopy(warning->line1, "Connection to ");
			StrCat(warning->line1, loggername);
			break;
		case MEMORYERR:
			success = false;
			StrCopy(warning->line1, "No Enough ");
			StrCat(warning->line1, " Memory");
			break;
		case WPTNOTFOUND:
			success = false;
			StrCopy(warning->line1, trim(data.task.wayptnames[CompCmdData], ' ', true));
			StrCat(warning->line1, " not in ");
			StrCat(warning->line1, loggername);
			break;
			
		default:
			StrCopy(warning->line1, loggername);
			StrCat(warning->line1, " data transfer");
			break;
	}
	
	if (success) {
		warning->type = Wfinished;
		StrCopy(warning->line2, "Succeeded!");
	} else {
		warning->type = Werror;
		StrCopy(warning->line2, "Failed!");
	}

	// popup message
	FrmPopupForm(form_warning);
	return(success);
}	

void refresh_flts_list(Int16 scr)
{
	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst;
	Int16 x;
	Int16 caifltIndex;
	static Char **items = NULL;
	Int16 nrecs;
	Int16 start;
	Int16 end;
	static Int16 prevNumRecs = 0;
	Char pageString[20];
	Char tmpString[20];
	Char dtgstr[15];


	// Free up each of the previous strings and then free up
	// the array of pointers itself.
	for (x = 0; x < prevNumRecs; x++) {
		MemPtrFree(items[x]);
	}
	if (items) {
		MemPtrFree(items);
		items = NULL;
	}

	if (scr != CAIFIFREE) {
		// Get the List pointer
		lst = FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_flts_list));

		// check current page exists
		while (currentCAIFltPage > cainumLogs/CAInumperpage) currentCAIFltPage--;

		// Compute the "page" of CAI Flights we're currently looking at.
		if (scr > 0) {
			if (((currentCAIFltPage + 1) * CAInumperpage) <  cainumLogs) {
				// If there are more waypoints to display, move down one page
				currentCAIFltPage++;
			} else {
				// If at the bottom, wrap to the first page
				currentCAIFltPage = 0;
			}
		} else if (scr < 0)  {
			if (currentCAIFltPage > 0) {
				// If not on the first page of waypoints, move up one page 
				currentCAIFltPage--;
			} else {
				// If at the top, wrap to the last page
				if (cainumLogs == 0) {
					currentCAIFltPage = 0;
				} else if (Fmod((double)cainumLogs,(double)CAInumperpage) == 0.0) {
					currentCAIFltPage = (Int16)(cainumLogs/CAInumperpage) - 1;
				} else {
					currentCAIFltPage = (Int16)(cainumLogs/CAInumperpage);
				}
			}
		}

		// Given the current "page", compute the starting
		//  and ending index and the number of records.
		start = currentCAIFltPage * CAInumperpage;
		end = ((start + CAInumperpage) > cainumLogs) ? cainumLogs : (start + CAInumperpage);
		nrecs = end - start;

		if (nrecs > 0) {
			
			// We got at least one record so allocate enough 
			// memory to hold nrecs pointers-to-pointers
			items = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
			prevNumRecs = nrecs;

			// Loop through each waypoint record
			for (x = 0, caifltIndex = start; caifltIndex < end; caifltIndex++, x++) { 
				// Assign each of the nrecs pointers-to-pointers
				//  the address of a newly allocated 40 character array,
				//  retrieve the waypoint name associated with that record,
				//  and copy that name into the array.
				items[x] = (Char *) MemPtrNew(FILENAMESIZE * (sizeof(Char)));
				MemSet(items[x],FILENAMESIZE,0);
				if (device.romVersion >= SYS_VER_35) {
					DateTemplateToAscii("^0z^2r^4s", cailogdata[caifltIndex].startDate.month, 
									 cailogdata[caifltIndex].startDate.day, 
									 cailogdata[caifltIndex].startDate.year, dtgstr, sizeof(dtgstr));
				} else {
					DateToAscii (cailogdata[caifltIndex].startDate.month, 
									 cailogdata[caifltIndex].startDate.day, 
									 cailogdata[caifltIndex].startDate.year, dfDMYLong, dtgstr);
				}

				StrCopy(items[x], dtgstr);
				StrCat(items[x], "  ");
				TimeToAscii ((UInt8)cailogdata[caifltIndex].startTime.hour, 
								 (UInt8)cailogdata[caifltIndex].startTime.min, tfColon24h, dtgstr);
				if (StrLen(dtgstr) < 5) {
					StrCat(items[x], "0");
				}
				StrCat(items[x], dtgstr);
				StrCat(items[x], "  ");
				StrNCat(items[x], cailogdata[caifltIndex].pilotName, 40);
			}
					
			// Reform the list
			LstSetListChoices(lst, items, nrecs);
		} else {
			items = (char **) MemPtrNew(1 * (sizeof(char *)));
			prevNumRecs = 1;		
			items[0] = (char *) MemPtrNew(FILENAMESIZE * (sizeof(char)));
			MemSet(items[0],FILENAMESIZE,0);
			StrNCopy(items[0], "No Flights", 12);
			LstSetListChoices(lst, items, 1);
			LstSetSelection(lst, 0);
			
			selectedCAIFltIndex = -1;
		}

		if (selectedCAIFltIndex == -1) {
			ctl_set_visible(form_list_flts_getbtn, false);
		} else {
			ctl_set_visible(form_list_flts_getbtn, true);
		}
		ctl_set_visible(form_list_flts_closebtn, true);
	
		// Create the "Page: # of #" string
		MemSet(pageString,20, 0);
		if (cainumLogs == 0) {
			StrCopy(pageString,StrIToA(tmpString,(currentCAIFltPage)));
		} else {
			StrCopy(pageString,StrIToA(tmpString,(currentCAIFltPage+1)));
		}
		StrCat(pageString, " of ");
		StrCat(pageString,StrIToA(tmpString,(cainumLogs % CAInumperpage) ? 
				(((int)(cainumLogs/CAInumperpage)) + 1) : (int)(cainumLogs/CAInumperpage)));
		field_set_value(form_list_flts_page, pageString);
		field_set_value(form_list_flts_nrecs, StrIToA(tmpString,cainumLogs));
	
		// Redraw the list
		if (data.config.listlinesvert > 1) {
			LstDrawList(lst);
		} else {
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_flts_list));
		}
	
		// If the currently selected waypoint is on the currently
		//  displayed page, then darken it as if it were selected.  If not then
		//  de-select everything.
		if ((selectedCAIFltIndex >= (currentCAIFltPage*CAInumperpage)) 
			&& (selectedCAIFltIndex < ((currentCAIFltPage*CAInumperpage) + CAInumperpage))) {
			LstSetSelection(lst, selectedCAIFltIndex % CAInumperpage);
		} else {
			LstSetSelection(lst, -1);
		}
		DrawHorizListLines(8, 28, 14);
	} else {
		prevNumRecs = 0;
	}
}

Boolean SetSelectedFlt(Int16 lstselection)
{
	if (cainumLogs > 0) {
		selectedCAIFltIndex = lstselection + (currentCAIFltPage * CAInumperpage);
//		HostTraceOutputTL(appErrorClass, "selectedCAIFltIndex-|%hd|", selectedCAIFltIndex);
		return(true);
	} else {
		return(false);
	}
}

