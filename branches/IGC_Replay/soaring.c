// $Id: soaring.c,v 1.160 2008/01/16 01:32:08 Administrator Exp $
// $Name:  $

/**
* \file soaring.c
* \brief this is the main SoarPilot module where it all starts and ends
*/

/**
* \mainpage
* This is the source code documentation for SoarPilot, a Palm powered flight computer.
* - Main site: http://www.soaringpilot.org
* - Wiki: http://www.soaringpilot.org/dokuwiki/
* - Yahoo user group: http://tech.groups.yahoo.com/group/soaringpilot/
*
* This documentation is intended for SoarPilot developers. Developers are
* expected to use the SVN repository as described in the wiki.
*/

#include <PalmOS.h>	// all the system toolbox headers
#include <SerialMgr.h>
#include <MemoryMgr.h>
#include <KeyMgr.h>
#include <SysEvtMgr.h>
#include <VFSMgr.h>
#include <DLServer.h>
#include <BtLibTypes.h>
#include <HsChars.h>
#include <SysUtils.h>
#include "AddIncs/SonyIncs/SonyHwrOEMIDs.h"
#include "AddIncs/GarminIncs/GarminChars.h"
#include "AddIncs/GarminIncs/GPSLib.h"
#include "Mathlib.h"
#include "soaring.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarDB.h"
#include "soarMap.h"
#include "soarUMap.h"
#include "soarLog.h"
#include "soarNMEA.h"
#include "soarIO.h"
#include "soarWay.h"
#include "soarWind.h"
#include "soarWLst.h"
#include "soarTask.h"
#include "soarPLst.h"
#include "soarComp.h"
#include "soarCAI.h"
#include "soarMath.h"
#include "soarSTF.h"
#include "soarSHA.h"
#include "soarTer.h"
#include "soarMem.h"
#include "soarGar.h"
#include "soarRECO.h"
#include "soarSUA.h"
#include "soarGPSInfo.h"
#include "soarFlrm.h"
#include "soarEW.h"
#include "soarENL.h"
#include "soarGEOMAG.h"

#ifdef VISORSUPPORT
	// lines needed for the Visor Keyboard Call
	#include <HsExt.h>
#endif

// Device Data
PalmData device;

// Main program data
ApplicationData data;

// program control
UInt16 	oldAutoOffTime;
Boolean allowExit = false;
Boolean menuopen = false;
UInt32 	origform;
UInt32 	origform2;
Int8 	screenchainpos;
Boolean chain2 = false;
Boolean lastkeywasmenu = false;
Boolean ManualDistChg = false;
Boolean taskonhold = false;
Boolean savtaskonhold = false;
Int16 defaultscreen;
Int16 startupscreen;

// time
DateTimeType curtime;
UInt32 cursecs;
DateTimeType gpstime;
UInt32 gpssecs;
UInt32 utcsecs;
UInt32 SPstartsecs;

// GPS data flags
UInt32 	readtime;
UInt32 	nofixtime;
Boolean checkfixtime = false;
UInt32 	no_read_count = 0xffffffff;
Boolean GPSdatavalid = false;
UInt32 nogpstime = 0;
Boolean hadGPSdatavalid = false;

// GPS sim (IGC replay)
Boolean sim_gps = false;
Int32	sim_idx = 0;
UInt32	sim_time = 0;
UInt32	sim_last_time = 0;
UInt16  sim_rec = 0;
SimPoint simpoint;

// Global screen Data
ScreenGlobals SCREEN;

//Global buffer for parsing
Char buf[PARSELEN];

// basic colours
IndexedColorType indexRed, indexGreen, indexBlue, indexBlack, indexWhite, indexGrey, indexOrange;
// defined colours
IndexedColorType indexTask, indexSUA, indexSUAwarn, indexSector, indexWaypt, indexSink, indexWeak, indexStrong;

// fonts
MemHandle waysymb11hand;
FontPtr waysymb11;
//MemHandle palmlargehihand;
//FontPtr palmlargehi;
//MemHandle font14hand;
//FontPtr font14;
//MemHandle font20hand;
//FontPtr font20;

// variables for new alert dialogs
QuestionData *question;
WarningData *warning;

// external variables
extern Boolean updatewind;
extern Boolean updatetime;
extern Boolean recv_data;
extern Boolean updatemap;
extern Boolean wasinthermal;
extern ThermalData lastthermal[THERMALSTOAVG];
extern Boolean   newTaskWay;
extern UInt16    numWaypts;
extern Int16     selectedWaypointIndex;
extern Boolean   addWayToTask;
extern Boolean   wayselect;
extern WaypointData  *selectedWaypoint;
extern WaypointData  *TempWpt;
extern PolarData *inusePolar;
extern PolarData *selectedPolar;
extern SUAIndex *selectedSUA;
extern RECOData *recodata;
extern EWMRData *ewmrdata;
extern SUAAlertData *suaalert;
extern SUAAlertRet *suaalertret;
extern FlightData *fltdata;
extern Int16     activetskway;
extern Int16     selectedTaskWayIdx;
extern Boolean   pressedgo;
extern Int16     currentWayptPage;
extern Int16     tskWayAddIdx;
extern Boolean   manualzoomchg;
extern double    zoommapscale;
extern double    curmapscale;
extern double    actualmapscale;
extern Boolean   inflight;
extern UInt8     thmode;
extern Boolean   recv_palt;
extern double 	 savmapscale;
extern Boolean   dispactive;
extern Boolean   SUAselectall;
extern Int16 currentTaskListPage;
extern GPSSatDataType *satData;
extern GPSSatDataType *satDataFlarm;
extern GPSPVTDataType *pvtData;
extern double sectorlat, sectorlon, sectorscale;
extern TaskData *edittsk;
extern TaskData *tsk;
extern TaskData *tsktoedit;
extern Boolean activetasksector;
extern Int16 numOfWaypts;
extern Int16 numOfTasks;
extern Int16 taskIndex;
extern Int16 currentSUAPage;
extern Int16 selectedSUAListIdx;
extern Int16 numSUARecs;
extern Boolean allowgenalerttap;
extern Boolean draw_log;
extern Boolean draw_task;
extern double logmapscale;
extern Int8 logmapscaleidx;
extern Int8 logmaporient;
extern Boolean saveqfe;
extern TaskData *temptsk;
extern Boolean settaskreadonly;
extern Boolean exittaskreadonly;
extern UInt16 WayptsortType;
extern Boolean emergencyland;
extern Int16 currentTaskPage;
extern Int16 selectedPolarIndex;
extern Int16 currentPolarPage;
extern Int16 numOfPolars;
extern Int16 cainumLogs;
extern Int16 currentCAIFltPage;
extern Int16 selectedCAIFltIndex;
extern Int16 CAInumperpage;
extern Boolean gotoGPSinfo;
extern Boolean Flarmpresent;
extern WindProfileType *windprofile;
extern Int16 *profilescale;
extern Int16 profilebase;
extern double profilestep;
extern Int8 xfrdialog;
extern Int16 currentFilePage;
extern Int16 selectedFileIndex;
extern Int16 Filenumperpage;
extern Int16 numfilesfound;
extern UInt8 mapmode;
extern Int16 suasortType;
extern Boolean skipmc;
extern Boolean skipbugs;
extern Boolean skipballast;
extern UInt32 warning_time;
extern Int16 inareasector;
extern Int16 tempwayidx;
extern Boolean IsGPS;
extern Int8 CompCmd;
extern Boolean skipnewflight;
extern Boolean chgscreen;
extern Boolean goingtotaskedit;
extern Boolean gototskwpt;
extern Int16 requestedtskidx;
extern Boolean tasknotfinished;
extern Boolean goingtotaskscrn;
extern double MCCurVal;
extern UInt32 EXportID;
extern UInt32 formopen;

// external variables for terrain
extern Boolean terrainpresent;
extern Boolean terrainvalid;
extern Boolean offterrain;
extern Boolean tskoffterrain;
extern Boolean crash;
extern double crashalt;
extern double crashlat;
extern double crashlon;
extern Boolean crash1;
extern double crashalt1;
extern double crashlat1;
extern double crashlon1;
extern Boolean wptcrash;
extern double wptcrashalt;
extern double wptcrashlat;
extern double wptcrashlon;
extern double *wptterheights;
extern Int32 numwptter;
extern Int32 prevnumwptter;
extern double prevterbear;
extern Boolean tskcrash;
extern double tskcrashalt;
extern double tskcrashlat;
extern double tskcrashlon;
extern double *tskterheights;
extern Int32 numtskter;
extern Int32 prevnumtskter;
extern Int8 io_file_type;
extern Int8 io_type;

#ifdef VISORSUPPORT
	// function required for Visor support
	Err HsExtKeyboardEnable(Boolean enable) SYS_SEL_TRAP (sysTrapHsSelector, hsSelExtKeyboardEnable);
#endif

// functions

/**
* \fn ResetDatebookAlarm()
* \brief reset databook alarms
*/
static void ResetDatebookAlarm()
{
 UInt16 cardNo;
 LocalID dbID;
 DmSearchStateType searchInfo;

 DmGetNextDatabaseByTypeCreator (true, &searchInfo, 
 sysFileTApplication, sysFileCDatebook, true, &cardNo, &dbID);

 AlmSetAlarm (cardNo, dbID, NULL, 0, true);
}

/**
* \fn AllocGlobals()
* \brief allocates memory for the various global data structures
*/
static void AllocGlobals()
{
	AllocMem((void *)&selectedWaypoint, sizeof(WaypointData));
	AllocMem((void *)&TempWpt, sizeof(WaypointData));
	AllocMem((void *)&inusePolar, sizeof(PolarData));
	AllocMem((void *)&selectedPolar, sizeof(PolarData));
	AllocMem((void *)&selectedSUA, sizeof(SUAIndex));
	AllocMem((void *)&recodata, sizeof(RECOData));
	AllocMem((void *)&ewmrdata, sizeof(EWMRData));
	AllocMem((void *)&suaalert, sizeof(SUAAlertData));
	AllocMem((void *)&suaalertret, sizeof(SUAAlertRet));
	AllocMem((void *)&fltdata, sizeof(FlightData));
	AllocMem((void *)&satData , GPSMAXSATS * sizeof(GPSSatDataType));
	AllocMem((void *)&satDataFlarm , GPSMAXSATS * sizeof(GPSSatDataType));
	AllocMem((void *)&pvtData, sizeof(GPSPVTDataType));
	AllocMem((void *)&tsk, sizeof(TaskData));
	AllocMem((void *)&edittsk, sizeof(TaskData));
	AllocMem((void *)&temptsk, sizeof(TaskData));
	AllocMem((void *)&windprofile, WINDPROFILEPOINTS * sizeof(WindProfileType));
	AllocMem((void *)&question, sizeof(QuestionData));
	AllocMem((void *)&warning, sizeof(WarningData));

	// clear allocated memory
	MemSet(selectedWaypoint, sizeof(WaypointData), 0);
	MemSet(TempWpt, sizeof(WaypointData), 0);
	MemSet(inusePolar, sizeof(PolarData), 0);
	MemSet(selectedPolar, sizeof(PolarData), 0);
	MemSet(selectedSUA, sizeof(SUAIndex), 0);
	MemSet(recodata, sizeof(RECOData), 0);
	MemSet(ewmrdata, sizeof(EWMRData), 0);
	MemSet(suaalert, sizeof(SUAAlertData), 0);
	MemSet(suaalertret, sizeof(SUAAlertRet), 0);
	MemSet(fltdata, sizeof(FlightData), 0);
	MemSet(satData, GPSMAXSATS * sizeof(GPSSatDataType), 0xff);
	MemSet(satDataFlarm, GPSMAXSATS * sizeof(GPSSatDataType), 0xff);
	MemSet(pvtData, sizeof(GPSPVTDataType), 0);
	MemSet(tsk, sizeof(TaskData), 0);
	MemSet(edittsk, sizeof(TaskData), 0);
	MemSet(temptsk, sizeof(TaskData), 0);
	MemSet(windprofile, WINDPROFILEPOINTS * sizeof(WindProfileType), 0);
	MemSet(question, sizeof(QuestionData), 0);
	MemSet(warning, sizeof(WarningData), 0);

	// display size of data
//	HostTraceOutputTL(appErrorClass, "Size of Task db %s", DblToStr(sizeof(TaskData),0));
//	HostTraceOutputTL(appErrorClass, "Size of Waypoint db %s", DblToStr(sizeof(WaypointData),0));
//	HostTraceOutputTL(appErrorClass, "Size of SUA Index db %s", DblToStr(sizeof(SUAIndex),0));
//	HostTraceOutputTL(appErrorClass, "Size of Flight db %s", DblToStr(sizeof(FlightData),0));
//	HostTraceOutputTL(appErrorClass, "Size of Logger db %s", DblToStr(sizeof(LoggerData),0));
//	HostTraceOutputTL(appErrorClass, "Size of SUA Index db %s", DblToStr(sizeof(SUAIndex),0));
//	HostTraceOutputTL(appErrorClass, "Size of SUA Data db %s", DblToStr(sizeof(SUAData),0));
//	HostTraceOutputTL(appErrorClass, "Size of Polar db %s", DblToStr(sizeof(PolarData),0));
//	HostTraceOutputTL(appErrorClass, "Size of Wind Profile Data %s", DblToStr(WINDPROFILEPOINTS * sizeof(WindProfileType),0));

}

/**
* \fn FreeGlobals()
* \brief frees the allocated memory buffers
*/
static void FreeGlobals()
{
	FreeMem((void *)&selectedWaypoint);
	FreeMem((void *)&TempWpt);
	FreeMem((void *)&inusePolar);
	FreeMem((void *)&selectedPolar);
	FreeMem((void *)&selectedSUA);
	FreeMem((void *)&recodata);
	FreeMem((void *)&ewmrdata);
	FreeMem((void *)&suaalert);
	FreeMem((void *)&suaalertret);
	FreeMem((void *)&fltdata);
	FreeMem((void *)&satData);
	FreeMem((void *)&satDataFlarm);
	FreeMem((void *)&pvtData);
	FreeMem((void *)&tsk);
	FreeMem((void *)&edittsk);
	FreeMem((void *)&temptsk);
	FreeMem((void *)&windprofile);
	FreeMem((void *)&question);
	FreeMem((void *)&warning);
}

/**
* \fn StartApplication(void)
* \brief called when SoarPilot is started
* \return always false meaning succesful startup
*/
static int StartApplication(void)
{
	UInt32 ftrDevVersion;
	UInt16 volRefNum;
	VolumeInfoType volInfoP;
	Char tempchar[35];
	Err err;
	MemHandle waypoint_hand;
	MemPtr waypoint_ptr;
	UInt16 nrecs;
	MemHandle flt_hand;
	MemPtr flt_ptr;
	LoggerData *logdata;
	MemHandle log_hand;
	MemPtr log_ptr;
	UInt32 	temprom;
	UInt32 	scrattr;

	HostTraceInit();
	HostTraceOutputTL(appErrorClass, "Tracing Initialized");

	// initialise time variables
	cursecs = TimGetSeconds();
	TimSecondsToDateTime(cursecs , &curtime);
	gpssecs = 0;
	TimSecondsToDateTime(gpssecs , &gpstime);
	utcsecs = 0;
	readtime = cursecs;
	nofixtime = cursecs;
	data.input.logpolldate = cursecs;
	data.input.logtrkdate = cursecs;
	data.input.logsmsdate = cursecs;
	SPstartsecs = cursecs;
	
	// get SoarPilot version
	MemSet(&device, sizeof(PalmData),0); /* all fields are invalid */
	StrCopy(tempchar, "SoarPilot ");
	GetSysVersion(device.appVersion);
	StrCat(tempchar, device.appVersion);
	StrCat(tempchar, " Loading.....");
	WinDrawChars(tempchar, StrLen(tempchar), 30, 10);

	// symbol font
	waysymb11hand=DmGetResource('NFNT', WAYSYMB11);
	waysymb11=MemHandleLock(waysymb11hand);
	FntDefineFont(WAYSYMB11, waysymb11);
	// large font
//	palmlargehihand=DmGetResource('NFNT', PALMLARGEHI);
//	palmlargehi=MemHandleLock(palmlargehihand);
//	FntDefineFont(PALMLARGEHI, palmlargehi);
	// font 14
//	font14hand=DmGetResource('NFNT', FONT14);
//	font14=MemHandleLock(font14hand);
//	FntDefineFont(FONT14, font14);
	// font 20
//	font20hand=DmGetResource('NFNT', FONT20);
//	font20=MemHandleLock(font20hand);
//	FntDefineFont(FONT20, font20);

	// Allocate the global variables
	AllocGlobals();
	MemSet(&data,sizeof(ApplicationData),0); /* all fields are invalid */

	// get application version
	GetSysVersion(device.appVersion);

	// Checks to see if this is a Sony Clie PDA
	FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &temprom);
	if (temprom == sonyHwrOEMCompanyID_Sony) {
//		HostTraceOutputTL(appErrorClass, "Setting CliePDA to true");
		device.CliePDA = true;
	} 

	// Checks to see if this is the StyleTap Emulator
	FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &temprom);
	if (temprom == 'stap') {
//		HostTraceOutputTL(appErrorClass, "Checking StyleTapPDA platform");
		FtrGet(sysFtrCreator, sysFtrNumOEMDeviceID, &temprom);
		if (temprom == 'iphn') {
			device.StyleTapPDA = STIPHONE;
		} else {
			device.StyleTapPDA = STWINMOBILE;
 		}
	}

	// Determines if the New Serial Manger and New Serial Manager V2 is installed
	err = FtrGet(sysFileCSerialMgr, sysFtrNewSerialPresent, &temprom);
	if (temprom != 0 && err == 0) {
		device.NewSerialPresent = true;
		err = FtrGet(sysFileCSerialMgr, sysFtrNewSerialVersion, &temprom);
		if (temprom == 2 && err == 0) {
			err = FtrGet(sysFtrCreator, sysFtrNumROMVersion, &temprom);
			if (temprom >= SYS_VER_40 && err == 0) {
				device.NewSerialV2Present = true;
			}
		}
	}

	// Determines if the Notification Manger is installed
//	FtrGet(sysFtrCreator, sysFtrNumNotifyMgrVersion, &temprom);
//	if (temprom != 0) {
//		NotifyMgrPresent = true;
//	}

//	HostTraceOutputTL(appErrorClass, "default kDensityLow Screen");
	device.HiDensityScrPresent = kCoordinatesNative;
	SCREEN.SRES = 1.0;
	SCREEN.WIDTH=WIDTH_BASE;
	SCREEN.HEIGHT=HEIGHT_BASE;
	SCREEN.GLIDERX=80;
	SCREEN.GLIDERY=100;

	// Determines if Hi-Density Screen Support is installed
	FtrGet(sysFtrCreator, sysFtrNumWinVersion, &temprom);
	if (temprom >= 4) {
		WinScreenGetAttribute(winScreenDensity, &scrattr);
		switch (scrattr) {
			case kDensityDouble:
//				HostTraceOutputTL(appErrorClass, "kDensityDouble Screen");
				device.HiDensityScrPresent = kCoordinatesDouble;
				SCREEN.SRES = 2.0;
				SCREEN.WIDTH=320;
				SCREEN.HEIGHT=320;
				SCREEN.GLIDERX=160;
				SCREEN.GLIDERY=200;
				break;
			case kDensityOneAndAHalf:
//				HostTraceOutputTL(appErrorClass, "kDensityOneAndAHalf Screen");
				device.HiDensityScrPresent = kCoordinatesOneAndAHalf;
				SCREEN.SRES = 1.5;
				SCREEN.WIDTH=240;
				SCREEN.HEIGHT=240;
				SCREEN.GLIDERX=120;
				SCREEN.GLIDERY=150;
				break;
			case kDensityTriple:
//				HostTraceOutputTL(appErrorClass, "kDensityTriple Screen");
				break;
			case kDensityQuadruple:
//				HostTraceOutputTL(appErrorClass, "kDensityQuadruple Screen");
				break;
		}
	}
/*
	// was used in lists to achieve aligned numbers
	if (HiDensityScrPresent) {
		palmlargehand=DmGetResource('nfnt', PALMLARGE);
		palmlarge=MemHandleLock(palmlargehand);
	} else {
		palmlargehand=DmGetResource('NFNT', PALMLARGEOLD);
		palmlarge=MemHandleLock(palmlargehand);
	}
	FntDefineFont(PALMLARGE, palmlarge);
*/

	// Sets the global system version variable
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &device.romVersion);

	// Check to see if the device supports Bluetooth
	device.BTCapable = BTEnabledDevice();

	// Check to see if the device supports USB
	device.USBCapable = USBEnabledDevice();

	// Check to see if on a Garmin iQue 
	device.iQueCapable = iQueEnabledDevice();
	if (device.iQueCapable) data.config.usegpstime = false;

	// Check to see if the device supports Dynamic Input Area
	device.DIACapable = DIAEnabledDevice();
	if (device.DIACapable) {
		RegisterWinResizedEventNotification(true);
	}

	// Check to see if the device supports Memory Cards
	device.VFSCapable = CardEnabledDevice();

	if (device.VFSCapable) {
		// Check to see if there is a card inserted
		device.CardPresent = FindVolRefNum(&volRefNum);
		// Register for Volume Mount Events
		RegisterVolumeNotification(true);
		// check if write proteceted or hidden
		if (device.CardPresent) {
			VFSVolumeInfo(volRefNum, &volInfoP);
			if (volInfoP.attributes & vfsVolumeAttrReadOnly) {
				device.CardRW = false;
			} else {
				device.CardRW = true;
			}
		}

#ifdef NMEALOG
		// log all NMEA records
		XferInit("nmealog.txt", IOOPENTRUNC, USEVFS);

		SecondsToDateOrTimeString(cursecs, tempchar, 1, 0);
		StrCat(tempchar, "-Start");
		outputlog(tempchar, true);

		StrCopy(tempchar, DblToStr(device.romVersion,0));
		StrCat(tempchar, "-ROM Version");
		outputlog(tempchar, true);

		StrCopy(tempchar, DblToStr(SYS_VER_50,0));
		StrCat(tempchar, "-SYS ROM 50");
		outputlog(tempchar, true);
#endif

	}

	// check to see if sound recording capable for ENL
	device.ENLCapable = ENLCapableDevice();

	// set battery levels
	device.lowbatlevel = 99;
	device.batpercent = 99;
	device.lowbatcount = 0;

	// load mathlib
	LoadUnloadMathLib(true);

	// Sets the Auto Off Timer to zero meaning it will never power off
	oldAutoOffTime = SysSetAutoOffTime(0);

#ifdef VISORSUPPORT
	// If on a Visor, we need to disable the keyboard daemon before opening
	// library/port else we get error 775
	// Checks to see if this is a Handspring PDA
	if (!FtrGet ('hsEx', 0, &ftrDevVersion)) {
		FtrGet(sysFtrCreator, sysFtrNumOEMDeviceID, &ftrDevVersion);
		if (ftrDevVersion != 'H101' && ftrDevVersion != 'H201') {
//			HostTraceOutputTL(appErrorClass, "Disabling HS Keyboard daemon");
			HsExtKeyboardEnable(false);
		}
	}
#endif

	// Check to see if this is a HandEra 330
	FtrGet(sysFtrCreator, sysFtrNumOEMDeviceID, &ftrDevVersion);
	if (ftrDevVersion == 'trg2') {
//		HostTraceOutputTL(appErrorClass, "This is a HandEra 330");
		device.CFCapable = true;
	}

	// Check if connection called "ExtGPS" is available
	if (device.romVersion >= SYS_VER_35) {
		device.EXCapable = (CncGetProfileInfo("ExtGPS", &EXportID, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == errNone);
	}
	
	// Open and Initialize(if necessary) all databases
	if (OpenAllDatabases()) {
		return(true);
	}

	// display Current Pilot Name on startup screen
	StrCopy(tempchar, "Pilot:");
	WinDrawChars(tempchar, StrLen(tempchar), 30, 26);
	StrCopy(tempchar, data.igchinfo.name);
	WinDrawChars(tempchar, StrLen(tempchar), 80, 26);
	// display Current Glider Type on startup screen
	StrCopy(tempchar, "Glider:");
	WinDrawChars(tempchar, StrLen(tempchar), 30, 38);
	StrCopy(tempchar, data.igchinfo.type);
	WinDrawChars(tempchar, StrLen(tempchar), 80, 38);
	// display Current Glider Polar on startup screen
	StrCopy(tempchar, "Polar:");
	WinDrawChars(tempchar, StrLen(tempchar), 30, 50);
	StrCopy(tempchar, data.polar.name);
	WinDrawChars(tempchar, StrLen(tempchar), 80, 50);
	// display flight computer selected
	StrCopy(tempchar, "Comp:");
	WinDrawChars(tempchar, StrLen(tempchar), 30, 62);
	SysStringByIndex(form_comp_table, data.config.flightcomp, tempchar, 15);
	WinDrawChars(tempchar, StrLen(tempchar), 80, 62);
	// display altitude source selected
	StrCopy(tempchar, "Alt:");
	WinDrawChars(tempchar, StrLen(tempchar), 30, 74);
	SysStringByIndex(form_alt_table, data.config.pressaltsrc, tempchar, 15);
	WinDrawChars(tempchar, StrLen(tempchar), 80, 74);

	// display config, waypoint and SUA files last loaded
	StrCopy(tempchar, "Config:");
	WinDrawChars(tempchar, StrLen(tempchar), 30, 98);
	WinDrawChars(data.config.config_file, StrLen(data.config.config_file), 80, 98);
	StrCopy(tempchar, "Waypoints:");
	WinDrawChars(tempchar, StrLen(tempchar), 30, 110);
	WinDrawChars(data.config.waypt_file, StrLen(data.config.waypt_file), 80, 110);
	StrCopy(tempchar, "SUA Data:");
	WinDrawChars(tempchar, StrLen(tempchar), 30, 122);
	WinDrawChars(data.config.SUA_file, StrLen(data.config.SUA_file), 80, 122);

	Sleep(3);

	// This tests whether the device is a Treo
	device.TreoPDA = TreoDevice();

	// This enforces that SMS messages are only for Treo
	// right now.
	if (!device.TreoPDA) data.config.outputSMS = false;

	// If there is no VFS card inserted and configured to use one for
	// data transfer, reset it to use the serial port
	if (!device.CardPresent && data.config.xfertype == USEVFS) {
		data.config.xfertype = USESER;
	}
	
	// This switches back to serial for nmea data if the transfer type
	// is Bluetooth or iQue and they aren't available
	if (!device.BTCapable && data.config.nmeaxfertype == USEBT) {
		data.config.nmeaxfertype = USESER;
	} else if (!device.USBCapable && data.config.nmeaxfertype == USEUSB) {
		data.config.nmeaxfertype = USESER;
	} else if (!device.iQueCapable && (data.config.nmeaxfertype == USEIQUE || data.config.nmeaxfertype == USEIQUESER)) {
		data.config.nmeaxfertype = USESER;
	}

	// initialise variables
	data.input.basemc = 0.0;
	data.input.newmc = 0.0;
	data.input.headwind = 0.0;
	data.input.gpslatdbl = 0.0;
	data.input.gpslngdbl = 0.0;
	data.input.coslat = 0.707;
	data.input.logpollint = data.config.slowlogint;
	data.input.curmaporient = data.config.maporient;
	data.input.QNHpressure = 1013.25;
	data.input.QNHaltcorrect = 0.0;
	data.input.lastglide = 0.0;
	recv_palt = false;
	data.input.showterbox = false;
	StrCopy(data.logger.gpsstat, "V");
	data.logger.enl = 0;
	data.input.isctlpt = false;
	data.input.nextwpt_dist = -1.0;
	data.input.nextwpt_bear = 0.0;
	savmapscale = curmapscale;
	if (data.config.AATmode == AAT_MTURN_ON) data.config.AATmode = AAT_MTURN;

	// initialise GPS communication
	pvtData->status.fix = gpsFixInvalid;
	SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
	XferInit(tempchar, NFC, data.config.nmeaxfertype);

	// initialise NMEA pass through communication
	if ((data.config.nmeaxfertype == USEBT) && data.config.echoNMEA)
		XferInitAlt(4800, NFC, USESER);

	// This call attempts to find the first waypoint marked with the HOME attribute
	// If it does, it initializes the current positions variables to be the lat/long
	// of the home airport.  This allows the map to be functional even without a GPS
	// attached.
	if (FindHomeWayptInitialize()) {
		updatemap = true;
		// Setup all data needed for finding current terrain elevation
		InitTerrainData();
		data.input.terelev = GetTerrainElev(data.input.gpslatdbl, data.input.gpslngdbl);

		// defaults to goto home waypoint as selected waypoint if using soarpilot waypoints
		if (data.config.usepalmway && data.config.defaulthome && (tempwayidx != -1)) {
			selectedWaypointIndex = tempwayidx; // set in FindHomeWayptInitialize
			OpenDBQueryRecord(waypoint_db, selectedWaypointIndex, &waypoint_hand, &waypoint_ptr);
			MemMove(selectedWaypoint, waypoint_ptr, sizeof(WaypointData));
			MemHandleUnlock(waypoint_hand);
			MemMove(&data.inuseWaypoint, selectedWaypoint, sizeof(WaypointData));
			data.input.destination_valid = true;
			InuseWaypointCalcEvent();
		}
	}

	// Find the Reference point if one exists
	FindRefWayptInitialize();

	// set up NMEA data parser function
	data.parser.parser_func = nmea_parser;
	StrCopy(data.parser.name, "Standard parser\0");

	// initialise colours
	if (device.romVersion >= SYS_VER_35) {
		WinScreenMode (winScreenModeGetSupportsColor, NULL, NULL, NULL, &device.colorCapable);
		if (device.colorCapable) {
			// basic colours
			indexRed = RGB(255, 0, 0);
			indexGreen = RGB(0, 255, 0);
			indexBlue = RGB(0, 0, 255);
			indexBlack = RGB(0, 0, 0);
			indexWhite = RGB(255, 255, 255);
			indexGrey = RGB(128, 128, 128);
			indexOrange = RGB(255, 128, 0);
			// user specified colour lines on moving map
			indexTask = RGB(data.config.TaskColour.r, data.config.TaskColour.g, data.config.TaskColour.b);
			indexSUA = RGB(data.config.SUAColour.r, data.config.SUAColour.g, data.config.SUAColour.b);
			indexSUAwarn = RGB(data.config.SUAwarnColour.r, data.config.SUAwarnColour.g, data.config.SUAwarnColour.b);
			indexSector = RGB(data.config.SectorColour.r, data.config.SectorColour.g, data.config.SectorColour.b);
			indexWaypt = 	RGB(data.config.WayptColour.r, 		data.config.WayptColour.g, 	data.config.WayptColour.b);
			indexSink = RGB(data.config.SinkColour.r, data.config.SinkColour.g, data.config.SinkColour.b);
			indexWeak = RGB(data.config.WeakColour.r, data.config.WeakColour.g, data.config.WeakColour.b);
			indexStrong = RGB(data.config.StrongColour.r, data.config.StrongColour.g, data.config.StrongColour.b);
		}
	}
	ResetDatebookAlarm();

	// reset SUA warning defaults
	ProcessSUA(false, false);

	// setup wind profile
	CalcWindProfile(true, true);

	// setup track trail
	InitTrkTrail(false);

	// calculate range (in pixels) and bearing (in degrees) to screen corners
	// with and without bottomlabels
	SCREEN.TOPCORNER = Sqrt((double)GLIDERX_BASE*(double)GLIDERX_BASE + (double)GLIDERY_BASE*(double)GLIDERY_BASE);
	SCREEN.TOPANGLE = RadiansToDegrees(Asin((double)GLIDERX_BASE/SCREEN.TOPCORNER));
	if (!device.DIACapable) {
		SCREEN.BOTTOMCORNER = Sqrt((double)GLIDERX_BASE*(double)GLIDERX_BASE + (double)(HEIGHT_BASE-GLIDERY_BASE)*(double)(HEIGHT_BASE-GLIDERY_BASE));
		SCREEN.BOTTOMANGLE = RadiansToDegrees(Asin((double)(HEIGHT_BASE-GLIDERY_BASE)/SCREEN.BOTTOMCORNER)) + 90;
		SCREEN.BOTTOMCORNERLBL = Sqrt((double)GLIDERX_BASE*(double)GLIDERX_BASE + (double)(HEIGHT_BASE-GLIDERY_BASE-LABEL_HEIGHT)*(double)(HEIGHT_BASE-GLIDERY_BASE-LABEL_HEIGHT));
		SCREEN.BOTTOMANGLELBL = RadiansToDegrees(Asin((double)(HEIGHT_BASE-GLIDERY_BASE-LABEL_HEIGHT)/SCREEN.BOTTOMCORNERLBL)) + 90;
	} else {
		SCREEN.BOTTOMCORNER = Sqrt((double)GLIDERX_BASE*(double)GLIDERX_BASE + (double)(HEIGHT_BASE_DIA-GLIDERY_BASE)*(double)(HEIGHT_BASE_DIA-GLIDERY_BASE));
		SCREEN.BOTTOMANGLE = RadiansToDegrees(Asin((double)(HEIGHT_BASE_DIA-GLIDERY_BASE)/SCREEN.BOTTOMCORNER)) + 90;
		SCREEN.BOTTOMCORNERLBL = Sqrt((double)GLIDERX_BASE*(double)GLIDERX_BASE + (double)(HEIGHT_BASE-GLIDERY_BASE)*(double)(HEIGHT_BASE-GLIDERY_BASE));
		SCREEN.BOTTOMANGLELBL = RadiansToDegrees(Asin((double)(HEIGHT_BASE-GLIDERY_BASE)/SCREEN.BOTTOMCORNERLBL)) + 90;
	}

//	HostTraceOutputTL(appErrorClass, "Top Corner %s", DblToStr(SCREEN.TOPCORNER,2));
//	HostTraceOutputTL(appErrorClass, "Top Angle %s", DblToStr(SCREEN.TOPANGLE,2));
//	HostTraceOutputTL(appErrorClass, "Bottom Corner %s", DblToStr(SCREEN.BOTTOMCORNER,2));
//	HostTraceOutputTL(appErrorClass, "Bottom Angle %s", DblToStr(SCREEN.BOTTOMANGLE,2));
//	HostTraceOutputTL(appErrorClass, "Bottom Corner Label %s", DblToStr(SCREEN.BOTTOMCORNERLBL,2));
//	HostTraceOutputTL(appErrorClass, "Bottom Angle Label %s", DblToStr(SCREEN.BOTTOMANGLELBL,2));

	// clear thermal waypoints and data
	DeleteThermalWaypts();
	wasinthermal = false;
	MemSet(&lastthermal, sizeof(lastthermal), 0);
	
	// check for cambridge logger and start flight mode
	if (data.config.flightcomp == C302COMP  || 
		 data.config.flightcomp == C302ACOMP ||
		 data.config.flightcomp == GPSNAVCOMP) {
		SendCAIFlightModeStart(true);
	}

	// check for correct wind calculation setting
	if (data.config.flightcomp != C302COMP   &&
	    data.config.flightcomp != LXCOMP     &&
	    data.config.flightcomp != LXVARCOMP  &&
	    data.config.flightcomp != FILSERCOMP && 
	    data.config.flightcomp != SN10COMP   &&
	    data.config.flightcomp != B50LXCOMP) {
		data.config.useinputwinds = false;
	}

	// repair last flight_db record if stopidx was lost and fix stop times
	if ((OpenDBCountRecords(flight_db) > 0) & (OpenDBCountRecords(logger_db) > 0)) {
                AllocMem((void *)&logdata, sizeof(LoggerData));
		nrecs = OpenDBCountRecords(flight_db)-1;
		// logger record stop index
		OpenDBQueryRecord(flight_db, nrecs, &flt_hand, &flt_ptr);
		MemMove(fltdata, flt_ptr, sizeof(FlightData));
		MemHandleUnlock(flt_hand);
		fltdata->stopidx = OpenDBCountRecords(logger_db)-1;
		// flight stop time
		OpenDBQueryRecord(logger_db, OpenDBCountRecords(logger_db)-1, &log_hand, &log_ptr);
		MemMove(logdata, log_ptr, sizeof(LoggerData));
		MemHandleUnlock(log_hand);
		StrCopy(fltdata->stopdtg, logdata->gpsdtg);
		StrCopy(fltdata->stoputc, logdata->gpsutc);
		// update flight database
		OpenDBUpdateRecord(flight_db, sizeof(FlightData), fltdata, nrecs);
		FreeMem((void *)&logdata);
//		HostTraceOutputTL(appErrorClass, "Start %s", DblToStr(fltdata->startidx,0));
//		HostTraceOutputTL(appErrorClass, "Stop  %s", DblToStr(fltdata->stopidx,0));
	}

	// decide default and startup screen
	defaultscreen = form_final_glide;
	if (data.config.showQFEQNHonstartup) {
		startupscreen = form_set_qnh;
	} else if (data.config.defaulttoFG) {
		startupscreen = form_final_glide;
	} else {
		startupscreen = form_moving_map;
		defaultscreen = form_moving_map;
	}

	// check for startup warnings
	if (device.CardPresent && !device.CardRW) {
		// popup warning for readonly SD card
		warning->type = Wcardwrite;
		FrmPopupForm(form_warning);
	} else if (estlogtimeleft() < 5.1) {
		// popup warning if < 5 hours remaining
		warning->type = Wlogtime;
		FrmPopupForm(form_warning);
	} else {
		// goto to startup screen
		FrmGotoForm(startupscreen);
	}

	return(false);
}

/**
* \fn StopApplication()
* \brief called when SoarPilot is exited
*/
static void StopApplication(void)
{
#ifdef NMEALOG
	Char tempchar[15];
#endif

/*
  Char tempchar[25];

	XferInit("styletap.txt", IOOPENTRUNC, USEVFS);
	SecondsToDateOrTimeString(cursecs, tempchar, 1, 0);
	StrCat(tempchar, "-Start");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/

	// test code to copy length of SP run into OOID in IGChinfo
//	StrCopy(data.igchinfo.ooid, DblToStr((double)(TimGetSeconds()-SPstartsecs)/3600.0, 2));

	// Close the serial port and cleanup memory use
//	HostTraceOutputTL(appErrorClass, "About to call XferClose");
//	HostTraceOutputTL(appErrorClass, "data.config.nmeaxfertype-|%hd|", data.config.nmeaxfertype);

/*
	StrCopy(tempchar, "XferClose");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/
	// close GPS communication
	XferClose(data.config.nmeaxfertype);

	// close NMEA pass through communication
	XferCloseAlt();

//	HostTraceOutputTL(appErrorClass, "About to call PerformLogging");
	/* If currently logging, will properly clean up the flight log */ 
/*
	StrCopy(tempchar, "PerformLogging");

	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/
	PerformLogging(true);

/*
	StrCopy(tempchar, "ThermalPoints");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/

	// delete temp thermal waypoints if the exist
	DeleteThermalWaypts();
/*
	StrCopy(tempchar, "ConfigInfo");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/

	/* Write out Config Info */
	OpenDBUpdateRecord(config_db, sizeof(ConfigFlight), &data.config, CONFIG_REC);

/*
	StrCopy(tempchar, "IGCHeader");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/
	/* Write out IGC Header Info */
	OpenDBUpdateRecord(config_db, sizeof(IGCHInfo), &data.igchinfo, IGCHINFO_REC);

/*
	StrCopy(tempchar, "VariousCleanup");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/

	// Various memory cleanup calls
	refresh_task_list(9999);
	refresh_waypoint_list(9999);
	refresh_polar_list(9999);
	refresh_task_details(TASKFREE);
	refresh_sua_list(SUAFREE);
	InitTrkTrail(true);
	UpdateMap2(true, 0.0, 0.0, 0.0, true, NULL, true);
	CleanUpTerrainData();
	ProcessSUA(false, true);

/*
	StrCopy(tempchar, "Waypoint Terrain Data");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/
	// free terrain arrays
	if ((prevnumwptter > 0) && (wptterheights != NULL)) {
		// free previous array
//		HostTraceOutputTL(appErrorClass, "Free Terrain Heights %s", DblToStr(prevnumter,0));
		FreeMem((void *)&wptterheights);
		wptterheights = NULL;
	}

/*
	StrCopy(tempchar, "Task Terrain Data");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/
	if ((prevnumtskter > 0) && (tskterheights != NULL)) {
		// free previous array
//		HostTraceOutputTL(appErrorClass, "Free Task Terrain Heights %s", DblToStr(prevnumter,0));
		FreeMem((void *)&tskterheights);
		tskterheights = NULL;
	}

/*
	StrCopy(tempchar, "Close All Forms Terrain Data");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/

	// Close all forms
	FrmCloseAllForms();

/*
	StrCopy(tempchar, "Close All Databases");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/

	// Close all open databases
	CloseAllDatabases();

/*
	StrCopy(tempchar, "Unload Mathlib");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/
	LoadUnloadMathLib(false);

/*
	StrCopy(tempchar, "Set AutoOff Time");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/

	// Sets the Auto Off Timer to back to the original value when the program was run
	SysSetAutoOffTime(oldAutoOffTime);

	// Unregister for Volume Mount Events

#ifdef NMEALOG
	if (device.VFSCapable) {
		SecondsToDateOrTimeString(cursecs, tempchar, 1, 0);
		StrCat(tempchar, "-End");
		outputlog(tempchar, true);

		XferClose(USEVFS);
	}
#endif

/*
	StrCopy(tempchar, "VolumeNotification");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/

	if (device.VFSCapable) {
		RegisterVolumeNotification(false);
	}

/*
	StrCopy(tempchar, "ResizeNotification");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/

	if (device.DIACapable) {
		RegisterWinResizedEventNotification(false);
	}

//	if (NotifyMgrPresent) {
//		RegisterCableAttachDetachNotification(false);
//	}

/*
	StrCopy(tempchar, "ReleaseFonts");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/
	DmReleaseResource (waysymb11hand);
//	DmReleaseResource (palmlargehihand);
//	DmReleaseResource (font14hand);
//	DmReleaseResource (font20hand);
//	DmReleaseResource (palmlargehand);

	MemHandleUnlock(waysymb11hand);
//	MemHandleUnlock(palmlargehihand);
//	MemHandleUnlock(font14hand);
//	MemHandleUnlock(font20hand);
//	MemHandleUnlock(palmlargehand);

	if (device.HiDensityScrPresent) {
/*
		StrCopy(tempchar, "CoordinateSystem");
		StrCatEOL(tempchar, USEVFS);
		TxData(tempchar, USEVFS);
*/
//		HostTraceOutputTL(appErrorClass, "StopApp-WinGetCoordinateSystem = |%hu|", WinGetCoordinateSystem());
		WinSetCoordinateSystem(kCoordinatesStandard);
	}

/*
	StrCopy(tempchar, "FreeGlobals");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
*/
	FreeGlobals();

/*
	SecondsToDateOrTimeString(cursecs, tempchar, 1, 0);
	StrCat(tempchar, "-End");
	StrCatEOL(tempchar, USEVFS);
	TxData(tempchar, USEVFS);
	XferClose(USEVFS);
*/

	HostTraceOutputTL(appErrorClass, "Closing Tracing");
	HostTraceClose();
	return;
}

/**
* \fn ApplicationHandleEvent(EventPtr event)
* \brief main event handler
* \param event pointer to event data
* \return true in case event is handled, else false
*/
Boolean ApplicationHandleEvent(EventPtr event)
{
	FormPtr frm;
	Int16 formId;
	Boolean handled = false;
	EventType newEvent;
	Int8 curxfertype;
	Boolean nextwptset = false;
	Int16 i,x,y,z;
	double rng1, rng2, rng3, rng4;
	double speed;
	double tempdbl, remainder;
	Char tempchar[80];
	Int16 totalturns;
	double tmpalt;
	Boolean aboveFG;
	static Boolean reachedwaypoint = false;
	MemHandle sim_hand;
	MemPtr sim_ptr;

	// display application events (except nilEvent)
//	if (event->eType != 0) {
//		HostTraceOutputT(appErrorClass, "ApplicationHandleEvent - |%hu|", event->eType);
//		HostTraceOutputTL(appErrorClass, "   Form ID - |%hu|", event->data.frmLoad.formID);
//	}
	switch (event->eType) {
		case reloadAppEvents:
//			HostTraceOutputTL(appErrorClass, "reloadAppEvents-|%hu|", FrmGetActiveFormID());
//			HostTraceOutputTL(appErrorClass, "      event->data.generic.datum[0]-|%hu|", event->data.generic.datum[0]);
		case frmLoadEvent:
			if (event->eType == frmLoadEvent) {
				formId = event->data.frmLoad.formID;
				frm = FrmInitForm(formId);
//				HostTraceOutputTL(appErrorClass, "frmLoadEvent -           |%hu|", formId);
			} else {
				formId = FrmGetActiveFormID();
				frm = FrmGetFormPtr(formId);
//				HostTraceOutputTL(appErrorClass, "reloadAppEvents -        |%hu|", formId);
			}
			FrmSetActiveForm(frm);
			MemSet(&(data.application), sizeof(ApplicationScreen),0); /* all fields are invalid */
			data.application.form_id=formId;
			// load default events
			data.application.display_events[0].valid=VALID;
			data.application.display_events[0].event=InuseWaypointCalcEvent;
			data.application.display_events[1].valid=VALID;
			data.application.display_events[1].event=update_waypoint_list_values_event;
			data.application.display_events[2].valid=VALID;
			data.application.display_events[2].event=check_sua_elements;
			// load form specific events	
			switch (formId) {
				// screens from the Flight menu
				case form_final_glide:
					FrmSetEventHandler(frm, form_final_glide_event_handler);
					data.application.display_events[3].valid=VALID;
					data.application.display_events[3].event=final_glide_event;
					data.application.display_events[4].valid=VALID;
					data.application.display_events[4].event=final_glide_event0;
					data.application.display_events[5].valid=VALID;
					data.application.display_events[5].event=final_glide_event1;
					data.application.display_events[6].valid=VALID;
					data.application.display_events[6].event=final_glide_event2;
					data.application.display_events[7].valid=VALID;
					data.application.display_events[7].event=final_glide_event3;
					data.application.display_events[8].valid=VALID;
					data.application.display_events[8].event=final_glide_event4;
					data.application.display_events[9].valid=VALID;
					data.application.display_events[9].event=final_glide_event5;
					break;
				case form_moving_map:
					FrmSetEventHandler(frm, form_moving_map_event_handler);
					break;
				case form_flt_info:
					FrmSetEventHandler(frm, form_flt_info_event_handler);
					data.application.display_events[3].valid=VALID;
					data.application.display_events[3].event=Update_Fltinfo_Event;
					data.application.display_events[4].valid=VALID;
					data.application.display_events[4].event=Update_Tskinfo_Event;
					break;
				case form_graph_flt:
					FrmSetEventHandler(frm, form_graph_flt_event_handler);
					break;
				case form_wind_disp:
					FrmSetEventHandler(frm, form_wind_disp_event_handler);
					break;
				case form_wind_spddir:
					FrmSetEventHandler(frm, form_wind_spddir_event_handler);
					break;
				case form_wind_3dinfo:
					FrmSetEventHandler(frm, form_wind_3dinfo_event_handler);
					break;
				case form_thermal_history:
					FrmSetEventHandler(frm, form_thermal_history_event_handler);
					break;
				case form_set_qnh:
					FrmSetEventHandler(frm, form_set_qnh_event_handler);
					break;
				// screens from the Navigation menu
				case form_list_waypt:
					FrmSetEventHandler(frm, form_list_waypt_event_handler);
					break;
				case form_av_waypt:
					FrmSetEventHandler(frm, form_av_waypt_event_handler);
					break;
				case form_waypt_addinfo:
					FrmSetEventHandler(frm, form_waypt_addinfo_event_handler);
					break;
				case form_set_task:
					FrmSetEventHandler(frm, form_set_task_event_handler);
					break;
				case form_task_waypt:
					FrmSetEventHandler(frm, form_task_waypt_event_handler);
					break;
				case form_waypoint_sector:
					FrmSetEventHandler(frm, form_waypoint_sector_event_handler);
					break;
				case form_list_task:
					FrmSetEventHandler(frm, form_list_task_event_handler);
					break;
				case form_task_rules:
					FrmSetEventHandler(frm, form_task_rules_event_handler);
					break;
				case form_list_sua:
					FrmSetEventHandler(frm, form_list_sua_event_handler);
					break;
				case form_disp_sua:
					FrmSetEventHandler(frm, form_disp_sua_event_handler);
					break;
				// screens from the Settings menu
				case form_list_polar:
					FrmSetEventHandler(frm, form_list_polar_event_handler);
					break;
				case form_av_polar:
					FrmSetEventHandler(frm, form_av_polar_event_handler);
					break;
				case form_set_fg:
					FrmSetEventHandler(frm, form_set_fg_event_handler);
					break;
				case form_set_units:
					FrmSetEventHandler(frm, form_set_units_event_handler);
					break;
				case form_set_logger:
					FrmSetEventHandler(frm, form_set_logger_event_handler);
					break;
				case form_config_task:
					FrmSetEventHandler(frm, form_config_task_event_handler);
					break;
				case form_set_port:
					FrmSetEventHandler(frm, form_set_port_event_handler);
					break;
				case form_set_map:
					FrmSetEventHandler(frm, form_set_map_event_handler);
					break;
				case form_set_map_colours:
					FrmSetEventHandler(frm, form_set_map_colours_event_handler);
					break;
				case form_set_map_track:
					FrmSetEventHandler(frm, form_set_map_track_event_handler);
					break;
				case form_set_sua:
					FrmSetEventHandler(frm, form_set_sua_event_handler);
					break;
				case form_set_suadisp:
					FrmSetEventHandler(frm, form_set_suadisp_event_handler);
					break;
				case form_set_suawarn:
					FrmSetEventHandler(frm, form_set_suawarn_event_handler);
					break;
				case form_set_pilot:
					FrmSetEventHandler(frm, form_set_pilot_event_handler);
					break;
				case form_sat_status:
					FrmSetEventHandler(frm, form_sat_status_event_handler);
					break;
				case form_transfer:
					FrmSetEventHandler(frm, form_transfer_event_handler);
					break;
				case form_list_files:
					FrmSetEventHandler(frm, form_list_files_event_handler);
					break;
				case form_list_flts:
					FrmSetEventHandler(frm, form_list_flts_event_handler);
					break;
				case form_set_scrorder:
					FrmSetEventHandler(frm, form_set_scrorder_event_handler);
					break;
				case form_set_sms:
					FrmSetEventHandler(frm, form_set_sms_event_handler);
					break;
				// flight computer setup screens
				case form_config_caiinst:
					FrmSetEventHandler(frm, form_config_caiinst_event_handler);
					break;
				case form_config_gpsnavinst:
					FrmSetEventHandler(frm, form_config_gpsnavinst_event_handler);
					break;
				case form_config_flarminst:
					FrmSetEventHandler(frm, form_config_flarminst_event_handler);
					break;
				case form_config_recoinst:
					FrmSetEventHandler(frm, form_config_recoinst_event_handler);
					data.application.display_events[3].valid=VALID;
					data.application.display_events[3].event=RECOEvent;
					break;
				case form_config_ewmrinst:
					FrmSetEventHandler(frm, form_config_ewmrinst_event_handler);
					break;
				// Alert screens
				case form_genalert:
					FrmSetEventHandler(frm, form_genalert_event_handler);
					break;
				case form_wayselect_tr:
					FrmSetEventHandler(frm, form_wayselect_tr_event_handler);
					break;
				case form_wayselect_ta:
					FrmSetEventHandler(frm, form_wayselect_ta_event_handler);
					break;
				case form_wayselect_te:
					FrmSetEventHandler(frm, form_wayselect_te_event_handler);
					break;
				case form_question:
					FrmSetEventHandler(frm, form_question_event_handler);
					break;
				case form_warning:
					FrmSetEventHandler(frm, form_warning_event_handler);
					break;
				case form_set_mc:
					FrmSetEventHandler(frm, form_set_mc_event_handler);
					break;
			}
			handled = true;
			break;
		
		case menuEvent:
//			HostTraceOutputTL(appErrorClass, "menuEvent");
			PlayKeySound();
			lastkeywasmenu = false;
			settaskreadonly = false;
			exittaskreadonly = true;
			origform2 = form_final_glide;
			if (FrmGetActiveFormID() == form_set_port) {
				// Have to do this otherwise the BT search dialogs steal
				// the events from the FrmGotoForm call
				newEvent.eType = frmCloseEvent;
				form_set_port_event_handler(&newEvent);
			}
			switch (event->data.menu.itemID) {
				case menu_final_glide:
					FrmGotoForm(form_final_glide);
					break;
				case menu_list_polar:
					selectedPolarIndex = 0;
					FrmGotoForm(form_list_polar);
					break;
				case menu_set_units:
					FrmGotoForm(form_set_units);
					break;
				case menu_set_logger:
					FrmGotoForm(form_set_logger);
					break;
				case menu_set_port:
					FrmGotoForm(form_set_port);
					break;
				case menu_flt_info:
					FrmGotoForm(form_flt_info);
					break;
				case menu_moving_map:
					FrmGotoForm(form_moving_map);
					break;
				case menu_set_waypoints:
					if (FrmGetActiveFormID() == form_set_task) {
						origform = defaultscreen;
					}
					if ((data.task.numwaypts > 0) && tasknotfinished) {
						// task active
						if (taskonhold) savtaskonhold = true; else savtaskonhold = false;
						select_fg_waypoint(WAYTEMP);
					} else {
						// no task active
						select_fg_waypoint(WAYNORM);
					}
					break;
				case menu_wind_disp:
					FrmGotoForm(form_wind_disp);
					break;
				case menu_wind_3dinfo:
					FrmGotoForm(form_wind_3dinfo);
					break;
				case menu_thermal_hist:
					FrmGotoForm(form_thermal_history);
					break;
				case menu_set_map:
					FrmGotoForm(form_set_map);
					break;
				case menu_set_pilot:
					FrmGotoForm(form_set_pilot);
					break;
				case menu_set_fg:
					FrmGotoForm(form_set_fg);
					break;
				case menu_active_task:
					// always go to active task
					if (dispactive && (taskIndex == 0) && (FrmGetActiveFormID() == form_set_task)) {
						FrmUpdateForm(form_set_task, frmRedrawUpdateCode);
 					} else {
						dispactive = true;
						taskIndex = 0;
						FrmGotoForm(form_set_task);
					}
					break;
				case menu_active_rules:
					// always go to active task
					skipnewflight = true;
					if (dispactive && (taskIndex == 0) && (FrmGetActiveFormID() == form_task_rules)) {
						FrmUpdateForm(form_task_rules, frmRedrawUpdateCode);
 					} else {     	
						dispactive = true;
						exittaskreadonly = true;
						taskIndex = 0;
						origform = defaultscreen;
						FrmGotoForm(form_task_rules);
					}
					break;
				case menu_list_task:
					taskIndex = 0;
					FrmGotoForm(form_list_task);
					break;
				case menu_list_sua:
					selectedSUAListIdx = 0;
					SUAselectall = false;
					FrmGotoForm(form_list_sua);
					break;
				case menu_config_task:
					FrmGotoForm(form_config_task);
					break;
				case menu_transfer:
					FrmGotoForm(form_transfer);
					break;
				case menu_set_sua:
					FrmGotoForm(form_set_sua);
					break;
				case menu_set_qnh:
					FrmGotoForm(form_set_qnh);
					break;
				case menu_replay:
					io_type = IO_RECEIVE;
					io_file_type = IGC_FILE;
					FrmGotoForm(form_list_files);
					break;					
				case menu_set_scrorder:
					FrmGotoForm(form_set_scrorder);
					break;
				case menu_set_sms:
					FrmGotoForm(form_set_sms);
					break;
				case menu_sat_status:
					chain2 = true;
					gotoGPSinfo = true;
					FrmGotoForm(form_sat_status);
					break;
				case menu_exit:
					if (device.HiDensityScrPresent) {
						WinSetCoordinateSystem(kCoordinatesStandard);
					}
					question->type = QexitSP;
					FrmPopupForm(form_question);
					break;
			}
			handled = true;
			break; 

		case nilEvent:
//******************************************************************************
// start nilEvent
//******************************************************************************
//			HostTraceOutputTL(appErrorClass, "nil event %lu", cursecs%86400);
//			HostTraceOutputTL(appErrorClass, "Logger UTC %s", data.logger.gpsutc);
//			HostTraceOutputTL(appErrorClass, " Form %s", DblToStr(FrmGetActiveFormID(),0));

			// trigger default events if menu is open
			if (menuopen) {
				InuseWaypointCalcEvent();
				update_waypoint_list_values_event();
				check_sua_elements();
			}

			// get latest time
			updatetime = false;
			cursecs = TimGetSeconds();

			// update the battery status
			device.batpercent = Update_BatteryInfo();
			if (device.batpercent > LOWBATWARNLEVEL) {
				// clear low battery warning
				device.lowbatlevel = 99;
				device.lowbatcount = 0;
			}

			//check for NMEA input or file transfer
			if (data.parser.parser_func == nmea_parser) {
				curxfertype = data.config.nmeaxfertype;
			} else {
				curxfertype = data.config.xfertype;
			}

//******************************************************************************
// get data from GPS
//******************************************************************************
			// get NMEA data

			// IGC simulator mode?
			if (sim_gps) {

				double gpsvar=0.0;

				// get next record from sim database
				OpenDBQueryRecord(sim_db, sim_idx, &sim_hand, &sim_ptr);
				MemMove(&simpoint, sim_ptr, sizeof(SimPoint));
				MemHandleUnlock(sim_hand);

				// first point?
				if (sim_last_time == 0) {
					Int8 j=0;
					// remember time
					sim_last_time = simpoint.seconds;
					sim_time = cursecs;
					// only need to do this once
					StrCopy(data.logger.gpsstat, "A"); 	// valid fix (Active)
					StrCopy(data.input.gpsnumsats, "10");// fake valid satelites
					data.input.siu = 10;
									
					// valid 3D fix
					pvtData->status.fix = gpsFix3D;
					pvtData->status.mode = gpsModeSim;
					// fake GPS satellite data
					for (j=1; j<GPSMAXSATS-1; j++) {
						satData[j].svid = j+1;
						satData[j].status = 0;
						satData[j].snr = 3000.0 + ((SysRandom(0) * 1500.0) / sysRandomMax);
						satData[j].elevation = ((SysRandom(0) * 180.0)/sysRandomMax) * degToRad;
						satData[j].azimuth = ((SysRandom(0) * 180.0) /sysRandomMax) * degToRad;
					}
					// invalid date
					SecondsToDateOrTimeString(0, data.logger.gpsdtg, 4, 0);	
					// stop loging
					PerformLogging(true);
					// disable auto QFE zero as logger stopping was due to data loss, not being below log speed
					saveqfe = true;
					recv_data = false;
					recv_palt = false;
					checkfixtime = false;
					data.input.ground_speed.value = 0.0;
					Flarmpresent = false;
				}
				else
				{
					// todo: option for max speed playback or real time
					if ((cursecs - sim_time) >= (simpoint.seconds - sim_last_time))
					{						
						char latbuf[21],
							 lonbuf[21];
							 
						// remember times for next point
						sim_last_time = simpoint.seconds;
						sim_time = cursecs;
						
						// set logger GPS position
						MemSet(latbuf, sizeof(latbuf), 0);
						MemSet(lonbuf, sizeof(lonbuf), 0);
						// LLToStringDM(double coord, Char *coordStr, Boolean lat, Boolean colon, Boolean period, Int8 numaddplaces)
						LLToStringDM(simpoint.lat,latbuf,true,false,true,3);
						StrCopy(data.logger.gpslat, latbuf);
						LLToStringDM(simpoint.lon,lonbuf,false,false,true,3);
						StrCopy(data.logger.gpslng, lonbuf);
		//				HostTraceOutputTL(appErrorClass, "GPS simulator mode..lat=%s -> lat=%s", DblToStr(simpoint.lat,5),latbuf);
		//				HostTraceOutputTL(appErrorClass, "GPS simulator mode..lon=%s -> lon=%s", DblToStr(simpoint.lon,5),lonbuf);

						data.input.gpslatdbl = simpoint.lat;
						data.input.gpslngdbl = simpoint.lon;
						data.input.coslat = cos(DegreesToRadians(data.input.gpslatdbl));

						//Altitude in meters divided by ALTMETCONST to convert to feet.
						data.input.inusealt = 
						data.logger.pressalt = 
						data.logger.gpsalt = (double) simpoint.alt/ALTMETCONST;
						/* Save Maximum Altitude  in feet*/
						if (data.input.maxalt < data.logger.gpsalt) {
							data.input.maxalt = data.logger.gpsalt;
						}
						/* Save Minimum Altitude  in feet*/
						if (data.input.minalt > data.logger.gpsalt) {
							data.input.minalt = data.logger.gpsalt;
						}

						// Have to convert from km/h to knots
						data.input.ground_speed.value=(double)simpoint.speed*AIRKMHKNCONST;	
						data.input.ground_speed.valid=VALID;
						// heading
						data.input.true_track.value=simpoint.heading;
						data.input.true_track.valid=VALID;
						
						gpsvar = GetDeviation();
						SecondsToDateOrTimeString(simpoint.seconds, data.logger.gpsutc, 2, 0);

						CalcLift(data.logger.gpsalt, data.logger.gpsutc, -9999.9, NORESET);

						data.input.magnetic_track.value = nice_brg(data.input.true_track.value);
						data.input.magnetic_track.valid=VALID;
		
						// next record
						sim_idx++;
						// done with replay?
						if (sim_idx >= sim_rec) {
							// exit sim mode, NO GPS will show eventually
							sim_gps = false;
							StrCopy(data.logger.gpsstat, "V"); 	// invalid fix
						}
					}
				}
				
				readtime = cursecs;
				recv_data = true;
				no_read_count = cursecs;
				nogpstime = 0;
				nofixtime = cursecs;
				checkfixtime = true;
				
//				HostTraceOutputTL(appErrorClass, "GPS sim.mode...cursec=%lu, simpoint=%ld, delta=%lu, next=%ld, sim_last_time=%lu", 
//									cursecs, 
//									simpoint.seconds, 
//									cursecs - sim_time,
//									simpoint.seconds - sim_last_time,
//									sim_last_time);
				
//				HostTraceOutputTL(appErrorClass, "GPS simulator mode...%ld of %ld", sim_idx, sim_rec);
//				HostTraceOutputTL(appErrorClass, "GPS simulator mode....lat=%s", DblToStr(simpoint.lat,5));
//				HostTraceOutputTL(appErrorClass, "GPS simulator mode....lon=%s", DblToStr(simpoint.lon,5));
//				HostTraceOutputTL(appErrorClass, "GPS simulator mode....alt=%s", DblToStr(simpoint.alt,5));
//				HostTraceOutputTL(appErrorClass, "GPS simulator mode..speed=%s", DblToStr(simpoint.speed,5));
				
				updatemap = true;
				updatetime = true;
				updatewind = true;
				
				data.application.changed = 1;										
			}				
			else // normal mode read GPS data
			{
				if (device.iQueCapable && data.parser.parser_func == nmea_parser && (data.config.nmeaxfertype == USEIQUE || data.config.nmeaxfertype == USEIQUESER)) {
	//				HostTraceOutputTL(appErrorClass, "Reading iQue Data");
					GetiQueInfo();
					readtime = cursecs;
	//				HostTraceOutputTL(appErrorClass, "Setting recv_data=true");
					recv_data = true;
					no_read_count = cursecs;
					nogpstime = 0;
					device.BTreconnects = 0;
					if (StrCompare(data.logger.gpsstat, "A") == 0) {
						nofixtime = cursecs;
						checkfixtime = true;
					} else {
						checkfixtime = false;
					}
					if (data.config.nmeaxfertype == USEIQUESER) {
						while(RxData(curxfertype)) {
	//						HostTraceOutputTL(appErrorClass, "Reading Serial Data");
						}
					}
				}
				else
				{
					while(RxData(curxfertype))
					{
						readtime = cursecs;
	//					HostTraceOutputTL(appErrorClass, "Setting recv_data=true");
						recv_data = true;
						no_read_count = cursecs;
						nogpstime = 0;
						device.BTreconnects = 0;
						if (StrCompare(data.logger.gpsstat, "A") == 0)
						{
							nofixtime = cursecs;
							checkfixtime = true;
						}
						else
						{
							checkfixtime = false;
						}
					}
				}
			}
			// check for newly valid GPS data, if so calc distance and bearing for all waypoints
			// plus task sector directions if task is active
			if (checkfixtime) {
				if (!GPSdatavalid) {
//					HostTraceOutputTL(appErrorClass, "Calc All Waypoints");
					calcallwaypoints(data.input.gpslatdbl, data.input.gpslngdbl);
					if ((data.task.numwaypts > 0) && !taskonhold && (activetskway >= 0)) {
						CalcStartFinishDirs(&data.task);
					}
					GPSdatavalid = true;
					hadGPSdatavalid = true;
				}
			} else {
				GPSdatavalid = false;
			}

			// Will play the No GPS Sound if no data for 10 secs until the logger closes the log due to loss of data
			if ((cursecs > no_read_count+10) && inflight && data.config.logonoff) {
				if (nogpstime == 0) nogpstime = cursecs;
//				HostTraceOutputTL(appErrorClass, "Playing NO GPS Sound");
//				HostTraceOutputTL(appErrorClass, "Time from no GPS data %s", DblToStr(((cursecs - nogpstime) % 60),0));
				// repeat every 45 secs for 5 secs
				if (((cursecs - nogpstime) % 45) < 5) PlayNoGPSSound();
				// update final glide GPS status
				if (FrmGetActiveFormID() == form_final_glide) {
					FntSetFont(boldFont);
					WinDrawInvertedChars(" NO GPS", 7, GPSX, GPSY);
					FntSetFont(stdFont);
				}
				// if using Bluetooth attempt re-connection
				if ((device.lowbatlevel > 0) && device.BTCapable && (data.config.nmeaxfertype == USEBT)) {
					if (device.BTreconnects < 3) {
						// display alert
						xfrdialog = XFRRECONNECT;
						device.BTreconnects++;
						HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, NULL);
						HandleWaitDialogUpdate(UPDATEDIALOG, device.BTreconnects, -1, "attemps");
						XferClose(data.config.nmeaxfertype);
						// clear alert
						HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
						PlayNoGPSSound();
						// attempt connection
						SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
						XferInit(tempchar, NFC, data.config.nmeaxfertype);
					} else if (device.BTreconnects == 3) {
						PlayNoGPSSound();
						device.BTreconnects = 99;
						warning->type = Werror;
						StrCopy(warning->line1, "Unable to Re-connect");
						StrCopy(warning->line2, "to Bluetooth!");
						FrmPopupForm(form_warning);
					}
				}
			}
			
//			HostTraceOutputTL(appErrorClass, "nodatatime=|%hd|", data.config.nodatatime);
			// close flight log if no data received for data timeout period
			// or no valid GPS fix for data timeout period
			if ((cursecs > (readtime + data.config.nodatatime)) || 	
			    (checkfixtime && (cursecs > (nofixtime + data.config.nodatatime))) ) { // (data.config.logstoptime > data.config.nodatatime ? data.config.logstoptime : data.config.nodatatime)))) ) { 
				// stop loging
				PerformLogging(true);
				// disable auto QFE zero as logger stopping was due to data loss, not being below log speed
				saveqfe = true;
//				HostTraceOutputTL(appErrorClass, "Setting recv_data=false");
				recv_data = false;
				recv_palt = false;
				checkfixtime = false;
				data.input.ground_speed.value = 0.0;
				Flarmpresent = false;
			} else {
				PerformLogging(false);
			}

			// get/update current time
			if (updatetime) {
				// get time from GPS
				StringToDateAndTime(data.logger.gpsdtg, data.logger.gpsutc, &gpstime);
				utcsecs = TimDateTimeToSeconds(&gpstime);
				TimAdjust(&gpstime, ((Int32)data.config.timezone * 3600 ));
				if (gpstime.year < 2000) gpstime.year = 2000;   // fixes issue with IGC files played back from SeeYou or IGC replay sim mode
				gpssecs = TimDateTimeToSeconds(&gpstime);
				// update Palm time if required
				if (data.config.usegpstime && ((cursecs>gpssecs+10) || (cursecs<gpssecs-10))) {
					//Update_PalmTime();
					TimSetSeconds(gpssecs);
					cursecs = TimGetSeconds();
					// check for GPS time update
					if (nofixtime > cursecs) nofixtime = cursecs;
					if (readtime > cursecs) readtime = cursecs;
					if (no_read_count > cursecs) no_read_count = cursecs;

				}
#ifdef POSITIONLOG
				// output latitude, longitude, altitude to single line file
				if (device.VFSCapable) {
					XferInit("position.gps", IOOPENTRUNC, USEVFS);
					//StrCopy(tempchar, "Position: Lat, Lon, Alt");
					//outputlog(tempchar, true);
					StrCopy(tempchar, DblToStr(data.input.gpslatdbl,5));
					StrCat(tempchar, ",");
					StrCat(tempchar, DblToStr(data.input.gpslngdbl,5));
					StrCat(tempchar, ",");
					StrCat(tempchar, DblToStr(data.input.inusealt*ALTMETCONST,1));
					StrCat(tempchar, ",");
					StrCat(tempchar, DblToStr(data.input.ground_speed.value*SPDKPHCONST,1));
					StrCat(tempchar, ",");
					StrCat(tempchar, DblToStr(data.input.magnetic_track.value,1));
					outputlog(tempchar, true);
					XferClose(USEVFS);
				}
#endif
			}

//******************************************************************************
// re-calculate wind
//******************************************************************************

// for testing only
//			recv_data = true;
//			updatetime = true;
//			inflight = true;
//			check_sua_elements();
//			data.logger.gpsalt = 264/ALTMETCONST;
//			data.logger.pressalt = data.input.inusealt;
//			data.input.deviation.valid = VALID;
//			data.input.deviation.value = 45.0;

			// May want to eventually use the thavglift value to set the basemc and
			// send it back to the 302 as well.
			if (updatewind) {
				speed = data.input.ground_speed.value;
				updatewind = false;

				if (data.config.usetas && (data.input.true_airspeed.valid==VALID))
					speed -= data.input.true_airspeed.value;

				if (CalcWind(data.input.magnetic_track.value, speed, &data.input.wnddir, &data.input.wndspd, data.config.useinputwinds, false)) {
					// left thermal
					if (data.config.setmcval) {
						// set final MC average from "THERMALSTOAVG" thermals (set in soarWind.h)
//						HostTraceOutputTL(appErrorClass, "Calc MC Final");
						if (data.input.thavglift >= 0.0) {
							lastthermal[THERMALSTOAVG-1].avglift = data.input.thavglift;
						} else {
							lastthermal[THERMALSTOAVG-1].avglift = 0.0;
						}
						data.input.newmc = 0.0;
						totalturns = 0;
						for (i = THERMALSTOAVG-1; i >= 0; i--) {
//							HostTraceOutputT(appErrorClass, " %s", DblToStr(lastthermal[i].avglift,2));
							data.input.newmc += lastthermal[i].avglift * lastthermal[i].turns;
							totalturns += lastthermal[i].turns;
						}
//						HostTraceOutputTL(appErrorClass, " "); 
						if (totalturns > 0) data.input.basemc = data.input.newmc / totalturns;
						data.input.newmc = data.input.basemc;
//						HostTraceOutputTL(appErrorClass, "New MC Value %s", DblToStr(data.input.basemc,2));
					}
					wasinthermal = false;
				}

				if (thmode == THERMAL) {
					if (!wasinthermal) {
						// just entered the thermal
						wasinthermal = true;
					} else {
						// thermalling
						if (data.config.dynamictrkcolour) {
							if (data.input.thavglift >= 0.0) {
								data.input.newmc = data.input.thavglift;
							} else {
								data.input.newmc = 0.0;
							}
						}
					}
				}
			}

			if (updatetime && data.config.usecalchw && recv_data) {
				CalcHWind(data.input.magnetic_track.value, data.input.wnddir, data.input.wndspd, &data.input.headwind);
			}

//******************************************************************************
// get altitude data from GPS and/or supported loggers
// also MC, bugs, ballast data
//******************************************************************************
			// update altitude
			if (data.config.pressaltsrc == C302ALT ) {
				// If using the C302, use the units True Altitude rather than
				// calculating one from pressure altitude and the qnh field value
				data.input.inusealt = data.input.comptruealt;
			} else if (data.config.usepalt) {
				// use pressure altitude corrected with QNH setting
				data.input.inusealt = data.logger.pressalt - data.input.QNHaltcorrect;
			} else {
				data.input.inusealt = data.logger.gpsalt;
			}

			// calculate true airspeed from ground speed and headwind
			if (!data.config.usetas || (data.input.true_airspeed.valid==NOT_VALID)) {
				// mark as not valid ie: calculated not from vario
				data.input.true_airspeed.value = data.input.ground_speed.value + data.input.headwind;
				data.input.true_airspeed.valid = NOT_VALID;
			}

			// If B50 or C302 data selected, update relevant fields
//			HostTraceOutputTL(appErrorClass, "About to update relevant fields");
			if (data.config.flightcomp == C302COMP ||
			    data.config.flightcomp == B50COMP  ||
			    data.config.flightcomp == B50VLKCOMP  ||
			    data.config.flightcomp == B50LXCOMP) {
				if (!data.config.setmcval && (pround(data.input.basemc, 1) != pround(data.input.compmc, 1))) {
					if (!skipmc) {
						data.input.basemc = data.input.compmc;
						data.input.newmc = data.input.basemc;
//						HostTraceOutputTL(appErrorClass, "About to update basemc field");
						if (FrmGetActiveFormID() == form_final_glide) {
							// This converts to the selected lift units
							if (data.config.lftunits == STATUTE) {
								tempdbl = pround(data.input.basemc*data.input.lftconst, data.input.lftprec);
								remainder = pround(Fmod(tempdbl, data.input.lftincr), data.input.lftprec);
								if (remainder > (data.input.lftincr/2.0)) {
									tempdbl = tempdbl - remainder + data.input.lftincr;
								} else {
									tempdbl = tempdbl - remainder;
								}
								field_set_value(form_final_glide_basemc, DblToStr(pround(tempdbl, data.input.lftprec), data.input.lftprec));
							} else {
								field_set_value(form_final_glide_basemc, DblToStr(pround(data.input.basemc*data.input.lftconst, data.input.lftprec), data.input.lftprec));
							}
						}
//						HostTraceOutputTL(appErrorClass, "Done updating basemc field fields");
					} else {
//						HostTraceOutputTL(appErrorClass, "Skipping mc");
					}
				} else {
//					HostTraceOutputTL(appErrorClass, "skipmc = false");
					skipmc = false;
				}

				// This is done because the 302 starts with a bug value of 0
				if (data.input.bugs == 0.0) {
					data.input.bugs = 1.0;
					data.config.bugfactor = data.input.bugs;
					if (data.config.flightcomp == C302COMP) {
						Output302GRec(GBUGS);
					}
				}
//				HostTraceOutputTL(appErrorClass, "bugfactor1");

				if (data.config.bugfactor != data.input.bugs) {
					if (!skipbugs) {
//						HostTraceOutputTL(appErrorClass, "setting bugfactor = bugs");
						data.config.bugfactor = data.input.bugs;
						if (FrmGetActiveFormID() == form_set_fg)
							field_set_value(form_set_fg_bugfactor, DblToStr(data.config.bugfactor*100.0,0));
					} else {
//						HostTraceOutputTL(appErrorClass, "Skipping bugs");
					}
				} else {
					skipbugs = false;
				}

				if (data.config.pctwater != data.input.ballast) {
					if (!skipballast) {
						data.config.pctwater = data.input.ballast;
						if (FrmGetActiveFormID() == form_set_fg)
							field_set_value(form_set_fg_pctwater, DblToStr(data.config.pctwater*100.0,0));
					} else {
//						HostTraceOutputTL(appErrorClass, "Skipping ballast");
					}
				} else {
					skipballast = false;
				}
			}

			// Centralized Function to update the MCCurVal global variable with the most recent information
			SetMCCurVal();

//******************************************************************************
// re-calcution inuseWaypoint, task, terrain etc
//******************************************************************************

// for testing only
//			data.input.inusealt = 650 / ALTMETCONST;
//			data.input.deviation.value = 3.0;
//			HostTraceOutputTL(appErrorClass, "QNH prs: %s",DblToStr(data.input.QNHpressure,0));
//			HostTraceOutputTL(appErrorClass, "GPS Alt: %s",DblToStr(data.logger.gpsalt,0));
//			HostTraceOutputTL(appErrorClass, "Std Alt: %s",DblToStr(data.logger.pressalt,0));
//			HostTraceOutputTL(appErrorClass, "GPS Lat: %s",DblToStr(data.input.gpslatdbl,3));
//			HostTraceOutputTL(appErrorClass, "GPS Lon: %s",DblToStr(data.input.gpslngdbl,3));
//			for (x=0; x<WINDPROFILEPOINTS; x++) {
//				windprofile[x].direction = 120.0;
//				windprofile[x].speed = 20.0;
//			}
//			data.input.true_track.value = 200.0;
//			data.input.true_track.valid = VALID;

			// handle task if active, or single waypoint
//			if (updatetime) { // new NMEA data received
				if ((data.task.numwaypts > 0) && !taskonhold && (activetskway >= 0)) {
//					HostTraceOutputTL(appErrorClass, "Handle Task");
					HandleTask(TSKNORMAL);
				} else if (inflight && ((taskonhold) || (!data.config.taskonoff))) {
//					HostTraceOutputTL(appErrorClass, "Handle Single Wpt");
					if (data.input.bearing_to_destination.valid==VALID) {
						if (data.input.distance_to_destination.value <= data.config.turncircrad) {
							if (!reachedwaypoint) {
								PlayTurnSound();
								reachedwaypoint = true;
							}
						} else if (reachedwaypoint) {
							reachedwaypoint = false;
						}
					}
				}
//			}

			// update terrain elevation array if required
			data.input.terelev = GetTerrainElev(data.input.gpslatdbl, data.input.gpslngdbl);
			if (terrainpresent && terrainvalid) {
//				HostTraceOutputTL(appErrorClass, "Checking Terrain Array");
				if (data.config.usefgterrain && (data.input.bearing_to_destination.valid==VALID) && (data.input.distance_to_destination.value > 0.0)) {
					// load dynamic array of terrain heights when required
					// ie: moved more than about 1km in distance OR bearing
					numwptter = (Int32)(data.input.distance_to_destination.value * TERGRID)+1;
						if ((numwptter != prevnumwptter) || (Fabs(data.input.bearing_to_destination.value - prevterbear) > (360/PI)/data.input.distance_to_destination.value)) {
						if ((prevnumwptter > 0) && (wptterheights != NULL)) {
							// free previous array
//							HostTraceOutputTL(appErrorClass, "Free Terrain Heights %s", DblToStr(prevnumwptter,0));
							FreeMem((void *)&wptterheights);
							wptterheights = NULL;
						}
						if (numwptter > 0) {
							// allocate new size array
//							HostTraceOutputTL(appErrorClass, "Alloc Terrain Heights %s", DblToStr(numwptter,0));
							if (AllocMem((void *)&wptterheights, (numwptter+1)*sizeof(double))) {
								MemSet(wptterheights, (numwptter+1)*sizeof(double), 0);
								prevnumwptter = numwptter;
								prevterbear = data.input.bearing_to_destination.value;
								// load terrain heights
								offterrain = !loadterrain(wptterheights, numwptter, data.input.gpslatdbl, data.input.gpslngdbl, 0, data.input.destination_lat, data.input.destination_lon, data.input.destination_elev);
							} else {
								// AllocMem for terrain failed
								offterrain = true;
							}
						}
						data.application.changed = 1;
					}
				}

				if (terrainpresent && terrainvalid && data.config.usefgterrain && data.input.destination_valid && !offterrain) {
					// check for terrain crash en-route to destination
					crash1 = false;
					CalcSTFSpdAlt2(MCCurVal, data.input.distance_to_destination.value, data.input.bearing_to_destination.value, &speed, &tmpalt);
					tmpalt = ConvertAltType(data.input.destination_elev, data.input.inusealt, true, ARVALT, tmpalt);
					wptcrash = terraincrash(wptterheights, numwptter, data.input.gpslatdbl, data.input.gpslngdbl, data.input.destination_lat, data.input.destination_lon,
								data.input.inusealt, tmpalt+data.input.destination_elev, true);
					if (wptcrash) {
//						HostTraceOutputTL(appErrorClass, "Inusewaypt Terrain Crash");
						wptcrashalt = crashalt;
						wptcrashlat = crashlat;
						wptcrashlon = crashlon;
					}
					// check FGA task terrain crash
					if ((data.task.numwaypts > 0) && !taskonhold && (activetskway >= 0)) {
						tskcrash = tskterraincrash(tskterheights, activetskway, tmpalt+data.input.destination_elev);
					}
					if (tskcrash) {
//						HostTraceOutputTL(appErrorClass, "Task Terrain Crash");
						tskcrashalt = crashalt;
						tskcrashlat = crashlat;
						tskcrashlon = crashlon;
					}
				} else {
					crash = false;
					crash1 = false;
					crashalt = 0.0;
					wptcrash = false;
					wptcrashalt = 0.0;
					tskcrash = false;
					tskcrashalt = 0.0;
				}

			} else {
				// not on terrain map
				offterrain = true;
				crash = false;
				crash1 = false;
				crashalt = 0.0;
				wptcrash = false;
				wptcrashalt = 0.0;
				tskcrash = false;
				tskcrashalt = 0.0;
			}

			// get FGA altitude and adjust for terrain crash
			if ((data.task.numwaypts > 0) && !taskonhold && (activetskway >= 0)) {
				data.input.FGAdispalt = data.activetask.FGAalt;
				if (tskcrash && !offterrain && !tskoffterrain) {
					switch (data.config.alttype) {
						case REQALT:
							data.input.FGAdispalt = data.input.inusealt - tskcrashalt;
							break;
						case ARVALT:
							// if crash is more than safety height, adjust AAlt figure
							if (tskcrashalt + data.config.safealt < 0) {
								data.input.FGAdispalt = tskcrashalt + data.config.safealt;
							}
							break;
						case DLTALT:
							data.input.FGAdispalt = tskcrashalt;
							break;
						default:
							break;
					}
				}
				// check if above final glide height
				if (data.config.FGalert) {
					aboveFG = false;
					switch (data.config.alttype) {
						case REQALT:
							if (data.input.FGAdispalt < data.input.inusealt) aboveFG = true;
							break;
						case ARVALT:
							if (data.input.FGAdispalt > data.config.safealt) aboveFG = true;
							break;
						case DLTALT:
							if (data.input.FGAdispalt > 0) aboveFG = true;
							break;
					}
					if (aboveFG && (data.logger.thermal == THERMAL)) {
						// alert final glide altitude achieved while thermalling
						if (data.activetask.FGstatus < ALERT_FG) {
							warning->type = Winfo;
							StrCopy(warning->line1, "Above Final Glide Height");
							StrCopy(warning->line2, "");
							FrmPopupForm(form_warning);
							data.activetask.FGstatus = ALERT_FG;
						}
					} else {
						data.activetask.FGstatus = BELOW_FG;
					}
				}
			}

			// clear the next waypoint data
			data.input.isctlpt = false;
			nextwptset = false;
			data.input.nextwpt_dist = -1.0;
			data.input.nextwpt_bear = 0.0;
			StrCopy(data.input.nextwpt_text,"None");

			// update next waypoint information, if required
			if ((data.config.shownextwpt != NEXTOFF) && ((FrmGetActiveFormID() == form_final_glide) || (FrmGetActiveFormID() == form_moving_map)|| (FrmGetActiveFormID() == form_waypoint_sector))) {
				// check which next waypoint data to show
				if ((activetskway < 0) || (data.task.numwaypts <= 0)) {
					// no task active
					if (data.input.destination_valid) {
						// single waypoint active so point to home waypoint
						data.input.nextwpt_lat = data.input.homeLat;
						data.input.nextwpt_lon = data.input.homeLon;
						LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.input.homeLat, data.input.homeLon, &data.input.nextwpt_dist, &data.input.nextwpt_bear);
						StrCopy(data.input.nextwpt_text,"Home");
						nextwptset = true;
					}
				} else {
					// if inside area point to target or max distance point
					z = activetskway-1;
					while (data.task.waypttypes[z] & CONTROL) z--;
					if ((activetskway > 0) && (data.task.waypttypes[z] & AREA)) {
						// check inside an area
						if ((inareasector > -1)) {
							// calculate max distance point in area
							CalcAreaMax(&data.task, z, &data.input.areamaxlat, &data.input.areamaxlon, false);
//							if (data.config.AATmode == AAT_MTURN_ON) {
//								// always point to max point in manual turn mode
//								data.input.nextwpt_lat = data.input.areamaxlat;
//								data.input.nextwpt_lon = data.input.areamaxlon;
//								LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.input.areamaxlat, data.input.areamaxlon, &data.input.nextwpt_dist, &data.input.nextwpt_bear);
//								StrCopy(data.input.nextwpt_text,"Max");
//								nextwptset = true;
//							} else {
								// calculate distance from position in area to previous point, skipping control points
								x = z-1;
								while (data.task.waypttypes[x] & CONTROL) x--;
								LatLonToRange(data.task.distlats[x], data.task.distlons[x], data.task.targetlats[z], data.task.targetlons[z], &rng1);
								LatLonToRange(data.task.distlats[x], data.task.distlons[x], data.input.gpslatdbl, data.input.gpslngdbl, &rng3);
								// calculate distance from position in area to next point, skipping control points
								y = activetskway;
								while (data.task.waypttypes[y] & CONTROL) y++;
								LatLonToRange(data.task.targetlats[z], data.task.targetlons[z], data.task.distlats[y], data.task.distlons[y], &rng2);
								LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, data.task.distlats[y], data.task.distlons[y], &rng4);
								// compare ranges
								if (rng1 + rng2 > rng3 + rng4) {
									data.input.nextwpt_lat = data.task.targetlats[z];
									data.input.nextwpt_lon = data.task.targetlons[z];
									LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.targetlats[z], data.task.targetlons[z], &data.input.nextwpt_dist, &data.input.nextwpt_bear);
									StrCopy(data.input.nextwpt_text,"Tgt");
									// do not show target for manual AAT mode
									if (data.config.AATmode < AAT_MTURN) {
//										HostTraceOutputTL(appErrorClass, "AAT TGT to %s", data.task.wayptnames[z]);
										nextwptset = true;
									}
								} else {
									data.input.nextwpt_lat = data.input.areamaxlat;
									data.input.nextwpt_lon = data.input.areamaxlon;
									LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.input.areamaxlat, data.input.areamaxlon, &data.input.nextwpt_dist, &data.input.nextwpt_bear);
									StrCopy(data.input.nextwpt_text,"Max");
									// do not show max for manual AAT mode when on
									if (data.config.AATmode != AAT_MTURN) {
//										HostTraceOutputTL(appErrorClass, "AAT MAX to %s", data.task.wayptnames[z]);
										nextwptset = true;
									}
								}
//							}
							data.input.isctlpt = true;
						}
					}

					// check if next waypoint is an area waypoint, if yes point to min distance point
					if (!nextwptset && (data.task.waypttypes[activetskway] & AREA) && (data.config.AATmode != AAT_MTURN_ON)) {
						CalcAreaMin(&data.task, activetskway,  &data.input.areaminlat, &data.input.areaminlon, false);
						data.input.nextwpt_lat = data.input.areaminlat;
						data.input.nextwpt_lon = data.input.areaminlon;
						LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.input.areaminlat, data.input.areaminlon, &data.input.nextwpt_dist, &data.input.nextwpt_bear);
						data.input.isctlpt = true;
						StrCopy(data.input.nextwpt_text,"Min");
//						HostTraceOutputTL(appErrorClass, "AAT Min to %s", data.task.wayptnames[activetskway]);
						nextwptset = true;
					}

					// otherwise point to next waypoint in task
					if (!nextwptset) {
						// check task waypoint is not the last point
						if (activetskway < (Int16)data.task.numwaypts-1) {
							// check for AAT mode
							if (data.config.AATmode == AAT_MTURN_ON) z = 0; else z = 1;
							data.input.nextwpt_lat = data.task.targetlats[activetskway+z];
							data.input.nextwpt_lon = data.task.targetlons[activetskway+z];
							LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.task.targetlats[activetskway+z], data.task.targetlons[activetskway+z], &data.input.nextwpt_dist, &data.input.nextwpt_bear);
							if (data.task.waypttypes[activetskway+z] & CONTROL) {
								data.input.isctlpt = true;
								StrCopy(data.input.nextwpt_text,"Ctl");
//								HostTraceOutputTL(appErrorClass, "Next Ctl Wpt %s", data.task.wayptnames[activetskway+z]);
							} else {
								StrCopy(data.input.nextwpt_text,"Task");
//								HostTraceOutputTL(appErrorClass, "Next Task Wpt %s", data.task.wayptnames[activetskway+z]);
							}
							nextwptset = true;
						}
					}
				}
				// Convert next waypoint bearing to Magnetic
				data.input.nextwpt_bear = nice_brg(data.input.nextwpt_bear+data.input.deviation.value);
			}

			// draw the moving map or waypoint sector screens
			// placed here so done after any updates in this nil event			
			if ((FrmGetActiveFormID() == form_moving_map) && !menuopen) {
				if (updatemap) {
					// This is used to set which map orientation mode is being used
					// depending on whether it is in THERMAL or CRUISE mode
					if (mapmode == THERMAL) {
						data.input.curmaporient = data.config.thmaporient;
					} else {
						data.input.curmaporient = data.config.maporient;
					}
					UpdateMap2(false, data.input.gpslatdbl, data.input.gpslngdbl, curmapscale, true, &data.task, true);
				}
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(device.HiDensityScrPresent);
				}
				if (!draw_log) {
					// update GPS status on moving map
					if (!recv_data || (nogpstime > 0)) {
						FntSetFont(largeBoldFont);
						if (data.config.btmlabels) {
							WinDrawInvertedChars("  No GPS Data!  ", 16, WIDTH_MIN+(SCREEN.SRES*42), HEIGHT_MIN+(SCREEN.SRES*141));
						} else {
							WinDrawInvertedChars("  No GPS Data!  ", 16, WIDTH_MIN+(SCREEN.SRES*42), HEIGHT_MIN+(SCREEN.SRES*133));
						}
						FntSetFont(stdFont);
					} else if (StrCompare(data.logger.gpsstat, "V") == 0) {
						FntSetFont(largeBoldFont);
						if (data.config.btmlabels) {
							WinDrawInvertedChars("  No GPS Sats!  ", 16, WIDTH_MIN+(SCREEN.SRES*42), HEIGHT_MIN+(SCREEN.SRES*141));
						} else {
							WinDrawInvertedChars("  No GPS Sats!  ", 16, WIDTH_MIN+(SCREEN.SRES*42), HEIGHT_MIN+(SCREEN.SRES*133));
						}
						FntSetFont(stdFont);
					}
				}
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(kCoordinatesStandard);
				}
			}

			// draw zoomed sector and information
			if ((FrmGetActiveFormID() == form_waypoint_sector) && !menuopen) {
				if (updatemap) {
					// This is used to set which map orientation mode is being used
					// depending on whether it is in THERMAL or CRUISE mode
					if (activetasksector && (selectedTaskWayIdx <= activetskway)) {
						if (mapmode == THERMAL) {
							data.input.curmaporient = data.config.thmaporient;
						} else {
							data.input.curmaporient = data.config.sectormaporient;
						}
					} else {
						// otherwise north up by default
						data.config.sectormaporient = NORTHUP;
						data.input.curmaporient = NORTHUP;
					}
					UpdateMap2(false, sectorlat, sectorlon, sectorscale, false, tsktoedit, false);
				} 
			}

//******************************************************************************
// send commands to supported loggers
//******************************************************************************
			// check for command to be sent to the logger
			if (CompCmd != DECNOCMD) {
				switch (CompCmd) {
					case DECQUESTION:
						CompCmd = DECNOCMD;
						// check if pilot wants to declare task to logger
						if (data.config.declaretasks &&
						   ((data.config.flightcomp == EWCOMP)     ||
						    (data.config.flightcomp == FLARMCOMP)  ||
						    (data.config.flightcomp == C302COMP)   ||
						    (data.config.flightcomp == C302ACOMP)  ||
						    (data.config.flightcomp == GPSNAVCOMP) ||
						    (data.config.flightcomp == VOLKSCOMP)  ||
						    (data.config.flightcomp == B50VLKCOMP) ||
						    (data.config.flightcomp == LXCOMP)     ||
						    (data.config.flightcomp == B50LXCOMP)  ||
						    (data.config.flightcomp == EWMRCOMP)   ||
						    (data.config.flightcomp == EWMRSDCOMP) ||
						    (data.config.flightcomp == EWMRSDTASCOMP) ||
						    (data.config.flightcomp == FILSERCOMP))) {
							question->type = Qdeclaretask;
							FrmPopupForm(form_question);
						}
						break;
					case DECSEND:
						CompCmd = DECNOCMD;
						// declare task to supported loggers
						DeclareTaskToLogger(DECSEND);
						break;
					case CLRQUESTION:
						CompCmd = DECNOCMD;
						if ((data.config.flightcomp == EWCOMP)      ||
						    (data.config.flightcomp == FLARMCOMP)   ||
						    (data.config.flightcomp == VOLKSCOMP    ||
						    (data.config.flightcomp == B50VLKCOMP))) {
							// clear task to supported loggers
							question->type = Qcleardec;
							FrmPopupForm(form_question);
						}
						break;
					case DECCLEAR:
						CompCmd = DECNOCMD;
						// clear declared task to supported loggers
						DeclareTaskToLogger(DECCLEAR);
						break;
					default:
						break;
				}
			}

 //******************************************************************************
// finish nilEvent
//******************************************************************************
			// clear map update flag
			updatemap = false;
//			HostTraceOutputTL(appErrorClass, "Returning from nil event");
			break;
		default:
			break;
	}
	return handled;
}

#define menuChrT3		0x510
#define calcChrT3		0x511
#define findChrT3		0x512
//#define hsChrVolumeUp		0x161B
//#define hsChrVolumeDown	0x161C

/**
* \fn PreprocessEvent(EventPtr event)
* \brief handle command buttons
* \return true in case event is handled, else false
*/
Boolean PreprocessEvent(EventPtr event)
{
	Boolean handled=false;
	double tempdbl, tempscale;
	UInt16 frmID = FrmGetActiveFormID();
	EventType newEvent;
	static Boolean performaction = false;
	static UInt32 pwrpresstime = 0;
	Char tempchar[20];
	MemHandle mem_hand;
	MemPtr mem_ptr;
	Int16 minsector;
	UInt8 Flarmmaxrange;
	UInt16 i, tempscr;
	Boolean chgwpt;

	if (menuopen) {
		// still need to handle power button and auto off even with menu open
		switch (event->data.keyDown.chr) {
			case hardPowerChr:
				PlayKeySound();
				if ((performaction) && ((pwrpresstime+2)>=TimGetSeconds())) {
					EvtEnqueueKey (vchrBacklight, 0, commandKeyMask);
					performaction = false;
				} else {
					performaction = true;
					pwrpresstime = TimGetSeconds();
				}
				handled = true;
				break;
			case autoOffChr: // Palm issuing auto turn off key
				handled = true;
				break;
		}
		return(handled);
	}

	switch (event->eType) {
		case menuOpenEvent:
//			HostTraceOutputTL(appErrorClass, "menuOpenEvent");
			break;
		case keyDownEvent:
//			HostTraceOutputTL(appErrorClass, "keyDownEvent chr char - |%c|", event->data.keyDown.chr);
//			HostTraceOutputTL(appErrorClass, "keyDownEvent chr UInt16 - |%hu|", event->data.keyDown.chr);
//			HostTraceOutputTL(appErrorClass, "keyDownEvent chr UInt16 hex- |%hX|", event->data.keyDown.chr);
//			HostTraceOutputTL(appErrorClass, "keyDownEvent keyCode Number UInt16 - |%hu|", event->data.keyDown.keyCode);
//			HostTraceOutputTL(appErrorClass, "keyDownEvent keyCode Number UInt16 hex- |%hX|", event->data.keyDown.keyCode);

			// checks for Palm 5-way Navigator presses - tested on Tungsten T & Vx
			// event->data.keyDown.chr values
			// 301 press
			// 1283 release or hold
			// event->data.keyDown.keyCode values
			// Left : Press 1028, Release 1024, Hold 4
			// Center : Press 4112, Release 4096, Hold 16
			// Right : Press 2056, Release 2048, Hold 8

			// changes event->data.keyDown.chr to rocker character codes
			if (event->data.keyDown.chr == 301) { // press code
				switch (event->data.keyDown.keyCode) {
					case 1028:
						event->data.keyDown.chr = vchrRockerLeft;
						event->data.keyDown.keyCode = 0;
						break;
					case 2056:
						event->data.keyDown.chr = vchrRockerRight;
						event->data.keyDown.keyCode = 0;
						break;
					default:
						break;
				}
			}

			if (event->data.keyDown.chr == 1283) { // release code
				switch (event->data.keyDown.keyCode) {
					case 4096:
						if (!lastkeywasmenu) {
							event->data.keyDown.chr = vchrRockerCenter;
							event->data.keyDown.keyCode = 0;
							lastkeywasmenu = true;
						} else {
							lastkeywasmenu = false;
						}
						break;
					case 4:
						// auto repeat
						if ((data.config.leftaction == 3) && chgscreen) {
							event->data.keyDown.chr = vchrRockerLeft;
							event->data.keyDown.keyCode = 0;
						}
						break;
					case 8:
						// auto repeat
						if ((data.config.leftaction == 3) && chgscreen) {
							event->data.keyDown.chr = vchrRockerRight;
							event->data.keyDown.keyCode = 0;
						}
						break;
					default:
						break;
				}
			}

			switch (event->data.keyDown.chr) {
				case hardPowerChr:
					PlayKeySound();
					if ((performaction) && ((pwrpresstime+2)>=TimGetSeconds())) {
						EvtEnqueueKey (vchrBacklight, 0, commandKeyMask);
						performaction = false;
					} else {
						performaction = true;
						pwrpresstime = TimGetSeconds();
					}
					handled = true;
					break;
				case autoOffChr: // Palm issuing auto turn off key
					handled = true;
					break;
				case lowBatteryChr:
					// warn due to low battery
					device.lowbatcount++;
					PlayNoGPSSound();

					// log time warned and critical battery level
					warning_time = cursecs;

					// set up alert window data
					StrCopy(suaalert->title, "Low Battery");
					if (device.lowbatcount > 1) {
						PlayNoGPSSound();
						StrCopy(suaalert->displaytext,"\n\n     Battery is Critically Low!");
						StrCat(suaalert->displaytext,"\n\n     Logger must be Stopped");
						// stop logger
						PerformLogging(true);
						device.lowbatlevel = 0;
						// close bluetooth connection
						if (data.config.nmeaxfertype == USEBT) XferClose(USEBT);
					} else {
						StrCopy(suaalert->displaytext,"\n\n                Battery is Low!");
					}
					suaalert->alerttype = LOWBATWARN_ALERT;
					suaalert->priority = LOWBATWARN;
					suaalert->alertidx = -1;
					suaalert->numbtns = 1;
					StrCopy(suaalert->btn0text, "OK");

					// clear last response
					suaalertret->valid = false;
					suaalertret->btnselected = -1;

					// popup alert window
					allowgenalerttap = true;
					HandleWaitDialogWin(1);;
					handled = true;
					break;
				case menuChrT3: // Menu silk screen button on T3
				case vchrRockerCenter: // Center button on 5-way navigator
					switch (frmID) {
						case form_set_mc:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_set_mc_done;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wayselect_ta:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_ta_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;							
						case form_wayselect_tr:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_tr_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;							
						case form_wayselect_te:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_te_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;							
						case form_question:
							if (question->autodismiss) {
								// push default button press into event queue
								newEvent.eType = ctlSelectEvent;
								if (question->default_answer) {
									newEvent.data.ctlEnter.controlID = form_question_yes;
								} else {
									newEvent.data.ctlEnter.controlID = form_question_no;
								}
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_warning:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_warning_ok;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_genalert:
							if (suaalert->priority != LOWBATSTOP) {
								PlayKeySound();
//								HostTraceOutputTL(appErrorClass, "genalert default tap btn6");
								HandleWaitDialogWin(0);
								suaalertret->valid = true;
								suaalertret->btnselected = 6;
								suaalertret->alertidxret = suaalert->alertidx;
								suaalertret->alerttype = suaalert->alerttype;
								suaalertret->priority = suaalert->priority;
							}
							handled=true;
							break;
						default:
							// push menu key press into event queue
							EvtEnqueueKey (menuChr, 0, commandKeyMask);
							break;
					}
					handled = true;
					break;
				case chrCarriageReturn: // This is used for the thumbwheel press in action
				case vchrGarminThumbWheelInHeld:  // This is for when the thumbwheel is held in. Nonpublished.
					PlayKeySound();
					question->type = Qaddnewwpt;
					handled = true;
					FrmPopupForm(form_question);
					break;
			// Home silk screen button
				case vchrRockerLeft:
				case chrLeftArrow:
					if (GotoNextScreen(frmID, -1)) {
						handled = true;
						break;
					} else if (data.config.leftaction >= 2) {
						handled = true;
					        break;
					}
				case launchChr:
				case vchrGarminEscape:
				case vchrGarminEscapeHeld:
				case hsChrVolumeDown: // Map the Treo 600 Volume down button to the home button
//					HostTraceOutputTL(appErrorClass, "frmID=|%s|", DblToStr((double)frmID,0));
					switch (frmID) {
						case form_final_glide:
							PlayKeySound();
							FrmGotoForm(form_moving_map);
							break;
						case form_moving_map:
							PlayKeySound();
							if (draw_log) {
								curmapscale = logmapscale;
								data.config.mapscaleidx = logmapscaleidx;
								actualmapscale = curmapscale * data.input.disconst;
								data.input.curmaporient = logmaporient;
								FindHomeWayptInitialize();
								if (draw_task) {
									draw_log = false;
									draw_task = false;
									FrmGotoForm(form_list_task);
								} else {
									draw_log = false;
									draw_task = false;
									FrmGotoForm(form_flt_info);
								}
							} else {
								FrmGotoForm(form_final_glide);
							}
							break;
						case form_list_waypt:
							PlayKeySound();
							// Cancel selection
//							HostTraceOutputTL(appErrorClass, "Home Cancel List Waypt");
							wayselect = false;
							addWayToTask = false;
							FrmGotoForm(origform);
							break;
						case form_set_task:
							PlayKeySound();
//							HostTraceOutputTL(appErrorClass, "Home Cancel Set Task");
							save_task_data(origform2);
							break;
						case form_task_rules:
							PlayKeySound();
							if (origform == form_set_task) goingtotaskedit = true;
							FrmGotoForm(origform);
							break;
						case form_set_port:
							PlayKeySound();
							// Have to do this otherwise the BT search dialogs steal
							// the events from the FrmGotoForm call
							newEvent.eType = frmCloseEvent;
							form_set_port_event_handler(&newEvent);
							FrmGotoForm(form_final_glide);
							lastkeywasmenu = false;
							break;
						case form_task_waypt:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_task_waypt_save;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_waypoint_sector:
							PlayKeySound();
							if (origform == form_set_task) {
								// return to task waypoint edit
								gototskwpt = true;
								FrmGotoForm(form_task_waypt);
							} else {
								// return to final glide or moving map
								gototskwpt = false;
								FrmGotoForm(origform);
							}
							break;
						case form_disp_sua:
							PlayKeySound();
							FrmGotoForm(origform);
							break;
						case form_set_mc:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_set_mc_done;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wayselect_ta:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_ta_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wayselect_tr:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_tr_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wayselect_te:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_te_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wind_spddir:
							PlayKeySound();
							FrmGotoForm(form_wind_disp);
							break;
						case form_question:
							if (question->autodismiss) {
								// push default button press into event queue
								newEvent.eType = ctlSelectEvent;
								if (question->default_answer) {
									newEvent.data.ctlEnter.controlID = form_question_yes;
								} else {
									newEvent.data.ctlEnter.controlID = form_question_no;
								}
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_warning:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_warning_ok;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_genalert:
							if (suaalert->priority != LOWBATSTOP) {
								PlayKeySound();
//								HostTraceOutputTL(appErrorClass, "genalert default tap btn6");
								HandleWaitDialogWin(0);
								suaalertret->valid = true;
								suaalertret->btnselected = 6;
								suaalertret->alertidxret = suaalert->alertidx;
								suaalertret->alerttype = suaalert->alerttype;
								suaalertret->priority = suaalert->priority;
							}
							break;
						default:
							PlayKeySound();
							FrmGotoForm(defaultscreen);
							break;
					}
					handled = true;
					break;
			// Calendar button
				case hard1Chr:
					switch (frmID) {
						case form_final_glide:
							if (!data.config.usecalchw || (data.input.bearing_to_destination.valid==NOT_VALID)) {
								// manual headwind adjustment
								// only allow if not using calculated headwind or no waypoint selected
								PlayKeySound();
								tempdbl = increment(field_get_double(form_final_glide_headwind), 1.0);
								if (data.config.hwposlabel) {
									data.input.headwind = tempdbl/data.input.wndconst;
								} else {
									data.input.headwind = tempdbl/data.input.wndconst*(-1.0);
								}
								field_set_value(form_final_glide_headwind, DblToStr(tempdbl,1));
								data.application.changed = 1;
							}
							 else {
								// select a waypoint
								if (!draw_log) {
									PlayKeySound();
									if ((inareasector > -1) && (data.config.AATmode & AAT_MTURN) && ((data.task.waypttypes[inareasector] & AREAEXIT) == 0)) {
										// open manual turn in AAT question
										question->type = QturnAAT;
										FrmPopupForm(form_question);
									} else {
										select_fg_waypoint(WAYSELECT);
									}
								}
							}

							break;
						case form_moving_map:
							if (!draw_log) {
								PlayKeySound();
								question->type = Qaddnewwpt;
								handled = true;
								FrmPopupForm(form_question);
							}
/*							 else {
								// select a waypoint
								if (!draw_log) {
									PlayKeySound();
									if ((inareasector > -1) && (data.config.AATmode & AAT_MTURN) && ((data.task.waypttypes[inareasector] & AREAEXIT) == 0)) {
										// open manual turn in AAT question
										question->type = QturnAAT;
										FrmPopupForm(form_question);
									} else {
										select_fg_waypoint(WAYSELECT);
									}
								}
							}
*/
							break;
						case form_wind_disp: 
							// mimic wind left button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wind_disp_wnddirminus;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wind_spddir:
							// mimic bearing left button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wind_spddir_dirminus;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_task_rules:
							if (!settaskreadonly) {
								// mimic minus button pressed
								newEvent.eType = ctlSelectEvent;
								newEvent.data.ctlEnter.controlID = form_task_rules_starttimedown;
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_task_waypt:
							// auto save (as launchchr)
							chgwpt = false;
							if (settaskreadonly) {
								dispactive = false; // go to edited task not active task
								chgwpt = true;
							} else if (save_task_waypt_fields(edittsk, selectedTaskWayIdx, taskIndex, (edittsk->waypttypes[selectedTaskWayIdx] & CONTROL), true)) {
								dispactive = false; // go to edited task not active task
								chgwpt = true;
							}
							if (chgwpt && (selectedTaskWayIdx > 0)) {
								PlayKeySound();
								selectedTaskWayIdx--;
								FrmGotoForm(form_task_waypt);
							}
							break;
						case form_waypoint_sector:
							PlayKeySound();
							if (!activetasksector) {
								minsector = 0;
							} else if (inareasector > -1) {
								minsector = inareasector;
							} else {
								minsector = activetskway;
							}
							if (selectedTaskWayIdx > minsector) {
								PlayKeySound();
//								HostTraceOutputTL(appErrorClass, "Moving back in active task %s", DblToStr(activetasksector,0));
								if (!activetasksector) {
									// force to go back to task waypoint screen if not active task
									gototskwpt = true;
									FrmGotoForm(form_task_waypt);
								} else {
									// goto next sector
									selectedTaskWayIdx--;
									FrmGotoForm(form_waypoint_sector);
								}
							} else if ((inareasector > -1) && (data.config.AATmode & AAT_MTURN) && ((data.task.waypttypes[inareasector] & AREAEXIT) == 0)) {
								// open manual turn in AAT question
								question->type = QturnAAT;
								FrmPopupForm(form_question);
							}
							updatemap = true;
							break;
						case form_set_task:
							PlayKeySound();
							refresh_task_details(TASKUP);
							break;
						case form_list_waypt:
							PlayKeySound();
							if (((Int16)selectedWaypointIndex/7 != currentWayptPage) || (selectedWaypointIndex == -1)) {
								selectedWaypointIndex = currentWayptPage*7 + 6;
								if (selectedWaypointIndex >= numOfWaypts-1) selectedWaypointIndex = numOfWaypts-1;
							} else {
								if (selectedWaypointIndex > 0) selectedWaypointIndex--;
								currentWayptPage = (Int16)selectedWaypointIndex/7;
							}
							refresh_waypoint_list(0);
							break;
						case form_list_sua:
							PlayKeySound();
							if (((Int16)selectedSUAListIdx/10 != currentSUAPage) || (selectedSUAListIdx == -1)) {
								selectedSUAListIdx = currentSUAPage*10 + 9;
								if (selectedSUAListIdx >= numSUARecs-1) selectedSUAListIdx = numSUARecs-1;
							} else {
								if (selectedSUAListIdx > 0) selectedSUAListIdx--;
								currentSUAPage = (Int16)selectedSUAListIdx/10;
							}
							refresh_sua_list(SUADISP);
							break;
						case form_list_task:
							PlayKeySound();
							if (((Int16)taskIndex/7 != currentTaskListPage) || (taskIndex == -1)) {
								taskIndex = currentTaskListPage*7 + 6;
								if (taskIndex >= numOfTasks-1) taskIndex = numOfTasks-1;
							} else {
								if (taskIndex > 0) taskIndex--;
								currentTaskListPage = (Int16)taskIndex/7;
							}
							refresh_task_list(0);
							break;
						case form_list_polar:
							PlayKeySound();
							if (((Int16)selectedPolarIndex/8 != currentPolarPage) || (selectedPolarIndex == -1)) {
								selectedPolarIndex = currentPolarPage*8 + 7;
								if (selectedPolarIndex >= numOfPolars-1) selectedPolarIndex = numOfPolars-1;
							} else {
								if (selectedPolarIndex > 0) selectedPolarIndex--;
								currentPolarPage = (Int16)selectedPolarIndex/8;
							}
							refresh_polar_list(0);
							break;
						case form_list_flts:
							PlayKeySound();
							if (((Int16)selectedCAIFltIndex/CAInumperpage != currentCAIFltPage) || (selectedCAIFltIndex == -1)) {
								selectedCAIFltIndex = currentCAIFltPage*CAInumperpage + CAInumperpage-1;
								if (selectedCAIFltIndex >= cainumLogs-1) selectedCAIFltIndex = cainumLogs-1;
							} else {
								if (selectedCAIFltIndex > 0) selectedCAIFltIndex--;
								currentCAIFltPage = (Int16)selectedCAIFltIndex/CAInumperpage;
							}
							refresh_flts_list(0);
							break;
						case form_list_files:
							PlayKeySound();
							if (((Int16)selectedFileIndex/Filenumperpage != currentFilePage) || (selectedFileIndex == -1)) {
								selectedFileIndex = currentFilePage*numfilesfound + Filenumperpage-1;
								if (selectedFileIndex >= numfilesfound-1) selectedFileIndex = numfilesfound-1;
							} else {
								if (selectedFileIndex > 0) selectedFileIndex--;
								currentFilePage = (Int16)selectedFileIndex/Filenumperpage;
							}
							refresh_files_list(0);
							break;
						case form_set_fg:
							PlayKeySound();
							tempdbl = increment(field_get_double(form_set_fg_bugfactor), -5.0);
							if (tempdbl > 100.0) tempdbl = 100.0;
							if (tempdbl < 75.0) tempdbl = 75.0;
							data.config.bugfactor = tempdbl / 100.0;
							if (data.config.flightcomp == C302COMP) {
								Output302GRec(GBUGS);
								skipbugs = true;
							} else {
								data.input.bugs = data.config.bugfactor;
							}
							field_set_value(form_set_fg_bugfactor, DblToStr(tempdbl,0));
							data.application.changed = 1;
							break;
						case form_wayselect_ta:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_ta_prevwpt;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
						case form_wayselect_tr:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_tr_resumebtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						default:
							break;
					}
					handled = true;
					break;
			// Contacts button
				case hard2Chr:
					switch (frmID) {
						case form_final_glide:
							if (!data.config.usecalchw || (data.input.bearing_to_destination.valid==NOT_VALID)) {
								// manual headwind adjustment
								// only allow if not using calculated headwind or no waypoint selected
								PlayKeySound();
								tempdbl = increment(field_get_double(form_final_glide_headwind), -1.0);
								if (data.config.hwposlabel) {
									data.input.headwind = tempdbl/data.input.wndconst;
								} else {
									data.input.headwind = tempdbl/data.input.wndconst*(-1.0);
								}
								field_set_value(form_final_glide_headwind, DblToStr(tempdbl,1));
							}
							data.application.changed = 1;
							break;
						case form_moving_map:
							// toggle thermal map scale
							if (!draw_log) {
								PlayKeySound();
								if (mapmode == THERMAL) {
									// Override Thermal mode :- map settings
									curmapscale = savmapscale;
									actualmapscale = curmapscale * data.input.disconst;
									data.input.curmaporient = data.config.maporient;
									mapmode = CRUISE;
								} else {	
									// Return to Thermal mode :- thermal map settings
									if (data.config.thzoom != THZOOMVAR) {
										// goto fixed thermal scale as set in Map Settings screen
										curmapscale = data.config.thzoomscale;
									} else {
										// goto variable thermal scale based on last thermal mode
										curmapscale = GetMapScale(data.config.thmapscaleidx) / data.input.disconst;
									}
									actualmapscale = curmapscale * data.input.disconst;
									data.input.curmaporient = data.config.thmaporient;
									mapmode = THERMAL;
								}
							}
							updatemap = true;
							break;
						case form_wind_disp: 
							// mimic wind right button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wind_disp_wnddirplus;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wind_spddir:
							// mimic bearing right button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wind_spddir_dirplus;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_task_rules:
							if (!settaskreadonly) {
								// mimic plus button pressed
								newEvent.eType = ctlSelectEvent;
								newEvent.data.ctlEnter.controlID = form_task_rules_starttimeup;
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_task_waypt:
							// auto save (as launchchr)
							chgwpt = false;
							if (settaskreadonly) {
								dispactive = false; // go to edited task not active task
								chgwpt = true;
							} else if (save_task_waypt_fields(edittsk, selectedTaskWayIdx, taskIndex, (edittsk->waypttypes[selectedTaskWayIdx] & CONTROL), true)) {
								dispactive = false; // go to edited task not active task
								chgwpt = true;
							}
							if (chgwpt && (selectedTaskWayIdx < edittsk->numwaypts-1)) {
								PlayKeySound();
								selectedTaskWayIdx++;
								FrmGotoForm(form_task_waypt);
							}
							break;
						case form_waypoint_sector:
							if (selectedTaskWayIdx < tsktoedit->numwaypts-1) {
								PlayKeySound();
//								HostTraceOutputTL(appErrorClass, "Moving forward in active task %s", DblToStr(activetasksector,0));
								if (!activetasksector) {
									// force to go back to task waypoint screen if not active task
									gototskwpt = true;
									FrmGotoForm(form_task_waypt);
								} else {
									// goto next sector
									selectedTaskWayIdx++;
									FrmGotoForm(form_waypoint_sector);
								}
							}
							updatemap = true;
							break;
						case form_set_task:
							PlayKeySound();
							refresh_task_details(TASKDWN);
							break;
						case form_list_waypt:
							if (selectedWaypointIndex < numOfWaypts-1) {
								PlayKeySound();
								if ((Int16)selectedWaypointIndex/7 != currentWayptPage) {
									selectedWaypointIndex = currentWayptPage*7;
								} else {
									selectedWaypointIndex++;
									currentWayptPage = (Int16)selectedWaypointIndex/7;
								}
								refresh_waypoint_list(0);
							}
							break;
						case form_list_task:
							if (taskIndex < numOfTasks-1) {
								PlayKeySound();
								if ((Int16)taskIndex/7 != currentTaskListPage) {
									taskIndex = currentTaskListPage*7;
								} else {
									taskIndex++;
									currentTaskListPage = (Int16)taskIndex/7;
								}
								refresh_task_list(0);
							}
							break;
						case form_list_sua:
							if (selectedSUAListIdx < numSUARecs-1) {
								PlayKeySound();
								if ((Int16)selectedSUAListIdx/10 != currentSUAPage) {
									selectedSUAListIdx = currentSUAPage*10;
								} else {
									selectedSUAListIdx++;
									currentSUAPage = (Int16)selectedSUAListIdx/10;
								}
								refresh_sua_list(SUADISP);
							}
							break;
						case form_list_polar:
							if (selectedPolarIndex < numOfPolars-1) {
								PlayKeySound();
								if ((Int16)selectedPolarIndex/8 != currentPolarPage) {
									selectedPolarIndex = currentPolarPage*8;
								} else {
									selectedPolarIndex++;
									currentPolarPage = (Int16)selectedPolarIndex/8;
								}
								refresh_polar_list(0);
							}
							break;
						case form_list_flts:
							if (selectedCAIFltIndex < cainumLogs-1) {
								PlayKeySound();
								if ((Int16)selectedCAIFltIndex/CAInumperpage != currentCAIFltPage) {
									selectedCAIFltIndex = currentCAIFltPage*CAInumperpage;
								} else {
									selectedCAIFltIndex++;
									currentCAIFltPage = (Int16)selectedCAIFltIndex/CAInumperpage;
								}
								refresh_flts_list(0);
							}
							break;
						case form_list_files:
							if (selectedFileIndex < numfilesfound-1) {
								PlayKeySound();
								if ((Int16)selectedFileIndex/Filenumperpage != currentFilePage) {
									selectedFileIndex = currentFilePage*Filenumperpage;
								} else {
									selectedFileIndex++;
									currentFilePage = (Int16)selectedFileIndex/Filenumperpage;
								}
								refresh_files_list(0);
							}
							break;
						case form_set_fg:
							PlayKeySound();
							tempdbl = increment(field_get_double(form_set_fg_bugfactor), 5.0);
							if (tempdbl > 100.0) tempdbl = 100.0;
							if (tempdbl < 75.0) tempdbl = 75.0;
							data.config.bugfactor = tempdbl / 100.0;
							if (data.config.flightcomp == C302COMP) {
								Output302GRec(GBUGS);
								skipbugs = true;
							} else {
								data.input.bugs = data.config.bugfactor;
							}
							field_set_value(form_set_fg_bugfactor, DblToStr(tempdbl,0));
							data.application.changed = 1;
							break;
						case form_wayselect_ta:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_ta_nextwpt;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						default:
							break;
					}
					handled = true;
					break;
			// To Do button
				case hard3Chr:
					switch (frmID) {
						case form_set_mc:
							formopen = cursecs; // prevent form auto-dismiss
						case form_final_glide:
						case form_thermal_history:
						case form_flt_info:
						case form_moving_map:
						case form_waypoint_sector:
							PlayKeySound();
							// option control my MCBUTTON line in config file
							if ((data.config.MCbutton == POPUP) && (frmID != form_set_mc)) {
								// 1 = POPUP
			        				FrmPopupForm(form_set_mc);
							} else {
								// 0 = NORMAL
								incMC(data.input.basemc * data.input.lftconst);
							}
							updatemap = true;
							break;
						case form_set_fg:
							PlayKeySound();
							data.config.safealt = increment(field_get_double(form_set_fg_safealt), 10.0)/data.input.altconst;
							field_set_value(form_set_fg_safealt, print_altitude(data.config.safealt));
							data.application.changed = 1;
							break;
						case form_set_qnh:
							PlayKeySound();
							data.config.qfealt = increment(field_get_double(form_set_qnh_fieldelev), 10.0)/data.input.altconst;
							field_set_value(form_set_qnh_fieldelev, print_altitude(data.config.qfealt));
							data.application.changed = 1;
							break;
						case form_set_task:
							if (!settaskreadonly && (taskIndex == 0) && (data.task.numwaypts > 0) && ((selectedTaskWayIdx != activetskway) || taskonhold)) {
								PlayKeySound();
								requestedtskidx = selectedTaskWayIdx;
								if (inflight) {
									question->type = Qsetactivewpt;
									handled = true;
									FrmPopupForm(form_question);
								} else {
									refresh_task_details(TASKSETWPT);
								}
							}
							break;
						case form_wind_disp: 
							// mimic wind increase button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wind_disp_wndspdplus;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wind_spddir:
							// mimic airspeed increase button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wind_spddir_spdplus;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wind_3dinfo:
							PlayKeySound();
							profilebase++;
							if (((7+profilebase) * profilescale[data.config.profilescaleidx]) >= WINDPROFILEPOINTS) profilebase--;
							CalcWindProfile(false, false);
							break;
						case form_task_rules:
							if (!settaskreadonly) {
								// mimic plus min time button pressed
								newEvent.eType = ctlSelectEvent;
								newEvent.data.ctlEnter.controlID = form_task_rules_mintimeup;
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_wayselect_ta:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_ta_addbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						default:
							break;
					}
					handled=true;
					break;
			// Memo button
				case hard4Chr:
					switch (frmID) {
						case form_set_mc:
							formopen = cursecs; // prevent form auto-dismiss
						case form_final_glide:
						case form_thermal_history:
						case form_flt_info:
						case form_moving_map:
						case form_waypoint_sector:
							PlayKeySound();
							// option control my MCBUTTON line in config file
							if ((data.config.MCbutton == POPUP) && (frmID != form_set_mc)) {
								// 1 = POPUP
			        				FrmPopupForm(form_set_mc);
							} else {
								// 0 = NORMAL
								decMC(data.input.basemc * data.input.lftconst);
							}
							updatemap = true;
							break;
						case form_wind_disp:
							// mimic wind decrease button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wind_disp_wndspdminus;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wind_spddir:
							// mimic aispeed decrease button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wind_spddir_spdminus;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wind_3dinfo:
							PlayKeySound();
							profilebase--;
							if (profilebase < 0) profilebase = 0;
							CalcWindProfile(false, false);
							break;
						case form_task_rules:
							if (!settaskreadonly) {
								// mimic minus min time button pressed
								newEvent.eType = ctlSelectEvent;
								newEvent.data.ctlEnter.controlID = form_task_rules_mintimedown;
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_set_task:
							// mimic task activate button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_set_task_actvbtn;
							EvtAddEventToQueue(&newEvent);
							break;
						case form_list_task:
							PlayKeySound();
							refresh_task_details(TASKACTV);
							currentTaskListPage = 0;
							refresh_task_list(0);
							break;
						case form_set_fg:
							PlayKeySound();
							tempdbl = increment(field_get_double(form_set_fg_safealt), -10.0);
							if (tempdbl < 0.0) tempdbl = 0.0;
							data.config.safealt = tempdbl/data.input.altconst;
							field_set_value(form_set_fg_safealt, print_altitude(data.config.safealt));
							data.application.changed = 1;
							break;
						case form_set_qnh:
							PlayKeySound();
							data.config.qfealt = increment(field_get_double(form_set_qnh_fieldelev), -10.0)/data.input.altconst;
							field_set_value(form_set_qnh_fieldelev, print_altitude(data.config.qfealt));
							data.application.changed = 1;
							break;
							break;
						case form_list_waypt:
							// mimic Save button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_list_waypt_OK;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
							break;
						case form_task_waypt:
							// mimic Save button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_task_waypt_save;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_av_waypt:
							// mimic Save button pressed
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_av_waypt_save;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_list_sua:
							if (!SUAselectall) {
								PlayKeySound();
								refresh_sua_list(SUAVIEW);
							}
							break;
						case form_disp_sua:
							PlayKeySound();
							saveSUAitem();
							break;
						case form_wayselect_tr:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_tr_tempbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wayselect_ta:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_ta_tempbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_genalert:
							if (suaalert->priority < SUAWARN_ALL) {
								PlayKeySound();
//								HostTraceOutputTL(appErrorClass, "genalert info btn");
								HandleWaitDialogWin(0);
								suaalertret->valid = true;
								suaalertret->btnselected = 7;
								suaalertret->alertidxret = suaalert->alertidx;
								suaalertret->alerttype = suaalert->alerttype;
								suaalertret->priority = suaalert->priority;
							}
							break;
						default:
							break;
					}
					handled = true;
					break;
			// Page Down button
				case pageDownChr:
				case vchrRockerDown:
					// ignore if menu open
					if (!menuopen) {
					switch (frmID) {
						case form_final_glide:
							if (data.input.bearing_to_destination.valid==NOT_VALID) {
								// only allow manual distance change if no waypoint selected
								PlayKeySound();
								tempdbl = field_get_double(form_final_glide_dis);
								tempdbl -= 1.0;
								if ((data.input.distance_to_destination.valid == VALID) &&
									(data.input.distance_to_destination.value > 0.0)) {
									data.input.distance_to_destination.value = tempdbl/data.input.disconst;
	
									if (tempdbl < 10.0) {
										StrCopy(tempchar, DblToStr(tempdbl, 1));
									} else if (tempdbl < 1000.0) {
										StrCopy(tempchar, DblToStr(tempdbl, 0));
									} else {
										StrCopy(tempchar, "999");
									}
									field_set_value(form_final_glide_dis, tempchar);
									ManualDistChg = true;
								} else {
									data.input.distance_to_destination.value = 0.0;		
									field_set_value(form_final_glide_dis, DblToStr(data.input.distance_to_destination.value, 1));
								}
								data.input.distance_to_destination.valid=VALID;
								data.application.changed = 1;
							}
							break;
						case form_moving_map:
							PlayKeySound();
							tempdbl = actualmapscale;
							if (mapmode == CRUISE) {
								// change map scale
								data.config.mapscaleidx = FindNextMapScale(&actualmapscale, true);
							} else if (data.config.thzoom == THZOOMVAR) {
								// change variable thermal scale
								data.config.thmapscaleidx = FindNextMapScale(&actualmapscale, true);
							} else {
								// temp change fixed thermal scale
								FindNextMapScale(&actualmapscale, true);
							}
							curmapscale = actualmapscale / data.input.disconst;
							if (mapmode == CRUISE) savmapscale = curmapscale;
							if (actualmapscale != tempdbl) {
								updatemap = true;
								// set flag if zoom level above calculated zoom level
								if (curmapscale > zoommapscale) {
									if (inflight) manualzoomchg = true;
								} else {
									manualzoomchg = false;
								}
								data.application.changed = 1;
							}
							break;
						case form_waypoint_sector:
							PlayKeySound();
							tempdbl = sectorscale * data.input.disconst;
							tempscale = Ceil(tempdbl);
							FindNextMapScale(&tempscale, true);
							sectorscale = tempscale / data.input.disconst;
							if (sectorscale != tempdbl / data.input.disconst) {
								updatemap = true;
								data.application.changed = 1;
							}
							break;
						case form_list_polar:
							PlayKeySound();
							refresh_polar_list(1);
							break;
						case form_list_waypt:
							PlayKeySound();
							refresh_waypoint_list(1);
							break;
						case form_list_task:
							PlayKeySound();
							refresh_task_list(1);
							break;
						case form_list_flts:
							PlayKeySound();
							refresh_flts_list(1);
							break;
						case form_list_files:
							PlayKeySound();
							refresh_files_list(1);
							break;
						case form_set_fg:
							PlayKeySound();
							tempdbl = increment(field_get_double(form_set_fg_pctwater), -10.0);
							if (tempdbl > 100.0) tempdbl = 100.0;
							if (tempdbl < 0.0) tempdbl = 0.0;
							data.config.pctwater = tempdbl / 100.0;
							if (data.config.flightcomp == C302COMP) {
								Output302GRec(GBALLAST);
								skipballast = true;
							} else {
								data.input.ballast = data.config.pctwater;
							}
							field_set_value(form_set_fg_pctwater, DblToStr(tempdbl,0));
							data.application.changed = 1;
							break;
						case form_set_task:
							PlayKeySound();
							refresh_task_details(TASKPGDWN);
							break;
						case form_list_sua:
							PlayKeySound();
							refresh_sua_list(SUAPGDWN);
							break;
						case form_set_qnh:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_set_qnh_minus1;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_sat_status:
							PlayKeySound();
							data.config.Flarmscaleidx--;
							if (data.config.Flarmscaleidx < 0) data.config.Flarmscaleidx = 0;
							break;
						case form_task_rules:
							if (!settaskreadonly) {
								// mimic minus height button pressed
								newEvent.eType = ctlSelectEvent;
								newEvent.data.ctlEnter.controlID = form_task_rules_maxstartdown;
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_wind_3dinfo:
							PlayKeySound();
							tempdbl = (WINDPROFILECONST + profilebase) * profilescale[data.config.profilescaleidx] * profilestep;
							data.config.profilescaleidx--;
							if (data.config.profilescaleidx < 0) data.config.profilescaleidx = 0;
							profilebase = (Int16)(data.input.inusealt / (profilescale[data.config.profilescaleidx] * profilestep)) - 5;
							if (profilebase < 0) profilebase = 0;
							CalcWindProfile(false, false);
							break;
						case form_task_waypt:
							if (field_get_enable(form_task_waypt_arearadius)) {
								PlayKeySound();
								tempdbl = increment(field_get_double(form_task_waypt_arearadius), -1.0);
								if (tempdbl < 0.0) tempdbl = 0.0;
								field_set_value(form_task_waypt_arearadius, DblToStr(tempdbl,1));
								data.application.changed = 1;
							}
							break;
						case form_av_waypt:
							if (field_get_enable(form_av_waypt_arearadius)) {
								PlayKeySound();
								tempdbl = increment(field_get_double(form_av_waypt_arearadius), -1.0);
								if (tempdbl < 0.0) tempdbl = 0.0;
								field_set_value(form_av_waypt_arearadius, DblToStr(tempdbl,1));
								data.application.changed = 1;
							}
							break;
						case form_set_scrorder:
							PlayKeySound();
							// find which screen is selected
							for (i=1; i<=10; i++) {
								if (ctl_get_value(form_set_scrorder_scrlbl0+i)) break;
							}
							if (i <= 9) {
								// move selected screen down
								tempscr = data.config.screenchain[i-1];
								data.config.screenchain[i-1] = data.config.screenchain[i];
								data.config.screenchain[i] = tempscr;
								ctl_set_value(form_set_scrorder_scrlbl0+i+1, true);
								ctl_set_value(form_set_scrorder_scrlbl0+i, false);
								// re-display screen
								newEvent.eType = frmUpdateEvent;
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
							
						default:
							break;
					}}
					handled = true;
					break;
			// Page Up Button
				case pageUpChr:
				case vchrRockerUp:
					// ignore if menu open
					if (!menuopen) {
					switch (frmID) {
						case form_final_glide:
							if (data.input.bearing_to_destination.valid==NOT_VALID) {
								// only allow manual distance change if no waypoint selected
								PlayKeySound();
								tempdbl = field_get_double(form_final_glide_dis);
								tempdbl += 1.0;
								if (data.input.distance_to_destination.valid==VALID) {
									data.input.distance_to_destination.value = tempdbl/data.input.disconst;
	
									if (tempdbl < 10.0) {
										StrCopy(tempchar, DblToStr(tempdbl, 1));
									} else if (tempdbl < 1000.0) {
										StrCopy(tempchar, DblToStr(tempdbl, 0));
									} else {
										StrCopy(tempchar, "999");
									}
									field_set_value(form_final_glide_dis, tempchar);
									ManualDistChg = true;
								} else {
									data.input.distance_to_destination.value = 0.0;		
									field_set_value(form_final_glide_dis, DblToStr(data.input.distance_to_destination.value, 1));
								}
								data.input.distance_to_destination.valid=VALID;
								data.application.changed = 1;
							}
							break;
						case form_moving_map:
							PlayKeySound();
							tempdbl = actualmapscale;
							if (mapmode == CRUISE) {
								// change map scale
								data.config.mapscaleidx = FindNextMapScale(&actualmapscale, false);
							} else if (data.config.thzoom == THZOOMVAR) {
								// change variable thermal scale
								data.config.thmapscaleidx = FindNextMapScale(&actualmapscale, false);
							} else {
								// temp change fixed thermal scale
								FindNextMapScale(&actualmapscale, false);
							}
							curmapscale = actualmapscale / data.input.disconst;
							if (mapmode == CRUISE) savmapscale = curmapscale;
							if (actualmapscale != tempdbl) {
								updatemap = true;
								// always set flag
								if (inflight) manualzoomchg = true;
								data.application.changed = 1;
							}
							break;
						case form_waypoint_sector:
							PlayKeySound();
							tempdbl = sectorscale * data.input.disconst;
							tempscale = Ceil(tempdbl);
							FindNextMapScale(&tempscale, false);
							sectorscale = tempscale / data.input.disconst;
							if (sectorscale != tempdbl / data.input.disconst) {
								updatemap = true;
								data.application.changed = 1;
							}
							break;
						case form_list_polar:
							PlayKeySound();
							refresh_polar_list(-1);
							break;
						case form_list_waypt:
							PlayKeySound();
							refresh_waypoint_list(-1);
							break;
						case form_list_task:
							PlayKeySound();
							refresh_task_list(-1);
							break;
						case form_list_flts:
							PlayKeySound();
							refresh_flts_list(-1);
							break;
						case form_list_files:
							PlayKeySound();
							refresh_files_list(-1);
							break;
						case form_set_fg:
							PlayKeySound();
							tempdbl = increment(field_get_double(form_set_fg_pctwater), 10.0);
							if (tempdbl > 100.0) tempdbl = 100.0;
							data.config.pctwater = tempdbl / 100.0;
							if (data.config.flightcomp == C302COMP) {
								Output302GRec(GBALLAST);
								skipballast = true;
							} else {
								data.input.ballast = data.config.pctwater;
							}
							field_set_value(form_set_fg_pctwater, DblToStr(tempdbl,0));
							data.application.changed = 1;
							break;
						case form_set_task:
							PlayKeySound();
							refresh_task_details(TASKPGUP);
							break;
						case form_list_sua:
							PlayKeySound();
							refresh_sua_list(SUAPGUP);
							break;
						case form_set_qnh:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_set_qnh_plus1;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_sat_status:
							PlayKeySound();
							Flarmmaxrange = (data.input.disconst<DISTKMCONST?8:10);
							data.config.Flarmscaleidx++;
							if (data.config.Flarmscaleidx > Flarmmaxrange) data.config.Flarmscaleidx = Flarmmaxrange;
							break;
						case form_task_rules:
							if (!settaskreadonly) {
								// mimic plus height button pressed
								newEvent.eType = ctlSelectEvent;
								newEvent.data.ctlEnter.controlID = form_task_rules_maxstartup;
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_wind_3dinfo:
							PlayKeySound();
							tempdbl = (WINDPROFILECONST + profilebase) * profilescale[data.config.profilescaleidx] * profilestep;
							data.config.profilescaleidx++;
							if (data.config.profilescaleidx > WINDPROFILESCALES-1) data.config.profilescaleidx = WINDPROFILESCALES-1;
							profilebase = (Int16)(data.input.inusealt / (profilescale[data.config.profilescaleidx] * profilestep)) - 5;
							if (profilebase < 0) profilebase = 0;
							CalcWindProfile(false, false);
							break;
						case form_task_waypt:
							if (field_get_enable(form_task_waypt_arearadius)) {
								PlayKeySound();
								tempdbl = increment(field_get_double(form_task_waypt_arearadius), 1.0);
								if (tempdbl > 99.9) tempdbl = 99.9;
								field_set_value(form_task_waypt_arearadius, DblToStr(tempdbl,1));
								data.application.changed = 1;
							}
							break;
						case form_av_waypt:
							if (field_get_enable(form_av_waypt_arearadius)) {
								PlayKeySound();
								tempdbl = increment(field_get_double(form_av_waypt_arearadius), 1.0);
								if (tempdbl > 99.9) tempdbl = 99.9;
								field_set_value(form_av_waypt_arearadius, DblToStr(tempdbl,1));
								data.application.changed = 1;
							}
							break;
						case form_set_scrorder:
							PlayKeySound();
							// find which screen is selected
							for (i=1; i<=10; i++) {
								if (ctl_get_value(form_set_scrorder_scrlbl0+i)) break;
							}
							if ((i > 1) && (i <=10)) {
								// move selected screen up
								tempscr = data.config.screenchain[i-2];
								data.config.screenchain[i-2] = data.config.screenchain[i-1];
								data.config.screenchain[i-1] = tempscr;
								ctl_set_value(form_set_scrorder_scrlbl0+i-1, true);
								ctl_set_value(form_set_scrorder_scrlbl0+i, false);
								// re-display screen
								newEvent.eType = frmUpdateEvent;
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						default:
							break;
					}}
					handled = true;
					break;
			// Calc silk screen button
				case calcChr:
				case calcChrT3:
				case vchrGarminRecord: // Map the Garmin Record button to bring up waypoint select
				case vchrGarminRecordHeld: // Map the Garmin Record held button to bring up waypoint select
				case hsChrVolumeUp: // Map the Treo 600 Volume Up button to the waypoint select
				case chrLineFeed: // Map the enter key to waypoint select for the Tungsten C 
				case vchrHard5: // This maps the Voice Record button to waypoint select on Tungsten T 
					switch (frmID) {
						case form_moving_map:
						case form_final_glide:
						case form_waypoint_sector:
							if (!draw_log) {
								PlayKeySound();
								if ((inareasector > -1) && (data.config.AATmode & AAT_MTURN) && ((data.task.waypttypes[inareasector] & AREAEXIT) == 0)) {
									// open manual turn in AAT question
									question->type = QturnAAT;
									FrmPopupForm(form_question);
								} else {
									select_fg_waypoint(WAYSELECT);
								}
							}
							break;
						case form_list_waypt:
							PlayKeySound();
							if ((WayptsortType != SortByDistApt) || (currentWayptPage != 0)) {
								// second press calls up airport sorted list for emergency landing options
								emergencyland = true;
								FrmGotoForm(form_list_waypt);
							} else {
								// This code had to be here because the Logger declare alert dialogs
								// steal the events from the FrmGotoForm call
								switch (origform) {
								case form_final_glide:
								case form_moving_map:
									if (wayselect) {
										pressedgo = true;
										if (selectedWaypointIndex != -1) {
											OpenDBQueryRecord(waypoint_db, selectedWaypointIndex, &mem_hand, &mem_ptr);
											MemMove(selectedWaypoint, mem_ptr, sizeof(WaypointData));
											MemHandleUnlock(mem_hand);
											// update last used time stamp
											if (!newTaskWay) {
												TimSecondsToDateTime(cursecs, &curtime);
												StrCopy(selectedWaypoint->UsedTime, leftpad(DblToStr(curtime.year,0), '0', 2));
												StrCat( selectedWaypoint->UsedTime, "/");
												StrCat( selectedWaypoint->UsedTime, leftpad(DblToStr(curtime.month,0), '0', 2));
												StrCat( selectedWaypoint->UsedTime, "/");
												StrCat( selectedWaypoint->UsedTime, leftpad(DblToStr(curtime.day,0), '0', 2));
												OpenDBUpdateRecord(waypoint_db, sizeof(WaypointData), selectedWaypoint, selectedWaypointIndex);
											}
										} else {
											MemSet(selectedWaypoint, sizeof(WaypointData),0);
										}
										if (addWayToTask && selectedWaypointIndex != -1) {
											tskWayAddIdx = selectedWaypointIndex;
											newTaskWay = false;
											refresh_task_details(TASKADDBEFORE);
											refresh_task_details(TASKACTV);
										} else if (selectedWaypointIndex == -1) {
											taskonhold = false;
											savtaskonhold = false;
											HandleTask(TSKREACTIVATE);
										}
									}
									break;
								}
								FrmGotoForm(origform);
							}
							break;
						case form_set_task:
							PlayKeySound();
							if (settaskreadonly) {
								exittaskreadonly = true;
								FrmGotoForm(form_flt_info);								
							} else {
								goingtotaskscrn = true;
								FrmPopupForm(form_wayselect_te);
							}
							break;
						case form_wayselect_tr:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_tr_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							select_fg_waypoint(WAYLAND);
							break;
						case form_set_mc:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_set_mc_done;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wayselect_ta:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_ta_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							select_fg_waypoint(WAYLAND);
							break;
						case form_wayselect_te:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_te_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_question:
							if (question->autodismiss) {
								// push default button press into event queue
								newEvent.eType = ctlSelectEvent;
								if (question->default_answer) {
									newEvent.data.ctlEnter.controlID = form_question_yes;
								} else {
									newEvent.data.ctlEnter.controlID = form_question_no;
								}
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_warning:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_warning_ok;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_genalert:
							if (suaalert->priority != LOWBATSTOP) {
								PlayKeySound();
//								HostTraceOutputTL(appErrorClass, "genalert default tap btn6");
								HandleWaitDialogWin(0);
								suaalertret->valid = true;
								suaalertret->btnselected = 6;
								suaalertret->alertidxret = suaalert->alertidx;
								suaalertret->alerttype = suaalert->alerttype;
								suaalertret->priority = suaalert->priority;
							}
							break;
						default:
							break;
					}
					handled = true;
					break;
			// Find silk screen button
				case findChr:
				case findChrT3:
				case vchrRockerRight:
				case chrRightArrow:
				case chrTab:
					// handle screen chain (forward)
					GotoNextScreen(frmID, 1);
					handled = true;
					break;
			// Menu silk screen button
				case menuChr:
//					HostTraceOutputTL(appErrorClass, "Menu Open");
					PlayKeySound();
					switch (frmID) {
						case form_moving_map:
							if (device.HiDensityScrPresent) {
								WinSetCoordinateSystem(kCoordinatesStandard);
							}
							break;
						default:
							break;
					}
					handled = false;
					break;
				case hardCradleChr:	// hotsync button on cradle
					switch (frmID) {
						case form_final_glide:
							PlayKeySound();
							FrmGotoForm(form_moving_map);
							break;
						case form_moving_map:
							PlayKeySound();
							FrmGotoForm(form_final_glide);
							break;
						case form_set_mc:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_set_mc_done;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wayselect_ta:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_ta_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wayselect_tr:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_tr_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_wayselect_te:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_wayselect_te_cancelbtn;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_question:
							if (question->autodismiss) {
								// push default button press into event queue
								newEvent.eType = ctlSelectEvent;
								if (question->default_answer) {
									newEvent.data.ctlEnter.controlID = form_question_yes;
								} else {
									newEvent.data.ctlEnter.controlID = form_question_no;
								}
								EvtAddEventToQueue(&newEvent);
								lastkeywasmenu = false;
							}
							break;
						case form_warning:
							// push button press into event queue
							newEvent.eType = ctlSelectEvent;
							newEvent.data.ctlEnter.controlID = form_warning_ok;
							EvtAddEventToQueue(&newEvent);
							lastkeywasmenu = false;
							break;
						case form_genalert:
							if (suaalert->priority != LOWBATSTOP) {
								PlayKeySound();
//								HostTraceOutputTL(appErrorClass, "genalert default tap btn6");
								HandleWaitDialogWin(0);
								suaalertret->valid = true;
								suaalertret->btnselected = 6;
								suaalertret->alertidxret = suaalert->alertidx;
								suaalertret->alerttype = suaalert->alerttype;
								suaalertret->priority = suaalert->priority;
							}
							break;
						default:
							PlayKeySound();
							FrmGotoForm(defaultscreen);
							break;
					}
					handled = true;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return handled;
}

/**
* \fn EventLoop
* \brief EventLoop is the event handler
*/
static void EventLoop(void)
{
	EventType event;
	UInt16 error;
	UInt16 x;

	do {
		/* wakeup every half second since data arrives every second*/
		EvtGetEvent(&event,SysTicksPerSecond()/2); 
//		EvtResetAutoOffTimer();
		if (! PreprocessEvent(&event)) 
			if (! SysHandleEvent(&event))
				if (! MenuHandleEvent(NULL, &event, &error))
					if (! ApplicationHandleEvent(&event)){
						FrmDispatchEvent(&event);
					}
		if ((event.eType == nilEvent) && (data.application.changed !=0)) {
			/* recalculate form */
			data.application.changed = 0;
			x = 0;
			while((x<MAX_EVENT_SZ) && (data.application.display_events[x].valid==VALID)) {

				if (menuopen == false) {
					data.application.display_events[x].event();
				}
				/* make sure we handle events while we recalculate */
				while (EvtEventAvail()) { 
					EvtGetEvent(&event,0); 
					if (! PreprocessEvent(&event)) 
						if (! SysHandleEvent(&event))
							if (! MenuHandleEvent(NULL, &event, &error))
								if (! ApplicationHandleEvent(&event)) {
									FrmDispatchEvent(&event);
								}
					if (event.eType == appStopEvent && allowExit) {
//						break;
						return;
					}
				}
				x++;
			}
		}
	} while (event.eType != appStopEvent || !allowExit);
}

/**
 \fn PilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
 \brief application main entry point
 \param cmd is the unsigned 16 bits command code
 \param cmdPBP is a pointer to optional additional command data
 \param launchFlags are launch flags
 \return result code
 \details Each application has a PilotMain function that is equivalent to main in C programs.
 To launch an application, the system calls PilotMain and sends it a launch code. 
 The launch code may specify that the application is to become active and display its user interface
 (called a normal launch), or it may specify that the application should simply perform a small task
 and exit without displaying its user interface. The sole purpose of the PilotMain function is to
 receive launch codes and respond to them. 
*/
UInt32 PilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	UInt16 error=0;

	// SoarPilot needs as a minimum PalmOS 3.0
	if ((error = RomVersionCompatible(SYS_VER_30, launchFlags)) != 0)
		return(error);

	if (cmd == sysAppLaunchCmdNormalLaunch) {
		error = StartApplication(); // Application start code
		if (error) return(error);
		EventLoop(); // Event loop
		StopApplication(); // Application stop code
	} else if (cmd == sysAppLaunchCmdAlarmTriggered) {
		FrmCustomAlert(WarningAlert, "Got a sysAppLaunchCmdAlarmTriggered Event"," "," ");
 		return(error);
	} else if (cmd == sysAppLaunchCmdDisplayAlarm) {
		FrmCustomAlert(WarningAlert, "Got a sysAppLaunchCmdDisplayAlarm Event"," "," ");
 		return(error);
 	}
	return(error);
}

/**
 * \fn RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
 * \brief This routine checks that a ROM version is meet your minimum requirement.
 * \param requiredVersion - minimum rom version required
 *              (see sysFtrNumROMVersion in SystemMgr.h for format)
 * \param launchFlags - flags that indicate if the application UI is initialized.
 *
 * \return error code or zero if rom is compatible
 *
 */
Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersionNew;

	// See if we're on in minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersionNew);
	if (romVersionNew < requiredVersion) {
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) {
			FrmAlert (RomIncompatibleAlert);
		
			// Palm OS 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersionNew < SYS_VER_30)
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
		}
		return sysErrRomIncompatible;
	}
	return(errNone);
}

// Determines if the device is capable of taking a memory card
Boolean CardEnabledDevice() 
{
	Err	err;
	UInt32	vfsMgrVersion;

	err = FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &vfsMgrVersion);
	if ( err ) {
		return(false);
	}

	return(true);
}

// Determines if the device is BlueTooth capable
Boolean BTEnabledDevice() 
{
	UInt32 btVersion;
	Err	err;
	Boolean	retval = true;

	// Make sure Bluetooth components are installed
	// This check also ensures Palm OS 4.0 or greater
	err = FtrGet(btLibFeatureCreator, btLibFeatureVersion, &btVersion);
	if ( err ) {
		return(false);
		retval = false;
	}

	return(retval);
}

Boolean DIAEnabledDevice() 
{
	UInt32	diaVersion;
	Err	err;
	UInt16 diaretval;

	// Make sure the Dynamic Input Manager is installed
	err = FtrGet(pinCreator, pinFtrAPIVersion, &diaVersion);
	if ( err ) {
//		HostTraceOutputTL(appErrorClass, "Error from FtrGet1");
		return(false);
	}
//	HostTraceOutputTL(appErrorClass, "No error from FtrGet1");

	// iQue 3600 or 3600a
	if (device.iQueCapable >= iQue3600) {
		return(true);
	// iQue 3000 or 3200
	} else if (device.iQueCapable > 0) {
		return(false);
	// Everything else
	} else {
		diaretval = PINGetInputAreaState();
		if (diaretval == pinInputAreaNone) {
//			HostTraceOutputTL(appErrorClass, "No PIN Input Area");
			return(false);
		}
//		HostTraceOutputTL(appErrorClass, "PIN Input Area Capable Device");

		diaretval = PINGetInputTriggerState();
		if (diaretval == pinInputTriggerNone) {
//			HostTraceOutputTL(appErrorClass, "No PIN Input Trigger");
			return(false);
		}
//		HostTraceOutputTL(appErrorClass, "PIN Input Trigger Capable Device");

		return(true);
	}
}

void RegisterVolumeNotification(Boolean reg)
{
	UInt16 cardNo;
	LocalID dbID;
	Err err;
	Boolean mounted = true;
	Boolean unmounted = false;

	err = SysCurAppDatabase(&cardNo, &dbID);
	ErrNonFatalDisplayIf(err != errNone, "Can't get app db info");
	if(err == errNone) {
		if (reg) {
			err = SysNotifyRegister(cardNo, dbID, sysNotifyVolumeUnmountedEvent, 
											(SysNotifyProcPtr)CardUnMountHandler, sysNotifyNormalPriority, (void *)&unmounted);
			err = SysNotifyRegister(cardNo, dbID, sysNotifyVolumeMountedEvent, 
											(SysNotifyProcPtr)CardMountHandler, sysNotifyNormalPriority, (void *)&mounted);
		} else {
			err = SysNotifyUnregister(cardNo, dbID, sysNotifyVolumeMountedEvent, sysNotifyNormalPriority);
			err = SysNotifyUnregister(cardNo, dbID, sysNotifyVolumeUnmountedEvent, sysNotifyNormalPriority);
		}
		ErrNonFatalDisplayIf((err != errNone) && (err != sysNotifyErrDuplicateEntry), "Can't register");
	}

	return;
}

void RegisterWinResizedEventNotification(Boolean reg)
{
	UInt16 cardNo;
	LocalID dbID;
	Err err;

	err = SysCurAppDatabase(&cardNo, &dbID);
	ErrNonFatalDisplayIf(err != errNone, "Can't Get App DB Info");
	if(err == errNone) {
		if (reg) {
			err = SysNotifyRegister(cardNo, dbID, sysNotifyDisplayResizedEvent, DisplayResizedEventCallback,
											sysNotifyNormalPriority, NULL );
			ErrNonFatalDisplayIf( ( err != errNone ), "Can't Register WinResizedEvent" );
		} else {
			err = SysNotifyUnregister(cardNo, dbID, sysNotifyDisplayResizedEvent, sysNotifyNormalPriority );
			ErrNonFatalDisplayIf( ( err != errNone ), "Can't Unregister WinResizedEvent" );
		}
	}

	return;
}

void RegisterCableAttachDetachNotification(Boolean reg)
{
	UInt16 cardNo;
	LocalID dbID;
	Err err;

	err = SysCurAppDatabase(&cardNo, &dbID);
	ErrNonFatalDisplayIf(err != errNone, "Can't get app db info");
	if(err == errNone) {
		if (reg) {
//			HostTraceOutputTL(appErrorClass, "Registering Cable Events");
			// Tell the system we want to know when the device is connected or disconnected
			err = SysNotifyRegister(cardNo, dbID, sysExternalConnectorAttachEvent, 
											(SysNotifyProcPtr)CableAttachHandler, sysNotifyNormalPriority, NULL);
			err = SysNotifyRegister(cardNo, dbID, sysExternalConnectorDetachEvent, 
											(SysNotifyProcPtr)CableDetachHandler, sysNotifyNormalPriority, NULL);

		} else {
//			HostTraceOutputTL(appErrorClass, "Un-Registering Cable Events");
			err = SysNotifyUnregister(cardNo, dbID, sysExternalConnectorAttachEvent, sysNotifyNormalPriority);
			err = SysNotifyUnregister(cardNo, dbID, sysExternalConnectorDetachEvent, sysNotifyNormalPriority);
		}
//		ErrNonFatalDisplayIf((err != errNone) && (err != sysNotifyErrDuplicateEntry), "Can't register");
	}

	return;
}

/******************************************************************************
*
*   PROCEDURE NAME:
*       DisplayResizedEventCallback - Display Change Event Callback
*
*   DESCRIPTION:
*       Gets called when a Display Resized Event Notification occurs.
*       It then encodes a new winDisplayChangedEvent.
*       This is for compatibility with PenManager 1.0
*
******************************************************************************/
Err DisplayResizedEventCallback( SysNotifyParamType *notifyParamsP )
{
	EventType resizedEvent;
	//add winDisplayChangedEvent to the event queue
	resizedEvent.eType = (eventsEnum)winDisplayChangedEvent;
	EvtAddUniqueEventToQueue(&resizedEvent, 0, true);

	return(0);

} /* DisplayResizedEventCallback() */

/******************************************************************************
*
*   PROCEDURE NAME:
*       CableAttachHandler 
*
*   DESCRIPTION:
*       Gets called when a cable is attached to the PDA port.
*
******************************************************************************/
Err CableAttachHandler( SysNotifyParamType *notifyParamsP )
{
	device.CableAttachStatus = true;

//	FrmCustomAlert( WarningAlert, "Cable Attach Event", "", "");


	//----------------------------------------------------------
	//Per the documentation return 0.
	//----------------------------------------------------------
	return(0);

} /* CableAttachHandler() */

/******************************************************************************
*
*   PROCEDURE NAME:
*       CableDetachHandler 
*
*   DESCRIPTION:
*       Gets called when a cable is attached to the PDA port.
*
******************************************************************************/
Err CableDetachHandler( SysNotifyParamType *notifyParamsP )
{
	device.CableAttachStatus = false;

//	FrmCustomAlert( WarningAlert, "Cable Detach Event", "", "");

	//----------------------------------------------------------
	//Per the documentation return 0.
	//----------------------------------------------------------
	return(0);

} /* CableDetachEventHandler() */
