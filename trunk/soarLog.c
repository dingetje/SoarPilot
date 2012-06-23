#include <PalmOS.h>	// all the system toolbox headers
#include "soaring.h"
#include "soarLog.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarDB.h"
#include "soarIO.h"
#include "soarUMap.h"
#include "soarSHA.h"
#include "soarSHAAdd.h"
#include "soarWind.h"
#include "soarTask.h"
#include "soarWay.h"
#include "soarComp.h"
#include "soarRECO.h"
#include "soarENL.h"

FlightData *fltdata;
Int16 selectedFltindex = 0;
Boolean logactive = false;
Boolean saveqfe = false;
Int16 activefltindex = NO_RECORD;
UInt32 thermalcount = 0;
Boolean atlogspd = false;
double logmapscale;
Int8 logmapscaleidx;
Int8 logmaporient;
Boolean inflight = false;
Boolean recordevent = false;
UInt32 lastevent = 0;

extern Boolean  recv_data;
extern Boolean  clearrect;
extern Boolean  tasknotfinished;
extern Boolean  mustActivateAsNew;
extern Int16    activetskway;
extern RECOData *recodata;
extern Int16 activectlpts;
extern Boolean saveqfe;
extern Int8 CompCmd;
extern UInt32 cursecs;
extern UInt32 utcsecs;
extern TaskSave tsksav;
extern UInt32 nogpstime;

void PerformLogging(Boolean progexit)
{
	static Int32 logstopdate = 0;
	static Int32 flightstopdate = 0;
	Boolean fgp_active=false;
	Boolean gpstatwrite = false;
	UInt16 logindex = NO_RECORD;
	Char tempchar[10];
	RectangleType rectP;
	static Char prevgpsutc[10];
	Int8 i;

	if (FrmGetActiveFormID() == form_final_glide) {
		fgp_active = true;
	}

	// Do I need to use this or will the inverse status just overwrite?
	if (recv_data && StrCompare(data.logger.gpsstat, "A") == 0) {
		gpstatwrite = true;
	}

	if (progexit) { 
//		HostTraceOutputTL(appErrorClass, "Stop Logging - Program Exit");
		// Stop Logging Because the program is exiting or Stop button pressed
//		if (atlogspd) {
		if (activefltindex != NO_RECORD) {      // a flight record is open
			StopLogging();
			// Send stop message for Program Exit
			SendSMS(SMSSENDPROGEND);
		} else {
			// only clear saveqfe flag if not in flight
			saveqfe = false;
		}
		logactive = false;
		inflight = false;
		if (fgp_active) {
			rectP.topLeft.x = GPSX;
			rectP.topLeft.y = GPSY;
			rectP.extent.x = GPSXOFF+1;
			rectP.extent.y = GPSYOFF;
			WinEraseRectangle(&rectP, 0);
			field_set_value(form_final_glide_gpsstat, " ");
			if (!recv_data || (nogpstime > 0)) {
				FntSetFont(boldFont);
				WinDrawInvertedChars(" NO GPS", 7, GPSX, GPSY);
				FntSetFont(stdFont);
				clearrect = true;
			} else if (StrCompare(data.logger.gpsstat, "V") == 0) {
				// Are both of these needed?
				field_set_value(form_final_glide_gpsstat, "NO SATS");
				FntSetFont(boldFont);
				WinDrawInvertedChars("NOSATS", 6, GPSX, GPSY);
				FntSetFont(stdFont);
				clearrect = true;
			} else {
				if (clearrect) {
					WinEraseRectangle(&rectP, 0);
					clearrect = false;
				}
				WinEraseChars("LS", 2, LOGX, LOGY);
				WinEraseChars("LW", 2, LOGX, LOGY);
				WinDrawChars("LS", 2, LOGX, LOGY);
				StrCopy(tempchar, "G");
				StrCat(tempchar, data.input.gpsnumsats);
				field_set_value(form_final_glide_gpsstat, tempchar);
			}
		}
		return;
	} else {
		if (data.input.logtrkdate > cursecs) data.input.logtrkdate = cursecs;
		// set inflight flag
		if (data.input.ground_speed.value >= data.config.logstartspd) {
			inflight = true;
			flightstopdate = cursecs;
			// Save Track Trail Info
			if (cursecs >= (UInt32)(data.input.logtrkdate + data.config.fastlogint))  {
				SaveTrkTrail();
				data.input.logtrkdate = cursecs;
			}
		} else if ((data.input.ground_speed.value < data.config.logstopspd) &&
			   (cursecs > (UInt32)(logstopdate + data.config.logstoptime))) {
			inflight = false;	
		}
		if (data.config.logonoff) {
			// Begin logging
			if ((data.input.ground_speed.value > data.config.logstartspd) && (device.lowbatlevel > LOWBATSTOPLEVEL)) {
				if (atlogspd == false) {
//					HostTraceOutputTL(appErrorClass, "Start Logging");
					StrCopy(data.flight.startutc, data.logger.gpsutc);
					StrCopy(data.flight.startdtg, data.logger.gpsdtg);
					StrCopy(data.flight.stoputc, "NT");
					StrCopy(data.flight.stopdtg, "NT");
					data.flight.maxalt = -999.9;
					data.input.maxalt = 0.0;
					data.flight.minalt = -999.9;
					data.input.minalt = 0.0;
					// copies IGC info at the start of logging
					data.flight.igchinfo = data.igchinfo;
					if (StrCompare(data.logger.gpsstat, "A") != 0) {
						StrCopy(data.logger.gpsstat, "V");
					}
					if (data.task.numwaypts > 0) {
						HandleTaskAutoZoom(0.0, 0.0, true);
						if (tasknotfinished && (data.activetask.tskstartsecs > 0)) {
							if (data.task.hastakeoff) {
								data.logger.taskleg = activetskway-1-activectlpts;
//								HostTraceOutputTL(appErrorClass, "Begin hastakeoff- taskleg:|%hd|", data.logger.taskleg);
							} else {
								data.logger.taskleg = activetskway-activectlpts;
//								HostTraceOutputTL(appErrorClass, "Begin no hastakeoff- taskleg:|%hd|", data.logger.taskleg);
							}
						} else {
							data.logger.taskleg = -1;
//							HostTraceOutputTL(appErrorClass, "Begin task not active- taskleg:|%hd|", data.logger.taskleg);
						}
					} else {
						data.logger.taskleg = -1;
//						HostTraceOutputTL(appErrorClass, "Begin no task defined- taskleg:|%hd|", data.logger.taskleg);
					}
					lastevent = 0;
					recordevent = false;
					data.logger.terelev = data.input.terelev;
					data.logger.eph = data.input.eph;
					data.logger.siu = data.input.siu;
					if (data.config.flightcomp == RECOCOMP) {
						data.logger.enl = recodata->enl;
						StrCopy(data.flight.fcserial, recodata->recoserial);
						StrCopy(data.flight.fcfirmwarever, recodata->recofirmwarever);
					} else {
						initENL(false);
						data.logger.enl = getENL();
						StrCopy(data.flight.fcserial, "0");
						StrCopy(data.flight.fcfirmwarever, "0");
					}
					for (i=0; i<12; i++) {
						data.logger.svns[i] = data.input.svns[i];
					}

					logindex = OpenDBAddRecord(logger_db, sizeof(LoggerData), &data.logger, false);
					if (logindex != NO_RECORD) {
						// logger record created OK
						data.flight.startidx = logindex;
						data.flight.stopidx = logindex;
						StrCopy(data.flight.tskstartutc, "NT");
						StrCopy(data.flight.tskstartdtg, "NT");
						StrCopy(data.flight.tskstoputc, "NT");
						StrCopy(data.flight.tskstopdtg, "NT");
						data.flight.tskdist = 0.0;
						data.activetask.tskstartsecs = 0;
						data.activetask.tskstopsecs = 0;
						data.flight.flightcomp = data.config.flightcomp;
						data.flight.valid = true;
						data.input.logpolldate = cursecs;
						activefltindex = OpenDBAddRecord(flight_db, sizeof(FlightData), &data.flight, false);
						if (activefltindex == NO_RECORD) {
							// remove orphan logger record
							OpenDBDeleteRecord(logger_db, logindex);
						} else {
							data.input.logfltupd = cursecs;
						}
					} else {
						// set activefltindex as invalid
						activefltindex = NO_RECORD;
					}

					if (fgp_active) {
						WinEraseChars("LS", 2, LOGX, LOGY);
						WinEraseChars("LW", 2, LOGX, LOGY);
						WinDrawChars("LW", 2, LOGX, LOGY);
					}
					// Since it is the start of a flight, reset the thermal posit count
					thermalcount = 0;

					// check auto zero QFE option
					if (data.config.autozeroQFE && !saveqfe) {
						if ((data.config.pressaltsrc != GPSALT) && data.config.usepalt) {
							data.config.qfealt = data.logger.pressalt + data.input.QNHaltcorrect;
						} else {
							data.config.qfealt = data.input.inusealt;
						}
					}
					saveqfe = true; // set to protect qfe setting while in flight

					// update field elevation field if on QNH/QFE screen
					if (FrmGetActiveFormID() == form_set_qnh) {
						field_set_value(form_set_qnh_fieldelev, print_altitude(data.config.qfealt));
					}

					// Need to reset the SMS timer variables
					data.input.logsmsdate = cursecs;

					// If configured to send SMS messages
					// send start message.
					SendSMS(SMSSENDTO);
				}
				if (activefltindex == NO_RECORD) {
					// flight and/or logger record not created OK
					logactive = false;
					atlogspd = false;
				} else {
					// flight and logger records created OK
					logactive = true;
					atlogspd = true;
				}
				logstopdate=cursecs;
			} else {
				if (data.input.ground_speed.value < data.config.logstopspd && data.config.logautooff) {
					// Stop Logging because you stopped
					if (atlogspd) {
						if (cursecs > (UInt32)(logstopdate+data.config.logstoptime)) {
							StopLogging();
							// Send stop message for End Of Flight
							SendSMS(SMSSENDLAND);
						}
					} 
				} else {
					if (atlogspd) {
						logstopdate = cursecs;
					} 
				}
			}
			if (atlogspd) {
				// Normal logging done here
				// check for change in Palm time due to GPS time update
				if (data.input.logsmsdate > cursecs) data.input.logsmsdate = cursecs;
				if (data.input.logpolldate > cursecs) data.input.logpolldate = cursecs;
				if (data.input.logfltupd > cursecs) data.input.logfltupd = cursecs;

				// update flight record every minute to avoid data loss
				if ((cursecs > data.input.logfltupd + 60) && (activefltindex >= 0)) {
//					HostTraceOutputTL(appErrorClass, "Save Flight Record");
					OpenDBUpdateRecord(flight_db, sizeof(FlightData), &data.flight, activefltindex);
					data.input.logfltupd = cursecs;
				}
				// SMS Send for periodic send mode
				if (cursecs >= (UInt32)(data.input.logsmsdate + data.config.SMSsendint))  {
					// Send periodic message 
					SendSMS(SMSSENDPERIOD);
					data.input.logsmsdate = cursecs;
				}
				if (cursecs >= (UInt32)(data.input.logpolldate + data.input.logpollint)
					 && StrCompare(data.logger.gpsutc, prevgpsutc) != 0) {
					if (StrCompare(data.logger.gpsstat, "A") != 0) {
						StrCopy(data.logger.gpsstat, "V");
					}

					if (data.task.numwaypts > 0) {
						if (tasknotfinished && (data.activetask.tskstartsecs > 0)) {
							if (data.task.hastakeoff) {
								data.logger.taskleg = activetskway-1-activectlpts;
//								HostTraceOutputTL(appErrorClass, "Normal hastakeoff- taskleg:|%hd|", data.logger.taskleg);
							} else {
								data.logger.taskleg = activetskway-activectlpts;
//								HostTraceOutputTL(appErrorClass, "Normal no hastakeoff- taskleg:|%hd|", data.logger.taskleg);
							}
						} else {
							data.logger.taskleg = -1;
//							HostTraceOutputTL(appErrorClass, "Normal task not active- taskleg:|%hd|", data.logger.taskleg);
						}
					} else {
						data.logger.taskleg = -1;
//						HostTraceOutputTL(appErrorClass, "Normal no task defined- taskleg:|%hd|", data.logger.taskleg);
					}

					data.logger.event = recordevent;
					if (recordevent) {
						recordevent = false;
						lastevent = utcsecs;
					}
					data.logger.terelev = data.input.terelev;
					data.logger.eph = data.input.eph;
					data.logger.siu = data.input.siu;
					if (data.config.flightcomp == RECOCOMP) {
						data.logger.enl = recodata->enl;
					} else {
						data.logger.enl = getENL();
					}
					for (i=0; i<12; i++) {
						data.logger.svns[i] = data.input.svns[i];
					}

					// Add a new GPS update to the logger database
					logindex = OpenDBAddRecord(logger_db, sizeof(LoggerData), &data.logger, false);
					if (logindex != NO_RECORD) data.flight.stopidx = logindex; // update end point if logger record created OK
					data.input.logpolldate = cursecs;	// new time so we don't continuously loop on a GPE error
					StrCopy(prevgpsutc, data.logger.gpsutc);

					if (fgp_active) {
						WinEraseChars("LS", 2, LOGX, LOGY);
						WinEraseChars("LW", 2, LOGX, LOGY);
						WinDrawChars("LW", 2, LOGX, LOGY);
// TEST CODE TO ENL
//						field_set_value(form_final_glide_wayelev, DblToStr(data.logger.enl,1));
					}
					// If in THERMAL mode, increment the thermal posit count to allow for determining
					// the thermal vs. cruise percentage
					if (data.logger.thermal == THERMAL) {
						thermalcount++;
					}
				}
				logactive = true;
			} else {
				if (fgp_active && gpstatwrite) {
					WinEraseChars("LS", 2, LOGX, LOGY);
					WinEraseChars("LW", 2, LOGX, LOGY);
					WinDrawChars("LS", 2, LOGX, LOGY);
				}
			}
		} else {
			// Stop Logging because the logger on/off checkbox was unchecked
			if (atlogspd) {
				StopLogging();
			} 
			if (fgp_active && gpstatwrite) {
				if (data.config.logonoff) {
					WinEraseChars("LS", 2, LOGX, LOGY);
					WinEraseChars("LW", 2, LOGX, LOGY);
					WinDrawChars("LS", 2, LOGX, LOGY);
				} else {
					WinEraseChars("LS", 2, LOGX, LOGY);
					WinEraseChars("LW", 2, LOGX, LOGY);
				}
			}
		}
		return;
	}
}

void MakeNewFlight()
{
	// save QFE setting
	saveqfe = true;
	// recall task data from previous flight
	StrCopy(data.flight.tskstartutc, tsksav.tskstartutc);
	StrCopy(data.flight.tskstartdtg, tsksav.tskstartdtg);
	StrCopy(data.flight.tskstoputc, tsksav.tskstoputc);
	StrCopy(data.flight.tskstopdtg, tsksav.tskstopdtg);
	data.flight.tskdist = tsksav.tskdist;
	// stop & start logger to make new flight and declare task
	PerformLogging(true);
	// make new task declaration
	CopyTaskToFlight();
	mustActivateAsNew = false;
	tasknotfinished = true;
	return;
}

void OutputIGCHeader(FlightData *fltdata)
{ 
	Char output_char[80];
	Char tempchar[16];
	DateTimeType IGCDate;
	Int16 fltnum = 1;

	// output flight number with "A" record
	StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &IGCDate);
	fltnum = FindFltNumOfDay(fltdata->startdtg, IGCDate.hour, IGCDate.minute, IGCDate.second);

	MemSet(output_char, sizeof(output_char), 0);
	StrCopy(output_char, "A");
	if (fltdata->flightcomp == RECOCOMP) {
		StrCat(output_char, "REC");
	StrCat(output_char, fltdata->fcserial);
	} else {
		StrCat(output_char, "XSP");
		StrCat(output_char, GenerateUID(GetSysRomID()));
	}
	StrCat(output_char, "FLIGHT:");
	StrCat(output_char, DblToStr(fltnum, 0));
	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "HFDTE");
	StrCat(output_char, fltdata->startdtg);
	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "HFFXA050");
	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	if (trim(fltdata->igchinfo.name, ' ', true)) {
		StrCopy(output_char, "HFPLTPILOT:");
		StrCat(output_char, fltdata->igchinfo.name);
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	if (trim(fltdata->igchinfo.type, ' ', true)) {
		StrCopy(output_char, "HFGTYGLIDERTYPE:");
		StrCat(output_char, fltdata->igchinfo.type);
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	if (trim(fltdata->igchinfo.gid, ' ', true)) {
		StrCopy(output_char, "HFGIDGLIDERID:");
		StrCat(output_char, fltdata->igchinfo.gid);
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	StrCopy(output_char, "HFDTM100GPSDATUM:WGS-1984");
	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "HFFTYFRTYPE:");
	if (fltdata->flightcomp == RECOCOMP) {
		SysStringByIndex(form_comp_table, RECOCOMP, tempchar, 15);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
		StrCat(output_char, fltdata->fcfirmwarever);
	} else {
		StrCat(output_char, "SoaringPilot,");
		StrCat(output_char, device.appVersion);
	}
	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "HFRFWFIRMWAREVERSION:");
	if (fltdata->flightcomp == RECOCOMP) {
		StrCat(output_char, fltdata->fcfirmwarever);
	} else {
		StrCat(output_char, device.appVersion);
	}
	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	StrCopy(output_char, "HFRHWHARDWAREVERSION:");
	if (fltdata->flightcomp == RECOCOMP) {
		StrCat(output_char, fltdata->fcserial);
	} else {
//		HostTraceOutputTL(appErrorClass, "SerialID:|%s|", GetSysRomID());
		StrCat(output_char, GetSysRomID());
	}
	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	if (trim(fltdata->igchinfo.gpsmodel, ' ', true)) {
		StrCopy(output_char, "HFGPS:");
		StrCat(output_char, fltdata->igchinfo.gpsmodel);
		StrCat(output_char, ",");
		StrCat(output_char, fltdata->igchinfo.gpsser);
		StrCat(output_char, ",12");
		StrCat(output_char, ",18000");
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	if (fltdata->flightcomp == RECOCOMP) {
		StrCopy(output_char, "HFPRSPRESSALTSENSOR:");
		StrCat(output_char, "Intersema");
		StrCat(output_char, ",");
		StrCat(output_char, "MS5534A");
		StrCat(output_char, ",9000");
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	if (trim(fltdata->igchinfo.cid, ' ', true)) {
		StrCopy(output_char, "HFCIDCOMPETITIONID:");
		StrCat(output_char, fltdata->igchinfo.cid);
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	if (trim(fltdata->igchinfo.cls, ' ', true)) {
		StrCopy(output_char, "HFCCLCOMPETITIONCLASS:");
		StrCat(output_char, fltdata->igchinfo.cls);
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//	 	HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	if (trim(fltdata->igchinfo.site, ' ', true)) {
		StrCopy(output_char, "HFSITSITE:");
		StrCat(output_char, fltdata->igchinfo.site);
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	if (trim(fltdata->igchinfo.ooid, ' ', true)) {
		StrCopy(output_char, "HFOOIOOID:");
		StrCat(output_char, fltdata->igchinfo.ooid);
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	StrCopy(output_char, "HFTZNTIMEZONE:");
	StrCat(output_char, StrIToA(tempchar, (Int32)data.config.timezone));
	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// Output an HFFRS Invalid Record saying the flight is invalid if the
	// power is disconnected from the PDA when configured for a RECO
	if (fltdata->valid == false && fltdata->flightcomp == RECOCOMP) {
		StrCopy(output_char, "HFFRS SECURITY SUSPECT USE VALI PROGRAM:Power Loss During Flight");
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	// Output Engine Noise Level?
	if (fltdata->flightcomp == RECOCOMP || data.config.logenl) {
		StrCopy(output_char, "I033638FXA3940SIU4143ENL");
	} else {
		StrCopy(output_char, "I023638FXA3940SIU");
	}

	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	return;
}

void OutputIGCFltLRecs(FlightData *fltdata)
{
	Char output_char[80];
	Char tempchar[30];
	Int16 wayindex=0;
	Int16 nrecs;
	MemHandle output_hand;
	MemPtr output_ptr;
	WaypointData waydata;
	UInt16 y;

	nrecs = OpenDBCountRecords(waypoint_db);

	if (fltdata->flightcomp == RECOCOMP) {
		StrCopy(output_char, "LRECNSOARING PILOT IGC OUTPUT");
		StrCatEOL(output_char, data.config.xfertype);
	} else {
		StrCopy(output_char, "LXSPNSOARING PILOT IGC OUTPUT");
		StrCatEOL(output_char, data.config.xfertype);
	}
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	for (wayindex=0; wayindex<nrecs; wayindex++) {
		OpenDBQueryRecord(waypoint_db, wayindex, &output_hand, &output_ptr);
		MemMove(&waydata, output_ptr, sizeof(WaypointData));
		MemHandleUnlock(output_hand);

		if (fltdata->flightcomp == RECOCOMP) {
			StrCopy(output_char, "LRECN" );
		} else {
			// L - Comment, XXX - Unregistered program, N - ?
			StrCopy(output_char, "LXXXN" );
		}

		// Format and output waypoint index
		StrIToA(tempchar, (wayindex+1));
		StrCat(output_char, (leftpad(tempchar, '0', 5)));

		// Format Latitude
		LLToStringDM(waydata.lat, tempchar, ISLAT, false, false, 3);
		StrCat(output_char, tempchar);

		// Format Longitude
		LLToStringDM(waydata.lon, tempchar, ISLON, false, false, 3);
		StrCat(output_char, tempchar);

		// Format Elevation with single decimal place and no period
		StrCat(output_char, 
				 leftpad(print_strnoperiod(DblToStr(pround(waydata.elevation * ALTMETCONST, 1),1)), '0', 6));

		// Append Waypoint "type"
		StrCopy(tempchar, "");
		y=0;
		if (waydata.type & AIRPORT) { StrCat(tempchar, "A"); y++; }
		if (waydata.type & TURN) { StrCat(tempchar, "T"); y++; }
		if (waydata.type & LAND) { StrCat(tempchar, "L"); y++; }
		if (waydata.type & START) { StrCat(tempchar, "S"); y++; }
		if (waydata.type & FINISH) { StrCat(tempchar, "F"); y++; }
		if (waydata.type & MARK) { StrCat(tempchar, "M"); y++; }
		if (waydata.type & HOME) { StrCat(tempchar, "H"); y++; }
		
		// Pad enough spaces to fill out the area
		StrCat(output_char, (rightpad(tempchar, ' ', 8)));
			
		// Append waypoint name
		StrCat(output_char, waydata.name);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}
	return;
}

void OutputIGCFltCRecs(FlightData *fltdata)
{
	Char output_char[80];
	Char tempchar[30];
	UInt16 x, numturnpts;
//	DateTimeType IGCDate;
//	Int16 fltNum = 1;

	// First subtract the Start and Finish
	numturnpts = fltdata->flttask.numwaypts - 2;
	// Remove one if there is a takeoff point 
	if (fltdata->flttask.hastakeoff) numturnpts--;
	// Remove one if there is a landing point 
	if (fltdata->flttask.haslanding) numturnpts--;

	/* Output First C Record */
	StrCopy(output_char, "C");
	StrCat(output_char, fltdata->declaredtg);
	StrCat(output_char, fltdata->declareutc);
	StrCat(output_char, fltdata->startdtg);
	// Should really figure out how many flights on this day
	// but this will do.
//	StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &IGCDate);
//	fltNum = FindFltNumOfDay(fltdata->startdtg, IGCDate.hour, IGCDate.minute, IGCDate.second);
//	StrCat(output_char, leftpad(StrIToA(tempchar, fltNum), '0', 4));
	// This is the task number of the day.  It's actually a bit bogus.
	StrCat(output_char, "0001");
	StrCat(output_char, leftpad(StrIToA(tempchar, numturnpts), '0', 2));
	StrCat(output_char, fltdata->flttask.name);
	HandleSHA(output_char, SHAHASH);
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "%s", output_char);

	// If there isn't a defined takeoff point,
	// put in a bogus one
	if (!fltdata->flttask.hastakeoff) {
		StrCopy(output_char, "C0000000N00000000E0000TAKEOFF");
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}
		
	/* Output Remainder of the C Records */
	for (x=0; x < fltdata->flttask.numwaypts; x++) {
		StrCopy(output_char, "C");
		// MFH Items that need to be output
		// Output Lat
		LLToStringDM(fltdata->flttask.wayptlats[x], tempchar, ISLAT, false, false, 3);
		StrCat(output_char, tempchar);
		// Output Lon
		LLToStringDM(fltdata->flttask.wayptlons[x], tempchar, ISLON, false, false, 3);
		StrCat(output_char, tempchar);
		// Output the waypoint index+1 from the Waypoint database
		StrCat(output_char, (leftpad(StrIToA(tempchar, (fltdata->flttask.wayptidxs[x]+1)), '0', 5)));
		StrCat(output_char, fltdata->flttask.wayptnames[x]);
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}
	// If there isn't a defined landing point,
	// put in a bogus one
	if (!fltdata->flttask.haslanding) {
		StrCopy(output_char, "C0000000N00000000E0000LANDING");
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}
}

void OutputIGCFltBRecs(Int32 startidx, Int32 stopidx, Int32 totalrecs)
{
	Char output_char[80];
	Char tempchar[30];
	UInt16 altlen;
	MemHandle output_hand;
	MemPtr output_ptr;
	LoggerData *logdata;
	MemHandle loghand;
	Boolean firsttime = true;
	Int32 fltindex;

	loghand = MemHandleNew(sizeof(LoggerData));
	logdata = MemHandleLock(loghand);

//	HostTraceOutputTL(appErrorClass, "startidx - %ld", startidx);
//	HostTraceOutputTL(appErrorClass, "stopidx - %ld", stopidx);

	for (fltindex = startidx; fltindex <= stopidx; fltindex++) {
		if (totalrecs > 0) {
			if ((fltindex % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, totalrecs, fltindex, "Records");
		} else {
			if (((fltindex-startidx) % 10)==0) HandleWaitDialogUpdate(UPDATEDIALOG, stopidx-startidx+1, fltindex-startidx, "Records");
		}

		if (OpenDBQueryRecord(logger_db, fltindex, &output_hand, &output_ptr) == false) { 
			return;
		}

		MemMove(logdata, output_ptr, sizeof(LoggerData));
		MemHandleUnlock(output_hand);

		// Output F Record if necessary
		OutputIGCFltFRec(logdata, firsttime);
		if (firsttime) {
			firsttime = false;
		}

		// Output E Record if necessary
		if (logdata->event) {
			StrCopy(output_char, "E");
			StrCat(output_char, logdata->gpsutc);
			StrCat(output_char, "PEV");
			HandleSHA(output_char, SHAHASH);
			StrCatEOL(output_char, data.config.xfertype);
			TxData(output_char, data.config.xfertype);
		}
		
		/* Output B Record */
		StrCopy(output_char, "B");
		StrCat(output_char, logdata->gpsutc);
		LLToStringDM(DegMinStringToLatLon(logdata->gpslat, logdata->gpslatdir[0]), 
						 tempchar, ISLAT, false, false, 3);
		StrCat(output_char, tempchar);

		/* Remove period from the lng */
		StrCopy(tempchar, print_strnoperiod(logdata->gpslng));

		LLToStringDM(DegMinStringToLatLon(logdata->gpslng, logdata->gpslngdir[0]), 
						 tempchar, ISLON, false, false, 3);
		StrCat(output_char, tempchar);
		StrCat(output_char, logdata->gpsstat);

		/* Pressure Altitude */
		if (logdata->pressalt < 0) {
			StrCopy(tempchar, DblToStr((logdata->pressalt * ALTMETCONST * -1),0));
			StrCat(output_char, "-");
			altlen = 4;
		} else {
			StrCopy(tempchar, DblToStr(logdata->pressalt * ALTMETCONST,0));
			altlen = 5;
		}
		StrCat(output_char, leftpad(tempchar, '0', altlen));
		/* GPS Altitude */
		if (logdata->gpsalt < 0) {
			StrCopy(tempchar, DblToStr((logdata->gpsalt*ALTMETCONST*-1),0));
			StrCat(output_char, "-");
			altlen = 4;
		} else {
			StrCopy(tempchar, DblToStr((logdata->gpsalt*ALTMETCONST),0));
			altlen = 5;
		}
		StrCat(output_char, leftpad(tempchar, '0', altlen));
	
		// Output the 2 sigma FXA (Fix Accuracy) Information
		StrIToA(tempchar, (Int32)(pround(logdata->eph, 0)));
		StrCat(output_char, leftpad(tempchar, '0', 3));

		// Output the SIU (Satellites In Use) Information
		StrIToA(tempchar, (Int32)(logdata->siu));
		StrCat(output_char, leftpad(tempchar, '0', 2));

		// Output ENL?
		if (data.config.flightcomp == RECOCOMP || data.config.logenl) {
			// Output the ENL (Engine Noise Level) Information
			StrIToA(tempchar, (Int32)(logdata->enl));
			StrCat(output_char, leftpad(tempchar, '0', 3));
		}

		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}
	if (totalrecs > 0) {
		HandleWaitDialogUpdate(UPDATEDIALOG, totalrecs, stopidx+1, "Records");
	} else {
		HandleWaitDialogUpdate(UPDATEDIALOG, stopidx-startidx+1, stopidx-startidx+1, "Records");
	}

	MemHandleUnlock(loghand);
	MemHandleFree(loghand);

	return;
}

// Output the Required IGC F Record listing the satellites
// currently be used by the GPS
void OutputIGCFltFRec(LoggerData *logdata, Boolean firsttime)
{
	Boolean outputFRec = false;
	Boolean found = false;
	Int16 i, j;
	static Int8 prevsiu = 0;
	static Int8 prevsvns[12];
	Char output_char[80];
	Char tempchar[30];

	// Determine if I need to output the F Record
	if (firsttime) {
		outputFRec = true;
	} else if (logdata->siu != prevsiu) {
		outputFRec = true;
	} else {
		for (i=0; i<12; i++) {
			found = false;
			for (j=0; i<12; i++) {
				if ((logdata->svns[i] == -1) ||
				   (prevsvns[j] == -1) ||
				   (logdata->svns[i] == prevsvns[j])) {
					found = true;
					j = 12;
				}
			}
			if (!found) {
				i = 12;
				outputFRec = true;
			}
		}
	}

	// Output the F Record if necessary
	if (outputFRec) {
		StrCopy(output_char, "F");
		StrCat(output_char, logdata->gpsutc);
		for (i=0; i<12; i++) {
			if (logdata->svns[i] != -1) {
				StrIToA(tempchar, (Int32)(logdata->svns[i]));
				StrCat(output_char, leftpad(tempchar, '0', 2));
			}
		}
		HandleSHA(output_char, SHAHASH);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
//		HostTraceOutputTL(appErrorClass, "%s", output_char);
	}

	// Copy the current values to the previous values
	prevsiu = logdata->siu;
	for (i=0; i<12; i++) {
		prevsvns[i] = logdata->svns[i];
	}
	return;
}

void OutputIGCInvalidFltLRec()
{
	Char output_char[40];
	
	StrCopy(output_char, "L Power Disconnected During Flight");
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
	StrCopy(output_char, "L Invalid RECO Flight");
	StrCatEOL(output_char, data.config.xfertype);
	TxData(output_char, data.config.xfertype);
	return;
}

Boolean CopyTaskToFlight()
{
	UInt16 x, y=0;

	// Only declare to SP log file when not logging
	if ((!logactive) || (data.flight.flttask.numwaypts == 0)) {
		if (data.task.numwaypts > 0) {
			// task data
			data.flight.flttask.numwaypts = data.task.numwaypts - data.task.numctlpts;
			data.flight.flttask.numctlpts = 0;
			data.flight.flttask.hastakeoff = data.task.hastakeoff;
			data.flight.flttask.haslanding = data.task.haslanding;
			StrCopy(data.flight.flttask.name, data.task.name);
			StrCopy(data.flight.declareutc, data.activetask.declareutc);
			StrCopy(data.flight.declaredtg, data.activetask.declaredtg);
			data.flight.flttask.ttldist = data.task.ttldist;
			data.flight.flttask.aataimtype = data.task.aataimtype;
			// task waypoints
			for (x=0; x<data.task.numwaypts; x++) {
				// skip control points from declaration
				if ((data.task.waypttypes[x] & CONTROL) == 0) { 
					StrCopy(data.flight.flttask.wayptnames[y], data.task.wayptnames[x]);
					StrCopy(data.flight.flttask.remarks[y], data.task.remarks[x]);	
					data.flight.flttask.wayptidxs[y] = data.task.wayptidxs[x];
					data.flight.flttask.wayptlats[y] = data.task.wayptlats[x];
					data.flight.flttask.wayptlons[y] = data.task.wayptlons[x];
					data.flight.flttask.distlats[y] = data.task.distlats[x];
					data.flight.flttask.distlons[y] = data.task.distlons[x];
					data.flight.flttask.targetlats[y] = data.task.targetlats[x];
					data.flight.flttask.targetlons[y] = data.task.targetlons[x];
					data.flight.flttask.elevations[y] = data.task.elevations[x];
					data.flight.flttask.distances[y] = data.task.distances[x];
					data.flight.flttask.bearings[y] = data.task.bearings[x];
					data.flight.flttask.arearadii[y] = data.task.arearadii[x];
					data.flight.flttask.arearadii2[y] = data.task.arearadii2[x];
					data.flight.flttask.sectbear1[y] = data.task.sectbear1[x];
					data.flight.flttask.sectbear2[y] = data.task.sectbear2[x];
					data.flight.flttask.waypttypes[y] = data.task.waypttypes[x];
					data.flight.flttask.terrainidx[y] = data.task.terrainidx[y];
					y++;
				}
			}
			// task rules
			CopyTaskRulesToFlight();
			// copies IGC info at the time of declaration
			data.flight.igchinfo = data.igchinfo;
		}
		
		// ask question to declare task to supported loggers or not
		CompCmd = DECQUESTION;

		return(true);
	} else {
		return(false);
	}
}

void CopyTaskRulesToFlight()
{
	// task rules
	data.flight.flttask.rulesactive = data.task.rulesactive;
	data.flight.flttask.startwarnheight = data.task.startwarnheight;
	data.flight.flttask.maxstartheight = data.task.maxstartheight;
	data.flight.flttask.timebelowstart = data.task.timebelowstart;
	data.flight.flttask.mintasktime = data.task.mintasktime;	
	data.flight.flttask.finishheight = data.task.finishheight;
	data.flight.flttask.fgto1000m = data.task.fgto1000m;
	data.flight.flttask.startaltref = data.task.startaltref;
	data.flight.flttask.finishaltref = data.task.finishaltref;
	data.flight.flttask.startlocaltime = data.task.startlocaltime;
	data.flight.flttask.startonentry = data.task.startonentry;
	data.flight.flttask.warnbeforestart = data.task.warnbeforestart;
	data.flight.flttask.inclcyldist = data.task.inclcyldist;
	return;
}

Int16 FindFltNumOfDay(Char* fltdtg, Int16 flthour, Int16 fltminute, Int16 fltsecond) 
{
	DateTimeType fltdt;
	Int16 x = 0;
	Int16 nrecs;
	Int16 fltnum = 1;
	FlightData *fltdata;
	MemHandle flthand;
	MemHandle output_hand;
	MemPtr output_ptr;

	nrecs = OpenDBCountRecords(flight_db);
//	HostTraceOutputTL(appErrorClass, "nrecs-%hd", nrecs);

//	HostTraceOutputTL(appErrorClass, "fltdtg-%s", fltdtg);
//	HostTraceOutputTL(appErrorClass, "flthour-%hd", flthour);
//	HostTraceOutputTL(appErrorClass, "fltminute-%hd", fltminute);
//	HostTraceOutputTL(appErrorClass, "fltsecond-%hd", fltsecond);
//	HostTraceOutputTL(appErrorClass, "==============================");
	

	flthand = MemHandleNew(sizeof(FlightData));
	fltdata = MemHandleLock(flthand);
	for (x=0; x<nrecs; x++) {
		OpenDBQueryRecord(flight_db, x, &output_hand, &output_ptr);
		MemMove(fltdata, output_ptr, sizeof(FlightData));
		MemHandleUnlock(output_hand);

		StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &fltdt);
//		HostTraceOutputTL(appErrorClass, "fltdata->startdtg-%s", fltdata->startdtg);
//		HostTraceOutputTL(appErrorClass, "fltdt.hour-%hd", fltdt.hour);
//		HostTraceOutputTL(appErrorClass, "fltdt.minute-%hd", fltdt.minute);
//		HostTraceOutputTL(appErrorClass, "fltdt.minute-%hd", fltdt.second);
//		HostTraceOutputTL(appErrorClass, "----------------------------");
		if (StrCompare(fltdtg, fltdata->startdtg) == 0) {
			if (fltdt.hour < flthour) {
				fltnum++;
			} else if (fltdt.hour == flthour) {
				if (fltdt.minute < fltminute) {
					fltnum++;
				} else if (fltdt.minute == fltminute) {
					if (fltdt.second < fltsecond) {
						fltnum++;
					}
				}
			}
		}
	}
	MemHandleUnlock(flthand);
	MemHandleFree(flthand);
//	HostTraceOutputTL(appErrorClass, "fltnum-%hd", (Int16)fltnum);
//	HostTraceOutputTL(appErrorClass, "----------------------------");
	return(fltnum);
}

void StopLogging() 
{
//	HostTraceOutputTL(appErrorClass, "Stop Logging");
	initENL(true); // close ENL stream
	if (recv_data) {
		StrCopy(data.flight.stoputc, data.logger.gpsutc);
		StrCopy(data.flight.stopdtg, data.logger.gpsdtg);
		data.flight.maxalt = data.input.maxalt;
		data.flight.minalt = data.input.minalt;
		if ((data.activetask.tskstartsecs > 0) && tasknotfinished && (data.task.numwaypts > 0)) {
			StrCopy(data.flight.tskstoputc, data.logger.gpsutc);
			StrCopy(data.flight.tskstopdtg, data.logger.gpsdtg);
			data.activetask.tskstopsecs = utcsecs;
			data.flight.tskdist = CalcCompletedTskDist();
			data.flight.tskspeed = CalcCompletedTskSpeed();
			data.flight.flttask.ttldist = data.task.ttldist;
			tasknotfinished = false;
			mustActivateAsNew = true;
		}
	}
	// Calculate the Thermal vs. Cruise Percentage
	if (data.flight.stopidx > data.flight.startidx) {
		data.flight.pctthermal = (double)thermalcount / (double)(data.flight.stopidx-data.flight.startidx);
	} else {
		data.flight.pctthermal = 0.0;

	}
	// Save the current flight computer type & serial into the flight info
	data.flight.flightcomp = data.config.flightcomp;
	StrCopy(data.flight.fcserial, recodata->recoserial);
	StrCopy(data.flight.fcfirmwarever, recodata->recofirmwarever);

	/* Save Flight Data */
	OpenDBUpdateRecord(flight_db, sizeof(FlightData), &data.flight, activefltindex);

	// Output Flight To IGC File Automatically
	if ((data.config.autoIGCxfer) && (((data.config.xfertype == USEVFS) && device.CardRW) || (data.config.xfertype == USEDOC))) {
		OutputSingleFlight(activefltindex, false);
	}

	activefltindex = NO_RECORD;
	atlogspd = false;
	logactive = false;
	data.application.changed = 1;

	return;
}

double estlogtimeleft() 
{
	UInt32 totalmem, dynamicmem, freemem;
	double flightsize, timeleft;

	// estimate available log file size and time
	freemem = GetOSFreeMem(&totalmem, &dynamicmem);
//	HostTraceOutputTL(appErrorClass, "Free Memory %s K", DblToStr(freemem,0));
	if (data.config.slowlogint < 1) data.config.slowlogint = 1;
	flightsize = 84+sizeof(FlightData)+20 + 84+((sizeof(LoggerData)+20)*(30*60/data.config.slowlogint));
//	HostTraceOutputTL(appErrorClass, "Flight size %s bytes", DblToStr(flightsize,0));
	timeleft =(((double)freemem*1024)/flightsize)/2;
	if (timeleft >= 1000.0) timeleft = 999.9;

	return(timeleft);
}

void OutputSingleFlight(Int16 selectedFlt, Boolean dispAlert)
{
	MemHandle output_hand;
	MemPtr output_ptr;
	Char tempchar[30];
	Int32 nrecs;
	Boolean initstat = true;

	// Outputting A Single Flight
	OpenDBQueryRecord(flight_db, selectedFlt, &output_hand, &output_ptr);
	MemMove(fltdata,output_ptr,sizeof(FlightData));
	MemHandleUnlock(output_hand);

	if (data.config.xfertype == USEVFS || data.config.xfertype == USEDOC) {
		// Build a filename to use
		GenerateIGCName(fltdata, tempchar);
		initstat = XferInit(tempchar, IOOPENTRUNC, data.config.xfertype);
	} else {
		SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
		initstat = XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
	}
	if (initstat) {
		//HandleWaitDialog(true);
		nrecs = fltdata->stopidx - fltdata->startidx + 1;
		HandleWaitDialogUpdate(SHOWDIALOG, nrecs, 0, NULL);
		HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, 0, "Records");
		if (data.config.xfertype == USEIR) {
			Sleep(10.0);
		}
		HandleSHA(" ", SHAINIT);
		OutputIGCHeader(fltdata);
		if (data.config.output_wypts) {
			OutputIGCFltLRecs(fltdata);
		}
		if (data.config.output_tasks && (fltdata->flttask.numwaypts > 0)) {
			OutputIGCFltCRecs(fltdata);
		}

		OutputIGCFltBRecs(fltdata->startidx, fltdata->stopidx, -1);
		HandleSHA(" ", SHAEND);
		// Output an L Record saying the flight is invalid if the
		// power is disconnected from the PDA when configured for a RECO
		if (fltdata->valid == false && fltdata->flightcomp == RECOCOMP) {
			OutputIGCInvalidFltLRec();
		} else {
			HandleSHA(" ", SHAOUTPUT);
		} 
		//HandleWaitDialog(false);
		HandleWaitDialogUpdate(STOPDIALOG, nrecs, nrecs, NULL);
		if (dispAlert) {
			FrmCustomAlert(FinishedAlert, "Finished Sending Data"," "," ");
		}
		XferClose(data.config.xfertype);
	}
	return;
}
