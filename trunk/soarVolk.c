#include <PalmOS.h>	// all the system toolbox headers
#include <SerialMgr.h>
#include <SerialMgrOld.h>
#include "soaring.h"
#include "soarIO.h"
#include "soarUtil.h"
#include "soarVolk.h"
#include "soarForm.h"
#include "soarMem.h"
#include "soarDB.h"
#include "soarMath.h"
#include "soarUMap.h"
#include "Mathlib.h"
#include "soarCAI.h"
#include "soarWay.h"

// used for testing only
#define VOLKSDIR	
#define VOLKSDLOAD
//#define VOLKSTESTFILES

// Modified from code supplied by Harald Maier and Georg Garrcht
Boolean roundup = false; // controls if sector/line/cylinder sizes are rounded up or down.
UInt8 VLKcommspeed = 1; // 9600 by volkslogger definition
UInt8 VLKdataspeed = 1; // set to 9600 default
Char c36[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // FAI base36 format
Int16 pos_ds_size[max_bfv+1][2] = {{11,0},{12,9}};
UInt8 *dbb;
UInt8 *dbb2;
VLKinfo volks;
Boolean havedbb = false;
Boolean IsInterrupted = false;
UInt8 zero = 0;
Int16 TAKEOFFSET=0, LANDOFFSET=0;
Char output_char[82];
VLKwpt wpttmp[16];

extern TaskData *tsk;
extern CAILogData *cailogdata;
extern Int16 cainumLogs;
extern Int16 selectedCAIFltIndex;

// variables used in testing only
UInt32 maxlen;
Boolean initstat;

//********************************************
//Functions called from main soarpilot program
//********************************************

Boolean VLKgetloginfo(Int16 cmd)
{
	Int32 ok = 0;

	if (cmd == CAIFIFREE) {
		// free memory
		if (cailogdata) {
			FreeMem((void *)&cailogdata);
			cailogdata = NULL;
		} 

		// restart logger to ensure it's running
		VLKsendcommand(cmd_RST,0,0) ;

		return(true);
	}

#ifdef VOLKSDIR
	// attempt to connect to the volkslogger
	HandleWaitDialog(true);
	if (!VLKconnect()) {
		HandleWaitDialog(false);
		return(false);
	}
#endif

	// allocate memory for the flight database
	// 64k not the full 80k, OK since reading the directory only
	AllocMem((void *)&dbb, MAX_MEMSIZE);

#ifdef VOLKSDIR
	// get serial number information
	VLKgetinfo(&volks);

	// read the flight directory
	if (VLKsendcommand(cmd_DIR,0,0) == 0) {
		if (VLKsetportspeed(VLKdataspeed)) {
			ok = VLKreadblock(dbb, 0, MAX_MEMSIZE);
		}
	}

#ifdef VOLKSTESTFILES
	// dump dbb to test file
	maxlen = MAX_MEMSIZE;
	initstat = XferInit("vlkdir.txt", IOOPENTRUNC, USEVFS);
	HandleVFSData(IOWRITE, dbb, &maxlen);
	XferClose(USEVFS);
#endif

#else
	// read dbb from test file 
	maxlen = MAX_MEMSIZE;
	initstat = XferInit("vlkdir.txt", IOOPEN, USEVFS);
	HandleVFSData(IOREAD, dbb, &maxlen);
	XferClose(USEVFS);
	ok = 1;
#endif

	// find number of flights in logger
	if (ok > 0) {
		cainumLogs = VLKconvdir(NULL, true);
//		HostTraceOutputTL(appErrorClass, "Num logs %s",DblToStr(cainumLogs,0));
	}

	// copy to CAI log structure for list
	if ((ok > 0) & (cainumLogs > 0)) {
//		HostTraceOutputTL(appErrorClass, "Set Up Log Info");

		AllocMem((void *)&cailogdata, sizeof(CAILogData)*cainumLogs);
		MemSet(cailogdata, cainumLogs * sizeof(CAILogData), 0);

		cainumLogs = VLKconvdir(cailogdata, false);
	}

	// free the memory
	FreeMem((void *)&dbb);

	HandleWaitDialog(false);
	return(ok);
}

Boolean VLKDownloadSelectedLog()
{ 
	Int16 cret;
	Boolean secmode;
	Char igcname[25];
	Boolean ok = true;
	UInt32 groesse;
	UInt32 sgr;

//	HostTraceOutputTL(appErrorClass, "Getting Flight: %s", DblToStr(selectedCAIFltIndex ,0));

	// find IGC output filename
	MemSet(igcname, sizeof(igcname) ,0);
	igcname[0] = c36[cailogdata[selectedCAIFltIndex].startDate.year % 10];
	igcname[1] = c36[cailogdata[selectedCAIFltIndex].startDate.month];
	igcname[2] = c36[cailogdata[selectedCAIFltIndex].startDate.day];
	StrCat(igcname, MFR_ID2);
	StrCat(igcname, volks.serialno);
	igcname[StrLen(igcname)] = (cailogdata[selectedCAIFltIndex].FlightOfDay < 36 ? c36[cailogdata[selectedCAIFltIndex].FlightOfDay] : '_');
	StrCat(igcname, ".IGC");
//	HostTraceOutputTL(appErrorClass, "IGC Filename: %s", igcname);

	// ask for signed secure file or not
//	secmode = (FrmCustomAlert(ConfirmAlertYN, "Include secure signature?"," "," ") == YES);
	secmode = true;

#ifdef VOLKSDLOAD
	// confirm connection to the volkslogger
	HandleWaitDialog(true);
	if (!VLKconnect()) {
		HandleWaitDialog(false);
		return(false);
	}
#endif

	// allocate memory for the full 80k log memory
//	HostTraceOutputTL(appErrorClass, "Allocate 80k memory");
	AllocMem((void *)&dbb, MAX_MEMSIZE);
	AllocMem((void *)&dbb2, VLK_LOG_MEMSIZE-MAX_MEMSIZE);
	MemSet(&dbb[0], MAX_MEMSIZE, 0xff);
	MemSet(&dbb2[0], VLK_LOG_MEMSIZE-MAX_MEMSIZE, 0xff);

	cret = 0;
	groesse = 0;
	sgr = 0;

#ifdef VOLKSDLOAD
	// read binary flightlog
	if (secmode) {
		cret = (VLKsendcommand(cmd_GFS, selectedCAIFltIndex, VLKdataspeed) == 0);
	} else {
		cret = (VLKsendcommand(cmd_GFL, selectedCAIFltIndex, VLKdataspeed) == 0);
	}
	if (!cret) ok = false;

	if (ok) {
//		HostTraceOutputTL(appErrorClass, "Getting binary flight log");
		VLKsetportspeed(VLKdataspeed);
		groesse = VLKreadBIGblock(0, VLK_LOG_MEMSIZE);
		if (groesse <= 0) {
			ok = false;
		} else {
//			HostTraceOutputTL(appErrorClass, "Flight log OK");
		}

		if (ok) {
			// read signature
			VLKsetportspeed(VLKcommspeed);
			Sleep(1);
//			HostTraceOutputTL(appErrorClass, "Getting signature");
			if (VLKsendcommand(cmd_SIG, 0,0) == 0) {
				if ((sgr = VLKreadBIGblock(groesse, VLK_LOG_MEMSIZE-groesse)) <= 0) {
				 	ok = false;
				} else {
//					HostTraceOutputTL(appErrorClass, "Signature OK");
				}
			}
		}
	}

#ifdef VOLKSTESTFILES
	// dump dbb to test file
	initstat = XferInit("vlkflt.vlk", IOOPENTRUNC, USEVFS);
	maxlen = MAX_MEMSIZE;
	HandleVFSData(IOWRITE, dbb, &maxlen);
	maxlen = VLK_LOG_MEMSIZE-MAX_MEMSIZE;
	HandleVFSData(IOWRITE, dbb2, &maxlen);
	XferClose(USEVFS);
#endif

#else
	// read dbb from test file 
	maxlen = MAX_MEMSIZE;
	initstat = XferInit("vlkflt.vlk", IOOPEN, USEVFS);
	HandleVFSData(IOREAD, dbb, &maxlen);
	XferClose(USEVFS);
	ok = true;
	StrCopy(igcname, "test.IGC");
#endif

	// decode the data to IGC file format
//	HostTraceOutputTL(appErrorClass, "Decode to IGC file");	
	if (ok) {
		// open the IGC output file
		XferInit(igcname, IOOPENTRUNC, data.config.xfertype);

		// convert to IGC format
		ok = VLKconvIGC();

		// close output file
		XferClose(data.config.xfertype);	
	}

	// free the memory
//	HostTraceOutputTL(appErrorClass, "Free Memory");
	FreeMem((void *)&dbb2);	
	FreeMem((void *)&dbb);	

	HandleWaitDialog(false);
	if (ok) {
		FrmCustomAlert(FinishedAlert, "Flight Log Download Succeeded!"," "," ");
	}
	return(ok);
}

Boolean DeclareVLKTask()
{
	Boolean ok = false;
	Int16 pos;
	Int16 i;
	UInt8 numwp;
	Char tempchar[65];
	Char tempchar2[26];

	// attempt to connect to the volkslogger
	HandleWaitDialog(true);
	if (!VLKconnect()) {
		HandleWaitDialog(false);
		return(false);
	}

	// allocate memory for the database
	AllocMem((void *)&dbb, VLK_DBB_MEMSIZE);

	// read dbb from test file 
//	maxlen = VLK_DBB_MEMSIZE;
//	initstat = XferInit("vlkdata.txt", IOOPEN, USEVFS);
//	HandleVFSData(IOREAD, dbb, &maxlen);
//	XferClose(USEVFS);

	// get serial number information
	VLKgetinfo(&volks);

	// read database
	if (VLKReadDatabase()) {
//		HostTraceOutputTL(appErrorClass, "Read Database ok");

		// dump dbb to test file
//		maxlen = VLK_DBB_MEMSIZE;
//		initstat = XferInit("vlkdata.txt", IOOPENTRUNC, USEVFS);
//		HandleVFSData(IOWRITE, dbb, &maxlen);
//		XferClose(USEVFS);

		// clear declaration portion
		MemSet(&dbb[VLK_DECLSTART], VLK_DECLEND-VLK_DECLSTART, 0xff);
		pos = 0;

		// write pilot data - up to 64 chars (4 x 16)
		MemSet(tempchar, sizeof(tempchar) , 0);
		StrCopy(tempchar, data.igchinfo.name);
		ConvertToUpper(tempchar);
		for (i = 0; i < 4; i++) {
			MemSet(tempchar2, sizeof(tempchar2), 0);
			MemMove(tempchar2, &tempchar[i*16], 16);
			pos = VLKAddDeclarationField(FLDPLT1+i, pos, tempchar2, StrLen(tempchar2)+1);
		}

		// write glider type
		MemSet(tempchar2, sizeof(tempchar2), 0);
		StrCopy(tempchar2, data.igchinfo.type);
		ConvertToUpper(tempchar2);
		tempchar2[12] = '\0'; // max 12 chars
		pos = VLKAddDeclarationField(FLDGTY, pos, tempchar2, StrLen(tempchar2)+1);

		// write glider id
		MemSet(tempchar2, sizeof(tempchar2), 0);
		StrCopy(tempchar2, data.igchinfo.gid);
		ConvertToUpper(tempchar2);
		tempchar2[7] = '\0'; // max 7 chars
		pos = VLKAddDeclarationField(FLDGID, pos, tempchar2, StrLen(tempchar2)+1);

		// write comp class
		MemSet(tempchar2, sizeof(tempchar2), 0);
		StrCopy(tempchar2, data.igchinfo.cls);
		ConvertToUpper(tempchar2);
		tempchar2[12] = '\0'; // max 12 chars
		pos = VLKAddDeclarationField(FLDCCL, pos, tempchar2, StrLen(tempchar2)+1);

		// write comp id
		MemSet(tempchar2, sizeof(tempchar2), 0);
		StrCopy(tempchar2, data.igchinfo.cid);
		ConvertToUpper(tempchar2);
		tempchar2[3] = '\0'; // max 3 chars
		pos = VLKAddDeclarationField(FLDCID, pos, tempchar2, StrLen(tempchar2)+1);

		// take off (home point)
		if (tsk->hastakeoff) {
			TAKEOFFSET = 1;
			pos = VLKputdeclwp(0, pos, FLDTKF);
		} else {
			TAKEOFFSET = 0;
//			MemSet(tempchar2, sizeof(tempchar2), 0);
//			StrCopy(tempchar2, "      ");
//			pos = VLKAddDeclarationField(FLDTKF, pos, tempchar2, 16);
		}

		if (tsk->haslanding) {
			LANDOFFSET = tsk->numwaypts - 1;
		} else {
			LANDOFFSET = tsk->numwaypts;
		}

		// number of turnpoints
		numwp = LANDOFFSET - TAKEOFFSET - 2 - tsk->numctlpts;
//		HostTraceOutputTL(appErrorClass, "numwp %s",DblToStr(numwp,0));
		pos = VLKAddDeclarationField(FLDNTP, pos, &numwp, 1);

		// start 
		pos = VLKputdeclwp(TAKEOFFSET, pos, FLDSTA);

		// finish 
		pos = VLKputdeclwp(LANDOFFSET-1, pos, FLDFIN);

		// landing (Can't set this using the volkslogger only!)
		if (tsk->haslanding) {
			pos = VLKputdeclwp(LANDOFFSET, pos, FLDLDG);		
		}
		
		// Turnpoints
		if (numwp > 0) {
			numwp = 0;
			for (i = TAKEOFFSET+1; i < LANDOFFSET-1; i++) {
				if ((tsk->waypttypes[i] & CONTROL) == 0) {
					pos = VLKputdeclwp(i, pos, FLDTP1+numwp);
					numwp++;
				}
			}	
		}

		// write database
		if (VLKwriteDatabase()) {
//			HostTraceOutputTL(appErrorClass, "Write Database ok");
			ok = true;
		}

		// dump dbb to testfile
//		maxlen = VLK_DBB_MEMSIZE;
//		initstat = XferInit("vlkdata2.txt", IOOPENTRUNC, USEVFS);
//		HandleVFSData(IOWRITE, dbb, &maxlen);
//		XferClose(USEVFS);
	}

	// free the memory
	FreeMem((void *)&dbb);

	// restart logger to ensure it's running
	VLKsendcommand(cmd_RST,0,0) ;

	HandleWaitDialog(false);
	return(ok);
}

Boolean ClearVLKDeclaration()
{
	Boolean ok = false;
	Int16 pos = 0;

	// attempt to connect to the volkslogger
	HandleWaitDialog(true);
	if (!VLKconnect()) {
		HandleWaitDialog(false);
		return(false);
	}

	// allocate memory for the database
	AllocMem((void *)&dbb, VLK_DBB_MEMSIZE);

	// get serial number information
	VLKgetinfo(&volks);

	// read database
	if (VLKReadDatabase()) {
//		HostTraceOutputTL(appErrorClass, "Read Database ok");

		// clear declaration portion and write minimum fields
		MemSet(&dbb[VLK_DECLSTART], VLK_DECLEND - VLK_DECLSTART, 0xff);
		pos = VLKAddDeclarationField(FLDPLT1, pos, &zero, 1);
		pos = VLKAddDeclarationField(FLDGTY,  pos, &zero, 1);
		pos = VLKAddDeclarationField(FLDNTP,  pos, &zero, 1);
		
		// write database
		if (VLKwriteDatabase()) {
//			HostTraceOutputTL(appErrorClass, "Write Database ok");
			ok = true;
		}
	}

	// free the memory
	FreeMem((void *)&dbb);

	// restart logger to ensure it's running
	VLKsendcommand(cmd_RST,0,0) ;

	HandleWaitDialog(false);
	return(ok);
}

Boolean DeleteVLKLogs()
{
	Boolean ok = false;
	
	// check for volkslogger
	HandleWaitDialog(true);
	if (!VLKconnect()) {
		HandleWaitDialog(false);
		return(false);
	}

	// allocate memory for the database
	AllocMem((void *)&dbb, VLK_DBB_MEMSIZE);

	// get serial number information
	VLKgetinfo(&volks);

	// free the memory
	FreeMem((void *)&dbb);

	// send delete command
	ok = (VLKsendcommand(cmd_CFL,0,0) == 0);
	HandleWaitDialog(false);	
	if (ok) {
		FrmCustomAlert(FinishedAlert, "Deleting Volkslogger Flights Succeeded"," "," ");
	} else {
		FrmCustomAlert(WarningAlert, "Deleting Volkslogger Flights Failed!"," "," ");
	}
	return(ok);
}

//******************************
//Internal Volkslogger functions
//******************************

UInt16 Crc16Table[256] = {
   0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
   0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
   0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
   0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
   0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
   0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
   0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
   0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
   0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
   0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
   0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
   0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
   0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
   0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
   0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
   0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
   0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
   0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
   0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
   0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
   0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
   0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
   0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
   0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
   0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
   0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
   0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
   0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
   0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
   0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
   0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
   0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

UInt16 UpdateCRC(UInt8 Octet, UInt16 CRC) 
{
	return  0xffff & ( (CRC << 8) ^ Crc16Table[ (CRC >> 8) ^ Octet ] );
}

Boolean VLKconnect()
{
	Int16 i, lc;
	UInt8 c = 0;
	UInt32 secs;

//	HostTraceOutputTL(appErrorClass, "-----------------------------------");

	// set volkslogger comm speed
	VLKsetportspeed(VLKcommspeed);

	// wake up logger
	Sleep(1);
	ClearSerial();
 	for(i=0; i<10; i++) {
		serial_out(CAN);
	}

	// send stream of R's and wait for an L
	secs = TimGetSeconds();
	do {
		serial_out('R');
	} while ((TimGetSeconds() < secs+5) && (serial_in(&c) || c != 'L'));

	if (TimGetSeconds() > secs+5) {
//			HostTraceOutputTL(appErrorClass, "VLK Connection timed out!");
			Sleep(1);
			ClearSerial();
			return(false);
	}
//	HostTraceOutputTL(appErrorClass, "Got an L");

	// wait for 4 L's
	lc = 1;
	secs = TimGetSeconds();
	do {
		if (!serial_in(&c)) {
			if (c == 'L') {
				lc++;	
//				HostTraceOutputTL(appErrorClass, "Got an L");	
				if (lc >= 4) {
//					HostTraceOutputTL(appErrorClass, "VLK Connected to Volkslogger");
					Sleep(1);
					ClearSerial();
					return(true);
				}
			}
		}
	} while((TimGetSeconds() < secs+5));

//	HostTraceOutputTL(appErrorClass, "VLK Connection timed out!");
	Sleep(1);
	ClearSerial();
	return(false);
}

UInt8 findVLKportspeed(UInt8 palmspeed)
{
	// converts soarpilot port speeds to the equiv volkslogger
	switch (palmspeed) {
		case 0: return(1); //9600
		case 1: return(1); //9600
		case 2: return(1); //9600
		case 3: return(1); //9600
		case 4: return(1); //9600
		case 5: return(2); //19200
		case 6: return(2); //19200
		case 7: return(3); //38400
		case 8: return(4); //57600
		case 9: return(5); //115200
		default: return(1); //9600
	}			
}

Boolean VLKsetportspeed(UInt8 portspeed)
{
	Int32 VLKspeedtable[8] = {9600, 9600, 19200, 38400, 57600, 115200, 115200, 115200};
	Char tempchar[7];

	XferClose(USESER);
	XferInit(StrIToA(tempchar, VLKspeedtable[portspeed & 0x07]), NFC, USESER);
	return(true);
}

Int16 VLKsendcommand(UInt8 cmd, UInt8 param1, UInt8 param2) 
{
	UInt16	i;
	UInt8	c = 0;
//	double 	d = 0.002;
	UInt8 	cmdarray[8];
	UInt16  crc16 = 0;
	Int32	t1;
	Int16 	timeout = 4;

//	HostTraceOutputTL(appErrorClass, "-----------------------------------");
//	HostTraceOutputTL(appErrorClass, "Sending Command : %hd", cmd);

	MemSet(&cmdarray, sizeof(cmdarray), 0);

	// alte Zeichen verwerfen
	Sleep(1);
	ClearSerial();

	// Kommandointerpreter im VL zuruecksetzen
	for(i=0; i<6; i++) {
		serial_out(CAN);
//		Sleep(d);
	}

	// Kommandopaket aufbauen
	cmdarray[0] = cmd;
	cmdarray[1] = param1;
	cmdarray[2] = param2;

	// Kommando verschicken ( ENQ,Daten,CRC )
	serial_out(ENQ);
//	Sleep(d);
	for(i=0; i<sizeof(cmdarray); i++) {
		crc16 = UpdateCRC(cmdarray[i],crc16);
		serial_out(cmdarray[i]);
//		Sleep(d);
	}
	serial_out(crc16/256);
//	Sleep(d);
	serial_out(crc16%256);
//	Sleep(d);

	// Kommandobestaetigung abwarten, aber hoechstens timeout Sekunden
	t1 = TimGetSeconds();
	while( !serial_in(&c) && TimGetSeconds()<t1+timeout ) {
//		HostTraceOutputTL(appErrorClass, "From Volks : %s",BytetoChar(c));
	}
	// Timeoutbehandlung
	if (TimGetSeconds() >= t1+timeout)
		c = 255;

//	HostTraceOutputTL(appErrorClass, "Command Reply : %hd", c);

/*	// Fehler (timeout oder Fehlercode vom VL) im Klartext ausgeben
	switch (c) {
	case 0:
//		HostTraceOutputTL(appErrorClass, "VLS_TXT_NIL");
		break;
 	case 1:
//		HostTraceOutputTL(appErrorClass, "VLS_TXT_BADCMD");
		break;
	case 2:
//		HostTraceOutputTL(appErrorClass, "VLS_TXT_WRONGFR");
		break;
	case 255:
//		HostTraceOutputTL(appErrorClass, "VLS_TXT_NOFR");
		break;
	}
*/
	// Fehlercode als Rueckgabewert der Funktion benutzen
	return(c);
	// Rueckgabewert:     -1 : timeout
	//		 0..255 : Bestaeigungscode vom Logger
}

Boolean VLKReadDatabase() 
{
	Boolean ok = false;

	// set volkslogger data speed
	VLKdataspeed = findVLKportspeed(data.config.dataspeed);

	// read database
	if (VLKsendcommand(cmd_RDB, 0, VLKdataspeed) == 0) {
		if (VLKsetportspeed(VLKdataspeed)) {
			ok = (VLKreadblock(dbb, 0, VLK_DBB_MEMSIZE) == VLK_DBB_MEMSIZE-2);
			if (ok) VLKdbbinit(&volks);	

		}
	}

	VLKsetportspeed(VLKcommspeed);
	return(ok);
}

Boolean VLKwriteDatabase() 
{
	Boolean ok = false;

	if (VLKsendcommand(cmd_PDB, 0, 0) == 0) {
		ok = (VLKwriteblock(dbb, VLK_DBB_MEMSIZE) == VLK_DBB_MEMSIZE);
	}

	VLKsetportspeed(VLKcommspeed);
	return(ok);
}

Boolean VLKgetinfo(VLKinfo *volks)
{
	Boolean ok = false;
	UInt32 serialNo;

	// display serial number, firmware version and build
	if (VLKsendcommand(cmd_INF,0,0) == 0) {
		ok = VLKreadblock(dbb, 0, VLK_DBB_MEMSIZE);
		if (ok) {
			serialNo = 256*dbb[2]+dbb[3];
			volks->serialno[0] = c36[(Int16)(serialNo / 36 / 36)];
			volks->serialno[1] = c36[((Int16)(serialNo / 36) % 36)];
			volks->serialno[2] = c36[(Int16)(serialNo % 36)];
			volks->serialno[3] = '\0';
			volks->fwmajor = dbb[4]/16;
			volks->fwminor = dbb[4]%16;
			volks->build = dbb[7];

//			HostTraceOutputTL(appErrorClass, "Serial No. : %s", volks->serialno);
//			HostTraceOutputTL(appErrorClass, "Firmware   : %s", DblToStr((double)volks->fwmajor + (double)volks->fwminor/10,1));		
//			HostTraceOutputTL(appErrorClass, "Build      : %hd", volks->build);
		}
	}
	return(ok);
}

UInt8 readbuffer(Int32 position)
{
	if (position >= MAX_MEMSIZE) {
		return(dbb2[position-MAX_MEMSIZE]);
	} else {
		return(dbb[position]);
	}
}

void writebuffer(Int32 position, UInt8 data)
{
	if (position >= MAX_MEMSIZE) {
		dbb2[position-MAX_MEMSIZE] = data;
	} else {
		dbb[position] = data;
	}
}

Int32 VLKreadBIGblock(Int32 startpos, Int32 maxlen)
{ 
// same as VLKreadblock function, but data split
// allows program to read >64kb of data

	UInt8 c;
	Boolean dle_r = false;
	UInt16 crc16 = 0;
	Boolean start = false;
	Boolean ende = false;
	Int32 idx;

	idx = startpos;
	Sleep(1);
	ClearSerial();

	while (!ende) {
		// Zeichen anfordern und darauf warten
		serial_out(ACK);     
//		HostTraceOutputTL(appErrorClass, "Send ACK");   
		while (!serial_in(&c)) {
			if (IsInterrupted) {
				break;
			}
		}

		if (IsInterrupted) {
		serial_out(CAN);
		serial_out(CAN);
		serial_out(CAN);
			break;
		}

		switch (c) {
		case DLE:
//			HostTraceOutputTL(appErrorClass, "DLE");
			if (!dle_r) {	     //!DLE, DLE -> Achtung!
				dle_r = true;
			}
			else { 			 // DLE, DLE -> DLE-Zeichen
				dle_r = false;
				if (start && idx < maxlen) {
//				buffer[idx++] = c;
				writebuffer(idx++,c);
				crc16 = UpdateCRC(c, crc16);
			}
		}
		break;
		case ETX:
//			HostTraceOutputTL(appErrorClass, "ETX");
			if (!dle_r) {	     //!DLE, ETX -> Zeichen
				if (start && idx < maxlen) {
//					buffer[idx++] = c;
					writebuffer(idx++,c);
					crc16 = UpdateCRC(c, crc16);
				}
			}
			else if (start) {
				ende = true;		   // DLE, ETX -> Blockende
				dle_r = false;
			}
		break;
		case STX:
//			HostTraceOutputTL(appErrorClass, "STX");
			if (!dle_r) {		 //!DLE, STX -> Zeichen
				if (start && idx < maxlen) {
//					buffer[idx++] = c;
					writebuffer(idx++,c);
					crc16 = UpdateCRC(c, crc16);
				}
			}
			else {
				start = true;	   // DLE, STX -> Blockstart
				dle_r = false;
				crc16 = 0;
			}
		break;
		default:
//			HostTraceOutputTL(appErrorClass, "Data %s", BytetoChar(c));
//			HostTraceOutputTL(appErrorClass, "Data %hd", c);
			if (start) {
				if (idx < maxlen) {
//				buffer[idx++] = c;
				writebuffer(idx++,c);
			}
				crc16 = UpdateCRC(c, crc16);
			}
		break;
		}
	}

	if (crc16 != 0) {
//		HostTraceOutputTL(appErrorClass, "CRC Error at position %ld", idx);
		idx = 0;
	} else if (idx > 2) {	      //CRC am Ende abschneiden
		idx -= 2;
		if (idx < maxlen) {
//			buffer[idx] = 0xff;
			writebuffer(idx,0xff);
		}

		if (idx < maxlen - 1) {
//			buffer[idx + 1] = 0xff;
			writebuffer(idx + 1,0xff);
		}
	} else {
		idx = 0;
	}

//	HostTraceOutputTL(appErrorClass, "Received Bytes : %ld",idx);
	return(idx);
}

Int32 VLKreadblock(UInt8 *buffer, Int32 startpos, Int32 maxlen)
{
	UInt8 c;
	Boolean dle_r = false;
	UInt16 crc16 = 0;
	Boolean start = false;
	Boolean ende = false;
	Int32 idx;

	// clear buffers before read
	idx = startpos;
	MemSet(&buffer[startpos], maxlen-startpos, 0xff);
	Sleep(1);
	ClearSerial();

	while (!ende) {
		// Zeichen anfordern und darauf warten
		serial_out(ACK);     
//		HostTraceOutputTL(appErrorClass, "Send ACK");   
		while (!serial_in(&c)) {
		    if (IsInterrupted) {
		       break;
		    }
		}

	   	if (IsInterrupted) {
			serial_out(CAN);
			serial_out(CAN);
			serial_out(CAN);
			break;
	     	}

		switch (c) {
		case DLE:
//    		    HostTraceOutputTL(appErrorClass, "DLE");
		    if (!dle_r) {	     //!DLE, DLE -> Achtung!
			dle_r = true;
		    }
		    else { 			 // DLE, DLE -> DLE-Zeichen
			dle_r = false;
			if (start && idx < maxlen) {
			    buffer[idx++] = c;
			    crc16 = UpdateCRC(c, crc16);
			}
		    }
		    break;
		case ETX:
//    		    HostTraceOutputTL(appErrorClass, "ETX");
		    if (!dle_r) {	     //!DLE, ETX -> Zeichen
			if (start && idx < maxlen) {
			    buffer[idx++] = c;
			    crc16 = UpdateCRC(c, crc16);
			}
		    }
		    else if (start) {
			ende = true;		   // DLE, ETX -> Blockende
			dle_r = false;
		    }
		    break;
		case STX:
//    		    HostTraceOutputTL(appErrorClass, "STX");
		    if (!dle_r) {		 //!DLE, STX -> Zeichen
			if (start && idx < maxlen) {
			    buffer[idx++] = c;
			    crc16 = UpdateCRC(c, crc16);
			}
		    }
		    else {
			start = true;	   // DLE, STX -> Blockstart
			dle_r = false;
			crc16 = 0;
		    }
		    break;
		default: 
//    			HostTraceOutputTL(appErrorClass, "Data %s", BytetoChar(c));
//    			HostTraceOutputTL(appErrorClass, "Data %hd", c);
		    if (start) {

			if (idx < maxlen) {
			    buffer[idx++] = c;
			}
			crc16 = UpdateCRC(c, crc16);
		    }
		    break;
		}
	    }

	    if (crc16 != 0) {
//	      HostTraceOutputTL(appErrorClass, "CRC Error at position %ld", idx);
		idx = 0;
	    } else if (idx > 2) {	      //CRC am Ende abschneiden
		idx -= 2;
		if (idx < maxlen) {
		    buffer[idx] = 0xff;
		}
		
		if (idx < maxlen - 1) {
		    buffer[idx + 1] = 0xff;
		}
	    } else {
		idx = 0;
	    }

//	    HostTraceOutputTL(appErrorClass, "Received Bytes : %ld",idx);
	    return(idx);
}

Int32 VLKwriteblock(UInt8 *buffer, Int32 maxlen)
{
	UInt8 c;
	Int32 i = 0;
	UInt16 crc16 = 0;

	    while (!serial_in(&c)) {
		if (IsInterrupted) {
		    break;
		}
	    }

	    if (c == ACK) {
		for (i = 0; i < maxlen; i++) {
		    if (IsInterrupted) {
			serial_out(CAN);
			serial_out(CAN);
			serial_out(CAN);
			break;
		    }
		    serial_out(buffer[i]);
		    crc16 = UpdateCRC(buffer[i], crc16);
		}

		serial_out((UInt8)(crc16 / 256));
		serial_out((UInt8)(crc16 % 256));

	    }

	    while (!serial_in(&c)) {
		if (IsInterrupted) {
		    i = 0;
		    break;
		}
	    }

	    if (c != ACK) {
		i = 0;
	    }

//	    HostTraceOutputTL(appErrorClass, "Written Bytes : %ld",i);
	    return(i);
}

void VLKdbbinit(VLKinfo *volks)
{
	Int16 i;

	// known table numbers
	// waypoints
	i = 0; 
	volks->wptstart = 256*dbb[6*i+0] + dbb[6*i+1];
	volks->wptend = 256*dbb[6*i+2] + dbb[6*i+3];
	volks->wptlen = dbb[6*i+4];
	if (volks->wptstart == 0xffff) {
		volks->numwpt = 0;
	} else {
		volks->numwpt = (volks->wptend - volks->wptstart) / volks->wptlen + 1;
	}

	// pilots
	i = 1; 
	volks->pltstart = 256*dbb[6*i+0] + dbb[6*i+1];
	volks->pltend = 256*dbb[6*i+2] + dbb[6*i+3];
	volks->pltlen = dbb[6*i+4];
	if (volks->pltstart == 0xffff) {
		volks->numplt = 0;
	} else {
		volks->numplt = (volks->pltend - volks->pltstart) / volks->pltlen + 1;
	}

	// routes
	i = 3; 
	volks->rtestart = 256*dbb[6*i+0] + dbb[6*i+1];
	volks->rteend = 256*dbb[6*i+2] + dbb[6*i+3];
	volks->rtelen = dbb[6*i+4];
	if (volks->rtestart == 0xffff) {
		volks->numrte = 0;
	} else {
		volks->numrte = (volks->rteend - volks->rtestart) / volks->rtelen + 1;
	}

//	HostTraceOutputTL(appErrorClass, "Waypoints : %hd",volks->numwpt);
//	HostTraceOutputTL(appErrorClass, "Pilots    : %hd",volks->numplt);
//	HostTraceOutputTL(appErrorClass, "Routes    : %hd",volks->numrte);

	// set flag if numbers seem reasonable
	if ((volks->numwpt <= 500) && (volks->numplt <= 25) && (volks->numrte <= 25) &&
	    (volks->numwpt >= 0)   && (volks->numplt >= 0)  && (volks->numrte >= 0)) {
		havedbb = true;
	}
}

Int16 VLKAddDeclarationField(Int16 id, Int16 pos, UInt8 *data, Int16 datalen) 
{
	if (pos + datalen + 2  < VLK_DECLEND) {
		dbb[VLK_DECLSTART + pos] = (UInt8)(datalen + 2);
		dbb[VLK_DECLSTART + pos + 1] = (UInt8)id;
		MemMove(&dbb[VLK_DECLSTART + pos + 2], data, datalen);
		pos += datalen + 2;
	}
	return(pos);
}

Int16 VLKputdeclwp(Int16 wptidx, Int16 pos, UInt8 FID)
{
	VLKwpt wpt;
	Char tempchar[13];
	
	StrCopy(tempchar, rightpad(tsk->wayptnames[wptidx], ' ', 6));
	tempchar[6] = 0;
	ConvertToUpper(tempchar);
	StrCopy(wpt.name, tempchar);

	wpt.lat = tsk->wayptlats[wptidx];
	wpt.lon = tsk->wayptlons[wptidx];

	wpt.typ = 0;
	if (tsk->waypttypes[wptidx] & LAND) wpt.typ = 1;
	if (tsk->waypttypes[wptidx] & AIRPORT) wpt.typ = 5;

	dbb[VLK_DECLSTART+pos] = 18;
	dbb[VLK_DECLSTART+pos+1] = FID;

	VLKputwp(&wpt, pos+2);

	if (wptidx == TAKEOFFSET) {
		// start sector
		VLKsector(data.config.starttype, data.config.startrad, data.config.startrad, (data.config.startdirauto?-1:data.config.startdir), pos+15);
	} else if (wptidx == LANDOFFSET -1) {
		// finish sector
		VLKsector(data.config.finishtype, data.config.finishrad, data.config.finishrad, -1,  pos+15);
	} else {
		// turnpoint
		VLKsector(data.config.turntype, data.config.turnfairad, data.config.turncircrad, (data.config.finishdirauto?-1:data.config.finishdir), pos+15);
	}

	pos += 18;
	return(pos);
}

void VLKsector(Int8 type, double fairad, double circrad, double sectdir, Int16 pos)
{
	Int16 i, lw;
	UInt8 w1 = 0, w2 = 0;
	UInt8 rz = 0, rs = 0;
	
	if ((type == TSKLINE) || (type == ARC)) {
		dbb[VLK_DECLSTART+pos+2] = 1;
		if (roundup) {
			lw = (Int16)(Ceil(fairad * DISTKMCONST));
		} else {
			lw = (Int16)(fairad * DISTKMCONST);
		}
		for (i=1; i<=15; i++) {
			if((lw%i == 0) && (lw/i <= 15)) {
				w1 = i;
				w2 = lw/i;
				break;
			}
		}
		dbb[VLK_DECLSTART+pos+1] = (UInt8)((w1 << 4) + w2);

	} else {
		if (roundup) {
			circrad = Ceil(circrad * DISTKMCONST * 10) / 10;
			fairad = Ceil(fairad * DISTKMCONST);
		} else {
			circrad = circrad * DISTKMCONST;
			fairad = fairad * DISTKMCONST;
		}
		dbb[VLK_DECLSTART+pos+2] = 0;
		if ((type == CYLINDER) || (type == BOTH)) {
			if (circrad > 1.5) circrad = 1.5;
			if (circrad < 0.1) circrad = 0.1;
			rz = (UInt8)(circrad*10) & 0x0f;
		}
		if ((type == FAI) || (type == BOTH)) {
			if (fairad > 15.0) fairad = 15.0;
			if (fairad < 1.0) fairad = 1.0;
			rs = (UInt8)(fairad) & 0x0f;
		}
		dbb[VLK_DECLSTART+pos+1] = (UInt8)((rs << 4) + rz);
	}
		
	if (sectdir < 0) {
//		HostTraceOutputTL(appErrorClass, "AUTO");
		dbb[VLK_DECLSTART+pos] = 180;
	} else {
//		HostTraceOutputTL(appErrorClass, "Set %s",DblToStr(sectdir,0));
		dbb[VLK_DECLSTART+pos] = (UInt8)nice_brg(sectdir)/2;
	}
}

void VLKputwp(VLKwpt *wpt, Int16 pos)
{
	// put a waypoint
	Int32 llat, llon;
	
	MemMove(&dbb[VLK_DECLSTART+pos], &wpt->name, 6);

	llat = (Int32)Fabs(wpt->lat * 60000.0);
	llon = (Int32)Fabs(wpt->lon * 60000.0);
	dbb[VLK_DECLSTART+pos+6] = (UInt8)((wpt->typ & 0x7f) | ((wpt->lon<0)?0x80:0));

	dbb[VLK_DECLSTART+pos+7] = (UInt8)(((llat >> 16) | ((wpt->lat<0)?0x80:0)));
	llat = llat & 0x0000ffff;
	dbb[VLK_DECLSTART+pos+8] = (UInt8)(llat >> 8);
	dbb[VLK_DECLSTART+pos+9] = (UInt8)(llat & 0x000000ff);

	dbb[VLK_DECLSTART+pos+10] = (UInt8)(llon >> 16);
	llon = llon & 0x0000ffff;
	dbb[VLK_DECLSTART+pos+11] = (UInt8)(llon >> 8);
	dbb[VLK_DECLSTART+pos+12] = (UInt8)(llon & 0x000000ff);
}

Int16 VLKGetDeclarationField(Int16 id) 
{
	Int16 i;
	Int16 ii = -1;
	for(i = VLK_DECLSTART; i < VLK_DECLEND; ) {
		if (dbb[i + 1] == id) {
			// Feld gefunden
			ii = i;
			break;
		}
		if (dbb[i] == 0) {
			// Zyklus verhindern
			ii = -1;
			break;
		}
		i += dbb[i];
	}
	return(ii);
}

void VLKgetwp(Int16 pos, VLKwpt *wpt)
{
	// get a waypoint from the database
	Int32 ll;
	double lat, lon;
	
	MemMove(&wpt->name, &dbb[pos], 6);
	wpt->name[6] = 0;

	wpt->typ = (dbb[pos + 6] & 0x7f);		

	ll = 65536*((Int32)dbb[pos + 7] & 0x7f) + 256*(Int32)dbb[pos + 8] + (Int32)dbb[pos + 9];
	lat = (double)ll / 60000.0;
	if (dbb[pos + 7] & 0x80) {
		lat = -lat;
		ll = -ll;
	}
	wpt->lat = lat;
	wpt->ilat = ll;

	ll = 65536*(Int32)dbb[pos + 10] + 256*(Int32)dbb[pos + 11] + (Int32)dbb[pos + 12];
	lon = (double)ll / 60000.0;
	if (dbb[pos + 6] & 0x80) {
		lon = -lon;
		ll = -ll;
	}
	wpt->lon = lon;
	wpt->ilon = ll;

	return;
}

void VLKgetpilot(Char *pilot, Int16 pltnum)
{
	Int16 pos;

	pos = volks.pltstart + volks.pltlen * pltnum;
	MemMove(pilot, &dbb[pos], volks.pltlen);
	pilot[volks.pltlen] = 0;
}

// read directory of flights stored in logger
Int16 VLKconvdir(CAILogData *logdata, Int16 countonly)
{
	    UInt8 mainType, subType;
	    Int32 idx, dsLength, idx2;
	    Int16 binaryFileVersion = 0;
	    Boolean end;
	    Char pilot1[17], pilot2[17], pilot3[17], pilot4[17];
	    UInt32 i = 0,h,m,s;
	    Int16 numflights = 0, flightofday = 1;
	    Int16 oldDay = 0, oldMonth = 0, oldYear = 0;
	    CAITime tmptime;
	    CAIDate tmpdate;

	    idx = 0;
	    idx2 = 0;
	    dsLength = 0;
	    end = false;

//	    HostTraceOutputTL(appErrorClass, "Analysing Directory");
	    while (!end) {
		mainType = dbb[idx] & rectyp_msk;
		switch (mainType) {
		case rectyp_sep:
//		    HostTraceOutputTL(appErrorClass, "rectyp_sep");
		    // new entry - clear pilot names
		    MemSet(pilot1, sizeof(pilot1), 0);
		    MemSet(pilot2, sizeof(pilot2), 0);
		    MemSet(pilot3, sizeof(pilot3), 0);
		    MemSet(pilot4, sizeof(pilot4), 0);
		    binaryFileVersion = dbb[idx] & ~rectyp_msk;
		    if (binaryFileVersion > max_bfv) {
//   			HostTraceOutputTL(appErrorClass, "binaryFileVersion > max_bfv");
			end = true;
		    }
		    dsLength = 1;
		    break;
		case rectyp_vrb:
		case rectyp_vrt:
//		    HostTraceOutputTL(appErrorClass, "rectyp_vrb/rectyp_vrb");
		    dsLength = dbb[idx + 1];
		    if(!countonly) {
			if (mainType == rectyp_vrb) {
			    idx2 = idx + 2;
			} else {
			    idx2 = idx + 3;
			}
		    	subType = dbb[idx2];
		    	switch (subType) {
				case FLDPLT1:  // Pilotenname einlesen
					MemMove(pilot1, &dbb[idx2 + 1], sizeof(pilot1)-1);
					StrCopy(pilot1, trim(pilot1, ' ', true));
					break;
				case FLDPLT2:  // Pilotenname einlesen
					MemMove(pilot2, &dbb[idx2 + 1], sizeof(pilot2)-1);
					StrCopy(pilot2, trim(pilot2, ' ', true));
					break;
				case FLDPLT3:  // Pilotenname einlesen
					MemMove(pilot3, &dbb[idx2 + 1], sizeof(pilot3)-1);
					StrCopy(pilot3, trim(pilot3, ' ', true));
					break;
				case FLDPLT4:  // Pilotenname einlesen
					MemMove(pilot4, &dbb[idx2 + 1], sizeof(pilot4)-1);
					StrCopy(pilot4, trim(pilot4, ' ', true));
					break;
				default: // do nothing
					break;
			}
		    };
		    break;
		case rectyp_fil:
//		    HostTraceOutputTL(appErrorClass, "rectyp_fil");
		    dsLength = 1;
		    break;
		case rectyp_pos:
//		    HostTraceOutputTL(appErrorClass, "rectyp_pos");
		    dsLength = pos_ds_size[binaryFileVersion][0];
		    break;
		case rectyp_poc:
//		    HostTraceOutputTL(appErrorClass, "rectyp_poc");
		    if((dbb[idx + 2] & 0x80) != 0) { // Endebedingung
			end = true;
		    }
		    dsLength = pos_ds_size[binaryFileVersion][1];
		    break;
		case rectyp_tnd:
//		    HostTraceOutputTL(appErrorClass, "rectyp_tnd");
		    dsLength = 8;
		    if(!countonly) {
		    	i = (65536L * dbb[idx + 2]) + (256L * dbb[idx + 3]) + dbb[idx + 4];
		    	h = i / 3600L;
		    	m = (i - (h * 3600L)) / 60;
		    	s = i - (h * 3600L) - (m * 60);
		    	tmptime.hour = h;
		    	tmptime.min = m;
		    	tmptime.sec = s;
		    	tmpdate.day = 10*(dbb[idx + 7] >> 4) + (dbb[idx + 7] & 0x0f);
	   	    	tmpdate.month = 10*(dbb[idx + 6] >> 4) + (dbb[idx + 6] & 0x0f);
		    	tmpdate.year = 10*(dbb[idx + 5] >> 4) + (dbb[idx + 5] & 0x0f);
		    	//if (tmpdate.year < 80) {
			//	tmpdate.year += 2000;
			//} else {
			//	tmpdate.year += 1900;
			//}
		    }
		    break;
		case rectyp_end:
//		    HostTraceOutputTL(appErrorClass, "rectyp_end");
		    dsLength = 7;
		    if(!countonly) {
			if (oldDay == tmpdate.day && oldMonth == tmpdate.month && oldYear == tmpdate.year) {
				flightofday++;
			} else {
				flightofday = 1;
				oldDay = tmpdate.day;
				oldMonth = tmpdate.month;
				oldYear = tmpdate.year;
			}
		    	StrCopy(logdata[numflights].pilotName, pilot1);
		    	 StrCat(logdata[numflights].pilotName, pilot2);
		    	 StrCat(logdata[numflights].pilotName, pilot3);
		    	 StrCat(logdata[numflights].pilotName, pilot4);
			logdata[numflights].startTime = tmptime;
			logdata[numflights].startDate = tmpdate;
			logdata[numflights].FlightOfDay = flightofday;
		    }
		    numflights++;
		    break;
		default:
//		    HostTraceOutputTL(appErrorClass, "hmm?");
		    // hmm something went wrong
		    numflights = 0;
		    end = true;
		    break;
		}

		idx += dsLength;
	    }
//	HostTraceOutputTL(appErrorClass, "Return Number of Flights");
	return(numflights);
}

//********************************************************
// Functions used to convert binary flight log to IGC file
//********************************************************

Int16 EnlFlt(Int16 enl) 
{
// calculate engine noise level

	if (enl < 500) {
		enl /= 2;
	} else if (enl < 750) {
		enl = 250 + 2 * (enl - 500);
	} else {
		enl = (int)(750 + (enl - 750) * 1.5);
	}
	if (enl>999) enl = 999;

	return(enl);
}

Int16 PressureToAltitude(Int16 druck) 
{
//	genaue Umrechnung von Druckwert nach H”he.
//	Druckwert ist ein innerhalb des Loggers temperaturkompensierter Wert, der
//	proportional zum gemessenen Umgebungsdruck am Logger ist.
//	1100 mbar entsprechen einem Druckwert von 4096;

	double GMR = 9.80665 * 28.9644 / 8314.32;
	double tgrad = -6.5E-3;
	double p0 = 1013.25;
	double p11 = 0.2233611050922 * p0;
	double t0 = 288.15;
	double t11 = 216.65;
	double p;
	double hoehe;

	// Umrechnung von normierten ADC-Werten in hPa
	p = 1100.0 * druck / 4096;
	// Berechnen der Druckh”he anhand des Druckes
	if (p > p11) {
		hoehe = (t0 * (exp((tgrad / GMR) * log(p0 / p)) - 1) / tgrad);
	} else {
		hoehe = (t11 * log(p11 / p) / GMR + 11000);
	}

	return((Int16)hoehe);
}

Int16 HdopToFxa(UInt8 hdop) 
{
	return((Int16)((double)hdop * 100.01 / 3));
}

void UnpackTaskWP(Int32 pos, VLKwpt *wpt) 
{
// get a waypoint from the binary flight log

	Int32 ll;
	double lat, lon;
	Int16 i;
	
	// name
	for (i = 0; i < 6; i++) {
		wpt->name[i] = readbuffer(pos+i);
	}
	wpt->name[6] = 0;

	// type
	wpt->typ = (readbuffer(pos + 6) & 0x7f);		

	// latitude
	ll = 65536*((Int32)readbuffer(pos + 7) & 0x7f) + 256*(Int32)readbuffer(pos + 8) + (Int32)readbuffer(pos + 9);
	if (ll > 5400000) ll = 5400000;
	lat = (double)ll / 60000.0;
	if (readbuffer(pos + 7) & 0x80) {
		lat = -lat;
		ll = -ll;
	}
	wpt->lat = lat;
	wpt->ilat = ll;

	// longitude
	ll = 65536*(Int32)readbuffer(pos + 10) + 256*(Int32)readbuffer(pos + 11) + (Int32)readbuffer(pos + 12);
	if (ll > 10800000) ll = 10800000;
	lon = (double)ll / 60000.0;
	if (readbuffer(pos + 6) & 0x80) {
		lon = -lon;
		ll = -ll;
	}
	wpt->lon = lon;
	wpt->ilon = ll;

	return;
}

void igc_filter(char *st) 
{
// filter IGC chars and output line
 
	static Char* alphabet = " \"#%&\'()+-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]_\140abcdefghijklmnopqrstuvwxyz{|}";
	Int16 alphabet_l = StrLen(alphabet);
	Int16 l = StrLen(st);
	Int16 i,j;
	Boolean found;

	if (l > 0) {
		for (i = 0; i < l; i++) {
			found = false;
			for (j = 0; j < alphabet_l; j++) {
				if (st[i] == alphabet[j]) {
					found = true;
				}
			}
			if (!found) {
				st[i] = ' ';
			}
		}
		StrCopy(output_char, trim(st, ' ', true));
		StrCatEOL(st, data.config.xfertype);
		TxData(st, data.config.xfertype);
	}

	return;
}

void OutputTaskWP(Int16 i) 
{
// output a Task waypoint to the IGC file

	Char tempchar[5];

	StrCopy(output_char, "C");
	StrCat(output_char, leftpad(DblToStr(Fabs(wpttmp[i].ilat / 60000),0), '0', 2));
	StrCat(output_char, leftpad(StrIToA(tempchar, Fabs(wpttmp[i].ilat % 60000)), '0', 5));
	StrCat(output_char, (wpttmp[i].lat < 0 ? "S" : "N"));
	StrCat(output_char, leftpad(DblToStr(Fabs(wpttmp[i].ilon / 60000),0), '0', 3));
	StrCat(output_char, leftpad(StrIToA(tempchar, Fabs(wpttmp[i].ilon % 60000)), '0', 5));
	StrCat(output_char, (wpttmp[i].lon < 0 ? "W" : "E"));
	StrCat(output_char, wpttmp[i].name);
	igc_filter(output_char);	
}


Char* ToBase64(UInt8 *b) 
{
// base64 conversion for G record

	Char *base64tab = "0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ`abcdefghijklmnopqrstuvwxyz";
	static Char bas64ar[5];

	bas64ar[0] = base64tab[   b[0] >> 2			   ];
	bas64ar[1] = base64tab[ ((b[0] & 0x03) << 4) | (b[1] >> 4)    ];
	bas64ar[2] = base64tab[ ((b[1] & 0x0f) << 2) | (b[2] >> 6)    ];
	bas64ar[3] = base64tab[   b[2] & 0x3f			 ];
	bas64ar[4] = 0;

  	return(bas64ar);
}

Boolean VLKconvIGC()
{
// convert binary log file into text IGC format

	UInt8 mainType, subType;
	Int32 idx = 0, dsLength = 0, idx2 = 0;
	Int16 binaryFileVersion = 0;
	Boolean end = false;
	Char pilot1[17], pilot2[17], pilot3[17], pilot4[17];
	VLKIGCheader igc;
	DateTimeType firstFix, realTime, declTime;
	Int32 signaturePos = 0;
	Int32 timeRelative = 0;
	Int32 declarationTime = -1;
	Int16 nrTurnpoints = 0;
	Int16 taskId = 0;
	Int32 i;
	Int32 h, m, s;
	Int32 lon = 0, tmpLon = 0;
	Boolean tzSet = false;
	double flightTimeZone = 0.0;
	Int16 timeZone = 4000; //check to see if set
	Boolean validFix = true;
	Char tempchar[80];
	UInt16 serialNo;

	// additional variables for section 2
	Int32 diffLat, diffLon;
	Int16 pressure = 0;
	Int16 gpsAlt = 0;
	Int16 enl = 0;
	Int16 pAlt = 0;
	Int16 fxa = 0;
	Int32 lat = 0;
	double hwversion = 0.0;

	// variables for G record conversion
	UInt8 tmp[3];
	Int16 triCnt;
	Int16 blockCnt;
	Char gRecord[82];

	// nr of turnpoints + 4 (start, takeoff, finish, landing)
	MemSet(&wpttmp, sizeof(wpttmp), 0);

	// clear pilot names
	MemSet(pilot1, sizeof(pilot1), 0);
	MemSet(pilot2, sizeof(pilot2), 0);
	MemSet(pilot3, sizeof(pilot3), 0);
	MemSet(pilot4, sizeof(pilot4), 0);

	// clear igc header
	MemSet(&igc, sizeof(igc), 0);

	//clear dates
	MemSet(&firstFix, sizeof(firstFix), 0);
	MemSet(&realTime, sizeof(realTime), 0);
	MemSet(&declTime, sizeof(declTime), 0);

//	HostTraceOutputTL(appErrorClass, "IGC decode START");
	    while (!end) {
		mainType = readbuffer(idx) & rectyp_msk;
		switch (mainType) {
		case rectyp_sep:
//		    HostTraceOutputTL(appErrorClass, "rectyp_sep");
		    // new entry
		    timeRelative = 0;
		    binaryFileVersion = readbuffer(idx) & ~rectyp_msk;
		    if (binaryFileVersion > max_bfv) {
			// unsupported binary file version
//			HostTraceOutputTL(appErrorClass, "1:Version Error");
			return(false);
		    }
		    dsLength = 1;
		    break;
		case rectyp_end:		  
//		    HostTraceOutputTL(appErrorClass, "rectyp_end");
		    dsLength = 41;
		    StrCat(pilot1, pilot2);
		    StrCat(pilot1, pilot3);
		    StrCat(pilot1, pilot4);
		    end = true;
		    break;
		case rectyp_fil:
//		    HostTraceOutputTL(appErrorClass, "rectyp_fil");
		    dsLength = 1;
		    break;
		case rectyp_tnd:
//		    HostTraceOutputTL(appErrorClass, "rectyp_tnd");
		    // errechnet rueckwaerts die Zeit des 1. Fixes
		    timeRelative += readbuffer(idx + 1);
		    i = (65536 * (Int32)readbuffer(idx + 2)) + (256 * (Int32)readbuffer(idx + 3)) + (Int32)readbuffer(idx + 4);
		    h = i / 3600;
		    m = (i % 3600) / 60;
		    s = i % 60;
		    firstFix.hour = h;
		    firstFix.minute = m;
		    firstFix.second = s;
		    firstFix.day =   10 * (readbuffer(idx + 7) >> 4) + (readbuffer(idx + 7) & 0x0f);
		    firstFix.month = 10 * (readbuffer(idx + 6) >> 4) + (readbuffer(idx + 6) & 0x0f);
		    firstFix.year =  10 * (readbuffer(idx + 5) >> 4) + (readbuffer(idx + 5) & 0x0f);
		    if (firstFix.year < 80) {
		    	firstFix.year += 2000;
		    } else {
		    	firstFix.year += 1900;
		    }
		    TimAdjust(&firstFix, -timeRelative);
		    dsLength = 8;
		    break;
		case rectyp_pos:
		case rectyp_poc:
//		    HostTraceOutputTL(appErrorClass, "rectyp_pos/poc");
		    if((readbuffer(idx + 2) & 0x80) != 0) { // Endebedingung
			end = true;
			dsLength = 0;
		    } else {
			timeRelative += readbuffer(idx + 2);
			validFix = (((readbuffer(idx) & 0x10) >> 4) != 0 ? true : false);
			if (mainType == rectyp_pos) {
			    dsLength = pos_ds_size[binaryFileVersion][0];
			    lon = ((Int32)readbuffer(idx + 6) << 16) | ((Int32)readbuffer(idx + 7) << 8) | (Int32)readbuffer(idx + 8);
			    if (readbuffer(idx + 9) & 0x80) {
				lon = -lon;
			    }
			} else {
			    dsLength = pos_ds_size[binaryFileVersion][1];
			    tmpLon = (((Int32)readbuffer(idx + 3) & 0x78) << 5) | (Int32)readbuffer(idx + 5);
			    if (readbuffer(idx + 6) & 0x80) {
				tmpLon = -tmpLon;
			    }
			    lon += tmpLon;
			}

			// ftz mit L„ngengrad fllen
			// der erste gltige ist der letzte,
			// der in ftz gespeichert wird
			if (!tzSet) {
			    	flightTimeZone = (double)lon;
				if (validFix) tzSet = true;
			}
		    }
		    break;
		case rectyp_vrt:
		case rectyp_vrb:
//		    HostTraceOutputTL(appErrorClass, "rectyp_vrt/vrb");
		    dsLength = readbuffer(idx + 1);
		    switch (mainType) {
		    case rectyp_vrb:
			idx2 = idx + 2;
			break;
		    case rectyp_vrt:
			timeRelative += readbuffer(idx + 2);
			idx2 = idx + 3;
			break;
		    default:
			idx2 = idx;
			break;
		    }
		    subType = readbuffer(idx2);
		    switch (subType) {
		    case FLDNTP:
//       		  HostTraceOutputTL(appErrorClass, "NTP");
			// we ignore number of turnpoints and read it on demand
			nrTurnpoints = readbuffer(idx2 + 1);
			declarationTime = timeRelative;
			break;
		    case FLDTID:
//       		  HostTraceOutputTL(appErrorClass, "TID");
			taskId = (readbuffer(idx2 + 1) * 256) + readbuffer(idx2 + 2);
			declarationTime = timeRelative;
			break;
		    case FLDFDT:
//       		  HostTraceOutputTL(appErrorClass, "FDT");
			// we ignore this because conv version is >= 422
			break;
		    case FLDTZN:  // Zeitzonenoffset einlesen
//       		  HostTraceOutputTL(appErrorClass, "TZN");
			if (readbuffer(idx2 + 1) < 128) {
			    timeZone = 15 * readbuffer(idx2 + 1);
			} else {
			    timeZone = 15 * (readbuffer(idx2 + 1) - 256);
			}
			break;
		    case FLDTKF:
//       		  HostTraceOutputTL(appErrorClass, "TKF");
			UnpackTaskWP(idx2 + 1, &wpttmp[0]);
			break;
		    case FLDSTA:
//       		  HostTraceOutputTL(appErrorClass, "STA");
			UnpackTaskWP(idx2 + 1, &wpttmp[1]);
			break;
		    case FLDTP1:
//       		  HostTraceOutputTL(appErrorClass, "TP1");
			UnpackTaskWP(idx2 + 1, &wpttmp[2]);
			break;
		    case FLDTP2:
//       		  HostTraceOutputTL(appErrorClass, "TP2");
			UnpackTaskWP(idx2 + 1, &wpttmp[3]);
			break;
		    case FLDTP3:
//       		  HostTraceOutputTL(appErrorClass, "TP3");
			UnpackTaskWP(idx2 + 1, &wpttmp[4]);
			break;
		    case FLDTP4:
//       		  HostTraceOutputTL(appErrorClass, "TP4");
			UnpackTaskWP(idx2 + 1, &wpttmp[5]);
			break;
		    case FLDTP5:
//       		  HostTraceOutputTL(appErrorClass, "TP5");
			UnpackTaskWP(idx2 + 1, &wpttmp[6]);
			break;
		    case FLDTP6:
//       		  HostTraceOutputTL(appErrorClass, "TP6");
			UnpackTaskWP(idx2 + 1, &wpttmp[7]);
			break;
		    case FLDTP7:
//       		  HostTraceOutputTL(appErrorClass, "TP7");
			UnpackTaskWP(idx2 + 1, &wpttmp[8]);
			break;
		    case FLDTP8:
//       		  HostTraceOutputTL(appErrorClass, "TP8");
			UnpackTaskWP(idx2 + 1, &wpttmp[9]);
			break;
		    case FLDTP9:
//       		  HostTraceOutputTL(appErrorClass, "TP9");
			UnpackTaskWP(idx2 + 1, &wpttmp[10]);
			break;
		    case FLDTP10:
//       		  HostTraceOutputTL(appErrorClass, "TP10");
			UnpackTaskWP(idx2 + 1, &wpttmp[11]);
			break;
		    case FLDTP11:
//       		  HostTraceOutputTL(appErrorClass, "TP11");
			UnpackTaskWP(idx2 + 1, &wpttmp[12]);
			break;
		    case FLDTP12:
//       		  HostTraceOutputTL(appErrorClass, "TP12");
			UnpackTaskWP(idx2 + 1, &wpttmp[13]);
			break;
		    case FLDFIN:
//       		  HostTraceOutputTL(appErrorClass, "FIN");
			UnpackTaskWP(idx2 + 1, &wpttmp[14]);
			break;
		    case FLDLDG:
//       		  HostTraceOutputTL(appErrorClass, "LDG");
			UnpackTaskWP(idx2 + 1, &wpttmp[15]);
			break;
		    case FLDPLT1:  // Pilotenname einlesen
//       		  HostTraceOutputTL(appErrorClass, "PLT1");
			MemSet(tempchar, sizeof(tempchar), 0);
			for (i = 0; i < sizeof(pilot1)-1; i++) {
				tempchar[i] = readbuffer(idx2 + 1 + i);
			}
			StrCopy(pilot1, trim(tempchar, ' ', true));
			break;
		    case FLDPLT2:  // Pilotenname einlesen
//       		  HostTraceOutputTL(appErrorClass, "PLT2");
			MemSet(tempchar, sizeof(tempchar), 0);
			for (i = 0; i < sizeof(pilot2)-1; i++) {
				tempchar[i] = readbuffer(idx2 + 1 + i);
			}
			StrCopy(pilot2, trim(tempchar, ' ', true));
			break;
		    case FLDPLT3:  // Pilotenname einlesen
//       		  HostTraceOutputTL(appErrorClass, "PLT3");
			MemSet(tempchar, sizeof(tempchar), 0);
			for (i = 0; i < sizeof(pilot3)-1; i++) {
				tempchar[i] = readbuffer(idx2 + 1 + i);
			}
			StrCopy(pilot3, trim(tempchar, ' ', true));
			break;
		    case FLDPLT4:  // Pilotenname einlesen
//       		  HostTraceOutputTL(appErrorClass, "PLT4");
			MemSet(tempchar, sizeof(tempchar), 0);
			for (i = 0; i < sizeof(pilot4)-1; i++) {
				tempchar[i] = readbuffer(idx2 + 1 + i);
			}
			StrCopy(pilot4, trim(tempchar, ' ', true));
			break;
		    case FLDGTY:  // Flugzeugtyp einlesen
//       		  HostTraceOutputTL(appErrorClass, "GTY");
			MemSet(tempchar, sizeof(tempchar), 0);
			for (i = 0; i < sizeof(igc.GTY)-1; i++) {
				tempchar[i] = readbuffer(idx2 + 1 + i);
			}
			if (StrLen(tempchar) > 0) {
			    StrCopy(igc.GTY, "HFGTYGLIDERTYPE:");
			    StrCat(igc.GTY, trim(tempchar, ' ', true));
			} else {
    			    StrCopy(igc.GTY, "HOGTYGLIDERTYPE:");
			}
			break;
		    case FLDGID: // Flugzeugkennzeichen einlesen
//       		  HostTraceOutputTL(appErrorClass, "GID");
			MemSet(tempchar, sizeof(tempchar), 0);
			for (i = 0; i < sizeof(igc.GID)-1; i++) {
				tempchar[i] = readbuffer(idx2 + 1 + i);
			}
			if (StrLen(tempchar) > 0) {
			    StrCopy(igc.GID, "HFGIDGLIDERID:");
			    StrCat(igc.GID, trim(tempchar, ' ', true));
			} else {
			    StrCopy(igc.GID, "HOGIDGLIDERID:");
			}
			break;
		    case FLDCCL:  // Wettbewerbsklasse einlesen
//       		  HostTraceOutputTL(appErrorClass, "CCL");
			MemSet(tempchar, sizeof(tempchar), 0);
			for (i = 0; i < sizeof(igc.CCL)-1; i++) {
				tempchar[i] = readbuffer(idx2 + 1 + i);
			}
			if (StrLen(tempchar) > 0) {
				StrCopy(igc.CCL, "HFCCLCOMPETITIONCLASS:");
				StrCat(igc.CCL, trim(tempchar, ' ', true));
			}
			break;
		    case FLDCID:  // Wettbewerbskennzeichen einlesen
//       		  HostTraceOutputTL(appErrorClass, "CID");
			MemSet(tempchar, sizeof(tempchar), 0);
			for (i = 0; i < sizeof(igc.CID)-1; i++) {
				tempchar[i] = readbuffer(idx2 + 1 + i);
			}
			if (StrLen(tempchar) > 0) {	
				StrCopy(igc.CID, "HFCIDCOMPETITIONID:");
				StrCat(igc.CID, trim(tempchar, ' ', true));
			}
			break;
		    case FLDHDR:  // igc file header
//       		  HostTraceOutputTL(appErrorClass, "HDR");
			serialNo = 256*readbuffer(idx2 + 1) + readbuffer(idx2 + 2);
			volks.serialno[0] = c36[(Int16)(serialNo / 36 / 36)];
			volks.serialno[1] = c36[((Int16)(serialNo / 36) % 36)];
			volks.serialno[2] = c36[(Int16)(serialNo % 36)];
			volks.serialno[3] = '\0';

			StrCopy(igc.A, MFR_ID2);
			StrCat(igc.A, MFR_ID);
			StrCat(igc.A, volks.serialno);

			StrCopy(igc.DTM, "HFDTM");
			StrCat(igc.DTM, leftpad(StrIToA(tempchar, readbuffer(idx2 + 3)), '0', 3));
			StrCat(igc.DTM, "GPSDATUM:WGS84");

			StrCopy(igc.RHW, "HFRHWHARDWAREVERSION:");
			hwversion = (readbuffer(idx2 + 4) >> 4) + (double)(readbuffer(idx2 + 4) & 0xf)/10;
			StrCat(igc.RHW, DblToStr(hwversion,1));

			StrCopy(igc.RFW, "HFRFWFIRMWAREVERSION:");
			StrCat(igc.RFW, StrIToA(tempchar, readbuffer(idx2 + 5) >> 4));
			StrCat(igc.RFW, ".");
			StrCat(igc.RFW, StrIToA(tempchar, readbuffer(idx2 + 5) & 0xf));

			StrCopy(igc.FXA, "HFFXA");
			StrCat(igc.FXA, leftpad(StrIToA(tempchar, readbuffer(idx2 + 7)), '0', 3));

			StrCopy(igc.FTY, "HFFTYFR TYPE:GARRECHT INGENIEURGESELLSCHAFT,VOLKSLOGGER 1.0");

			break;
		    }
		    break;
		default:
		    dsLength = 1;
		    end = true;
		    break;
		}
		idx += dsLength;
	    }
	    signaturePos = idx;

//	    	HostTraceOutputTL(appErrorClass, "Section 1 done");

	    	flightTimeZone = flightTimeZone + 450000;
	    	flightTimeZone = flightTimeZone / 900000;
	
	    	if (timeZone != 4000) {
			StrCopy(igc.TZN, "UTC");
			StrCat(igc.TZN, (timeZone < 0 ? "-" : "+"));
			StrCat(igc.TZN, leftpad(StrIToA(tempchar, (Int16)Fabs(timeZone) / 60), '0', 2));
			StrCat(igc.TZN, leftpad(StrIToA(tempchar, (Int16)Fabs(timeZone) % 60), '0', 2));
	    	}

	    	// flight date
  	    	StrCopy(igc.DTE, "HFDTE");
	    	StrCat(igc.DTE, leftpad(StrIToA(tempchar, firstFix.day), '0', 2));
	    	StrCat(igc.DTE, leftpad(StrIToA(tempchar, firstFix.month), '0', 2));
	    	StrCat(igc.DTE, leftpad(StrIToA(tempchar, firstFix.year % 100), '0', 2));

		// output IGC header
//	    	HostTraceOutputTL(appErrorClass, "Output IGC header");
    		igc_filter(igc.A);
    		igc_filter(igc.DTE);
    		igc_filter(igc.FXA);
    		if (StrLen(pilot1) > 0) {
      			StrCopy(igc.PLT, "HFPLTPILOT:");
      			StrCat(igc.PLT, pilot1);
      			igc_filter(igc.PLT);
    		} else {
			StrCopy(igc.PLT, "HOPLTPILOT:");
      			igc_filter(igc.PLT);
    		}
		igc_filter(igc.GTY);
		igc_filter(igc.GID);
    		igc_filter(igc.DTM);
    		igc_filter(igc.RFW);
    		igc_filter(igc.RHW);
		// don't use IGC filter as this line includes a comma!
		StrCatEOL(igc.FTY, data.config.xfertype);
  		TxData(igc.FTY, data.config.xfertype);
    		igc_filter(igc.CID);
    		igc_filter(igc.CCL);
    		igc_filter(igc.TZN);
    		if (hwversion < 3.3) {
			StrCopy(igc.FXA,"I013638FXA");
    		} else {
			StrCopy(igc.FXA,"I023638FXA3941ENL");
    		}
		igc_filter(igc.FXA);
		StrCopy(tempchar, "LCONV-VER:4.24");
		igc_filter(tempchar);

	    	// declaration time
    		declTime = firstFix;
		TimAdjust(&declTime, declarationTime);

//	    	HostTraceOutputTL(appErrorClass, "Output Task");
		// output task declaration
		if (taskId > 9999) taskId = 9999;
		if (nrTurnpoints > 12) nrTurnpoints = 12;
		StrCopy(output_char, "C");
	    	StrCat(output_char, leftpad(StrIToA(tempchar, declTime.day), '0', 2));
	    	StrCat(output_char, leftpad(StrIToA(tempchar, declTime.month), '0', 2));
	    	StrCat(output_char, leftpad(StrIToA(tempchar, declTime.year % 100), '0', 2));
	    	StrCat(output_char, leftpad(StrIToA(tempchar, declTime.hour), '0', 2));
	    	StrCat(output_char, leftpad(StrIToA(tempchar, declTime.minute), '0', 2));
	    	StrCat(output_char, leftpad(StrIToA(tempchar, declTime.second), '0', 2));		
		StrCat(output_char, "000000"); // FDT always zero in version 4.24
		StrCat(output_char, leftpad(StrIToA(tempchar, taskId), '0', 4));			
		StrCat(output_char, leftpad(StrIToA(tempchar, nrTurnpoints), '0', 2));			
		igc_filter(output_char);
		// output task waypoints
		OutputTaskWP(0);
		OutputTaskWP(1);
		for (i = 0; i < nrTurnpoints; i++) {
			OutputTaskWP(i+2);
		}
		OutputTaskWP(14);
		OutputTaskWP(15);

//	    HostTraceOutputTL(appErrorClass, "Section 2 START");
	    end = false;
	    idx = 0;
	    realTime = firstFix;

	    while (!end) {
		mainType = (readbuffer(idx) & rectyp_msk);
		switch (mainType) {
		case rectyp_sep:
//		    HostTraceOutputTL(appErrorClass, "rectyp_sep");
		    dsLength = 1;
		    if (binaryFileVersion > max_bfv) {
			// unsupported binary file version
//			HostTraceOutputTL(appErrorClass, "2:Version Error");
			dsLength = 0;
			end = true;
			break;
		    }
		    break;
		case rectyp_fil:
//		    HostTraceOutputTL(appErrorClass, "rectyp_fil");
		    dsLength = 1;
		    break;
		case rectyp_pos:
		case rectyp_poc:
//		    HostTraceOutputTL(appErrorClass, "rectyp_pos/poc");
		    if((readbuffer(idx + 2) & 0x80) != 0) { // Endebedingung
			end = true;
		    } else {
			timeRelative += readbuffer(idx + 2);
			TimAdjust(&realTime, readbuffer(idx + 2));
			validFix = ((readbuffer(idx) & 0x10) >> 4) != 0 ? true : false;
			pressure = ((readbuffer(idx) & 0x0f) << 8) | readbuffer(idx + 1);
			if (mainType == rectyp_pos) {
			    dsLength = pos_ds_size[binaryFileVersion][0];
			    lat = ((Int32)readbuffer(idx + 3) & 0x7f) << 16 | ((Int32)readbuffer(idx + 4) << 8) | (Int32)readbuffer(idx + 5);
			    if (readbuffer(idx + 3) & 0x80) {
				lat = -lat;
			    }
			    lon = ((Int32)readbuffer(idx + 6) << 16) | ((Int32)readbuffer(idx + 7) << 8) | (Int32)readbuffer(idx + 8);
			    if (readbuffer(idx + 9) & 0x80) {
				lon = -lon;
			    }
			    gpsAlt = ((readbuffer(idx + 9) & 0x70) << 4) | readbuffer(idx + 10);
			    fxa = HdopToFxa(readbuffer(idx + 9) & 0x0f);
			    enl = 4 * readbuffer(idx + 11);
			} else {
			    dsLength = pos_ds_size[binaryFileVersion][1];
			    diffLat = (((Int32)readbuffer(idx + 3) & 0x07) << 8) | (Int32)readbuffer(idx + 4);
			    if (readbuffer(idx + 3) & 0x80) {
				diffLat = -diffLat;
			    }
			    diffLon = (((Int32)readbuffer(idx + 3) & 0x78) << 5) | (Int32)readbuffer(idx + 5);
			    if (readbuffer(idx + 6) & 0x80) {
				diffLon = -diffLon;
			    }
			    lat += diffLat;
			    lon += diffLon;
			    gpsAlt = ((readbuffer(idx + 6) & 0x70) << 4) | readbuffer(idx + 7);
			    fxa = HdopToFxa(readbuffer(idx + 6) & 0x0f);
			    enl = 4 * readbuffer(idx + 8);
			}
		     	gpsAlt = 10 * gpsAlt - 1000;
			enl = EnlFlt(enl);
			pAlt = PressureToAltitude(pressure);

			// output B record
//			HostTraceOutputTL(appErrorClass, "Output B record");
			StrCopy(output_char, "B");
			StrCat(output_char, leftpad(StrIToA(tempchar, realTime.hour), '0', 2));
			StrCat(output_char, leftpad(StrIToA(tempchar, realTime.minute), '0', 2));
			StrCat(output_char, leftpad(StrIToA(tempchar, realTime.second), '0', 2));
			StrCat(output_char, leftpad(DblToStr(Fabs(lat / 60000),0), '0', 2));
			StrCat(output_char, leftpad(StrIToA(tempchar, Fabs((Int32)lat % 60000)), '0', 5));
			StrCat(output_char, (lat < 0 ? "S" : "N"));
			StrCat(output_char, leftpad(DblToStr(Fabs(lon / 60000),0), '0', 3));
			StrCat(output_char, leftpad(StrIToA(tempchar, Fabs((Int32)lon % 60000)), '0', 5));
			StrCat(output_char, (lon < 0 ? "W" : "E"));
			StrCat(output_char, (validFix ? "A" : "V"));
			if (pAlt < 0) {
				StrCat(output_char, "-");
				StrCat(output_char, leftpad(StrIToA(tempchar, -pAlt), '0', 4));
			} else {	
				StrCat(output_char, leftpad(StrIToA(tempchar, pAlt), '0', 5));
			}
			if (gpsAlt < 0) {
				StrCat(output_char, "-");
				StrCat(output_char, leftpad(StrIToA(tempchar, -gpsAlt), '0', 4));
			} else {
				StrCat(output_char, leftpad(StrIToA(tempchar, gpsAlt), '0', 5));
			}
			StrCat(output_char, leftpad(StrIToA(tempchar, fxa), '0', 3));
			if (hwversion >= 3.3) StrCat(output_char, leftpad(StrIToA(tempchar, enl), '0', 3));
			igc_filter(output_char);
		    }
		    break;
		case rectyp_vrt:
		case rectyp_vrb:
//		    HostTraceOutputTL(appErrorClass, "rectyp_vrt/vrb");
		    dsLength = readbuffer(idx + 1);
		    switch (mainType) {
		    case rectyp_vrb:
			idx2 = idx + 2;
			break;
		    case rectyp_vrt:
			TimAdjust(&realTime, readbuffer(idx+2));
			idx2 = idx + 3;
			break;
		    default:
			idx2 = idx;
			break;
		    }
		    subType = readbuffer(idx2);
		    switch (subType) {
		    case FLDEPEV:
//       			HostTraceOutputTL(appErrorClass, "PEV");
			StrCopy(output_char, "E");
			StrCat(output_char, leftpad(StrIToA(tempchar, realTime.hour), '0', 2));
			StrCat(output_char, leftpad(StrIToA(tempchar, realTime.minute), '0', 2));
			StrCat(output_char, leftpad(StrIToA(tempchar, realTime.second), '0', 2));
			StrCat(output_char, "PEVEVENTBUTTON PRESSED");
	  		StrCatEOL(output_char, data.config.xfertype);
  			TxData(output_char, data.config.xfertype);
			break;
		    case FLDETKF:
//       			HostTraceOutputTL(appErrorClass, "TKF");
			StrCopy(output_char, "LGCSTKF");
			StrCat(output_char, leftpad(StrIToA(tempchar, realTime.hour), '0', 2));
			StrCat(output_char, leftpad(StrIToA(tempchar, realTime.minute), '0', 2));
			StrCat(output_char, leftpad(StrIToA(tempchar, realTime.second), '0', 2));
			StrCat(output_char, "TAKEOFF DETECTED");
	  		StrCatEOL(output_char, data.config.xfertype);
  			TxData(output_char, data.config.xfertype);
			break;
		    }
		    break;
		case rectyp_tnd:
//       			HostTraceOutputTL(appErrorClass, "TND");
		    dsLength = 8;
		    TimAdjust(&realTime, readbuffer(idx+1));
		    break;
		default:
		    end = true;
		    break;
		}
		idx += dsLength;
	    }

	    // create g-record
//	    HostTraceOutputTL(appErrorClass, "Create G Record");
	    MemSet(gRecord, sizeof(gRecord), 0);
	    triCnt = 0;
	    blockCnt = 0;
	    tmp[0] = 0xff;
	    tmp[1] = 0xff;
	    tmp[2] = 0xff;

	    for (idx = 0; idx < signaturePos; idx++) {
		tmp[triCnt++] = readbuffer(idx);
		if (triCnt == 3) {
		    triCnt = 0;
		    StrCat(gRecord, ToBase64(tmp));
		    tmp[0] = 0xff;
		    tmp[1] = 0xff;
		    tmp[2] = 0xff;
		    blockCnt++;
		    if (blockCnt == 18) {
			blockCnt = 0;
			StrCopy(output_char, "G");
			StrCat(output_char, gRecord);
			StrCatEOL(output_char, data.config.xfertype);
  			TxData(output_char, data.config.xfertype);
			MemSet(gRecord, sizeof(gRecord), 0);
		    }
		}
	    }

	    if (triCnt > 0 || blockCnt > 0) {
		StrCat(gRecord, ToBase64(tmp));
		StrCopy(output_char, "G");
		StrCat(output_char, gRecord);
		StrCatEOL(output_char, data.config.xfertype);
		TxData(output_char, data.config.xfertype);
	    }

//	HostTraceOutputTL(appErrorClass, "IGC decode END");
	return(true);
}
