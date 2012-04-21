#include <PalmOS.h>	// all the system toolbox headers
#include <SerialMgr.h>
#include <SerialMgrOld.h>
#include "soaring.h"
#include "soarIO.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarMem.h"
#include "soarDB.h"
#include "soarMath.h"
#include "soarFlrm.h"
#include "soarCAI.h"
#include "soarUMap.h"
#include "AddIncs/GarminIncs/GPSLib68K.h"

// used for Flarm data and traffic display
GPSSatDataType *satDataFlarm;
Boolean Flarmpresent = false;
Boolean FlarmAlarm = false;
Int16 FlarmIdx = 1;
UInt8 FlarmRX = 0;
Int16 FlarmVSep;
Char errstring[15] = "Flarm";
Int8 FLARMxfertype = USESER;

double Flarm_HW_Version = 0.0; // to hold the flarm hardware version
double Flarm_SW_Version = 0.0; // to hold the flarm software version

// Bug in version 2.15 - 4.0 and onwards that counts the NMEA checksum in the configuration string
#define IGC_checksum_error 999.9	// This will be set to the version that corrects this bug

// external variables
extern Int8 		xfrdialog;
extern CAILogData 	*cailogdata;
extern Int16 		cainumLogs;
extern Int16 		selectedCAIFltIndex;
extern UInt32		origform;
extern Boolean	menuopen;

// internal variabels
UInt16 SeqNo = 0;
FlarmConfigData FlarmData;

Boolean DeclareFlarmTask(Boolean declaretoSD)
{
	Boolean	endResult = true;
	Char output_char[50];
	Char tempchar[25];
	Int16 i = 0;
	UInt8 flarmspeed = 255;
	
	xfrdialog = XFRXFR;
	HandleWaitDialog(true);
	StrCopy(errstring, "Declare Task");

	if (declaretoSD) {
		if (!device.CardRW) {
			HandleWaitDialog(false);
			return(false);
		}
		// declaring to SD card
		FLARMxfertype = USEVFS;
		XferInit("flarmcfg.txt", IOOPENTRUNC, FLARMxfertype);
		// header
		StrCopy(output_char, "// SOARINGPILOT Version ");
		StrCat(output_char, device.appVersion);
		StrCat(output_char, "\r\n");
		TxData(output_char, FLARMxfertype);
		StrCopy(output_char, "// Flarm Task Declaration ");
		StrCat(output_char, "\r\n");
		TxData(output_char, FLARMxfertype);
		StrCopy(output_char, "// Date : ");
		StrCat(output_char, data.flight.declaredtg);
		StrCat(output_char, "\r\n");
		TxData(output_char, FLARMxfertype);
		StrCopy(output_char, "// Time : ");
		StrCat(output_char, data.flight.declareutc);
		StrCat(output_char, "\r\n");
		TxData(output_char, FLARMxfertype);
	}

	// IGC Info
	if (data.config.FlrmCopyIGC) if (!FlarmSendIGCInfo(declaretoSD)) return(FlarmDataErr(declaretoSD));

	// task name
	StrCopy(errstring, "Task Name");
	StrCopy(output_char, "$PFLAC,S,NEWTASK,");
	StrCat(output_char, data.task.name);
	if (!declaretoSD) {
		StrCat(output_char, "*");
		StrCat(output_char, CalcChkSum(output_char) );
	}
	StrCat(output_char, "\r\n");
	TxData(output_char, FLARMxfertype);
	if (!declaretoSD) {
		output_char[7] = 'A';
//		output_char[9] = 0;
		output_char[StrLen(output_char)-5] = 0;
		if (!WaitFor(output_char, FLARMxfertype)) return(FlarmDataErr(declaretoSD));
	}
 
	if (!data.task.hastakeoff) {
		// blank take off waypoint
		StrCopy(errstring, "Takeoff");
		StrCopy(output_char, "$PFLAC,S,ADDWP,");
		StrCat(output_char, "0000000N,00000000E,TAKEOFF");
		if (!declaretoSD) {
			StrCat(output_char, "*");
			StrCat(output_char, CalcChkSum(output_char) );
		}
		StrCat(output_char, "\r\n");
		TxData(output_char, FLARMxfertype);
		if (!declaretoSD) {
			output_char[7] = 'A';
//			output_char[9] = 0;
			output_char[StrLen(output_char)-5] = 0;
			if (!WaitFor(output_char, FLARMxfertype)) return(FlarmDataErr(declaretoSD));
		}
	}

	// output each waypoint
	for (i = 0; i < data.task.numwaypts; i++) {
		StrCopy(errstring, "WP ");
		StrCat(errstring, DblToStr(i,0));
		StrCopy(output_char, "$PFLAC,S,ADDWP,");
		LLToStringDM(data.task.wayptlats[i], tempchar, ISLAT, false, false, 3);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
		LLToStringDM(data.task.wayptlons[i], tempchar, ISLON, false, false, 3);
		StrCat(output_char, tempchar);
		StrCat(output_char, ",");
		StrCat(output_char, data.task.wayptnames[i]);
		if (!declaretoSD) {
			StrCat(output_char, "*");
			StrCat(output_char, CalcChkSum(output_char) );
		}
		StrCat(output_char, "\r\n");
		TxData(output_char, FLARMxfertype);
		if (!declaretoSD) {
			output_char[7] = 'A';
//			output_char[9] = 0;
			output_char[StrLen(output_char)-5] = 0;
			if (!WaitFor(output_char, FLARMxfertype)) return(FlarmDataErr(declaretoSD));
		}
	}

	if (!data.task.haslanding) {
		// blank landing waypoint
		StrCopy(errstring, "Landing");
		StrCopy(output_char, "$PFLAC,S,ADDWP,");
		StrCat(output_char, "0000000N,00000000E,LANDING");
		if (!declaretoSD) {
			StrCat(output_char, "*");
			StrCat(output_char, CalcChkSum(output_char) );
		}
		StrCat(output_char, "\r\n");
		TxData(output_char, FLARMxfertype);
		if (!declaretoSD) {
			output_char[7] = 'A';
//			output_char[9] = 0;
			output_char[StrLen(output_char)-5] = 0;
			if (!WaitFor(output_char, FLARMxfertype)) return(FlarmDataErr(declaretoSD));
		}
	}

	// logging interval
	if (declaretoSD) {
		StrCopy(errstring, "Log Interval");
		StrCopy(output_char, "$PFLAC,S,LOGINT,");
		StrCat(output_char, DblToStr(data.config.slowlogint,0));
		if (!declaretoSD) {
			StrCat(output_char, "*");
			StrCat(output_char, CalcChkSum(output_char) );
		}
		StrCat(output_char, "\r\n");
		TxData(output_char, FLARMxfertype);
	}

	// baud rate
	switch (data.config.nmeaspeed) {
//		case 0: // 1200
//		case 1: // 2400
		case 2: // 4800
			flarmspeed = 0;
			break;
		case 3: // 9600
//		case 4: // 14400
			flarmspeed = 1;
			break;
		case 5: // 19200
			flarmspeed = 2;
			break;
//		case 6: // 28880
			flarmspeed = 3;
			break;
		case 7: // 38400
			flarmspeed = 4;
			break;
		case 8: // 57600
//		case 9: // 115200
			flarmspeed = 5;
			break;
	}
	if (flarmspeed == 255) return(FlarmDataErr(declaretoSD));
	if (declaretoSD) {
		StrCopy(errstring, "Baud Rate");
		StrCopy(output_char, "$PFLAC,S,BAUD,");
		StrCat(output_char, DblToStr(flarmspeed,0));
		if (!declaretoSD) {
			StrCat(output_char, "*");
			StrCat(output_char, CalcChkSum(output_char) );
		}
		StrCat(output_char, "\r\n");
		TxData(output_char, FLARMxfertype);
	}

	if (declaretoSD) {
		// close SD file
		XferClose(FLARMxfertype);
		if (!VFSFileCopy("/PALM/Programs/SoarPilot/flarmcfg.txt", "/flarmcfg.txt")) {
			HandleWaitDialog(false);
			return(false);
		}
		FLARMxfertype = USESER;
	}

	HandleWaitDialog(false);
	return(endResult);
}

Boolean ClearFlarmDeclaration(Boolean declaretoSD)
{
	Boolean	endResult = true;
	Char output_char[50];
	
	xfrdialog = XFRXFR;
	HandleWaitDialog(true);
	StrCopy(errstring, "Clear Task");

	MemSet(&FlarmData, sizeof(FlarmConfigData), 0);

	// New Task
	StrCopy(output_char, "$PFLAC,S,NEWTASK,");
	StrCat(output_char, " ");
	StrCat(output_char, "*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	output_char[7] = 'A';
	output_char[StrLen(output_char)-5] = 0;
	if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));

	// set zero waypoints to clear task declaration
	// take off
	StrCopy(output_char, "$PFLAC,S,ADDWP,");
	StrCat(output_char, "0000000N,00000000E,TAKEOFF");
	StrCat(output_char, "*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	output_char[7] = 'A';
	output_char[StrLen(output_char)-5] = 0;
	if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));

	// landing
	StrCopy(output_char, "$PFLAC,S,ADDWP,");
	StrCat(output_char, "0000000N,00000000E,LANDING");
	StrCat(output_char, "*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	output_char[7] = 'A';
	output_char[StrLen(output_char)-5] = 0;
	if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));

	HandleWaitDialog(false);
	return(endResult);
}

Boolean FlarmDownloadFlight()
{
	Boolean	endResult;
	UInt16 	frame_length = 0;
	UInt8	*frame_data = NULL;
	UInt8 	cmdarray[10];
	UInt16  crc = 0;
	Boolean	cmdOK = false;
	Boolean	seqOK = false;
	UInt8	lastchar = 0;
	Boolean	initstat;
	UInt32	numbytes;

	if (Flarm_ping()) {
		HandleWaitDialogUpdate(SHOWDIALOG, 0, -1, NULL);
		HandleWaitDialogUpdate(UPDATEDIALOG, 0, -1, "%");

		// select selected flight record
//		HostTraceOutputTL(appErrorClass, "Flight %hu", (UInt16)selectedCAIFltIndex);

		// build command
		SeqNo++; 				// increment sequence no.
		cmdarray[0] = 0x73;			// start frame
		cmdarray[1] = 0x09;			// frame length
		cmdarray[2] = 0x00;			//
		cmdarray[3] = 0x01;			// version
		cmdarray[4] = SeqNo%256; 		// sequence no.
		cmdarray[5] = SeqNo/256;		//
		cmdarray[6] = 0x20;			// select log record
		cmdarray[7] = cailogdata[selectedCAIFltIndex].StartTape;	// record to select (only here for crc check)
		crc = Calc_crc(&cmdarray[1], 7);	// calculate crc
		cmdarray[7] = crc%256; 			// crc
		cmdarray[8] = crc/256;			//
		cmdarray[9] = cailogdata[selectedCAIFltIndex].StartTape;	// record to select
		Send_command(&cmdarray[0], 10);

		// read response
		frame_length = Wait_Frame_Start();
//		HostTraceOutputTL(appErrorClass, "frame length1 %hu", frame_length);
		AllocMem((void *)&frame_data, frame_length+1);
		if (Read_Frame(frame_data, frame_length)) {
//			HostTraceOutputTL(appErrorClass, "Msg1 %hx", (UInt16)frame_data[5]);
			cmdOK = (frame_data[5] == 0xA0);
			seqOK = ((frame_data[8]+frame_data[9]*256) == SeqNo);
			FreeMem((void *)&frame_data);
			if (cmdOK && seqOK) {

				// open IGC output file
				initstat = XferInit(cailogdata[selectedCAIFltIndex].IGCname, IOOPENTRUNC, data.config.xfertype);

				// request flight log download
				do {
					cmdOK = false;
					seqOK = false;

					do {
						// build command
						SeqNo++; 			// increment sequence no.
						cmdarray[0] = 0x73;		// start frame
						cmdarray[1] = 0x08;		// frame length
						cmdarray[2] = 0x00;		//
						cmdarray[3] = 0x01;		// version
						cmdarray[4] = SeqNo%256; 	// sequence no.
						cmdarray[5] = SeqNo/256;	//
						cmdarray[6] = 0x22;		// get flight log
						crc = Calc_crc(&cmdarray[1], 6);// calculate crc
						cmdarray[7] = crc%256; 		// crc
						cmdarray[8] = crc/256;		//
						Send_command(&cmdarray[0], 9);

						// read response
						frame_length = Wait_Frame_Start();
//						HostTraceOutputTL(appErrorClass, "frame length2 %hu", frame_length);
					} while (frame_length == 0);

					AllocMem((void *)&frame_data, frame_length+1);
					if (Read_Frame(frame_data, frame_length)) {
//						HostTraceOutputTL(appErrorClass, "Msg2 %hx", (UInt16)frame_data[5]);
						cmdOK = (frame_data[5] == 0xA0);
						seqOK = ((frame_data[8]+frame_data[9]*256) == SeqNo);
						lastchar = frame_data[frame_length-1]; // check for EOF marker

						if (cmdOK && seqOK) {
							// update progress
//							HostTraceOutputTL(appErrorClass, "Done %hu", (UInt16)frame_data[10]);
							HandleWaitDialogUpdate(UPDATEDIALOG, frame_data[10], -1, "%");

							// save IGC data
							numbytes = frame_length-((lastchar==0x1A)?12:11);
							if (data.config.xfertype == USEVFS) 
								HandleVFSData(IOWRITE, &frame_data[11], &numbytes);
							else if (data.config.xfertype == USEDOC) 
								HandleDOCData(IOWRITE, &frame_data[11], &numbytes);
						}
						FreeMem((void *)&frame_data);
					}			
				} while (cmdOK && seqOK && (lastchar != 0x1A));
			}
		}
		if (frame_data) FreeMem((void *)&frame_data);

		// close IGC output file
		XferClose(data.config.xfertype);

		HandleWaitDialogUpdate(STOPDIALOG, 100, -1, NULL);
		endResult = (lastchar == 0x1A);
		if (endResult) FrmCustomAlert(FinishedAlert, "Flight Log Download Succeeded!"," "," ");
	} else {
		// connection in binary mode failed
		endResult = false;	
	}
	
	return(endResult);
}

Boolean FlarmGetFlightList(Int16 cmd)
{
	UInt16		numberOfFlights = 0;
	UInt16		OKFlights = 0;
	UInt16 		frame_length = 0;
	UInt8		*frame_data = NULL;
	UInt8 		cmdarray[10];
	UInt16		crc = 0;
	CAILogData	*caitemp = NULL;
	Boolean		cmdOK = false;
	Boolean		seqOK = false;
	Char 		tempchar[82];
	
	if (cmd == CAIFIFREE) {
		if (cailogdata)	{	// free the previously allocated memory
			MemPtrFree(cailogdata);
			cailogdata = NULL;
		}
		cainumLogs = 0;
		Flarm_reset();		// reset Flarm to put into NMEA mode.
		return(true);
	}
	
	if (cmd == CAIFISTART)	{	// download the list of flights stored in the logger
		HandleWaitDialogUpdate(SHOWDIALOG, 0, -1, NULL);
		if (Flarm_binary_connect()) { // connection test
//			HostTraceOutputTL(appErrorClass, "Flarm Connected");
			HandleWaitDialogUpdate(UPDATEDIALOG, 0, -1, "Flights");

			// loop to read flight data
			do {
		
				cmdOK = false;
				seqOK = false;

				// build command
				SeqNo++; 				// increment sequence no.
				cmdarray[0] = 0x73;			// start frame
				cmdarray[1] = 0x09;			// frame length
				cmdarray[2] = 0x00;			//
				cmdarray[3] = 0x01;			// version
				cmdarray[4] = SeqNo%256; 		// sequence no.
				cmdarray[5] = SeqNo/256;		//
				cmdarray[6] = 0x20;			// select log record
				cmdarray[7] = numberOfFlights%256;	// record to select (only here for crc check)
				crc = Calc_crc(&cmdarray[1], 7);	// calculate crc
				cmdarray[7] = crc%256; 			// crc
				cmdarray[8] = crc/256;			//
				cmdarray[9] = numberOfFlights%256;	// record to select
				Send_command(&cmdarray[0], 10);

				// read response
				frame_length = Wait_Frame_Start();
//				HostTraceOutputTL(appErrorClass, "frame length1 %hu", frame_length);
				AllocMem((void *)&frame_data, frame_length+1);
				if (Read_Frame(frame_data, frame_length)) {
//					HostTraceOutputTL(appErrorClass, "Msg1 %hx", (UInt16)frame_data[5]);
					cmdOK = (frame_data[5] == 0xA0);
					seqOK = ((frame_data[8]+frame_data[9]*256) == SeqNo);
					FreeMem((void *)&frame_data);
					if (cmdOK && seqOK) {
						// move already read flights to temp storage
						if (numberOfFlights > 0) {
							AllocMem((void *)&caitemp, sizeof(CAILogData)*numberOfFlights);
							MemMove(caitemp, cailogdata, sizeof(CAILogData)*numberOfFlights);
							FreeMem((void *)&cailogdata);
					}
						// resize cailogdata
						numberOfFlights++;
						AllocMem((void *)&cailogdata, sizeof(CAILogData)*numberOfFlights);
						// copy temp data back to cailogdata
						if (caitemp) {
							MemMove(cailogdata, caitemp, sizeof(CAILogData)*(numberOfFlights-1));
							FreeMem((void *)&caitemp);
						}
						MemSet(&cailogdata[numberOfFlights-1], sizeof(CAILogData), 0);

						// build command
						SeqNo++; 			// increment sequence no.						cmdarray[0] = 0x73;		// start frame
  						cmdarray[0] = 0x73;		// start frame
						cmdarray[1] = 0x08;		// frame length
						cmdarray[2] = 0x00;		//
						cmdarray[3] = 0x01;		// version
						cmdarray[4] = SeqNo%256; 	// sequence no.
						cmdarray[5] = SeqNo/256;	//
						cmdarray[6] = 0x21;		// get record info
						crc = Calc_crc(&cmdarray[1], 6);// calculate crc
						cmdarray[7] = crc%256; 		// crc
						cmdarray[8] = crc/256;		//
						Send_command(&cmdarray[0], 9);
	
						// read response
						frame_length = Wait_Frame_Start();
//						HostTraceOutputTL(appErrorClass, "frame length2 %hu", frame_length);
						AllocMem((void *)&frame_data, frame_length+1);
						if (Read_Frame(frame_data, frame_length)) {

//							HostTraceOutputTL(appErrorClass, "Msg2 %hx", (UInt16)frame_data[5]);
							cmdOK = (frame_data[5] == 0xA0);
							seqOK = ((frame_data[8]+frame_data[9]*256) == SeqNo);
							frame_data[frame_length] = 0; // terminate data string
	
							if (cmdOK && seqOK) {
								// fill log data structure
//								HostTraceOutputTL(appErrorClass, "Flight Log %hu OK", numberOfFlights%256);
//								HostTraceOutputTL(appErrorClass, "Frame Data %s", &frame_data[10]);
	
								// IGC filename
								GetFieldDelim(&frame_data[10], 0, '|', '|', tempchar);
//								HostTraceOutputTL(appErrorClass, "IGC %s",tempchar);
								StrNCopy(cailogdata[OKFlights].IGCname, tempchar, 15);

								// Pilot
								GetFieldDelim(&frame_data[10], 4, '|', '|', tempchar);
//								HostTraceOutputTL(appErrorClass, "Pilot %s",tempchar);
								StrNCopy(cailogdata[OKFlights].pilotName, tempchar, GL_CAI_NAME_SIZE);

								// Start Date
								GetFieldDelim(&frame_data[10], 1, '|', '|', tempchar);
//								HostTraceOutputTL(appErrorClass, "Date %s", tempchar);
								cailogdata[OKFlights].startDate.year =  (Int16)StrAToI(Mid(tempchar,2,3));
								cailogdata[OKFlights].startDate.month = (Int16)StrAToI(Mid(tempchar,2,6));
								cailogdata[OKFlights].startDate.day =   (Int16)StrAToI(Right(tempchar,2));

								// Start Time
								GetFieldDelim(&frame_data[10], 2, '|', '|', tempchar);
//								HostTraceOutputTL(appErrorClass, "Time %s",tempchar);
								cailogdata[OKFlights].startTime.hour =   (Int16)StrAToI(Left(tempchar,2));
								cailogdata[OKFlights].startTime.min = (Int16)StrAToI(Mid(tempchar,2,4));
								cailogdata[OKFlights].startTime.sec = (Int16)StrAToI(Right(tempchar,2));
	
								// Flight Log No.
								cailogdata[OKFlights].StartTape = numberOfFlights%256-1;
	
								OKFlights++;
								HandleWaitDialogUpdate(UPDATEDIALOG, OKFlights, -1, "Flights");
							} else {
//								HostTraceOutputTL(appErrorClass, "Flight Log %hu NG", numberOfFlights%256);
							}
						} else {
//							HostTraceOutputTL(appErrorClass, "Flight Log %hu NG", numberOfFlights%256);
						}
					}			
				}
				if (frame_data) FreeMem((void *)&frame_data);

			} while (cmdOK && seqOK);

//			HostTraceOutputTL(appErrorClass, "%hu Valid Flights", OKFlights);
			cainumLogs = OKFlights;
			HandleWaitDialogUpdate(STOPDIALOG, OKFlights, -1, NULL);
			return(true);
		} else {
			// connection in binary mode failed
			cainumLogs = 0;
			cailogdata = NULL;
			HandleWaitDialogUpdate(STOPDIALOG, 0, -1, NULL);
			return(false);
		}
	}
	return(false);
}

Boolean FlarmSendIGCInfo(Boolean declaretoSD)
{
	Char output_char[50];
	StrCopy(errstring, "IGC Info");

	// Pilot
	StrCopy(output_char, "$PFLAC,S,PILOT,");
	StrCat(output_char, data.igchinfo.name);
	if (!declaretoSD && (Flarm_SW_Version >= IGC_checksum_error)) {
		StrCat(output_char, "*");
		StrCat(output_char, CalcChkSum(output_char) );
	}
	StrCat(output_char, "\r\n");
	TxData(output_char, FLARMxfertype);
	if (!declaretoSD) {
		output_char[7] = 'A';
		output_char[StrLen(output_char)-5] = 0;
		if (!WaitFor(output_char, USESER)) return(false);
	}

	// Co-Pilot
	StrCopy(output_char, "$PFLAC,S,COPIL,None");
	if (!declaretoSD && (Flarm_SW_Version >= IGC_checksum_error)) {
		StrCat(output_char, "*");
		StrCat(output_char, CalcChkSum(output_char) );
	}
	StrCat(output_char, "\r\n");
	TxData(output_char, FLARMxfertype);
	if (!declaretoSD) {
		output_char[7] = 'A';
		output_char[StrLen(output_char)-5] = 0;
		if (!WaitFor(output_char, USESER)) return(false);
	}

	// Glider Type
	StrCopy(output_char, "$PFLAC,S,GLIDERTYPE,");
	StrCat(output_char, data.igchinfo.type);
	if (!declaretoSD && (Flarm_SW_Version >= IGC_checksum_error)) {
		StrCat(output_char, "*");
		StrCat(output_char, CalcChkSum(output_char) );
	}
	StrCat(output_char, "\r\n");
	TxData(output_char, FLARMxfertype);
	if (!declaretoSD) {
		output_char[7] = 'A';
		output_char[StrLen(output_char)-5] = 0;
		if (!WaitFor(output_char, USESER)) return(false);
	}
	
	// Glider ID
	StrCopy(output_char, "$PFLAC,S,GLIDERID,");
	StrCat(output_char, data.igchinfo.gid);
	if (!declaretoSD && (Flarm_SW_Version >= IGC_checksum_error)) {
		StrCat(output_char, "*");
		StrCat(output_char, CalcChkSum(output_char) );
	}
	StrCat(output_char, "\r\n");
	TxData(output_char, FLARMxfertype);
	if (!declaretoSD) {
		output_char[7] = 'A';
		output_char[StrLen(output_char)-5] = 0;
		if (!WaitFor(output_char, USESER)) return(false);
	}

	// Comp ID
	StrCopy(output_char, "$PFLAC,S,COMPID,");
	StrCat(output_char, data.igchinfo.cid);
	if (!declaretoSD && (Flarm_SW_Version >= IGC_checksum_error)) {
		StrCat(output_char, "*");
		StrCat(output_char, CalcChkSum(output_char) );
	}
	StrCat(output_char, "\r\n");
	TxData(output_char, FLARMxfertype);
	if (!declaretoSD) {
		output_char[7] = 'A';
		output_char[StrLen(output_char)-5] = 0;
		if (!WaitFor(output_char, USESER)) return(false);
	}

	// Comp Class
	StrCopy(output_char, "$PFLAC,S,COMPCLASS,");
	StrCat(output_char, data.igchinfo.cls);
	if (!declaretoSD && (Flarm_SW_Version >= IGC_checksum_error)) {
		StrCat(output_char, "*");
		StrCat(output_char, CalcChkSum(output_char) );
	}
	StrCat(output_char, "\r\n");
	TxData(output_char, FLARMxfertype);
	if (!declaretoSD) {
		output_char[7] = 'A';
		output_char[StrLen(output_char)-5] = 0;
		if (!WaitFor(output_char, USESER)) return(false);
	}

	return(true);
}

Boolean FlarmSendConfig()
{
	Char output_char[50];
	Boolean retval = true;

	xfrdialog = XFRXFR;
	HandleWaitDialog(true);
	StrCopy(errstring, "Send Config");
	
	// Device ID
	StrCopy(output_char, "$PFLAC,S,ID,");
	StrCat(output_char, FlarmData.DeviceID);
	StrCat(output_char, "*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	output_char[7] = 'A';
	output_char[StrLen(output_char)-5] = 0;
	if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));

	// RF Freq
	StrCopy(output_char, "$PFLAC,S,FREQ,");
	StrCat(output_char, DblToStr(FlarmData.RFFreq,0));
	StrCat(output_char, "*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	output_char[7] = 'A';
	output_char[StrLen(output_char)-5] = 0;
	if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));

	if (Flarm_SW_Version < 4.0) {
		// Ground RF
		StrCopy(output_char, "$PFLAC,S,RFTX,");
		StrCat(output_char, DblToStr(FlarmData.RFGndTX,0));
		StrCat(output_char, "*");
		StrCat(output_char, CalcChkSum(output_char) );
		StrCat(output_char, "\r\n");
		TxData(output_char, USESER);
		output_char[7] = 'A';
		output_char[StrLen(output_char)-5] = 0;
		if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));
	}
	
	// Private
	StrCopy(output_char, "$PFLAC,S,PRIV,");
	StrCat(output_char, DblToStr(FlarmData.Private,0));
	StrCat(output_char, "*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	output_char[7] = 'A';
	output_char[StrLen(output_char)-5] = 0;
	if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));

	// Aircraft Type
	StrCopy(output_char, "$PFLAC,S,ACFT,");
	StrCat(output_char, DblToStr(FlarmData.Type,0));
	StrCat(output_char, "*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	output_char[7] = 'A';
	output_char[StrLen(output_char)-5] = 0;
	if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));

	// User Interface
	StrCopy(output_char, "$PFLAC,S,UI,");
	StrCat(output_char, DblToStr(FlarmData.UserIF,0));
	StrCat(output_char, "*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	output_char[7] = 'A';
	output_char[StrLen(output_char)-5] = 0;
	if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));

	if (Flarm_SW_Version < 4.0) {
		// Logging
		StrCopy(output_char, "$PFLAC,S,LOGGING,");
		StrCat(output_char, DblToStr(FlarmData.Logging,0));
		StrCat(output_char, "*");
		StrCat(output_char, CalcChkSum(output_char) );
		StrCat(output_char, "\r\n");
		TxData(output_char, USESER);
		output_char[7] = 'A';
		output_char[StrLen(output_char)-5] = 0;
		if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));
	}
	
	// Logging Interval
	StrCopy(output_char, "$PFLAC,S,LOGINT,");
	StrCat(output_char, DblToStr(FlarmData.Logint,0));
	StrCat(output_char, "*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	output_char[7] = 'A';
	output_char[StrLen(output_char)-5] = 0;
	if (!WaitFor(output_char, USESER)) return(FlarmDataErr(false));

	// IGC Info
	if (data.config.FlrmCopyIGC) if (!FlarmSendIGCInfo(false))return(FlarmDataErr(false));

	HandleWaitDialog(false);
	return(retval);
}

Boolean FlarmGetConfig()
{
	Char output_char[50];
	Char input_char[50];
	Boolean retval = true;
	StrCopy(errstring, "Get Config");

	if (!Flarm_Version()) return(false);
	xfrdialog = XFRXFR;
	MemSet(&FlarmData, sizeof(FlarmConfigData), 0);
	HandleWaitDialog(true);

	// Device ID
	MemSet(input_char, sizeof(input_char), 0);
	StrCopy(output_char, "$PFLAC,R,ID,0*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	if (WaitFor("$PFLAC,A,ID,", USESER)) {
		GetData(input_char, 6, USESER);
		StrNCopy(FlarmData.DeviceID, input_char, 6);
	} else {
		return(FlarmDataErr(false));
	}
	
	// RF Freq
	MemSet(input_char, sizeof(input_char), 0);
	StrCopy(output_char, "$PFLAC,R,FREQ,0*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	if (WaitFor("$PFLAC,A,FREQ,", USESER)) {
		GetData(input_char, 1, USESER);
		FlarmData.RFFreq = StrAToI(input_char);
	} else {
		return(FlarmDataErr(false));
	}
		
	if (Flarm_SW_Version < 4.0) {
		// Ground RF
		MemSet(input_char, sizeof(input_char), 0);
		StrCopy(output_char, "$PFLAC,R,RFTX,0*");
		StrCat(output_char, CalcChkSum(output_char) );
		StrCat(output_char, "\r\n");
		TxData(output_char, USESER);
		if (WaitFor("$PFLAC,A,RFTX,", USESER)) {
			GetData(input_char, 1, USESER);
			FlarmData.RFGndTX = StrAToI(input_char);
		} else {
			return(FlarmDataErr(false));
		}
	}
	
	// Private
	MemSet(input_char, sizeof(input_char), 0);
	StrCopy(output_char, "$PFLAC,R,PRIV,0*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	if (WaitFor("$PFLAC,A,PRIV,", USESER)) {
		GetData(input_char, 1, USESER);
		FlarmData.Private = StrAToI(input_char);
	} else {
		return(FlarmDataErr(false));
	}

	// Aircraft Type
	MemSet(input_char, sizeof(input_char), 0);
	StrCopy(output_char, "$PFLAC,R,ACFT,0*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	if (WaitFor("$PFLAC,A,ACFT,", USESER)) {
		GetData(input_char, 2, USESER);
		StrCopy(input_char, trim(input_char, '*', true));
		FlarmData.Type = StrAToI(input_char);
	} else {
		return(FlarmDataErr(false));
	}

	// User Interface
	MemSet(input_char, sizeof(input_char), 0);
	StrCopy(output_char, "$PFLAC,R,UI,0*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	if (WaitFor("$PFLAC,A,UI,", USESER)) {
		GetData(input_char, 1, USESER);
		FlarmData.UserIF = StrAToI(input_char);
	} else {
		return(FlarmDataErr(false));
	}

	if (Flarm_SW_Version < 4.0) {
		// Logging
		MemSet(input_char, sizeof(input_char), 0);
		StrCopy(output_char, "$PFLAC,R,LOGGING,0*");
		StrCat(output_char, CalcChkSum(output_char) );
		StrCat(output_char, "\r\n");
		TxData(output_char, USESER);
		if (WaitFor("$PFLAC,A,LOGGING,", USESER)) {
			GetData(input_char, 1, USESER);
			FlarmData.Logging = StrAToI(input_char);
		} else {
			return(FlarmDataErr(false));
		}
	} else {
                FlarmData.Logging = 1;
	}

	// Log Interval
	MemSet(input_char, sizeof(input_char), 0);
	StrCopy(output_char, "$PFLAC,R,LOGINT,0*");
	StrCat(output_char, CalcChkSum(output_char) );
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	if (WaitFor("$PFLAC,A,LOGINT,", USESER)) {
		GetData(input_char, 5, USESER);
		FlarmData.Logint = StrAToI(input_char);
	} else {
		return(FlarmDataErr(false));
	}

	HandleWaitDialog(false);
	return(retval);
}

Boolean form_config_flarminst_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormPtr frm;
	ControlPtr ctl;
	ListPtr lst;
	Char tempchar[20];
	Int8 i;
	Boolean hexOK;

	frm = FrmGetActiveForm();
	switch (event->eType) {
		case frmOpenEvent:
			FlarmGetConfig();
		case frmUpdateEvent:
			FrmDrawForm(FrmGetActiveForm());

			field_set_value(form_config_flarminst_id, FlarmData.DeviceID);
			switch (FlarmData.RFFreq) {
				case 0:
					ctl_set_value(form_config_flarminst_freq_world, true);
					ctl_set_value(form_config_flarminst_freq_usa, false);
					ctl_set_value(form_config_flarminst_freq_nz, false);
					break;
				case 2:
					ctl_set_value(form_config_flarminst_freq_world, false);
					ctl_set_value(form_config_flarminst_freq_usa, true);
					ctl_set_value(form_config_flarminst_freq_nz, false);
					break;
				case 3:
					ctl_set_value(form_config_flarminst_freq_world, false);
					ctl_set_value(form_config_flarminst_freq_usa, false);
					ctl_set_value(form_config_flarminst_freq_nz, true);
					break;
				default:
					ctl_set_value(form_config_flarminst_freq_world, false);
					ctl_set_value(form_config_flarminst_freq_usa, false);
					ctl_set_value(form_config_flarminst_freq_nz, false);
					break;
			}
			ctl_set_value(form_config_flarminst_gndrf, (FlarmData.RFGndTX == 1));
			ctl_set_value(form_config_flarminst_priv, (FlarmData.Private == 1));

			ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, form_config_flarminst_pop1));
			lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, form_config_flarminst_pop2));
			LstSetSelection (lst, FlarmData.Type);
			LstSetTopItem (lst, FlarmData.Type);
			SysStringByIndex(form_config_flarminst_acrft_table, FlarmData.Type, tempchar, 19);
			CtlSetLabel (ctl, tempchar);

			ctl_set_value(form_config_flarminst_userif_led,  ((FlarmData.UserIF == 0) || (FlarmData.UserIF == 3)));
			ctl_set_value(form_config_flarminst_userif_buzz, ((FlarmData.UserIF == 0) || (FlarmData.UserIF == 2)));
			ctl_set_value(form_config_flarminst_logging, (FlarmData.Logging == 1));
			field_set_value(form_config_flarminst_logint, DblToStr(FlarmData.Logint,0));

			ctl_set_value(form_config_flarminst_copyigchinfo, data.config.FlrmCopyIGC);

			WinDrawLine(0,92,160,92);
			WinDrawChars("(hex)", 5, 130, 15);
			WinDrawChars("secs", 4, 130, 112);

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
				case form_config_flarminst_savebtn:
					data.config.FlrmCopyIGC = ctl_get_value(form_config_flarminst_copyigchinfo);
					StrNCopy(FlarmData.DeviceID,field_get_str(form_config_flarminst_id),6);
					ConvertToUpper(FlarmData.DeviceID);
					field_set_value(form_config_flarminst_id, FlarmData.DeviceID);
					hexOK = true;
					tempchar[1] = 0;
					for (i=0; i<6; i++) {
						tempchar[0] = FlarmData.DeviceID[i];
						if (StrStr("0123456789ABCDEF", tempchar) == NULL) {
							hexOK = false;
							FrmCustomAlert(WarningAlert, "Device ID Invalid!\n\n","Must be 6 digit hex"," ");
							break;
						}
					}
					if (hexOK) {
						if (ctl_get_value(form_config_flarminst_freq_world)) {
							FlarmData.RFFreq = 0;
						} else if (ctl_get_value(form_config_flarminst_freq_usa)) {
							FlarmData.RFFreq = 2;
						} else if (ctl_get_value(form_config_flarminst_freq_nz)) {
							FlarmData.RFFreq = 3;
						} 
						FlarmData.RFGndTX = (ctl_get_value(form_config_flarminst_gndrf)?1:0);
						FlarmData.Private = (ctl_get_value(form_config_flarminst_priv)?1:0);
						if (ctl_get_value(form_config_flarminst_userif_led)) {
							if (ctl_get_value(form_config_flarminst_userif_buzz))
								FlarmData.UserIF = 0;
							else 
								FlarmData.UserIF = 3;
						} else {
							if (ctl_get_value(form_config_flarminst_userif_buzz))
								FlarmData.UserIF = 2;
							else 
								FlarmData.UserIF = 1;
						}
						FlarmData.Logging = (ctl_get_value(form_config_flarminst_logging)?1:0);
						FlarmData.Logint = (Int16)Fabs(field_get_double(form_config_flarminst_logint));
						
						FlarmSendConfig();
						FrmGotoForm(form_set_logger);
					}
					handled = true;
					break;
				case form_config_flarminst_cancelbtn:
					FrmGotoForm(form_set_logger);
					handled = true;
					break;
				default:
					break;
			}
			break;
		case popSelectEvent:  // A popup button was pressed and released.
			PlayKeySound();
//			HostTraceOutputTL(appErrorClass, "Popup pressed");
			switch (event->data.popSelect.controlID) {
				case form_config_flarminst_pop1:
					FlarmData.Type = event->data.popSelect.selection;
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

UInt16 crc_update(UInt16 crc, UInt8 datain)
{
	UInt8 i;
	UInt16 data = datain;

	crc = crc ^ (data << 8);
	for (i=0; i<8; i++) {
		if (crc & 0x8000) {
			crc = (crc << 1) ^ 0x1021;
		} else {
			crc <<= 1;
		}
	}

	return(crc);
}

UInt16 Calc_crc(UInt8 *data, UInt16 len) 
{
	UInt16 i,crc;

	crc = 0x00;
	for (i=0; i<len; i++) {
		crc = crc_update(crc, (UInt8)*(data+i));
	}
	return(crc);
}

Boolean Send_command(UInt8 *cmdarray, UInt16 len)
{
	UInt16 i;
	// clear serial port
	ClearSerial();

	// send command
		for(i=0; i<len; i++) {
//		HostTraceOutputTL(appErrorClass, "Data Sent %hu",(UInt16)cmdarray[i]);
		serial_out(cmdarray[i]);
	}
	return(true);
}

UInt16 Wait_Frame_Start() 
{
	UInt8 c = 0;
	UInt32 secs = TimGetSeconds();
	UInt16 len = 0;

	do {
		while (!serial_in(&c)) if (TimGetSeconds() > secs+2) return(0);
//		HostTraceOutputTL(appErrorClass, "Data In %hu",(UInt16)c);
	} while (c != 0x73);
//	HostTraceOutputTL(appErrorClass, "Frame Start");

	// get frame length
	while (!serial_in(&c)) if (TimGetSeconds() > secs+2) return(0);
	len = c;
	while (!serial_in(&c)) if (TimGetSeconds() > secs+2) return(0);
	len += c*256;
//	HostTraceOutputTL(appErrorClass, "Frame length %hu",len);

	return(len);	
}

Boolean Read_Frame(UInt8 *dataarray, UInt16 len)
{
	UInt16 i;
	UInt32 secs = TimGetSeconds();
	UInt16 crc = 0;
	UInt8 c = 0;

	if (len == 0) return(false);
//	HostTraceOutputTL(appErrorClass, "Data Length %hu", len);

	dataarray[0] = len%256;
	crc = crc_update(crc, dataarray[0]);
//	HostTraceOutputTL(appErrorClass, "Data In %hx",(UInt16)dataarray[0]);
	dataarray[1] = len/256;
	crc = crc_update(crc, dataarray[1]);
//	HostTraceOutputTL(appErrorClass, "Data In %hx",(UInt16)dataarray[1]);

	for (i=2; i<len; i++) {
		while (!serial_in(&c)) if (TimGetSeconds() > secs+2) return(false);
//		HostTraceOutputTL(appErrorClass, "Data Ct %hu",i);
//		HostTraceOutputTL(appErrorClass, "Data In %hx",(UInt16)c);
		if (c == 0x78) {
			// escape char received, need to decode next char
			while (!serial_in(&c)) if (TimGetSeconds() > secs+2) return(false);
			switch (c) {
				case 0x55:
					c = 0x78;
					break;
				case 0x31:
					c = 0x73;
					break;
				default:
					break;
			}
		}
		dataarray[i] = c;
		if ((i != 6) && (i != 7)) crc = crc_update(crc, c);
		secs = TimGetSeconds();
	}
//	HostTraceOutputTL(appErrorClass, "Data read %hu", i);

//	HostTraceOutputTL(appErrorClass, "crc %hx",crc);
	if ((crc%256 == dataarray[6]) && (crc/256 == dataarray[7])) {
		// crc is OK
		return(true);
	}
	return(false);
}

Boolean Flarm_reset()
{	
	UInt8 	cmdarray[9];
	UInt16  crc = 0;

	// reset Flarm to NMEA mode
	// build command
	SeqNo++; 			// increment sequence no.
	cmdarray[0] = 0x73;		// start frame
	cmdarray[1] = 0x08;		// frame length
	cmdarray[2] = 0x00;		//
	cmdarray[3] = 0x01;		// version
	cmdarray[4] = SeqNo%256; 	// sequence no.
	cmdarray[5] = SeqNo/256;	//
	cmdarray[6] = 0x12;		// reset command
	crc = Calc_crc(&cmdarray[1], 6);// calculate crc
	cmdarray[7] = crc%256; 		// crc
	cmdarray[8] = crc/256;		//
	Send_command(&cmdarray[0], 9);

	return(true);
}

Boolean Flarm_ping()
{
	UInt8  cmdarray[9];
	UInt16 crc = 0;
	UInt16 frame_length = 0;
	UInt16 SeqChk = 0;
	UInt16 i;

	// build command
	SeqNo++; 			// increment sequence no.
	cmdarray[0] = 0x73;		// start frame
	cmdarray[1] = 0x08;		// frame length
	cmdarray[2] = 0x00;		//
	cmdarray[3] = 0x01;		// version
	cmdarray[4] = SeqNo%256; 	// sequence no.
	cmdarray[5] = SeqNo/256;	//
	cmdarray[6] = 0x01;		// ping command
	crc = Calc_crc(&cmdarray[1], 6);// calculate crc
	cmdarray[7] = crc%256; 		// crc
	cmdarray[8] = crc/256;		//

	// check connection up to 5 times
	for (i=0; i<5; i++) {
//		HostTraceOutputTL(appErrorClass, "Ping %hu", i);
		Sleep(0.1);
		Send_command(&cmdarray[0], 9);

		// read response
		frame_length = Wait_Frame_Start();
//		HostTraceOutputTL(appErrorClass, "Got frame length 10");
		if (Read_Frame(&cmdarray[0], frame_length)) {
			if (cmdarray[5] == 0xA0) {
//				HostTraceOutputTL(appErrorClass, "got ACK");
				SeqChk = cmdarray[8] + cmdarray[9]*256;
//				HostTraceOutputTL(appErrorClass, "Seq Check %hu",SeqChk);
				if (SeqChk == SeqNo) {
					return(true);
				}
			}
		}
	}
	return(false);
}

Boolean Flarm_Version()
{
	Char output_char[15];
	Char input_char[35];
	Char tempchar[10];

	// get Flarm version numbers
	StrCopy(output_char, "$PFLAV,R*");
	StrCat(output_char, CalcChkSum(output_char));
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);
	if (WaitFor("PFLAV,A,", USESER)) {
		GetData(input_char, 34, USESER);
		GetField(input_char, 0, tempchar);
		Flarm_HW_Version = StrToDbl(tempchar);
//		HostTraceOutputTL(appErrorClass, "HW Ver %s", DblToStr(Flarm_HW_Version,2));
		GetField(input_char, 1, tempchar);
		Flarm_SW_Version = StrToDbl(tempchar);
//		HostTraceOutputTL(appErrorClass, "SW Ver %s", DblToStr(Flarm_SW_Version,2));
	} else {
		FrmCustomAlert(WarningAlert, "Data Error!"," "," ");
		return(false);
	}
	return(true);
}

Boolean Flarm_binary_connect()
{
	Char output_char[15];

	// get Flarm version numbers
	if (!Flarm_Version()) return(false);

	// put Flarm into binary mode
	StrCopy(output_char, "$PFLAX*");
	StrCat(output_char, CalcChkSum(output_char));
	StrCat(output_char, "\r\n");
	TxData(output_char, USESER);

	// check connection
	if (!Flarm_ping()) return(false);

	// set baud rate
	if (!FlarmSetPortSpeed(data.config.dataspeed)) return(false);

	return(true);	
}

Boolean FlarmSetPortSpeed(UInt8 requestedspeed)
{
	UInt8 flarmspeed = 255;
	UInt16 palmspeed = 2;
	Char tempchar[7];

	UInt8 cmdarray[10];
	UInt16 crc = 0;
	UInt16 frame_length = 0;
	UInt8 *frame_data = NULL;
	Boolean cmdOK = false;
	Boolean seqOK = false;

	switch (requestedspeed) {
		case 0:
		case 1:
		case 2:
//			HostTraceOutputTL(appErrorClass, "4800");
			flarmspeed = 0;
			palmspeed = 2;
			break;
		case 3:
		case 4:
//			HostTraceOutputTL(appErrorClass, "9600");
			flarmspeed = 1;
			palmspeed = 3;
			break;
		case 5:
//			HostTraceOutputTL(appErrorClass, "19200");
			flarmspeed = 2;
			palmspeed = 5;
			break;
		case 6:
//			HostTraceOutputTL(appErrorClass, "28800");
			flarmspeed = 3;
			palmspeed = 6;
			break;
		case 7:
//			HostTraceOutputTL(appErrorClass, "38400");
			flarmspeed = 4;
			palmspeed = 7;
			break;
		case 8:
		case 9:
//			HostTraceOutputTL(appErrorClass, "57600");
			flarmspeed = 5;
			palmspeed = 8;
			break;
		default:
			flarmspeed = 255;
			break;
	}

	if (flarmspeed < 255) {

		// build command
		SeqNo++; 				// increment sequence no.
		cmdarray[0] = 0x73;			// start frame
		cmdarray[1] = 0x09;			// frame length
		cmdarray[2] = 0x00;			//
		cmdarray[3] = 0x01;			// version
		cmdarray[4] = SeqNo%256; 		// sequence no.
		cmdarray[5] = SeqNo/256;		//
		cmdarray[6] = 0x02;			// set baud rate
		cmdarray[7] = flarmspeed;		// baud rate (only here for crc check)
		crc = Calc_crc(&cmdarray[1], 7);	// calculate crc
		cmdarray[7] = crc%256; 			// crc
		cmdarray[8] = crc/256;			//
		cmdarray[9] = flarmspeed;		// baud rate
		Send_command(&cmdarray[0], 10);

		// read response
		frame_length = Wait_Frame_Start();
//		HostTraceOutputTL(appErrorClass, "frame length %hu", frame_length);
		AllocMem((void *)&frame_data, frame_length+1);
		if (Read_Frame(frame_data, frame_length)) {
//			HostTraceOutputTL(appErrorClass, "Msg %hx", (UInt16)frame_data[5]);
			cmdOK = (frame_data[5] == 0xA0);
			seqOK = (frame_data[8]+frame_data[9]*256 == SeqNo);
			FreeMem((void *)&frame_data);
			if (cmdOK & seqOK) {
				// baud changed OK so set palm port
				XferClose(USESER);
				SysStringByIndex(form_set_port_speed_table, palmspeed, tempchar, 7);
//				HostTraceOutputTL(appErrorClass, "Palm %s", tempchar);
				XferInit(tempchar, NFC, USESER);
				
				// check connection
				if (Flarm_ping()) return(true);
			}
		}
		if (frame_data) FreeMem((void *)&frame_data);
	}

	return(false);
}

Boolean FlarmDataErr(Boolean declaretoSD)
{
	HandleWaitDialog(false);
	FrmCustomAlert(WarningAlert,errstring, " Data Error!", " ");
	if (declaretoSD) {
		// close SD file
		XferClose(FLARMxfertype);
		FLARMxfertype = USESER;
	}
	return(false);
}
