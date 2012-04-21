#include <PalmOS.h>	// all the system toolbox headers
#include "soaring.h"
#include "soarForm.h"
#include "soarUtil.h"
#include "soarMem.h"
#include "soarNMEA.h"
#include "soarDB.h"
#include "soarWay.h"
#include "soarMap.h"
#include "soarUMap.h"
#include "soarLog.h"
#include "soarIO.h"
#include "soarConf.h"
#include "soarWind.h"
#include "soarWLst.h"
#include "soarPLst.h"
#include "soarTask.h"
#include "soarGraph.h"
#include "soarSUA.h"
#include "soarComp.h"
#include "soarCAI.h"
#include "soarMath.h"
#include "soarSTF.h"
#include "soarSHA.h"
#include "soarSHAAdd.h"
#include "soarGPSInfo.h"
#include "soarVolk.h"
#include "soarCOL.h"
#include "soarFlrm.h"
#include "soarTer.h"
#include "soarGEOMAG.h"
#include "soarRECO.h"
#include "soarEW.h"
#include "soarWind.h"

static Boolean waytoggle=false;
static Boolean polartoggle=false;
static Boolean tasktoggle=false;
static Boolean configtoggle=false;
static Boolean suatoggle=false;

Boolean taskpreview = false;
Boolean clearrect=false;
double MCMult = 1.0;
Int16 MCXVal = 18;
Int16 MCYVal = 35;
Int16 MCWidth = 2;
double SinkConst;
Char SinkText[4];
Boolean graphing = false;
UInt8 graphtype = 0;
Boolean wayaddinfoedit = true;
Boolean pressedgo = false;
double tskpvwdist = 0.0;
Boolean wasrefwpt = false;
Boolean wenttoedit = false;
Boolean	newTaskWay = false;
Boolean	emergencyland = false;
Boolean frommenuorchain = false;
Boolean goingtotaskscrn = false;
Boolean FltInfoshowMC = false;
UInt32 formopen = 0xffffffff;
Boolean taskactvpressed = false;

extern Boolean defaultrules;
extern double windinc;
extern Boolean fieldupd;
extern double logmapscale;
extern Int8 logmapscaleidx;
extern Int8 logmaporient;
extern FlightData *fltdata;
extern Int16 selectedFltindex;
extern Boolean allowgenalerttap;
extern Boolean wayselect;
extern Boolean allowExit;
extern Boolean inflight;
extern Int16   TextOff;
extern Int16   activetskway;
extern Boolean savtaskonhold;
extern Int16   taskIndex;
extern IndexedColorType indexSUA, indexSUAwarn, indexTask, indexSector, indexWaypt, indexBlack, indexWhite;
extern IndexedColorType indexSink, indexWeak, indexStrong;
extern TaskData *edittsk;
extern Boolean activetasksector;
extern TaskData *tsktoedit;
extern Boolean IsMap;
extern Boolean draw_log;
extern Boolean chgstartaltref;
extern DateTimeType curtime;
extern Int8 xfrdialog;
extern Char transfer_filename[30];
extern Int8 io_file_type;
extern Int8 io_type;
extern Boolean skipballast;
extern Boolean skipbugs;
extern UInt32 cursecs;
extern UInt32 utcsecs;
extern Int16 defaultscreen;
extern Boolean skipnewflight;
extern UInt32 nogpstime;
extern Boolean logactive;
extern EWMRData *ewmrdata;
extern Boolean recordevent;
extern UInt32 lastevent;
extern Boolean tskoffterrain;

// External variables used by new polar code
extern Int16 		numOfPolars;
extern Int16		currentPolarPage;
extern PolarData	*selectedPolar;
extern PolarData	*inusePolar;
extern Int16 		selectedPolarIndex;
extern Boolean 		newPolar;

// External variables used by new waypoint code
extern Int16		currentWayptPage;
extern WaypointData	*selectedWaypoint;
extern WaypointData	*TempWpt;
extern Int16 		selectedWaypointIndex;
extern Boolean 		newWaypt;
extern UInt16		WayptsortType;
extern Boolean		recv_data;
extern UInt32		origform;
extern UInt32		origform2;

// External variables used by new task code
extern Int16	selectedTaskWayIdx;
extern UInt16	numWaypts;
extern Int16	tskWayAddIdx;
extern Boolean	addWayToTask;
extern Int16	currentTaskPage;
extern Boolean	dispactive;
extern TaskData	*tsk;
extern Int16	inareasector;
extern Boolean	settaskreadonly;
extern Int8	settaskstats;
extern Boolean	exittaskreadonly;
extern Boolean	settaskshowpct;
extern Boolean	draw_task;
extern double	taskctrlat;
extern double	taskctrlon;
extern double	taskmapscale;
extern Boolean	manchgwpt;
extern Int8	AATdist;
extern Boolean	starttask;
extern Boolean	mustActivateAsNew;
extern Int16	minnumWaypts;
extern Int8	CompCmd;
extern Boolean	menuopen;
extern Boolean	updatemap;
extern UInt8	glidingtoheight;
extern Boolean	set_task_changed;
extern Boolean	goingtomenu;
extern Boolean	goingtotaskedit;
extern Boolean	tasknotfinished;
extern Char	timeinfo[15];
extern Boolean	tgtchg;

extern Boolean		taskonhold;
extern Boolean		forceupdate;
extern double		curmapscale;
extern double		savmapscale;
extern double		actualmapscale;
extern UInt8		mapmode;

// External variables for the Alert Windows
extern SUAAlertData 	*suaalert;
extern SUAAlertRet 	*suaalertret;
extern Int16 		startupscreen;
extern UInt32 		warning_time;

// External variables for RECO Instrument Support
extern RECOData		*recodata;

// External variables used by the SUA List code
extern Int16		selectedSUAListIdx;
extern Int16    	currentSUAPage;
extern SUAIndex		*selectedSUA;

// External variables used by the Wind Profile code
extern Boolean		profilegraph;
extern Int16		*profilescale;
extern Int16		profilebase;
extern double		profilestep;

// Main Final Glide Screen
Boolean form_final_glide_event_handler(EventPtr event)
{
	Boolean handled =false;
	Char MCUnitsTxt[6];
	static Char TempChar[10];
	Int16 x;
	RectangleType rectP;
	MemHandle waypoint_hand;
	MemPtr waypoint_ptr;
	static Boolean pen_addinfo = false; 
	Coord  arrowX=0, arrowY=0;
	static RectangleType saverectP;
	static Boolean validpendown = false;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "final_glide-OpenEvent");
			HandleTask(TSKNONE);
			FrmDrawForm(FrmGetActiveForm());
			origform = form_final_glide;
			origform2 = form_final_glide;
			menuopen = false;
			if (data.config.optforspd) {
				frm_set_title("Final Glide - Speed");
				ctl_set_label(form_final_glide_sinkbtn, " MC ");
			} else {
				frm_set_title("Final Glide - Distance");
				ctl_set_label(form_final_glide_sinkbtn, "SINK");
			}
			if (data.config.showrgs) {
				ctl_set_visible(form_final_glide_speedlbl, false);
				ctl_set_visible(form_final_glide_rgslbl, true);
			} else {
				ctl_set_visible(form_final_glide_rgslbl, false);
				ctl_set_visible(form_final_glide_speedlbl, true);
			}
			if (data.config.hwposlabel) {
				field_set_value(form_final_glide_headwind, DblToStr(pround(data.input.headwind*data.input.wndconst,1),1));
			} else {
				field_set_value(form_final_glide_headwind, DblToStr(pround(data.input.headwind*data.input.wndconst*(-1.0),1),1));
			}
			field_set_value(form_final_glide_basemc, DblToStr(pround(data.input.basemc*data.input.lftconst,	data.input.lftprec+1), data.input.lftprec));

			switch (data.config.altreftype) {
				case MSL:
					WinDrawChars(data.input.alttext, 2, 136, 68);
					ctl_set_label(form_final_glide_curaltbtn1, "MSL");
					break;
				case QFE:
					WinDrawChars(data.input.alttext, 2, 136, 68);
					ctl_set_label(form_final_glide_curaltbtn1, "QFE");
					break;
				case AGL:
					WinDrawChars(data.input.alttext, 2, 136, 68);
					ctl_set_label(form_final_glide_curaltbtn1, "AGL");
					break;
				case PALT:
					// FL always in ft
					WinDrawChars("ft", 2, 136, 68);
					ctl_set_label(form_final_glide_curaltbtn1, "FL ");
					break;
				default:
					break;
			}

			switch (data.config.alttype) {
				case REQALT:
					ctl_set_label(form_final_glide_altbtn, "R.ALT");
					break;
				case ARVALT:
					ctl_set_label(form_final_glide_altbtn, "A.ALT");
					break;
				case DLTALT:
					ctl_set_label(form_final_glide_altbtn, "D.ALT");
					break;
				default:
					break;
			}

			rectP.topLeft.x = GPSX;
			rectP.topLeft.y = GPSY;
			rectP.extent.x = GPSXOFF+1;
			rectP.extent.y = GPSYOFF;
			WinEraseRectangle(&rectP, 0);
			if (!recv_data || (nogpstime > 0)) {
				FntSetFont(boldFont);
				WinDrawInvertedChars(" NO GPS", 7, GPSX, GPSY);
				FntSetFont(stdFont);
				clearrect = true;
			} else if (StrCompare(data.logger.gpsstat, "V") == 0) {
				FntSetFont(boldFont);
				WinDrawInvertedChars("NOSATS", 6, GPSX, GPSY);
				FntSetFont(stdFont);
				clearrect = true;
			} else {
				if (clearrect) {
					WinEraseRectangle(&rectP, 0);
					clearrect = false;
				}
				StrCopy(TempChar, "G");
				StrCat(TempChar, data.input.gpsnumsats);
				field_set_value(form_final_glide_gpsstat, TempChar);
//				clearrect = false;
			}

			switch (data.config.altunits) {
				case NAUTICAL:
					ctl_set_label(form_final_glide_altbtn1, "      (ft)");
					break;
				case METRIC:
					ctl_set_label(form_final_glide_altbtn1, "      (m)");
					break;
				default:
					break;
			}

			StrCopy(TempChar, "(");
			StrCat(TempChar, data.input.spdtext);
			StrCat(TempChar, ")");
			WinDrawChars(TempChar, 5, 29, 24);
			WinDrawChars(data.input.wndtext, 3, 136, 34-2);
			WinDrawChars(data.input.lfttext, 3, 136, 52-2);
			WinDrawChars(data.input.lfttext, 3, 136, 88-2);
//			WinDrawChars(data.input.lfttext, 3, 136, 106);
			WinDrawChars(data.input.alttext, 2, 136, 124-7);
			WinDrawChars(data.input.spdtext, 3,  45, 147);
			WinDrawChars(data.input.distext, 2, 145, 147);

			// This takes care of the zero value
			switch (data.config.lftunits) {
				case STATUTE:
					// fpm
					MCMult = 100;
					MCXVal = 8;
					MCWidth = 4;
					StrCopy(MCUnitsTxt, " 000");
					ctl_set_label(form_final_glide_sinkbtn1,"(fpm)");
					break;
				case NAUTICAL:
					// kts
					MCMult = 1;
					MCXVal = 18;
					MCWidth = 2;
					StrCopy(MCUnitsTxt, " 0");
					ctl_set_label(form_final_glide_sinkbtn1,"(kts)");
					break;
				case METRIC:
					// m/s
					MCMult = 0.5;
					MCXVal = 8;
					MCWidth = 4;
					StrCopy(MCUnitsTxt, " 0.0");
					ctl_set_label(form_final_glide_sinkbtn1,"(m/s)");
					break;
				default:
					break;
			}

			WinDrawChars(MCUnitsTxt, MCWidth, MCXVal, MCYVal);
			// This takes care of the rest of the values
			for(x=1;x<6;x++) {
				StrCopy(MCUnitsTxt, DblToStr(x*MCMult*data.config.mcrngmult, (data.config.lftunits==METRIC?1:0)));
				WinDrawChars(leftpad(MCUnitsTxt, ' ', MCWidth), MCWidth, MCXVal, MCYVal+x*12);
			}
			// This forces the redraw of the various values when the form is
			// first drawn.  Useful with using with manual distance.
			data.application.changed = 1;
			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "final_glide-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
//			HostTraceOutputTL(appErrorClass, "final_glide-winEnterEvent");
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "final_glide-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "final_glide-frmCloseEvent-menuopen = false");
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
			case form_final_glide_altbtn:
			case form_final_glide_altbtn1:
				switch (data.config.alttype) {
					case REQALT:
						ctl_set_label(form_final_glide_altbtn, "A.ALT");
						data.config.alttype = ARVALT;
						data.application.changed = 1;
						break;
					case ARVALT:
						ctl_set_label(form_final_glide_altbtn, "D.ALT");
						data.config.alttype = DLTALT;
						data.application.changed = 1;
						break;
					case DLTALT:
						ctl_set_label(form_final_glide_altbtn, "R.ALT");
						data.config.alttype = REQALT;
						data.application.changed = 1;
						break;
					default:
						break;
				}
				handled = true;
				break;
			case form_final_glide_curaltbtn1:
				switch ( data.config.altreftype ) {
					case MSL:
						WinDrawChars(data.input.alttext, 2, 136, 68);
						ctl_set_label(form_final_glide_curaltbtn1, "AGL");
						data.config.altreftype = AGL;
						break;
					case AGL:
						WinDrawChars(data.input.alttext, 2, 136, 68);
						ctl_set_label(form_final_glide_curaltbtn1, "QFE");
						data.config.altreftype = QFE;
						break;
					case QFE:
						// FL always in ft
						WinDrawChars("ft  ", 4, 136, 68);
						ctl_set_label(form_final_glide_curaltbtn1, "FL ");
						data.config.altreftype = PALT;
						break;
					case PALT:
						WinDrawChars(data.input.alttext, 2, 136, 68);
						ctl_set_label(form_final_glide_curaltbtn1, "MSL");
						data.config.altreftype = MSL;
						break;
					default:
						break;
				}
				data.application.changed = 1;
				handled = true;
				break;
			case form_final_glide_hwbtn:
				FrmGotoForm( form_wind_disp );
				handled = true;
				break;
			case form_final_glide_sinkbtn:
			case form_final_glide_sinkbtn1:
			case form_final_glide_sinkbtn2:
				if (data.config.mcrngmult==1) {
					data.config.mcrngmult = 2;
				} else {
					data.config.mcrngmult = 1;
				}
				// Erase the Current MC/Sink Values on Left
				rectP.topLeft.x = 0;
				rectP.topLeft.y = MCYVal;
				rectP.extent.x = 27;
				rectP.extent.y = 110-MCYVal;
				WinEraseRectangle(&rectP, 0);
				// This takes care of the zero value
				switch (data.config.lftunits) {
					case STATUTE:
						// fpm
						MCMult = 100;
						MCXVal = 8;
						MCWidth = 4;
						StrCopy(MCUnitsTxt, " 000");
						ctl_set_label(form_final_glide_sinkbtn1,"(fpm)");
						break;
					case NAUTICAL:
						// kts
						MCMult = 1;
						MCXVal = 18;
						MCWidth = 2;
						StrCopy(MCUnitsTxt, " 0");
						ctl_set_label(form_final_glide_sinkbtn1,"(kts)");
						break;
					case METRIC:
						// m/s
						MCMult = 0.5;
						MCXVal = 8;
						MCWidth = 4;
						StrCopy(MCUnitsTxt, " 0.0");
						ctl_set_label(form_final_glide_sinkbtn1,"(m/s)");
						break;
					default:
						break;
				}
				WinDrawChars(MCUnitsTxt, MCWidth, MCXVal, MCYVal);
				// This takes care of the rest of the values
				for(x=1;x<6;x++) {
					StrCopy(MCUnitsTxt, DblToStr(x*MCMult*data.config.mcrngmult, (data.config.lftunits==METRIC?1:0)));
					WinDrawChars(leftpad(MCUnitsTxt, ' ', MCWidth), MCWidth, MCXVal, MCYVal+x*12);
				}
				// This forces the redraw of the various values when the form is
				// first drawn.  Useful with using with manual distance.
				data.application.changed = 1;
				handled = true;
				break;
			case form_final_glide_liftbtn:
			case form_final_glide_aliftbtn:
				PlayKeySound();
				FrmGotoForm(form_thermal_history);
				handled = true;
				break;
			default:
				break;
			}
			break;
		case fldEnterEvent:
			switch (event->data.ctlEnter.controlID) {
			case form_final_glide_curalt:
				PlayKeySound();
				switch ( data.config.altreftype ) {
					case MSL:
						WinDrawChars(data.input.alttext, 2, 136, 68);
						ctl_set_label(form_final_glide_curaltbtn1, "AGL");
						data.config.altreftype = AGL;
						break;
					case AGL:
						WinDrawChars(data.input.alttext, 2, 136, 68);
						ctl_set_label(form_final_glide_curaltbtn1, "QFE");
						data.config.altreftype = QFE;
						break;
					case QFE:
						// FL always in ft
						WinDrawChars("ft  ", 4, 136, 68);
						ctl_set_label(form_final_glide_curaltbtn1, "FL ");
						data.config.altreftype = PALT;
						break;
					case PALT:
						WinDrawChars(data.input.alttext, 2, 136, 68);
						ctl_set_label(form_final_glide_curaltbtn1, "MSL");
						data.config.altreftype = MSL;
						break;
					default:
						break;
				}
				data.application.changed = 1;
				handled = true;
				break;
			case form_final_glide_headwind:
				PlayKeySound();
				FrmGotoForm( form_wind_disp );
				handled = true;
				break;
			case form_final_glide_waypt:
			case form_final_glide_wayelev:
				pen_addinfo = true; // required to stop exit button on add.info screen generating a pendown event
				if (data.input.destination_valid) {
					PlayKeySound();
					wayaddinfoedit = false;
					// load Waypoint into TempWpt from waypoint database
					if (OpenDBQueryRecord(waypoint_db, FindWayptRecordByName(data.input.destination_name), &waypoint_hand, &waypoint_ptr)) {
						MemMove(TempWpt,waypoint_ptr,sizeof(WaypointData));
						MemHandleUnlock(waypoint_hand);
						FrmGotoForm( form_waypt_addinfo );
					}
				}
				handled = true;
				break;
			case form_final_glide_track:
			case form_final_glide_gs:
				PlayKeySound();
				if ((inareasector > -1) && (data.config.AATmode & AAT_MTURN) && ((data.task.waypttypes[inareasector] & AREAEXIT) == 0)) {
					// open manual turn in AAT question
					question->type = QturnAAT;
					FrmPopupForm(form_question);
				} else {
					select_fg_waypoint(WAYSELECT);
				}
				handled = true;
				break;
			case form_final_glide_bearing:
			case form_final_glide_dis:
				PlayKeySound();
				taskIndex = 0;
				dispactive = true;
				settaskreadonly = false;
				FrmGotoForm(form_set_task);
				data.application.changed = 1;
				handled = true;
				break;
			case form_final_glide_lift:
			case form_final_glide_alift:
				PlayKeySound();
				FrmGotoForm(form_thermal_history);
				handled = true;
				break;
			case form_final_glide_0_alt:
			case form_final_glide_1_alt:
			case form_final_glide_2_alt:
			case form_final_glide_3_alt:
			case form_final_glide_4_alt:
			case form_final_glide_5_alt:
				switch (data.config.alttype) {
					case REQALT:
						ctl_set_label(form_final_glide_altbtn, "A.ALT");
						data.config.alttype = ARVALT;
						data.application.changed = 1;
						break;
					case ARVALT:
						ctl_set_label(form_final_glide_altbtn, "D.ALT");
						data.config.alttype = DLTALT;
						data.application.changed = 1;
						break;
					case DLTALT:
						ctl_set_label(form_final_glide_altbtn, "R.ALT");
						data.config.alttype = REQALT;
						data.application.changed = 1;
						break;
					default:
						break;
				}
				handled = true;
				break;
			case form_final_glide_basemc:
				if (data.config.MCbutton == POPUP) FrmPopupForm(form_set_mc);
				handled = true;
				break;
			default:
				break;
			}
		case penDownEvent:
			validpendown = false;
			arrowX = ARROWCENTX - ARROWCENTXOFF;
			arrowY = ARROWCENTY - ARROWCENTYOFF;
			RctSetRectangle (&rectP, arrowX, arrowY, ARROWAREAWIDTH, ARROWAREAWIDTH);
//			HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//			HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
			if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
				WinInvertRectangle (&rectP, 0);
				validpendown = true;
				saverectP = rectP;
				pen_addinfo = false;
				handled=true;
			}
			break;
		case penUpEvent:
			// not needed on low res screen
			//if (device.HiDensityScrPresent) {
			//		WinSetCoordinateSystem(kCoordinatesStandard);
			//	event->screenX = WinScaleCoord (event->screenX, true);
			//	event->screenY = WinScaleCoord (event->screenY, true);
//				HostTraceOutputTL(appErrorClass, "screenX=|%hd|", event->screenX);
//				HostTraceOutputTL(appErrorClass, "screenY=|%hd|", event->screenY);
			//}
			// Modified as this is a low res screen, was:-
			// arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*16);
			// Waypoint Direction Arrow Area - Select Waypoint

			if (validpendown) {
				WinInvertRectangle(&saverectP, 0);	
			}

			arrowX = ARROWCENTX - ARROWCENTXOFF;
			arrowY = ARROWCENTY - ARROWCENTYOFF;
			RctSetRectangle (&rectP, arrowX, arrowY, ARROWAREAWIDTH, ARROWAREAWIDTH);
//			HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//			HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
			if (RctPtInRectangle (event->screenX, event->screenY, &rectP) && (validpendown) && (!pen_addinfo)) {
				PlayKeySound();
				if ((data.task.numwaypts > 0) && !taskonhold) {
					if (inareasector > -1) {
						selectedTaskWayIdx = inareasector;
					} else {
						selectedTaskWayIdx = activetskway;
					}
					activetasksector = true;
					tsktoedit = &data.task;
					FrmGotoForm(form_waypoint_sector);
				}
				handled = true;					
			}
			validpendown = false;
			break;

		default:
			break;
	}
	return handled;
}

Boolean form_moving_map_event_handler(EventPtr event)
{
	Boolean handled=false;
	RectangleType rectP;
	static RectangleType saverectP;
	static Boolean validpendown = false;
	Coord  arrowX=0, arrowY=0;   
	static Boolean pendown = false;
	FormType *pfrm = FrmGetActiveForm();
	EventType newEvent;
	double xfactor, yfactor, truecse, step;
	static Int16 postskway = 0;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "moving_map-OpenEvent");
			FrmDrawForm(pfrm);
			if (draw_log) {
				origform = form_final_glide;
				origform2 = form_final_glide;
			} else {
				origform = form_moving_map;
				origform2 = form_moving_map;
			}
			updatemap = true;
			if (device.DIACapable && !draw_log)  {
				//----------------------------------------------------------
				//Resize the form.
				//----------------------------------------------------------
				rectP.topLeft.x = 0;
				rectP.topLeft.y = 0;
				rectP.extent.x = 320;
				rectP.extent.y = 480;
				WinSetWindowBounds( FrmGetWindowHandle( pfrm ), &rectP );

				//------------------------------------------------------
				//Set this forms's constraints.
				//------------------------------------------------------
				WinSetConstraintsSize( FrmGetWindowHandle(pfrm), 160, 160, 240, 160, 160, 160 );

				//------------------------------------------------------
				//Set the form's dynamic Input Area Policy.
				//------------------------------------------------------
				FrmSetDIAPolicyAttr( pfrm, frmDIAPolicyCustom );

				//------------------------------------------------------
				//Enable the input trigger.
				//------------------------------------------------------
				PINSetInputTriggerState( pinInputTriggerDisabled );

				//------------------------------------------------------
				//If your application wanted to close the input area to
				//have as much space available as possible, you would
				//use the following call.
				//------------------------------------------------------
				PINSetInputAreaState( pinInputAreaClosed );

				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_moving_map_wayselbtn)), 4, 208);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_moving_map_homebtn)), 82, 208);
				ctl_set_visible(form_moving_map_homebtn, true);
				ctl_set_visible(form_moving_map_wayselbtn, true);
			}

			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}

			// set up map scale, orientation and scale
			if (!menuopen && (!recv_data || draw_log || (recv_data && (StrCompare(data.logger.gpsstat, "A") != 0)))) {
				if (draw_log) {
					if (draw_task) {
						if (taskpreview) {
							// goto first waypoint
							postskway = 0;
							data.input.gpslatdbl = data.task.targetlats[postskway];
							data.input.gpslngdbl = data.task.targetlons[postskway];
							activetskway = 1;
							data.input.true_track.value = data.task.bearings[activetskway];
							data.input.true_track.valid = VALID;
							tskpvwdist = 0.0;
							data.input.curmaporient = TRACKUP;
						} else {
							// center on flight
							data.input.gpslatdbl = taskctrlat;
							data.input.gpslngdbl = taskctrlon;
							curmapscale = taskmapscale;
							actualmapscale = curmapscale*data.input.disconst;
							data.input.curmaporient = NORTHUP;
						}
					} else {
						getfirstloggedpoint(selectedFltindex);
						data.input.curmaporient = NORTHUP;
					}
					updatemap = true;
				} else if (FindHomeWayptInitialize()) {
					updatemap = true;
				}
				// Calculate the GPS Variation
				data.input.deviation.value = GetDeviation();
				data.input.deviation.valid = VALID;
			}
			HandleTask(TSKNONE);
			FrmDrawForm(pfrm);
			handled=true;
			break;
		case winDisplayChangedEvent:
//			HostTraceOutputTL(appErrorClass, "winDisplayChangedEvent");
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
			if (device.DIACapable && !draw_log)  {
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_moving_map_wayselbtn)), 4, 208);
				FrmSetObjectPosition (pfrm, (FrmGetObjectIndex(pfrm, form_moving_map_homebtn)), 82, 208);
				ctl_set_visible(form_moving_map_homebtn, true);
				ctl_set_visible(form_moving_map_wayselbtn, true);
			}
			FrmDrawForm(pfrm);
			handled=true;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "moving_map-frmCloseEvent");
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
			if (device.DIACapable) {
				PINSetInputAreaState( pinInputAreaOpen );
			}
			if (draw_task) {
				activetskway = 0;
				draw_task = false;
				if (data.task.numwaypts > 0) CalcStartFinishDirs(&data.task);
			}
			if (draw_log) {
				draw_log = false;
				curmapscale = logmapscale;
				data.config.mapscaleidx = logmapscaleidx;
				actualmapscale = curmapscale * data.input.disconst;
				data.input.curmaporient = logmaporient;
				FindHomeWayptInitialize();
			}
			//HandleTaskAutoZoom(0.0, 0.0, true);
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "moving_map-winExitEvent");
//			HostTraceOutputTL(appErrorClass, "menuopen = true");
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
			menuopen = true;
			handled = true;
			break;
		case winEnterEvent:
//			HostTraceOutputTL(appErrorClass, "moving_map-winEnterEvent");
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "winEnterEvent-inside");
//				HostTraceOutputTL(appErrorClass, "menuopen = false");
				menuopen = false;
			}
			handled=true;
			break;
		case ctlSelectEvent:
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
				case form_moving_map_homebtn:
//					HostTraceOutputTL(appErrorClass, "homebtn pressed");
					EvtEnqueueKey (findChr, 0, commandKeyMask);
//					FrmGotoForm(form_final_glide);
					break;
				case form_moving_map_wayselbtn:
//					HostTraceOutputTL(appErrorClass, "wayselbtn pressed");
					// This is a sneaky way of reusing the waypoint select
					// code for the CalcButton in PreProcessEvent
					newEvent.eType = keyDownEvent;
					newEvent.data.keyDown.chr = calcChr;
					PreprocessEvent(&newEvent);
					break;
				case form_moving_map_backbtn:
					if (draw_task) {
						activetskway = 0;
						FrmGotoForm(form_list_task);
					} else {
						FrmGotoForm(form_flt_info);
					}
					break;	
				default:
					break;
			}
			handled=true;
			break;
		case penDownEvent:
			pendown = true;
			validpendown = false;
			
//			HostTraceOutputTL(appErrorClass, "penDownEvent-FormID=|%hu|", FrmGetActiveFormID());
//			HostTraceOutputTL(appErrorClass, "Raw screenX=|%hd|", event->screenX);
//			HostTraceOutputTL(appErrorClass, "Raw screenY=|%hd|", event->screenY);
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(device.HiDensityScrPresent);
				event->screenX = WinScaleCoord (event->screenX, true);
				event->screenY = WinScaleCoord (event->screenY, true);
//				HostTraceOutputTL(appErrorClass, "screenX=|%hd|", event->screenX);
//				HostTraceOutputTL(appErrorClass, "screenY=|%hd|", event->screenY);
			}

			if (!draw_log) {
				// goto active task by tapping top right (task leg distance)
				arrowX = SCREEN.WIDTH-(SCREEN.SRES*22);
				arrowY = HEIGHT_MIN;
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22));
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
			}
			
//			if (!draw_log) {
				// select waypoint or active task options by tapping CSE: box
				arrowX = WIDTH_MIN+(SCREEN.SRES*1); // 2
				arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff; //138
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					if (!draw_log) {
						WinInvertRectangle (&rectP, 0);
						validpendown = true;
						saverectP = rectP;
					} else if (device.StyleTapPDA == STIPHONE) {
						// work around for button on map screen with StyleTap
						newEvent.eType = ctlSelectEvent;
						newEvent.data.ctlEnter.controlID = form_moving_map_backbtn;
						EvtAddEventToQueue(&newEvent);
					}
					handled=true;
				}
//			}

			if (!draw_log || (draw_task && (taskIndex == 0))) {
				// goto view sector by tapping top left (task bearing)
				arrowX = WIDTH_MIN;
				arrowY = HEIGHT_MIN;
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22));
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
			}

			if (!draw_log || (draw_task && taskpreview)) {
				// Invert the Scale Selection Area
				arrowX = SCREEN.WIDTH-(SCREEN.SRES*24);
				arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff;
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*24), (SCREEN.SRES*22));
//				HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//				HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
			}

			// Invert the R/D/A.Alt Selection Area
			if (data.config.btmlabels && !draw_log) {
				arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*15); //65
				arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff; //138
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*35), (SCREEN.SRES*22)); 
//				HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//				HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
			}

			// Invert the Altitude Selection Area
			if (data.config.btmlabels && !draw_log) {
				arrowX = SCREEN.WIDTH-(SCREEN.SRES*55); 
				arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff;
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*30), (SCREEN.SRES*22)); 
//				HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//				HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
			}

			if (!menuopen && (!recv_data || draw_log || (recv_data && (StrCompare(data.logger.gpsstat, "A") != 0)))) {

				// Move Left Button
				arrowX = WIDTH_MIN;
				arrowY = SCREEN.HEIGHT/2-(SCREEN.SRES*11);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
					FntSetFont(symbolFont);
					//Left
					//WinDrawInvertedChars("\003", 1, arrowX, arrowY);
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					FntSetFont(stdFont);
					handled=true;
				}

				// Move Right Button
				arrowX = SCREEN.WIDTH-(SCREEN.SRES*22);
				arrowY = SCREEN.HEIGHT/2-(SCREEN.SRES*11);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
					FntSetFont(symbolFont);
					//Right
					//WinDrawInvertedChars("\004", 1, arrowX, arrowY);
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					FntSetFont(stdFont);
					handled=true;
				}

				// Move Up Button
				arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*11);
				arrowY = HEIGHT_MIN;
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
					FntSetFont(symbolFont);
					//Up
					//WinDrawInvertedChars("\005", 1, arrowX, arrowY);
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					FntSetFont(stdFont);
					handled=true;
				}

				// Move Down Button
				if (data.config.btmlabels && !draw_log) {
					arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*11);
					arrowY = SCREEN.HEIGHT-(SCREEN.SRES*43);
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				} else {
					arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*11);
					arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22);
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				}
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
					FntSetFont(symbolFont);
					//Down
					//WinDrawInvertedChars("\006", 1, arrowX, arrowY);
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					FntSetFont(stdFont);
					handled=true;
				}

				// Reset Button
				arrowX = SCREEN.WIDTH/2+(SCREEN.SRES*15);
				arrowY = HEIGHT_MIN;
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*35), (SCREEN.SRES*15)); 
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
					FntSetFont(largeBoldFont);
					//Reset
					WinDrawInvertedChars("RESET", 5, arrowX, arrowY);
					FntSetFont(stdFont);
					handled=true;
				}

			} else if (IsMap) { // only valid on moving map screen where glider symbol is in the centre
				// glider symbol centre of screen
				arrowX = SCREEN.GLIDERX-(SCREEN.SRES*30); 
				arrowY = SCREEN.GLIDERY-(SCREEN.SRES*30);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*60), (SCREEN.SRES*60));
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					WinInvertRectangle (&rectP, 0);
					validpendown = true;
					saverectP = rectP;
					handled=true;
				}
			}

			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
			break;

		case penUpEvent:
			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(device.HiDensityScrPresent);
				event->screenX = WinScaleCoord (event->screenX, true);
				event->screenY = WinScaleCoord (event->screenY, true);
//				HostTraceOutputTL(appErrorClass, "screenX=|%hd|", event->screenX);
//				HostTraceOutputTL(appErrorClass, "screenY=|%hd|", event->screenY);
			}

			// un-invert saved rectangle
			if (validpendown) {
				WinInvertRectangle(&saverectP, 0);
			}

			// goto view sector by tapping top left (task bearing)
			arrowX = WIDTH_MIN;
			arrowY = HEIGHT_MIN;
			RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
//			HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//			HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
			if (RctPtInRectangle (event->screenX, event->screenY, &rectP) && (validpendown)) {
//				HostTraceOutputTL(appErrorClass, "active task pressed");
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(kCoordinatesStandard);
				}
				if (!draw_log && (data.task.numwaypts > 0) && !taskonhold) {
					PlayKeySound();
					// goto waypoint sector
					if (inareasector > -1) {
						selectedTaskWayIdx = inareasector;
					} else {
						selectedTaskWayIdx = activetskway;
					}
					activetasksector = true;
					tsktoedit = &data.task;
					FrmGotoForm(form_waypoint_sector);
				} else if (draw_task) {
					PlayKeySound();
					// toggle task preview mode
					taskpreview = !taskpreview;
					if (taskpreview) {
						// goto first waypoint
						postskway = 0;
						data.input.gpslatdbl = data.task.targetlats[postskway];
						data.input.gpslngdbl = data.task.targetlons[postskway];
						activetskway = 1;
						data.input.true_track.value = data.task.bearings[activetskway];
						data.input.true_track.valid = VALID;
						tskpvwdist = 0.0;
						data.input.curmaporient = TRACKUP;
					} else {
						// center on flight
						data.input.gpslatdbl = taskctrlat;
						data.input.gpslngdbl = taskctrlon;
						curmapscale = taskmapscale;
						actualmapscale = curmapscale*data.input.disconst;
					}
					updatemap = true;
				}
				handled=true;
			}

			// goto active task by tapping top right (task leg distance)
			arrowX = SCREEN.WIDTH-(SCREEN.SRES*22);
			arrowY = HEIGHT_MIN;
			RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
//			HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//			HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
			if (RctPtInRectangle (event->screenX, event->screenY, &rectP) && (validpendown)) {
//				HostTraceOutputTL(appErrorClass, "active task pressed");
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(kCoordinatesStandard);
				}
				if (!draw_log) {
					PlayKeySound();
					taskIndex = 0;
					dispactive = true;
					settaskreadonly = false;
					FrmGotoForm(form_set_task);
				}
				handled=true;
			}

			// select waypoint or active task options by tapping CSE: box
			arrowX = WIDTH_MIN+(SCREEN.SRES*1); // 2
			arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff; //138
			RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
//			HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//			HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
			if (RctPtInRectangle (event->screenX, event->screenY, &rectP) && (validpendown)) {
//				HostTraceOutputTL(appErrorClass, "wayselbtn pressed");
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(kCoordinatesStandard);
				}
				if (!draw_log) {
					PlayKeySound() ;
					if ((inareasector > -1) && (data.config.AATmode & AAT_MTURN) && ((data.task.waypttypes[inareasector] & AREAEXIT) == 0)) {
						// open manual turn in AAT question
						question->type = QturnAAT;
						FrmPopupForm(form_question);
					} else {
						select_fg_waypoint(WAYSELECT);
					}
				}
				handled=true;
			}

			if (!menuopen && (!recv_data || draw_log || (recv_data && (StrCompare(data.logger.gpsstat, "A") != 0)))) {
				// find true course
				if (data.input.curmaporient == TRACKUP) {
					truecse = data.input.true_track.value;
				} else if ((data.input.curmaporient == COURSEUP) && (data.input.bearing_to_destination.valid==VALID)) {
					truecse = nice_brg(data.input.bearing_to_destination.value - data.input.deviation.value);
				} else {
					truecse = nice_brg(0.0 - data.input.deviation.value);
				}
				// set movement factors
				yfactor = (curmapscale/60.0*0.25) * Cos(DegreesToRadians(truecse));
				xfactor = (curmapscale/60.0*0.25) * Sin(DegreesToRadians(truecse));
				if (data.task.distances[activetskway] > 0.0) {
					step = (curmapscale*0.25) / data.task.distances[activetskway];
				} else {
					step = 0.0;
				}

				// Move Left Button
				arrowX = WIDTH_MIN;
				arrowY = SCREEN.HEIGHT/2-(SCREEN.SRES*11);
//				HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//				HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					//Left
					PlayKeySound();
					FntSetFont(symbolFont);
					//WinDrawChars("\003", 1, arrowX, arrowY);
					//WinDrawChars("\003", 1, WIDTH_MIN+(SCREEN.SRES*1), SCREEN.HEIGHT/2-(SCREEN.SRES*6));
					WinInvertRectangle (&rectP, 0);
					FntSetFont(stdFont);
					if (draw_task && taskpreview) { // && (data.task.numwaypts > 0) && !taskonhold) {
						// previous turnpoint
						if (tskpvwdist == 0.0) {
							if ((activetskway > 1) && (postskway < activetskway)) activetskway--;
							data.input.true_track.value = data.task.bearings[activetskway];
							data.input.true_track.valid = VALID;
							if (postskway > 0)  postskway--;
						}
						tskpvwdist = 0.0;
						data.input.gpslatdbl = data.task.targetlats[postskway];
						data.input.gpslngdbl = data.task.targetlons[postskway];
					} else {
						updateposition(xfactor*data.input.coslat, -yfactor);
					}
					updatemap = true;
					handled=true;
				}
				// Move Right Button
				arrowX = SCREEN.WIDTH-(SCREEN.SRES*22);
				arrowY = SCREEN.HEIGHT/2-(SCREEN.SRES*11);
//				HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//				HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					//Right
					PlayKeySound();
					FntSetFont(symbolFont);
					//WinDrawChars("\004", 1, arrowX, arrowY);
					//WinDrawChars("\004", 1, SCREEN.WIDTH-(SCREEN.SRES*11), SCREEN.HEIGHT/2-(SCREEN.SRES*6));
					WinInvertRectangle (&rectP, 0);
					FntSetFont(stdFont);
					if (draw_task && taskpreview) { // && (data.task.numwaypts > 0) && !taskonhold) {
						// next turnpoint
						if (postskway < data.task.numwaypts-1) postskway++;
						if (activetskway < data.task.numwaypts-1) activetskway++;
						data.input.true_track.value = data.task.bearings[activetskway];
						data.input.true_track.valid = VALID;
						tskpvwdist = 0.0;
						if (postskway == activetskway) {
							tskpvwdist = 1.0;
							postskway--;
						}
						data.input.gpslatdbl = tskpvwdist*data.task.targetlats[activetskway] + (1-tskpvwdist)*data.task.targetlats[postskway];
						data.input.gpslngdbl = tskpvwdist*data.task.targetlons[activetskway] + (1-tskpvwdist)*data.task.targetlons[postskway];
					} else {
						updateposition(-xfactor*data.input.coslat, +yfactor);
					}
					updatemap = true;
					handled=true;
				}
				// Move Up Button
				arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*11);
				arrowY = HEIGHT_MIN;
//				HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//				HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					//Up
					PlayKeySound();
					FntSetFont(symbolFont);
					//WinDrawChars("\005", 1, arrowX, arrowY);
					//WinDrawChars("\005", 1, SCREEN.WIDTH/2-(SCREEN.SRES*5), HEIGHT_MIN+(SCREEN.SRES*1));
					WinInvertRectangle (&rectP, 0);
					FntSetFont(stdFont);
					if (draw_task && taskpreview) { // && (data.task.numwaypts > 0) && !taskonhold) {
						// at turnpoint?
						if (tskpvwdist == 1.0) {
							// next turnpoint
							if (postskway < data.task.numwaypts-1) postskway++;
							if (activetskway < data.task.numwaypts-1) activetskway++;
							data.input.true_track.value = data.task.bearings[activetskway];
							data.input.true_track.valid = VALID;
							tskpvwdist = 0.0;
							if (postskway == activetskway) {
								tskpvwdist = 1.0;
								postskway--;
							}
						} else {
							// step towards turnpoint
							tskpvwdist += step;
							if (tskpvwdist > 1.0) tskpvwdist = 1.0;
						}
						data.input.gpslatdbl = tskpvwdist*data.task.targetlats[activetskway] + (1-tskpvwdist)*data.task.targetlats[postskway];
						data.input.gpslngdbl = tskpvwdist*data.task.targetlons[activetskway] + (1-tskpvwdist)*data.task.targetlons[postskway];
					} else {
						updateposition(yfactor*data.input.coslat, xfactor);
					}
					updatemap = true;
					handled=true;
				}
				// Move Down Button
				if (data.config.btmlabels && !draw_log) {
					arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*11);
					arrowY = SCREEN.HEIGHT-(SCREEN.SRES*43);
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22)); 
				} else {
					arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*11);
					arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22);
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*22), (SCREEN.SRES*22));
				}
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					//Down
					PlayKeySound();
					FntSetFont(symbolFont);
					//WinDrawChars("\006", 1, arrowX, arrowY);
					//if (data.config.btmlabels && !draw_log) {
					//	WinDrawChars("\006", 1, SCREEN.WIDTH/2-(SCREEN.SRES*5), SCREEN.HEIGHT-(SCREEN.SRES*32));
					//} else {
					//	WinDrawChars("\006", 1, SCREEN.WIDTH/2-(SCREEN.SRES*5), SCREEN.HEIGHT-(SCREEN.SRES*10));
					//}
					WinInvertRectangle (&rectP, 0);
					FntSetFont(stdFont);
					if (draw_task && taskpreview) { // && (data.task.numwaypts > 0) && !taskonhold) {
						// at turnpoint?
						if (tskpvwdist == 0.0) {
							// previous turnpoint
							if ((activetskway > 1) && (postskway < activetskway)) activetskway--;
							data.input.true_track.value = data.task.bearings[activetskway];
							data.input.true_track.valid = VALID;
							tskpvwdist = 0.0;
							if (postskway > 0)  {
								postskway--;
								tskpvwdist = 1.0;
							}
						} else {
							// step backwards from turnpoint
							tskpvwdist -= step;
							if (tskpvwdist < 0.0) tskpvwdist = 0.0;
						}
						data.input.gpslatdbl = tskpvwdist*data.task.targetlats[activetskway] + (1-tskpvwdist)*data.task.targetlats[postskway];
						data.input.gpslngdbl = tskpvwdist*data.task.targetlons[activetskway] + (1-tskpvwdist)*data.task.targetlons[postskway];
					} else {
						updateposition(-yfactor*data.input.coslat, -xfactor);
					}
					updatemap = true;
					handled=true;
				}
				// Reset Button
				arrowX = SCREEN.WIDTH/2+(SCREEN.SRES*15);
				arrowY = HEIGHT_MIN;
//				HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//				HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*35), (SCREEN.SRES*15)); 
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					PlayKeySound();
					FntSetFont(largeBoldFont);
					//Reset
					WinDrawChars("RESET", 5, arrowX, arrowY);
					FntSetFont(stdFont);
					// set up map scale, orientation and scale
					if (draw_log) {
						if (draw_task) {
							if (taskpreview) {
								// goto first waypoint
								postskway = 0;
								data.input.gpslatdbl = data.task.targetlats[postskway];
								data.input.gpslngdbl = data.task.targetlons[postskway];
								activetskway = 1;
								data.input.true_track.value = data.task.bearings[activetskway];
								data.input.true_track.valid = VALID;
								tskpvwdist = 0.0;
								data.input.curmaporient = TRACKUP;
							} else {
								// center on flight
								data.input.gpslatdbl = taskctrlat;
								data.input.gpslngdbl = taskctrlon;
								curmapscale = taskmapscale;
								actualmapscale = curmapscale*data.input.disconst;
								data.input.curmaporient = NORTHUP;
							}
						} else {
							getfirstloggedpoint(selectedFltindex);
							data.input.curmaporient = NORTHUP;
						}
						updatemap = true;
					} else if (FindHomeWayptInitialize()) {
						updatemap = true;
					}
					// Calculate the GPS Variation
					data.input.deviation.value = GetDeviation();
					data.input.deviation.valid = VALID;
					handled=true;
				}
				HandleTask(TSKNONE);
				if (updatemap) InuseWaypointCalcEvent();

			} else if (IsMap) {
				// glider symbol centre of screen
				arrowX = SCREEN.GLIDERX-(SCREEN.SRES*30); 
				arrowY = SCREEN.GLIDERY-(SCREEN.SRES*30);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*60), (SCREEN.SRES*60)); 
//				HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//				HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP) && (validpendown)) {
					PlayKeySound();
//					HostTraceOutputTL(appErrorClass, "active task pressed");
					if (device.HiDensityScrPresent) {
						WinSetCoordinateSystem(kCoordinatesStandard);
					}	
					// mark thermal with current altitude
					//SaveWaypoint(data.input.gpslatdbl, data.input.gpslngdbl, data.input.inusealt, true);
					// mark thermal with current terrain elevation
					SaveWaypoint(data.input.gpslatdbl, data.input.gpslngdbl, data.input.terelev, true);
					updatemap = true;
					handled=true;
				}
			}

			if (data.config.btmlabels && pendown && !draw_log) {
				// Change R/D/A.Alt Modes
				if (validpendown) {
					arrowX = SCREEN.WIDTH/2-(SCREEN.SRES*15);
					arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff;
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*35), (SCREEN.SRES*22)); 
					if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
						PlayKeySound();
						switch (data.config.alttype) {
							case REQALT:
								data.config.alttype = ARVALT;
//								data.application.changed = 1;
								break;
							case ARVALT:
								data.config.alttype = DLTALT;
//								data.application.changed = 1;
								break;
							case DLTALT:
								data.config.alttype = REQALT;
//								data.application.changed = 1;
								break;
							default:
								break;
						}
						updatemap = true;
						//WinInvertRectangle (&rectP, 0);
						handled=true;
					}
				}
				// Invert the Altitude Selection Area
				if (data.config.btmlabels && validpendown && !draw_log) {
					arrowX = SCREEN.WIDTH-(SCREEN.SRES*55); 
					arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff;
					RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*30), (SCREEN.SRES*22)); 
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
					if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
						PlayKeySound();
						switch ( data.config.altreftype ) {
							case MSL:
								data.config.altreftype = AGL;
								break;
							case AGL:
								data.config.altreftype = QFE;
								break;
							case QFE:
								data.config.altreftype = PALT;
								break;
							case PALT:
								data.config.altreftype = MSL;
								break;
							default:
								break;
						}
						updatemap = true;
						//WinInvertRectangle (&rectP, 0);
						handled=true;
					}
				}
				
			}

			// Invert the Scale Selection Area
			// to change the Map Mode
			if (validpendown) {
				arrowX = SCREEN.WIDTH-(SCREEN.SRES*24); 
				arrowY = SCREEN.HEIGHT-(SCREEN.SRES*22)+TextOff;
				// These values are for the glider selection area.
				// Saved for future use.
				/*
				arrowX = SCREEN.GLIDERX-(SCREEN.SRES*10); 
				arrowY = SCREEN.GLIDERY-(SCREEN.SRES*10);
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*20), (SCREEN.SRES*20)); 
				*/
				RctSetRectangle (&rectP, arrowX, arrowY, (SCREEN.SRES*24), (SCREEN.SRES*22)); 
//					HostTraceOutputTL(appErrorClass, "arrowX=|%hd|", arrowX);
//					HostTraceOutputTL(appErrorClass, "arrowY=|%hd|", arrowY);
				if (RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
					PlayKeySound();
					if (mapmode == THERMAL) {
						switch ( data.config.thmaporient ) {
							case NORTHUP:
								data.config.thmaporient = TRACKUP;
								break;
							case TRACKUP:
								data.config.thmaporient = COURSEUP;
								break;
							case COURSEUP:
								data.config.thmaporient = NORTHUP;
								break;
							default:
								break;
						}
					} else {
						switch ( data.config.maporient ) {
							case NORTHUP:
								data.config.maporient = TRACKUP;
								break;
							case TRACKUP:
								data.config.maporient = COURSEUP;
								break;
							case COURSEUP:
								data.config.maporient = NORTHUP;
								break;
							default:
								break;
						}
					}
					updatemap = true;
					//WinInvertRectangle (&rectP, 0);
					handled=true;
				}
			}
			pendown = false;
			validpendown = false;

			if (device.HiDensityScrPresent) {
				WinSetCoordinateSystem(kCoordinatesStandard);
			}
			break;
		default:
			break;
	}
	return handled;
}

Boolean form_flt_info_event_handler(EventPtr event)
{
	Boolean handled=false;
	Int16 nrecs;
//	MemHandle output_hand;
//	MemPtr output_ptr;

	switch (event->eType)
		{
			case frmOpenEvent:
			case frmUpdateEvent:
//				HostTraceOutputTL(appErrorClass, "Open/Update Event");
				menuopen = false;
				FrmDrawForm(FrmGetActiveForm());
				nrecs = OpenDBCountRecords(flight_db);
				if (nrecs > 0) {
					Update_Fltinfo_List(false);
					ctl_set_visible(form_flt_info_graphbtn, false);
					ctl_set_visible(form_flt_info_graphbtn2, false);
					ctl_set_visible(form_flt_info_graphbtn3, false);
					ctl_set_visible(form_flt_info_taskbtn, false);
					frm_set_label(form_flt_info_spdlbl, data.input.tskspdtext);
					frm_set_label(form_flt_info_tskspdlbl, "Avg:");
					FltInfoshowMC = false;
				} else {
					frm_set_label(form_flt_info_spdlbl, data.input.lfttext);
					frm_set_label(form_flt_info_tskspdlbl, "MC:");
					FltInfoshowMC = true;
				}
				forceupdate = true;
				data.application.changed = 1;
				// draw units
				WinDrawChars(data.input.alttext, 2, 149, 30);
				WinDrawChars(data.input.alttext, 2, 149, 45);
				WinDrawChars(data.input.alttext, 2, 82, 61);
				WinDrawChars(data.input.distext, 2, 37, 103);
				WinDrawChars(data.input.distext, 2, 92, 103);
				WinDrawChars(data.input.distext, 2, 146, 103);
				// default to Elev
				ctl_set_visible(form_flt_info_tskstartaltAGL, true);
				ctl_set_visible(form_flt_info_tskstartaltMSL, false);
				chgstartaltref = false;
				handled=true;
				break;
			case winExitEvent:
//				HostTraceOutputTL(appErrorClass, "menuopen = true");
				menuopen = true;
				handled = false;
				break;
			case winEnterEvent:
				if (event->data.winEnter.enterWindow == (WinHandle)FrmGetFirstForm()) {
//					HostTraceOutputTL(appErrorClass, "menuopen = false");
					menuopen = false;
				}
				handled=false;
				break;
			case frmCloseEvent:
//				HostTraceOutputTL(appErrorClass, "Close Event flt_info");
				Update_Fltinfo_List(true);
				handled=false;
				break;
			case popSelectEvent:  // A control button was pressed and released.
				PlayKeySound();
				if (event->data.popSelect.controlID == form_flt_info_pop1) {
					nrecs = OpenDBCountRecords(flight_db);
					selectedFltindex = (nrecs-1) - event->data.popSelect.selection;
					forceupdate = true;
					data.application.changed = 1;
				}
				break;
			case ctlSelectEvent:
				PlayKeySound();
				switch (event->data.ctlEnter.controlID) {
					case form_flt_info_graphbtn:
						graphing = true;
						graphtype = 0;
						FrmGotoForm(form_graph_flt);
						handled=true;
						break;
					case form_flt_info_graphbtn2:
						graphing = true;
						graphtype = 1;
						FrmGotoForm(form_graph_flt);
						handled=true;
						break;
					case form_flt_info_graphbtn3:
						draw_log = true;
						draw_task = false;
						graphing = true;
						logmapscale = curmapscale;
						logmapscaleidx = data.config.mapscaleidx;
						logmaporient = data.input.curmaporient;
						getfirstloggedpoint(selectedFltindex);
						FrmGotoForm(form_moving_map);
						handled=true;
						break;
					case form_flt_info_resetlbl:
						// reset start cruise altitude and distance to target
						data.input.startcruisealt = data.input.inusealt;
						data.input.startcruisedist = data.input.distance_to_destination.value;
						handled = true;
						break;
					case form_flt_info_taskbtn:
						graphing = true;
						newTaskWay = false;
						origform2 = form_flt_info;
						settaskreadonly = true;
						dispactive = false;
						selectedTaskWayIdx = 0; 
						// pre-load tsk with task from flight
						MemMove(tsk, &fltdata->flttask, sizeof(TaskData));
						numWaypts = tsk->numwaypts;
						FrmGotoForm(form_set_task);
						handled = true;
						break;
					default:
						break;
				}
				break;
			case fldEnterEvent:
				switch (event->data.ctlEnter.controlID) {
					case form_flt_info_tskstartalt:
						PlayKeySound();
						// set flag to change start alt ref
						chgstartaltref = !chgstartaltref;
						forceupdate = true;
						Update_Tskinfo_Event();
						break;
					case form_flt_info_tskspd:
						if (FltInfoshowMC) {
							//if (data.config.MCbutton == POPUP) FrmPopupForm(form_set_mc);
							break;
						}
					case form_flt_info_speedreqd:
					case form_flt_info_avgtskspd:
						PlayKeySound();
						switch (data.config.tskspdunits) {
							case METRIC:
								data.config.tskspdunits = STATUTE;
								data.input.tskspdconst = SPDMPHCONST;
								StrCopy(data.input.tskspdtext, "mph"); 
								break;
							case STATUTE:
								data.config.tskspdunits = NAUTICAL;
								data.input.tskspdconst = SPDNAUCONST;
								StrCopy(data.input.tskspdtext, "kts");
								break;
							case NAUTICAL:
								data.config.tskspdunits = METRIC;
								data.input.tskspdconst = SPDKPHCONST;
								StrCopy(data.input.tskspdtext, "kph");
								break;
							default:
								break;
						}
						forceupdate = true;
						Update_Tskinfo_Event();
						break;
					case form_flt_info_ldi:
						PlayKeySound();
						// reset start cruise altitude and distance to target
						data.input.startcruisealt = data.input.inusealt;
						data.input.startcruisedist = data.input.distance_to_destination.value;
						break;
					case form_flt_info_tskstop:
					case form_flt_info_tskarrive:
					case form_flt_info_timetogo:
						if (taskonhold) {
							// resume task
							PlayKeySound();
							select_fg_waypoint(WAYRESUME);
						}
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

Boolean form_graph_flt_event_handler(EventPtr event)
{
	Boolean handled=false;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(FrmGetActiveForm());
			if (graphtype == 0) {
				DrawAltTimeGraph(selectedFltindex, 0);
			} else {
				DrawPctThermalGraph(selectedFltindex, 0);
			}
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
			//graphing = false;
			handled=false;
			break;
		case ctlSelectEvent:
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
				case form_graph_flt_closebtn:
					FrmGotoForm(form_flt_info);
					break;
				default:
					break;
			}
			handled=true;
			break;
		default:
			break;
	}
	return handled;
}

Boolean form_wind_disp_event_handler(EventPtr event) 
{ 
	Boolean handled=false; 
	static Boolean manupd = false;
	static Boolean fieldupd = false;

	switch (event->eType) { 
		case frmOpenEvent: 
		case frmUpdateEvent: 
			origform = form_final_glide; 
			menuopen = false; 
			manupd = false; 
			fieldupd = false; 
			FrmDrawForm(FrmGetActiveForm()); 
			field_set_value(form_wind_disp_awnddir, print_direction2(data.input.wnddir)); 
			field_set_value(form_wind_disp_awndspd, print_horizontal_wndspeed(data.input.wndspd,1)); 
			field_set_value(form_wind_disp_thavglft, print_vertical_speed2(data.input.thavglift,1)); 
			WinDrawChars(data.input.wndtext, 3, 91, 33);
			WinDrawChars(data.input.wndtext, 3, 91, 51);
			WinDrawChars(data.input.lfttext, 3, 100, 69);

			ctl_set_value(form_wind_disp_calchwonoff, data.config.calcwind);
			if (data.config.calcwind) {
				ctl_set_visible(form_wind_disp_mancalc, false);
				ctl_set_visible(form_wind_disp_calcSP, true);
//				ctl_set_label(form_wind_disp_resetcalc, "Reset");
				if (data.config.flightcomp == C302COMP   ||
				    data.config.flightcomp == LXCOMP     ||
				    data.config.flightcomp == LXVARCOMP  ||
				    data.config.flightcomp == FILSERCOMP ||
				    data.config.flightcomp == SN10COMP   ||
				    data.config.flightcomp == B50LXCOMP) {
					ctl_set_visible(form_wind_disp_calccomp, true);
					if (data.config.useinputwinds) {
						ctl_set_value(form_wind_disp_calcSP, false);
						ctl_set_value(form_wind_disp_calccomp, true);
					} else {
						ctl_set_value(form_wind_disp_calcSP, true);
						ctl_set_value(form_wind_disp_calccomp, false);
					}
				} else {
					ctl_set_value(form_wind_disp_calcSP, true);
					ctl_set_visible(form_wind_disp_calccomp, false);
					data.config.useinputwinds = false;
				}
			} else {
				// manual wind calculation
				field_set_value(form_wind_disp_mode, "MANUAL");
				field_set_value(form_wind_disp_turndir, "N/A");
				ctl_set_visible(form_wind_disp_calcSP, false);
				ctl_set_visible(form_wind_disp_calccomp, false);
				ctl_set_visible(form_wind_disp_mancalc, true);
			}
			ctl_set_value(form_wind_disp_usecalchw, data.config.usecalchw);
			ctl_set_value(form_wind_disp_hwposlabel, data.config.hwposlabel); 

			// allows adjustment of wind increment size and precision on rounding
			switch (data.config.wndunits) {
				case METRICMPS:
					windinc = 0.5;
					break;
				case METRIC:
					windinc = 2.5;
					break;
				case STATUTE:
					windinc = 1.0;
					break;
				case NAUTICAL:
					windinc = 1.0;
					break;
				default:
					windinc = 1.0;
					break;
			}
			switch (data.config.windprofileorient) {
				case TRACKUP:
					ctl_set_value(form_wind_disp_trackup, true);
					ctl_set_value(form_wind_disp_northup, false);
					break;
				case NORTHUP:
					ctl_set_value(form_wind_disp_trackup, false);
					ctl_set_value(form_wind_disp_northup, true);
					break;
				default:
					ctl_set_value(form_wind_disp_trackup, false);
					ctl_set_value(form_wind_disp_northup, true);
					break;
			} 
			handled=true; 
			break; 
		case frmCloseEvent: 
			if (!inflight && fieldupd && !manupd) {
				data.input.wnddir = field_get_double(form_wind_disp_winddir); 
				data.input.wndspd = field_get_double(form_wind_disp_windspd) / data.input.wndconst; 
			} 
			handled=false; 
			break; 
		case fldEnterEvent: 
			if ((event->data.ctlEnter.controlID == form_wind_disp_winddir) || (event->data.ctlEnter.controlID == form_wind_disp_windspd)) {
				PlayKeySound();
				manupd = false; 
				fieldupd = true; 
			} 
			handled=false; 
			break; 
		case ctlSelectEvent:  // A control button was pressed and released. 
			PlayKeySound(); 
			switch ( event->data.ctlEnter.controlID ) { 
				case form_wind_disp_calcSP:
					ctl_set_value(form_wind_disp_calcSP, true); 
					ctl_set_value(form_wind_disp_calccomp, false); 
					data.config.useinputwinds = false;
					handled = true;
					break;
				case form_wind_disp_calccomp:
					ctl_set_value(form_wind_disp_calcSP, false); 
					ctl_set_value(form_wind_disp_calccomp, true); 
					data.config.useinputwinds = true;
					handled = true;
					break;
				case form_wind_disp_trackup:
					data.config.windprofileorient = TRACKUP;
					handled = true;
					break;
				case form_wind_disp_northup:
					data.config.windprofileorient = NORTHUP;
					handled = true;
					break;
				case form_wind_disp_3dinfo:
					FrmGotoForm(form_wind_3dinfo);
					handled = true; 
					break; 
				case form_wind_disp_resetcalc: 
					CalcWind(data.input.magnetic_track.value, data.input.ground_speed.value, &data.input.wnddir, &data.input.wndspd, data.config.useinputwinds, true);
					handled = true; 
					break; 
				case form_wind_disp_wndspdplus: 
					data.input.wndspd = increment(field_get_double(form_wind_disp_awndspd), windinc)/data.input.wndconst;
					field_set_value(form_wind_disp_awndspd, print_horizontal_wndspeed(data.input.wndspd,1));
					manupd = true;
					fieldupd = false; 
					handled = true; 
					break; 
				case form_wind_disp_wndspdminus: 
					data.input.wndspd = increment(field_get_double(form_wind_disp_awndspd), -windinc)/data.input.wndconst;
					if (data.input.wndspd < 0.0) data.input.wndspd = 0.0;
					field_set_value(form_wind_disp_awndspd, print_horizontal_wndspeed(data.input.wndspd,1));
					manupd = true;
					fieldupd = false; 
					handled = true; 
					break; 
				case form_wind_disp_wnddirplus: 
					data.input.wnddir = nice_brg(increment(field_get_double(form_wind_disp_awnddir), 10));
					field_set_value(form_wind_disp_awnddir, print_direction2(data.input.wnddir));
					manupd = true;
					fieldupd = false; 
					handled = true; 
					break; 
				case form_wind_disp_wnddirminus:
					data.input.wnddir = nice_brg(increment(field_get_double(form_wind_disp_awnddir), -10));
					field_set_value(form_wind_disp_awnddir, print_direction2(data.input.wnddir));
					manupd = true;
					fieldupd = false; 
					handled = true; 
					break; 
				case form_wind_disp_calchwonoff:
					if (ctl_get_value(form_wind_disp_calchwonoff)) {
						data.config.calcwind = true;
						// reset wind calculation
						CalcWind(data.input.magnetic_track.value, data.input.ground_speed.value, &data.input.wnddir, &data.input.wndspd, data.config.useinputwinds, true);
						ctl_set_visible(form_wind_disp_mancalc, false);
						ctl_set_visible(form_wind_disp_calcSP, true);
//						ctl_set_label(form_wind_disp_resetcalc, "Reset");
						if (data.config.flightcomp == C302COMP   ||
						    data.config.flightcomp == LXCOMP     ||
						    data.config.flightcomp == LXVARCOMP  ||
						    data.config.flightcomp == FILSERCOMP ||
						    data.config.flightcomp == SN10COMP   ||
						    data.config.flightcomp == B50LXCOMP) {
							ctl_set_visible(form_wind_disp_calccomp, true);
							if (data.config.useinputwinds) {
								ctl_set_value(form_wind_disp_calcSP, false);
								ctl_set_value(form_wind_disp_calccomp, true);
							} else {
								ctl_set_value(form_wind_disp_calcSP, true);
								ctl_set_value(form_wind_disp_calccomp, false);
							}
						} else {
							ctl_set_value(form_wind_disp_calcSP, true);
							ctl_set_visible(form_wind_disp_calccomp, false);
							data.config.useinputwinds = false;
						}
					} else {
						// manual wind calcualtion
						question->type = Qwindcalcoff;
						FrmPopupForm(form_question);
					}
					handled = true;
					break; 
				case form_wind_disp_usecalchw:
					data.config.usecalchw = ctl_get_value(form_wind_disp_usecalchw); 
					handled = true; 
					break; 
				case form_wind_disp_hwposlabel: 
					data.config.hwposlabel = ctl_get_value(form_wind_disp_hwposlabel); 
					handled = true; 
					break; 
				case form_wind_disp_mancalc:
                                        FrmGotoForm(form_wind_spddir);
					handled = true;
					break;
				default: 
					break; 
			} 
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
		default: 
			break; 
	} 
	return handled; 
} 

Boolean form_wind_spddir_event_handler(EventPtr event)
{
	Boolean handled=false;
	static double glider_bearing = 0.0;
	static double glider_airspeed = 54.0;
	static double wind_direction;
	static double wind_speed;
	double gx, gy, tx, ty, wx, wy;
	static double wndspds[MAXWINDIDX+1];
	static double wnddirs[MAXWINDIDX+1];
	static Int16 wndnum = 0;
	static Boolean wndmaxnum_reached = false;
	static UInt32 lastcursecs = 0;
	double sumwnddir = 0.0, sumwndspd = 0.0, wnddir = 0.0, wndspd = 0.0;
	Int16 x = 0, wndmaxnum = 0;
	EventType newEvent;

	switch (event->eType) {
		case frmOpenEvent:
			// reset wind averages
			wndnum = 0;
			wndmaxnum_reached = false;
			lastcursecs = cursecs;
		case frmUpdateEvent:
			menuopen = false;
			FrmDrawForm(FrmGetActiveForm());
			// get current wind
			wind_direction = data.input.wnddir;
			wind_speed = data.input.wndspd;
			// display values
			field_set_value(form_wind_spddir_dir, print_direction2(glider_bearing));
			field_set_value(form_wind_spddir_spd, print_horizontal_speed2(glider_airspeed, 1));
			// units
			WinDrawChars(data.input.wndtext, 3, 91, 58);
			WinDrawChars(data.input.spdtext, 3, 91, 89);
			WinDrawChars(data.input.spdtext, 3, 91, 120);
			// allows adjustment of wind increment size and precision on rounding
			switch (data.config.wndunits) {
				case METRICMPS:
					windinc = 0.5;
					break;
				case METRIC:
					windinc = 2.5;
					break;
				case STATUTE:
					windinc = 1.0;
					break;
				case NAUTICAL:
					windinc = 1.0;
					break;
				default:
					windinc = 1.0;
					break;
			}
			formopen = cursecs;
			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
				// auto dismiss dialog form
				if (cursecs > formopen + data.config.autodismisstime) {
					// push cancel button press into event queue
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_wind_spddir_cancel;
					EvtAddEventToQueue(&newEvent);
				}
				// calculate wind values
				if (cursecs != lastcursecs) {
					lastcursecs = cursecs;
//					// test track data
//					data.input.magnetic_track.value = 0.0;
//					data.input.magnetic_track.valid = true;
//					data.input.ground_speed.value = 25.0;
//					data.input.ground_speed.valid = true;
					if (data.input.magnetic_track.valid && data.input.ground_speed.valid) {
						// testing
						// glider (TAS)
						gx = Sin(DegreesToRadians(glider_bearing)) * glider_airspeed * (1.0+(0.02*data.logger.gpsalt/1000));
						gy = Cos(DegreesToRadians(glider_bearing)) * glider_airspeed * (1.0+(0.02*data.logger.gpsalt/1000));
						// track
						tx = Sin(DegreesToRadians(data.input.magnetic_track.value)) * data.input.ground_speed.value;
						ty = Cos(DegreesToRadians(data.input.magnetic_track.value)) * data.input.ground_speed.value;
						// wind
						wx = gx - tx;
						wy = gy - ty;
						wndspd = Sqrt(wx*wx + wy*wy);
						wnddir = nice_brg(RadiansToDegrees(Atan2(wx, wy)));
						// average wind
						wnddirs[wndnum] = wnddir;
						wndspds[wndnum] = wndspd;
						if (wndmaxnum_reached) {
							wndmaxnum = MAXWINDIDX;
						} else {
							wndmaxnum = wndnum;
						}
						for (x = 0; x <= wndmaxnum; x++) {
							sumwnddir = glAngleAverage(sumwnddir, wnddirs[x], x + 1);
							sumwndspd += wndspds[x];
						}
						wndnum++;
						if (wndnum >= MAXWINDIDX) {
							wndnum = MAXWINDIDX;
							wndmaxnum_reached = true;
						}
						wind_direction = sumwnddir;
						wind_speed = sumwndspd / (double)(wndmaxnum + 1);
					}
					// display wind values
					field_set_value(form_wind_spddir_wnddir, print_direction2(wind_direction));
					field_set_value(form_wind_spddir_wndspd, print_horizontal_wndspeed(wind_speed,1));
					// display ground values
					field_set_value(form_wind_spddir_gnddir, print_direction(data.input.magnetic_track));
					field_set_value(form_wind_spddir_gndspd, print_horizontal_speed(data.input.ground_speed));
				}
			}
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			formopen = cursecs;
			switch ( event->data.ctlEnter.controlID ) {
				case form_wind_spddir_spdplus:
					// change glider speed
					glider_airspeed = increment(glider_airspeed*data.input.spdconst, windinc)/data.input.spdconst;
					field_set_value(form_wind_spddir_spd, print_horizontal_speed2(glider_airspeed, 1));
					// reset wind averages
					wndnum = 0;
					wndmaxnum_reached = false;
					handled = true;
					break;
				case form_wind_spddir_spdminus:
					// change glider speed
					glider_airspeed = increment(glider_airspeed*data.input.spdconst, -windinc)/data.input.spdconst;
					field_set_value(form_wind_spddir_spd, print_horizontal_speed2(glider_airspeed, 1));
					// reset wind averages
					wndnum = 0;
					wndmaxnum_reached = false;
					handled = true;
					break;
				case form_wind_spddir_dirplus:
					// change glider direction
					glider_bearing = nice_brg(increment(glider_bearing, 10));
					field_set_value(form_wind_spddir_dir, print_direction2(glider_bearing));
					// reset wind averages
					wndnum = 0;
					wndmaxnum_reached = false;
					handled = true;
					break;
				case form_wind_spddir_dirminus:
					// change glider direction
					glider_bearing = nice_brg(increment(glider_bearing, -10));
					field_set_value(form_wind_spddir_dir, print_direction2(glider_bearing));
					// reset wind averages
					wndnum = 0;
					wndmaxnum_reached = false;
					handled = true;
					break;
				case form_wind_spddir_docalc:
					data.input.wnddir = wind_direction;
					data.input.wndspd = wind_speed;
					FrmGotoForm(form_wind_disp);
					handled = true;
					break;
				case form_wind_spddir_cancel:
					FrmGotoForm(form_wind_disp);
					handled = true;
					break;
				default:
					break;
			}
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
		default:
			break;
	}
	return handled;
}


Boolean form_wind_3dinfo_event_handler(EventPtr event) 
{ 
	Boolean handled=false; 
	Int16 i;
	static Boolean validpendown = false;
	static RectangleType rectP;

	switch (event->eType) { 
		case frmOpenEvent: 
		case frmUpdateEvent: 
			menuopen = false; 
			FrmDrawForm(FrmGetActiveForm()); 
			if (data.config.windprofileorient == TRACKUP) {
				frm_set_title("Wind Profile - Track Up");
			} else {
				frm_set_title("Wind Profile - North Up");
			}
				
			// draw units
			for (i=0; i<7; i++) {
				WinDrawChars(data.input.alttext, 2, 32, 20+i*21);
				WinDrawChars(data.input.wndtext, 3, 90, 20+i*21); 
			}

			// align profile to current altitude
			profilebase = (Int16)(data.input.inusealt / (profilescale[data.config.profilescaleidx] * profilestep)) - 5;
			if (profilebase < 0) profilebase = 0;

			// touch area on screen
			rectP.topLeft.x = 107;
			rectP.topLeft.y = 12;
			rectP.extent.x = 159;
			rectP.extent.y = 159;

			handled=true; 
			break;
		case nilEvent: 
			if (!menuopen) {
				// calculate wind profile			
				CalcWindProfile(false, false);
			}
			handled=false; 
			break;
		case frmCloseEvent: 
			handled=false; 
			break; 
/*		case ctlSelectEvent:  // A control button was pressed and released. 
			PlayKeySound(); 
			switch ( event->data.ctlEnter.controlID ) { 
				case form_wind_3dinfo_closebtn:
					FrmGotoForm(form_wind_disp);
					handled = true; 
					break; 
				default: 
					break; 
			} 
			break;
*/ 
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
		case penDownEvent:
			validpendown = false;
			if (!menuopen && RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
				//WinInvertRectangle (&rectP, 0);
				validpendown = true;
				handled=true;
			}
			break;
		case penUpEvent:
			if (validpendown) {
				//WinInvertRectangle(&rectP, 0);	
			}
			if (validpendown && RctPtInRectangle (event->screenX, event->screenY, &rectP) && (validpendown)) {
				PlayKeySound();
				// switch from profile graphics to numbers
				profilegraph = !profilegraph;
				CalcWindProfile(false, false);
				handled = true;					
			}
			validpendown = false;
			break;
		default: 
			break; 
	} 
	return handled; 
} 

// David Lane - new waypoint list handler
Boolean form_list_waypt_event_handler(EventPtr event)
{
	Boolean handled = false;
	ListPtr lst, lst2, lst3, lst4;
	Int16 lstpos = 0;
	Char tempchar[40];
	FormType *pfrm = FrmGetActiveForm();
	MemHandle waypoint_hand;
	MemPtr waypoint_ptr;
	Int16 selectedwayptsave;
	static Int16 savewptidx;
	static UInt16 sortTypeSav = -1;
	static Boolean selecttempwpt = false;

	lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_waypt_list));
	lst2 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_waypt_list2));
	lst3 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_waypt_list3));
	lst4 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_waypt_list4));

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "list wpt open/update");
//			HostTraceOutputTL(appErrorClass, "Wpt idx - Draw Frm %s", DblToStr(selectedWaypointIndex,0));
			FrmDrawForm(pfrm);
			menuopen = false;
			pressedgo = false;
			savewptidx = -1;
			selecttempwpt = false;
                        
			// set sort order to nearest airport / landing sites
			if (sortTypeSav == -1) {
				if (emergencyland) {
					// save sort order
					sortTypeSav = WayptsortType;
					WayptsortType = SortByDistApt;
					currentWayptPage = 0;
				} else {
					if (!wenttoedit) sortTypeSav = -1;
				}
			}
			wenttoedit = false;

			// if valid GPS position, set distance sort orders to nearest waypoints i.e. first page in list
			if ( (StrCompare(data.logger.gpsstat, "A") == 0) && ((WayptsortType == SortByDistance) || (WayptsortType == SortByDistApt)) ) {
				currentWayptPage = 0;
			}

			// David Lane - if no waypoint is selected then hide the Edit and Delete buttons
//			if (selectedWaypointIndex == -1) {
//				ctl_set_visible(form_list_waypt_edit, false);
//				ctl_set_visible(form_list_waypt_findbtn, false);
//			} else {
				ctl_set_visible(form_list_waypt_edit, true);
				ctl_set_visible(form_list_waypt_findbtn, true);
//			}

			// setup buttons
			ctl_set_visible(form_list_waypt_new, true);
			ctl_set_visible(form_list_waypt_OK, true);
			if (wayselect) {
				if (newTaskWay) {
					// adding a waypoint to a task
					ctl_set_label(form_list_waypt_OK ,"ADD");
				} else 	if ((data.task.numwaypts > 0) && tasknotfinished) {
					// selecting a temp waypoint
					taskonhold = savtaskonhold;
					selecttempwpt = true;
					currentWayptPage = 0;
					ctl_set_label(form_list_waypt_OK ,"TEMP");
   				} else {
					// selecting a waypoint, task not active
					ctl_set_label(form_list_waypt_OK ,"GO");
				}
			} else {
				// just viewing the list
				ctl_set_label(form_list_waypt_OK ,"OK");
			}

			DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, WayptsortType);

			// MFH - Find the currently In Use Waypoint in the db to set the selected Index (if we have re-sorted it)
			if (emergencyland) {
				if (selectedWaypointIndex != -1) selectedWaypointIndex = FindWayptRecordByName(data.inuseWaypoint.name);
				emergencyland = false;
			}

			// David Lane - redraw the list
			refresh_waypoint_list(0);
			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
				// setup title to show full name of waypoint
				if (wayselect) {
					if (selectedWaypointIndex != -1) {
						StrCopy(tempchar, selectedWaypoint->name);
						StrCat(tempchar, " - ");
					} else {
						StrCopy(tempchar, "SELECT - ");
					}
				} else {
					StrCopy(tempchar, "List - ");
				}
				switch (WayptsortType) {
					case SortByNameA:
						//StrCat(tempchar, "Name A - ");
						WinDrawLine(92,26,119,26);
						break;
					case SortByNameD:
						//StrCat(tempchar, "Name D - ");
						WinDrawLine(92,26,119,26);
						break;
					case SortByDistance:
						//StrCat(tempchar, "Distance - ");
						WinDrawLine(7,26,26,26);
						break;
					case SortByDistApt:
						//StrCat(tempchar, "Airport - ");
						WinDrawLine(42,26,79,26);
						break;
					case SortByLastUsed:
						//StrCat(tempchar, "Last Used - ");
						WinDrawLine(132,26,153,26);
						break;
					default:
						break;
				}
				switch (data.config.alttype) {
					case REQALT:
						StrCat(tempchar, "R.Alt");
						break;
					case ARVALT:
						StrCat(tempchar, "A.Alt");
						break;
					case DLTALT:
						StrCat(tempchar, "D.Alt");
						break;
					default:
						break;
				}
				frm_set_title(tempchar);
			}
			break;
		case ctlSelectEvent:
			PlayKeySound();
			savewptidx = -1;
			switch ( event->data.ctlEnter.controlID ) {
				case form_list_waypt_edit:
//					HostTraceOutputTL(appErrorClass, "Wpt idx - Edit %s", DblToStr(selectedWaypointIndex,0));
					if (selectedWaypointIndex != -1) {
						// David Lane - open the Edit form without a new record
						newWaypt = false;
						wenttoedit = true;
						// check for ref waypoint
						if (selectedWaypoint->type & REFWPT) {
							wasrefwpt = true;
						} else {
							wasrefwpt = false;
						}
						FrmGotoForm( form_av_waypt );
					}
					break;
				case form_list_waypt_new:
					// David Lane - open the Edit form with a new "clean" record
					newWaypt = true;
					wenttoedit = true;
					wayaddinfoedit = false;
					wasrefwpt = false;
					FrmGotoForm( form_av_waypt );
					break;
				case form_list_waypt_distapt:
				case form_list_waypt_name:
				case form_list_waypt_dist:
				case form_list_waypt_recent:
					ctl_set_visible(form_list_waypt_findlbl, false);
					ctl_set_visible(form_list_waypt_findname, false);
					ctl_set_visible(form_list_waypt_findlookup, false);
					ctl_set_visible(form_list_waypt_new, true);
					ctl_set_visible(form_list_waypt_OK, true);
					ctl_set_visible(form_list_waypt_name, true);
					ctl_set_visible(form_list_waypt_dist, true);
					ctl_set_visible(form_list_waypt_distapt, true);
					ctl_set_visible(form_list_waypt_recent, true);
					if (event->data.ctlEnter.controlID == form_list_waypt_name) {
						// David Lane - sort the database by Name
						if (WayptsortType == SortByNameA) {
							WayptsortType = SortByNameD;
						} else {
							WayptsortType = SortByNameA;
						}
						WinDrawLine(92,26,119,26);
					} else if (event->data.ctlEnter.controlID == form_list_waypt_dist) {
						// David Lane - sort the database by Distance
						WayptsortType = SortByDistance;
						WinDrawLine(7,26,26,26);
					} else if (event->data.ctlEnter.controlID == form_list_waypt_distapt) {
						// MFH - sort the database by Airport/Landpoint(or not) then by Distance
						WayptsortType = SortByDistApt;
						WinDrawLine(42,26,79,26);
					} else if (event->data.ctlEnter.controlID == form_list_waypt_recent) {
						// PG - sort by most recently used
						WayptsortType = SortByLastUsed;
						WinDrawLine(132,26,153,26);
					}
					DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, WayptsortType);
					if (selectedWaypointIndex != -1) {
						selectedWaypointIndex = FindWayptRecordByName(data.inuseWaypoint.name);
					}
					currentWayptPage = 0;
//					if (selectedWaypointIndex == -1) {
//						ctl_set_visible(form_list_waypt_edit, false);
//						ctl_set_visible(form_list_waypt_findbtn, false);
//					} else {
						ctl_set_visible(form_list_waypt_edit, true);
						ctl_set_visible(form_list_waypt_findbtn, true);
//					}
					refresh_waypoint_list(0);
					break;
				case form_list_waypt_findbtn:
					ctl_set_visible(form_list_waypt_name, true);
					ctl_set_visible(form_list_waypt_dist, true);
					ctl_set_visible(form_list_waypt_distapt, true);
					ctl_set_visible(form_list_waypt_recent, true);
					WinDrawLine(92,26,119,26);
					sortTypeSav = WayptsortType;
					WayptsortType = SortByNameA;
					DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, WayptsortType);
					if (selectedWaypointIndex != -1) {
						selectedWaypointIndex = FindWayptRecordByName(data.inuseWaypoint.name);
					}
					currentWayptPage = 0;
//					if (selectedWaypointIndex == -1) {
//						ctl_set_visible(form_list_waypt_edit, false);
//						ctl_set_visible(form_list_waypt_delete, false);
//					}
					refresh_waypoint_list(0);
					field_set_value(form_list_waypt_findname,"");
					ctl_set_visible(form_list_waypt_edit, false);
					ctl_set_visible(form_list_waypt_findbtn, false);
					ctl_set_visible(form_list_waypt_new, false);
					ctl_set_visible(form_list_waypt_OK, false);
					ctl_set_visible(form_list_waypt_findlbl, true);
					ctl_set_visible(form_list_waypt_findname, true);
					ctl_set_visible(form_list_waypt_findlookup, true);
					ctl_set_visible(form_list_waypt_findname, true);
					FrmSetFocus(pfrm,  FrmGetObjectIndex(pfrm, form_list_waypt_findname));
					break;
				case form_list_waypt_findlookup:
					selectedwayptsave = selectedWaypointIndex;
					selectedWaypointIndex = FindWayptRecordByNamePartial(field_get_str(form_list_waypt_findname));
					if (selectedWaypointIndex != -1) {
						ctl_set_visible(form_list_waypt_findlbl, false);
						ctl_set_visible(form_list_waypt_findname, false);
						ctl_set_visible(form_list_waypt_findlookup, false);
						ctl_set_visible(form_list_waypt_new, true);
						ctl_set_visible(form_list_waypt_edit, true);
						ctl_set_visible(form_list_waypt_findbtn, true);
						ctl_set_visible(form_list_waypt_OK, true);
						OpenDBQueryRecord(waypoint_db, selectedWaypointIndex, &waypoint_hand, &waypoint_ptr);
						MemMove(selectedWaypoint, waypoint_ptr, sizeof(WaypointData));
						MemHandleUnlock(waypoint_hand);
					} else {
						selectedWaypointIndex = selectedwayptsave;
						if (selectedWaypointIndex == -1) {
							MemSet(selectedWaypoint, sizeof(WaypointData),0);
						}
					}
					currentWayptPage = (Int16)selectedWaypointIndex/7;
					refresh_waypoint_list(0);
					break;
				case form_list_waypt_OK:
					if (selecttempwpt) taskonhold = true;
					select_fg_waypoint(WAYGO);
					break;
				default:
					break;
			}
			handled=true;
			break;
		case lstSelectEvent:
			if (OpenDBCountRecords(waypoint_db) > 0) {
				switch (event->data.lstSelect.listID) {
				case form_list_waypt_list:
					PlayKeySound();
					lstpos = LstGetSelection(lst);
					selectedWaypointIndex = lstpos + (currentWayptPage * 7);
					if (selectedWaypointIndex == savewptidx) {
						// go to waypoint edit
						if (selectedWaypointIndex != -1) {
							PlayKeySound();
							newWaypt = false;
							wenttoedit = true;
							// check for ref waypoint
							if (selectedWaypoint->type & REFWPT) {
								wasrefwpt = true;
							} else {
								wasrefwpt = false;
							}
							FrmGotoForm( form_av_waypt );
						}
						savewptidx = -1; // forces double click again
					} else {
						savewptidx = selectedWaypointIndex;
					}					
					break;
				case form_list_waypt_list2:
					PlayKeySound();
					lstpos = LstGetSelection(lst2);
					break;
				case form_list_waypt_list3:
					PlayKeySound();
					lstpos = LstGetSelection(lst3);
					break;
				case form_list_waypt_list4:
					PlayKeySound();
					lstpos = LstGetSelection(lst4);
					break;
				default:
					break;
				}
				// get waypoint data
				selectedWaypointIndex = lstpos + (currentWayptPage * 7);
				OpenDBQueryRecord(waypoint_db, selectedWaypointIndex, &waypoint_hand, &waypoint_ptr);
				MemMove(selectedWaypoint, waypoint_ptr, sizeof(WaypointData));
				MemHandleUnlock(waypoint_hand);

				ctl_set_visible(form_list_waypt_findlbl, false);
				ctl_set_visible(form_list_waypt_findname, false);
				ctl_set_visible(form_list_waypt_findlookup, false);
				ctl_set_visible(form_list_waypt_OK, true);
				ctl_set_visible(form_list_waypt_edit, true);
				ctl_set_visible(form_list_waypt_findbtn, true);
				ctl_set_visible(form_list_waypt_new, true);
				// update lists
				LstSetSelection(lst, lstpos);
				LstSetSelection(lst2, lstpos);
				LstSetSelection(lst3, lstpos);
				LstSetSelection(lst4, lstpos);
			}
			DrawHorizListLines(7, 44, 14);
			handled=true;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "list_waypt-frmCloseEvent");
			if ((sortTypeSav != -1) && !wenttoedit) {
				// restore original sort order
				WayptsortType = sortTypeSav;
				DmQuickSort(waypoint_db, (DmComparF*)waypt_comparison, WayptsortType);
				selectedWaypointIndex = FindWayptRecordByName(selectedWaypoint->name);
				currentWayptPage = (Int16)selectedWaypointIndex/7;
				sortTypeSav = -1;
			}
			if (wayselect && pressedgo) {
				if (!wenttoedit) {
					wayselect = false;
				}
				if (origform == form_set_task) {
					// add a waypt to a task
					tskWayAddIdx = selectedWaypointIndex;
				} else if (origform == form_final_glide || origform == form_moving_map) {
					if ((data.task.numwaypts <= 0 || taskonhold) || !tasknotfinished) {
						// Only do this if there is no task currently active
						MemMove(&data.inuseWaypoint, selectedWaypoint, sizeof(WaypointData));
						data.input.destination_valid = true;
						InuseWaypointCalcEvent();
					}
				}
			} else {
				tskWayAddIdx = -1;
			}
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "list_waypt-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "list_waypt-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		default:
			break;
	}
	return(handled);
}

// David Lane - new View/Edit waypoints event handler
Boolean form_av_waypt_event_handler(EventPtr event)
{
	Boolean handled = false;
	FormType *pfrm = FrmGetActiveForm();
	Char tempchar[30];

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "Wpt idx - Edit Frm Draw %s", DblToStr(selectedWaypointIndex,0));
			FrmDrawForm(pfrm);
			if (newWaypt && !wayaddinfoedit) {
				MemSet(selectedWaypoint, sizeof(WaypointData), 0);
				// default N/S and E/W to home waypoint position
				if (data.input.homeLat < 0.0) selectedWaypoint->lat = -0.0000001;
				if (data.input.homeLon < 0.0) selectedWaypoint->lon = -0.0000001;
				// replaced Palm function to handle months/days < 10 correctly i.e. "09" not "9"
				TimSecondsToDateTime(cursecs, &curtime);
				StrCopy(selectedWaypoint->UsedTime, leftpad(DblToStr(curtime.year,0), '0', 2));
				StrCat( selectedWaypoint->UsedTime, "/");
				StrCat( selectedWaypoint->UsedTime, leftpad(DblToStr(curtime.month,0), '0', 2));
				StrCat( selectedWaypoint->UsedTime, "/");
				StrCat( selectedWaypoint->UsedTime, leftpad(DblToStr(curtime.day,0), '0', 2));
			}
			field_set_value(form_av_waypt_name, selectedWaypoint->name);
			field_set_value(form_av_waypt_distance, print_distance2(selectedWaypoint->distance,1));
			field_set_value(form_av_waypt_elev, print_altitude(selectedWaypoint->elevation));
			field_set_value(form_av_waypt_bearing, print_direction2(selectedWaypoint->bearing));
			WinDrawChars(data.input.distext, 2, 69, 28);
			WinDrawChars(data.input.alttext, 2, 69, 39);
			switch (data.config.alttype) {
				case REQALT:
					field_set_value(form_av_waypt_altbtn, "R.Alt:");
					break;
				case ARVALT:
					field_set_value(form_av_waypt_altbtn, "A.Alt:");
					break;
				case DLTALT:
					field_set_value(form_av_waypt_altbtn, "D.Alt:");
					break;
				default:
					break;
			}
			field_set_value(form_av_waypt_altbtn1, print_altitude(selectedWaypoint->alt));
			WinDrawChars(data.input.alttext, 2, 144, 39);

			if (selectedWaypoint->type & AIRPORT) ctl_set_value(form_av_waypt_typeA, true);
			if (selectedWaypoint->type & TURN) ctl_set_value(form_av_waypt_typeT, true);
			if (selectedWaypoint->type & LAND)	ctl_set_value(form_av_waypt_typeL, true);
			if (selectedWaypoint->type & START) ctl_set_value(form_av_waypt_typeS, true);
			if (selectedWaypoint->type & FINISH) ctl_set_value(form_av_waypt_typeF, true);
			if (selectedWaypoint->type & MARK) ctl_set_value(form_av_waypt_typeM, true);
			if (selectedWaypoint->type & THRML) ctl_set_value(form_av_waypt_typeTH, true);
			if (selectedWaypoint->type & HOME) ctl_set_value(form_av_waypt_typeH, true);
			//if (selectedWaypoint->type & AIRLAND) ctl_set_value(form_av_waypt_typeAL, true);
			if (selectedWaypoint->type & REFWPT) ctl_set_value(form_av_waypt_typeRF, true);

			if (selectedWaypoint->type & AREA) { 
				ctl_set_value(form_av_waypt_typeAR, true);

				field_set_enable(form_av_waypt_arearadial1, true);
				ctl_set_enable(form_av_waypt_arearadial1m, true);
				ctl_set_enable(form_av_waypt_arearadial1t, true);
				if (selectedWaypoint->arearadial1mag) {
					ctl_set_value(form_av_waypt_arearadial1m, true);
					ctl_set_value(form_av_waypt_arearadial1t, false);
					field_set_value(form_av_waypt_arearadial1, print_direction2(nice_brg(((double)selectedWaypoint->arearadial1)+ data.input.deviation.value)));
				} else {
					ctl_set_value(form_av_waypt_arearadial1m, false);
					ctl_set_value(form_av_waypt_arearadial1t, true);
					field_set_value(form_av_waypt_arearadial1, print_direction2((double)selectedWaypoint->arearadial1));
				}
				
				field_set_enable(form_av_waypt_arearadial2, true);
				ctl_set_enable(form_av_waypt_arearadial2m, true);
				ctl_set_enable(form_av_waypt_arearadial2t, true);
				if (selectedWaypoint->arearadial2mag) {
					ctl_set_value(form_av_waypt_arearadial2m, true);
					ctl_set_value(form_av_waypt_arearadial2t, false);
					field_set_value(form_av_waypt_arearadial2, print_direction2(nice_brg(((double)selectedWaypoint->arearadial2)+ data.input.deviation.value)));
				} else {
					ctl_set_value(form_av_waypt_arearadial2m, false);
					ctl_set_value(form_av_waypt_arearadial2t, true);
					field_set_value(form_av_waypt_arearadial2, print_direction2((double)selectedWaypoint->arearadial2));
				}
				field_set_enable(form_av_waypt_arearadius, true);
				field_set_value(form_av_waypt_arearadius, print_distance2(selectedWaypoint->arearadius,1));
				field_set_enable(form_av_waypt_arearadius2, true);
				field_set_value(form_av_waypt_arearadius2, print_distance2(selectedWaypoint->arearadius2,1));
			} else {
				field_set_value(form_av_waypt_arearadial1, "N/A");
				field_set_enable(form_av_waypt_arearadial1, false);
				field_set_value(form_av_waypt_arearadial2, "N/A");
				field_set_enable(form_av_waypt_arearadial2, false);
				field_set_value(form_av_waypt_arearadius, "N/A");
				field_set_enable(form_av_waypt_arearadius, false);
				field_set_value(form_av_waypt_arearadius2, "N/A");
				field_set_enable(form_av_waypt_arearadius2, false);
				ctl_set_enable(form_av_waypt_arearadial1m, true);
				ctl_set_value(form_av_waypt_arearadial1m, false);
				ctl_set_enable(form_av_waypt_arearadial1m, false);
				ctl_set_enable(form_av_waypt_arearadial1t, true);
				ctl_set_value(form_av_waypt_arearadial1t, false);
				ctl_set_enable(form_av_waypt_arearadial1t, false);
				ctl_set_enable(form_av_waypt_arearadial2m, true);
				ctl_set_value(form_av_waypt_arearadial2m, false);
				ctl_set_enable(form_av_waypt_arearadial2m, false);
				ctl_set_enable(form_av_waypt_arearadial2t, true);
				ctl_set_value(form_av_waypt_arearadial2t, false);
				ctl_set_enable(form_av_waypt_arearadial2t, false);
			}
			field_set_value(form_av_waypt_remark, selectedWaypoint->rmks);
			switch (data.config.llfmt) {
				case LLUTM:
					frm_set_label(form_av_waypt_latlbl, "Est:");
					frm_set_label(form_av_waypt_lonlbl, "Nth:");
					ctl_set_value(form_av_waypt_llutm, true);
					ctl_set_value(form_av_waypt_lldm, false);
					ctl_set_value(form_av_waypt_lldms, false);
					ctl_set_visible(form_av_waypt_utmzone, true);
					LLToStringUTM(selectedWaypoint->lat, selectedWaypoint->lon, tempchar, EASTING);
					field_set_value(form_av_waypt_lat, tempchar);
					LLToStringUTM(selectedWaypoint->lat, selectedWaypoint->lon, tempchar, NORTHING);
					field_set_value(form_av_waypt_lon, tempchar);
					LLToStringUTM(selectedWaypoint->lat, selectedWaypoint->lon, tempchar, ZONE);
					field_set_value(form_av_waypt_utmzone, tempchar);
					break;
				case LLDM:
					frm_set_label(form_av_waypt_latlbl, "Lat:");
					frm_set_label(form_av_waypt_lonlbl, "Lon:");
					ctl_set_value(form_av_waypt_llutm, false);
					ctl_set_value(form_av_waypt_lldm, true);
					ctl_set_value(form_av_waypt_lldms, false);
					ctl_set_visible(form_av_waypt_utmzone, false);
					LLToStringDM(selectedWaypoint->lat, tempchar, ISLAT, true, true, 3);
					field_set_value(form_av_waypt_lat, tempchar);
					LLToStringDM(selectedWaypoint->lon, tempchar, ISLON, true, true, 3);
					field_set_value(form_av_waypt_lon, tempchar);
					break;
				case LLDMS:
				default:
					frm_set_label(form_av_waypt_latlbl, "Lat:");
					frm_set_label(form_av_waypt_lonlbl, "Lon:");
					ctl_set_value(form_av_waypt_llutm, false);
					ctl_set_value(form_av_waypt_lldm, false);
					ctl_set_value(form_av_waypt_lldms, true);
					ctl_set_visible(form_av_waypt_utmzone, false);
					LLToStringDMS(selectedWaypoint->lat, tempchar, ISLAT);
					field_set_value(form_av_waypt_lat, tempchar);
					LLToStringDMS(selectedWaypoint->lon, tempchar, ISLON);
					field_set_value(form_av_waypt_lon, tempchar);
					break;
			}
			WinDrawChars(data.input.distext, 2, 55, 91);
			WinDrawChars(data.input.distext, 2, 134, 91);
			handled=true;
			break;
		case ctlSelectEvent:
//			HostTraceOutputTL(appErrorClass, "Wpt idx - Edit Frm Event %s", DblToStr(selectedWaypointIndex,0));
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
				case form_av_waypt_delete:
					if (selectedWaypointIndex != -1) {
						question->type = Qdelwpt;
						FrmPopupForm(form_question);
					}
					break;
				case form_av_waypt_save:
					if (wasrefwpt) ClearWayptREFStatus();
					if (save_waypt_fields()) {
						FrmGotoForm(form_list_waypt);
					}
					break;
				case form_av_waypt_quit:
					FrmGotoForm( form_list_waypt );
					break;
				case form_av_waypt_addinfo:
					wayaddinfoedit = true;
					if (TempWptSave()) {
						FrmGotoForm(form_waypt_addinfo);
					}
					break;
				case form_av_waypt_lldm:
					frm_set_label(form_av_waypt_latlbl, "Lat:");
					frm_set_label(form_av_waypt_lonlbl, "Lon:");
					ctl_set_visible(form_av_waypt_utmzone, false);
					LLToStringDM(selectedWaypoint->lat, tempchar, ISLAT, true, true, 3);
					field_set_value(form_av_waypt_lat, tempchar);
					LLToStringDM(selectedWaypoint->lon, tempchar, ISLON, true, true, 3);
					field_set_value(form_av_waypt_lon, tempchar);
					data.config.llfmt = LLDM;
					break;
				case form_av_waypt_lldms:
					frm_set_label(form_av_waypt_latlbl, "Lat:");
					frm_set_label(form_av_waypt_lonlbl, "Lon:");
					ctl_set_visible(form_av_waypt_utmzone, false);
					LLToStringDMS(selectedWaypoint->lat, tempchar, ISLAT);
					field_set_value(form_av_waypt_lat, tempchar);
					LLToStringDMS(selectedWaypoint->lon, tempchar, ISLON);
					field_set_value(form_av_waypt_lon, tempchar);
					data.config.llfmt = LLDMS;
					break;
				case form_av_waypt_llutm:
					frm_set_label(form_av_waypt_latlbl, "Est:");
					frm_set_label(form_av_waypt_lonlbl, "Nth:");
					ctl_set_visible(form_av_waypt_utmzone, true);
					LLToStringUTM(selectedWaypoint->lat, selectedWaypoint->lon, tempchar, EASTING);
					field_set_value(form_av_waypt_lat, tempchar);
					LLToStringUTM(selectedWaypoint->lat, selectedWaypoint->lon, tempchar, NORTHING);
					field_set_value(form_av_waypt_lon, tempchar);
					LLToStringUTM(selectedWaypoint->lat, selectedWaypoint->lon, tempchar, ZONE);
					field_set_value(form_av_waypt_utmzone, tempchar);
					data.config.llfmt = LLUTM;
					break;
				case form_av_waypt_typeAR:
					if (ctl_get_value(form_av_waypt_typeAR)) {
						field_set_enable(form_av_waypt_arearadial1, true);
						ctl_set_enable(form_av_waypt_arearadial1m, true);
						ctl_set_enable(form_av_waypt_arearadial1t, true);
						if (selectedWaypoint->arearadial1mag) {
							ctl_set_value(form_av_waypt_arearadial1m, true);
							ctl_set_value(form_av_waypt_arearadial1t, false);
							field_set_value(form_av_waypt_arearadial1, print_direction2(nice_brg(((double)selectedWaypoint->arearadial1) + data.input.deviation.value)));
						} else {
							ctl_set_value(form_av_waypt_arearadial1m, false);
							ctl_set_value(form_av_waypt_arearadial1t, true);
							field_set_value(form_av_waypt_arearadial1, print_direction2((double)selectedWaypoint->arearadial1));
						}

						field_set_enable(form_av_waypt_arearadial2, true);
						ctl_set_enable(form_av_waypt_arearadial2m, true);
						ctl_set_enable(form_av_waypt_arearadial2t, true);
						if (selectedWaypoint->arearadial2mag) {
							ctl_set_value(form_av_waypt_arearadial2m, true);
							ctl_set_value(form_av_waypt_arearadial2t, false);
							field_set_value(form_av_waypt_arearadial2, print_direction2(nice_brg(((double)selectedWaypoint->arearadial2) + data.input.deviation.value)));
						} else {
							ctl_set_value(form_av_waypt_arearadial2m, false);
							ctl_set_value(form_av_waypt_arearadial2t, true);
							field_set_value(form_av_waypt_arearadial2, print_direction2((double)selectedWaypoint->arearadial2));
						}
						field_set_enable(form_av_waypt_arearadius, true);
						field_set_value(form_av_waypt_arearadius, print_distance2(selectedWaypoint->arearadius,1));
						field_set_enable(form_av_waypt_arearadius2, true);
						field_set_value(form_av_waypt_arearadius2, print_distance2(selectedWaypoint->arearadius2,1));
						if (selectedWaypoint->arearadius == 0 ) {
							FrmSetFocus(pfrm,  FrmGetObjectIndex(pfrm, form_av_waypt_arearadius));
						}
					} else {
						field_set_value(form_av_waypt_arearadial1, "N/A");
						field_set_enable(form_av_waypt_arearadial1, false);
						field_set_value(form_av_waypt_arearadial2, "N/A");
						field_set_enable(form_av_waypt_arearadial2, false);
						field_set_value(form_av_waypt_arearadius, "N/A");
						field_set_enable(form_av_waypt_arearadius, false);
						field_set_value(form_av_waypt_arearadius2, "N/A");
						field_set_enable(form_av_waypt_arearadius2, false);
						ctl_set_enable(form_av_waypt_arearadial1m, true);
						ctl_set_value(form_av_waypt_arearadial1m, false);
						ctl_set_enable(form_av_waypt_arearadial1m, false);
						ctl_set_enable(form_av_waypt_arearadial1t, true);
						ctl_set_value(form_av_waypt_arearadial1t, false);
						ctl_set_enable(form_av_waypt_arearadial1t, false);
						ctl_set_enable(form_av_waypt_arearadial2m, true);
						ctl_set_value(form_av_waypt_arearadial2m, false);
						ctl_set_enable(form_av_waypt_arearadial2m, false);
						ctl_set_enable(form_av_waypt_arearadial2t, true);
						ctl_set_value(form_av_waypt_arearadial2t, false);
						ctl_set_enable(form_av_waypt_arearadial2t, false);
					}
					break;
				default:
					break;
			}
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
		default:
			break;
	}
	return handled;
}

Boolean form_waypt_addinfo_event_handler(EventPtr event)
{
	Boolean handled=false;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(FrmGetActiveForm());
			// copied from waypoint screen
			field_set_value(form_add_waypt_name, TempWpt->name);
			field_set_value(form_add_waypt_distance, print_distance2(TempWpt->distance,1));
			field_set_value(form_add_waypt_elev, print_altitude(TempWpt->elevation));
			field_set_value(form_add_waypt_bearing, print_direction2(TempWpt->bearing));
			WinDrawChars(data.input.distext, 2, 69, 24);
			WinDrawChars(data.input.alttext, 2, 69, 36);
			switch (data.config.alttype) {
				case REQALT:
					field_set_value(form_add_waypt_altbtn, "R.Alt:");
					break;
				case ARVALT:
					field_set_value(form_add_waypt_altbtn, "A.Alt:");
					break;
				case DLTALT:
					field_set_value(form_add_waypt_altbtn, "D.Alt:");
					break;
				default:
					break;
			}
			field_set_value(form_add_waypt_altbtn1, print_altitude(TempWpt->alt));
			WinDrawChars(data.input.alttext, 2, 144, 36);

			field_set_value(form_add_waypt_remark, TempWpt->rmks);

			field_set_value(form_waypt_addinfo_rwdir, print_direction2((double)TempWpt->rwdir));
			if (data.config.disunits == STATUTE || data.config.disunits == NAUTICAL) {
				field_set_value(form_waypt_addinfo_rwlen, DblToStr(pround(TempWpt->rwlen * ALTMPHNAUCONST,0),0));
				WinDrawChars("ft", 2, 130, 72);
			} else {
				field_set_value(form_waypt_addinfo_rwlen, DblToStr(pround(TempWpt->rwlen * ALTMETCONST,0),0));
				WinDrawChars("m", 1, 130, 72);
			}
			field_set_value(form_waypt_addinfo_freq, TempWpt->freq);
			field_set_value(form_waypt_addinfo_geninfo, TempWpt->geninfo);
			field_set_value(form_waypt_addinfo_lastused, TempWpt->UsedTime);
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
			if (wayaddinfoedit) {
				MemMove(selectedWaypoint,TempWpt,sizeof(WaypointData));
				selectedWaypoint->rwdir = (Int16)field_get_long(form_waypt_addinfo_rwdir);
				if (data.config.disunits == STATUTE || data.config.disunits == NAUTICAL) {
					selectedWaypoint->rwlen = field_get_double(form_waypt_addinfo_rwlen) / ALTMPHNAUCONST;
				} else {
					selectedWaypoint->rwlen = field_get_double(form_waypt_addinfo_rwlen) / ALTMETCONST;
				}
				StrCopy(selectedWaypoint->freq, NoComma(field_get_str(form_waypt_addinfo_freq),"."));
				StrCopy(selectedWaypoint->geninfo, NoComma(field_get_str(form_waypt_addinfo_geninfo)," "));
				StrCopy(selectedWaypoint->rmks, NoComma(field_get_str(form_add_waypt_remark)," "));
				StrCopy(selectedWaypoint->UsedTime, field_get_str(form_waypt_addinfo_lastused));
			}
			handled=false;
			break;
		case ctlSelectEvent:
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
				case form_waypt_addinfo_exitbtn:
					if (wayaddinfoedit == false) {
						FrmGotoForm(form_final_glide);
					} else {
						// restore saved waypoint data
						MemMove(selectedWaypoint,TempWpt,sizeof(WaypointData));
						selectedWaypoint->rwdir = (Int16)field_get_long(form_waypt_addinfo_rwdir);
						if (data.config.disunits == STATUTE || data.config.disunits == NAUTICAL) {
							selectedWaypoint->rwlen = field_get_double(form_waypt_addinfo_rwlen) / ALTMPHNAUCONST;
						} else {
							selectedWaypoint->rwlen = field_get_double(form_waypt_addinfo_rwlen) / ALTMETCONST;
						}
						StrCopy(selectedWaypoint->freq, field_get_str(form_waypt_addinfo_freq));
						StrCopy(selectedWaypoint->geninfo, field_get_str(form_waypt_addinfo_geninfo));
						StrCopy(selectedWaypoint->rmks, field_get_str(form_add_waypt_remark));
						StrCopy(selectedWaypoint->UsedTime, field_get_str(form_waypt_addinfo_lastused));
						FrmGotoForm(form_av_waypt);
					}
					break;
				default:
					break;
			}
			handled=true;
			break;
		default:
			break;
	}
	return handled;
}

Boolean form_set_task_event_handler(EventPtr event)
{
	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst, lst2, lst3, lst4;
	Boolean handled = false;
	Boolean atexit = false;
	Int16 selectedIdx = 0;
	static Int16 savewptidx1, savewptidx4;
	Int16 wptidx;
	MemHandle mem_hand;
	MemPtr mem_ptr;
	static Boolean copiedtask = false;
	Char tempchar[30];
	DateTimeType strtdt;
	Int16 x,y;
	Char origname[13];
//	static Boolean taskactvpressed = false;
	static Boolean AAT_dist_chg = false;
	
	lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_task_list));
	lst2 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_task_list2));
	lst3 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_task_list3));
	lst4 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_task_list4));

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "set_task update/open");
			FrmDrawForm(pfrm);
			origform = form_set_task;
			exittaskreadonly = false;
			savewptidx1 = -1;
			savewptidx4 = -1;
			settaskshowpct = false;
			settaskstats = STATSDIST;
			taskactvpressed = false;

			if (settaskreadonly) {
				frm_set_title("Task Statistics");
				// change buttons for read only view
				ctl_set_visible(form_set_task_upbtn, false);
				ctl_set_visible(form_set_task_dwnbtn, false);
				ctl_set_visible(form_set_task_addbtn, false);

				ctl_set_label(form_set_task_actvbtn, "Copy");
				ctl_set_label(form_set_task_editbtn, "Back");
				ctl_set_label(form_set_task_viewbtn, "View");

				if (copiedtask) ctl_set_visible(form_set_task_actvbtn, false);

				ctl_set_enable(form_set_task_hastakeoff, false);
				ctl_set_enable(form_set_task_haslanding, false);
				
				ctl_set_visible(form_set_task_stats, true);
				ctl_set_visible(form_set_task_stats1, true);
				ctl_set_visible(form_set_task_stats2, true);
				ctl_set_value(form_set_task_stats, true);

				ctl_set_visible(form_set_task_delbtn, false);
				ctl_set_visible(form_set_task_viewbtn, true);
			} else if (taskIndex ==0) {
				if (taskonhold) {
					frm_set_title("Task On Hold");
				} else {
					frm_set_title("Active Task Edit");
				}
			} else {
				frm_set_title("Task Edit");
			}

			if (newTaskWay) {
				newTaskWay = false;
				refresh_task_details(TASKADDAFTER);
			} else {
				refresh_task_details(TASKDISP);
			}
			StrCopy(origname, tsk->name);
//			HostTraceOutputTL(appErrorClass, "origname %s", origname);

			if (tsk->rulesactive) {
				ctl_set_label(form_set_task_advset, "RULES");
			} else {
				ctl_set_label(form_set_task_advset, "Rules");
			}
			handled=true;
			break;
		case popSelectEvent:  // A popup button was pressed and released.
			PlayKeySound();
//			HostTraceOutputTL(appErrorClass, "Popup pressed");
			switch (event->data.popSelect.controlID) {
				case form_set_task_AATdist_pop1:
					AATdist = event->data.popSelect.selection;
					//if (AATdist != tsk->aataimtype) {
						if (tsk->aataimtype == AATusr) {
							// ask before overwriting user set targets
							question->type = QAAToverwrite;
							FrmPopupForm(form_question);
						} else {
							setAATdist(AATdist);
							setAATdist(AATdist); // must repeat to ensure correct calculation
							refresh_task_details(TASKDISP);
						}
						// automatically reflect target point changes
						if ((taskIndex == 0) && (data.task.numwaypts > 0)) {
							//set_task_changed = true;
							AAT_dist_chg = true;
						}
					//}
					handled = false;
					break;
				default:
					break;
			}
			break;
		case ctlSelectEvent:
			PlayKeySound();
			savewptidx1 = -1;
			savewptidx4 = -1;
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_task_stats:
					settaskstats = STATSDIST;
					settaskshowpct = false;
					refresh_task_details(TASKDISP);
					handled = true;
					break;
				case form_set_task_stats1:
					settaskstats = STATSSPEED;
					settaskshowpct = false;
					refresh_task_details(TASKDISP);
					handled = true;
					break;
				case form_set_task_stats2:
					settaskstats = STATSTIME;
					settaskshowpct = false;
					refresh_task_details(TASKDISP);
					handled = true;
					break;
				case form_set_task_viewbtn:
					if (settaskreadonly) {
						dispactive = false; // go to selected task not active task
						if (selectedTaskWayIdx != -1) {
							// load task into edittsk
							MemMove(edittsk, tsk, sizeof(TaskData));
							goingtotaskscrn = true;
							save_task_data(form_task_waypt);
						}
					} else {
						save_task_data(form_list_task);
					}
					handled = true;
					break;
				case form_set_task_advset:
					goingtotaskscrn = true;
					skipnewflight = false;
					save_task_data(form_task_rules);
					handled = true;
					break;
				case form_set_task_actvbtn:
					if (settaskreadonly) {
						if (!copiedtask) {
							// add new record to task_db
							StrCopy(tsk->name, "Flt:");
							StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &strtdt);
							DateToAscii(strtdt.month, strtdt.day, strtdt.year, (DateFormatType)PrefGetPreference(prefDateFormat), tempchar); 
							StrCat(tsk->name, tempchar);
							taskIndex = OpenDBAddRecord(task_db, sizeof(TaskData), tsk, false);
							copiedtask = true;
							ctl_set_visible(form_set_task_actvbtn, false);
						}
					} else {
						taskactvpressed = true;
						if (save_task_data(defaultscreen)) refresh_task_details(TASKACTV);
					}
					handled = true;
					break;
				case form_set_task_upbtn:
					save_task_data(0);
					refresh_task_details(TASKMVUP);
					handled = true;
					break;
				case form_set_task_dwnbtn:
					save_task_data(0);
					refresh_task_details(TASKMVDWN);
					handled = true;
					break;
				case form_set_task_hastakeoff:
					save_task_data(0);
					if (ctl_get_value(form_set_task_hastakeoff) && (tsk->numwaypts <= minnumWaypts)) {
						ctl_set_value(form_set_task_hastakeoff, false);
					}
					refresh_task_details(TASKSAVETL);					
					handled = true;
					break;
				case form_set_task_haslanding:
					save_task_data(0);
					if (ctl_get_value(form_set_task_haslanding) && (tsk->numwaypts <= minnumWaypts)) {
						ctl_set_value(form_set_task_haslanding, false);
					}
					refresh_task_details(TASKSAVETL);
					handled = true;
					break;
//				case form_set_task_revbtn:
//					save_task_data(0);
//					refresh_task_details(TASKREV);
//					handled = true;
//					break;
				case form_set_task_addbtn:
					save_task_data(0);
					goingtotaskscrn = true;
					select_fg_waypoint(WAYADD);
					handled = true;
					break;
				case form_set_task_editbtn:
					if (settaskreadonly) {
						exittaskreadonly = true;
						FrmGotoForm(form_flt_info);
					} else {
						// goto edit task
						if ((taskIndex == 0) && (selectedTaskWayIdx < (inareasector!=-1?inareasector:activetskway)) && (numWaypts >= 1) && (data.activetask.tskstartsecs > 0)) {
							warning->type = Wgeneric;
							StrCopy(warning->line1, "Cannot Edit Accomplished");
							StrCopy(warning->line2, "Waypoints");
							FrmPopupForm(form_warning);
						} else {
							dispactive = false; // go to selected task not active task
							if (selectedTaskWayIdx != -1) {
								// load task into edittsk
								OpenDBQueryRecord(task_db, taskIndex, &mem_hand, &mem_ptr);
								MemMove(edittsk, mem_ptr, sizeof(TaskData));
								MemHandleUnlock(mem_hand);
								goingtotaskscrn = true;
								save_task_data(form_task_waypt);
							}
						}
					}
					handled = true;
					break;
				case form_set_task_delbtn:
					save_task_data(0);
					refresh_task_details(TASKREM);
					handled = true;
					break;
				default:
					break;
				}
			break;
		case lstSelectEvent:
			switch (event->data.lstSelect.listID) {
				case form_set_task_list:
					if (numWaypts > 0) {
						PlayKeySound();
						selectedIdx = LstGetSelection(lst);
						wptidx = selectedIdx + (currentTaskPage * 7); 
						if (wptidx + (100 * taskIndex) == savewptidx1) {
							// go to waypoint edit
							if ((taskIndex == 0) && (selectedTaskWayIdx < (inareasector!=-1?inareasector:activetskway)) && (numWaypts >= 1) && (data.activetask.tskstartsecs > 0)) {
								warning->type = Wgeneric;
								StrCopy(warning->line1, "Cannot Edit Accomplished");
								StrCopy(warning->line2, "Waypoints");
								FrmPopupForm(form_warning);
							} else {
								dispactive = false; // go to selected task not active task
								if (selectedTaskWayIdx != -1) {
									// load task into edittsk
									if (!settaskreadonly) {
										OpenDBQueryRecord(task_db, taskIndex, &mem_hand, &mem_ptr);
										MemMove(edittsk, mem_ptr, sizeof(TaskData));
										MemHandleUnlock(mem_hand);
									} else {
										MemMove(edittsk, tsk, sizeof(TaskData));
									}
									goingtotaskscrn = true;
									save_task_data(form_task_waypt);
								}
							}
							savewptidx1 = -1; // forces double click again
						} else {
							savewptidx1 = wptidx + (100 * taskIndex);
						}
//						HostTraceOutputTL(appErrorClass, "selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
					}
					savewptidx4 = -1;
					break;
				case form_set_task_list2:
					if (numWaypts > 0) {
						PlayKeySound();
						selectedIdx = LstGetSelection(lst2);
//						HostTraceOutputTL(appErrorClass, "selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
					}
					savewptidx1 = -1;
					savewptidx4 = -1;
					break;
				case form_set_task_list3:
					if (numWaypts > 0) {
						PlayKeySound();
						selectedIdx = LstGetSelection(lst3);
//						HostTraceOutputTL(appErrorClass, "selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
					}
					savewptidx1 = -1;
					savewptidx4 = -1;
					break;
				case form_set_task_list4:
					if (numWaypts > 0) {
						PlayKeySound();
						selectedIdx = LstGetSelection(lst4);
						wptidx = selectedIdx + (currentTaskPage * 7);
						if (wptidx + (100 * taskIndex) == savewptidx4) {
							// toggle control point attribute
							if (!settaskreadonly && (wptidx > (tsk->hastakeoff?1:0)) && (wptidx < numWaypts-(tsk->haslanding?2:1))) {
								OpenDBQueryRecord(task_db, taskIndex, &mem_hand, &mem_ptr);
								MemMove(tsk,mem_ptr,sizeof(TaskData));
								MemHandleUnlock(mem_hand);
								tsk->waypttypes[wptidx] = tsk->waypttypes[wptidx] ^ CONTROL;
								if ((tsk->waypttypes[wptidx] & CONTROL) == 0) {
									// clear radii if not a control point
									tsk->arearadii[wptidx] = 0.0;
									tsk->arearadii2[wptidx] = 0.0;								
								}
								// clear area type
								if (tsk->waypttypes[wptidx] & AREA) tsk->waypttypes[wptidx] = tsk->waypttypes[wptidx] ^ AREA;
								OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
								if (taskIndex == 0) set_task_changed = true;
								refresh_task_details(TASKDISP);
							}
							savewptidx4 = -1; // forces double click again
						} else {
							savewptidx4 = wptidx + (100 * taskIndex);
						}
//						HostTraceOutputTL(appErrorClass, "selectedTaskWayIdx =|%hd|", selectedTaskWayIdx);
					}
					savewptidx1 = -1;
					break;

				default:
					break;
			}
			if (selectedTaskWayIdx != -1) {
				selectedTaskWayIdx = selectedIdx + (currentTaskPage * 7);
				refresh_task_details(TASKUPNUM); // Just update the waypoint X of X display
			}
			LstSetSelection(lst, selectedIdx);
			LstSetSelection(lst2, selectedIdx);
			LstSetSelection(lst3, selectedIdx);
			LstSetSelection(lst4, selectedIdx);
			DrawHorizListLines(7, 43, 14);

			// strike through name for achieved waypoints in active task
			if ((taskIndex == 0) && ((Int16)(activetskway / 7) == currentTaskPage) && (data.activetask.tskstartsecs > 0)) {
				for (x = currentTaskPage*7; x < activetskway; x++) {
					y = x - currentTaskPage*7;
					if (y < 7) {
						if (x == selectedTaskWayIdx) {
							WinEraseLine(2, 36+y*14, 78, 36+y*14);
						} else {
							 WinDrawLine(2, 36+y*14, 78, 36+y*14);
						}
					}
				}
			}
			handled=true;
			break;
		case fldEnterEvent:
//			HostTraceOutputTL(appErrorClass, "Inside Set Task-fldEnterEvent");
			if (event->data.ctlEnter.controlID == form_set_task_ttldist) {
				PlayKeySound();
				switch (settaskstats) {
					case STATSDIST:
						// toggle displaying distances and percentages
						settaskshowpct = !settaskshowpct;
						break;
					case STATSSPEED:
						// change task speed units
						switch (data.config.tskspdunits) {
							case METRIC:
								data.config.tskspdunits = STATUTE;
								data.input.tskspdconst = SPDMPHCONST;
								StrCopy(data.input.tskspdtext, "mph");
								break;
							case STATUTE:
								data.config.tskspdunits = NAUTICAL;
								data.input.tskspdconst = SPDNAUCONST;
								StrCopy(data.input.tskspdtext, "kts");
								break;
							case NAUTICAL:
								data.config.tskspdunits = METRIC;
								data.input.tskspdconst = SPDKPHCONST;
								StrCopy(data.input.tskspdtext, "kph");
								break;
							default:
								break;
						}
						break;
					case STATSTIME:
						break;
					default:
						break;
				}
				refresh_task_details(TASKDISP);
				handled=true;
			}
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Inside Set Task-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Inside Set Task-winExitEvent-menuopen = true");
			atexit = true;
			menuopen = true;
			save_task_data(0);
			if (!goingtotaskscrn) {
				goingtomenu = true;
				if (!settaskreadonly && (taskIndex == 0) && (data.task.numwaypts > 0) && set_task_changed) {
					OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
					// active task has changes, declare changes?
					question->type = Qacttaskchg;
					FrmPopupForm(form_question);
				}
			}
			goingtotaskscrn = false;			                     	
			handled = false;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "Set Task Close Event");
			if (exittaskreadonly) {
				settaskreadonly = false;
				exittaskreadonly = false;
				copiedtask = false;
			} else if (!settaskreadonly) {
				OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
				if (!taskactvpressed && !goingtotaskscrn && !settaskreadonly && (taskIndex == 0) && (data.task.numwaypts > 0) && set_task_changed) {
					// active task has changes, declare changes?
					question->type = Qacttaskchg;
					FrmPopupForm(form_question);
				} else if (AAT_dist_chg) {
					// reflect target point changes
					setAATdist(AATdist);
					taskIndex = 0;
					refresh_task_details(TASKACTV);
				}
				AAT_dist_chg = false;
				goingtotaskscrn = false;
			}
			if (!allowExit) {
				if (!atexit && !newTaskWay) {
//					HostTraceOutputTL(appErrorClass, "Task List frmCloseEvent");
					//TASKDISP is called here to set taskIndex = 0
					//but only if a true frmCloseEvent was received
					//and we're not going to the waypoint selection
					//window
					refresh_task_details(TASKDISP);
				}
			}
			goingtomenu = false;
			handled=false;
			break;
		default:
			break;
	}
	return(handled);
}

Boolean save_task_data(Int16 next_form)
{
 	Char taskname[15];
 	
	StrCopy(taskname, field_get_str(form_set_task_name));
	if (StrLen(trim(NoComma(taskname," "),' ',true)) == 0) {
		// popup warning for empty name
		field_set_value(form_set_task_name, tsk->name);
		warning->type = Wgeneric;
		StrCopy(warning->line1, "Task name");
		StrCopy(warning->line2, "cannot be blank");
		FrmPopupForm(form_warning);
		return(false);
	} else {
		// only check distance if trying to activate task
		if (taskactvpressed && (tsk->ttldist == 0.0)) next_form = 0;
		StrCopy(tsk->name, trim(NoComma(taskname," "),' ',true));
		if (next_form != 0) {
			FrmGotoForm(next_form);
		} else {
			OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
		}
	}
	return(true);
}

// David Lane - new polar list handler
Boolean form_list_polar_event_handler(EventPtr event)
{
	Boolean handled = false;
	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst;
	UInt16 numrecs;
	static Int16 savepolaridx;
	Int16 polaridx;
	MemHandle polar_hand;
	MemPtr polar_ptr;
	static Boolean pressed_select = false;
	
	lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_polar_list));

	if (data.config.lftunits==STATUTE) {
		StrCopy(SinkText, "fps");
		SinkConst=data.input.lftconst/60.0;
	} else {
		StrCopy(SinkText, data.input.lfttext);
		SinkConst=data.input.lftconst;
	}

	switch (event->eType) {
		case frmOpenEvent:
			numrecs = DmNumRecords(polar_db);
			// David Lane - sort the polar database by name
			if(numrecs > 1) {
				DmQuickSort(polar_db, (DmComparF*)polar_comparison, 0);
			}
               
			// David Lane - if a polar has been selected then find it
			// and move it to the top of the database.
			if (selectedPolarIndex != -1) {
				selectedPolarIndex = FindPolarRecordByName(inusePolar->name);
				if ((numrecs > 1) && selectedPolarIndex != -1) {
					DmMoveRecord(polar_db, selectedPolarIndex, 0);
				}
				selectedPolarIndex = 0;
				currentPolarPage = 0;
			}
		case frmUpdateEvent:
			FrmDrawForm(pfrm);
			savepolaridx = -1;

			// David Lane - redraw the list
			refresh_polar_list(0);

			// David Lane - if no polar is selected then hide the Edit and Delete buttons
			if (selectedPolarIndex == -1) {
				ctl_set_visible(form_list_polar_select, false);
				ctl_set_visible(form_list_polar_edit, false);
				ctl_set_visible(form_list_polar_delete, false);
			} else {
				ctl_set_visible(form_list_polar_select, true);
				ctl_set_visible(form_list_polar_edit, true);
				ctl_set_visible(form_list_polar_delete, true);
			}

			handled=true;
			break;

		case ctlSelectEvent:
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID )
			{
				case form_list_polar_select:
					pressed_select = true;
					FrmGotoForm(form_list_polar);
					break;

				case form_list_polar_edit:
					// David Lane - open the Edit form without a new record
					newPolar = false;
					FrmGotoForm( form_av_polar );
					break;

				case form_list_polar_new:
					// David Lane - open the Edit form with a new "clean" record
					newPolar = true;
					FrmGotoForm( form_av_polar );
					break;

				case form_list_polar_delete:
					if (selectedPolarIndex == 0) {
						warning->type = Wgeneric;
						StrCopy(warning->line1, "Cannot Delete the");
						StrCopy(warning->line2, "Active Polar");
						FrmPopupForm(form_warning);
					} else {
						question->type = Qdelpolar;
						FrmPopupForm(form_question);
					}
					break;

				default:
					break;

			}
			handled=true;
			break;

		case lstSelectEvent:
		{
			MemHandle polar_hand;
			MemPtr polar_ptr;

			if (event->data.lstSelect.listID == form_list_polar_list) {
				if (OpenDBCountRecords(polar_db) > 0) {
					PlayKeySound();
					// David Lane - we've selected a polar so get it's record
					// and show the Edit and Delete buttons.
					selectedPolarIndex = LstGetSelection(lst) + (currentPolarPage * 8);

					OpenDBQueryRecord(polar_db, selectedPolarIndex, &polar_hand, &polar_ptr);
					MemMove(selectedPolar, polar_ptr, sizeof(PolarData));
					MemHandleUnlock(polar_hand);

					ctl_set_visible(form_list_polar_select, true);
					ctl_set_visible(form_list_polar_edit, true);
					ctl_set_visible(form_list_polar_delete, true);

					polaridx = selectedPolarIndex + (currentSUAPage * 7); 
					if (polaridx == savepolaridx) {
						// go to polar edit
						newPolar = false;
						FrmGotoForm( form_av_polar );
						savepolaridx = -1; // forces double click again
					} else {
						savepolaridx = polaridx;
					}
				}
			}
			DrawHorizListLines(8, 28, 14);
			handled=true;
			break;
		}
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Inside polar-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Inside polar-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "Inside polar frmCloseEvent");
			if (!allowExit && pressed_select) {
				*inusePolar = *selectedPolar;
				// David Lane - If we've got a Polar selected then move it to
				// to the first member of the database.  This is to allow
				// the selected polar to be used upon program boot.
				if ( selectedPolarIndex != -1 ) {
					DmMoveRecord(polar_db, selectedPolarIndex, 0);
					selectedPolarIndex = 0;
					currentPolarPage = 0;
					OpenDBQueryRecord(polar_db, selectedPolarIndex, &polar_hand, &polar_ptr);
					MemMove(selectedPolar, polar_ptr, sizeof(PolarData));
					MemHandleUnlock(polar_hand);

					// David Lane - if we're leaving the polar menu then
					// let's move all the pertainent info from the selected
					// polar into the record used by the "other" menus.
					StrCopy(data.polar.name, selectedPolar->name);
					data.polar.maxdrywt = selectedPolar->maxdrywt;
					data.polar.maxwater = selectedPolar->maxwater;

					data.polar.v1 = selectedPolar->v1;
					data.polar.w1 = selectedPolar->w1;
					if (data.polar.w1 > 0.0) {
						data.polar.w1 *= -1.0;
					}

					data.polar.v2 = selectedPolar->v2;
					data.polar.w2 = selectedPolar->w2;
					if (data.polar.w2 > 0.0) {
						data.polar.w2 *= -1.0;
					}

					data.polar.v3 = selectedPolar->v3;
					data.polar.w3 = selectedPolar->w3;
					if (data.polar.w3 > 0.0) {
						data.polar.w3 *= -1.0;
					}

					CalcPolarABC(&data.polar, data.config.bugfactor);

					OpenDBUpdateRecord(polar_db, sizeof(PolarData), &data.polar, POLAR_REC);

				}
			}
			pressed_select = false;
			handled=false;
		default:
			break;

	}
	return(handled);
}

Boolean form_av_polar_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
	static Char sinktext[4], spdtext[4], wgttext[4], wtrtext[4];
	double ldspd=0.0, ldsnk=0.0, ld=0.0, minsnk=0.0;
	Char tempchar[10];
	Boolean OKpolar=false;
	
	switch (event->eType)
	{
		case frmOpenEvent:
			if (newPolar) {
				MemSet(selectedPolar, sizeof(PolarData), 0);
			}
		case frmUpdateEvent:
			FrmDrawForm(pfrm);

                        
			// David Lane - fill in all of the polar fields from
			// the members of the selected polar record
			field_set_value(form_av_polar_name, selectedPolar->name);
			field_set_value(form_av_polar_v1,
				DblToStr(pround(selectedPolar->v1*data.input.spdconst,0),0));
			field_set_value(form_av_polar_w1,
				DblToStr(pround(selectedPolar->w1*SinkConst,
				data.input.lftprec+1),data.input.lftprec+1));
			field_set_value(form_av_polar_v2,
				DblToStr(pround(selectedPolar->v2*data.input.spdconst,0),0));
			field_set_value(form_av_polar_w2,
				DblToStr(pround(selectedPolar->w2*SinkConst,
				data.input.lftprec+1),data.input.lftprec+1));
			field_set_value(form_av_polar_v3,
				DblToStr(pround(selectedPolar->v3*data.input.spdconst,0),0));
			field_set_value(form_av_polar_w3,
				DblToStr(pround(selectedPolar->w3*SinkConst,
				data.input.lftprec+1),data.input.lftprec+1));
			field_set_value(form_av_polar_maxdrywt,
				DblToStr(pround(selectedPolar->maxdrywt*data.input.wgtconst,0),0));
			field_set_value(form_av_polar_maxwater,
				DblToStr(pround(selectedPolar->maxwater*data.input.wtrconst,1),1));

			OKpolar = CalcPolarABC(selectedPolar, 1.0);

			if ((newPolar) || (!OKpolar)) {
				ldspd = 0.0;
				ldsnk = 0.0;
				ld = 0.0;
				minsnk = 0.0;
			} else {
//				HostTraceOutputTL(appErrorClass, "A=|%s|", DblToStr(selectedPolar->a, 6));
//				HostTraceOutputTL(appErrorClass, "B=|%s|", DblToStr(selectedPolar->b, 6));
//				HostTraceOutputTL(appErrorClass, "C=|%s|", DblToStr(selectedPolar->c, 6));
//				HostTraceOutputTL(appErrorClass, "C/A=|%s|", DblToStr(selectedPolar->c/selectedPolar->a, 6));

				ldspd = Sqrt(selectedPolar->c/selectedPolar->a);
//				HostTraceOutputTL(appErrorClass, "ldspd=|%s|", DblToStr(ldspd, 6));

				ldsnk = (selectedPolar->a*ldspd*ldspd) + (selectedPolar->b*ldspd) + selectedPolar->c;
//				HostTraceOutputTL(appErrorClass, "ldsnk=|%s|", DblToStr(ldsnk, 6));

				// Alternate way to calculate
				ld = ldspd / ldsnk * -1.0;
				ld = -1.0/(2.0*selectedPolar->a*Sqrt(selectedPolar->c/selectedPolar->a)+selectedPolar->b);
//				HostTraceOutputTL(appErrorClass, "ld=|%s|", DblToStr(ld, 6));

				minsnk = (-1.0 * selectedPolar->b) / (2.0 * selectedPolar->a);
//				HostTraceOutputTL(appErrorClass, "minsnk=|%s|", DblToStr(minsnk, 6));
			}

			FntSetFont(boldFont);
//			field_set_value(form_av_polar_maxld, DblToStr(pround(ld, 1), 1));
			StrCopy(tempchar, DblToStr(pround(ld, 0), 0));
			WinDrawChars(tempchar, StrLen(tempchar), 25, 128);
//			field_set_value(form_av_polar_ldspd, print_horizontal_speed2(ldspd, 1));
			StrCopy(tempchar, print_horizontal_speed2(ldspd, 0));
			StrCat(tempchar, data.input.spdtext);
			WinDrawChars(tempchar, StrLen(tempchar), 53, 128);
			StrCopy(tempchar, print_horizontal_speed2(minsnk, 0));
			StrCat(tempchar, data.input.spdtext);
			WinDrawChars(tempchar, StrLen(tempchar), 125, 128);
			FntSetFont(stdFont);

			CalcPolarABC(selectedPolar, data.config.bugfactor);

			WinDrawChars(data.input.spdtext, 3, 82, 52);
			WinDrawChars(SinkText, 3, 142, 52);
			WinDrawChars(data.input.spdtext, 3, 82, 67);
			WinDrawChars(SinkText, 3, 142, 67);
			WinDrawChars(data.input.spdtext, 3, 82, 82);
			WinDrawChars(SinkText, 3, 142, 82);
			WinDrawChars(data.input.wgttext, 3, 142, 97);
			WinDrawChars(data.input.wtrtext, 3, 142, 112);

			StrCopy(sinktext, SinkText);
			StrCopy(spdtext, data.input.spdtext);
			StrCopy(wgttext, data.input.wgttext);
			StrCopy(wtrtext, data.input.wtrtext);

			handled=true;
			break;

		case ctlSelectEvent:
			PlayKeySound();
			if (event->data.ctlEnter.controlID == form_av_polar_save) {
				if (save_polar_fields(SinkConst, true)) {
					if (selectedPolarIndex != -1) {
						selectedPolarIndex = FindPolarRecordByName(inusePolar->name);
					}
					FrmGotoForm( form_list_polar );
				}
			} else if (event->data.ctlEnter.controlID == form_av_polar_quit) {
				if (selectedPolarIndex != -1) {
					selectedPolarIndex = FindPolarRecordByName(inusePolar->name);
				}
				FrmGotoForm( form_list_polar );
			}
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
		default:
			break;
	}
	return handled;
}

Boolean form_set_fg_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(pfrm);
			field_set_value(form_set_fg_pctwater, DblToStr(data.config.pctwater * 100.0, 0));
			field_set_value(form_set_fg_bugfactor, DblToStr(data.config.bugfactor * 100.0, 0));
			field_set_value(form_set_fg_safealt, print_altitude(data.config.safealt));
			field_set_value(form_set_fg_thermal_turns, DblToStr(data.config.thermal_turns,0));
			ctl_set_value(form_set_fg_setmcval, data.config.setmcval);
			ctl_set_value(form_set_fg_usepalmway, data.config.usepalmway);
			ctl_set_value(form_set_fg_defaulthome, data.config.defaulthome);
			ctl_set_value(form_set_fg_optforspd, data.config.optforspd);
			ctl_set_value(form_set_fg_refwptbyradial, data.config.RefWptRadial);
			ctl_set_value(form_set_fg_reqdgndspd, data.config.showrgs);
			ctl_set_value(form_set_fg_useterrain, data.config.usefgterrain);

			if (!data.config.usepalmway) {
				data.config.shownextwpt = NEXTOFF;
				ctl_set_value(form_set_fg_shownextwptOFF, true);
				ctl_set_visible(form_set_fg_shownextwptALL, false);
				ctl_set_visible(form_set_fg_shownextwptCTL, false);
				ctl_set_visible(form_set_fg_shownextwptOFF, true);
				ctl_set_visible(form_set_fg_defaulthome, false);
			} else {
				ctl_set_visible(form_set_fg_shownextwptALL, true);
				ctl_set_visible(form_set_fg_shownextwptCTL, true);
				ctl_set_visible(form_set_fg_shownextwptOFF, true);
				ctl_set_visible(form_set_fg_defaulthome, true);
				switch ( data.config.shownextwpt ) {
					case NEXTALL:
						ctl_set_value(form_set_fg_shownextwptALL, true);
						ctl_set_value(form_set_fg_shownextwptCTL, false);
						ctl_set_value(form_set_fg_shownextwptOFF, false);	
						break;
					case NEXTCTL:
						ctl_set_value(form_set_fg_shownextwptALL, false);
						ctl_set_value(form_set_fg_shownextwptCTL, true);
						ctl_set_value(form_set_fg_shownextwptOFF, false);	
						break;
					case NEXTOFF:
						ctl_set_value(form_set_fg_shownextwptALL, false);
						ctl_set_value(form_set_fg_shownextwptCTL, false);
						ctl_set_value(form_set_fg_shownextwptOFF, true);
						break;
					default:
						break;
				}
			}
/*			switch ( data.config.altreftype ) {
				case MSL:
					ctl_set_value(form_set_fg_usemsl, true);
					ctl_set_value(form_set_fg_useqnh, false);
					ctl_set_value(form_set_fg_useagl, false);
					ctl_set_value(form_set_fg_usefl, false);
					break;
				case QFE:
					ctl_set_value(form_set_fg_usemsl, false);
					ctl_set_value(form_set_fg_useqnh, true);
					ctl_set_value(form_set_fg_useagl, false);
					ctl_set_value(form_set_fg_usefl, false);
					break;
				case AGL:
					ctl_set_value(form_set_fg_usemsl, false);
					ctl_set_value(form_set_fg_useqnh, false);
					ctl_set_value(form_set_fg_useagl, true);
					ctl_set_value(form_set_fg_usefl, false);
					break;
				case PALT:
					ctl_set_value(form_set_fg_usemsl, false);
					ctl_set_value(form_set_fg_useqnh, false);
					ctl_set_value(form_set_fg_useagl, false);
					ctl_set_value(form_set_fg_usefl, true);
					break;
				default:
					break;
			}
*/
			WinDrawChars("%  ", 3, 142, 18);
			WinDrawChars("%  ", 3, 142, 33);
			WinDrawChars(data.input.alttext, 2, 142, 48);

			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Inside Set Final Glide-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Inside Set Final Glide-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
			data.config.pctwater = field_get_double(form_set_fg_pctwater) / 100.0;
			if (data.config.flightcomp == C302COMP) {
				Output302GRec(GBALLAST);
				skipballast = true;
			} else {
				data.input.ballast = data.config.pctwater;
			}
			data.config.bugfactor = field_get_double(form_set_fg_bugfactor) / 100.0;
			if ( data.config.bugfactor < 0.5 ) {
				data.config.bugfactor = 0.5;
			}
			if (data.config.flightcomp == C302COMP) {
				Output302GRec(GBUGS);
				skipbugs = true;
			} else {
				data.input.bugs = data.config.bugfactor;
			}
			// Recalculate the Polar Values based on the bugfactor;
			CalcPolarABC(&data.polar, data.config.bugfactor);
			data.config.safealt = field_get_double(form_set_fg_safealt)/data.input.altconst;
			data.config.thermal_turns = field_get_double(form_set_fg_thermal_turns);
			if (data.config.thermal_turns < 1) data.config.thermal_turns = 1;
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_fg_setmcval:
					data.config.setmcval = ctl_get_value(form_set_fg_setmcval);
					handled = true;
					break;
				case form_set_fg_usepalmway:
					if (!inflight || !data.config.usepalmway) {
						data.config.usepalmway = ctl_get_value(form_set_fg_usepalmway);
						if (!data.config.usepalmway) {
							ctl_set_value(form_set_fg_defaulthome, false);
							data.config.defaulthome = false;
						}
					}
					FrmUpdateForm(form_set_fg, frmRedrawUpdateCode);
					handled = true;
					break;
				case form_set_fg_defaulthome:
					data.config.defaulthome = ctl_get_value(form_set_fg_defaulthome);
					FrmUpdateForm(form_set_fg, frmRedrawUpdateCode);
					handled = true;
					break;
				case form_set_fg_optforspd:
					data.config.optforspd = ctl_get_value(form_set_fg_optforspd);
					handled = true;
					break;
				case form_set_fg_refwptbyradial:
					data.config.RefWptRadial = ctl_get_value(form_set_fg_refwptbyradial);
					handled = true;
					break;
/*				case form_set_fg_useqnh:
					data.config.altreftype = QFE;
					handled = true;
					break;
				case form_set_fg_usemsl:
					data.config.altreftype = MSL;
					handled = true;
					break;
				case form_set_fg_useagl:
					data.config.altreftype = AGL;
					handled = true;
					break;
				case form_set_fg_usefl:
					data.config.altreftype = PALT;
					handled = true;
					break;
*/
				case form_set_fg_shownextwptALL:
					if (data.config.usepalmway) data.config.shownextwpt = NEXTALL;
					handled = true;
					break;
				case form_set_fg_shownextwptCTL:
					if (data.config.usepalmway) data.config.shownextwpt = NEXTCTL;
					handled = true;
					break;
				case form_set_fg_shownextwptOFF:
					data.config.shownextwpt = NEXTOFF;
					handled = true;
					break;
				case form_set_fg_reqdgndspd:
					data.config.showrgs = ctl_get_value(form_set_fg_reqdgndspd);
					handled = true;
					break;
				case form_set_fg_useterrain:
					data.config.usefgterrain = ctl_get_value(form_set_fg_useterrain);
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

Boolean form_set_units_event_handler(EventPtr event)
{
	Boolean handled=false;
	Char tempchar[10];
	FormType *pfrm = FrmGetActiveForm();


	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			if (data.config.flightcomp == C302COMP) {
				ctl_set_visible(form_set_units_QNHlabel, false);
				ctl_set_visible(form_set_units_mb, false);
				ctl_set_visible(form_set_units_in, false);
			}
			FrmDrawForm(pfrm);
			StrIToA(tempchar, (Int32)data.config.timezone);
			field_set_value(form_set_units_timezone, tempchar);
			switch (data.config.earthmodel) {
				case EMSPHERE:
					ctl_set_value(form_set_units_sphere, true);
					ctl_set_value(form_set_units_fai, false);
					ctl_set_value(form_set_units_wgs84, false);
					break;
				case EMWGS84:
					ctl_set_value(form_set_units_sphere, false);
					ctl_set_value(form_set_units_fai, false);
					ctl_set_value(form_set_units_wgs84, true);
					break;
				case EMFAI:
					ctl_set_value(form_set_units_sphere, false);
					ctl_set_value(form_set_units_fai, true);
					ctl_set_value(form_set_units_wgs84, false);
					break;
				default:
					break;
			}

			switch (data.config.altunits) {
				case NAUTICAL:
					ctl_set_value(form_set_units_altft, true);
					ctl_set_value(form_set_units_altmt, false);
					break;
				case METRIC:
					ctl_set_value(form_set_units_altft, false);
					ctl_set_value(form_set_units_altmt, true);
					break;
				default:
					break;
			}

			switch (data.config.disunits) {
			case STATUTE:
					ctl_set_value(form_set_units_dissm, true);
					ctl_set_value(form_set_units_disnm, false);
					ctl_set_value(form_set_units_diskm, false);
					break;
				case NAUTICAL:
					ctl_set_value(form_set_units_dissm, false);
					ctl_set_value(form_set_units_disnm, true);
					ctl_set_value(form_set_units_diskm, false);
					break;
				case METRIC:
					ctl_set_value(form_set_units_dissm, false);
					ctl_set_value(form_set_units_disnm, false);
					ctl_set_value(form_set_units_diskm, true);
				break;
			default:
				break;
			}
	
			switch (data.config.spdunits) {
				case STATUTE:
					ctl_set_value(form_set_units_spdmph, true);
					ctl_set_value(form_set_units_spdkts, false);
					ctl_set_value(form_set_units_spdkph, false);
					break;
				case NAUTICAL:
					ctl_set_value(form_set_units_spdmph, false);
					ctl_set_value(form_set_units_spdkts, true);
					ctl_set_value(form_set_units_spdkph, false);
					break;
				case METRIC:
					ctl_set_value(form_set_units_spdmph, false);
					ctl_set_value(form_set_units_spdkts, false);
					ctl_set_value(form_set_units_spdkph, true);
					break;
				default:
					break;
			}

			switch (data.config.lftunits) {
				case STATUTE:
					ctl_set_value(form_set_units_lftfpm, true);
					ctl_set_value(form_set_units_lftkts, false);
					ctl_set_value(form_set_units_lftmps, false);
					break;
				case NAUTICAL:
					ctl_set_value(form_set_units_lftfpm, false);
					ctl_set_value(form_set_units_lftkts, true);
					ctl_set_value(form_set_units_lftmps, false);
					break;
				case METRIC:
					ctl_set_value(form_set_units_lftfpm, false);
					ctl_set_value(form_set_units_lftkts, false);
					ctl_set_value(form_set_units_lftmps, true);
					break;
				default:
					break;
			}
			switch (data.config.wgtunits) {
				case NAUTICAL:
					ctl_set_value(form_set_units_wgtlbs, true);
					ctl_set_value(form_set_units_wgtkg, false);
					break;
				case METRIC:
					ctl_set_value(form_set_units_wgtlbs, false);
					ctl_set_value(form_set_units_wgtkg, true);
					break;
				default:
					break;
			}
			switch (data.config.wtrunits) {
				case NAUTICAL:
					ctl_set_value(form_set_units_wtrgal, true);
					ctl_set_value(form_set_units_wtrltr, false);
					ctl_set_value(form_set_units_wtrimp, false);
					break;
				case METRIC:
					ctl_set_value(form_set_units_wtrgal, false);
					ctl_set_value(form_set_units_wtrltr, true);
					ctl_set_value(form_set_units_wtrimp, false);
					break;
				case STATUTE:
					ctl_set_value(form_set_units_wtrgal, false);
					ctl_set_value(form_set_units_wtrltr, false);
					ctl_set_value(form_set_units_wtrimp, true);
					break;
				default:
					break;
			}
			switch (data.config.wndunits) {
				case STATUTE:
					ctl_set_value(form_set_units_wndmph, true);
					ctl_set_value(form_set_units_wndkts, false);
					ctl_set_value(form_set_units_wndkph, false);
					ctl_set_value(form_set_units_wndmps, false);
					break;
				case NAUTICAL:
					ctl_set_value(form_set_units_wndmph, false);
					ctl_set_value(form_set_units_wndkts, true);
					ctl_set_value(form_set_units_wndkph, false);
					ctl_set_value(form_set_units_wndmps, false);
					break;
				case METRIC:
					ctl_set_value(form_set_units_wndmph, false);
					ctl_set_value(form_set_units_wndkts, false);
					ctl_set_value(form_set_units_wndkph, true);
					ctl_set_value(form_set_units_wndmps, false);
					break;
				case METRICMPS:
					ctl_set_value(form_set_units_wndmph, false);
					ctl_set_value(form_set_units_wndkts, false);
					ctl_set_value(form_set_units_wndkph, false);
					ctl_set_value(form_set_units_wndmps, true);
					break;
				default:
					break;
			}

			switch (data.config.QNHunits) {
				case MILLIBARS:
					ctl_set_value(form_set_units_mb, true);
					ctl_set_value(form_set_units_in, false);
					break;
				case INHG:
					ctl_set_value(form_set_units_mb, false);
					ctl_set_value(form_set_units_in, true);
					break;
				default:
					break;
			}

			if (device.iQueCapable || device.StyleTapPDA) {
				data.config.usegpstime = false;
				ctl_set_visible(form_set_units_usegps, false);
			} else {
				ctl_set_value(form_set_units_usegps, data.config.usegpstime);
			}
			ctl_set_value(form_set_units_keysoundon, data.config.keysoundon);
			ctl_set_value(form_set_units_accuratedist, data.config.accuratedistcalc);
			handled=true;
			break;

		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_units_sphere:
					data.config.earthmodel = EMSPHERE;
					handled = true;
					break;
				case form_set_units_wgs84:
					data.config.earthmodel = EMWGS84;
					handled = true;
					break;
				case form_set_units_fai:
					data.config.earthmodel = EMFAI;
					handled = true;
					break;
				case form_set_units_usegps:
					data.config.usegpstime = ctl_get_value(form_set_units_usegps);
					handled = true;
					break;
				case form_set_units_keysoundon:
					data.config.keysoundon = ctl_get_value(form_set_units_keysoundon);
					handled = true;
					break;
				case form_set_units_accuratedist:
					data.config.accuratedistcalc = ctl_get_value(form_set_units_accuratedist);
					handled = true;
					break;
				default:
					break;
			}
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Inside Set units-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Inside Set Units-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
		{
			Int8 oldaltunits=data.config.altunits;
			Int8 olddisunits=data.config.disunits;
			Int8 oldspdunits=data.config.spdunits;
			Int8 oldlftunits=data.config.lftunits;
			Int8 oldwgtunits=data.config.wgtunits;
			Int8 oldwtrunits=data.config.wtrunits;
			Int8 oldwndunits=data.config.wndunits;
			Int8 oldQNHunits=data.config.QNHunits;

			// Save Altitude Units Setting
			if (ctl_get_value(form_set_units_altft))
				data.config.altunits = NAUTICAL;
			if (ctl_get_value(form_set_units_altmt))
				data.config.altunits = METRIC;

			if (oldaltunits != data.config.altunits) {
				set_unit_constants();
			}

			// Save Distance Units Setting
			if (ctl_get_value(form_set_units_dissm))
				data.config.disunits = STATUTE;
			if (ctl_get_value(form_set_units_disnm))
				data.config.disunits = NAUTICAL;
			if (ctl_get_value(form_set_units_diskm))
				data.config.disunits = METRIC;

			if (olddisunits != data.config.disunits) {
				set_unit_constants();
				data.input.distance_to_destination.value = 0.0;
			}

			// Save Speed Units Setting
			if (ctl_get_value(form_set_units_spdmph))
				data.config.spdunits = STATUTE;
			if (ctl_get_value(form_set_units_spdkts))
				data.config.spdunits = NAUTICAL;
			if (ctl_get_value(form_set_units_spdkph))
				data.config.spdunits = METRIC;

			if (oldspdunits != data.config.spdunits) {
				set_unit_constants();
			}

			// Save Lift Units Setting
			if (ctl_get_value(form_set_units_lftfpm))
				data.config.lftunits = STATUTE;
			if (ctl_get_value(form_set_units_lftkts))
				data.config.lftunits = NAUTICAL;
			if (ctl_get_value(form_set_units_lftmps))
				data.config.lftunits = METRIC;

			if (oldlftunits != data.config.lftunits) {
				set_unit_constants();
				data.input.basemc = 0.0;
				SetMCCurVal();
			}

			// Save Weight Units Setting
			if (ctl_get_value(form_set_units_wgtlbs))
				data.config.wgtunits = NAUTICAL;
			if (ctl_get_value(form_set_units_wgtkg))
				data.config.wgtunits = METRIC;

			if (oldwgtunits != data.config.wgtunits) {
				set_unit_constants();
			}

			// Save Water Units Setting
			if (ctl_get_value(form_set_units_wtrgal))
				data.config.wtrunits = NAUTICAL;
			if (ctl_get_value(form_set_units_wtrltr))
				data.config.wtrunits = METRIC;
			if (ctl_get_value(form_set_units_wtrimp))
				data.config.wtrunits = STATUTE;

			if (oldwtrunits != data.config.wtrunits) {
				set_unit_constants();
			}

			// Save Wind Speed Units Setting
			if (ctl_get_value(form_set_units_wndmph))
				data.config.wndunits = STATUTE;
			if (ctl_get_value(form_set_units_wndkts))
				data.config.wndunits = NAUTICAL;
			if (ctl_get_value(form_set_units_wndkph))
				data.config.wndunits = METRIC;
			if (ctl_get_value(form_set_units_wndmps))
				data.config.wndunits = METRICMPS;

			if (oldwndunits != data.config.wndunits) {
				set_unit_constants();
				data.input.headwind = 0.0;
			}

			// Save QNH units
			if (ctl_get_value(form_set_units_mb)) {
				data.config.QNHunits = MILLIBARS;
			} else {
				data.config.QNHunits = INHG;
			}
			
			if (oldQNHunits != data.config.QNHunits) {
				set_unit_constants();
			}

			data.config.timezone = (Int8)field_get_long(form_set_units_timezone);
			handled=false;
		}
			break;
		default:
			break;
	}
	return handled;
}

Boolean form_set_logger_event_handler(EventPtr event)
{
	Char tempchar[50];
	Boolean handled=false;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			origform = form_set_logger;
			StrCopy(tempchar, "Logger Config - ");

			if (data.config.flightcomp == RECOCOMP) {
//				StrCat(tempchar, GenerateUID(recodata->recoserial));
				StrCat(tempchar, recodata->recoserial);
			} else {
				StrCat(tempchar, GenerateUID(GetSysRomID()));
			}

			frm_set_title(tempchar);
			FrmDrawForm(FrmGetActiveForm());
			GetSysRomID();

			ctl_set_value(form_set_logger_onoff, data.config.logonoff);
			ctl_set_value(form_set_logger_autooff, data.config.logautooff);
			if (data.config.logautooff) {
				field_set_enable(form_set_logger_offspd, true);
				field_set_enable(form_set_logger_offtime, true);
				ctl_set_visible(form_set_logger_stopbtn, logactive);
				field_set_value(form_set_logger_offspd,	DblToStr(pround(data.config.logstopspd*data.input.spdconst,0),0));
				StrIToA(tempchar, data.config.logstoptime);
				field_set_value(form_set_logger_offtime, tempchar);
			} else {
				field_set_value(form_set_logger_offspd, "N/A");
				field_set_value(form_set_logger_offtime, "N/A");
				field_set_enable(form_set_logger_offspd, false);
				field_set_enable(form_set_logger_offtime, false);
				ctl_set_visible(form_set_logger_stopbtn, true);
			}

			if (data.config.flightcomp == C302COMP || 
			    data.config.flightcomp == C302ACOMP ||
			    data.config.flightcomp == GPSNAVCOMP ||
			    data.config.flightcomp == FLARMCOMP ||
			    data.config.flightcomp == EWMRCOMP ||
			    data.config.flightcomp == EWMRSDCOMP ||
			    data.config.flightcomp == EWMRSDTASCOMP ||
			    data.config.flightcomp == RECOCOMP) {
				ctl_set_visible(form_set_logger_loginfo, true);
				switch (data.config.flightcomp) {
					case C302COMP:
					case C302ACOMP:
						ctl_set_label(form_set_logger_loginfo, "C302 Setup");
						break;
					case GPSNAVCOMP:
						ctl_set_label(form_set_logger_loginfo, "GPSNav Setup");
						break;
					case RECOCOMP:
						ctl_set_label(form_set_logger_loginfo, "ReCo Setup");
						break;
					case FLARMCOMP:
						ctl_set_visible(form_set_logger_declaretoSD, true);
						ctl_set_value(form_set_logger_declaretoSD, data.config.declaretoSD);
						ctl_set_label(form_set_logger_loginfo, "Flarm Setup");
						break;
					case EWMRCOMP:
						ctl_set_label(form_set_logger_loginfo, "EWMR Setup");
						data.config.declaretoSD = false;
						break;
					case EWMRSDCOMP:
					case EWMRSDTASCOMP:
						ctl_set_visible(form_set_logger_declaretoSD, true);
						ctl_set_value(form_set_logger_declaretoSD, data.config.declaretoSD);
						ctl_set_label(form_set_logger_loginfo, "EWMR-SD Setup");
						break;
					default:
						break;
				}
			} else {
				ctl_set_visible(form_set_logger_loginfo, false);
			}

			StrIToA(tempchar, data.config.slowlogint);
			field_set_value(form_set_logger_slowlogint, tempchar);
			StrIToA(tempchar, data.config.fastlogint);
			field_set_value(form_set_logger_fastlogint, tempchar);

			field_set_value(form_set_logger_onspd, DblToStr(pround(data.config.logstartspd*data.input.spdconst,0),0));

			StrIToA(tempchar, data.config.nodatatime);
			field_set_value(form_set_logger_nodatatime, tempchar);

			if ((data.config.flightcomp == EWCOMP)     ||
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
			    (data.config.flightcomp == FILSERCOMP)) {
				ctl_set_visible(form_set_logger_declaretasks, true);
				ctl_set_value(form_set_logger_declaretasks, data.config.declaretasks);
			}

			if (data.config.xfertype == USEVFS || data.config.xfertype == USEDOC) {
				ctl_set_visible(form_set_logger_autoIGCxfer, true);
				ctl_set_value(form_set_logger_autoIGCxfer, data.config.autoIGCxfer);
			}
			WinDrawChars(data.input.spdtext, 3, 128, 68);
			WinDrawChars(data.input.spdtext, 3, 128, 85);

			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
//				HostTraceOutputTL(appErrorClass, "calc estimated log time left");
				data.config.slowlogint = field_get_long(form_set_logger_slowlogint);
				if (data.config.slowlogint < 1) data.config.slowlogint = 1;
				if (!menuopen) {
					field_set_value(form_set_logger_timeleft, DblToStr(estlogtimeleft(),1));
					if (lastevent > 0) {
						StrCopy(tempchar, DblToStr((utcsecs-lastevent)/60,0));
						StrCat(tempchar, " mins");
					} else {
						StrCopy(tempchar, "   none");
					}
     					frm_set_label(form_set_logger_eventlbl, tempchar);
				}
			}	
			handled=false;
			break;	
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Inside Set Logger-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Inside Set Logger-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
			data.config.slowlogint = field_get_long(form_set_logger_slowlogint);
			if (data.config.slowlogint < 1) data.config.slowlogint = 1;
			data.config.fastlogint = field_get_long(form_set_logger_fastlogint);
			if (data.config.fastlogint < 1) data.config.fastlogint = 1;
			data.input.logpollint = data.config.slowlogint;
			data.config.logstartspd = field_get_double(form_set_logger_onspd)/data.input.spdconst;
			if (data.config.logstartspd < 1.0/data.input.spdconst) data.config.logstartspd = 1.0/data.input.spdconst;
			if (data.config.logautooff) {
				data.config.logstopspd = field_get_double(form_set_logger_offspd)/data.input.spdconst;
				if (data.config.logstopspd > data.config.logstartspd) data.config.logstopspd = data.config.logstartspd;
				data.config.logstoptime = field_get_long(form_set_logger_offtime);
			}
			data.config.nodatatime = field_get_long(form_set_logger_nodatatime);
			// maintain sensible minimums
			if (data.config.logstoptime < 60) data.config.logstoptime = 60;
			if (data.config.nodatatime < 60) data.config.nodatatime = 60;
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_logger_onoff:
					data.config.logonoff = ctl_get_value(form_set_logger_onoff);
					handled = true;
					break;
				case form_set_logger_autooff:
					data.config.logautooff = ctl_get_value(form_set_logger_autooff);
					if (data.config.logautooff) {
						field_set_enable(form_set_logger_offspd, true);
						field_set_enable(form_set_logger_offtime, true);
						ctl_set_visible(form_set_logger_stopbtn, logactive);
						field_set_value(form_set_logger_offspd, DblToStr(pround(data.config.logstopspd*data.input.spdconst,0),0));
						StrIToA(tempchar, data.config.logstoptime);
						field_set_value(form_set_logger_offtime, tempchar);
					} else {
						field_set_value(form_set_logger_offspd, "N/A");
						field_set_value(form_set_logger_offtime, "N/A");
						field_set_enable(form_set_logger_offspd, false);
						field_set_enable(form_set_logger_offtime, false);
						ctl_set_visible(form_set_logger_stopbtn, true);
					}
					handled = true;
					break;
				case form_set_logger_stopbtn:
					PerformLogging(true);
					handled = true;
					break;
				case form_set_logger_loginfo:
					switch (data.config.flightcomp) {
						case C302COMP:
						case C302ACOMP:
							FrmGotoForm(form_config_caiinst);
							break;
						case RECOCOMP:
							FrmGotoForm(form_config_recoinst);
							break;
						case GPSNAVCOMP:
							FrmGotoForm(form_config_gpsnavinst);
							break;
						case FLARMCOMP:
							FrmGotoForm(form_config_flarminst);
							break;
						case EWMRCOMP:
						case EWMRSDCOMP:
						case EWMRSDTASCOMP:
							FrmGotoForm(form_config_ewmrinst);
							break;
					}
					handled = true;
					break;
				case form_set_logger_declaretasks:
					data.config.declaretasks = ctl_get_value(form_set_logger_declaretasks);
					handled = true;
					break;
				case form_set_logger_declaretoSD:
					data.config.declaretoSD = ctl_get_value(form_set_logger_declaretoSD);
					handled = true;
					break;
				case form_set_logger_event:
					recordevent = true;
					handled = true;
					break;
				case form_set_logger_autoIGCxfer:
					data.config.autoIGCxfer = ctl_get_value(form_set_logger_autoIGCxfer);
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

Boolean form_config_task_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
	ControlPtr ctl;
	ListPtr lst;
	
	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			origform = form_config_task;
			FrmDrawForm(FrmGetActiveForm());

			// Start Info
			switch (data.config.starttype) {
			case TSKLINE:
					ctl_set_value(form_config_task_startline, true);
					ctl_set_value(form_config_task_startfai,  false);
					ctl_set_value(form_config_task_startcirc, false);
					ctl_set_value(form_config_task_startarc, false);
				break;
			case FAI:
					ctl_set_value(form_config_task_startline, false);
					ctl_set_value(form_config_task_startfai,  true);
					ctl_set_value(form_config_task_startcirc, false);
					ctl_set_value(form_config_task_startarc,  false);
				break;
			case CYLINDER:
					ctl_set_value(form_config_task_startline, false);
					ctl_set_value(form_config_task_startfai,  false);
					ctl_set_value(form_config_task_startcirc, true);
					ctl_set_value(form_config_task_startarc,  false);
				break;
			case ARC:
					ctl_set_value(form_config_task_startline, false);
					ctl_set_value(form_config_task_startfai,  false);
					ctl_set_value(form_config_task_startcirc, false);
					ctl_set_value(form_config_task_startarc,  true);
				break;
			default:
				break;
			}
			field_set_value(form_config_task_startrad, print_distance2(data.config.startrad, 3));
			ctl_set_value(form_config_task_startdirauto, data.config.startdirauto);

			if (data.config.startdirmag) {
				// If set to Magnetic, convert from True to Magnetic for display
				field_set_value(form_config_task_startdir, print_direction2(nice_brg(data.config.startdir + data.input.deviation.value)));
				ctl_set_value(form_config_task_startdirm, true);
				ctl_set_value(form_config_task_startdirt, false);
			} else {
				field_set_value(form_config_task_startdir, print_direction2(data.config.startdir));
				ctl_set_value(form_config_task_startdirm, false);
				ctl_set_value(form_config_task_startdirt, true);
			}

			if (data.config.startdirauto) {
				field_set_enable(form_config_task_startdir, false);
				ctl_set_enable(form_config_task_startdirm, false);
				ctl_set_enable(form_config_task_startdirt, false);
			}

			// Turnpoint Info
			switch (data.config.turntype) {
			case FAI:
					ctl_set_value(form_config_task_turnfai, true);
					ctl_set_value(form_config_task_turncirc, false);
					ctl_set_value(form_config_task_turnboth, false);
				break;
			case CYLINDER:
					ctl_set_value(form_config_task_turnfai, false);
					ctl_set_value(form_config_task_turncirc, true);
					ctl_set_value(form_config_task_turnboth, false);
				break;
			case BOTH:
					ctl_set_value(form_config_task_turnfai, false);
					ctl_set_value(form_config_task_turncirc, false);
					ctl_set_value(form_config_task_turnboth, true);
				break;
				default:
					break;
			}

			ctl = (ControlPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_config_task_AATmode_pop1));
			lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_config_task_AATmode_pop2));
			LstSetSelection (lst, data.config.AATmode);
			CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.AATmode));

			ctl_set_value(form_config_task_ctllinevis, data.config.ctllinevis);
			field_set_value(form_config_task_turncircrad, print_distance2(data.config.turncircrad, 3));
			field_set_value(form_config_task_turnfairad, print_distance2(data.config.turnfairad, 3));

			// Finish Info
			switch (data.config.finishtype) {
			case TSKLINE:
					ctl_set_value(form_config_task_finishline, true);
					ctl_set_value(form_config_task_finishfai,  false);
					ctl_set_value(form_config_task_finishcirc, false);
				break;
			case FAI:
					ctl_set_value(form_config_task_finishline, false);
					ctl_set_value(form_config_task_finishfai,  true);
					ctl_set_value(form_config_task_finishcirc, false);
				break;
			case CYLINDER:
					ctl_set_value(form_config_task_finishline, false);
					ctl_set_value(form_config_task_finishfai,  false);
					ctl_set_value(form_config_task_finishcirc, true);
				break;
			default:
				break;
			}
			field_set_value(form_config_task_finishrad, print_distance2(data.config.finishrad, 3));
			ctl_set_value(form_config_task_finishdirauto, data.config.finishdirauto);
			ctl_set_value(form_config_task_restarts, data.config.tskrestarts);

			if (data.config.finishdirmag) {
				// If set to Magnetic, convert from True to Magnetic for display
				field_set_value(form_config_task_finishdir, print_direction2(nice_brg(data.config.finishdir + data.input.deviation.value)));
				ctl_set_value(form_config_task_finishdirm, true);
				ctl_set_value(form_config_task_finishdirt, false);
			} else {
				field_set_value(form_config_task_finishdir, print_direction2(data.config.finishdir));
				ctl_set_value(form_config_task_finishdirm, false);
				ctl_set_value(form_config_task_finishdirt, true);
			}

			if (data.config.finishdirauto) {
				field_set_enable(form_config_task_finishdir, false);
				ctl_set_enable(form_config_task_finishdirm, false);
				ctl_set_enable(form_config_task_finishdirt, false);
			}

			WinDrawChars(data.input.distext, 2,  147, 16);
			WinDrawChars("°", 1, 120, 40);
			WinDrawLine (0, 56, 160, 56);
			WinDrawChars(data.input.distext, 2,  147, 72);
			WinDrawChars(data.input.distext, 2,  147, 87);
			WinDrawLine (0, 117, 160, 117);
			WinDrawChars(data.input.distext, 2,  147, 123);
			WinDrawChars("°", 1, 120, 147);
			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle)FrmGetFirstForm()) {
//				HostTraceOutputTL(appErrorClass, "menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
			handled=false;
			data.config.startrad = field_get_double(form_config_task_startrad)/data.input.disconst;
			data.config.startdir = field_get_double(form_config_task_startdir);
			data.config.finishrad = field_get_double(form_config_task_finishrad)/data.input.disconst;
			data.config.finishdir = field_get_double(form_config_task_finishdir);

			// If set to Magnetic, convert entry from Magnetic to True for storage
			if (data.config.startdirmag) {
				data.config.startdir = nice_brg(data.config.startdir - data.input.deviation.value);
			}
			if (data.config.finishdirmag) {
				data.config.finishdir = nice_brg(data.config.finishdir - data.input.deviation.value);
			}
			if (data.config.turntype != BOTH) {
				data.config.turnfairad = field_get_double(form_config_task_turnfairad)/data.input.disconst;
				data.config.turncircrad = field_get_double(form_config_task_turncircrad)/data.input.disconst;
			} else if ((field_get_double(form_config_task_turnfairad)) > (field_get_double(form_config_task_turncircrad))) {
				data.config.turnfairad = field_get_double(form_config_task_turnfairad)/data.input.disconst;
				data.config.turncircrad = field_get_double(form_config_task_turncircrad)/data.input.disconst;
			} else {
				FrmCustomAlert(WarningAlert, "FAI Sector Radius Must Be Larger Than Cylinder Radius"," "," ");
				// stay on task settings screen
				handled = true;
			}
			// update active task
			if (data.task.numwaypts > 0) CalcStartFinishDirs(&data.task);
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_config_task_startline:
					data.config.starttype = TSKLINE;
					handled = true;
					break;
				case form_config_task_startfai:
					data.config.starttype = FAI;
					handled = true;
					break;
				case form_config_task_startcirc:
					data.config.starttype = CYLINDER;
					handled = true;
					break;
				case form_config_task_startarc:
					data.config.starttype = ARC;
					handled = true;
					break;
				case form_config_task_turnfai:
					data.config.turntype = FAI;
					handled = true;
					break;
				case form_config_task_turncirc:
					data.config.turntype = CYLINDER;
					handled = true;
					break;
				case form_config_task_turnboth:
					data.config.turntype = BOTH;
					handled = true;
					break;
//				case form_config_task_aatoption:
//					data.config.mode = ctl_get_value(form_config_task_aatoption);
//					handled = true;
//					break;
				case form_config_task_ctllinevis:
					data.config.ctllinevis = ctl_get_value(form_config_task_ctllinevis);
					handled = true;
					break;
				case form_config_task_finishline:
					data.config.finishtype = TSKLINE;
					handled = true;
					break;
				case form_config_task_finishfai:
					data.config.finishtype = FAI;
					handled = true;
					break;
				case form_config_task_finishcirc:
					data.config.finishtype = CYLINDER;
					handled = true;
					break;
				case form_config_task_restarts:
					data.config.tskrestarts = ctl_get_value(form_config_task_restarts);
					handled = true;
					break;
				case form_config_task_startdirauto:
					data.config.startdirauto = ctl_get_value(form_config_task_startdirauto);
					if (data.config.startdirauto) {
						field_set_enable(form_config_task_startdir, false);
						ctl_set_enable(form_config_task_startdirm, false);
						ctl_set_enable(form_config_task_startdirt, false);
					} else {
						field_set_enable(form_config_task_startdir, true);
						ctl_set_enable(form_config_task_startdirm, true);
						ctl_set_enable(form_config_task_startdirt, true);
					}
					handled = true;
					break;
				case form_config_task_startdirm:
					data.config.startdirmag = true;
					handled = true;
					break;
				case form_config_task_startdirt:
					data.config.startdirmag = false;
					handled = true;
					break;
				case form_config_task_finishdirauto:
					data.config.finishdirauto = ctl_get_value(form_config_task_finishdirauto);
					if (data.config.finishdirauto) {
						field_set_enable(form_config_task_finishdir, false);
						ctl_set_enable(form_config_task_finishdirm, false);
						ctl_set_enable(form_config_task_finishdirt, false);
					} else {
						field_set_enable(form_config_task_finishdir, true);
						ctl_set_enable(form_config_task_finishdirm, true);
						ctl_set_enable(form_config_task_finishdirt, true);
					}
					handled = true;
					break;
				case form_config_task_finishdirm:
					data.config.finishdirmag = true;
					handled = true;
					break;
				case form_config_task_finishdirt:
					data.config.finishdirmag = false;
					handled = true;
					break;
				case form_config_task_defrules:
					MemSet(tsk, sizeof(TaskData), 0);
					defaultrules = true;
					// copy default rules from config file
					StrCopy(tsk->name, "Default");
					AssignDefaultRules(tsk);
					FrmGotoForm(form_task_rules);
					handled = true;
					break;
				default:
					break;
			}
			break;
		case popSelectEvent:
			PlayKeySound();
			switch (event->data.popSelect.controlID) {
				case form_config_task_AATmode_pop1:
					data.config.AATmode = event->data.popSelect.selection;
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

Boolean form_set_port_event_handler(EventPtr event)
{
	Boolean handled=false;
	Char tempchar[13];
	FormPtr frm;
	ControlPtr ctl;
	ListPtr lst;
	static UInt16 oldspeed;
	static UInt16 oldnmeaxfertype;
	static UInt16 newnmeaxfertype;
	static UInt32 newstdevcreator=0;
	static UInt32 oldstdevcreator=0;
	Boolean atexit=false;
	static Boolean closeperformed = false;
	static Char **nmeaints=NULL;
	static Char **stints=NULL;
	static Int16 nmeatypes[10];
	static Int16 intcount=0;
	static Int16 stintcount=0;
	static UInt32 *stdevcreators=NULL;
	static Int8 oldfltcomp;
	Int16 x;
	DeviceInfoType deviceInfoP;

	frm = FrmGetActiveForm();
	switch (event->eType) {
		case frmOpenEvent:
//		HostTraceOutputTL(appErrorClass, "Inside Set Port-OpenEvent");
		case frmUpdateEvent:
//		HostTraceOutputTL(appErrorClass, "Inside Set Port-UpdateEvent");
			// allocate memory
			if (nmeaints == NULL) {
//				HostTraceOutputTL(appErrorClass, "Inside Set Port-MemPtrNew nmeaints");
				nmeaints = (Char **) MemPtrNew((8) * (sizeof(Char *)));
				for (x=0; x<8; x++) {
					nmeaints[x] = (Char *) MemPtrNew(8 * (sizeof(Char)));
				}
			}
			if (device.StyleTapPDA) {
				// StyleTap Support
				SrmGetDeviceCount(&stintcount);
				if (stints == NULL) {
//					HostTraceOutputTL(appErrorClass, "Inside Set Port-MemPtrNew stints");
					stints = (Char **) MemPtrNew((stintcount) * (sizeof(Char *)));
					for (x=0;x<stintcount;x++) {
						stints[x] = (Char *) MemPtrNew(30 * (sizeof(Char)));
 					}
				}
				if (stdevcreators == NULL) {
//					HostTraceOutputTL(appErrorClass, "Inside Set Port-AllocMem stdevcreators");
					AllocMem((void *)&stdevcreators, stintcount*sizeof(UInt32));
				}
				newstdevcreator = data.config.stcurdevcreator;
				oldstdevcreator = data.config.stcurdevcreator;
			}

			closeperformed = false;
			FrmDrawForm(frm);
			WinDrawLine (0, 103, 160, 103);

			oldspeed = data.config.nmeaspeed;
			oldnmeaxfertype = data.config.nmeaxfertype;
			newnmeaxfertype = data.config.nmeaxfertype;
			oldfltcomp = data.config.flightcomp;

			ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_nmeapop1));
			lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_nmeapop2));
			LstSetSelection (lst, data.config.nmeaspeed);
			LstSetTopItem (lst, data.config.nmeaspeed);
			CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.nmeaspeed));

			ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_datapop1));
			lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_datapop2));
			LstSetSelection (lst, data.config.dataspeed);
			LstSetTopItem (lst, data.config.dataspeed);
			CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.dataspeed));

			ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_altsrc1));
			lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_altsrc2));
			LstSetSelection (lst, data.config.pressaltsrc);
			LstSetTopItem (lst, data.config.pressaltsrc);
			CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.pressaltsrc));

			ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_fltcmp1));
			lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_fltcmp2));
			LstSetSelection (lst, data.config.flightcomp);
			LstSetTopItem (lst, data.config.flightcomp);
			CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.flightcomp));

			if (device.NewSerialPresent || device.CliePDA) {
				if (data.config.flowctrl == XON) {
					data.config.flowctrl = NOFLOW;
				}
				if (data.config.flowctrl == FLOWBOTH) {
					data.config.flowctrl = RTS;
				}
			}
			switch (data.config.flowctrl) {
				case NOFLOW:
					ctl_set_value(form_set_port_noflow, true);
					ctl_set_value(form_set_port_hardflow, false);
					break;
				case RTS:
					ctl_set_value(form_set_port_noflow, false);
					ctl_set_value(form_set_port_hardflow, true);
					break;
				default:
					break;
			}

			ctl_set_value(form_set_port_usechksums, data.config.usechksums);

			if ((data.config.pressaltsrc == GPSALT) || (data.config.pressaltsrc == C302ALT)) {
				ctl_set_visible(form_set_port_usepalt, false);
				data.config.usepalt = false;
			} else {
				ctl_set_visible(form_set_port_usepalt, true);
				ctl_set_value(form_set_port_usepalt, data.config.usepalt);
			}

			if (!device.CardPresent && data.config.xfertype == USEVFS) {
				data.config.xfertype = USESER;
			}

			// find which input types are valid
			intcount = 0;
			SysStringByIndex(form_port_table, USESER, tempchar, 7);
			StrCopy(nmeaints[intcount], tempchar);
			nmeatypes[intcount] = USESER;
			intcount++;
			
			if (device.NewSerialPresent || device.CliePDA) {
				ctl_set_visible(form_set_port_useir, true);
				SysStringByIndex(form_port_table, USEIR, tempchar, 7);
				StrCopy(nmeaints[intcount], tempchar);
				nmeatypes[intcount] = USEIR;
				intcount++;
			} else {
				ctl_set_visible(form_set_port_useir, false);
			}

			if (device.BTCapable) {
				ctl_set_visible(form_set_port_usebt, true);
				SysStringByIndex(form_port_table, USEBT, tempchar, 7);
				StrCopy(nmeaints[intcount], tempchar);
				nmeatypes[intcount] = USEBT;
				intcount++;
			} else {
				ctl_set_visible(form_set_port_usebt, false);
			}

			if (device.iQueCapable) {
				SysStringByIndex(form_port_table, USEIQUE, tempchar, 7);
				StrCopy(nmeaints[intcount], tempchar);
				nmeatypes[intcount] = USEIQUE;
				intcount++;
			}

			if (device.CFCapable) {
				SysStringByIndex(form_port_table, USECF, tempchar, 7);
				StrCopy(nmeaints[intcount], tempchar);
				nmeatypes[intcount] = USECF;
				intcount++;
			}

			if (device.USBCapable) {
				SysStringByIndex(form_port_table, USEUSB, tempchar, 7);
				StrCopy(nmeaints[intcount], tempchar);
				nmeatypes[intcount] = USEUSB;
				intcount++;
			}

			if (device.StyleTapPDA) {
				SysStringByIndex(form_port_table, USEST, tempchar, 8);
				StrCopy(nmeaints[intcount], tempchar);
				nmeatypes[intcount] = USEST;
				intcount++;
			}

			if (device.EXCapable) {
				SysStringByIndex(form_port_table, USEEXT, tempchar, 7);
				StrCopy(nmeaints[intcount], tempchar);
				nmeatypes[intcount] = USEEXT;
				intcount++;
			}

			if (device.CardPresent) {
				ctl_set_visible(form_set_port_usevfs, true);
			} else {
				ctl_set_visible(form_set_port_usevfs, false);
			}

			if (device.NewSerialPresent || device.CliePDA) {
				ctl_set_visible(form_set_port_useir, true);
			} else {
				ctl_set_visible(form_set_port_useir, false);
			}

			ctl_set_value(form_set_port_useiquesim, data.config.useiquesim);
			ctl_set_value(form_set_port_useiqueser, data.config.useiqueser);
			ctl_set_value(form_set_port_calcdev, data.config.gpscalcdev);
			if (data.config.nmeaxfertype == USEIQUE || data.config.nmeaxfertype == USEIQUESER) {
				ctl_set_visible(form_set_port_calcdev, false);
				ctl_set_visible(form_set_port_echoNMEA, false);
				ctl_set_visible(form_set_port_stint1, false);
				ctl_set_visible(form_set_port_useiquesim, true);
				ctl_set_visible(form_set_port_useiqueser, true);
			} else {
				ctl_set_visible(form_set_port_useiquesim, false);
				ctl_set_visible(form_set_port_useiqueser, false);
				ctl_set_visible(form_set_port_calcdev, true);
				ctl_set_visible(form_set_port_stint1, true);
			}

			if (data.config.nmeaxfertype == USEST) {
				ctl_set_visible(form_set_port_stint1, true);
			} else {
				ctl_set_visible(form_set_port_stint1, false);
			}

			if (data.config.nmeaxfertype == USEBT) {
				ctl_set_visible(form_set_port_echoNMEA, true);
				ctl_set_value(form_set_port_echoNMEA, data.config.echoNMEA);
			} else {
				ctl_set_visible(form_set_port_echoNMEA, false);
				data.config.echoNMEA = false;
			}

			// Have to do this to ensure the right line of the button is displayed
			ctl_set_visible(form_set_port_usedoc, true);

			switch(data.config.xfertype) {
				case USESER:
					ctl_set_value(form_set_port_useserial, true);
					break;
				case USEMEMO:
					ctl_set_value(form_set_port_usememo, true);
					break;
				case USEIR:
					ctl_set_value(form_set_port_useir, true);
					break;
				case USEVFS:
					ctl_set_value(form_set_port_usevfs, true);
					break;
				case USEDOC:
					ctl_set_value(form_set_port_usedoc, true);
					break;
				case USEBT:
					ctl_set_value(form_set_port_usebt, true);
					break;
			}

			ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_nmeaint1));
			lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_nmeaint2));
			LstSetListChoices(lst, nmeaints, intcount);
			LstSetHeight(lst, intcount);
//			HostTraceOutputTL(appErrorClass, "intcount=%hd", intcount);
//			HostTraceOutputTL(appErrorClass, "data.config.nmeaxfertype=%hd", data.config.nmeaxfertype);
			for(x=0; x<intcount; x++) {
//				HostTraceOutputTL(appErrorClass, "x=%hd", x);
//				HostTraceOutputTL(appErrorClass, "nmeatypes[x]=%hd", nmeatypes[x]);
				if ((nmeatypes[x] == data.config.nmeaxfertype) ||
					(data.config.nmeaxfertype == USEIQUESER && nmeatypes[x] == USEIQUE)) {
//					HostTraceOutputTL(appErrorClass, "match found setting to %hd", x);
					LstSetSelection (lst, x);
					LstSetTopItem (lst, x);
					CtlSetLabel (ctl, LstGetSelectionText(lst, x));
				}
			}

			if (device.StyleTapPDA) {
				// Build the list of interfaces for StyleTap Support
				ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_stint1));
				lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_stint2));
//				HostTraceOutputTL(appErrorClass, "stintcount=|%hd|", stintcount);
				for (x=0;x<stintcount;x++) {
//					HostTraceOutputTL(appErrorClass, "x-|%hd|", x);
					SrmGetDeviceInfo(x, &deviceInfoP);

//					HostTraceOutputTL(appErrorClass, "deviceInfoP.serDevPortInfoStr=|%s|", deviceInfoP.serDevPortInfoStr);
					StrNCopy(stints[x], deviceInfoP.serDevPortInfoStr, 29);
					stints[x][29] = 0;
//					HostTraceOutputTL(appErrorClass, "stints[x]=|%s|", stints[x]);

					stdevcreators[x] = deviceInfoP.serDevCreator;
					if (stdevcreators[x] == data.config.stcurdevcreator || (x == stintcount-1 && data.config.stcurdevcreator == 0)) {
						LstSetSelection (lst, x);
						LstSetTopItem (lst, x);
						CtlSetLabel (ctl, stints[x]);
					}
				}
				LstSetListChoices(lst, stints, stintcount);
				LstSetHeight(lst, stintcount);
			}

			field_set_value(form_set_port_gpsmsladj, print_altitude(data.config.gpsmsladj));
			ctl_set_value(form_set_port_altrefmsl, !data.config.gpsaltref);
			ctl_set_value(form_set_port_altrefw84, data.config.gpsaltref);
			WinDrawChars(data.input.alttext, 2, 75, 51);

			// close NMEA pass through communication
			XferCloseAlt();

			handled=true;
			break;
		case winEnterEvent:
//			HostTraceOutputTL(appErrorClass, "Inside Set Port-winEnterEvent");
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Inside Set Port-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Inside Set Port-winExitEvent-menuopen = true");
			atexit = true;
			menuopen = true;
			//handled = false;
			//break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "frmCloseEvent");
//			HostTraceOutputTL(appErrorClass, "newnmeaxfertype-|%hd|", newnmeaxfertype);
//			HostTraceOutputTL(appErrorClass, "oldnmeaxfertype-|%hd|", oldnmeaxfertype);
			data.config.gpsmsladj = field_get_double(form_set_port_gpsmsladj)/data.input.altconst;
			if (!atexit && !closeperformed) {
				if (oldnmeaxfertype != newnmeaxfertype || oldspeed != data.config.nmeaspeed || newstdevcreator != oldstdevcreator) {
					// Don't close port and reopen port unless there is a change
					XferClose(oldnmeaxfertype);
					data.config.nmeaxfertype = newnmeaxfertype;
					// Must do this after the above Close so that it will close the old port properly.
					data.config.stcurdevcreator = newstdevcreator;
					SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
					XferInit(tempchar, NFC, data.config.nmeaxfertype);
				}
				// AGM: minor change here to reset preferred BT address in config
				if (data.config.nmeaxfertype == USEBT) {
					if (data.config.echoNMEA) {
						XferInitAlt(4800, NFC, USESER);
					}
					// reset BT address in config to (re)enable BT search
					data.config.BTAddr[5] = 0;
					data.config.BTAddr[4] = 0;
					data.config.BTAddr[3] = 0;
					data.config.BTAddr[2] = 0;
					data.config.BTAddr[1] = 0;
					data.config.BTAddr[0] = 0;
				}
				closeperformed = true;

				// Free memory for nmea dropdown list
				if (nmeaints) {
//					HostTraceOutputTL(appErrorClass, "Inside Set Port-MemPtrFree nmeaints");
					for (x=0; x<8; x++) {
						MemPtrFree(nmeaints[x]);
					}
					MemPtrFree(nmeaints);
					nmeaints = NULL;
				}
				if (device.StyleTapPDA) {
					// Free memory for StyleTap dropdown list
					if (stints) {
//						HostTraceOutputTL(appErrorClass, "Inside Set Port-MemPtrFree stints");
						for (x = 0; x < stintcount; x++) {
//							HostTraceOutputTL(appErrorClass, "Inside Set Port-MemPtrFree stints[%hd]", x);
							MemPtrFree(stints[x]);
						}
						stintcount = 0;
						MemPtrFree(stints);
						stints = NULL;
					}
					if (stdevcreators) {
//						HostTraceOutputTL(appErrorClass, "Inside Set Port-FreeMem stdevcreators");
						FreeMem((void *)&stdevcreators);
						stdevcreators = NULL;
					}
				}

				// Send the command to put the GPSNAV or C302
				// into the proper output mode
				if (oldfltcomp != data.config.flightcomp) {
					if (data.config.flightcomp == C302COMP || 
						 data.config.flightcomp == C302ACOMP ||
						 data.config.flightcomp == GPSNAVCOMP) {
						SendCAIFlightModeStart(true);
					}
				}

				// set EWMR datarate to NMEA data rate
				if (data.config.flightcomp == EWMRCOMP ||
				    data.config.flightcomp == EWMRSDCOMP  ||
				    data.config.flightcomp == EWMRSDTASCOMP) {
					SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
					ewmrdata->datarate = StrAToI(tempchar);
				}

				// Doing this to reset back to the correct lift increment value
				// if switching from the C302 to something else
				set_unit_constants();
			}
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
				case form_set_port_hardflow:
					data.config.flowctrl = RTS;
					handled = true;
					break;
				case form_set_port_noflow:
					data.config.flowctrl = NOFLOW;
					handled = true;
					break;
				case form_set_port_xonflow:
					data.config.flowctrl = XON;
					handled = true;
					break;
				case form_set_port_bothflow:
					data.config.flowctrl = FLOWBOTH;
					handled = true;
					break;
				case form_set_port_usechksums:
					data.config.usechksums = ctl_get_value(form_set_port_usechksums);
					handled = true;
					break;
				case form_set_port_usepalt:
					data.config.usepalt = ctl_get_value(form_set_port_usepalt);
					handled = true;
					break;
				case form_set_port_useir:
					data.config.xfertype = USEIR;
					handled = true;
					break;
				case form_set_port_useserial:
					data.config.xfertype = USESER;
					handled = true;
					break;
				case form_set_port_usememo:
					data.config.xfertype = USEMEMO;
					handled = true;
					break;
				case form_set_port_usevfs:
					data.config.xfertype = USEVFS;
					handled = true;
					break;
				case form_set_port_usedoc:
					data.config.xfertype = USEDOC;
					handled = true;
					break;
				case form_set_port_usebt:
					data.config.xfertype = USEBT;
					handled = true;
					break;
				case form_set_port_useiquesim:
					data.config.useiquesim = ctl_get_value(form_set_port_useiquesim);
					handled = true;
					break;
				case form_set_port_useiqueser:
					data.config.useiqueser = ctl_get_value(form_set_port_useiqueser);
//					if (data.config.nmeaxfertype == USEIQUE || data.config.nmeaxfertype == USEIQUESER) {
					if (newnmeaxfertype == USEIQUE || newnmeaxfertype == USEIQUESER) {
						if (data.config.useiqueser) {
							newnmeaxfertype = USEIQUESER;
						} else {
							newnmeaxfertype = USEIQUE;
						}
					} 
					handled = true;
					break;
				case form_set_port_calcdev:
					data.config.gpscalcdev = ctl_get_value(form_set_port_calcdev);
					handled = true;
					break;
				case form_set_port_altrefmsl:
					data.config.gpsaltref = GPSMSL;
					handled = true;
					break;
				case form_set_port_altrefw84:
					data.config.gpsaltref = GPSWGS84;
					handled = true;
					break;
				case form_set_port_echoNMEA:
					data.config.echoNMEA = ctl_get_value(form_set_port_echoNMEA);
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
				case form_set_port_nmeapop1:
					data.config.nmeaspeed = event->data.popSelect.selection;
					CalcLift(0.0, NULL, -9999.9, RESET);
					handled = false; //Must be set to false for popup to work
					break;
				case form_set_port_datapop1:
					data.config.dataspeed = event->data.popSelect.selection;
					handled = false; //Must be set to false for popup to work
					break;
				case form_set_port_nmeaint1:
					newnmeaxfertype = nmeatypes[event->data.popSelect.selection];
					if (newnmeaxfertype == USEIQUE && data.config.useiqueser) {
						newnmeaxfertype = USEIQUESER;
					} 
					if (newnmeaxfertype == USEIQUE || newnmeaxfertype == USEIQUESER) {
						ctl_set_visible(form_set_port_calcdev, false);
						ctl_set_visible(form_set_port_echoNMEA, false);
						ctl_set_visible(form_set_port_useiquesim, true);
						ctl_set_visible(form_set_port_useiqueser, true);
					} else {
						ctl_set_visible(form_set_port_useiquesim, false);
						ctl_set_visible(form_set_port_useiqueser, false);
						ctl_set_visible(form_set_port_calcdev, true);
					}
					if (newnmeaxfertype == USEST) {
						ctl_set_visible(form_set_port_stint1, true);
					} else {
						ctl_set_visible(form_set_port_stint1, false);
						if (newnmeaxfertype == USEIQUE || newnmeaxfertype == USEIQUESER) {
							ctl_set_visible(form_set_port_useiquesim, true);
						}
					}
					if (newnmeaxfertype == USEBT) {
						ctl_set_visible(form_set_port_echoNMEA, true);
						ctl_set_value(form_set_port_echoNMEA, data.config.echoNMEA);
					} else {
						ctl_set_visible(form_set_port_echoNMEA, false);
						data.config.echoNMEA = false;
					}
					handled = false; //Must be set to false for popup to work
					break;
				case form_set_port_stint1:
					newstdevcreator = stdevcreators[event->data.popSelect.selection];
//					HostTraceOutputTL(appErrorClass, "Set newstdevcreator to-|%lX|", newstdevcreator);
					handled = false; //Must be set to false for popup to work
					break;
				case form_set_port_altsrc1:
					data.config.pressaltsrc = event->data.popSelect.selection;
//					HostTraceOutputTL(appErrorClass, "Pressure Alt |%s|", DblToStr(data.config.pressaltsrc,0));
					if ((data.config.pressaltsrc == GPSALT) || (data.config.pressaltsrc == C302ALT)) {
						ctl_set_visible(form_set_port_usepalt, false);
						data.config.usepalt = false;
					} else {
						ctl_set_visible(form_set_port_usepalt, true);
						ctl_set_value(form_set_port_usepalt, data.config.usepalt);
					}
					handled = false; //Must be set to false for popup to work
					break;
				case form_set_port_fltcmp1:
					data.config.flightcomp = event->data.popSelect.selection;
//					HostTraceOutputTL(appErrorClass, "Flight Comp |%s|", DblToStr(data.config.flightcomp,0));
					data.input.bugs = data.config.bugfactor;
					if (data.config.flightcomp == C302COMP ||
					    data.config.flightcomp == B50COMP  ||
					    data.config.flightcomp == TASCOMP  ||
					    data.config.flightcomp == EWMRSDTASCOMP  ||
					    data.config.flightcomp == B50VLKCOMP  ||
					    data.config.flightcomp == B50LXCOMP ||
					    data.config.flightcomp == WESTBCOMP) {
						data.config.usetas = true;
					} else {
						data.config.usetas = false;
					}
					// The C302 and LX / Colibri both output component wind values
					// msr: incorporated TAS in wind calc when available
					if (data.config.flightcomp == C302COMP   ||
					    data.config.flightcomp == LXCOMP     ||
					    data.config.flightcomp == LXVARCOMP  ||
					    data.config.flightcomp == FILSERCOMP || 
					    data.config.flightcomp == SN10COMP   ||
					    data.config.flightcomp == B50LXCOMP) {
						data.config.useinputwinds = true;
					} else {
						data.config.useinputwinds = false;
					}
					switch (data.config.flightcomp) {
						case C302COMP:
							data.config.pressaltsrc = C302ALT;
							break;
						case C302ACOMP:
							data.config.pressaltsrc = C302AALT;
							break;
						case TASCOMP:
						case EWMRSDTASCOMP:
							data.config.pressaltsrc = TASALT;
							break;
						case RECOCOMP:
							data.config.pressaltsrc = RECOALT;
							data.config.nmeaspeed = 5;
							ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_nmeapop1));
							lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_nmeapop2));
							LstSetSelection (lst, data.config.nmeaspeed);
							LstSetTopItem (lst, data.config.nmeaspeed);
							CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.nmeaspeed));
							break;
						case LXCOMP:
						case LXVARCOMP:
						case B50LXCOMP:
						case FILSERCOMP:
							data.config.pressaltsrc = LXALT;
							break;
						case VOLKSCOMP:
						case B50VLKCOMP:
							data.config.pressaltsrc = VOLKSALT;
							break;
						case GPSNAVCOMP:
							data.config.pressaltsrc = GPSNAVALT;
							break;
						case FLARMCOMP:
							data.config.pressaltsrc = FLARMALT;
							break;
						case SN10COMP:
							data.config.pressaltsrc = SN10ALT;
							break;
						case EWMRCOMP:
							data.config.declaretoSD = false;
						case EWMRSDCOMP:
							data.config.pressaltsrc = EWMRALT;
							data.config.nmeaspeed = 7;
							ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_nmeapop1));
							lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_nmeapop2));
							LstSetSelection (lst, data.config.nmeaspeed);
							LstSetTopItem (lst, data.config.nmeaspeed);
							CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.nmeaspeed));
							break;
						case WESTBCOMP:
							data.config.pressaltsrc = WESTBALT;
							break;
						default:
							break;
					}
					ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_altsrc1));
					lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_port_altsrc2));
					LstSetSelection (lst, data.config.pressaltsrc);
					LstSetTopItem (lst, data.config.pressaltsrc);
					CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.pressaltsrc));
					if ((data.config.pressaltsrc == GPSALT) || (data.config.pressaltsrc == C302ALT)) {
						ctl_set_visible(form_set_port_usepalt, false);
						data.config.usepalt = false;
					} else {
						ctl_set_visible(form_set_port_usepalt, true);
						ctl_set_value(form_set_port_usepalt, data.config.usepalt);
					}
					CalcLift(0.0, NULL, -9999.9, RESET);
					handled = false; //Must be set to false for popup to work
					break;
				default:
					break;
			}
		default:
			break;
	}
	return handled;
}

Boolean form_set_map_event_handler(EventPtr event)
{
	Boolean handled=false;
	Char tempchar[10];
	Int16 tempInt16=0;
	Int32 tempInt32=0;
	FormType *pfrm = FrmGetActiveForm();
	Boolean trailchg = false;
	static double tempscale;
	ControlPtr ctl;
	ListPtr lst;
	
	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(pfrm);
			if (device.colorCapable || device.HiDensityScrPresent) {
				ctl_set_visible(form_set_map_setcolours, true);
				ctl_set_value(form_set_map_setcolours, false);
				ctl_set_visible(form_set_map_settrack, true);
				ctl_set_value(form_set_map_settrack, false);
			} else {
				ctl_set_visible(form_set_map_terbox, true);
				ctl_set_value(form_set_map_terbox, data.input.showterbox);
			}
			if (!device.DIACapable) {
				ctl_set_visible(form_set_map_RHfieldlbl, true);
				ctl_set_visible(form_set_map_RHfieldpop1, true);
			}
			ctl_set_value(form_set_map_wayonoff, data.config.wayonoff);
			StrIToA(tempchar, (Int32)data.config.waymaxlen);
			field_set_value(form_set_map_waymaxlen, tempchar);
			ctl_set_value(form_set_map_wayline, data.config.wayline);
			ctl_set_value(form_set_map_btmlabels, data.config.btmlabels);
//			ctl_set_value(form_set_map_trktrail, data.config.trktrail);
			StrIToA(tempchar, (Int32)data.config.numtrkpts);
			field_set_value(form_set_map_numtrkpts, tempchar);
			StrIToA(tempchar, (Int32)data.config.thnumtrkpts);
			field_set_value(form_set_map_numtrkpts_th, tempchar);
			ctl_set_value(form_set_map_taskonoff, data.config.taskonoff);
			switch (data.config.taskdrawtype) {
				case 0:
					ctl_set_value(form_set_map_taskall, true);
					ctl_set_value(form_set_map_taskpts1, false);
					ctl_set_value(form_set_map_taskpts2, false);
					ctl_set_value(form_set_map_taskpts3, false);
					break;
				case 1:
					ctl_set_value(form_set_map_taskall, false);
					ctl_set_value(form_set_map_taskpts1, true);
					ctl_set_value(form_set_map_taskpts2, false);
					ctl_set_value(form_set_map_taskpts3, false);
					break;
				case 2:
					ctl_set_value(form_set_map_taskall, false);
					ctl_set_value(form_set_map_taskpts1, false);
					ctl_set_value(form_set_map_taskpts2, true);
					ctl_set_value(form_set_map_taskpts3, false);
					break;
				case 3:
					ctl_set_value(form_set_map_taskall, false);
					ctl_set_value(form_set_map_taskpts1, false);
					ctl_set_value(form_set_map_taskpts2, false);
					ctl_set_value(form_set_map_taskpts3, true);
					break;
			}
			ctl_set_value(form_set_map_windarrow, data.config.windarrow);
			field_set_value(form_set_map_declutter, print_distance2(data.config.declutter,0));
			ctl_set_value(form_set_map_inrngcalc, data.config.inrngcalc);
			field_set_value(form_set_map_mapcircrad1, print_distance2(data.config.mapcircrad1,1));
			field_set_value(form_set_map_mapcircrad2, print_distance2(data.config.mapcircrad2,1));
			ctl_set_value(form_set_map_thzoomset, data.config.thzoom != THZOOMOFF);
			field_set_value(form_set_map_thzoom, print_distance2(data.config.thzoomscale, 2));
			tempscale = data.config.thzoomscale;
//			ctl_set_value(form_set_map_tskzoomoff, !data.config.tskzoom);
//			ctl_set_value(form_set_map_tskzoomon, data.config.tskzoom);
			ctl_set_value(form_set_map_tskzoomset, data.config.tskzoom);

			switch (data.config.maporient) {
				case TRACKUP:
					ctl_set_value(form_set_map_trackup, true);
					ctl_set_value(form_set_map_northup, false);
					ctl_set_value(form_set_map_courseup, false);
					break;
				case NORTHUP:
					ctl_set_value(form_set_map_trackup, false);
					ctl_set_value(form_set_map_northup, true);
					ctl_set_value(form_set_map_courseup, false);
					break;
				case COURSEUP:
					ctl_set_value(form_set_map_trackup, false);
					ctl_set_value(form_set_map_northup, false);
					ctl_set_value(form_set_map_courseup, true);
					break;
			}

			switch (data.config.thmaporient) {
				case TRACKUP:
					ctl_set_value(form_set_map_thtrackup, true);
					ctl_set_value(form_set_map_thnorthup, false);
					ctl_set_value(form_set_map_thcourseup, false);
					break;
				case NORTHUP:
					ctl_set_value(form_set_map_thtrackup, false);
					ctl_set_value(form_set_map_thnorthup, true);
					ctl_set_value(form_set_map_thcourseup, false);
					break;
				case COURSEUP:
					ctl_set_value(form_set_map_thtrackup, false);
					ctl_set_value(form_set_map_thnorthup, false);
					ctl_set_value(form_set_map_thcourseup, true);
					break;
			}

			ctl = (ControlPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_map_RHfieldpop1));
			lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_set_map_RHfieldpop2));
			LstSetSelection (lst, data.config.mapRHfield);
			CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.mapRHfield));

			ctl_set_value(form_set_map_thprofile, data.config.thermalprofile);
			WinDrawChars(data.input.distext, 2, 147, 24);
			WinDrawChars(data.input.distext, 2, 147, 36);
			WinDrawChars(data.input.distext, 2, 147, 96);
			WinDrawChars(data.input.distext, 2, 95, 58);
			WinDrawLine(107, 12, 107, 55);
			WinDrawLine(107, 55, 160, 55);
			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Inside Set Map-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Inside Set Map-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
			tempInt32 = field_get_long(form_set_map_numtrkpts);
			if (tempInt32 > 2725) tempInt32 = 2725; // maximum points for 64k memory allocation
			if (tempInt32 < 0) tempInt32 = 0;
			if (data.config.numtrkpts != tempInt32) {
				data.config.numtrkpts = tempInt32;
				trailchg = true;
			}
			tempInt32 = field_get_long(form_set_map_numtrkpts_th);
			if (tempInt32 > 2725) tempInt32 = 2725; // maximum points for 64k memory allocation
			if (tempInt32 < 0) tempInt32 = 0;
			if (data.config.thnumtrkpts != tempInt32) {
				data.config.thnumtrkpts = tempInt32;
				trailchg = true;
			}
			if (trailchg) {
				// free and re-allocate TrkTrail
				InitTrkTrail(true);
				InitTrkTrail(false);
			}
			tempInt16 = (Int16)field_get_long(form_set_map_waymaxlen);
			if (tempInt16 > 12) {
				data.config.waymaxlen = 12;
			} else {
				data.config.waymaxlen = (Int8)tempInt16;
			}
			data.config.mapcircrad1 = field_get_double(form_set_map_mapcircrad1)/data.input.disconst;
			data.config.mapcircrad2 = field_get_double(form_set_map_mapcircrad2)/data.input.disconst;
			data.config.declutter = field_get_double(form_set_map_declutter)/data.input.disconst;
			data.config.thzoomscale = field_get_double(form_set_map_thzoom)/data.input.disconst;
			if (ctl_get_value(form_set_map_thzoomset)) {
				if (data.config.thzoomscale > 0.0) {
					// thermal zoom to fixed scale
					data.config.thzoom = THZOOMFIX;
				} else {
					data.config.thzoom = THZOOMVAR;
					data.config.thzoomscale = 0.0;
					if (data.config.thzoomscale != tempscale) {
						// value has just been set to zero
						// so set default for thermal mode map scale
						data.config.thmapscaleidx = data.config.mapscaleidx;
					}
				}
			} else {
				// thermal zoom mode
				data.config.thzoom = THZOOMOFF;
			}
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_map_setcolours:
					FrmGotoForm(form_set_map_colours);
					handled = true;
					break;
				case form_set_map_settrack:
					FrmGotoForm(form_set_map_track);
					handled = true;
					break;
				case form_set_map_wayonoff:
					data.config.wayonoff = ctl_get_value(form_set_map_wayonoff);
					handled = true;
					break;
				case form_set_map_wayline:
					data.config.wayline = ctl_get_value(form_set_map_wayline);
					handled = true;
					break;
				case form_set_map_btmlabels:
					data.config.btmlabels = ctl_get_value(form_set_map_btmlabels);
					handled = true;
					break;
//				case form_set_map_trktrail:
//					data.config.trktrail = ctl_get_value(form_set_map_trktrail);
//					handled = true;
//					break;
				case form_set_map_thzoomset:
					data.config.thzoom = ctl_get_value(form_set_map_thzoomset);
					handled = true;
					break;
				case form_set_map_taskonoff:
					data.config.taskonoff = ctl_get_value(form_set_map_taskonoff);
					handled = true;
					break;
				case form_set_map_taskall:
					data.config.taskdrawtype = 0;
					handled = true;
					break;
				case form_set_map_taskpts1:
					data.config.taskdrawtype = 1;
					handled = true;
					break;
				case form_set_map_taskpts2:
					data.config.taskdrawtype = 2;
					handled = true;
					break;
				case form_set_map_taskpts3:
					data.config.taskdrawtype = 3;
					handled = true;
					break;
				case form_set_map_windarrow:
					data.config.windarrow = ctl_get_value(form_set_map_windarrow);
					handled = true;
					break;
				case form_set_map_inrngcalc:
					data.config.inrngcalc = ctl_get_value(form_set_map_inrngcalc);
					handled = true;
					break;
//				case form_set_map_tskzoomoff:
//					data.config.tskzoom = false;
//					handled = true;
//					break;
//				case form_set_map_tskzoomon:
//					data.config.tskzoom = true;
//					handled = true;
//					break;
				case form_set_map_tskzoomset:
					data.config.tskzoom = ctl_get_value(form_set_map_tskzoomset);
					handled = true;
					break;
				case form_set_map_trackup:
					data.config.maporient = TRACKUP;
					handled = true;
					break;
				case form_set_map_northup:
					data.config.maporient = NORTHUP;
					handled = true;
					break;
				case form_set_map_courseup:
					data.config.maporient = COURSEUP;
					handled = true;
					break;
				case form_set_map_thtrackup:
					data.config.thmaporient = TRACKUP;
					handled = true;
					break;
				case form_set_map_thnorthup:
					data.config.thmaporient = NORTHUP;
					handled = true;
					break;
				case form_set_map_thcourseup:
					data.config.thmaporient = COURSEUP;
					handled = true;
					break;
				case form_set_map_terbox:
					data.input.showterbox = ctl_get_value(form_set_map_terbox);
					handled = true;
					break;
				case form_set_map_thprofile:
					data.config.thermalprofile = ctl_get_value(form_set_map_thprofile);
					handled = true;
					break;
				default:
					break;
			}
			break;
		case popSelectEvent:  // A popup button was pressed and released.
			PlayKeySound();
			switch (event->data.popSelect.controlID) {
				case form_set_map_RHfieldpop1:
					data.config.mapRHfield = event->data.popSelect.selection;
					handled = false;
					break;
			}
			break;
		default:
			break;
	}
	return handled;
}

Boolean form_set_map_colours_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
	IndexedColorType indexTest;
	Int8 i;
	
	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(pfrm);
			field_set_value(form_set_map_colours_TaskR , DblToStr(data.config.TaskColour.r,0));
			field_set_value(form_set_map_colours_TaskG , DblToStr(data.config.TaskColour.g,0));
			field_set_value(form_set_map_colours_TaskB , DblToStr(data.config.TaskColour.b,0));
			field_set_value(form_set_map_colours_SectorR , DblToStr(data.config.SectorColour.r,0));
			field_set_value(form_set_map_colours_SectorG , DblToStr(data.config.SectorColour.g,0));
			field_set_value(form_set_map_colours_SectorB , DblToStr(data.config.SectorColour.b,0));
			field_set_value(form_set_map_colours_SUAR , DblToStr(data.config.SUAColour.r,0));
			field_set_value(form_set_map_colours_SUAG , DblToStr(data.config.SUAColour.g,0));
			field_set_value(form_set_map_colours_SUAB , DblToStr(data.config.SUAColour.b,0));
			field_set_value(form_set_map_colours_SUAwarnR , DblToStr(data.config.SUAwarnColour.r,0));
			field_set_value(form_set_map_colours_SUAwarnG , DblToStr(data.config.SUAwarnColour.g,0));
			field_set_value(form_set_map_colours_SUAwarnB , DblToStr(data.config.SUAwarnColour.b,0));
			field_set_value(form_set_map_colours_WayptR , DblToStr(data.config.WayptColour.r,0));
			field_set_value(form_set_map_colours_WayptG , DblToStr(data.config.WayptColour.g,0));
			field_set_value(form_set_map_colours_WayptB , DblToStr(data.config.WayptColour.b,0));

			if (device.HiDensityScrPresent) {
				// bold line
				ctl_set_visible(form_set_map_colours_boldTask, true);
				ctl_set_value(form_set_map_colours_boldTask, data.config.BoldTask);
				ctl_set_visible(form_set_map_colours_boldSector, true);
				ctl_set_value(form_set_map_colours_boldSector, data.config.BoldSector);
				ctl_set_visible(form_set_map_colours_boldSUA, true);
				ctl_set_value(form_set_map_colours_boldSUA, data.config.BoldSUA);
			}

			ctl_set_value(form_set_map_colours_boldWaypt, data.config.BoldWaypt);
			ctl_set_value(form_set_map_colours_terbox, data.input.showterbox);
		case frmCloseEvent:
			handled=false;
			break;
		case winExitEvent:
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
		case nilEvent:
			if (!menuopen) {
				// task
				i = 24;
				indexTest = RGB(field_get_double(form_set_map_colours_TaskR),field_get_double(form_set_map_colours_TaskG),field_get_double(form_set_map_colours_TaskB));
				WinSetForeColor(indexTest);
				WinDrawLine(150,i, 150, i+8);
				if (!ctl_get_value(form_set_map_colours_boldTask)) WinSetForeColor(indexWhite);
				WinDrawLine(151,i, 151, i+8);
				// sector
				i += 12;
				indexTest = RGB(field_get_double(form_set_map_colours_SectorR),field_get_double(form_set_map_colours_SectorG),field_get_double(form_set_map_colours_SectorB));
				WinSetForeColor(indexTest);
				WinDrawLine(150,i, 150, i+8);
				if (!ctl_get_value(form_set_map_colours_boldSector)) WinSetForeColor(indexWhite);
				WinDrawLine(151,i, 151, i+8);
				// SUA
				i += 12;
				indexTest = RGB(field_get_double(form_set_map_colours_SUAR),field_get_double(form_set_map_colours_SUAG),field_get_double(form_set_map_colours_SUAB));
				WinSetForeColor(indexTest);
				WinDrawLine(150,i, 150, i+8);
				if (!ctl_get_value(form_set_map_colours_boldSUA)) WinSetForeColor(indexWhite);
				WinDrawLine(151,i, 151, i+8);
				// SUA Warn
				i += 12;
				indexTest = RGB(field_get_double(form_set_map_colours_SUAwarnR),field_get_double(form_set_map_colours_SUAwarnG),field_get_double(form_set_map_colours_SUAwarnB));
				WinSetForeColor(indexTest);
				WinDrawLine(150,i, 150, i+8);
				WinDrawLine(151,i, 151, i+8);
				// Waypoint
				i += 12;
				indexTest = RGB(field_get_double(form_set_map_colours_WayptR),field_get_double(form_set_map_colours_WayptG),field_get_double(form_set_map_colours_WayptB));
				WinSetForeColor(indexTest);
				if (ctl_get_value(form_set_map_colours_boldWaypt)) {
					WinDrawClippedLine(150,i, 150, i+8, SOLID);
					WinDrawClippedLine(151,i, 151, i+8, SOLID);
				} else {
					WinDrawClippedLine(150,i, 150, i+8, DASHED);
					WinDrawClippedLine(151,i, 151, i+8, DASHED);
				}
				// reset colour
				WinSetForeColor(indexBlack);
			}
			handled = false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_map_colours_default:
					// Restore default values
					field_set_value(form_set_map_colours_TaskR , "0");
					field_set_value(form_set_map_colours_TaskG , "0");
					field_set_value(form_set_map_colours_TaskB , "255");
					field_set_value(form_set_map_colours_SectorR , "255");
					field_set_value(form_set_map_colours_SectorG , "0");
					field_set_value(form_set_map_colours_SectorB , "255");
					field_set_value(form_set_map_colours_SUAR , "255");
					field_set_value(form_set_map_colours_SUAG , "192");
					field_set_value(form_set_map_colours_SUAB , "0");
					field_set_value(form_set_map_colours_SUAwarnR , "255");
					field_set_value(form_set_map_colours_SUAwarnG , "0");
					field_set_value(form_set_map_colours_SUAwarnB , "0");
					field_set_value(form_set_map_colours_WayptR , "0");
					field_set_value(form_set_map_colours_WayptG , "0");
					field_set_value(form_set_map_colours_WayptB , "0");
					ctl_set_value(form_set_map_colours_boldTask, false);
					ctl_set_value(form_set_map_colours_boldSector, false);
					ctl_set_value(form_set_map_colours_boldSUA, false);
					ctl_set_value(form_set_map_colours_boldWaypt, false);
					handled = true;
					break;
				case form_set_map_colours_save:
					// Update colour values
					data.config.TaskColour.r = (UInt8)field_get_double(form_set_map_colours_TaskR);
					data.config.TaskColour.g = (UInt8)field_get_double(form_set_map_colours_TaskG);
					data.config.TaskColour.b = (UInt8)field_get_double(form_set_map_colours_TaskB);
					data.config.SectorColour.r = (UInt8)field_get_double(form_set_map_colours_SectorR);
					data.config.SectorColour.g = (UInt8)field_get_double(form_set_map_colours_SectorG);
					data.config.SectorColour.b = (UInt8)field_get_double(form_set_map_colours_SectorB);
					data.config.SUAColour.r = (UInt8)field_get_double(form_set_map_colours_SUAR);
					data.config.SUAColour.g = (UInt8)field_get_double(form_set_map_colours_SUAG);
					data.config.SUAColour.b = (UInt8)field_get_double(form_set_map_colours_SUAB);
					data.config.SUAwarnColour.r = (UInt8)field_get_double(form_set_map_colours_SUAwarnR);
					data.config.SUAwarnColour.g = (UInt8)field_get_double(form_set_map_colours_SUAwarnG);
					data.config.SUAwarnColour.b = (UInt8)field_get_double(form_set_map_colours_SUAwarnB);
					data.config.WayptColour.r = (UInt8)field_get_double(form_set_map_colours_WayptR);
					data.config.WayptColour.g = (UInt8)field_get_double(form_set_map_colours_WayptG);
					data.config.WayptColour.b = (UInt8)field_get_double(form_set_map_colours_WayptB);
					// Update colour index
					indexTask = RGB(data.config.TaskColour.r, data.config.TaskColour.g, data.config.TaskColour.b);
					indexSector = RGB(data.config.SectorColour.r, data.config.SectorColour.g, data.config.SectorColour.b);
					indexSUA = RGB(data.config.SUAColour.r, data.config.SUAColour.g, data.config.SUAColour.b);
					indexSUAwarn = RGB(data.config.SUAwarnColour.r, data.config.SUAwarnColour.g, data.config.SUAwarnColour.b);
					indexWaypt = RGB(data.config.WayptColour.r, data.config.WayptColour.g, data.config.WayptColour.b);
					// update bold values + terrain box
					data.config.BoldTask = ctl_get_value(form_set_map_colours_boldTask);
					data.config.BoldSector = ctl_get_value(form_set_map_colours_boldSector);
					data.config.BoldSUA = ctl_get_value(form_set_map_colours_boldSUA);
					data.config.BoldWaypt = ctl_get_value(form_set_map_colours_boldWaypt);
					data.input.showterbox = ctl_get_value(form_set_map_colours_terbox);
					// Exit to map settings form
					FrmGotoForm(form_set_map);
					handled = true;
					break;
				case form_set_map_colours_done:
					// Exit to map settings form
					FrmGotoForm(form_set_map);
					handled = true;
					break;
				case form_set_map_colours_boldTask:
					handled = true;
					break;
				case form_set_map_colours_boldSector:
					handled = true;
					break;
				case form_set_map_colours_boldSUA:
					handled = true;
					break;
				case form_set_map_colours_boldWaypt:
					handled = true;
					break;
				case form_set_map_colours_terbox:
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

Boolean form_set_map_track_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
	IndexedColorType indexTest;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(pfrm);
			field_set_value(form_set_map_track_sinkR , DblToStr(data.config.SinkColour.r,0));
			field_set_value(form_set_map_track_sinkG , DblToStr(data.config.SinkColour.g,0));
			field_set_value(form_set_map_track_sinkB , DblToStr(data.config.SinkColour.b,0));
			field_set_value(form_set_map_track_weakR , DblToStr(data.config.WeakColour.r,0));
			field_set_value(form_set_map_track_weakG , DblToStr(data.config.WeakColour.g,0));
			field_set_value(form_set_map_track_weakB , DblToStr(data.config.WeakColour.b,0));
			field_set_value(form_set_map_track_strongR , DblToStr(data.config.StrongColour.r,0));
			field_set_value(form_set_map_track_strongG , DblToStr(data.config.StrongColour.g,0));
			field_set_value(form_set_map_track_strongB , DblToStr(data.config.StrongColour.b,0));
			ctl_set_value(form_set_map_track_dynamiccolour, data.config.dynamictrkcolour);
			ctl_set_value(form_set_map_track_mccolour, !data.config.dynamictrkcolour);
			ctl_set_value(form_set_map_track_totalenergy, data.config.totalenergy);
			ctl_set_value(form_set_map_track_netto, data.config.netto);
			if (device.HiDensityScrPresent) {
				ctl_set_visible(form_set_map_track_boldTrack, true);
				ctl_set_value(form_set_map_track_boldTrack, data.config.BoldTrack);
			}
		case frmCloseEvent:
			handled=false;
			break;
		case winExitEvent:
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
		case nilEvent:
			if (!menuopen) {
				if (ctl_get_value(form_set_map_track_dynamiccolour)) {
					field_set_value(form_set_map_track_lessthan_lbl, "Lift < Tlft");
					field_set_value(form_set_map_track_morethan_lbl, "Lift > Tlft");
				} else {
					field_set_value(form_set_map_track_lessthan_lbl, "Lift < MC");
					field_set_value(form_set_map_track_morethan_lbl, "Lift > MC");
				}
				indexTest = RGB(field_get_double(form_set_map_track_sinkR),field_get_double(form_set_map_track_sinkG),field_get_double(form_set_map_track_sinkB));
				WinSetForeColor(indexTest);
				WinDrawLine(150,27,150,37);
				if (!ctl_get_value(form_set_map_track_boldTrack)) WinSetForeColor(indexWhite);
				WinDrawLine(151,27,151,37);
				indexTest = RGB(field_get_double(form_set_map_track_weakR),field_get_double(form_set_map_track_weakG),field_get_double(form_set_map_track_weakB));
				WinSetForeColor(indexTest);
				WinDrawLine(150,40,150,50);
				if (!ctl_get_value(form_set_map_track_boldTrack)) WinSetForeColor(indexWhite);
				WinDrawLine(151,40,151,50);
				indexTest = RGB(field_get_double(form_set_map_track_strongR),field_get_double(form_set_map_track_strongG),field_get_double(form_set_map_track_strongB));
				WinSetForeColor(indexTest);
				WinDrawLine(150,53,150,63);
				if (!ctl_get_value(form_set_map_track_boldTrack)) WinSetForeColor(indexWhite);
				WinDrawLine(151,53,151,63);
				WinSetForeColor(indexBlack);
			}
			handled = false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_map_track_default:
					// Restore default values
					field_set_value(form_set_map_track_sinkR , DblToStr(255,0));
					field_set_value(form_set_map_track_sinkG , DblToStr(0,0));
					field_set_value(form_set_map_track_sinkB , DblToStr(0,0));
					field_set_value(form_set_map_track_weakR , DblToStr(0,0));
					field_set_value(form_set_map_track_weakG , DblToStr(0,0));
					field_set_value(form_set_map_track_weakB , DblToStr(255,0));
					field_set_value(form_set_map_track_strongR , DblToStr(0,0));
					field_set_value(form_set_map_track_strongG , DblToStr(255,0));
					field_set_value(form_set_map_track_strongB , DblToStr(0,0));
					ctl_set_value(form_set_map_track_boldTrack, false);
					ctl_set_value(form_set_map_track_dynamiccolour, false);
					ctl_set_value(form_set_map_track_mccolour, true);
					ctl_set_value(form_set_map_track_totalenergy, false);
					ctl_set_value(form_set_map_track_netto, false);
					handled = true;
					break;
				case form_set_map_track_save:
					// Update colour values
					data.config.SinkColour.r = (UInt8)field_get_double(form_set_map_track_sinkR);
					data.config.SinkColour.g = (UInt8)field_get_double(form_set_map_track_sinkG);
					data.config.SinkColour.b = (UInt8)field_get_double(form_set_map_track_sinkB);
					data.config.WeakColour.r = (UInt8)field_get_double(form_set_map_track_weakR);
					data.config.WeakColour.g = (UInt8)field_get_double(form_set_map_track_weakG);
					data.config.WeakColour.b = (UInt8)field_get_double(form_set_map_track_weakB);
					data.config.StrongColour.r = (UInt8)field_get_double(form_set_map_track_strongR);
					data.config.StrongColour.g = (UInt8)field_get_double(form_set_map_track_strongG);
					data.config.StrongColour.b = (UInt8)field_get_double(form_set_map_track_strongB);
					// Update colour index
					indexSink = RGB(data.config.SinkColour.r, data.config.SinkColour.g, data.config.SinkColour.b);
					indexWeak = RGB(data.config.WeakColour.r, data.config.WeakColour.g, data.config.WeakColour.b);
					indexStrong = RGB(data.config.StrongColour.r, data.config.StrongColour.g, data.config.StrongColour.b);
					// update bold and dynamic colour values
					data.config.BoldTrack = ctl_get_value(form_set_map_track_boldTrack);
					data.config.dynamictrkcolour = ctl_get_value(form_set_map_track_dynamiccolour);
					data.config.totalenergy = ctl_get_value(form_set_map_track_totalenergy);
					data.config.netto = ctl_get_value(form_set_map_track_netto);
					// Exit to colour settings form
					FrmGotoForm(form_set_map);
					handled = true;
					break;
				case form_set_map_track_done:
					// Exit to colour settings form
					FrmGotoForm(form_set_map);
					handled = true;
					break;
				case form_set_map_track_boldTrack:
					handled = true;
					break;
				case form_set_map_track_netto:
					ctl_set_value(form_set_map_track_totalenergy, false);
					break;
				case form_set_map_track_totalenergy:
					ctl_set_value(form_set_map_track_netto, false);
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

Boolean form_set_pilot_event_handler(EventPtr event)
{
	Boolean handled=false;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(FrmGetActiveForm());
			field_set_value(form_set_pilot_name, data.igchinfo.name);
			field_set_value(form_set_pilot_type, data.igchinfo.type);
			field_set_value(form_set_pilot_gid, data.igchinfo.gid);
			field_set_value(form_set_pilot_cid, data.igchinfo.cid);
			field_set_value(form_set_pilot_class, data.igchinfo.cls);
			field_set_value(form_set_pilot_site, data.igchinfo.site);
			field_set_value(form_set_pilot_ooid, data.igchinfo.ooid);
			field_set_value(form_set_pilot_gpsmodel, data.igchinfo.gpsmodel);
			field_set_value(form_set_pilot_gpsser, data.igchinfo.gpsser);
			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Inside Set Pilot-winExitEvent-menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Inside Set Pilot-winEnterEvent-menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
			StrCopy(data.igchinfo.name, field_get_str(form_set_pilot_name));
			StrCopy(data.igchinfo.type, field_get_str(form_set_pilot_type));
			StrCopy(data.igchinfo.gid, field_get_str(form_set_pilot_gid));
			StrCopy(data.igchinfo.cid, field_get_str(form_set_pilot_cid));
			StrCopy(data.igchinfo.cls, field_get_str(form_set_pilot_class));
			StrCopy(data.igchinfo.site, field_get_str(form_set_pilot_site));
			StrCopy(data.igchinfo.ooid, field_get_str(form_set_pilot_ooid));
			StrCopy(data.igchinfo.gpsmodel, field_get_str(form_set_pilot_gpsmodel));
			StrCopy(data.igchinfo.gpsser, field_get_str(form_set_pilot_gpsser));
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				default:
					break;
			}
			break;
		default:
			break;
	}
	return handled;
}

void form_transfer_hide_fields(Boolean all)
{
	// Hide everything at this point
	if (all) {
//		ctl_set_visible(form_transfer_waycai, false);
//		ctl_set_visible(form_transfer_waycu, false);
//		ctl_set_visible(form_transfer_waygpwpl, false);
//		ctl_set_visible(form_transfer_waygpwplalt, false);
//		ctl_set_visible(form_transfer_waycu_key, false);
		ctl_set_visible(form_transfer_lrecords, false);
		ctl_set_visible(form_transfer_records, false);
		ctl_set_visible(form_transfer_recvbtn, false);
		ctl_set_visible(form_transfer_loggerdata, false);
		ctl_set_visible(form_transfer_loggerpop1, false);
		ctl_set_visible(form_transfer_loggerlbl, false);
		ctl_set_visible(form_transfer_tologgerbtn, false);
		ctl_set_visible(form_transfer_fmloggerbtn, false);
		ctl_set_visible(form_transfer_delloggerbtn, false);
		ctl_set_visible(form_transfer_openair, false);
		ctl_set_visible(form_transfer_tnp, false);
		ctl_set_visible(form_transfer_waypop1, false);
	}
	ctl_set_visible(form_transfer_linclude, false);
	ctl_set_visible(form_transfer_igc_wypts, false);
	ctl_set_visible(form_transfer_igc_tasks, false);
	ctl_set_visible(form_transfer_xmitbtn, false);
	ctl_set_visible(form_transfer_delbtn, false);
	ctl_set_visible(form_transfer_delonebtn, false);
	ctl_set_visible(form_transfer_lflight, false);
	ctl_set_visible(form_transfer_pop1, false);
	ctl_set_visible(form_transfer_lpoints, false);
	ctl_set_visible(form_transfer_points, false);

}

Boolean form_transfer_event_handler(EventPtr event)
{
	Boolean handled = false;
	FormPtr pfrm;
	static Int16 nflights = 0;
	static Int32 npoints;
	static Int16 nwaypts;
	static Int16 npolars;
	static Int16 ntasks;
	static Int16 nsua;
	Char tempchar[30];
	Char tempchar2[30];
	static Int16 database = 0;
	static Int16 loggerdatatype = CAI_WAYPOINTDATA;
	Int16 fltidx;
	Int32 nrecs;
	MemHandle output_hand;
	MemPtr output_ptr;
	UInt16 AlertResp = 0;
	static ListPtr lst;
	static ControlPtr ctl;
	static Char **items = NULL;
	Int32 x=0, n=0;
	MemHandle flt_hand;
	MemPtr flt_ptr;
	static Int16 selectedFlt;
	DateTimeType strtdt;
	Boolean initstat = true;
	EventType newEvent;
	UInt16 controlID;
	UInt32 filename_len;

	pfrm = FrmGetActiveForm();

	switch (event->eType)
	{
		case frmOpenEvent:
		case frmUpdateEvent:
		{
			StrCopy(tempchar2,"Transfer - ");
			SysStringByIndex(form_port_table, data.config.xfertype, tempchar, 7);
			StrCat(tempchar2,tempchar);
			if (data.config.xfertype == USESER) {
				StrCat(tempchar2,":");
				SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
				StrCat(tempchar2,tempchar);
			}
			frm_set_title(tempchar2);
			FrmDrawForm(pfrm);
			ctl = (ControlPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_transfer_pop1));
			lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_transfer_pop2));

			// If we have any flights we'll need to free the
			// memory we allocated during construction of the
			// drop down list.
			if (nflights > 0) {
				for (x = 0; x < (nflights+1); x++) {
					MemPtrFree(items[x]);
				}
			}
			nflights = 0;
			if (items) {
				MemPtrFree(items);
				items = NULL;
			}
//			HostTraceOutputTL(appErrorClass, "Building Flight List....");
			// Prepare a drop down list of flight start times,
			// including the last element "All", and relabel the
			// latest flight to "Current".
			selectedFlt = -1;
			nflights = OpenDBCountRecords(flight_db);
			if (nflights > 0) {
				if (items == NULL) {
					items = (char **) MemPtrNew((nflights+1) * (sizeof(char *)));
					for (x = 0; x < nflights; x++) {
						items[x] = (char *) MemPtrNew(20 * (sizeof(char)));
						OpenDBQueryRecord(flight_db, (nflights - 1 - x), &flt_hand, &flt_ptr);
						MemMove(fltdata, flt_ptr, sizeof(FlightData));
						MemHandleUnlock(flt_hand);
						StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &strtdt);
						if (data.config.timezone != 0) {
							TimAdjust(&strtdt, ((Int32)data.config.timezone * 3600 ));
						}
						// Build the Date
						DateToAscii(strtdt.month, strtdt.day, strtdt.year, (DateFormatType)PrefGetPreference(prefDateFormat), tempchar2);
						StrCopy(items[x], tempchar2);
						// Add a dash
						StrCat(items[x], "-");
						// Add the Time
						StrIToA(tempchar, (UInt32)strtdt.hour);
						StrCopy(tempchar2, leftpad(tempchar, '0', 2));
						StrCat(tempchar2, ":");
						StrIToA(tempchar, (UInt32)strtdt.minute);
						StrCat(tempchar2, leftpad(tempchar, '0', 2));
						StrCat(items[x], tempchar2);
					}
					items[nflights] = (char *) MemPtrNew(40 * (sizeof(char)));
					StrCopy(items[nflights], "All");
				}
				LstSetListChoices(lst, items, (nflights+1));
				if ((nflights+1) < 5) {
					LstSetHeight(lst, (nflights+1));
				} else {
					LstSetHeight(lst, 5);
				}
				LstSetTopItem (lst, 0);
				selectedFlt = (nflights-1);
//				HostTraceOutputTL(appErrorClass, "selectedFlt-|%hd|", selectedFlt);
				CtlSetLabel (ctl, "Current");
			}

			// hide all fields
			form_transfer_hide_fields(true);
			ctl_set_value(form_transfer_flight, false);
			ctl_set_value(form_transfer_waypoint, false);
			ctl_set_value(form_transfer_polar, false);
			ctl_set_value(form_transfer_task, false);
			ctl_set_value(form_transfer_config, false);
			ctl_set_value(form_transfer_sua, false);
			ctl_set_value(form_transfer_loggerdata, false);

			// set flight options
			ctl_set_value(form_transfer_igc_wypts, data.config.output_wypts);
			ctl_set_value(form_transfer_igc_tasks, data.config.output_tasks);
			// set SUA formats
			ctl_set_value(form_transfer_openair, data.config.SUAformat==SUAOPENAIR);
			ctl_set_value(form_transfer_tnp, data.config.SUAformat==SUATNP);
			// set waypoint formats
//			ctl_set_value(form_transfer_waycai, data.config.wpformat==WPCAI);
//			ctl_set_value(form_transfer_waycu,  data.config.wpformat==WPCU);
//			ctl_set_value(form_transfer_waygpwpl, data.config.wpformat==WPGPWPL);
			ctl = (ControlPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm, form_transfer_waypop1));
			lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm, form_transfer_waypop2));
			LstSetSelection (lst, data.config.wpformat);
			CtlSetLabel (ctl, LstGetSelectionText(lst, data.config.wpformat));

			// external loggers
			if (data.config.flightcomp == C302COMP || data.config.flightcomp == C302ACOMP) {
				ctl_set_label(form_transfer_loggerdata, "C302 Data");
				ctl_set_visible(form_transfer_loggerdata, true);
				loggerdatatype = CAI_FLIGHTDATA;
				ctl_set_enable(form_transfer_loggerpop1, true); // ensure we can pick from the list
			} else if ((data.config.flightcomp == VOLKSCOMP) || (data.config.flightcomp == B50VLKCOMP)) {
				ctl_set_label(form_transfer_loggerdata, "Volks Data");
    				ctl_set_visible(form_transfer_loggerdata, true);
				loggerdatatype = VLK_FLIGHTDATA;
				ctl_set_enable(form_transfer_loggerpop1, false); // disable list as only flight transfer supported
			} else if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP)) {
				ctl_set_label(form_transfer_loggerdata, "LX Data");
    				ctl_set_visible(form_transfer_loggerdata, true);
				loggerdatatype = LX_FLIGHTDATA;
				ctl_set_enable(form_transfer_loggerpop1, false); // disable list as only flight transfer supported
			} else if (data.config.flightcomp == FILSERCOMP) {
				ctl_set_label(form_transfer_loggerdata, "Filser Data");
    				ctl_set_visible(form_transfer_loggerdata, true);
				loggerdatatype = LX_FLIGHTDATA;
				ctl_set_enable(form_transfer_loggerpop1, false); // disable list as only flight transfer supported
			} else if (data.config.flightcomp == GPSNAVCOMP) {
				ctl_set_label(form_transfer_loggerdata, "GPSNAV Data");
    				ctl_set_visible(form_transfer_loggerdata, true);
				loggerdatatype = GPN_FLIGHTDATA;
				ctl_set_enable(form_transfer_loggerpop1, true); // ensure we can pick from the list
			} else if (data.config.flightcomp == FLARMCOMP) {
				ctl_set_label(form_transfer_loggerdata, "Flarm Data");
    				ctl_set_visible(form_transfer_loggerdata, true);
				loggerdatatype = FLRM_FLIGHTDATA;
				ctl_set_enable(form_transfer_loggerpop1, false); // disable list as only flight transfer supported
			}
			xfrdialog = XFRXFR; // default setting;

			// push dummy events into event queue to trigger receiving or transmission
			// after filename has been selected from file list
			// transfer_filename string contains selected filename
//			HostTraceOutputTL(appErrorClass, "Pushing Events - %s", DblToStr(receive_file_type,0));
			switch (io_type) {
				case IO_RECEIVE:	
					controlID = form_transfer_recvbtn;
					break;
				case IO_TRANSMIT:	
					controlID = form_transfer_xmitbtn;
					break;
				default:
					controlID = form_transfer;
					break;
			}
			io_type = IO_NONE;
			switch (io_file_type) {
				case CONFIG_SCG:
					ctl_set_value(form_transfer_config, true);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_transfer_config;
					EvtAddEventToQueue(&newEvent);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = controlID;
					EvtAddEventToQueue(&newEvent);
					break;
				case WAYPOINTS_DAT:
				case WAYPOINTS_CUP:
				case WAYPOINTS_WPL:
				case WAYPOINTS_OZI:
					ctl_set_value(form_transfer_waypoint, true);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_transfer_waypoint;
					EvtAddEventToQueue(&newEvent);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = controlID;
					EvtAddEventToQueue(&newEvent);
					break;
				case TASKS_SPT:
					ctl_set_value(form_transfer_task, true);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_transfer_task;
					EvtAddEventToQueue(&newEvent);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = controlID;
					EvtAddEventToQueue(&newEvent);
					break;
				case POLARS_SPL:
					ctl_set_value(form_transfer_polar, true);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_transfer_polar;
					EvtAddEventToQueue(&newEvent);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = controlID;
					EvtAddEventToQueue(&newEvent);
					break;
				case SUADATA_TNP:
				case SUADATA_OPENAIR:
					ctl_set_value(form_transfer_sua, true);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_transfer_sua;
					EvtAddEventToQueue(&newEvent);
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = controlID;
					EvtAddEventToQueue(&newEvent);
					break;
				case NO_FILE_TYPE:
//					HostTraceOutputTL(appErrorClass, "No File Type");
				default:
//					HostTraceOutputTL(appErrorClass, "No Events, Clear Filename");
					io_file_type = NO_FILE_TYPE;
					io_type = IO_NONE;
					transfer_filename[0] = 0;	
					break;				
			}
			// close NMEA pass through communication
			XferCloseAlt();

			handled=true;
			break;
		}

		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Transfer menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Transfer menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
 		{
			if (data.parser.parser_func != nmea_parser) {
				// close transfer and re-start nmea parser
				XferClose(data.config.xfertype);
				SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
				XferInit(tempchar, NFC, data.config.nmeaxfertype);
				nmea_parser(NULL, 0, true);
				data.parser.parser_func = nmea_parser;
				ctl_set_enable(form_transfer_xmitbtn, true);
				ctl_set_enable(form_transfer_delbtn, true);
				ctl_set_label(form_transfer_recvbtn, "Receive");
			}
			// initialise NMEA pass through communication
			if ((data.config.nmeaxfertype == USEBT) && data.config.echoNMEA)
				XferInitAlt(4800, NFC, USESER);
			// If we have any flights we'll need to free the
			// memory we allocated during construction of the
			// drop down list.
			if (nflights > 0) {
				for (x = 0; x < (nflights+1); x++) {
					MemPtrFree(items[x]);
				}
			}
			nflights = 0;
			if (items) {
				MemPtrFree(items);
				items = NULL;
			}
			handled=false;
			break;
		}

		case popSelectEvent:
		{
			PlayKeySound();
			switch (event->data.popSelect.controlID) {
				// We've just made a seledction from the drop down list.
				case form_transfer_waypop1:
				{
					data.config.wpformat = event->data.popSelect.selection;
				}
				break;
				case form_transfer_pop1:
				{
					npoints = OpenDBCountRecords(logger_db);
					selectedFlt = event->data.popSelect.selection;
					if (selectedFlt != nflights ) {
						selectedFlt = (nflights - 1) - event->data.popSelect.selection;
						OpenDBQueryRecord(flight_db, selectedFlt, &flt_hand, &flt_ptr);
						MemMove(fltdata,flt_ptr,sizeof(FlightData));
						MemHandleUnlock(flt_hand);
						npoints = fltdata->stopidx - fltdata->startidx + 1;
					}
					StrIToA(tempchar, npoints);
					field_set_value(form_transfer_points, tempchar);
				}
				break;
				case form_transfer_loggerpop1: 
				{
					loggerdatatype = event->data.popSelect.selection;
					// increment types for Volks/LX/GPSNAV loggers
					if ((data.config.flightcomp == VOLKSCOMP) || (data.config.flightcomp == B50VLKCOMP)) loggerdatatype += 10;
					if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == FILSERCOMP) || (data.config.flightcomp == B50LXCOMP)) loggerdatatype += 20;
					if (data.config.flightcomp == GPSNAVCOMP) loggerdatatype += 30;

					// hide buttons
					ctl_set_visible(form_transfer_xmitbtn, false);
					ctl_set_visible(form_transfer_recvbtn, false);
					ctl_set_visible(form_transfer_tologgerbtn, false);
					ctl_set_visible(form_transfer_fmloggerbtn, false);
					ctl_set_visible(form_transfer_delloggerbtn, false);

					switch (loggerdatatype)
					{
						case CAI_WAYPOINTDATA:
						case GPN_WAYPOINTDATA:
							ctl_set_visible(form_transfer_tologgerbtn, true);
							ctl_set_visible(form_transfer_delloggerbtn, true);
							break;
						case VLK_WAYPOINTDATA:
						case LX_WAYPOINTDATA:
						case FLRM_WAYPOINTDATA:
							ctl_set_visible(form_transfer_tologgerbtn, true);
							break;
						case CAI_GLIDERDATA:
						case VLK_GLIDERDATA:
						case LX_GLIDERDATA:
						case GPN_GLIDERDATA:
						case FLRM_GLIDERDATA:
							ctl_set_visible(form_transfer_tologgerbtn, true);
							break;
						case CAI_PILOTDATA:
						case VLK_PILOTDATA:
						case LX_PILOTDATA:
						case GPN_PILOTDATA:
						case FLRM_PILOTDATA:
							ctl_set_visible(form_transfer_tologgerbtn, true);
							break;
						case LX_FLIGHTDATA:
						case FLRM_FLIGHTDATA:
							if (data.config.xfertype == USEVFS || data.config.xfertype == USEDOC) {
								ctl_set_visible(form_transfer_fmloggerbtn, true);
							} else {
								FrmCustomAlert(WarningAlert, "Unsupported Data Xfer Type Selected.\n\n","Select DOC or Card From NMEA/Port Config Screen.", " ");
							}
							break;
						case CAI_FLIGHTDATA:
						case GPN_FLIGHTDATA:
						case VLK_FLIGHTDATA:
							ctl_set_visible(form_transfer_delloggerbtn, true);
							if (data.config.xfertype == USEVFS || data.config.xfertype == USEDOC) {
								ctl_set_visible(form_transfer_fmloggerbtn, true);
							} else {
								FrmCustomAlert(WarningAlert, "Unsupported Data Xfer Type Selected.\n\n","Select DOC or Card From NMEA/Port Config Screen.", " ");
							}
							break;
						default:
							break;
					}
				}
				break;
			}
			handled = false;
			break;
		} // case popSelectEvent:

		case ctlSelectEvent:
			PlayKeySound();
			if ((data.parser.parser_func != nmea_parser) && (event->data.ctlEnter.controlID != form_transfer_recvbtn)) {
				// receive operation still in progress, so close and restart nmea parser
				XferClose(data.config.xfertype);
				SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
				XferInit(tempchar, NFC, data.config.nmeaxfertype);
				nmea_parser(NULL, 0, true);
				data.parser.parser_func = nmea_parser;
				ctl_set_enable(form_transfer_xmitbtn, true);
				ctl_set_enable(form_transfer_delbtn, true);
				ctl_set_label(form_transfer_recvbtn, "Receive");
			}

			switch ( event->data.ctlEnter.controlID )
			{
				case form_transfer_setport:
					FrmGotoForm(form_set_port);
					handled = true;
					break;

				case form_transfer_pop1:
				{
					handled = false;
					break;
				} // case form_transfer_pop1:

				case form_transfer_loggerpop1:
				{
					handled = false;
					break;
				} // case form_transfer_loggerpop1:

				case form_transfer_igc_wypts:
				{
					data.config.output_wypts = ctl_get_value(form_transfer_igc_wypts);
					handled = true;
					break;
				} // case form_transfer_igc_wypts:

				case form_transfer_igc_tasks:
				{
					data.config.output_tasks = ctl_get_value(form_transfer_igc_tasks);
					handled = true;
					break;
				} // case form_transfer_igc_tasks:
/*
//				case form_transfer_waycai:
//				{
//					data.config.wpformat = WPCAI;
//					if (OpenDBCountRecords(waypoint_db) > 0) {
//						ctl_set_visible(form_transfer_xmitbtn, true);
//					}
//					ctl_set_visible(form_transfer_waycu_key, false);
//					ctl_set_visible(form_transfer_waygpwplalt, false);
//					handled = true;
//					break;
//				} // case form_transfer_waycai:
//
//				case form_transfer_waycu:
//				{
//					data.config.wpformat = WPCU;
//					if (OpenDBCountRecords(waypoint_db) > 0) {
//						ctl_set_visible(form_transfer_xmitbtn, true);
//					}
//					ctl_set_visible(form_transfer_waygpwplalt, false);
//					ctl_set_visible(form_transfer_waycu_key, true);
//					handled = true;
//					break;
//				} // case form_transfer_waycu:
//
//				case form_transfer_waygpwpl:
//				{
//					data.config.wpformat = WPGPWPL;
//					if (OpenDBCountRecords(waypoint_db) > 0) {
//						ctl_set_visible(form_transfer_xmitbtn, true);
//					}
//					ctl_set_visible(form_transfer_waycu_key, false);
//					ctl_set_visible(form_transfer_waygpwplalt, true);
//					handled = true;
//					break;
//				} // case form_transfer_waygpwpl:
//
//				case form_transfer_waygpwplalt:
//				{
//					data.config.GPWPLalt = ctl_get_value(form_transfer_waygpwplalt);
//					handled = true;
//					break;
//				} // case form_transfer_waygpwplalt:
//
//				case form_transfer_waycu_key:
//				{
//					CUkeyoncode = !CUkeyoncode;
//					if (CUkeyoncode) {
//						ctl_set_label(form_transfer_waycu_key, "By:Code");
//					} else {
//						ctl_set_label(form_transfer_waycu_key, "By:Name");
//					}
//					handled = true;
//					break;
//				} // case form_transfer_waycu_key:
*/
				case form_transfer_openair:
				{
					data.config.SUAformat = SUAOPENAIR;
					handled = true;
					break;
				} // case form_transfer_openair:

				case form_transfer_tnp:
				{
					data.config.SUAformat = SUATNP;
					handled = true;
					break;
				} // case form_transfer__tnp:

				// The Flight button has been pressed
				case form_transfer_flight:
				{	
					form_transfer_hide_fields(true);

					database = form_transfer_flight;
					frm_set_label(form_transfer_lrecords, "  Flights:");
					frm_set_label(form_transfer_linclude, "Include:");

					nflights = OpenDBCountRecords(flight_db);
					StrIToA(tempchar, nflights);
					field_set_value(form_transfer_records, tempchar);
					ctl_set_visible(form_transfer_lrecords, true);
					ctl_set_visible(form_transfer_records, true);

					if (nflights > 0) {
						ctl_set_visible(form_transfer_linclude, true);
						ctl_set_visible(form_transfer_igc_wypts, true);
						ctl_set_visible(form_transfer_xmitbtn, true);
						// This button gives access to single flight delete
						ctl_set_visible(form_transfer_delonebtn, true);
						ctl_set_visible(form_transfer_delbtn, true);
						ctl_set_visible(form_transfer_lflight, true);
						ctl_set_visible(form_transfer_pop1, true);
						ctl_set_visible(form_transfer_lpoints, true);
						ctl_set_visible(form_transfer_points, true);

						npoints = OpenDBCountRecords(logger_db);
						if (selectedFlt != nflights) {
							OpenDBQueryRecord(flight_db, selectedFlt, &flt_hand, &flt_ptr);
							MemMove(fltdata,flt_ptr,sizeof(FlightData));
							MemHandleUnlock(flt_hand);
							npoints = fltdata->stopidx - fltdata->startidx + 1;
							if (fltdata->flttask.numwaypts > 0) {
								ctl_set_visible(form_transfer_linclude, true);
								ctl_set_visible(form_transfer_igc_tasks, true);
							}
						}
						StrIToA(tempchar, npoints);
						field_set_value(form_transfer_points, tempchar);
					}
					handled=true;
					break;
				}  // case form_transfer_flight:

				// The Waypoint button has been pressed
				case form_transfer_waypoint:
				{
					form_transfer_hide_fields(true);
					
					database = form_transfer_waypoint;
					frm_set_label(form_transfer_lrecords, "Waypoints:");
					frm_set_label(form_transfer_linclude, " Format:");
					nwaypts = OpenDBCountRecords(waypoint_db);
					StrIToA(tempchar, nwaypts);
					field_set_value(form_transfer_records, tempchar);
					ctl_set_visible(form_transfer_linclude, true);
					ctl_set_visible(form_transfer_waypop1, true);
					ctl_set_visible(form_transfer_lrecords, true);
					ctl_set_visible(form_transfer_records, true);
					ctl_set_visible(form_transfer_recvbtn, true);
/*
//					ctl_set_visible(form_transfer_waycai, true);
//					ctl_set_visible(form_transfer_waycu, true);
//					ctl_set_visible(form_transfer_waygpwpl, true);
//					ctl_set_value(form_transfer_waycai, data.config.wpformat==WPCAI);
//					ctl_set_value(form_transfer_waycu,  data.config.wpformat==WPCU);
//					ctl_set_value(form_transfer_waygpwpl, data.config.wpformat==WPGPWPL);
//					if (data.config.wpformat==WPGPWPL) {
//						ctl_set_visible(form_transfer_waygpwplalt, true);
//						ctl_set_value(form_transfer_waygpwplalt, data.config.GPWPLalt);
//					}
//					if (data.config.wpformat==WPCU) {
//						ctl_set_visible(form_transfer_waycu_key, true);
//						if (CUkeyoncode) {
//							ctl_set_label(form_transfer_waycu_key, "By:Code");
//						} else {
//							ctl_set_label(form_transfer_waycu_key, "By:Name");
//						}
//					}
*/
					if (nwaypts > 0) {
						ctl_set_visible(form_transfer_xmitbtn, true);
						ctl_set_visible(form_transfer_delbtn, true);
					}
					selectedWaypointIndex = -1;
					handled=true;
					break;
				} // case form_transfer_waypoint:

				// The Polar button has been pressed
				case form_transfer_polar:
				{
					form_transfer_hide_fields(true);

					database = form_transfer_polar;
					ctl_set_visible(form_transfer_lrecords, false);
					frm_set_label(form_transfer_lrecords, "   Polars:");
					npolars = OpenDBCountRecords(polar_db);
					StrIToA(tempchar, npolars);
					field_set_value(form_transfer_records, tempchar);
					ctl_set_visible(form_transfer_lrecords, true);
					ctl_set_visible(form_transfer_records, true);
					ctl_set_visible(form_transfer_recvbtn, true);
					if (npolars > 0)
					{
						ctl_set_visible(form_transfer_xmitbtn, true);
						ctl_set_visible(form_transfer_delbtn, true);
					}
					handled=true;
					break;
				} // case form_transfer_polar:

				// The Task button has been pressed
				case form_transfer_task:
				{
					form_transfer_hide_fields(true);

					database = form_transfer_task;
					ctl_set_visible(form_transfer_lrecords, false);
					frm_set_label(form_transfer_lrecords, "    Tasks:");
					ntasks = OpenDBCountRecords(task_db);
					if (ntasks > 0) ntasks--;
					StrIToA(tempchar, ntasks);
					field_set_value(form_transfer_records, tempchar);
					ctl_set_visible(form_transfer_lrecords, true);
					ctl_set_visible(form_transfer_records, true);
					ctl_set_visible(form_transfer_recvbtn, true);
					if (ntasks > 0) {
						ctl_set_visible(form_transfer_xmitbtn, true);
						ctl_set_visible(form_transfer_delbtn, true);
					}
					handled=true;
					break;
				} // case form_transfer_task:

				// The Config button has been pressed
				case form_transfer_config:
				{
					form_transfer_hide_fields(true);

					database = form_transfer_config;
					ctl_set_visible(form_transfer_recvbtn, true);
					ctl_set_visible(form_transfer_xmitbtn, true);
					handled=true;
					break;
				} // case form_transfer_config:

				// The SUA button has been pressed
				case form_transfer_sua:
				{
					form_transfer_hide_fields(true);

					database = form_transfer_sua;
					frm_set_label(form_transfer_lrecords, "SUA Items:");
					frm_set_label(form_transfer_linclude, " Format:");
					nsua = OpenDBCountRecords(suaidx_db);
					StrIToA(tempchar, nsua);
					field_set_value(form_transfer_records, tempchar);
					npoints = OpenDBCountRecords(suadata_db);
					StrIToA(tempchar, npoints);
					field_set_value(form_transfer_points, tempchar);
					ctl_set_visible(form_transfer_lpoints, true);
					ctl_set_visible(form_transfer_points, true);
					ctl_set_visible(form_transfer_lrecords, true);
					ctl_set_visible(form_transfer_records, true);
					ctl_set_visible(form_transfer_recvbtn, true);
					ctl_set_visible(form_transfer_linclude, true);
					ctl_set_visible(form_transfer_openair, true);
					ctl_set_visible(form_transfer_tnp, true);
					ctl_set_value(form_transfer_openair, data.config.SUAformat==SUAOPENAIR);
					ctl_set_value(form_transfer_tnp, data.config.SUAformat==SUATNP);
					if (nsua > 0) {
						ctl_set_visible(form_transfer_delbtn, true);
					}
					handled=true;
					break;
				} // case form_transfer_sua:

				// The Logger Data button has been pressed
				case form_transfer_loggerdata:
				{
					form_transfer_hide_fields(true);

					database = form_transfer_loggerdata;
					ctl_set_visible(form_transfer_loggerpop1, true);
					ctl_set_visible(form_transfer_loggerlbl, true);
					switch (loggerdatatype)
					{
						case CAI_WAYPOINTDATA:
						case GPN_WAYPOINTDATA:
							ctl_set_visible(form_transfer_tologgerbtn, true);
							ctl_set_visible(form_transfer_delloggerbtn, true);
							break;
						case VLK_WAYPOINTDATA:
						case LX_WAYPOINTDATA:
							ctl_set_visible(form_transfer_tologgerbtn, true);
							break;
						case CAI_GLIDERDATA:
						case VLK_GLIDERDATA:
						case LX_GLIDERDATA:
							ctl_set_visible(form_transfer_tologgerbtn, true);
							break;
						case CAI_PILOTDATA:
						case VLK_PILOTDATA:
						case LX_PILOTDATA:
						case GPN_PILOTDATA:
							ctl_set_visible(form_transfer_tologgerbtn, true);
							break;
						case CAI_FLIGHTDATA:
						case GPN_FLIGHTDATA:
						case VLK_FLIGHTDATA:
							ctl_set_visible(form_transfer_fmloggerbtn, true);
							ctl_set_visible(form_transfer_delloggerbtn, true);
							break;
						case LX_FLIGHTDATA:
						case FLRM_FLIGHTDATA:
							ctl_set_visible(form_transfer_fmloggerbtn, true);
							break;
						default:
							break;
					}
					handled=true;
					break;
				} // case form_transfer_config:

				// The Transmit button has been pressed
				case form_transfer_xmitbtn:
				{
					Err error=NULL;
					xfrdialog = XFRTRANSMIT;

					if ((data.config.xfertype == USEVFS) && !device.CardRW) {
						FrmCustomAlert(WarningAlert, "Cannot write to SD Card"," "," ");
						break;
					}

					// check for VFS or DOC file transfer and no receive file type set
					// trigger file list of required to select file
					if ( ((data.config.xfertype == USEVFS) ||  (data.config.xfertype == USEDOC))
					  && (io_file_type == NO_FILE_TYPE) && (io_type == IO_NONE)
					  && (database != form_transfer_flight)) { 

						io_type = IO_TRANSMIT;
						switch ( database ) {
							case form_transfer_config:
								io_file_type = CONFIG_SCG;
								break;
							case form_transfer_waypoint:
								switch (data.config.wpformat) {
									case WPCAI:
										io_file_type = WAYPOINTS_DAT;
										break;
									case WPCU:
									case WPCUN:
										io_file_type = WAYPOINTS_CUP;
										break;
									case WPGPWPL:
									case WPGPWPLALT:
										io_file_type = WAYPOINTS_WPL;
										break;
									case WPOZI:
										io_file_type = WAYPOINTS_OZI;
										break;
									default:
										break;
								}
								break;
							case form_transfer_task:
								io_file_type = TASKS_SPT;
								break;
							case form_transfer_polar:
								io_file_type = POLARS_SPL;
								break;
							case form_transfer_sua:
								if (data.config.SUAformat == SUAOPENAIR) {
									io_file_type = SUADATA_OPENAIR;
								} else {
									io_file_type = SUADATA_TNP;
								}
								break;
							default:
								io_file_type = NO_FILE_TYPE;
								break;
						}

						// bring up file list to select a file
//						HostTraceOutputTL(appErrorClass, "Get Transmit Filename from List");
						FrmGotoForm(form_list_files);

					} else {

					// check file name length
					if ((StrLen(transfer_filename) < 5) && ((data.config.xfertype == USEVFS) ||  (data.config.xfertype == USEDOC)) && (database != form_transfer_flight)) {
//						HostTraceOutputTL(appErrorClass, "Cancel Transmission - No Filename");
						io_file_type = NO_FILE_TYPE;
						io_type = IO_NONE;
					} else {

					// set default filenames
					if  ((data.config.xfertype != USEVFS) && (data.config.xfertype != USEDOC) && (database != form_transfer_flight)) {
//						HostTraceOutputTL(appErrorClass, "Setting Default Filenames");
						switch ( database ) {
							case form_transfer_config:
								StrCopy(transfer_filename, "config.scg");
								break;
							case form_transfer_waypoint:
								switch (data.config.wpformat) {
									case WPCAI:
										StrCopy(transfer_filename, "waypoints.dat");
										break;
									case WPCU:
									case WPCUN:
										StrCopy(transfer_filename, "waypoints.cup");
										break;
									case WPGPWPL:
									case WPGPWPLALT:
										StrCopy(transfer_filename, "waypoints.wpl");
										break;
									case WPOZI:
										StrCopy(transfer_filename, "waypoints.wpt");
										break;
									default:
										break;
								}
								break;
							case form_transfer_task:
								StrCopy(transfer_filename, "tasks.spt");
								break;
							case form_transfer_polar:
								StrCopy(transfer_filename, "polars.spl");
								break;
							case form_transfer_sua:
								StrCopy(transfer_filename, "suadata.sua");
								break;
							default:
								transfer_filename[0] = 0;
								break;
						}
					}

					// clear receive file type
					io_file_type = NO_FILE_TYPE;
					io_type = IO_NONE;
	
					// handle transmit normally
//					HostTraceOutputTL(appErrorClass, "Transmitting - %s", transfer_filename);
					filename_len = (UInt32)StrStr(transfer_filename, ".") - (UInt32)transfer_filename;
					if (filename_len > 20) filename_len = 20;
					switch (database)
					{
						// Transmit the Flight database
						case form_transfer_flight:
						{
							if (data.config.xfertype == USEMEMO) {
								FrmCustomAlert(WarningAlert, "Cannot Output Flight Info To MemoPad"," "," ");
								break;
							}
							
							XferClose(data.config.nmeaxfertype);
							if (selectedFlt == nflights) {
								// Outputting All Flights
								nrecs = OpenDBCountRecords(logger_db);
								HandleWaitDialogUpdate(SHOWDIALOG, nrecs, 0, NULL);
								HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, 0, "Records");
								for (fltidx = 0; fltidx < nflights; fltidx++) {

									OpenDBQueryRecord(flight_db, fltidx, &output_hand, &output_ptr);
									MemMove(fltdata, output_ptr, sizeof(FlightData));
									error = MemHandleUnlock(output_hand);

									if (data.config.xfertype == USEVFS || data.config.xfertype == USEDOC) {
										// Build a filename to use
										GenerateIGCName(fltdata, tempchar);
										initstat = XferInit(tempchar, IOOPENTRUNC, data.config.xfertype);
									} else if (fltidx == 0) {
										SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
										initstat = XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
										if (data.config.xfertype == USEIR) {
											Sleep(10.0);
										}
									}
									if (initstat) {

										HandleSHA(" ", SHAINIT);
										OutputIGCHeader(fltdata);
										if (data.config.output_wypts) {
											OutputIGCFltLRecs(fltdata);
										}
										if (data.config.output_tasks && (fltdata->flttask.numwaypts > 0)) {
											OutputIGCFltCRecs(fltdata);
										}
	
										OutputIGCFltBRecs(fltdata->startidx, fltdata->stopidx, nrecs);
										HandleSHA(" ", SHAEND);
										// Output an L Record saying the flight is invalid if the
										// power is disconnected from the PDA when configured for a RECO
										if (fltdata->valid == false && fltdata->flightcomp == RECOCOMP) {
											OutputIGCInvalidFltLRec();
										} else {
											HandleSHA(" ", SHAOUTPUT);
										} 
										if (data.config.xfertype == USEVFS || 
											 data.config.xfertype == USEDOC || 
											fltidx == nflights-1) {
											XferClose(data.config.xfertype);
										}
									} else {
										HandleWaitDialogUpdate(STOPDIALOG, nflights, fltidx, NULL);
										FrmCustomAlert(WarningAlert, "Could Not Finish Sending Data"," "," ");
										// Doing this to get out of for loop
										fltidx = nflights;
									}
								}
								if (initstat) {
									HandleWaitDialogUpdate(STOPDIALOG, nflights, fltidx, NULL);
									FrmCustomAlert(FinishedAlert, "Finished Sending Data"," "," ");
								}
							} else if ((selectedFlt != -1) && (selectedFlt < nflights)) {
								OutputSingleFlight(selectedFlt, true);
							}
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
							break;
						} // case form_transfer_flight:

						// Transmit the Waypoint database
						case form_transfer_waypoint:
						{
							XferClose(data.config.nmeaxfertype);
							if (data.config.xfertype == USEVFS || 
							    data.config.xfertype == USEDOC ||
							    data.config.xfertype == USEMEMO) {
								initstat = false;
								switch (data.config.wpformat) {
								case WPCAI:
									initstat = XferInit(transfer_filename, IOOPENTRUNC, data.config.xfertype);
									break;
								case WPCU:
								case WPCUN:
									initstat = XferInit(transfer_filename, IOOPENTRUNC, data.config.xfertype);
									break;
								case WPGPWPL:
								case WPGPWPLALT:
									initstat = XferInit(transfer_filename, IOOPENTRUNC, data.config.xfertype);
									break;
								case WPOZI:
									initstat = XferInit(transfer_filename, IOOPENTRUNC, data.config.xfertype);
									break;
								default:
									break;								
								}
							} else {
								SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
								initstat = XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
							}
							if (initstat) {
								DeleteThermalWaypts();
								StrCopy(data.config.waypt_file, Left(transfer_filename, filename_len));
								//HandleWaitDialog(true);
								nrecs = OpenDBCountRecords(waypoint_db);
								HandleWaitDialogUpdate(SHOWDIALOG, nrecs, 0, NULL);
								HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, 0, "Waypoints");
								if (data.config.xfertype == USEIR) {
									Sleep(10.0);
								}
								OutputWayHeader(data.config.wpformat);
								switch (data.config.wpformat) {
								case WPCAI:
									OutputWayCAI();
									break;
								case WPCU:
								case WPCUN:
									OutputWayCU();
									break;
								case WPGPWPL:
								case WPGPWPLALT:
									OutputWayGPWPL();
									break;
								case WPOZI:
									OutputWayOZI();
									break;
								default:
									break;
								}
								//HandleWaitDialog(false);
								HandleWaitDialogUpdate(STOPDIALOG, nrecs, nrecs, NULL);
								FrmCustomAlert(FinishedAlert, "Finished Sending Data"," "," ");
								XferClose(data.config.xfertype);
							}
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
							break;
						} // case form_transfer_waypoint:

						// Transmit the Polar database
						case form_transfer_polar:
						{
							XferClose(data.config.nmeaxfertype);
							if (data.config.xfertype == USEVFS || 
							    data.config.xfertype == USEDOC || 
							    data.config.xfertype == USEMEMO) {
								initstat = XferInit(transfer_filename, IOOPENTRUNC, data.config.xfertype);
							} else {
								SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
								initstat = XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
							}
							if (initstat) {
								//HandleWaitDialog(true);
								nrecs = OpenDBCountRecords(polar_db);
								HandleWaitDialogUpdate(SHOWDIALOG, nrecs, 0, NULL);
								HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, 0, "Polars");
								if (data.config.xfertype == USEIR) {
									Sleep(10.0);
								}
								OutputPolarHeader();
								OutputPolarRecords();
								//HandleWaitDialog(false);
								HandleWaitDialogUpdate(STOPDIALOG, nrecs, nrecs, NULL);
								FrmCustomAlert(FinishedAlert, "Finished Sending Data"," "," ");
								XferClose(data.config.xfertype);
							}
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
 							break;
						} // case form_transfer_polar:

						// Transmit the Task database
						case form_transfer_task:
						{
							XferClose(data.config.nmeaxfertype);
							if (data.config.xfertype == USEVFS || 
							    data.config.xfertype == USEDOC ||
							    data.config.xfertype == USEMEMO) {
								initstat = XferInit(transfer_filename, IOOPENTRUNC, data.config.xfertype);
							} else {
								SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
								initstat = XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
							}
							if (initstat) {
								//HandleWaitDialog(true);
								nrecs = OpenDBCountRecords(task_db)-1;
								HandleWaitDialogUpdate(SHOWDIALOG, nrecs, 0, NULL);
								HandleWaitDialogUpdate(UPDATEDIALOG, nrecs, 0, "Tasks");
								if (data.config.xfertype == USEIR) {
									Sleep(10.0);
								}
								OutputTasks();
								//HandleWaitDialog(false);
								HandleWaitDialogUpdate(STOPDIALOG, nrecs, nrecs, NULL);
								FrmCustomAlert(FinishedAlert, "Finished Sending Data"," "," ");
								XferClose(data.config.xfertype);
							}
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
 							break;
						} // case form_transfer_task:

						// Transmit the Config database
						case form_transfer_config:
						{
							XferClose(data.config.nmeaxfertype);
							if (data.config.xfertype == USEVFS || 
							    data.config.xfertype == USEDOC ||
							    data.config.xfertype == USEMEMO) {
								initstat = XferInit(transfer_filename, IOOPENTRUNC, data.config.xfertype);
							} else {
								SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
								initstat = XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
							}
							if (initstat) {
								StrCopy(data.config.config_file, Left(transfer_filename, filename_len));
								HandleWaitDialog(true);
								if (data.config.xfertype == USEIR) {
									Sleep(10.0);
								}
								OutputConfig();
								HandleWaitDialog(false);
								FrmCustomAlert(FinishedAlert, "Finished Sending Data"," "," ");
								XferClose(data.config.xfertype);
							}
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
							break;
						} 
						default:
							break;
					} // switch ( database )
//					HostTraceOutputTL(appErrorClass, "Transmitting Complete");

					} // else part of if (((data.config.xfertype == USEVFS) ||  (data.config.xfertype == USEDOC)) && (io_file_type == NO_FILE_TYPE) && (io_type == NO_NONE)) {
					} // else part of if (StrLen(transfer_filename) < 5) {

					handled=true;
					break;
				} // case form_transfer_xmitbtn:

				// The Receive button has been pressed
				case form_transfer_recvbtn:
				{
					xfrdialog = XFRRECEIVE;

					// check for VFS or DOC file transfer and no receive file type set
					// trigger file list of required to select file
					if ( ((data.config.xfertype == USEVFS) ||  (data.config.xfertype == USEDOC))
					  && (io_file_type == NO_FILE_TYPE) && (io_type == IO_NONE)) {

						io_type = IO_RECEIVE;
						switch ( database ) {
							case form_transfer_config:
								io_file_type = CONFIG_SCG;
								break;
							case form_transfer_waypoint:
								switch (data.config.wpformat) {
									case WPCAI:
										io_file_type = WAYPOINTS_DAT;
										break;
									case WPCU:
									case WPCUN:
										io_file_type = WAYPOINTS_CUP;
										break;
									case WPGPWPL:
									case WPGPWPLALT:
										io_file_type = WAYPOINTS_WPL;
										break;
									case WPOZI:
										io_file_type = WAYPOINTS_OZI;
										break;
									default:
										break;
								}
								break;
							case form_transfer_task:
								io_file_type = TASKS_SPT;
								break;
							case form_transfer_polar:
								io_file_type = POLARS_SPL;
								break;
							case form_transfer_sua:
								if (data.config.SUAformat == SUAOPENAIR) {
									io_file_type = SUADATA_OPENAIR;
								} else {
									io_file_type = SUADATA_TNP;
								}
								break;
							default:
								io_file_type = NO_FILE_TYPE;
								break;
						}

						// bring up file list to select a file
//						HostTraceOutputTL(appErrorClass, "Get Receive Filename from List");
						FrmGotoForm(form_list_files);

					} else {

					// check file name length
					if ((StrLen(transfer_filename) < 5) && ((data.config.xfertype == USEVFS) ||  (data.config.xfertype == USEDOC))) {
//						HostTraceOutputTL(appErrorClass, "Cancel Receiving - No Filename");
						io_file_type = NO_FILE_TYPE;
						io_type = IO_NONE;
					} else {
						// set default filenames
					if ((data.config.xfertype != USEVFS) && (data.config.xfertype != USEDOC)) {
//						HostTraceOutputTL(appErrorClass, "Setting Default Filenames");
						switch ( database ) {
							case form_transfer_config:
								StrCopy(transfer_filename, "config.scg");
								break;
							case form_transfer_waypoint:
								switch (data.config.wpformat) {
									case WPCAI:
										StrCopy(transfer_filename, "waypoints.dat");
										break;
									case WPCU:
									case WPCUN:
										StrCopy(transfer_filename, "waypoints.cup");
										break;
									case WPGPWPL:
									case WPGPWPLALT:
										StrCopy(transfer_filename, "waypoints.wpl");
										break;
									case WPOZI:
										StrCopy(transfer_filename, "waypoints.wpt");
										break;
									default:
										break;
								}
								break;
							case form_transfer_task:
								StrCopy(transfer_filename, "tasks.spt");
								break;
							case form_transfer_polar:
								StrCopy(transfer_filename, "polars.spl");
								break;
							case form_transfer_sua:
								if (data.config.SUAformat == SUAOPENAIR) {
									StrCopy(transfer_filename, "suadata.air");
								} else {
									StrCopy(transfer_filename, "suadata.sua");
								}
								break;
							default:
								transfer_filename[0] = 0;
								break;
						}
					}

					// clear receive file type
					io_file_type = NO_FILE_TYPE;
					io_type = IO_NONE;
	
					// handle receive normally
//					HostTraceOutputTL(appErrorClass, "Receiving File - %s", transfer_filename);
					filename_len = (UInt32)StrStr(transfer_filename, ".") - (UInt32)transfer_filename;
					if (filename_len > 20) filename_len = 20;
					switch ( database ) {

						// Receive the Waypoint database
						case form_transfer_waypoint:
						{
							if (!waytoggle) {
								XferClose(data.config.nmeaxfertype);
								switch (data.config.wpformat) {
								case WPCAI:
									waypoint_parser(NULL, 0, true);
									data.parser.parser_func = waypoint_parser;
									break;
								case WPCU:
								case WPCUN:
									cuwaypoint_parser(NULL, 0, true);
									data.parser.parser_func = cuwaypoint_parser;
									break;
								case WPGPWPL:
								case WPGPWPLALT:
									GPWPLwaypoint_parser(NULL, 0, true);
									data.parser.parser_func = GPWPLwaypoint_parser;
									break;
								case WPOZI:
									OZIwaypoint_parser(NULL, 0, true);
									data.parser.parser_func = OZIwaypoint_parser;
									break;
								default:
									break;
								}
								ctl_set_enable(form_transfer_xmitbtn, false);
								ctl_set_enable(form_transfer_delbtn, false);
								ctl_set_label(form_transfer_recvbtn, "Stop");
								if (data.config.xfertype == USEVFS || 
									 data.config.xfertype == USEDOC ||
									 data.config.xfertype == USEMEMO) {
//									HostTraceOutputTL(appErrorClass, "Openning waypoint file");
									if(XferInit(transfer_filename, IOOPEN, data.config.xfertype)) {
										StrCopy(data.config.waypt_file, Left(transfer_filename, filename_len));
										//HandleWaitDialog(true);
										HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, NULL);
										HandleWaitDialogUpdate(UPDATEDIALOG, 0, -1, "Waypoints");
										RxData(data.config.xfertype);
										//HandleWaitDialog(false);
										HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
										FrmCustomAlert(FinishedAlert, "Finished Receiving Data"," "," ");
									} else {
										FrmCustomAlert(WarningAlert, "Data Not Found"," "," ");
									}
									waytoggle = true;
								} else {
									SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
									XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
									waytoggle = false;
								}
							}
							if (waytoggle) {
//								HostTraceOutputTL(appErrorClass, "Closing waypoint file");
								XferClose(data.config.xfertype);
								SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
								XferInit(tempchar, NFC, data.config.nmeaxfertype);
								nmea_parser(NULL, 0, true);
								data.parser.parser_func = nmea_parser;
								ctl_set_enable(form_transfer_xmitbtn, true);
								ctl_set_enable(form_transfer_delbtn, true);
								ctl_set_label(form_transfer_recvbtn, "Receive");
								nwaypts = OpenDBCountRecords(waypoint_db);
								StrIToA(tempchar, nwaypts);
								field_set_value(form_transfer_records, tempchar);
								if (nwaypts > 0) {
									ctl_set_visible(form_transfer_delbtn, true);
									ctl_set_visible(form_transfer_xmitbtn, true);
								} else {
									ctl_set_visible(form_transfer_delbtn, false);
									ctl_set_visible(form_transfer_xmitbtn, false);
								}
								waytoggle = false;
							} else {
								waytoggle = true;
							}
							if (FindHomeWayptInitialize()) {
								updatemap = true;
							}
							break;
						} // case form_transfer_waypoint:

						// Receive the Polar database
						case form_transfer_polar:
						{
							if (!polartoggle) {
								XferClose(data.config.nmeaxfertype);
								polar_parser(NULL, 0, true);
								data.parser.parser_func = polar_parser;
								ctl_set_enable(form_transfer_xmitbtn, false);
								ctl_set_enable(form_transfer_delbtn, false);
								ctl_set_label(form_transfer_recvbtn, "Stop");
								if (data.config.xfertype == USEVFS || 
									 data.config.xfertype == USEDOC ||
									 data.config.xfertype == USEMEMO) {
//									HostTraceOutputTL(appErrorClass, "Opening polar file");
									if(XferInit(transfer_filename, IOOPEN, data.config.xfertype)) { 
										//HandleWaitDialog(true);
										HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, NULL);
										HandleWaitDialogUpdate(UPDATEDIALOG, 0, -1, "Polars");
										RxData(data.config.xfertype);
										//HandleWaitDialog(false);
										HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
										FrmCustomAlert(FinishedAlert, "Finished Receiving Data"," "," ");
									} else {
										FrmCustomAlert(WarningAlert, "Data Not Found"," "," ");
									}
									polartoggle = true;
								} else {
									SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
									XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
									polartoggle = false;
								}
							}
							if (polartoggle) {
//								HostTraceOutputTL(appErrorClass, "Closing polar file");
								XferClose(data.config.xfertype);
								SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
								XferInit(tempchar, NFC, data.config.nmeaxfertype);
								nmea_parser(NULL, 0, true);
								data.parser.parser_func = nmea_parser;
								ctl_set_enable(form_transfer_xmitbtn, true);
								ctl_set_enable(form_transfer_delbtn, true);
								ctl_set_label(form_transfer_recvbtn, "Receive");
								npolars = OpenDBCountRecords(polar_db);
								StrIToA(tempchar, npolars);
								field_set_value(form_transfer_records, tempchar);
								if (npolars > 0) {
									ctl_set_visible(form_transfer_delbtn, true);
									ctl_set_visible(form_transfer_xmitbtn, true);
								} else {
									ctl_set_visible(form_transfer_delbtn, false);
									ctl_set_visible(form_transfer_xmitbtn, false);
								}
								polartoggle = false;
							} else {
								polartoggle = true;
							}
							break;
						} // case form_transfer_polar:

						// Receive the Task database
						case form_transfer_task:
						{
							if (!tasktoggle) {
								XferClose(data.config.nmeaxfertype);
								task_parser(NULL, 0, true);
								data.parser.parser_func = task_parser;
								ctl_set_enable(form_transfer_xmitbtn, false);
								ctl_set_enable(form_transfer_delbtn, false);
								ctl_set_label(form_transfer_recvbtn, "Stop");
								if (data.config.xfertype == USEVFS || 
									 data.config.xfertype == USEDOC ||
									 data.config.xfertype == USEMEMO) {
//									HostTraceOutputTL(appErrorClass, "Opening task file");
									if(XferInit(transfer_filename, IOOPEN, data.config.xfertype)) {
										//HandleWaitDialog(true);
										HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, NULL);
										HandleWaitDialogUpdate(UPDATEDIALOG, 0, -1, "Tasks");
										RxData(data.config.xfertype);
										//HandleWaitDialog(false);
										HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);
										FrmCustomAlert(FinishedAlert, "Finished Receiving Data"," "," ");
									} else {
										FrmCustomAlert(WarningAlert, "Data Not Found"," "," ");
									}
									tasktoggle = true;
								} else {
									SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
									XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
									tasktoggle = false;
								}
							}
							if (tasktoggle) {
//								HostTraceOutputTL(appErrorClass, "Closing task file");
								XferClose(data.config.xfertype);
								SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
								XferInit(tempchar, NFC, data.config.nmeaxfertype);
								nmea_parser(NULL, 0, true);
								data.parser.parser_func = nmea_parser;
								ctl_set_enable(form_transfer_xmitbtn, true);
								ctl_set_enable(form_transfer_delbtn, true);
								ctl_set_label(form_transfer_recvbtn, "Receive");
								ntasks = OpenDBCountRecords(task_db)-1;
								StrIToA(tempchar, ntasks);
								field_set_value(form_transfer_records, tempchar);
								if (ntasks > 0) {
									ctl_set_visible(form_transfer_delbtn, true);
									ctl_set_visible(form_transfer_xmitbtn, true);
								} else {
									ctl_set_visible(form_transfer_delbtn, false);
									ctl_set_visible(form_transfer_xmitbtn, false);
								}
								tasktoggle = false;
							} else {
								tasktoggle = true;
							}
							break;
						} // case form_transfer_task:

						// Receive the Config database
						case form_transfer_config:
						{
							if (!configtoggle) {
								XferClose(data.config.nmeaxfertype);
								config_parser(NULL, 0, true);
								data.parser.parser_func = config_parser;
								ctl_set_enable(form_transfer_xmitbtn, false);
								ctl_set_label(form_transfer_recvbtn, "Stop");
								if (data.config.xfertype == USEVFS || 
									 data.config.xfertype == USEDOC ||
									 data.config.xfertype == USEMEMO) {
//									HostTraceOutputTL(appErrorClass, "Openning config file");
									if (XferInit(transfer_filename, IOOPEN, data.config.xfertype)) {
										StrCopy(data.config.config_file, Left(transfer_filename, filename_len));
										HandleWaitDialog(true);
										RxData(data.config.xfertype);
										HandleWaitDialog(false);
										FrmCustomAlert(FinishedAlert, "Finished Receiving Data"," "," ");
									} else {
										FrmCustomAlert(WarningAlert, "Data Not Found"," "," ");
									}
									configtoggle = true;
								} else {
									SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
									XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
									configtoggle = false;
								}
							}
							if (configtoggle) {
//								HostTraceOutputTL(appErrorClass, "Closing config file");
								XferClose(data.config.xfertype);
								SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
								XferInit(tempchar, NFC, data.config.nmeaxfertype);
								nmea_parser(NULL, 0, true);
								data.parser.parser_func = nmea_parser;
								ctl_set_enable(form_transfer_xmitbtn, true);
								ctl_set_enable(form_transfer_delbtn, true);
								ctl_set_label(form_transfer_recvbtn, "Receive");
								configtoggle = false;

								// check for change in flight computer
								if (data.config.flightcomp == C302COMP || data.config.flightcomp == C302ACOMP) {
								ctl_set_label(form_transfer_loggerdata, "C302 Data");
									ctl_set_visible(form_transfer_loggerdata, true);
									loggerdatatype = CAI_FLIGHTDATA;
									ctl_set_enable(form_transfer_loggerpop1, true); // ensure we can pick from the list
								} else if ((data.config.flightcomp == VOLKSCOMP) || (data.config.flightcomp == B50VLKCOMP)) {
									ctl_set_label(form_transfer_loggerdata, "Volks Data");
									ctl_set_visible(form_transfer_loggerdata, true);
									loggerdatatype = VLK_FLIGHTDATA;
									ctl_set_enable(form_transfer_loggerpop1, false); // disable list as only flight transfer supported
								} else if ((data.config.flightcomp == LXCOMP) || (data.config.flightcomp == B50LXCOMP)) {
									ctl_set_label(form_transfer_loggerdata, "LX Data");
									ctl_set_visible(form_transfer_loggerdata, true);
									loggerdatatype = LX_FLIGHTDATA;
									ctl_set_enable(form_transfer_loggerpop1, false); // disable list as only flight transfer supported
								} else if (data.config.flightcomp == FILSERCOMP) {
									ctl_set_label(form_transfer_loggerdata, "Filser Data");
									ctl_set_visible(form_transfer_loggerdata, true);
									loggerdatatype = LX_FLIGHTDATA;
									ctl_set_enable(form_transfer_loggerpop1, false); // disable list as only flight transfer supported
								} else if (data.config.flightcomp == GPSNAVCOMP) {
									ctl_set_label(form_transfer_loggerdata, "GPSNAV Data");
									ctl_set_visible(form_transfer_loggerdata, true);
									loggerdatatype = GPN_FLIGHTDATA;
									ctl_set_enable(form_transfer_loggerpop1, true); // ensure we can pick from the list
								} else if (data.config.flightcomp == FLARMCOMP) {
									ctl_set_label(form_transfer_loggerdata, "Flarm Data");
									ctl_set_visible(form_transfer_loggerdata, true);
									loggerdatatype = FLRM_FLIGHTDATA;
									ctl_set_enable(form_transfer_loggerpop1, false); // disable list as only flight transfer supported
								} else {
									ctl_set_visible(form_transfer_loggerdata, false);
								}
								// redraw lower 2 buttons to fix missing part of button border
								ctl_set_visible(form_transfer_config, true);
								ctl_set_visible(form_transfer_sua, true);
							} else {
								configtoggle = true;
							}
							break;
						} // case form_transfer_config:

						// Receive the SUA data
						case form_transfer_sua:
						{
							if (!suatoggle) {
								XferClose(data.config.nmeaxfertype);
								if (data.config.SUAformat == SUAOPENAIR) {
									SUA_parser_openair(NULL, 0, true);
									data.parser.parser_func = SUA_parser_openair;
								} else {
									SUA_parser_tnp(NULL, 0, true);
									data.parser.parser_func = SUA_parser_tnp;
								}
								ctl_set_enable(form_transfer_xmitbtn, false);
								ctl_set_enable(form_transfer_delbtn, false);
								ctl_set_label(form_transfer_recvbtn, "Stop");
								if (data.config.xfertype == USEVFS || 
									 data.config.xfertype == USEDOC ||
									 data.config.xfertype == USEMEMO) {
									if (XferInit(transfer_filename, IOOPEN, data.config.xfertype)) {
										StrCopy(data.config.SUA_file, Left(transfer_filename, filename_len));
										//HandleWaitDialog(true);
										HandleWaitDialogUpdate(SHOWDIALOG, 0, 0, NULL);
										HandleWaitDialogUpdate(UPDATEDIALOG, 0, -1, "SUA Items");
										RxData(data.config.xfertype);
										// process the SUA database after parsing
										ProcessSUA(true,false);
										//HandleWaitDialog(false);
										HandleWaitDialogUpdate(STOPDIALOG, 0, 0, NULL);

										FrmCustomAlert(FinishedAlert, "Finished Receiving Data"," "," ");
									} else {
										FrmCustomAlert(WarningAlert, "Data Not Found"," "," ");
									}
									suatoggle = true;
								} else {
									SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
									XferInit(tempchar, data.config.flowctrl, data.config.xfertype);
									suatoggle = false;
								}
							}
							if (suatoggle) {
								XferClose(data.config.xfertype);
								SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
								XferInit(tempchar, NFC, data.config.nmeaxfertype);
								nmea_parser(NULL, 0, true);
								data.parser.parser_func = nmea_parser;
								ctl_set_enable(form_transfer_xmitbtn, true);
								ctl_set_enable(form_transfer_delbtn, true);
								ctl_set_label(form_transfer_recvbtn, "Receive");
								nsua = OpenDBCountRecords(suaidx_db);
								StrIToA(tempchar, nsua);
								field_set_value(form_transfer_records, tempchar);
								npoints = OpenDBCountRecords(suadata_db);
								StrIToA(tempchar, npoints);
								field_set_value(form_transfer_points, tempchar);
								if (npoints > 0) {
									ctl_set_visible(form_transfer_delbtn, true);
								} else {
									ctl_set_visible(form_transfer_delbtn, false);
								}
								suatoggle = false;
							} else {
								suatoggle = true;
							}
							break;
						} // case form_transfer_sua:

					} // switch ( database )
//					HostTraceOutputTL(appErrorClass, "Receiving Complete");

					} // else part of if (((data.config.xfertype == USEVFS) ||  (data.config.xfertype == USEDOC)) && (io_file_type == NO_FILE_TYPE) && (io_type == NO_NONE)) {
					} // else part of if (StrLen(transfer_filename) < 5) {

					handled=true;
					break;
				} // case form_transfer_recvbtn:

				// The Delete One button has been pressed
				case form_transfer_delonebtn:
				{
					xfrdialog = XFRDELETE;
					if (selectedFlt >= nflights) {
						// same as Delete All button
						AlertResp = FrmCustomAlert(ConfirmAlert, "Delete All Flights?"," "," ");
						if (AlertResp == OK) {
							HandleWaitDialog(true);
							OpenDBEmpty(flight_db);
							OpenDBEmpty(logger_db);
							HandleWaitDialog(false);
							FrmCustomAlert(FinishedAlert, "Finished Deleting Data"," "," ");	

							if (nflights > 0 && items != NULL) {
								for (x = 0; x < (nflights+1); x++) {
									MemPtrFree(items[x]);
								}
								MemPtrFree(items);
								items = NULL;
							}
							nflights = 0;
							selectedFlt = -1;
							StrIToA(tempchar, OpenDBCountRecords(flight_db));
							field_set_value(form_transfer_records, tempchar);
							form_transfer_hide_fields(false);
						}
						break;
					} else {
						AlertResp = FrmCustomAlert(ConfirmAlert, "Delete Flight?\n",items[nflights - selectedFlt - 1]," ");
						PlayKeySound();
						if (AlertResp == OK) {
							// Delete a single flight from the flight database
							HandleWaitDialog(true);

							// get the selected flight record
							OpenDBQueryRecord(flight_db, selectedFlt, &flt_hand, &flt_ptr);
							MemMove(fltdata,flt_ptr,sizeof(FlightData));
							MemHandleUnlock(flt_hand);
//							HostTraceOutputTL(appErrorClass, "-----------------");	
//							HostTraceOutputTL(appErrorClass, "Flight Rec: %s",DblToStr(selectedFlt,0));

							// delete log database records
							for (x = fltdata->startidx; x <= fltdata->stopidx; x++) {
//							HostTraceOutputTL(appErrorClass, "Del Log Rec: %s",DblToStr(x,0));
								OpenDBDeleteRecord(logger_db, fltdata->startidx);
							}
//							HostTraceOutputTL(appErrorClass, "Log Recs left: %s",DblToStr(OpenDBCountRecords(logger_db),0));
							n = fltdata->stopidx - fltdata->startidx + 1;
//							HostTraceOutputTL(appErrorClass, "n : %s",DblToStr(n,0));	
//							HostTraceOutputTL(appErrorClass, "-----------------");	
							
							// adjust start / stop indices to logger_db in remaining flight records
							if (selectedFlt < nflights) {
								for (x = selectedFlt+1; x < nflights; x++) {
									OpenDBQueryRecord(flight_db, x, &flt_hand, &flt_ptr);
									MemMove(fltdata,flt_ptr,sizeof(FlightData));
									MemHandleUnlock(flt_hand);
//									HostTraceOutputTL(appErrorClass, "Flight Chg: %s",DblToStr(x,0));
//									HostTraceOutputTL(appErrorClass, "Flight start1: %s",DblToStr(fltdata->startidx,0));
									fltdata->startidx = fltdata->startidx - n;
									fltdata->stopidx = fltdata->stopidx - n;
//									HostTraceOutputTL(appErrorClass, "Flight start2: %s",DblToStr(fltdata->startidx,0));
									OpenDBUpdateRecord(flight_db, sizeof(FlightData), fltdata, x);
								}
//								HostTraceOutputTL(appErrorClass, "-----------------");	
							}

							HandleWaitDialog(false);
							FrmCustomAlert(FinishedAlert, "Finished Deleting Data"," "," ");

							// hide popup list
							ctl_set_visible(form_transfer_pop1, false);

							// free the items in drop down list
							if (nflights > 0 && items != NULL) {
								for (x = 0; x < (nflights+1); x++) {
									MemPtrFree(items[x]);
								}
								MemPtrFree(items);
								items = NULL;
							}

							// delete the selected flight record
//							HostTraceOutputTL(appErrorClass, "Flight Del: %s",DblToStr(selectedFlt,0));
							OpenDBDeleteRecord(flight_db, selectedFlt);

							// update number of flights in database
							nflights = OpenDBCountRecords(flight_db);
							StrIToA(tempchar, nflights);
							field_set_value(form_transfer_records, tempchar);

							// rebuild drop down list of flights
							selectedFlt = -1;
							if (nflights > 0) {
								// flights still left in database
								items = (char **) MemPtrNew((nflights+1) * (sizeof(char *)));
								for (x = 0; x < nflights; x++) {
									items[x] = (char *) MemPtrNew(40 * (sizeof(char)));
									OpenDBQueryRecord(flight_db, (nflights - 1 - x), &flt_hand, &flt_ptr);
									MemMove(fltdata, flt_ptr, sizeof(FlightData));
									MemHandleUnlock(flt_hand);
									StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &strtdt);
									if (data.config.timezone != 0) {
										TimAdjust(&strtdt, ((Int32)data.config.timezone * 3600 ));
									}
									// Build the Date
									DateToAscii(strtdt.month, strtdt.day, strtdt.year, (DateFormatType)PrefGetPreference(prefDateFormat), tempchar2);
									StrCopy(items[x], tempchar2);
									// Add a dash
									StrCat(items[x], "-");
									// Add the Time
									StrIToA(tempchar, (UInt32)strtdt.hour);
									StrCopy(tempchar2, leftpad(tempchar, '0', 2));
									StrCat(tempchar2, ":");
									StrIToA(tempchar, (UInt32)strtdt.minute);
									StrCat(tempchar2, leftpad(tempchar, '0', 2));
									StrCat(items[x], tempchar2);
								}
								items[nflights] = (char *) MemPtrNew(40 * (sizeof(char)));
								StrCopy(items[nflights], "All");
	
								LstSetListChoices(lst, items, (nflights+1));
								if ((nflights+1) < 5) {
									LstSetHeight(lst, (nflights+1));
								} else {
									LstSetHeight(lst, 5);
								}
								LstSetTopItem (lst, 0);
								selectedFlt = (nflights-1);
								CtlSetLabel (ctl, "Current");

								// make new list visible
								ctl_set_visible(form_transfer_pop1, true);

								// update points count
								if (selectedFlt != nflights) {
									// single flight 
									OpenDBQueryRecord(flight_db, selectedFlt, &flt_hand, &flt_ptr);
									MemMove(fltdata,flt_ptr,sizeof(FlightData));
									MemHandleUnlock(flt_hand);
									npoints = fltdata->stopidx - fltdata->startidx + 1;
									if (fltdata->flttask.numwaypts > 0) {
										ctl_set_visible(form_transfer_linclude, true);
										ctl_set_visible(form_transfer_igc_tasks, true);
									}
								} else {
									npoints = OpenDBCountRecords(logger_db);
								}
								StrIToA(tempchar, npoints);
								field_set_value(form_transfer_points, tempchar);
							} else {
								// no flights left in database, so hide all fields
								form_transfer_hide_fields(false);
							}
						}  // if (AlertResp == OK)
					} // if (selectedFlt == nflights)
					handled=true;
					break;
				} // case form_transfer_delonebtn:

				// The Delete button has been pressed
				case form_transfer_delbtn:
				{
					xfrdialog = XFRDELETE;
					switch ( database )
					{
						// Delete the flight database
						case form_transfer_flight:
						{
							AlertResp = FrmCustomAlert(ConfirmAlert, "Delete All Flights?"," "," ");
							if (AlertResp == OK) {
								HandleWaitDialog(true);
								OpenDBEmpty(flight_db);
								OpenDBEmpty(logger_db);
								HandleWaitDialog(false);
								FrmCustomAlert(FinishedAlert, "Finished Deleting Data"," "," ");

								if (nflights > 0 && items != NULL) {
									for (x = 0; x < (nflights+1); x++) {
										MemPtrFree(items[x]);
									}
									MemPtrFree(items);
									items = NULL;
								}
								nflights = 0;
								selectedFlt = -1;
								StrIToA(tempchar, OpenDBCountRecords(flight_db));
								field_set_value(form_transfer_records, tempchar);
								form_transfer_hide_fields(false);							}
							break;
						} // case form_transfer_flight:

						// Delete the waypoint database
						case form_transfer_waypoint:
						{
							AlertResp = FrmCustomAlert(ConfirmAlert, "Delete All Waypoints?"," "," ");
							if (AlertResp == OK) {
								HandleWaitDialog(true);
								OpenDBEmpty(waypoint_db);
								HandleWaitDialog(false);
								FrmCustomAlert(FinishedAlert, "Finished Deleting Data"," "," ");

								StrIToA(tempchar, OpenDBCountRecords(waypoint_db));
								field_set_value(form_transfer_records, tempchar);
								selectedWaypointIndex = -1;
								if (data.config.usepalmway) {
									data.input.distance_to_destination.valid = NOT_VALID;
									data.input.distance_to_destination.value = 0.0;
									data.input.bearing_to_destination.valid = NOT_VALID;
									data.input.bearing_to_destination.value = 000.0;
									data.input.destination_name[0] = 0;
									data.input.destination_elev = 0.0;
									data.input.destination_valid = false;
								}
								ctl_set_visible(form_transfer_xmitbtn, false);
								ctl_set_visible(form_transfer_delbtn, false);
								StrCopy(data.config.waypt_file, "None");
							}
							break;
						} // case form_transfer_waypoint:

						// Delete the polar database
						case form_transfer_polar:
						{
							AlertResp = FrmCustomAlert(ConfirmAlert, "Delete All Polars?"," "," ");
							if (AlertResp == OK) {
								HandleWaitDialog(true);
								OpenDBEmpty(polar_db);
								HandleWaitDialog(false);
								FrmCustomAlert(FinishedAlert, "Finished Deleting Data"," "," ");

								StrIToA(tempchar, OpenDBCountRecords(polar_db));
								field_set_value(form_transfer_records, tempchar);
								selectedPolarIndex = -1;
								MemSet(selectedPolar, sizeof(PolarData), 0);
								MemSet(&data.polar, sizeof(PolarData), 0);
								MemSet(inusePolar, sizeof(PolarData), 0);
								ctl_set_visible(form_transfer_xmitbtn, false);
								ctl_set_visible(form_transfer_delbtn, false);
							}
							break;
						} // case form_transfer_polar:

						// Delete the task database
						case form_transfer_task:
						{
							AlertResp = FrmCustomAlert(ConfirmAlert, "Delete All Tasks?"," "," ");
							if (AlertResp == OK) {
								HandleWaitDialog(true);
								refresh_task_details(TASKDELALL);
								HandleWaitDialog(false);
								FrmCustomAlert(FinishedAlert, "Finished Deleting Data"," "," ");

								ntasks = OpenDBCountRecords(task_db);
								if (ntasks > 0) ntasks--;
								StrIToA(tempchar, ntasks);
								field_set_value(form_transfer_records, tempchar);
								MemSet(&data.task, sizeof(TaskData), 0);
								ctl_set_visible(form_transfer_xmitbtn, false);
								ctl_set_visible(form_transfer_delbtn, false);
							}
							break;
						} // case form_transfer_task:

						// Delete the SUA databases
						case form_transfer_sua:
						{
							AlertResp = FrmCustomAlert(ConfirmAlert, "Delete All SUA items?"," "," ");
							if (AlertResp == OK) {
								HandleWaitDialog(true);
								OpenDBEmpty(suaidx_db);
								OpenDBEmpty(suadata_db);
								HandleWaitDialog(false);
								FrmCustomAlert(FinishedAlert, "Finished Deleting Data"," "," ");

								StrIToA(tempchar, OpenDBCountRecords(suaidx_db));
								field_set_value(form_transfer_records, tempchar);
								StrIToA(tempchar, OpenDBCountRecords(suadata_db));
								field_set_value(form_transfer_points, tempchar);
								ctl_set_visible(form_transfer_delbtn, false);
								StrCopy(data.config.SUA_file, "None");
								// process the SUA database after parsing
								// Use this to reset SUAnumrecs and SUAwarnrecs
								ProcessSUA(true,false);
							}
							break;
						} // case form_transfer_sua:
					}  // switch(database)
					handled=true;
					break;
				} // case form_transfer_delbtn:

// Support for communication with flight computers
				case form_transfer_tologgerbtn:
				{
					switch (loggerdatatype)
					{
						case CAI_WAYPOINTDATA:
							XferClose(data.config.nmeaxfertype);
							XferInit("4800", NFC, USESER);
							if (!(UploadCAIWaypoints(true))) {
								//Display an Error Dialog
								FrmCustomAlert(WarningAlert, "Sending Waypoints to C302/CFR Failed"," "," ");
							}
							XferClose(USESER);
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
							break;
						case GPN_WAYPOINTDATA:
							XferClose(data.config.nmeaxfertype);
							XferInit("4800", NFC, USESER);
							if (!(UploadCAIWaypoints(true))) {
								//Display an Error Dialog
								FrmCustomAlert(WarningAlert, "Sending Waypoints to GPSNAV Failed"," "," ");
							}
							XferClose(USESER);
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
							break;
						case CAI_GLIDERDATA:
							XferClose(data.config.nmeaxfertype);
							XferInit("4800", NFC, USESER);
							if (!(UploadCAIGliderInfo())) {
								//Display an Error Dialog
								FrmCustomAlert(WarningAlert, "Sending Glider Info to C302/CFR Failed"," "," ");
							}
							XferClose(USESER);
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
							break;
						case CAI_PILOTDATA:
							XferClose(data.config.nmeaxfertype);
							XferInit("4800", NFC, USESER);
							if (!(UploadCAIPilotInfo())) {
								//Display an Error Dialog
								FrmCustomAlert(WarningAlert, "Sending Pilot Info to C302/CFR Failed"," "," ");
							}
							XferClose(USESER);
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
							break;
						case GPN_PILOTDATA:
						case GPN_GLIDERDATA:
							XferClose(data.config.nmeaxfertype);
							XferInit("4800", NFC, USESER);
							if (!(UploadCAIPilotInfo())) {
								//Display an Error Dialog
								FrmCustomAlert(WarningAlert, "Sending Config Info to GPSNAV Failed"," "," ");
							}
							XferClose(USESER);
							SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
							XferInit(tempchar, NFC, data.config.nmeaxfertype);
							break;
						case VLK_WAYPOINTDATA:
							// not supported
							break;
						case VLK_GLIDERDATA:
							// not supported
							break;
						case VLK_PILOTDATA:
							// not supported		
							break;
						case LX_WAYPOINTDATA:
							// not supported
							break;
						case LX_GLIDERDATA:
							// not supported
							break;
						case LX_PILOTDATA:
							// not supported		
							break;
						default:
							break;
					}
					break;
				} // case form_transfer_tologgerbtn
				case form_transfer_fmloggerbtn:
				{
					if ((data.config.xfertype == USEVFS) && !device.CardRW) {
						FrmCustomAlert(WarningAlert, "Cannot write to SD Card"," "," ");
						break;
					}
					xfrdialog = XFRRECEIVE;
					switch (loggerdatatype)
					{
						case CAI_FLIGHTDATA:
//							HostTraceOutputTL(appErrorClass, "Transfer C302 Flights");
							XferClose(USESER);
							SysStringByIndex(form_set_port_speed_table, C302speed, tempchar, 7);
							XferInit(tempchar, NFC, USESER);
							// Speed gets reset at form close event
							FrmGotoForm( form_list_flts );
							break;
						case GPN_FLIGHTDATA:
//							HostTraceOutputTL(appErrorClass, "Transfer GPSNAV Flights");
							XferClose(USESER);
							// Set Max Speed Here for GPSNAV
							SysStringByIndex(form_set_port_speed_table, C302speed, tempchar, 7);
							XferInit(tempchar, NFC, USESER);
							// Speed gets reset at form close event
							FrmGotoForm( form_list_flts );
							break;
						case VLK_FLIGHTDATA:
//							HostTraceOutputTL(appErrorClass, "Transfer Volkslogger Flights");
							XferClose(USESER);
							SysStringByIndex(form_set_port_speed_table, VLKspeed, tempchar, 7);
							XferInit(tempchar, NFC, USESER);
							// Speed gets reset at form close event
							FrmGotoForm( form_list_flts );
							break;
						case LX_FLIGHTDATA:
//							HostTraceOutputTL(appErrorClass, "Transfer LX logger Flights");
							if (data.config.dataspeed < 8) {			// LX logger data speeds up to 38400bps, ethzsa
								XferClose(USESER);
								SysStringByIndex(form_set_port_speed_table, data.config.dataspeed, tempchar, 7);
								XferInit(tempchar, NFC, USESER);
								FrmGotoForm( form_list_flts );
							} else {
								FrmCustomAlert(WarningAlert, "Data speed can be max 38400bps!"," "," ");
							}
							// Speed gets reset at form close event
							break;
						case FLRM_FLIGHTDATA:
//							HostTraceOutputTL(appErrorClass, "Transfer Flarm logger Flights");
							//XferClose(USESER);
							//SysStringByIndex(form_set_port_speed_table, LXspeed, tempchar, 7);
							//XferInit(tempchar, NFC, USESER);
							// Speed gets reset at form close event
							FrmGotoForm( form_list_flts );
							break;
						default:
							break;
					}
					break;
				} // case form_transfer_tologgerbtn
				case form_transfer_delloggerbtn:
				{
					xfrdialog = XFRDELETE;
					switch (loggerdatatype)
					{
						case VLK_FLIGHTDATA:
//							HostTraceOutputTL(appErrorClass, "Delete Volks Flights");						
							if (FrmCustomAlert(ConfirmAlertYN, "Delete ALL flights in Volkslogger?"," "," ") == YES)
							if (FrmCustomAlert(ConfirmAlertYN, "Are you sure?"," "," ") == YES) {
								XferClose(USESER);
								SysStringByIndex(form_set_port_speed_table, VLKspeed, tempchar, 7);
								XferInit(tempchar, NFC, USESER);
								DeleteVLKLogs();
								XferClose(USESER);
								SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
								XferInit(tempchar, NFC, USESER);
							}
							break;
						case CAI_FLIGHTDATA:
						case GPN_FLIGHTDATA:
//							HostTraceOutputTL(appErrorClass, "Delete CAI Flights");						
							if (FrmCustomAlert(ConfirmAlertYN, "Delete ALL Flights in Logger?"," "," ") == YES) {
								if (FrmCustomAlert(ConfirmAlertYN, "Are you sure?"," "," ") == YES) {
									XferClose(USESER);
									SysStringByIndex(form_set_port_speed_table, C302speed, tempchar, 7);
									XferInit(tempchar, NFC, USESER);
									ClearCAIFlights();
									XferClose(USESER);
									SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
									XferInit(tempchar, NFC, USESER);
								}
							}
							break;
						case CAI_WAYPOINTDATA:
						case GPN_WAYPOINTDATA:
//							HostTraceOutputTL(appErrorClass, "Delete CAI Waypoints");						
							if (FrmCustomAlert(ConfirmAlertYN, "Delete ALL Waypoints in Logger?"," "," ") == YES) {
								if (FrmCustomAlert(ConfirmAlertYN, "Are you sure?"," "," ") == YES) {
									XferClose(USESER);
									SysStringByIndex(form_set_port_speed_table, C302speed, tempchar, 7);
									XferInit(tempchar, NFC, USESER);
									ClearCAIWaypoints();
									XferClose(USESER);
									SysStringByIndex(form_set_port_speed_table, data.config.nmeaspeed, tempchar, 7);
									XferInit(tempchar, NFC, USESER);
								}
							}
							break;
						default:
							break;
					}
					break;
				} // case form_transfer_delloggerbtn
			}  // switch ( event->data.ctlEnter.controlID )
			break;
//		} // switch (event->eType)

		default:
			break;
	}
	return(handled);
}

Boolean form_genalert_event_handler(EventPtr event)
{
	Boolean handled=false;
	static Int16 alertidx;
	RectangleType rectP;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "genalert Open");
			menuopen = false;
			alertidx = suaalert->alertidx;
			// setting the valid flag to false
			// that means that the window is open and hasn't returned yet
			// This should probably get set elsewhere after the return has
			// been processed but this covers things for now.
			suaalertret->valid = false;
			suaalertret->btnselected = -1;

			FrmDrawForm(FrmGetActiveForm());
			ctl_set_visible(form_genalert_btn0, false);
			ctl_set_visible(form_genalert_btn1, false);
			ctl_set_visible(form_genalert_btn2, false);
			ctl_set_visible(form_genalert_btn3, false);
			ctl_set_visible(form_genalert_btn4, false);
			ctl_set_visible(form_genalert_btn5, false);

			frm_set_title(suaalert->title);
			field_set_value(form_genalert_field0, suaalert->displaytext);

			if (suaalert->numbtns == 3) {
				ctl_set_label(form_genalert_btn3, suaalert->btn0text);
				ctl_set_label(form_genalert_btn4, suaalert->btn1text);
				ctl_set_label(form_genalert_btn5, suaalert->btn2text);
				ctl_set_visible(form_genalert_btn3, true);
				ctl_set_visible(form_genalert_btn4, true);
				ctl_set_visible(form_genalert_btn5, true);
			} else if (suaalert->numbtns == 2) {
				ctl_set_label(form_genalert_btn0, suaalert->btn0text);
				ctl_set_label(form_genalert_btn1, suaalert->btn1text);
				ctl_set_visible(form_genalert_btn0, true);
				ctl_set_visible(form_genalert_btn1, true);
			} else if (suaalert->numbtns == 1) {
				ctl_set_label(form_genalert_btn2, suaalert->btn0text);
				ctl_set_visible(form_genalert_btn2, true);
			}

			// underline default answer for task start alerts
			if ((suaalert->alerttype == TASKSTART_ALERT) && (suaalert->numbtns == 2) && allowgenalerttap) {
				if (suaalert->priority == RESTARTTASK) {
					// re-start
					WinDrawLine(30, 144, 47, 144);
				} else if (suaalert->priority == INVALIDSTART) {
					// invalid start
					WinDrawLine(25, 144, 51, 144);
				}
			}

			handled=true;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "genalert Close");
			allowgenalerttap = false;
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_genalert_btn0:
//					HostTraceOutputTL(appErrorClass, "genalert btn0");
					HandleWaitDialogWin(0);
					suaalertret->valid = true;
					suaalertret->btnselected = 0;
					suaalertret->alertidxret = suaalert->alertidx;
					suaalertret->alerttype = suaalert->alerttype;
					suaalertret->priority = suaalert->priority;
					handled = true;
					break;
				case form_genalert_btn1:
//					HostTraceOutputTL(appErrorClass, "genalert btn1");
					HandleWaitDialogWin(0);
					suaalertret->valid = true;
					suaalertret->btnselected = 1;
					suaalertret->alertidxret = suaalert->alertidx;
					suaalertret->alerttype = suaalert->alerttype;
					suaalertret->priority = suaalert->priority;
					handled = true;
					break;
				case form_genalert_btn2:
//					HostTraceOutputTL(appErrorClass, "genalert btn2");
					HandleWaitDialogWin(0);
					suaalertret->valid = true;
					suaalertret->btnselected = 2;
					suaalertret->alertidxret = suaalert->alertidx;
					suaalertret->alerttype = suaalert->alerttype;
					suaalertret->priority = suaalert->priority;
					handled = true;
					break;
				case form_genalert_btn3:
//					HostTraceOutputTL(appErrorClass, "genalert btn3");
					HandleWaitDialogWin(0);
					suaalertret->valid = true;
					suaalertret->btnselected = 3;
					suaalertret->alertidxret = suaalert->alertidx;
					suaalertret->alerttype = suaalert->alerttype;
					suaalertret->priority = suaalert->priority;
					handled = true;
					break;
				case form_genalert_btn4:
//					HostTraceOutputTL(appErrorClass, "genalert btn4");
					HandleWaitDialogWin(0);
					suaalertret->valid = true;
					suaalertret->btnselected = 4;
					suaalertret->alertidxret = suaalert->alertidx;
					suaalertret->alerttype = suaalert->alerttype;
					suaalertret->priority = suaalert->priority;
					handled = true;
					break;
				case form_genalert_btn5:
//					HostTraceOutputTL(appErrorClass, "genalert btn5");
					HandleWaitDialogWin(0);
					suaalertret->valid = true;
					suaalertret->btnselected = 5;
					suaalertret->alertidxret = suaalert->alertidx;
					suaalertret->alerttype = suaalert->alerttype;
					suaalertret->priority = suaalert->priority;
					handled = true;
					break;
				default:
					break;
			}
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "genalert Exit");
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
		case penUpEvent:
			// close alert if tapped in window
			// on the SUA info button
			//RctSetRectangle (&rectP, 106, 32, 154, 47); 
			//if (allowgenalerttap && (RctPtInRectangle (event->screenX, event->screenY, &rectP))) {
			//		PlayKeySound();
//			//		HostTraceOutputTL(appErrorClass, "genalert btninfo");
			//		HandleWaitDialogWin(0);
			//		suaalertret->valid = true;
			//		suaalertret->btnselected = 7;
			//		suaalertret->alertidxret = suaalert->alertidx;
			//		suaalertret->alerttype = suaalert->alerttype;
			//		suaalertret->priority = suaalert->priority;
			//		handled = true;
			//} else {
				// anywhere else on the screen
				RctSetRectangle (&rectP, 3, 4, 152, 103);
				if (allowgenalerttap && (RctPtInRectangle (event->screenX, event->screenY, &rectP))) {
					PlayKeySound();
//					HostTraceOutputTL(appErrorClass, "genalert default tap btn6");
					HandleWaitDialogWin(0);
					suaalertret->valid = true;
					suaalertret->btnselected = 6;
					suaalertret->alertidxret = suaalert->alertidx;
					suaalertret->alerttype = suaalert->alerttype;
					suaalertret->priority = suaalert->priority;
					handled=true;
				}
			//}
			break;
		default:
			break;
	}
//	if (suaalertret->valid) {
//		HostTraceOutputTL(appErrorClass, "Alert %s", DblToStr(suaalert->alerttype, 0));
//		HostTraceOutputTL(appErrorClass, "Priority %s", DblToStr(suaalert->priority, 0));
//		HostTraceOutputTL(appErrorClass, "Button %s", DblToStr(suaalertret->btnselected, 0));
//	}
	return(handled);
}

// Version of select_fg_waypoint using forms, not dialogs
// therefore hardware buttons work as well
void select_fg_waypoint(Int16 action) 
{
	//UInt16 alertresp=0;
	Char tempchar[30];
	//UInt16 TAKEOFFSET = 0;
	Boolean select_waypt = false;
	MemHandle waypoint_hand;
	MemPtr waypoint_ptr;

	switch (action) {
	case WAYNORM:
		// just go and select a waypoint
		select_waypt = true;
		break;
	case WAYSELECT:	
		// decide which dialog to invoke
  		if ((data.task.numwaypts > 0) && tasknotfinished) {
			// task is active
			if (taskonhold) {
				savtaskonhold = true;
				FrmPopupForm(form_wayselect_tr);
			} else {
				savtaskonhold = false;
				FrmPopupForm(form_wayselect_ta);
			}
		} else {
			// no task active, goto waypoint list
			select_waypt = true;
		}
		break;
	case WAYNEXT:
		// select next waypoint in task
		if (activetskway < data.task.numwaypts - 1) {
			activetskway++;
			tskpvwdist = 0.0;
			selectedTaskWayIdx = activetskway;
//			HostTraceOutputTL(appErrorClass, "Task Next Wpt %s",DblToStr(activetskway,0));
			HandleTaskAutoZoom(0.0, 0.0, true);
		}
		break;
	case WAYPREV:
		// select previous waypoint in task
		if (activetskway > 0) {
			activetskway--;
			tskpvwdist = 0.0;
			selectedTaskWayIdx = activetskway;
//		 	HostTraceOutputTL(appErrorClass, "Task Prev Wpt %s",DblToStr(activetskway,0));
			HandleTaskAutoZoom(0.0, 0.0, true);
		}
		break;
	case WAYADD:
		// Add new waypoint to active tsk
		if (data.task.numwaypts >= MAXWAYSPERTASK) {
			warning->type = Wgeneric;
			StrCopy(warning->line1, "Cannot Add Waypoint");
			StrCopy(tempchar, "Max ");
			StrCat(tempchar, DblToStr(MAXWAYSPERTASK,0));
			StrCat(tempchar, " per Task");
			StrCopy(warning->line2, tempchar);
			FrmPopupForm(form_warning);
		} else {
			if ((taskIndex == 0) && (selectedTaskWayIdx < activetskway-1) && (numWaypts >= 1) && (data.activetask.tskstartsecs > 0)) {
				warning->type = Wgeneric;
				StrCopy(warning->line1, "Cannot Add Before");
				StrCopy(warning->line2, "Accomplished Waypoint");
				FrmPopupForm(form_warning);
			} else {
				newTaskWay = true;
				selectedWaypointIndex = -1;
				addWayToTask = true;
				select_waypt = true;
				HandleTaskAutoZoom(0.0, 0.0, true);
			}
		}
		break;
	case WAYTEMP:
		if (!recv_data || (recv_data && (StrCompare(data.logger.gpsstat, "A") != 0))) {
			// clear track value if in task preview mode
			data.input.true_track.value = 0.0;
			data.input.true_track.valid = VALID;
		}
		taskonhold = true;
		glidingtoheight = ELEVATION;
		selectedWaypointIndex = -1;
		select_waypt = true;
		HandleTaskAutoZoom(0.0, 0.0, true);
		break;
	case WAYDEACT:
		if (!recv_data || (recv_data && (StrCompare(data.logger.gpsstat, "A") != 0))) {
			// clear track value if in task preview mode
			data.input.true_track.value = 0.0;
			data.input.true_track.valid = VALID;
		}
		StrCopy(tempchar, data.inuseWaypoint.name);
		refresh_task_details(TASKCLRACT);
		selectedWaypointIndex = FindWayptRecordByName(tempchar);
		select_waypt = true;
		break;
	case WAYRESUME:
		taskonhold = false;
		savtaskonhold = false;
		timeinfo[0] = '\0';
		HandleTask(TSKREACTIVATE);
		break;
	case WAYLAND:
		// goto to nearest airport list
		emergencyland = true;
		taskonhold = true;
		glidingtoheight = ELEVATION;
		selectedWaypointIndex = -1;
		select_waypt = true;
		break;
	case WAYGO:
		// select the highlighted waypoint
		if (wayselect) {
			pressedgo = true;
			if (selectedWaypointIndex != -1) {
//				HostTraceOutputTL(appErrorClass, "Selected Waypt Idx : %s", DblToStr(selectedWaypointIndex,0));
				OpenDBQueryRecord(waypoint_db, selectedWaypointIndex, &waypoint_hand, &waypoint_ptr);
				MemMove(selectedWaypoint, waypoint_ptr, sizeof(WaypointData));
				MemHandleUnlock(waypoint_hand);
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
				FrmGotoForm(origform);
				switch (origform) {
					case form_final_glide:
					case form_moving_map:
						if (addWayToTask && selectedWaypointIndex != -1) {
	//						HostTraceOutputTL(appErrorClass, "tskWayAddIdx = selectedWaypointIndex BUTTON");
							tskWayAddIdx = selectedWaypointIndex;
							newTaskWay = false;
							refresh_task_details(TASKADDBEFORE);
							refresh_task_details(TASKACTV);
						} else if (selectedWaypointIndex == -1) {
							taskonhold = false;
							savtaskonhold = false;
							HandleTask(TSKREACTIVATE);
						}
						break;
				}
 			} else {
//				HostTraceOutputTL(appErrorClass, "No Waypoint Selected");
				MemSet(selectedWaypoint, sizeof(WaypointData),0);
				FrmGotoForm(origform);
			}
		} else {
			FrmGotoForm(origform);
 		}
		break;
	default:
		break;
	}
	HandleTask(TSKNONE);
	if (select_waypt) {
		// put list pointer at currently inuse waypoint : SELECT mode
		wayselect = data.config.usepalmway;
		selectedWaypointIndex = FindWayptRecordByName(data.inuseWaypoint.name);
		currentWayptPage = (Int16)selectedWaypointIndex/7;
		FrmGotoForm(form_list_waypt);
	}
	data.application.changed = 1;
	updatemap = true;
	return;
}

// set MC value
Boolean form_set_mc_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
//	static UInt32 formopen = 0xffffffff;
	EventType newEvent;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "set_mc Open");
			menuopen = false;
			FrmDrawForm(pfrm);
			field_set_value(form_set_mc_units, data.input.lfttext);
			WinDrawLine(32, 61, 44, 61);
			formopen = cursecs;
			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
				// update MC value field
				field_set_value(form_set_mc_value, print_vertical_speed2(data.input.basemc, data.input.lftprec));
				// auto dismiss dialog form
				if (cursecs > formopen + 10) {
					// push cancel button press into event queue
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_set_mc_done;
					EvtAddEventToQueue(&newEvent);
				}
			}
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_ta Close");
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_ta Exit");
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
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			formopen = cursecs;
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_mc_up:
					incMC(data.input.basemc * data.input.lftconst);
					updatemap = true;
					handled = true;
					break;
				case form_set_mc_down:
					decMC(data.input.basemc * data.input.lftconst);
					updatemap = true;
					handled = true;
					break;
				case form_set_mc_done:
					return_to_form(0);
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

// select waypoint dialog with task active
Boolean form_wayselect_ta_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
	Char tempchar[30];
//	static UInt32 formopen = 0xffffffff;
	EventType newEvent;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_ta Open");
			menuopen = false;
			FrmDrawForm(pfrm);

			StrCopy(tempchar,"Task Wpt : ");
			StrCat(tempchar, data.task.wayptnames[activetskway]);
			frm_set_title(tempchar);
			if (activetskway > 0) {
				ctl_set_label(form_wayselect_ta_prevwpt, data.task.wayptnames[activetskway-1]);
			} else {
				ctl_set_label(form_wayselect_ta_prevwpt, "None");
			}
			if (activetskway < data.task.numwaypts-1) {
				ctl_set_label(form_wayselect_ta_nextwpt, data.task.wayptnames[activetskway+1]);
			} else {
				ctl_set_label(form_wayselect_ta_nextwpt, "None");
			}
			WinDrawLine(61, 115, 93, 115);
			formopen = cursecs;
			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
				// auto dismiss dialog form
				if (cursecs > formopen + data.config.autodismisstime) {
					// push cancel button press into event queue
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_wayselect_ta_cancelbtn;
					EvtAddEventToQueue(&newEvent);
				}
			}
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_ta Close");
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_ta Exit");
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
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			return_to_form(0);
			switch ( event->data.ctlEnter.controlID ) {
				case form_wayselect_ta_prevwpt:
					// flag manual waypoint change
					manchgwpt = true;
					data.flight.invalidturn[activetskway] = true;
					data.flight.timeatturn[activetskway] = utcsecs;
					select_fg_waypoint(WAYPREV);
					handled = true;
					break;
				case form_wayselect_ta_nextwpt:
					if (inflight) {
						// flag manual waypoint change
						manchgwpt = true;
						data.flight.invalidturn[activetskway] = true;
						data.flight.timeatturn[activetskway] = utcsecs;
					}
					if (inflight && (data.activetask.tskstartsecs == 0)) {
						// task not started, prompt for manual start
						question->type = Qmanualstart;
						FrmPopupForm(form_question);
						data.application.changed = 1;
						updatemap = true;
					} else {
						// manually force next waypoint
						select_fg_waypoint(WAYNEXT);
					}
					handled = true;
					break;
				case form_wayselect_ta_cancelbtn:
					handled = true;
					break;
				case form_wayselect_ta_addbtn:
					select_fg_waypoint(WAYADD);
					handled = true;
					break;
				case form_wayselect_ta_tempbtn:
					select_fg_waypoint(WAYTEMP);
					handled = true;
					break;
				case form_wayselect_ta_deactbtn:
					// ask for confirmation if flight is active and task started
					if (inflight && (activetskway > (data.task.hastakeoff?1:0))) {
						question->type = Qdeacttask;
						FrmPopupForm(form_question);
					} else {
						select_fg_waypoint(WAYDEACT);
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
	return(handled);
}

// select waypoint dialog with temp waypoint currently selected (and task active)
Boolean form_wayselect_tr_event_handler(EventPtr event)
{
	Boolean handled=false;
//	static UInt32 formopen = 0xffffffff;
	EventType newEvent;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_tr Open");
			menuopen = false;

			FrmDrawForm(FrmGetActiveForm());
			WinDrawLine(61, 107, 93, 107);
			formopen = cursecs;
			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
				// auto dismiss dialog form
				if (cursecs > formopen + data.config.autodismisstime) {
					// push cancel button press into event queue
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_wayselect_tr_cancelbtn;
					EvtAddEventToQueue(&newEvent);
				}
			}
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_tr Close");
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_tr Exit");
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
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			return_to_form(0);
			switch ( event->data.ctlEnter.controlID ) {
				case form_wayselect_tr_cancelbtn:
					handled = true;
					break;
				case form_wayselect_tr_tempbtn:
					select_fg_waypoint(WAYTEMP);
					handled = true;
					break;
				case form_wayselect_tr_deactbtn:
					// ask for confirmation if flight is active and task started
					if (inflight && (activetskway > (data.task.hastakeoff?1:0))) {
						question->type = Qdeacttask;
						FrmPopupForm(form_question);
					} else {
						select_fg_waypoint(WAYDEACT);
					}
					handled = true;
					break;
				case form_wayselect_tr_resumebtn:
					select_fg_waypoint(WAYRESUME);
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

// add remove delete dialog in task editor
Boolean form_wayselect_te_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
//	static UInt32 formopen = 0xffffffff;
	EventType newEvent;
	Char tempchar[30];

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_te Open");
			menuopen = false;

			FrmDrawForm(pfrm);
			StrCopy(tempchar,"Edit Wpt : ");
			if (selectedTaskWayIdx >= 0) {
				StrCat(tempchar, tsk->wayptnames[selectedTaskWayIdx]);
			} else {
				StrCat(tempchar, "None");
			}
			frm_set_title(tempchar);
			WinDrawLine(61, 85, 93, 85);
			formopen = cursecs;
			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
				// auto dismiss dialog form
				if (cursecs > formopen + data.config.autodismisstime) {
					// push cancel button press into event queue
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_wayselect_te_cancelbtn;
					EvtAddEventToQueue(&newEvent);
				}
			}
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_te Close");
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "wayselect_te Exit");
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
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			dispactive = false; // don't want to change selected waypoint in task
			return_to_form(0);
			switch ( event->data.ctlEnter.controlID ) {
				case form_wayselect_te_cancelbtn:
					handled = true;
					break;
				case form_wayselect_te_addbtn:
					// push add button press into event queue
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_set_task_addbtn;
					EvtAddEventToQueue(&newEvent);
					handled = true;
					break;
				case form_wayselect_te_editbtn:
					// push add button press into event queue
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_set_task_editbtn;
					EvtAddEventToQueue(&newEvent);
					handled = true;
					break;
				case form_wayselect_te_deletebtn:
					refresh_task_details(TASKREM);
					handled = true;
					break;
				case form_wayselect_te_reversebtn:
					refresh_task_details(TASKREV);
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

// question dialog
Boolean form_question_event_handler(EventPtr event)
{
	Boolean handled=false;
//	static UInt32 formopen = 0xffffffff;
	EventType newEvent;
	Int16 defstart, defend, defwidth;
	Char deflabel[11];
	Char loggername[20];
	Char tempchar[30];
	RectangleType rectP;
	static Boolean allowgentap = false;
	
	// find logger name
	if ((data.config.flightcomp == EWMRSDCOMP) ||
		(data.config.flightcomp == EWMRSDTASCOMP)) {
		// special case for EWMR or EWMR-SD logger
		SysStringByIndex(form_comp_table, EWMRCOMP, loggername, 15);
	} else {
		SysStringByIndex(form_comp_table, data.config.flightcomp, loggername, 15);
	}
	
	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "question Open");
			menuopen = false;
			allowgentap = false;
			FrmDrawForm(FrmGetActiveForm());

			// set up question text and buttons
			frm_set_title("Confirm!");
			switch (question->type) {
				case Qmanualstart:
					field_set_value(form_question_line1, "Manually Start Task?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Start");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = true;
					break;
				case Qnewflight:
					field_set_value(form_question_line1, "Task already declared");
					field_set_value(form_question_line2, "in this flight!");
					ctl_set_label(form_question_yes, "Continue");
					ctl_set_label(form_question_no, "New Flight");
					question->autodismiss = true;
					question->default_answer = true;
					break;
				case QexitSP:
					field_set_value(form_question_line1, "Do you want to continue");
					field_set_value(form_question_line2, "running SoarPilot?");
					ctl_set_label(form_question_yes, "Continue");
					ctl_set_label(form_question_no, "Exit");
					question->autodismiss = true;
					question->default_answer = true;
					break;
				case Qdeacttask:
					field_set_value(form_question_line1, "Delete Active Task?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Delete");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case Qaddnewwpt:
					field_set_value(form_question_line1, "Add New Waypoint?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Add");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case Qdelwpt:
					field_set_value(form_question_line1, "Delete Waypoint?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Delete");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case QAAToverwrite:
					field_set_value(form_question_line1, "Overwrite Manual Target");
					field_set_value(form_question_line2, "Placement?");
					ctl_set_label(form_question_yes, "Overwrite");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case Qdelpolar:
					field_set_value(form_question_line1, "Delete Polar?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Delete");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case Qdeltask:
					field_set_value(form_question_line1, "Delete Task?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Delete");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case Qclrtask:
					field_set_value(form_question_line1, "Clear Active Task?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Clear");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case Qdeclaretask:
					StrCopy(tempchar, "Declare to ");
					// special case for EWMR-SD logger, declaring to SD card
					if ((data.config.flightcomp == EWMRSDCOMP || 
						  data.config.flightcomp == EWMRSDTASCOMP ||
						  data.config.flightcomp == FLARMCOMP) &&
						 (data.config.declaretoSD)) {
						StrCat(loggername, " SD Card");
					}
					StrCat(loggername, "?");
					StrCat(tempchar, loggername);
					if (inflight) {
						field_set_value(form_question_line1, "Logger is active!");
						field_set_value(form_question_line2, tempchar);
					} else {
						field_set_value(form_question_line1, tempchar);
						field_set_value(form_question_line2, " ");
					}
					ctl_set_label(form_question_yes, "Declare");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case Qcleardec:
					StrCopy(tempchar, "Clear ");
					StrCat(tempchar, loggername);
					field_set_value(form_question_line1, tempchar);
					field_set_value(form_question_line2, "Declaration?");
					ctl_set_label(form_question_yes, "Clear");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case QdelSUA:										
					field_set_value(form_question_line1, "Delete SUA Item?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Delete");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case Qacttaskchg:
					field_set_value(form_question_line1, "Active Task Changed!");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Activate");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = inflight;
					question->default_answer = inflight;	// default to declare changes if in flight
					break;
				case Qsetactivewpt:
					field_set_value(form_question_line1, "Change Active Waypoint To");
					StrCopy(tempchar, data.task.wayptnames[selectedTaskWayIdx]);
					field_set_value(form_question_line2, tempchar);
					ctl_set_label(form_question_yes, "Change");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;	// default to declare changes if in flight
					break;
				case Qrulesoff:
					field_set_value(form_question_line1, "Turn Task Rules Off?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Rules Off");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case QturnAAT:
					if (data.config.AATmode == AAT_MTURN_ON) {
						field_set_value(form_question_line1, "Manually Turn in Area?");
						ctl_set_label(form_question_yes, "Turn");
					} else {
						field_set_value(form_question_line1, "Return to Area?");
						ctl_set_label(form_question_yes, "Return");
					}
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
					break;
				case QturnOLC:
					field_set_value(form_question_line1, "Record OLC turnpoint here?");
					StrCopy(tempchar, "Replacing ");
					StrCat(tempchar, data.task.wayptnames[selectedTaskWayIdx]);
					field_set_value(form_question_line2, tempchar);
					ctl_set_label(form_question_yes, "Turn");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;	// default to declare changes if in flight
					break;
				case Qwindcalcoff:
					field_set_value(form_question_line1, "Turn Off Wind Calculation?");
					field_set_value(form_question_line2, " ");
					ctl_set_label(form_question_yes, "Off");
					ctl_set_label(form_question_no, "Cancel");
					question->autodismiss = true;
					question->default_answer = false;
				        break;
				default:
					break;
			}
			// underline default answer
			FntSetFont(largeBoldFont);
			if (question->default_answer) {
				StrCopy(deflabel, ctl_get_label(form_question_yes));
				defwidth = FntCharsWidth (deflabel, StrLen(deflabel));
				defstart = 36 - defwidth/2;
				defend = defstart + defwidth - 2;
			} else {
				StrCopy(deflabel, ctl_get_label(form_question_no));
				defwidth = FntCharsWidth (deflabel, StrLen(deflabel));
				defstart = 120 - defwidth/2;
				defend = defstart + defwidth-2;
			}
			FntSetFont(stdFont);
			WinDrawLine(defstart, 57 , defend, 57);
//			HostTraceOutputT(appErrorClass, "%s", field_get_str(form_question_line1));
//			HostTraceOutputTL(appErrorClass, " %s", field_get_str(form_question_line2));
			formopen = cursecs;
			PlaySound(1047, 125, sndMaxAmp);
			handled=true;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "question Close");
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "question Exit");
//			HostTraceOutputTL(appErrorClass, "menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "question Exit");
//				HostTraceOutputTL(appErrorClass, "menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case nilEvent:
			if (!menuopen) {
				// auto dismiss question form
				allowgentap = true;
				if (question->autodismiss && (cursecs > formopen + data.config.autodismisstime)) {
					// push default button press into event queue
					newEvent.eType = ctlSelectEvent;
					if (question->default_answer) {
						newEvent.data.ctlEnter.controlID = form_question_yes;
					} else {
						newEvent.data.ctlEnter.controlID = form_question_no;
					}
					EvtAddEventToQueue(&newEvent);
				}
			}
			handled = false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_question_yes:
					return_to_form(0); // close question dialog
					switch (question->type) {
						case Qmanualstart:
							// force manual start
							HandleTask(TSKFORCESTART);
							break;
						case Qnewflight:
							// continue
							break;
						case QexitSP:
							// continue
							break;
						case Qdeacttask:
							// de-activate task
							select_fg_waypoint(WAYDEACT);
							break;
						case Qaddnewwpt:
							// add new waypoint at current position and terrain elevation
							SaveWaypoint(data.input.gpslatdbl, data.input.gpslngdbl, GetTerrainElev(data.input.gpslatdbl, data.input.gpslngdbl), false);
							updatemap = true;
							break;
						case Qdelwpt:
							// delete waypoint
							OpenDBDeleteRecord(waypoint_db, selectedWaypointIndex);
							if (selectedWaypointIndex != -1) {
								selectedWaypointIndex = FindWayptRecordByName(data.inuseWaypoint.name);
							}
							if (selectedWaypointIndex == -1) {
								MemSet(selectedWaypoint, sizeof(WaypointData), 0);
							}
							FrmGotoForm(form_list_waypt);
							break;
						case QAAToverwrite:
							// set AAT target points
							setAATdist(AATdist);
							refresh_task_details(TASKDISP);
							break;
						case Qdelpolar:
							// delete polar
							OpenDBDeleteRecord(polar_db, selectedPolarIndex);
							if (selectedPolarIndex != -1) {
								FrmGotoForm(form_list_polar);
							} else {
								MemSet(selectedPolar, sizeof(PolarData), 0);
								MemSet(&data.polar, sizeof(PolarData), 0);
								MemSet(inusePolar, sizeof(PolarData), 0);
							}
							break;
						case Qdeltask:
							// delete task
							OpenDBDeleteRecord(task_db, taskIndex);
							taskIndex = taskIndex - 1;
							selectedTaskWayIdx = -1;
							break;
						case Qclrtask:
							// cler active task
							numWaypts = 0;
							tsk->numwaypts = 0;
							tsk->hastakeoff = false;
							tsk->haslanding = false;
							StrCopy(tsk->name, "Active Task");
							AssignDefaultRules(tsk);
							OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
							if (data.task.numwaypts > 0) {
								HandleTask(TSKDEACTIVATE);
								data.task.numwaypts = 0;
								data.task.numctlpts = 0;
							}
							mustActivateAsNew = true;
							break;
						case Qdeclaretask:
							// declare task
							CompCmd = DECSEND;
							break;
						case Qcleardec:
							// clear declared task
							CompCmd = DECCLEAR;
							break;
						case QdelSUA:
							// delete SUA item
							OpenDBDeleteRecord(suaidx_db, selectedSUAListIdx);
							selectedSUAListIdx = 0;
							FrmGotoForm(form_list_sua);
							break;	
						case Qacttaskchg:
							// re-activate task
							taskIndex = 0;
							refresh_task_details(TASKACTV);
							if (goingtomenu) {
								// push menu key press into event queue
								EvtEnqueueKey (menuChr, 0, commandKeyMask);
								goingtomenu = false;
							}
							break;
						case Qsetactivewpt:
							// move activetskway to the requested waypoint
							refresh_task_details(TASKSETWPT);
							break;
						case Qrulesoff:
							tsk->rulesactive = false;
							goingtomenu = false;
							goingtotaskedit = true;
							if (skipnewflight) {
								OpenDBUpdateRecord(task_db, sizeof(TaskData), tsk, taskIndex);
								FrmGotoForm(origform);
							}
							break;
						case QturnAAT:
							// toggle manual turn in Area, if required
							if (data.config.AATmode == AAT_MTURN) {
//								HostTraceOutputTL(appErrorClass, "SET : Manual Turn On %s", DblToStr(inareasector,0));
								data.config.AATmode = AAT_MTURN_ON;
							} else if (data.config.AATmode == AAT_MTURN_ON) {
								data.config.AATmode = AAT_MTURN;
//								HostTraceOutputTL(appErrorClass, "RESET : Manual Turn Off %s", DblToStr(inareasector,0));
								if (inareasector > -1) {
									// update task distances achieved in area
//									HostTraceOutputTL(appErrorClass, "TURNING IN AREA: active dists -> task dists");
									data.task.distlats[inareasector] = data.activetask.distlats[inareasector];
									data.task.distlons[inareasector] = data.activetask.distlons[inareasector];
								}
								PlayTurnSound();
							}
							tgtchg = true;
							break;
						case QturnOLC:
							// activate modified task
							FrmGotoForm(defaultscreen);
							skipnewflight = true;
							taskIndex = 0;
							refresh_task_details(TASKACTV);
							// move activetskway to the requested waypoint
							refresh_task_details(TASKSETWPT);
							PlayTurnSound();
							break;
						case Qwindcalcoff:
							data.config.calcwind = false;
							break;
						default:
							// continue
							break;
					}
					handled = true;
					break;
				case form_question_no:
					if (question->type != QexitSP) return_to_form(0); // close question dialog
					switch (question->type) {
						case Qmanualstart:
							// don't start the task, but go to the next waypoint anyway
							select_fg_waypoint(WAYNEXT);
							break;
						case Qnewflight:
							// stop logger and declare new task
							MakeNewFlight();
							break;
						case QexitSP:
							// exit from SoarPilot
							allowExit = true;
							newEvent.eType = appStopEvent;
							EvtAddEventToQueue(&newEvent);
							break;
						case Qacttaskchg:
							// re-load task terrain
							tskoffterrain = !loadtskterrain(&data.task);
							// copy active task back to record first record in task db
							OpenDBUpdateRecord(task_db, sizeof(TaskData), &data.task, 0);
							set_task_changed = false;
							if (goingtomenu) {
								// push menu key press into event queue
								EvtEnqueueKey (menuChr, 0, commandKeyMask);
								goingtomenu = false;
							}
							break;
						case Qrulesoff:
							goingtomenu = false;
							goingtotaskedit = true;
							break;
						case Qdeacttask:
						case Qaddnewwpt:
						case Qdelwpt:
						case QAAToverwrite:
						case Qdelpolar:
						case Qdeltask:
						case Qclrtask:
						case Qdeclaretask:
						case Qcleardec:
						case QdelSUA:
						case Qsetactivewpt:
						case QturnAAT:
						case QturnOLC:
						case Qwindcalcoff:
						default:
							// do nothing
							break;
					}
					handled = true;
					break;

				default:
					break;
			}
			break;
		case penUpEvent:
			//  tap anywhere in question form to dismiss
			RctSetRectangle (&rectP, 3, 3, 152, 63);
			if (allowgentap && RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//				HostTraceOutputTL(appErrorClass, "dismiss question");
				PlayKeySound();
				// push default button press into event queue
				newEvent.eType = ctlSelectEvent;
				if (question->default_answer) {
					newEvent.data.ctlEnter.controlID = form_question_yes;
				} else {
					newEvent.data.ctlEnter.controlID = form_question_no;
				}
				EvtAddEventToQueue(&newEvent);
				handled=true;
			}
			break;
		default:
			break;
	}
	return(handled);
}

// warning dialog
Boolean form_warning_event_handler(EventPtr event)
{
	Boolean handled=false;
//	static UInt32 formopen = 0xffffffff;
	EventType newEvent;
	Char tempchar[21];
	RectangleType rectP;
	static Boolean allowgentap = false;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "warning Open");
			menuopen = false;
			allowgentap = false;
			FrmDrawForm(FrmGetActiveForm());

			// set up warning text and buttons
			switch (warning->type) {
				case Wgeneric:
				case Wlogtime:
				case Wcardwrite:
					frm_set_title("Warning!");
					break;
				case Wfinished:
					frm_set_title("Finished");
					break;
				case Werror:
					frm_set_title("Error!");
					break;
				case Winfo:
					frm_set_title("Information");
					break;
				default:
					break;
			}
			switch (warning->type) {
				case Wgeneric:
				case Wfinished:
				case Werror:
				case Winfo:
					field_set_value(form_warning_line1, warning->line1);
					field_set_value(form_warning_line2, warning->line2);
					ctl_set_label(form_warning_ok, "OK");
					break;
				case Wcardwrite:
					field_set_value(form_warning_line1, "Cannot write to SD card");
					field_set_value(form_warning_line2, "");
					ctl_set_label(form_warning_ok, "OK");
					break;
				case Wlogtime:
					StrCopy(tempchar, "Only ");
					StrCat(tempchar, DblToStr(estlogtimeleft(),1));
					StrCat(tempchar, " hours");
					field_set_value(form_warning_line1, tempchar);
					field_set_value(form_warning_line2, "Logging Time Left");
					ctl_set_label(form_warning_ok, "OK");
					break;
				default:
					break;
			}
//			HostTraceOutputT(appErrorClass, "%s", warning->line1);
//			HostTraceOutputTL(appErrorClass, " %s", warning->line2);
			formopen = cursecs;
			PlaySound(1047, 125, sndMaxAmp);
			handled=true;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "warning Close");
			handled = false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "warning Exit");
//			HostTraceOutputTL(appErrorClass, "menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "menuopen = false");
				menuopen = false;
			}
			handled = false;
			break;
		case nilEvent:
			if (!menuopen) {
				// auto dismiss warning form
				allowgentap = true;
				if (cursecs > formopen + data.config.autodismisstime) {
					// push default button press into event queue
					newEvent.eType = ctlSelectEvent;
					newEvent.data.ctlEnter.controlID = form_warning_ok;
					EvtAddEventToQueue(&newEvent);
				}
			}
			handled = false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_warning_ok:
					switch (warning->type) {
						case Wgeneric:
						case Wfinished:
						case Werror:
						case Winfo:
							// continue
							return_to_form(0);
							break;
						case Wcardwrite:
						case Wlogtime:
							// goto startup screen
							FrmGotoForm(startupscreen);
							break;
						default:
							break;
					}
					handled = true;
					break;
				default:
					break;
			}
			break;
		case penUpEvent:
			//  tap anywhere in warning form to dismiss
			RctSetRectangle (&rectP, 3, 3, 152, 63);
			if (allowgentap && RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
//				HostTraceOutputTL(appErrorClass, "dismiss warning");
				PlayKeySound();
				// push default button press into event queue
				newEvent.eType = ctlSelectEvent;
				newEvent.data.ctlEnter.controlID = form_warning_ok;
				EvtAddEventToQueue(&newEvent);
				handled=true;
			}
			break;
		default:
			break;
	}
	return(handled);
}

Boolean form_set_sms_event_handler(EventPtr event)
{
	Boolean handled=false;
	Char tempchar[30];

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
//			HostTraceOutputTL(appErrorClass, "set_sms Open");
			menuopen = false;

			FrmDrawForm(FrmGetActiveForm());

			if (device.TreoPDA) {
			ctl_set_value(form_set_sms_enable, data.config.outputSMS);
			field_set_value(form_set_sms_address, data.config.SMSaddress);
			switch(data.config.SMSouttype) {
				case SMSOUTGEN:
					ctl_set_value(form_set_sms_outgen, true);
					break;
				case SMSOUTPW:
					ctl_set_value(form_set_sms_outpw, true);
					break;
				default:
					ctl_set_value(form_set_sms_outgen, true);
					break;
			}
			if (data.config.SMSsendtype & SMSSENDTO) ctl_set_value(form_set_sms_sendto, true);
			if (data.config.SMSsendtype & SMSSENDLAND) ctl_set_value(form_set_sms_sendland, true);
			if (data.config.SMSsendtype & SMSSENDPERIOD) ctl_set_value(form_set_sms_sendperiod, true);

			StrIToA(tempchar, data.config.SMSsendint / 60);
			field_set_value(form_set_sms_sendint, tempchar);
			} else {
				ctl_set_visible(form_set_sms_enable, false);
				ctl_set_visible(form_set_sms_addresslbl, false);
				field_set_visible(form_set_sms_address, false);
				ctl_set_visible(form_set_sms_outlbl, false);
				ctl_set_visible(form_set_sms_outgen, false);
				ctl_set_visible(form_set_sms_outpw, false);
				ctl_set_visible(form_set_sms_sendlbl, false);
				ctl_set_visible(form_set_sms_sendto, false);
				ctl_set_visible(form_set_sms_sendland, false);
				ctl_set_visible(form_set_sms_sendperiod, false);
				ctl_set_visible(form_set_sms_sendintlbl, false);
				field_set_visible(form_set_sms_sendint, false);
				ctl_set_visible(form_set_sms_minslbl, false);
				ctl_set_visible(form_set_sms_sendnow, false);

				StrCopy(tempchar, "SMS Not Supported!");
				WinDrawChars(tempchar, StrLen(tempchar), 40, 40);
				StrCopy(tempchar, "Must Be Palm Treo.");
				WinDrawChars(tempchar, StrLen(tempchar), 40, 50);
			}

			handled=true;
			break;
		case frmCloseEvent:
//			HostTraceOutputTL(appErrorClass, "set_sms Close");
			if (device.TreoPDA) {
				StrCopy(data.config.SMSaddress, field_get_str(form_set_sms_address));
				data.config.SMSsendtype = NULL;
				if (ctl_get_value(form_set_sms_sendto)) data.config.SMSsendtype |= SMSSENDTO;
				if (ctl_get_value(form_set_sms_sendland)) data.config.SMSsendtype |= SMSSENDLAND;
				if (ctl_get_value(form_set_sms_sendperiod)) data.config.SMSsendtype |= SMSSENDPERIOD;
				data.config.SMSsendint = (field_get_long(form_set_sms_sendint) * 60);
			}
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "set_sms Exit");
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
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_sms_enable:
					data.config.outputSMS = ctl_get_value(form_set_sms_enable);
					handled = true;
					break;
				case form_set_sms_outgen:
					data.config.SMSouttype = SMSOUTGEN;
					handled = true;
					break;
				case form_set_sms_outpw:
					data.config.SMSouttype = SMSOUTPW;
					handled = true;
					break;
				case form_set_sms_sendto:
					handled = true;
					break;
				case form_set_sms_sendland:
					handled = true;
					break;
				case form_set_sms_sendperiod:
					handled = true;
					break;
				case form_set_sms_sendnow:
					if (recv_data && StrCompare(data.logger.gpsstat, "A") == 0) {
						// Send the current positional info if getting good data
						// regardless of whether the logger is
						// active or not.
						SendSMS(SMSSENDNOW);
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
	return(handled);
}

Boolean form_set_scrorder_event_handler(EventPtr event) 
{ 
	Boolean handled=false; 
	FormPtr frm;
	ControlPtr ctl;
	ListPtr lst;
	Int8 curscr,i;
	UInt16 fldID, newfldID;
	static Int16 selectedscreen = -1;
	
	switch (event->eType) { 
		case frmOpenEvent: 
		case frmUpdateEvent: 
			menuopen = false; 
			frm = FrmGetActiveForm();
			FrmDrawForm(frm);
			WinDrawLine(0,37,160,37);
			// default screen
			ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_scrorder_scrd1));
			lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_set_scrorder_scrd2));
			LstSetSelection (lst, (data.config.defaulttoFG?0:1));
			CtlSetLabel (ctl, LstGetSelectionText(lst, (data.config.defaulttoFG?0:1)));
			// show Set QFE/QNH on startup
			ctl_set_value(form_set_scrorder_scrs1, data.config.showQFEQNHonstartup);
			// screen chain
			for (i=0; i<SCREENCHAINMAX; i++) {
				fldID = form_set_scrorder_scr01 + 10*i;
				ctl = (ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,fldID));
				lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,fldID+1));
				LstSetSelection (lst, FrmIDToScr(data.config.screenchain[i]));
				//LstSetTopItem (lst, FrmIDToScr(data.config.screenchain[i]));
				CtlSetLabel (ctl, LstGetSelectionText(lst, FrmIDToScr(data.config.screenchain[i])));
			}
			handled=true; 
			break;
		case frmCloseEvent:
			data.config.showQFEQNHonstartup = ctl_get_value(form_set_scrorder_scrs1);
			handled=false; 
			break; 
		case popSelectEvent:  // A popup button was pressed and released.
//			HostTraceOutputTL(appErrorClass, "Popup Pressed %s", DblToStr(event->data.popSelect.controlID ,0));
			PlayKeySound();
			if (event->data.popSelect.controlID == form_set_scrorder_scrd1) {
				// handle default screen
				if (event->data.popSelect.selection == 1) {
					data.config.defaulttoFG = false;
					defaultscreen = form_moving_map;
				} else {
					data.config.defaulttoFG = true;
					defaultscreen = form_final_glide;
				}
			} else if (event->data.popSelect.controlID == form_set_scrorder_scrs1) {
				// handle start screen
				//data.config.showQFEQNHonstartup = ctl_get_value(form_set_scrorder_scrs1);
			} else {
				// handle screen chain
				fldID = (event->data.popSelect.controlID - form_set_scrorder_scr01)/10 ;
				if ((fldID >= 0) && (fldID < SCREENCHAINMAX)) {
					newfldID = ScrToFrmID(event->data.popSelect.selection);
					curscr = IsScreenInUse(newfldID, fldID, data.config.screenchain);
					if ((curscr < SCREENCHAINLIST) && (newfldID != form_set_scrorder_noscr)) {
						// Open Error Dialog
						warning->type = Wgeneric;
						StrCopy(warning->line1, "Screen can only appear");
						StrCopy(warning->line2, "in the list once!");
						FrmPopupForm(form_warning);
					} else {
						data.config.screenchain[fldID] = newfldID;
					}
				}
			}
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
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_scrorder_scrlbl1:
				case form_set_scrorder_scrlbl2:
				case form_set_scrorder_scrlbl3:
				case form_set_scrorder_scrlbl4:
				case form_set_scrorder_scrlbl5:
				case form_set_scrorder_scrlbl6:
				case form_set_scrorder_scrlbl7:
				case form_set_scrorder_scrlbl8:
				case form_set_scrorder_scrlbl9:
				case form_set_scrorder_scrlbl10:
					if (ctl_get_value(event->data.ctlEnter.controlID) && (selectedscreen == event->data.ctlEnter.controlID)) {
						ctl_set_value(event->data.ctlEnter.controlID, false);
						selectedscreen = -1;
					} else {
						selectedscreen = event->data.ctlEnter.controlID;
					}
				default:
				        break;
			}
			break;
		default:
			break; 
	} 
	return handled; 
} 

void HandleEvents(EventPtr event);

void CheckEvents(void);
