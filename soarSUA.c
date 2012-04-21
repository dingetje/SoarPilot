#include <PalmOS.h>
#include "soaring.h"
#include "soarSUA.h"
#include "soarForm.h"
#include "soarDB.h"
#include "soarUtil.h"
#include "soarUMap.h"
#include "soarIO.h"
#include "Mathlib.h"
#include "soarMath.h"
#include "soarMem.h"
#include "soarWind.h"
#include "soarComp.h"

//#define CHKBALL	// check bounding ball in SUA warnings
//#define DRAW_BOUNDS	// draw the bounding box and ball for each SUA element
//#define DRAW_ARC	// draw ARC as using sector routine

SUAIndex	*suaidx;
SUAData		*suadata;
SUAAlertData	*suaalert;
SUAAlertRet	*suaalertret;
SUAIndex	*selectedSUA;

Int16 selectedSUAListIdx=0;
Int16 currentSUAPage=0;
Int16 numSUARecs=0;
Int16 suasortType = SUASortByDistA;
Int16 QNHnumdigits = 0;
double temppalt, tempgalt;
Boolean SUAselectall = false;
Boolean lookingahead = false;
Int16 *warnlist=NULL;
Boolean dispsuareadonly = false;
Char WarnString[40];
Int32 SUAallclasses;
Int32 SUAdrawclasses;
Int32 SUAwarnclasses;
UInt32 	warning_time = 0xffffffff;
Boolean allowgenalerttap = false;
double OpenAirfirstlat = INVALID_LAT_LON;
double OpenAirfirstlon = INVALID_LAT_LON;
double OpenAirlastlat = INVALID_LAT_LON;
double OpenAirlastlon = INVALID_LAT_LON;
double SUAdatalastlat = INVALID_LAT_LON;
double SUAdatalastlon = INVALID_LAT_LON;

extern Boolean inflight;
extern Int16 curformpriority;
extern Boolean recv_palt;
extern Boolean terrainvalid;
extern Boolean menuopen;
extern Boolean allowExit;
extern IndexedColorType indexSUA, indexSUAwarn, indexBlack, indexRed;
extern double xdist, ydistul, ydistll;
extern Boolean allowgenalerttap;
extern double mapmaxlat, mapminlat;
extern double mapmaxlon, mapminlon;
extern Int32  rxrecs;
extern char rxtype[15];
extern UInt32 origform; 
extern UInt8 mapmode;
extern Boolean draw_log;
extern UInt32 cursecs;
extern Char buf[PARSELEN];

void check_sua_elements()
{
	Boolean SUAWarned = false;
	UInt16 x;
	static UInt16 SUAWarn_count=0;
	double projlat, projlon, altadj;
	DateTimeType timenow;
	MemHandle suaidx_hand=NULL;
	MemHandle output_hand=NULL;
	MemPtr output_ptr=NULL;
	SUAIndex *tempsuaidx;
	UInt16 checkrecs;

	// only check when GPS active and warnings are on
	if ((data.config.CheckSUAWarn) && (data.input.SUAnumrecs > 0) && (inflight) && !draw_log) {
 //		HostTraceOutputTL(appErrorClass, "SUA Check %s",DblToStr(SUAWarn_count,0));					

		// check for SUA warnings  : 64 at one time
		if ((data.config.SUAlookahead > 0) && (data.input.SUAwarnrecs > 1)) { 
			checkrecs = data.input.SUAwarnrecs / 2; // checking a look ahead position as well doubles the number of checks
		} else {
			checkrecs = data.input.SUAwarnrecs;
		}

		SUAWarned = false;
		for (x = 0; x < checkrecs; x++) {
			SUAWarn_count++;
			if (SUAWarn_count >= data.input.SUAnumrecs) {
				SUAWarn_count = 0;
//				HostTraceOutputTL(appErrorClass, "Done all SUA %s",data.logger.gpsutc);						
//				PlaySound(784, 125, sndMaxAmp); // for debugging / testing only
			}

			// check current position
			lookingahead = false;
			SUAWarned = WarnSUA(SUAWarn_count, data.input.gpslatdbl, data.input.gpslngdbl, 
					data.input.inusealt, data.logger.pressalt, data.input.inusealt-data.input.terelev, 
					data.config.vert_dist, data.config.horiz_dist, data.config.SUArewarn);

			if ((data.config.SUAlookahead > 0) && (mapmode == CRUISE)) {
				// project glider position in cruise mode only, check if inside airspace (not near)
				RangeBearingToLatLonLinearInterp(data.input.gpslatdbl, data.input.gpslngdbl, 
					data.input.ground_speed.value * (data.config.SUAlookahead / 3600.0), data.input.true_track.value, 
					&projlat, &projlon);
				altadj = 0.0; // consider some altitude adjustment based on glide??
				lookingahead = true;
				SUAWarned = WarnSUA(SUAWarn_count, projlat, projlon, 
						data.input.inusealt-altadj, data.logger.pressalt-altadj, data.input.inusealt-data.input.terelev-altadj, 
						data.config.vert_dist, 0.0, 0.0);
			}
		}

		//used to automatically dismiss SUA warnings
		if ((suaalert->alerttype == SUAWARNING_ALERT) && (data.application.form_id == form_genalert) && (data.config.autodismisstime > 0)) {
				// automatically close alert after set delay
				if (cursecs > warning_time + data.config.autodismisstime) {
					// automatically dismiss the warning and close window
//					HostTraceOutputTL(appErrorClass, "SUA autodismiss");
					warning_time = 0xffffffff;
					HandleWaitDialogWin(0);
					suaalertret->valid = true;
					suaalertret->btnselected = 6;
					suaalertret->alerttype = SUAWARNING_ALERT;
					suaalertret->alertidxret = suaalert->alertidx;
					suaalertret->priority = suaalert->priority;
				}
		}

		// check for return from SUA alert
		if ((suaalert->alerttype == SUAWARNING_ALERT) && suaalertret->valid) {
// 			HostTraceOutputTL(appErrorClass, "Dismiss %hd", suaalertret->alertidxret);
	
			// one of the buttons pressed, need to update the suaidx record

			// open SUA index database
			suaidx_hand = MemHandleNew(sizeof(SUAIndex));
			tempsuaidx = MemHandleLock(suaidx_hand);
			OpenDBQueryRecord(suaidx_db, suaalertret->alertidxret, &output_hand, &output_ptr);
			MemMove(tempsuaidx, output_ptr, sizeof(SUAIndex));
			MemHandleUnlock(output_hand);

			// update last warned time
			tempsuaidx->Dismissed = true;	
//			HostTraceOutputTL(appErrorClass, "SUA Selected %hd",suaalertret->btnselected);
			switch (suaalertret->btnselected) {
				case 3: // 5 mins
//					HostTraceOutputTL(appErrorClass, "SUA 5 mins");
					tempsuaidx->LastWarned = cursecs + 300;
					if (tempsuaidx->WarnType == SUAWARN_APPROACHING) {
						tempsuaidx->ApproachLastWarned = tempsuaidx->LastWarned;		
					}
					tempsuaidx->WarnType = SUAWARN_SNOOZE; 
					break;
				case 4: // 1 hour
//					HostTraceOutputTL(appErrorClass, "SUA 1 hour");
					tempsuaidx->LastWarned = cursecs + 3600;
					if (tempsuaidx->WarnType == SUAWARN_APPROACHING) {
						tempsuaidx->ApproachLastWarned = tempsuaidx->LastWarned;		
					}
					tempsuaidx->WarnType = SUAWARN_SNOOZE;
					break;
				case 5: // Today (24 hrs or until SoarPilot starts again)
//					HostTraceOutputTL(appErrorClass, "SUA midnight");
					TimSecondsToDateTime(cursecs, &timenow);
					timenow.hour = 23;
					timenow.minute = 59;
					timenow.second = 59;
					tempsuaidx->LastWarned = TimDateTimeToSeconds(&timenow);
					//tempsuaidx->LastWarned = cursecs + (UInt32)3600*24;
					if (tempsuaidx->WarnType == SUAWARN_APPROACHING) {
						tempsuaidx->ApproachLastWarned = tempsuaidx->LastWarned;		
					}
					tempsuaidx->WarnType = SUAWARN_NONE;
					break;
				case 7:
//					HostTraceOutputTL(appErrorClass, "SUA immediate and popup SUA display");
					tempsuaidx->LastWarned = cursecs;
					if (tempsuaidx->WarnType == SUAWARN_APPROACHING) {
						tempsuaidx->ApproachLastWarned = cursecs + (1 - data.config.SUArewarn) * data.config.SUAlookahead;
					}
					selectedSUAListIdx = suaalertret->alertidxret;
					MemMove(selectedSUA, tempsuaidx, sizeof(SUAIndex));
					dispsuareadonly = true;
					FrmGotoForm(form_disp_sua);
					break;
				default: // immediately active again (new warning when more serious)
//					HostTraceOutputTL(appErrorClass, "SUA immediate");
					tempsuaidx->LastWarned = cursecs;
					if (tempsuaidx->WarnType == SUAWARN_APPROACHING) {
						tempsuaidx->ApproachLastWarned = cursecs + (1 - data.config.SUArewarn) * data.config.SUAlookahead;
					}
					break;
			}			

			// Update suaidx record
			OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), tempsuaidx, suaalertret->alertidxret);

			// free memory
			MemHandleUnlock(suaidx_hand);
			MemHandleFree(suaidx_hand); 

			// clear alert response
			suaalertret->alerttype = NULL_ALERT;
			suaalertret->alertidxret = -1;
			suaalertret->valid = false;
			suaalertret->btnselected = -1;
		}
	}
	return;
}

// Check a single SUA element for airspace warning
// Inputs:
// SUAelement			// which SUA element to check
// curlat, curlon		// position to check for airspace warnings
// curalt, curFL, curAGL	// current altitudes in MSL, FL and AGL
// vert_dist			// vertical warning distance in ft
// horiz_dist			// horizontal warning distance in nm
// SUArewarn			// % of vertical or horizontal distance to display an urgent warning
Boolean WarnSUA(UInt16 SUAelement, double curlat, double curlon, double curalt, 
		 double curFL, double curAGL, double vert_dist, double horiz_dist, double SUArewarn)
{
	Boolean SUAwarned;
	MemHandle suaidx_hand=NULL;
	MemHandle output_hand=NULL;
	MemPtr output_ptr=NULL;
	double neardist;
//	double coslat;
	UInt16 totalidxrecs=0;
	Int8 relalt=0;
	double width=0;
	Boolean chkbase, chktops; 
	Boolean rewarnalt, rewarnbase, rewarntops;
	double chkdist;
	double topsalt, basealt;

//	HostTraceOutputTL(appErrorClass, "Warn SUAelement=|%s|", DblToStr(SUAelement,0));	
//	HostTraceOutputTL(appErrorClass, "     Altitude  =|%s|", DblToStr(curalt,0));
//	HostTraceOutputTL(appErrorClass, "     Cur Lat   =|%s|", DblToStr(curlat,3));
//	HostTraceOutputTL(appErrorClass, "     Cur Lon   =|%s|", DblToStr(curlon,3));
//	HostTraceOutputTL(appErrorClass, "     Vert Dist =|%s|", DblToStr(vert_dist,1));
//	HostTraceOutputTL(appErrorClass, "    Horiz Dist =|%s|", DblToStr(horiz_dist,1));
//	HostTraceOutputTL(appErrorClass, "----------------");	

	SUAwarned = false;

	// catch if pressure altitude source defined but no data received
	if (!recv_palt && (data.config.pressaltsrc != GPSALT)) {
		curFL = curalt + data.input.QNHaltcorrect;
	}

	// check SUAelement number is valid
	totalidxrecs = OpenDBCountRecords(suaidx_db);
//	HostTraceOutputTL(appErrorClass, "    totalidxrecs =|%s|", DblToStr(totalidxrecs,0));
	if ((totalidxrecs > 0) && (SUAelement < totalidxrecs)) {

		// open SUA index database
		suaidx_hand = MemHandleNew(sizeof(SUAIndex));
		suaidx = MemHandleLock(suaidx_hand);
		OpenDBQueryRecord(suaidx_db, SUAelement, &output_hand, &output_ptr);
		MemMove(suaidx, output_ptr, sizeof(SUAIndex));
		MemHandleUnlock(output_hand);

//		HostTraceOutputTL(appErrorClass, "----------------");
//		HostTraceOutputTL(appErrorClass, "     Title     =|%s|", suaidx->title);
//		HostTraceOutputTL(appErrorClass, "     Element   =|%hd|", SUAelement);
//		HostTraceOutputTL(appErrorClass, "     WarnType  =|%hd|", suaidx->WarnType);
//		HostTraceOutputTL(appErrorClass, "     Type      =|%s|", DblToStr(suaidx->plottype,0));
//		HostTraceOutputTL(appErrorClass, "     Tops      =|%s|", DblToStr(suaidx->tops,0));
//		HostTraceOutputTL(appErrorClass, "     Base      =|%s|", DblToStr(suaidx->base,0));
//		HostTraceOutputTL(appErrorClass, "     Max Lat   =|%s|", DblToStr(suaidx->SUAmaxlat,3));
//		HostTraceOutputTL(appErrorClass, "     Min Lat   =|%s|", DblToStr(suaidx->SUAminlat,3));
//		HostTraceOutputTL(appErrorClass, "     Max Lon   =|%s|", DblToStr(suaidx->SUAmaxlon,3));	
//		HostTraceOutputTL(appErrorClass, "     Min Lon   =|%s|", DblToStr(suaidx->SUAminlon,3));
//		HostTraceOutputTL(appErrorClass, "     Dismissed =|%s|", DblToStr(suaidx->Dismissed,0));
//		HostTraceOutputTL(appErrorClass, "----------------");	

		// check warnings are active for this type and not deactivated for this element
		if (  (data.config.suawarntypes & suaidx->type) 
		   && (SUAwarnclasses & suaidx->class) 
		   && (suaidx->WarnActive)) {
//			HostTraceOutputTL(appErrorClass, "Active OK");

		// check altitude types for base and tops
		rewarnbase = false;
		chkbase = false;
//		HostTraceOutputTL(appErrorClass, "Base=|%s|", DblToStr(suaidx->basetype,0));
		switch (suaidx->basetype) {
			case SUA_ALT:
				basealt = curalt;
				break;
			case SUA_FL:
				basealt = curFL;
				break;
			case SUA_AGL:
			case SUA_SFC:
				basealt = curAGL;
				break;
			default:
				basealt = curalt;
				break;
		}
		if (basealt >= suaidx->base - vert_dist) {
			chkbase = true;
			if (basealt >= suaidx->base - (vert_dist * SUArewarn)) {
				rewarnbase = true;
			}
		}

//		HostTraceOutputTL(appErrorClass, "Tops=|%s|", DblToStr(suaidx->topstype,0));
		rewarntops = false;
		chktops = false;
		switch (suaidx->topstype) {
			case SUA_ALT:
				topsalt = curalt;
				break;
			case SUA_FL:
				topsalt = curFL;
				break;
			case SUA_AGL:
			case SUA_SFC:
				topsalt = curAGL;
				break;
			default:
				topsalt = curalt;
				break;
		}
		if (topsalt <= suaidx->tops + vert_dist) {
			chktops = true;
			if (topsalt <= suaidx->tops + (vert_dist * SUArewarn)) {
				rewarntops = true;
			}
		}


//		HostTraceOutputTL(appErrorClass, "Tops=|%s|", DblToStr(chktops,0));
//		HostTraceOutputTL(appErrorClass, "Base=|%s|", DblToStr(chkbase,0));
//		HostTraceOutputTL(appErrorClass, "rewarnalt=|%s|", DblToStr(rewarnalt,0));
//		HostTraceOutputTL(appErrorClass, "rewarn=|%s|", DblToStr(vert_dist * SUArewarn,0));

		rewarnalt = (rewarntops && rewarnbase);
		// check altitude is within tops and base +/- vert_dist
		if ((chkbase) && (chktops)) {
//			HostTraceOutputTL(appErrorClass, "Alt OK");

			// define relalt as above, below or in the airspace by altitude
			relalt = 0;
			if (basealt < suaidx->base) {
				relalt = -1; // below airspace
//				HostTraceOutputTL(appErrorClass, "Below");
			} else 	if (topsalt > suaidx->tops) {
				relalt = 1; // above airspace
//				HostTraceOutputTL(appErrorClass, "Above");
			}

			if (suaidx->plottype == SUAAWY) {
				// add half airway width to horiz_dist for checking bounding box
				width = suaidx->width / 2.0;
			} else {
				width = 0.0;
			}

			// check position bounding box + horiz_dist
			if     ((curlat <= suaidx->SUAmaxlat + (horiz_dist + width) / 60.0) &&
				(curlat >= suaidx->SUAminlat - (horiz_dist + width) / 60.0) &&
				(curlon <= suaidx->SUAmaxlon + (horiz_dist + width) / data.input.coslat / 60.0) &&
				(curlon >= suaidx->SUAminlon - (horiz_dist + width) / data.input.coslat / 60.0)) {
//				HostTraceOutputTL(appErrorClass, "Box OK");
#ifdef CHKBALL
				// check bounding ball + horiz_dist
				neardist = LatLonToRange2(curlat, curlon, suaidx->reflat, suaidx->reflon);
				if (neardist < (suaidx->maxdist + horiz_dist + width) * (suaidx->maxdist + horiz_dist + width)) {
// 					HostTraceOutputTL(appErrorClass, "Ball OK");
#endif
					// Inside altitude limits and both bounding box and ball so need to do detailed check
					// check INSIDE or NEAR for each type of element
// 					HostTraceOutputTL(appErrorClass, "Type: %s",DblToStr(suaidx->plottype,0));
					switch (suaidx->plottype) {
						case SUATFR: 
						case SUACIRCLE: 
#ifndef CHKBALL
							neardist = LatLonToRange2(curlat, curlon, suaidx->reflat, suaidx->reflon); 
#endif
							// check if INSIDE circle
							if (neardist <= (suaidx->maxdist * suaidx->maxdist)) { 
								// check for exiting airspace warnings
								if (suaidx->warnonexit && (relalt == 0) && (horiz_dist > 0)) {
									// check vertical for high or low
									rewarnbase = false;
									chkbase = false;
									if ((basealt <= suaidx->base + vert_dist) && (basealt >= suaidx->base)) {
										chkbase = true;
										if (basealt <= suaidx->base + (vert_dist * SUArewarn)) {
											rewarnbase = true;
										}
									}
									rewarntops = false;
									chktops = false;
									if ((topsalt >= suaidx->tops - vert_dist) && (topsalt <= suaidx->tops)) {
										chktops = true;
										if (topsalt >= suaidx->tops - (vert_dist * SUArewarn)) {
											rewarntops = true;
										}
									}
									rewarnalt = (rewarntops || rewarnbase);
									relalt = 0;
									if (basealt <= suaidx->base + vert_dist) {
										relalt = -1;
									} else if (topsalt >= suaidx->tops - vert_dist) {
										relalt = 1;
									}

									// check horizontal for edge
									chkdist = suaidx->maxdist - horiz_dist;
									if ((neardist >= chkdist * chkdist) || chkbase || chktops) {
										// near edge, top or bottom
										SUAwarned = true;
										// check if urgent edge warning
										chkdist = suaidx->maxdist - horiz_dist * SUArewarn;
										if (neardist >= chkdist * chkdist) {
											WarnSet(SUAelement, relalt, true, true, true);
										} else {
											chkdist = suaidx->maxdist - horiz_dist;
											WarnSet(SUAelement, relalt, (neardist >= chkdist * chkdist), rewarnalt, true);
										}

									} else { 
										// if not set to SUAURGENT_IN warning
										SUAwarned = true;
										WarnSet(SUAelement, relalt, true, true, false);
									}

								} else {
									// normal INSIDE warning
									SUAwarned = true;
									WarnSet(SUAelement, relalt, true, rewarnalt, false);
								}

							} else if (horiz_dist > 0) {
								// check if NEAR circle within horiz_dist
								chkdist = suaidx->maxdist + horiz_dist;
								if (neardist <= chkdist * chkdist) {
									SUAwarned = true;
									// check if urgent warning
									chkdist = suaidx->maxdist + horiz_dist * SUArewarn;
									if (neardist <= chkdist * chkdist) {
										WarnSet(SUAelement, relalt, false, rewarnalt, false);
									} else {
										WarnSet(SUAelement, relalt, false, false, false);
									}

								} else { // if not clear the warning
									WarnClear();
								}

							} else { // if not clear the warning
								WarnClear();
							}
							break;

						case SUAAWY:
							// check using checkdist function with width to get inside or distance
							chkdist = suaidx->width / 2.0; // note: for speed, exit checkdist function if less than this distance
							neardist = checkdist(curlat, curlon, suaidx->startidx, suaidx->stopidx, chkdist);
							if (neardist <= chkdist * chkdist) {
								// check for exiting airspace warnings
								if (suaidx->warnonexit && (relalt == 0) && (horiz_dist > 0)) {
									rewarnbase = false;
									chkbase = false;
									if ((basealt <= suaidx->base + vert_dist) && (basealt >= suaidx->base)) {
										chkbase = true;
										if (basealt <= suaidx->base + (vert_dist * SUArewarn)) {
											rewarnbase = true;
										}
									}
									rewarntops = false;
									chktops = false;
									if ((topsalt >= suaidx->tops - vert_dist) && (topsalt <= suaidx->tops)) {
										chktops = true;
										if (topsalt >= suaidx->tops - (vert_dist * SUArewarn)) {
											rewarntops = true;
										}
									}
									rewarnalt = (rewarntops || rewarnbase);
									relalt = 0;
									if (basealt <= suaidx->base + vert_dist) {
										relalt = -1;
									} else if (topsalt >= suaidx->tops - vert_dist) {
										relalt = 1;
									}

									// check horizontal for edge
									chkdist = suaidx->width / 2.0 - horiz_dist;
									if ((neardist >= chkdist * chkdist) || chkbase || chktops) {
										// near edge, top or bottom
										SUAwarned = true;
										// check if urgent edge warning
										chkdist = suaidx->width / 2.0 - horiz_dist * SUArewarn;
										if (neardist >= chkdist * chkdist) {
											WarnSet(SUAelement, relalt, true, true, true);
										} else {
											chkdist =  suaidx->width / 2.0 - horiz_dist;	
											WarnSet(SUAelement, relalt, (neardist >= chkdist * chkdist), rewarnalt, true);
										}

									} else { 
										// if not set to SUAURGENT_IN warning
										SUAwarned = true;
										WarnSet(SUAelement, relalt, true, true, false);
									}

								} else {
									// normal INSIDE warning
									SUAwarned = true;
									WarnSet(SUAelement, relalt, true, rewarnalt, false);
								}
	
							} else if (horiz_dist > 0) { 
								// check if near to airway
								chkdist = suaidx->width / 2.0 + horiz_dist;
								if (neardist <= chkdist * chkdist) {
									// near to airway
									SUAwarned = true;
									// check if urgent warning
									chkdist = suaidx->width / 2.0 + horiz_dist * SUArewarn;
									if (neardist <= chkdist * chkdist) {
										WarnSet(SUAelement, relalt, false, rewarnalt, false);
									} else {
										WarnSet(SUAelement, relalt, false, false, false);
									}

								} else { // if not clear warning
									WarnClear();
								}

							} else { // if not clear the warning
								WarnClear();
							}
							break;

						case SUAARC: // should never have SUAARC as first type but included here just in case
						case SUAPOINT:
//							HostTraceOutputTL(appErrorClass, "Check POLYGON");
							// check if INSIDE polygon
// 		 					HostTraceOutputTL(appErrorClass, "Check In/Out");
							if (checkinout(curlat, curlon, suaidx->startidx, suaidx->stopidx)) {
								// check for exiting airspace warnings
								if (suaidx->warnonexit && (relalt == 0) && (horiz_dist > 0)) {
									rewarnbase = false;
									chkbase = false;
									if ((basealt <= suaidx->base + vert_dist) && (basealt >= suaidx->base)) {
										chkbase = true;
										if (basealt <= suaidx->base + (vert_dist * SUArewarn)) {
											rewarnbase = true;
										}
									}
									rewarntops = false;
									chktops = false;
									if ((topsalt >= suaidx->tops - vert_dist) && (topsalt <= suaidx->tops)) {
										chktops = true;
										if (topsalt >= suaidx->tops - (vert_dist * SUArewarn)) {
											rewarntops = true;
										}
									}
									rewarnalt = (rewarntops || rewarnbase);
									relalt = 0;
									if (basealt <= suaidx->base + vert_dist) {
										relalt = -1;
									} else if (topsalt >= suaidx->tops - vert_dist) {
										relalt = 1;
									}

									// check horizontal for edge
									neardist = checkdist(curlat, curlon, suaidx->startidx, suaidx->stopidx, horiz_dist * SUArewarn);
									chkdist = horiz_dist;							
									if ((neardist <= chkdist * chkdist) || chkbase || chktops) {
										// near edge, top or bottom
										SUAwarned = true;
										// check if urgent edge warning
										chkdist = horiz_dist * SUArewarn;
										if (neardist <= chkdist * chkdist) {
											WarnSet(SUAelement, relalt, true, true, true);
										} else {
											chkdist = horiz_dist;	
											WarnSet(SUAelement, relalt, (neardist <= chkdist * chkdist), rewarnalt, true);
										}

									} else { 
										// if not set to SUAURGENT_IN warning
										SUAwarned = true;
										WarnSet(SUAelement, relalt, true, true, false);
									}

								} else {
									// normal INSIDE warning
									SUAwarned = true;
									WarnSet(SUAelement, relalt, true, rewarnalt, false);
								}

							} else if (horiz_dist > 0) { // check if NEAR polygon within horiz_dist
								// note: for speed, exit checkdist function if less than horiz_dist * SUArewarn
//	 		 					HostTraceOutputTL(appErrorClass, "Check Distance");
								neardist = checkdist(curlat, curlon, suaidx->startidx, suaidx->stopidx, horiz_dist * SUArewarn);

								// check if near within horiz_dist
								chkdist = horiz_dist;							
								if (neardist <= chkdist * chkdist) {
									SUAwarned = true;

									// check if urgent warning
									chkdist = horiz_dist * SUArewarn;
									if (neardist <= chkdist * chkdist) {
										WarnSet(SUAelement, relalt, false, rewarnalt, false);
									} else {
										WarnSet(SUAelement, relalt, false, false, false);
									}

								} else { // if not clear the warning
									WarnClear();
								}

							} else { // if not clear the warning
								WarnClear();
							}
							break;

						default:
							break;

					} // from switch statement
#ifdef CHKBALL
				} else { // from bounding ball check
					WarnClear();
				}
#endif
			} else { // from bounding box check
				WarnClear();
			}

		} else { // from altitude checks
			WarnClear();
		}

		} else { // from active and ignore checks
			WarnClear();
		}

		// Update suaidx record
		OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), suaidx, SUAelement);
	
	} // from totalidxrecs checks

	// free memory
	MemHandleUnlock(suaidx_hand);
	MemHandleFree(suaidx_hand); 

	return(SUAwarned);
}

void WarnSet(UInt16 SUAelement, Int16 relalt, Boolean inside, Boolean rewarn, Boolean warnonexit)
// Warn when near airspace
{
	Int16 NewWarnType;
	Char TypeString[11];
	Char ClassString[2];
	Char TTstr[15];
	Char BTstr[15];

	// find which warning to display
	if (lookingahead) {
		NewWarnType = SUAWARN_APPROACHING;
		rewarn = false;

	} else if (warnonexit) {
		// exiting airspace warnings
		if (relalt == -1) {
			// low warnings
			if (suaidx->base == 0) return;	// no need for low warnings when the base is at ground level!
			if (rewarn) {
				if (inside) {
					NewWarnType = SUAURGENT_LOW_EDGE;
				} else {
					NewWarnType = SUAURGENT_LOW;
				}
			} else {
				if (inside) {
					NewWarnType = SUAWARN_LOW_EDGE;
				} else {
					NewWarnType = SUAWARN_LOW;
				}
			}
		} else if (relalt == 1) {
			// high warnings
			if (rewarn) {
				if (inside) {
					NewWarnType = SUAURGENT_HIGH_EDGE;
				} else {
					NewWarnType = SUAURGENT_HIGH;
				}
			} else {
				if (inside) {
					NewWarnType = SUAWARN_HIGH_EDGE;
				} else {
					NewWarnType = SUAWARN_HIGH;
				}
			}
		} else {
			// edge warnings
			if (rewarn) {
				NewWarnType = SUAURGENT_EDGE;
			} else {
				NewWarnType = SUAWARN_EDGE;
			}
		}

	} else if (rewarn) {
		// URGENT warnings
		if (inside) {
			// Horizontally inside airspace
			if (relalt == -1) { // check if below
				NewWarnType = SUAURGENT_BELOW;
			} else 	if (relalt == 1) { // check if above
				NewWarnType = SUAURGENT_ABOVE;
			} else { // must be inside
				NewWarnType = SUAURGENT_IN;
			}
		} else {
			// near the airspace horizontally
			if (relalt == -1) { // check if below
				NewWarnType = SUAURGENT_NEAR_BELOW;
			} else if (relalt == 1) { // check if above
				NewWarnType = SUAURGENT_NEAR_ABOVE;
			} else { // must be near with vertical limits
	 			NewWarnType = SUAURGENT_NEAR;
			}
		}

	} else {	
		// normal warnings
		if (inside) {
			// Horizontally inside airspace
			if (relalt == -1) { // check if below
				NewWarnType = SUAWARN_BELOW;
			} else 	if (relalt == 1) { // check if above
				NewWarnType = SUAWARN_ABOVE;
			} else { // must be inside
				NewWarnType = SUAWARN_IN;
			}
		} else {
			// near the airspace horizontally
			if (relalt == -1) { // check if below
				NewWarnType = SUAWARN_NEAR_BELOW;
			} else if (relalt == 1) { // check if above
				NewWarnType = SUAWARN_NEAR_ABOVE;
			} else { // must be near with vertical limits
	 			NewWarnType = SUAWARN_NEAR;
			}
		}
	}

	// display if the warning is not dismissed or a different type of warning 
	if ((!suaidx->Dismissed) || (suaidx->WarnType != NewWarnType)) {
//		HostTraceOutputTL(appErrorClass, "Checking Warning.....");
//		HostTraceOutputTL(appErrorClass, "Suaidx   Type=|%s|",DblToStr(suaidx->WarnType,0));
//		HostTraceOutputTL(appErrorClass, "New Warn Type=|%s|",DblToStr(NewWarnType,0));
//		HostTraceOutputTL(appErrorClass, "Relalt=|%s|", DblToStr(relalt,0));
//		HostTraceOutputTL(appErrorClass, "Inside=|%s|", DblToStr(inside,0));
//		HostTraceOutputTL(appErrorClass, "Rewarn=|%s|", DblToStr(rewarn,0));
//		HostTraceOutputTL(appErrorClass, "WarnonExit=|%s|", DblToStr(warnonexit  ,0));

		// check for lower priority alerts and time since last warning not expired
		// for example when flying out of / away from the SUA item, or open alert form on screen
		if (((NewWarnType > suaidx->WarnType) || (suaidx->warnonexit && ((NewWarnType < SUAURGENT_IN) && (suaidx->WarnType >= SUAURGENT_IN)))) && (cursecs > suaidx->LastWarned)) {

			// play warning sound
			if (rewarn) {
				PlayNoGPSSound();
			} else {
				PlayWarnSound();
			}

			// higher or equal priority warning for same SUA item
//			HostTraceOutputTL(appErrorClass, "Higher Priority Alert");

			if (suaidx->warnonexit && (NewWarnType >= SUAURGENT_IN) && (suaidx->WarnType < SUAURGENT_IN)) {
				// entering airspace
				StrCopy(WarnString, "          ! ENTERING Airspace !");

			} else if (suaidx->warnonexit && (NewWarnType < SUAURGENT_IN) && (suaidx->WarnType >= SUAURGENT_IN)) {
				// leaving airspace
				StrCopy(WarnString, "           ! LEAVING Airspace !");

			} else {
				// Decide appropriate warning text
				switch (NewWarnType) {
					// approaching warnings
					case SUAWARN_APPROACHING:
						StrCopy(WarnString, "         Approaching Airspace  ");
						break;

					// normal warnings
					case SUAWARN_BELOW:
						StrCopy(WarnString, "               Below Airspace  ");
						break;
					case SUAWARN_ABOVE:
						StrCopy(WarnString, "               Above Airspace  ");
						break;
					case SUAWARN_IN:
						StrCopy(WarnString, "              INSIDE Airspace  ");
						break;
					case SUAWARN_NEAR_BELOW:
						StrCopy(WarnString, "          Below Near Airspace  ");
						break;
					case SUAWARN_NEAR_ABOVE:
						StrCopy(WarnString, "          Above Near Airspace  ");
						break;
					case SUAWARN_NEAR:
						StrCopy(WarnString, "                Near Airspace  ");
						break;

					// URGENT warnings
					case SUAURGENT_BELOW:
						StrCopy(WarnString, "             ! Below Airspace !");
						break;
					case SUAURGENT_ABOVE:
						StrCopy(WarnString, "             ! Above Airspace !");
						break;
					case SUAURGENT_IN:
						StrCopy(WarnString, "            ! INSIDE Airspace !");
						break;
					case SUAURGENT_NEAR_BELOW:
						StrCopy(WarnString, "        ! Below Near Airspace !");
						break;
					case SUAURGENT_NEAR_ABOVE:
						StrCopy(WarnString, "        ! Above Near Airspace !");
						break;
					case SUAURGENT_NEAR:
						StrCopy(WarnString, "              ! Near Airspace !");
						break;

					// exiting airspace warnings
					case SUAWARN_EDGE:
						StrCopy(WarnString, "           Near Airspace Edge  ");
						break;
					case SUAWARN_LOW:
						StrCopy(WarnString, "         Near Airspace Bottom  ");
						break;
					case SUAWARN_LOW_EDGE:
						StrCopy(WarnString, "  Near Airspace Bottom Edge    ");
						break;
					case SUAWARN_HIGH:
						StrCopy(WarnString, "            Near Airspace Top  ");
						break;
					case SUAWARN_HIGH_EDGE:
						StrCopy(WarnString, "       Near Airspace Top Edge  ");
						break;
					case SUAURGENT_EDGE:
						StrCopy(WarnString, "         ! Near Airspace Edge !");
						break;
					case SUAURGENT_LOW:
						StrCopy(WarnString, "       ! Near Airspace Bottom !");
						break;
					case SUAURGENT_LOW_EDGE:
						StrCopy(WarnString, "!Near Airspace Bottom Edge!    ");
						break;
					case SUAURGENT_HIGH:
						StrCopy(WarnString, "          ! Near Airspace Top !");
						break;
					case SUAURGENT_HIGH_EDGE:
						StrCopy(WarnString, "     ! Near Airspace Top Edge !");
						break;

					default:
						StrCopy(WarnString, "            ! UNKNOWN Warning !");
						break;
				}
			}
//			HostTraceOutputTL(appErrorClass, "Warning=|%s|",WarnString);

			// update latest warn type
			suaidx->WarnType = NewWarnType;

			// Convert the current type to a string
			// 11 is the maximum length of the resulting string including the null terminator
			// false means to use the long form of the string for output
			GetSUATypeStr(TypeString, suaidx->type, 11, false);		
			GetSUAClassStr(ClassString, suaidx->class);		

			// decode tops and base types to text
			GetSUAALTTypeStr(TTstr, suaidx->topstype, suaidx->tops, suaidx->topsunits, 15);
			GetSUAALTTypeStr(BTstr, suaidx->basetype, suaidx->base, suaidx->baseunits, 15);

//			HostTraceOutputTL(appErrorClass, "Type Tops: %s",TTstr);
//			HostTraceOutputTL(appErrorClass, "Type Tops: %s",DblToStr(suaidx->topstype,0));
//			HostTraceOutputTL(appErrorClass, "Type Base: %s",BTstr);
//			HostTraceOutputTL(appErrorClass, "Type Base: %s",DblToStr(suaidx->basetype,0));

			// Alert Pilot
//			HostTraceOutputTL(appErrorClass, "Title: %s",suaidx->title);	
//			HostTraceOutputTL(appErrorClass, "Type:  %s",TypeString);
//			HostTraceOutputTL(appErrorClass, "Tops:  %s",DblToStr(suaidx->tops,0));
//			HostTraceOutputTL(appErrorClass, "Base:  %s",DblToStr(suaidx->base,0));

			// popup warning only if greater priority than any current alert on screen
			if (NewWarnType >= curformpriority) {

				// Build warning message
				// log time warned
				warning_time = cursecs;
								
				// use warning type as priority for genalert form
				suaalert->alerttype = SUAWARNING_ALERT;
				suaalert->priority = suaidx->WarnType;

				// Buttons
				suaalert->numbtns = 3;
//				StrCopy(suaalert->btn0text, "DISMISS");
				StrCopy(suaalert->btn0text, "5 Mins");
				StrCopy(suaalert->btn1text, "1 Hour");
				StrCopy(suaalert->btn2text, "Today");

				// Text
				//StrCopy(suaalert->displaytext, trim(WarnString, ' ', true));		
				StrCopy(suaalert->displaytext, WarnString);		

				StrCat(suaalert->displaytext, "\n");
				StrCat(suaalert->displaytext, "\n");
				StrCat(suaalert->displaytext, suaidx->title);

				StrCat(suaalert->displaytext, "\nClass:       ");
				if (StrCompare(ClassString, " ") != 0) {
					StrCat(suaalert->displaytext, ClassString);
					StrCat(suaalert->displaytext, " / ");
				}
				StrCat(suaalert->displaytext, TypeString);

				StrCat(suaalert->displaytext, "\nTops:        ");
				StrCat(suaalert->displaytext, TTstr);
	
				StrCat(suaalert->displaytext, "\nBase:         ");
				StrCat(suaalert->displaytext, BTstr);

				StrCat(suaalert->displaytext, "\nRadio:       ");
				StrCat(suaalert->displaytext, suaidx->radiofreq);

				// store SUA element idx with warning
				suaalert->alertidx = SUAelement;

				// I put this here for now.  It's also inside of the 
				// window event frmOpen are for the alert window.
				// This is to allow you to tell when there is a response
				// that needs to be processed.
				// Would need to handle any return/response from the
				// Alert window prior to this.
				suaalertret->valid = false;
				suaalertret->btnselected = -1;
		
				// decide if warning is urgent or not.
				allowgenalerttap = true; // allow tapping anywhere in the alert window to dimiss
				if (rewarn) {
//					HostTraceOutputTL(appErrorClass, "URGENT: %hd", suaalert->alertidx);
//					HostTraceOutputTL(appErrorClass, "URGENT: %hd", suaidx->WarnType);
//					PlayNoGPSSound();
					StrCopy(suaalert->title, "URGENT Airspace Alert");
//					StrCopy(suaalert->title, trim(WarnString, ' ', true));
					HandleWaitDialogWin(1);
				} else {	
//					HostTraceOutputTL(appErrorClass, "Warning: %hd", suaalert->alertidx);
//					HostTraceOutputTL(appErrorClass, "Warning: %hd", suaidx->WarnType);
//					PlayWarnSound();
					StrCopy(suaalert->title, "Airspace Warning");
//					StrCopy(suaalert->title, trim(WarnString, ' ', true));
					HandleWaitDialogWin(1);
				}
			}
			// stops repeat warnings on the same SUA item
			suaidx->Dismissed = true; 

		} else { // low priority, don't display a warning
//			HostTraceOutputTL(appErrorClass, "Lower Priority Alert");

			// still update latest warn type if not an approach warning
			if ((NewWarnType != SUAWARN_APPROACHING) && (cursecs > suaidx->LastWarned)) {
				suaidx->WarnType = NewWarnType;
				suaidx->Dismissed = true;	
			}
		}		
	}

	return;
}

void WarnClear()
// clear the warning from an suaidx
{	
	// clear if it's not an approaching warning or it is an approach warning and the time has expired
	if (!lookingahead) 
		if ((suaidx->WarnType > SUAWARN_APPROACHING)
			|| ((suaidx->WarnType == SUAWARN_APPROACHING) && (cursecs > suaidx->ApproachLastWarned)) ) {

		// clear warnings
//		HostTraceOutputTL(appErrorClass, "Clear Warning");
		suaidx->WarnType = SUAWARN_NONE;
		suaidx->Dismissed = false;
	}
	return;
}

// Check all SUA elements for airspace warning
// NOT USED CURRENTLY
void WarnAllSUA(double curlat, double curlon, double curalt, double curFL, double curAGL, double vert_dist, double horiz_dist, double SUArewarn)
{
	UInt16 x;
	MemHandle suaidx_hand;
	MemHandle output_hand;
	MemPtr output_ptr;
	UInt16 totalidxrecs=0;
	UInt16 totaldatarecs=0;
	Boolean SUAwarned;

//	HostTraceOutputTL(appErrorClass, "-------------------------");
//	HostTraceOutputTL(appErrorClass, "CHECKING FOR SUA WARNINGS - %s ft",DblToStr(data.input.inusealt,0));	
//	HostTraceOutputTL(appErrorClass, "%s ft",DblToStr(vert_dist,0));	
//	HostTraceOutputTL(appErrorClass, "%s nm",DblToStr(horiz_dist,0));	

	SUAwarned = false;
	totalidxrecs = OpenDBCountRecords(suaidx_db);
	if (totalidxrecs > 0) {
		suaidx_hand = MemHandleNew(sizeof(SUAIndex));
		suaidx = MemHandleLock(suaidx_hand);

		// Loop for each SUA element
		for (x=0; x < totalidxrecs; x++) {
			OpenDBQueryRecord(suaidx_db, x, &output_hand, &output_ptr);
			MemMove(suaidx, output_ptr, sizeof(SUAIndex));
			MemHandleUnlock(output_hand);
			totaldatarecs = (suaidx->stopidx - suaidx->startidx)+1;
			if (totaldatarecs > 0) {
				SUAwarned = WarnSUA(x, curlat, curlon, curalt, curFL, curAGL, vert_dist, horiz_dist, SUArewarn);
			}
		}

		// free memory
		MemHandleUnlock(suaidx_hand);
		MemHandleFree(suaidx_hand);
	}	

//	HostTraceOutputTL(appErrorClass, "DONE");
	return;
}

// used to process the SUA data after parsing setting defaults
void ProcessSUA(Boolean afterparsing, Boolean reset)
{
	UInt16 x,y;
	MemHandle suaidx_hand, suadata_hand;
	MemHandle output_hand, doutput_hand;
	MemPtr output_ptr, doutput_ptr;
	Int16 ErrorCount = 0;
	Int16 TotalErrorCount = 0;
	double firstlat = INVALID_LAT_LON, firstlon = INVALID_LAT_LON;
//	Char tempchar[15];

	// free memory from warning list if one exists
	if (warnlist != NULL) {
		FreeMem((void *)&warnlist);
		warnlist = NULL;
	}
	if (reset) return;

	// setup SUA actice classes
	SUAdrawclasses = SUANONE;
	if (data.config.suaactivetypes & CLASSA) SUAdrawclasses |= CLASSA;
	if (data.config.suaactivetypes & CLASSB) SUAdrawclasses |= CLASSB;
	if (data.config.suaactivetypes & CLASSC) SUAdrawclasses |= CLASSC;
	if (data.config.suaactivetypes & CLASSD) SUAdrawclasses |= CLASSD;
	if (data.config.suaactivetypes & CLASSE) SUAdrawclasses |= CLASSE;
	if (data.config.suaactivetypes & CLASSF) SUAdrawclasses |= CLASSF;
	if (data.config.suaactivetypes & CLASSG) SUAdrawclasses |= CLASSG;
	SUAdrawclasses |= CLASSX;

	SUAwarnclasses = SUANONE;
	if (data.config.suawarntypes & CLASSA) SUAwarnclasses |= CLASSA;
	if (data.config.suawarntypes & CLASSB) SUAwarnclasses |= CLASSB;
	if (data.config.suawarntypes & CLASSC) SUAwarnclasses |= CLASSC;
	if (data.config.suawarntypes & CLASSD) SUAwarnclasses |= CLASSD;
	if (data.config.suawarntypes & CLASSE) SUAwarnclasses |= CLASSE;
	if (data.config.suawarntypes & CLASSF) SUAwarnclasses |= CLASSF;
	if (data.config.suawarntypes & CLASSG) SUAwarnclasses |= CLASSG;
	SUAwarnclasses |= CLASSX;

	SUAallclasses = SUANONE | CLASSA | CLASSB | CLASSC | CLASSD | CLASSE | CLASSF | CLASSG | CLASSX;	

	// store number of SUA idx records, and how many to check each nil event
	data.input.SUAnumrecs = OpenDBCountRecords(suaidx_db);

	if (data.input.SUAnumrecs > 0) {
		// allocate memory for list of SUA elements with warnings
		AllocMem((void *)&warnlist, sizeof(Int16) * data.input.SUAnumrecs);
	}

	if (data.input.SUAnumrecs > 64 ) {
		data.input.SUAwarnrecs = 64;
	} else {
		data.input.SUAwarnrecs = data.input.SUAnumrecs;
	}

	// reset alert return
	suaalertret->valid = false;
	suaalertret->alerttype = NULL_ALERT;
	suaalertret->alertidxret = -1;
	suaalertret->btnselected = -1;

#ifdef SUALOG
	if (!afterparsing) {
		// open file for debugging information
		if (device.VFSCapable) XferInit("sualog.txt", IOOPENTRUNC, USEVFS);
		outputlog("Start SUA Check", true);
	}
#endif

	// run though all SUA element and reset warnings etc
	if (data.input.SUAnumrecs > 0) {
//		HostTraceOutputTL(appErrorClass, "Check SUA Data.....");
		suaidx_hand = MemHandleNew(sizeof(SUAIndex));
		suaidx = MemHandleLock(suaidx_hand);
		suadata_hand = MemHandleNew(sizeof(SUAData));
		suadata = MemHandleLock(suadata_hand);
		TotalErrorCount = 0;

		// Loop for each SUA element
		for (x=0; x < data.input.SUAnumrecs; x++) {
			OpenDBQueryRecord(suaidx_db, x, &output_hand, &output_ptr);
			MemMove(suaidx, output_ptr, sizeof(SUAIndex));
			MemHandleUnlock(output_hand);
			ErrorCount = 0;
#ifdef SUALOG
	if (!afterparsing) {
		outputlog(suaidx->title, true);
	}
#endif

			// Reset required data items
			WarnClear();
			suaidx->LastWarned = 0;
			suaidx->ApproachLastWarned = 0;
			suaidx->WarnType = SUAWARN_NONE;
			// set nearest distance
			suaidx->neardist = disttoSUAitem(suaidx);
				
			// check for not TFR type
			if (suaidx->type != TFR) {

				// check each data point (expect manually entered TFR's)
				for (y=suaidx->startidx ;y<=suaidx->stopidx; y++) {
					OpenDBQueryRecord(suadata_db, y, &doutput_hand, &doutput_ptr);
					MemMove(suadata, doutput_ptr, sizeof(SUAData));
					MemHandleUnlock(doutput_hand);

#ifdef SUALOG
	if (!afterparsing) {
		outputlog(DblToStr(y-suaidx->startidx+1,0), false);
		outputlog(" Type: ", false);
		outputlog(DblToStr(suadata->plottype,0), false);
		outputlog(" N: ", false);
		outputlog(DblToStr(suadata->n,0), false);
		outputlog(" Rad: ", false);
		outputlog(DblToStr(suadata->radius,3), false);
		outputlog(" L: ", false);
		outputlog(DblToStr(suadata->leftbrg,0), false);
		outputlog(" R: ", false);
		outputlog(DblToStr(suadata->rightbrg,0), false);
		outputlog(" LAT: ", false);
		outputlog(DblToStr(suadata->lat,3), false);
		outputlog(" LON: ", false);
		outputlog(DblToStr(suadata->lon,3), false);
	}
#endif
					// store first point
					if (y == suaidx->startidx) {
						firstlat = suadata->lat;
						firstlon = suadata->lon;
					}

					// check last point for closed SUA item (except airways)
					if ((y == suaidx->stopidx) && !(suaidx->type & AIRWAYS) && (suaidx->plottype != SUAAWY)) {
						if ((firstlat != suadata->lat) || (firstlon != suadata->lon)) {
							if (afterparsing && (data.config.SUAformat == SUAOPENAIR)) {
								// Close SUA polygon if required (only for openair files)
//								HostTraceOutputTL(appErrorClass, "Check Last Point of Last Item");
//								LLToStringDMS(OpenAirlastlat, tempchar, ISLAT);
//								HostTraceOutputT(appErrorClass, "Last Point  = |%s|", tempchar);
//								LLToStringDMS(OpenAirlastlon, tempchar, ISLON);
//								HostTraceOutputTL(appErrorClass, " |%s|", tempchar);
//								LLToStringDMS(firstlat, tempchar, ISLAT);
//								HostTraceOutputT(appErrorClass, "New Point  = |%s|", tempchar);
//								LLToStringDMS(firstlon, tempchar, ISLON);
//								HostTraceOutputTL(appErrorClass, " |%s|", tempchar);
								suadata->plottype = SUAPOINT;
								suadata->radius = 0.0;
								suadata->lat = firstlat;
								suadata->lon = firstlon;
								SaveSUAData(false);
							} else {
								// SUA polygon is not closed
//								HostTraceOutputTL(appErrorClass, "First != Last: %s",suaidx->title);
//								LLToStringDMS(firstlat, tempchar, ISLAT);
//								HostTraceOutputTL(appErrorClass, "1st  Lat: %s",tempchar);
//								LLToStringDMS(suadata->lat, tempchar, ISLAT);
//								HostTraceOutputTL(appErrorClass, "Last Lat: %s",tempchar);
//								LLToStringDMS(firstlon, tempchar, ISLON);
//								HostTraceOutputTL(appErrorClass, "1st  Lon: %s",tempchar);
//								LLToStringDMS(suadata->lon, tempchar, ISLON);
//								HostTraceOutputTL(appErrorClass, "1st  Lon: %s",tempchar);
								ErrorCount++;
								TotalErrorCount++;
								// record error type
								suaidx->WarnType |= SUAfirstlasterror;
//								HostTraceOutputTL(appErrorClass, "Last Point Error: %s", suaidx->title);
#ifdef SUALOG
	if (!afterparsing) {
		outputlog(" : First/Last Error", false);
	}
#endif
							}
						}
					}

					// check lat/lon for valid values
					if ((Fabs(suadata->lat) > 90.0) || (Fabs(suadata->lon) > 180.0)) {
//						HostTraceOutputTL(appErrorClass, "Invalid Lat/Lon: %s",suaidx->title);
//						LLToStringDMS(suadata->lat, tempchar, ISLAT);
//						HostTraceOutputTL(appErrorClass, "Invalid Lat: %s",tempchar);
//						LLToStringDMS(suadata->lon, tempchar, ISLON);
//						HostTraceOutputTL(appErrorClass, "Invalid Lon: %s",tempchar);
						ErrorCount++;
						TotalErrorCount++;
						// record error type
						suaidx->WarnType |= SUAlatlonerror;

#ifdef SUALOG
	if (!afterparsing) {
		outputlog(" : Lat/Lon Error", false);
	}
#endif
					}

					// check tops and base for valid values
					if (suaidx->base > suaidx->tops) {
						ErrorCount++;
						TotalErrorCount++;
						// record error type
						suaidx->WarnType |= SUAtopsbaseerror;
#ifdef SUALOG
	if (!afterparsing) {
		outputlog(" : Tops/Base Error", false);
	}
#endif
					}
#ifdef SUALOG
	if (!afterparsing) {
		outputlog(" ", true);
	}
#endif
				}

				// update record
				if (ErrorCount > 0) {
					suaidx->type = SUAERROR;
					suaidx->WarnActive = false;
					suaidx->active = false;
				} else if (afterparsing) {
					// reset warn active flag
					suaidx->WarnActive = true;
					// use fastball to update bounding ball
					if ((suaidx->plottype != SUACIRCLE) && (suaidx->plottype != SUATFR)) {
						FastBall();
					}
				}

			} // from TFR type check

			OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), suaidx, x);
		
		}

		// free memory
		MemHandleUnlock(suadata_hand);
		MemHandleFree(suadata_hand);
		MemHandleUnlock(suaidx_hand);
		MemHandleFree(suaidx_hand);

		if (afterparsing && (TotalErrorCount > 0)) {
			// alert if errors found
			FrmCustomAlert(WarningAlert, "SUA Data Errors!\n","See SUA List for details"," ");
			suasortType = SUASortByTypeA;
		}		

	}	

#ifdef SUALOG
	if (!afterparsing) {
		outputlog("End SUA Check", true);
		if (device.VFSCapable) XferClose(USEVFS);
	}
#endif

	return;
}

void DrawSUAElement(SUAIndex *suaidx, double gliderLat, double gliderLon, double xratio, double yratio, double ulRng, Boolean highlight, UInt8 offset)
{
	Int16 y;
	Int16 plotX=0, plotY=0;
	Int16 prevplotX=0, prevplotY=0;
	Int16 prevtype=SUAPOINT;
	double poirange, poibearing;
	Boolean plotvalstat=false, prevplotvalstat=false;
	Boolean firstpoint = true;
	UInt16 totaldatarecs=0;
	MemHandle suadata_hand;
	MemHandle output_hand;
	MemPtr output_ptr;
	double curalt;
	double boldline = 1.0;
//	double prevlat=INVALID_LAT_LON, prevlon=INVALID_LAT_LON;
	
#ifdef DRAW_BOUNDS
	double temppoirange, temppoibearing;
	RectangleType Bounding_Rect;
#endif

	// check for bold line
	if (device.HiDensityScrPresent && data.config.BoldSUA) {
		WinSetCoordinateSystem(kCoordinatesStandard);
		boldline = SCREEN.SRES;
	}

	// allocate memory	
	suadata_hand = MemHandleNew(sizeof(SUAData));
	suadata = MemHandleLock(suadata_hand);

//	HostTraceOutputTL(appErrorClass, "suaidx->title = |%s|", suaidx->title);
//	HostTraceOutputTL(appErrorClass, "==============================================");
//	HostTraceOutputTL(appErrorClass, "Inside for loop1-totaldatarecs=|%hu|", totaldatarecs);
//	HostTraceOutputTL(appErrorClass, "     data.config.suamaxalt=|%s|", DblToStr(data.config.SUAmaxalt, 1));
//	HostTraceOutputTL(appErrorClass, "     name=|%s|", suaidx->title);
//	HostTraceOutputTL(appErrorClass, "     base=|%s|", DblToStr(suaidx->base, 1));
//	HostTraceOutputTL(appErrorClass, "suaidx->maxdist=|%s|", DblToStr(suaidx->maxdist, 1));
//	HostTraceOutputTL(appErrorClass, "    ulRng=|%s|", DblToStr(ulRng, 1));

	// Check screen bounds : using max and min lat/lon of SUA item 
	// and max and min lat/lon of screen
	if ( (mapmaxlat > suaidx->SUAminlat) &&
	     (mapminlat < suaidx->SUAmaxlat) &&
	     (mapmaxlon > suaidx->SUAminlon) &&
	     (mapminlon < suaidx->SUAmaxlon) ) {

	// calculate number of points in SUA item and get altitude reference
	totaldatarecs = (suaidx->stopidx - suaidx->startidx)+1;
	if (suaidx->basetype == SUA_FL) {
		curalt = data.logger.pressalt;
	} else {
		curalt = data.logger.gpsalt;
	}

	// Check all other criteria for display
	if (  suaidx->active 
	   && (data.config.suaactivetypes & suaidx->type)
	   && (SUAdrawclasses & suaidx->class) 
	   && (suaidx->base < data.config.SUAmaxalt) 
	   && (totaldatarecs > 0) 
	   && (curalt >= suaidx->base - data.config.SUAdispalt) 
	   && (curalt <= suaidx->tops + data.config.SUAdispalt) ) {

//		HostTraceOutputTL(appErrorClass, "Inside if to plot-x=|%hu|", x);
		firstpoint = true;

		if ((suaidx->plottype == SUATFR) || (suaidx->plottype == SUACIRCLE)) {
			// draw circle using TFR / CIRCLE data in SUA index record (rather than SUA data record)
			plotvalstat = CalcPlotValues(gliderLat, gliderLon,  suaidx->reflat, suaidx->reflon, xratio, yratio, &plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
//			HostTraceOutputTL(appErrorClass, "Inside SUATFR / CIRCLE");
			if (highlight) {
//				HostTraceOutputTL(appErrorClass, "Plotting Highlighted %s", DblToStr(x,0)); 
				if (offset & 1) plotX++;
				if (offset & 2) plotY++;
				// set highlight colour for SUA 
				if (device.colorCapable) {
					WinSetForeColor(indexSUAwarn);
				}
				// set as bold
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(kCoordinatesStandard);
					boldline = SCREEN.SRES;
				}
			} else {
				// set normal colour for SUA 
				if (device.colorCapable) {
					WinSetForeColor(indexSUA);
				}
			}
			//MFH Need to make WinDrawCircle accept radius values in Nautical Miles
			if (plotvalstat && (poirange <= (suaidx->maxdist + ulRng))) {
				WinDrawCircle(plotX/boldline, plotY/boldline, (Int32)(suaidx->maxdist*xratio)/boldline, SOLID);
			}

		} else for (y=suaidx->startidx; y<(suaidx->stopidx+1); y++) { 
			// draw polygons from SUA data records
			OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
			MemMove(suadata, output_ptr, sizeof(SUAData));
			MemHandleUnlock(output_hand);

			// determine if point is on the screen
			plotvalstat = CalcPlotValues(gliderLat, gliderLon,  suadata->lat, suadata->lon,	xratio, yratio, &plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
//			HostTraceOutputT(appErrorClass, "PlotX %s", DblToStr(plotX,0));
//			HostTraceOutputTL(appErrorClass, " : PlotY %s", DblToStr(plotY,0));

			if (highlight) {
//				HostTraceOutputTL(appErrorClass, "Plotting Highlighted %s", DblToStr(x,0)); 
				if (offset & 1) plotX++;
				if (offset & 2) plotY++;
				// set highlight colour for SUA 
				if (device.colorCapable) {
					WinSetForeColor(indexSUAwarn);
				}
				// set as bold
				if (device.HiDensityScrPresent) {
					WinSetCoordinateSystem(kCoordinatesStandard);
					boldline = SCREEN.SRES;
				}
			} else {
				// set normal colour for SUA 
				if (device.colorCapable) {
					WinSetForeColor(indexSUA);
				}
			}
	
//			HostTraceOutputTL(appErrorClass, "plottype=|%hd|", suadata->plottype);
			switch (suadata->plottype) {
				case SUAAWY:
				case SUAPOINT:
//					HostTraceOutputTL(appErrorClass, "Inside SUAAWY/SUAPOINT");
					// Allows for airway and point items to be back-to-back
					if (suadata->plottype == SUAPOINT) { 
						if (prevtype == SUAAWY) {
							firstpoint = true;
						}
						prevtype = SUAPOINT;
					} else {
						if (prevtype == SUAPOINT) {
							firstpoint = true;
						}
						prevtype = SUAAWY;
					}
					// Draw the connecting line between the current and previous points
					if (firstpoint) {
						firstpoint = false;
					} else if (plotvalstat && prevplotvalstat) {
						if (suadata->plottype == SUAPOINT) {
							// SUAPOINTS will be output with a solid line
							WinDrawLine(plotX/boldline, plotY/boldline, prevplotX/boldline, prevplotY/boldline);
						} else {
							// SUAAWY's will be output with a dashed line
							WinDrawGrayLine(plotX/boldline, plotY/boldline, prevplotX/boldline, prevplotY/boldline);
						}
					}
					prevplotX = plotX;
					prevplotY = plotY;
					prevplotvalstat = plotvalstat;
					break;
				case SUAARC:
//					HostTraceOutputTL(appErrorClass, "Inside SUAARC");
#ifdef DRAW_ARC
					// draw ARC using WinDrawSector
					if (!firstpoint) {
//						HostTraceOutputTL(appErrorClass, "firstpoint = false");
						if (plotvalstat && (poirange <= (suadata->radius + ulRng))) {
//							HostTraceOutputTL(appErrorClass, "inside if for plotting arc");
							// Expects bearing to be magnetic
							// tolat = left bearing
							// tolon = right bearing
							WinDrawSector(xratio, yratio, plotX/boldline, plotY/boldline, 0.0, suadata->radius/boldline,
									(Int16)nice_brg(suadata->leftbrg+data.input.deviation.value),
									(Int16)nice_brg(suadata->rightbrg+data.input.deviation.value),
									NOENDS);
						}
					}
					firstpoint = true;
					// Used WinDrawSector so skip decomposed ARC points
					y = y + (Int16)suadata->n;
#else
					// skip ARC record and draw as decomposed ARC points
#endif
					break;
				default:
					break;
			}
		}
	} // criteria check
	} // screen bounds check

	// free memory
	MemHandleUnlock(suadata_hand);
	MemHandleFree(suadata_hand);

	// reset colour to black and normal lines
	if (device.colorCapable) {
		WinSetForeColor(indexBlack);
	}
	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(device.HiDensityScrPresent);
	}

	return;
}

void DrawSUA(double gliderLat, double gliderLon, double xratio, double yratio, double ulRng)
{
	Int16 x;
	UInt16 totalidxrecs=0;
	MemHandle suaidx_hand;
	MemHandle output_hand;
	MemPtr output_ptr;
	SUAIndex *suaidx;
	Int16 numwarn=0;

//	HostTraceOutputTL(appErrorClass, "Inside DrawSUA");

	totalidxrecs = OpenDBCountRecords(suaidx_db);

	if (totalidxrecs > 0) {

		// allocate memory
		suaidx_hand = MemHandleNew(sizeof(SUAIndex));
		suaidx = MemHandleLock(suaidx_hand);

		// first loop through all SUA items
		for (x=0; x < (Int16)totalidxrecs; x++) {
			OpenDBQueryRecord(suaidx_db, x, &output_hand, &output_ptr);
			MemMove(suaidx, output_ptr, sizeof(SUAIndex));
			MemHandleUnlock(output_hand);

			if (!data.config.SUAdisponlywarned && (suaidx->WarnType == SUAWARN_NONE)) {	
				// draw the SUA element normally
				DrawSUAElement(suaidx, gliderLat, gliderLon, xratio, yratio, ulRng, false, 0);
			} else {
				// push onto list of elements with a warning
				// to be drawn in the second loop
				if (suaidx->WarnType > SUAWARN_NONE) {
					warnlist[numwarn] = x;
					numwarn++;
				}
			}
		}

		// second loop to draw SUA elements with a warning
		for (x=0; x < numwarn; x++) {
			// draw the SUA element highlighted
			OpenDBQueryRecord(suaidx_db, warnlist[x], &output_hand, &output_ptr);
			MemMove(suaidx, output_ptr, sizeof(SUAIndex));
			MemHandleUnlock(output_hand);
			if (device.HiDensityScrPresent || data.config.SUAdisponlywarned) {
				DrawSUAElement(suaidx, gliderLat, gliderLon, xratio, yratio, ulRng, data.config.SUAhighlightwarned, 0);
			} else {
				// draw twice to simulate bold lines on lo-res palm
				DrawSUAElement(suaidx, gliderLat, gliderLon, xratio, yratio, ulRng, data.config.SUAhighlightwarned, 1);
				DrawSUAElement(suaidx, gliderLat, gliderLon, xratio, yratio, ulRng, data.config.SUAhighlightwarned, 2);
			}
		}
		
		// free memory
		MemHandleUnlock(suaidx_hand);
		MemHandleFree(suaidx_hand);
	}

	return;
}

void SUA_parser_tnp(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	UInt16 strLen = 0;
	static Int16 next = 0;
	static double width = 10.0; // default 10nm width
	Char tempchar[PARSELEN];
	Char tempchar2[12];
	Char nsew[12];
	static Boolean skip = false;
	static Boolean incdata = true;
	static Boolean headerinfo = false;
//	static double lastlat = INVALID_LAT_LON, lastlon = INVALID_LAT_LON;
	double arctolat, arctolon;
	double rightbrg, leftbrg;
	double tmprange1, tmprange2;
	MemHandle suadata_hand;
	Int16 n_seg;
	Int16 i;
	double d_seg;
	double clat, clon;
	static Boolean lineoverflow = false;

	double radmod;
	double circ_seg = 32.0;

	radmod = 0.5 + 0.5/Cos(PI/circ_seg); // expand the ARC radius slightly to give best approximation.

	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		MemSet(&buf,sizeof(buf),0);
		width = 10.0;
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		MemSet(selectedSUA, sizeof(SUAIndex) ,0);
		suaidx = selectedSUA;
		suaidx->class = SUAallclasses; // set default class and type
		suaidx->type = OTHER;
		incdata = true;
		headerinfo = false;
		skip = false;
		return;
	}

//	HostTraceOutputTL(appErrorClass, "Entering SUA Parser TNP Format");
	StrCopy(rxtype, "SUA Items");

	suadata_hand = MemHandleNew(sizeof(SUAData));
	suadata = MemHandleLock(suadata_hand);

	suaidx->width = width;
	while (cur<length) {
		while( (cur<length) && (next<PARSELEN) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		buf[next] = '\0';
		ConvertToUpper(buf);

		if (buf[0] == '*' || buf[0] == '#') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
		}

		if (next >= PARSELEN) {
			/* Parsing error start over */
			lineoverflow = true;
			next=0;
//			HostTraceOutputTL(appErrorClass, "Parse Error - Line Overflow");
		}
		
		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

			if (cur <= length && skip == false) {
				GetFieldDelim(buf, 0, ' ', '=', tempchar);
//				HostTraceOutputTL(appErrorClass, "Initial GetFieldDelim=|%s|", tempchar);

				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else if (StrNCompare(tempchar, "INCLUDE", 7) == 0) {
					// INCLUDE
					headerinfo = true;
//					HostTraceOutputTL(appErrorClass, "Got an INCLUDE");
					GetFieldDelim(buf, 1, '=', ' ', tempchar);
					if (StrNCompare(tempchar, "YES", 3) == 0) {
//						HostTraceOutputTL(appErrorClass, "     INCLUDE=YES");
						incdata = true;
					} else {
//						HostTraceOutputTL(appErrorClass, "     INCLUDE=NO");
						incdata = false;
					}

				} else if (StrNCompare(tempchar, "TITLE", 5) == 0 && (incdata)) {
					// TITLE
					headerinfo = true;
//					HostTraceOutputTL(appErrorClass, "------------------------------");
//					HostTraceOutputTL(appErrorClass, "Got a TITLE");
					GetFieldDelim(buf, 1, '=', '=', tempchar);
					StrNCopy(suaidx->title, trim(NoComma(tempchar," "),' ',true), 25);
//					HostTraceOutputTL(appErrorClass, "     TITLE=|%s|", suaidx->title);
					// reset RADIO and ACTIVITY for every TITLE
					//suaidx->daysactive[0] = '\0';
					//suaidx->radiofreq[0] = '\0';

				} else if (StrNCompare(tempchar, "CLASS", 5) == 0 && (incdata)) {
					// CLASS
//					HostTraceOutputTL(appErrorClass, "Got a CLASS");
					headerinfo = true;
					suaidx->class = SUAallclasses;
					GetFieldDelim(buf, 1, '=', '=', tempchar);
					if (tempchar[0] == 'A') {
						suaidx->class = CLASSA;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS A");
					} else if (tempchar[0] == 'B') {
						suaidx->class = CLASSB;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS B");
					} else if (tempchar[0] == 'C') {
						suaidx->class = CLASSC;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS C");
					} else if (tempchar[0] == 'D') {
						suaidx->class = CLASSD;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS D");
					} else if (tempchar[0] == 'E') {
						suaidx->class = CLASSE;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS E");
					} else if (tempchar[0] == 'F') {
						suaidx->class = CLASSF;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS F");
					} else if (tempchar[0] == 'G') {
						suaidx->class = CLASSG;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS G");
					}

				} else if (StrNCompare(tempchar, "TYPE", 4) == 0 && (incdata)) {
					// TYPE
//					HostTraceOutputTL(appErrorClass, "Got a TYPE");
					headerinfo = true;
					suaidx->type = OTHER;
					GetFieldDelim(buf, 1, '=', '=', tempchar);
					if (tempchar[0] == 'C') {
						// old CLASS definitions in the TYPE line
						// switches class to all types
						if ((StrNCompare(tempchar, "CLASS A", 7) == 0) || (StrNCompare(tempchar, "CLASSA", 6) == 0)) {
							suaidx->type = CLASSA;
							suaidx->class = SUAallclasses;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE CLASSA");
						} else if ((StrNCompare(tempchar, "CLASS B", 7) == 0) || (StrNCompare(tempchar, "CLASSB", 6) == 0)) {
							suaidx->type = CLASSB;
							suaidx->class = SUAallclasses;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE CLASSB");
						} else if ((StrNCompare(tempchar, "CLASS C", 7) == 0) || (StrNCompare(tempchar, "CLASSC", 6) == 0)) {
							suaidx->type = CLASSC;
							suaidx->class = SUAallclasses;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE CLASSC");
						} else if ((StrNCompare(tempchar, "CLASS D", 7) == 0) || (StrNCompare(tempchar, "CLASSD", 6) == 0)) {
							suaidx->type = CLASSD;
							suaidx->class = SUAallclasses;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE CLASSD");
						} else if ((StrNCompare(tempchar, "CLASS E", 7) == 0) || (StrNCompare(tempchar, "CLASSE", 6) == 0)) {
							suaidx->type = CLASSE;
							suaidx->class = SUAallclasses;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE CLASSE");
						} else if ((StrNCompare(tempchar, "CLASS F", 7) == 0) || (StrNCompare(tempchar, "CLASSF", 6) == 0)) {
							suaidx->type = CLASSF;
							suaidx->class = SUAallclasses;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE CLASSF");
						} else if ((StrNCompare(tempchar, "CLASS G", 7) == 0) || (StrNCompare(tempchar, "CLASSG", 6) == 0)) {
							suaidx->type = CLASSG;
							suaidx->class = SUAallclasses;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE CLASSG");

						// control area/zone accepts CTA, CTA, CTLZ, C
						} else if ((StrNCompare(tempchar, "CTA", 3) == 0) 
							|| (StrNCompare(tempchar, "CTR", 3) == 0) 
							|| (StrNCompare(tempchar, "CTL", 3) == 0) 
							|| (StrCompare(tempchar, "C") == 0)) {
							suaidx->type = CTACTR;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE CTACTR");
						}
					} else if (tempchar[0] == 'A') {
						if (StrNCompare(tempchar, "ALERT", 5) == 0) {
							suaidx->type = ALERT;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE ALERT");
						} else if ((StrNCompare(tempchar, "AIRWAY", 6) == 0) 
							|| (StrNCompare(tempchar, "AWY", 3) == 0) 
							|| (StrCompare(tempchar, "A") == 0)) {
							suaidx->type = AIRWAYS;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE AIRWAYS");
						}
					} else if (tempchar[0] == 'R') {
						suaidx->type = RESTRICTED;
//						HostTraceOutputTL(appErrorClass, "     Set TYPE RESTRICTED");
					} else if (tempchar[0] == 'P') {
						suaidx->type = PROHIBITED;
//						HostTraceOutputTL(appErrorClass, "     Set TYPE PROHIBITED");
					} else if (tempchar[0] == 'D') {
						suaidx->type = DANGER;
//						HostTraceOutputTL(appErrorClass, "     Set TYPE DANGER");
					} else if (tempchar[0] == 'Z') {
						suaidx->type = TRAINING;
//						HostTraceOutputTL(appErrorClass, "     Set TYPE TRAINING");
					} else if (tempchar[0] == 'I') {
						suaidx->type = TRAFFIC;
//						HostTraceOutputTL(appErrorClass, "     Set TYPE TRAFFIC");
					} else if (tempchar[0] == 'M') {
						if (StrNCompare(tempchar, "MOA", 3) == 0) {
							suaidx->type = MOA;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE MOA");
						} else if (StrNCompare(tempchar, "MATZ", 4) == 0) {
							suaidx->type = MATZ;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE MATZ");
						}
					} else if (tempchar[0] == 'G') {
						suaidx->type = GSEC;
//						HostTraceOutputTL(appErrorClass, "     Set TYPE GSEC");
					} else if (tempchar[0] == 'W') {
						if (StrNCompare(tempchar, "WARN", 4) == 0) {
							suaidx->type = WARNING;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE WARNING");
						}
					} else if (tempchar[0] == 'T') {
						if (StrNCompare(tempchar, "TRAINING ZONE", 13) == 0) {
							suaidx->type = TRAINING;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE TRAINING");
						} else if ((StrNCompare(tempchar, "TRAFFIC INFO", 12) == 0) 
							|| (StrNCompare(tempchar, "TIZ", 3) == 0)
							|| (StrNCompare(tempchar, "TIA", 3) == 0)) {
							suaidx->type = TRAFFIC;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE TRAFFIC");
						} else if (StrNCompare(tempchar, "TMA", 3) == 0) {
							suaidx->type = TMA;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE TMA");
						} else if (StrNCompare(tempchar, "TMZ", 3) == 0) {
							suaidx->type = TMZ;
//							HostTraceOutputTL(appErrorClass, "     Set TYPE TMZ");
						} else if (StrNCompare(tempchar, "TFR", 10) == 0) {
							suaidx->type = TFR;
//							hostTraceOutputTL(appErrorClass, "     Set TYPE MANUAL TFR");
						} else if (StrNCompare(tempchar, "TEMP", 4) == 0) {
							suaidx->type = TEMPOR;
//							hostTraceOutputTL(appErrorClass, "     Set TYPE TEMPORARY");
						}
					}

				} else if (StrNCompare(tempchar, "RADIO", 5) == 0) {
					// RADIO
					headerinfo = true;
//					HostTraceOutputTL(appErrorClass, "Got a RADIO");
					GetFieldDelim(buf, 1, '=', '=', tempchar);
					StrNCopy(suaidx->radiofreq, tempchar, 25);
//					HostTraceOutputTL(appErrorClass, "     RADIO=|%s|", tempchar);

				} else if (StrNCompare(tempchar, "ACTIVE", 6) == 0) {
					// ACTIVE
					headerinfo = true;
//					HostTraceOutputTL(appErrorClass, "Got a ACTIVE");
					GetFieldDelim(buf, 1, '=', '=', tempchar);
					StrNCopy(suaidx->daysactive, tempchar, 25);
//					HostTraceOutputTL(appErrorClass, "     ACTIVE=|%s|", tempchar);

				} else if (StrNCompare(tempchar, "TOPS", 4) == 0 && (incdata)) {
					// TOPS
					headerinfo = true;
//					HostTraceOutputTL(appErrorClass, "Got a TOPS");
					GetFieldDelim(buf, 1, '=', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     TOPS=|%s|", tempchar);
					strLen = StrLen(tempchar);
					if (tempchar[0] == 'S' || tempchar[0] == 'G') {
						// SFC, GND
						suaidx->topstype = SUA_SFC;
						suaidx->tops = SFC;
//						HostTraceOutputTL(appErrorClass, "     TOPSSFC=|%s|", DblToStr(suaidx->tops,0));
					} else if (tempchar[0] == 'F' || tempchar[0] == 'U') {
//						HostTraceOutputTL(appErrorClass, "     TOPSFL=|%s|", tempchar);
						if (tempchar[0] == 'U') {
							// Unlimited
							suaidx->topstype = SUA_UNL;
							suaidx->tops = UNLIMITED;
						} else {
							// FL
							suaidx->topstype = SUA_FL;
							suaidx->tops = StrToDbl(ToNumeric(tempchar)) * 100.0;
							suaidx->topsunits = NAUTICAL;
						}
//						HostTraceOutputTL(appErrorClass, "     TOPSFL=|%s|", DblToStr(suaidx->tops,0));
					} else {
						// ALT, MSL, AMSL or AGL, AAL, SFC, GND with Value
						if ((strLen > 3) && ((tempchar[strLen-3] == 'S') || (tempchar[strLen-3] == 'G') || ((tempchar[strLen-3] == 'A') && ((tempchar[strLen-2] == 'G') || (tempchar[strLen-2] == 'A'))))) {
							suaidx->topstype = SUA_AGL;
//							HostTraceOutputTL(appErrorClass, "     AGL");
						} else {
							suaidx->topstype = SUA_ALT;
//							HostTraceOutputTL(appErrorClass, "     ALT");
						}
//						HostTraceOutputTL(appErrorClass, "     TOPS=|%s|", tempchar);
						suaidx->topsunits = ChkUnits(tempchar);
						suaidx->tops = StrToDbl(ToNumeric(tempchar));
						if (suaidx->topsunits == METRIC) suaidx->tops = suaidx->tops/ALTMETCONST;
//						HostTraceOutputTL(appErrorClass, "     suaidx->tops=|%s|", DblToStr(suaidx->tops, 1));
					}

				} else if (StrNCompare(tempchar, "BASE", 4) == 0 && (incdata)) {
					// BASE
					headerinfo = true;
//					HostTraceOutputTL(appErrorClass, "Got a BASE");
					GetFieldDelim(buf, 1, '=', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     BASE=|%s|", tempchar);
					strLen = StrLen(tempchar);
					if (tempchar[0] == 'S' || tempchar[0] == 'G') {
						// SFC, GND
						suaidx->basetype = SUA_SFC;
						suaidx->base = SFC;
//						HostTraceOutputTL(appErrorClass, "     BASESFC=|%s|", DblToStr(suaidx->base,0));
					} else if (tempchar[0] == 'F'|| tempchar[0] == 'U') {
						suaidx->basetype = SUA_FL;
//						HostTraceOutputTL(appErrorClass, "     TOPSFL=|%s|", tempchar);
						if (tempchar[0] == 'U') {
							// Unlimited
							suaidx->basetype = SUA_UNL;
							suaidx->base = UNLIMITED;
						} else {
							// FL
							suaidx->basetype = SUA_FL;
							suaidx->base = StrToDbl(ToNumeric(tempchar)) * 100.0;
							suaidx->baseunits = NAUTICAL;
						}
//						HostTraceOutputTL(appErrorClass, "     BASEFL=|%s|", DblToStr(suaidx->base,0));
					} else {
						// ALT, MSL, AMSL or AGL, AAL, SFC, GND with Value
						if ((strLen > 3) && ((tempchar[strLen-3] == 'S') || (tempchar[strLen-3] == 'G') || ((tempchar[strLen-3] == 'A') && ((tempchar[strLen-2] == 'G') || (tempchar[strLen-2] == 'A'))))) {
							suaidx->basetype = SUA_AGL;
//							HostTraceOutputTL(appErrorClass, "     AGL");
						} else {
							suaidx->basetype = SUA_ALT;
//							HostTraceOutputTL(appErrorClass, "     ALT");
						}
//						HostTraceOutputTL(appErrorClass, "     BASE=|%s|", tempchar);
						suaidx->baseunits = ChkUnits(tempchar);
						suaidx->base = StrToDbl(ToNumeric(tempchar));
						if (suaidx->baseunits == METRIC) suaidx->base = suaidx->base/ALTMETCONST;
//						HostTraceOutputTL(appErrorClass, "     suaidx->base=|%s|", DblToStr(suaidx->base, 1));
					}

				} else if (StrNCompare(tempchar, "WIDTH", 5) == 0 && (incdata)) {
					// WIDTH
					headerinfo = true;
//					HostTraceOutputTL(appErrorClass, "Got a WIDTH");

					GetFieldDelim(buf, 1, '=', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     WIDTH=|%s|", tempchar);
					suaidx->width = StrToDbl(ToNumeric(tempchar));
					width = suaidx->width;

				} else if (StrNCompare(tempchar, "POINT", 5) == 0 && (incdata)) {
					// POINT
					if (headerinfo) {
						suaidx->plottype = SUAPOINT;
						SaveSUAData(true);
						headerinfo = false;
					}
//					HostTraceOutputTL(appErrorClass, "Got a POINT");
					MemSet(suadata, sizeof(SUAData) ,0);
					suadata->plottype = SUAPOINT;

					// Parse out the Lat & Long together
					MemSet(tempchar, sizeof(tempchar), 0);
					GetFieldDelim(buf, 1, '=', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     LAT/LON=|%s|", tempchar);

					// Parse the Lat from the string
					GetFieldDelim(tempchar, 0, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LAT w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Lat to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LAT=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Lat to the structure
					suadata->lat = DegMinSecStringToLatLon(tempchar2, nsew[0]);
					data.input.coslat = cos(DegreesToRadians(suadata->lat));
//					HostTraceOutputTL(appErrorClass, "     suadata->lat=|%s|", DblToStr(suadata->lat, 3));

					// Parse the Long from the string
					GetFieldDelim(tempchar, 1, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LON w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Long to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LON=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Long to the structure
					suadata->lon = DegMinSecStringToLatLon(tempchar2, nsew[0]);
//					HostTraceOutputTL(appErrorClass, "     suadata->lon=|%s|", DblToStr(suadata->lon, 3));
					SaveSUAData(false);

				} else if ((StrNCompare(tempchar, "CLOCKWISE RADIUS", 16) == 0 || StrNCompare(tempchar, "ANTI-CLOCKWISE RADIUS", 21) == 0) && (incdata)) {
					// Saving the last Lat / Lon values
//					lastlat = suadata->lat;
//					lastlon = suadata->lon;

					// CLOCKWISE or ANTI-CLOCKWISE
					if (headerinfo) {
						suaidx->plottype = SUAARC; // SUAARC should never be the first point, no from point
						SaveSUAData(true);
						headerinfo = false;
						MemSet(suadata, sizeof(SUAData), 0);
					}
					suadata->plottype = SUAARC;

					if (StrNCompare(tempchar, "CLOCKWISE RADIUS", 16) == 0) {
//						HostTraceOutputTL(appErrorClass, "Got a CLOCKWISE RADIUS");
						suadata->cw = true;
					} else {
//						HostTraceOutputTL(appErrorClass, "Got an ANTI-CLOCKWISE RADIUS");
						suadata->cw = false;
					}

					// Parse the Arc Radius
					GetFieldDelim(buf, 1, '=', ' ', tempchar);
//					HostTraceOutputTL(appErrorClass, "     RADIUS=|%s|", tempchar);
					suadata->radius = StrToDbl(ToNumeric(tempchar));
//					HostTraceOutputTL(appErrorClass, "     suadata->radius0=|%s|", DblToStr(suadata->radius, 2));

					// Parse out the Center Lat & Long together
					GetFieldDelim(buf, 2, '=', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     buf2=|%s|", buf);
//					HostTraceOutputTL(appErrorClass, "     LAT/LON=|%s|", tempchar);

					// Parse the Lat from the string
					GetFieldDelim(tempchar, 0, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LAT w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Lat to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LAT=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Lat to the structure
					suadata->lat = DegMinSecStringToLatLon(tempchar2, nsew[0]);
					data.input.coslat = cos(DegreesToRadians(suadata->lat));
//					HostTraceOutputTL(appErrorClass, "     suadata->lat=|%s|", DblToStr(suadata->lat, 3));

					// Parse the Long from the string
					GetFieldDelim(tempchar, 1, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LON w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Long to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LON=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Long to the structure
					suadata->lon = DegMinSecStringToLatLon(tempchar2, nsew[0]);
//					HostTraceOutputTL(appErrorClass, "     suadata->lon=|%s|", DblToStr(suadata->lon, 3));

					// Parse out the To Lat & Long together
					GetFieldDelim(buf, 3, '=', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     buf2=|%s|", buf);
//					HostTraceOutputTL(appErrorClass, "     LAT/LON=|%s|", tempchar);

					// Parse the Lat from the string
					GetFieldDelim(tempchar, 0, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LAT w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Lat to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LAT=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Lat to the structure
					arctolat = DegMinSecStringToLatLon(tempchar2, nsew[0]);
//					HostTraceOutputTL(appErrorClass, "     arctolat=|%s|", DblToStr(arctolat, 3));

					// Parse the Long from the string
					GetFieldDelim(tempchar, 1, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LON w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Long to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LON=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Long to the structure
					arctolon = DegMinSecStringToLatLon(tempchar2, nsew[0]);
//					HostTraceOutputTL(appErrorClass, "     arctolon=|%s|", DblToStr(arctolon, 3));

					if (suadata->cw) {
						LatLonToRangeBearingEll(suadata->lat, suadata->lon, SUAdatalastlat, SUAdatalastlon, &tmprange1, &leftbrg);
						LatLonToRangeBearingEll(suadata->lat, suadata->lon, arctolat, arctolon, &tmprange2, &rightbrg);
					} else {
						LatLonToRangeBearingEll(suadata->lat, suadata->lon, arctolat, arctolon, &tmprange1, &leftbrg);
						LatLonToRangeBearingEll(suadata->lat, suadata->lon, SUAdatalastlat, SUAdatalastlon, &tmprange2, &rightbrg);
					}

					// left and right bearings for drawing using WinDrawSector
					suadata->leftbrg = leftbrg;
					suadata->rightbrg = rightbrg;

					// Find number of segments to draw and increment
					// check for large arcs 
					if (leftbrg > rightbrg) {
						n_seg = (Int16)((360 + rightbrg - leftbrg)/360 * circ_seg);
						if (n_seg < 2) { // minimum 2
							n_seg = 2;
						}
						d_seg = (360 + rightbrg - leftbrg)/n_seg;
					} else {
						n_seg = (Int16)((rightbrg - leftbrg)/360 * circ_seg);
						if (n_seg < 2) { // minimum 2
							n_seg = 2;
						}
						d_seg = (rightbrg - leftbrg)/n_seg;
					}

					suadata->n = n_seg - 1; // save number of points in this ARC
					SaveSUAData(false); // save ARC record, used for drawing, ignored for polygons

//					HostTraceOutputTL(appErrorClass, "DECOMPOSE ARC TO POINTS");
//					HostTraceOutputTL(appErrorClass, "leftbrg =|%s|", DblToStr(leftbrg, 2));
//					HostTraceOutputTL(appErrorClass, "rightbrg=|%s|", DblToStr(rightbrg, 2));
//					HostTraceOutputTL(appErrorClass, "cw=|%s|", DblToStr(suadata->cw, 0));
//					HostTraceOutputTL(appErrorClass, "n_seg=|%s|", DblToStr(n_seg, 0));
//					HostTraceOutputTL(appErrorClass, "d_seg=|%s|", DblToStr(d_seg, 2));

					// save centre lat / lon
					clat = suadata->lat;
					clon = suadata->lon;		
					data.input.coslat = cos(DegreesToRadians(suadata->lat));

					// change SUA type for polygons
					suadata->plottype = SUAPOINT;

					// breakdown ARC into multiple points
					if (suadata->cw) { // clockwise
						for (i=1; i < n_seg; i++) {
//							HostTraceOutputTL(appErrorClass, "i=|%s|", DblToStr(i, 0));
							leftbrg = leftbrg + d_seg;
//							HostTraceOutputTL(appErrorClass, "brg=|%s|", DblToStr(leftbrg, 2));
							RangeBearingToLatLon(clat, clon, suadata->radius * radmod, leftbrg, &suadata->lat, &suadata->lon);
							SaveSUAData(false);
						}
					} else { // anti-clockwise
						for (i=1; i < n_seg; i++) {
//							HostTraceOutputTL(appErrorClass, "i=|%s|", DblToStr(i, 0));
							rightbrg = rightbrg - d_seg;
//							HostTraceOutputTL(appErrorClass, "brg=|%s|", DblToStr(rightbrg, 2));
							RangeBearingToLatLon(clat, clon, suadata->radius * radmod, rightbrg, &suadata->lat, &suadata->lon);
							SaveSUAData(false);
						}
					}
					
					// Save TO point
					suadata->lat = arctolat;
					suadata->lon = arctolon;
					SaveSUAData(false);

				} else if (StrNCompare(tempchar, "CIRCLE RADIUS", 13) == 0 && (incdata)) {
					// CIRCLE
					if (headerinfo) {
						suaidx->plottype = SUACIRCLE;
						SaveSUAData(true);
						headerinfo = false;
						MemSet(suadata, sizeof(SUAData), 0);
					}
//					HostTraceOutputTL(appErrorClass, "Got a CIRCLE RADIUS");
					suadata->plottype = SUACIRCLE;
					suadata->n = circ_seg;

					// Parse the Circle Radius
					GetFieldDelim(buf, 1, '=', ' ', tempchar);
//					HostTraceOutputTL(appErrorClass, "     RADIUS=|%s|", tempchar);
					suadata->radius = StrToDbl(ToNumeric(tempchar));

					// Parse out the Lat & Long together
					GetFieldDelim(buf, 2, '=', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     buf2=|%s|", buf);
//					HostTraceOutputTL(appErrorClass, "     LAT/LON=|%s|", tempchar);

					// Parse the Lat from the string
					GetFieldDelim(tempchar, 0, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LAT w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Lat to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LAT=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Lat to the structure
					suadata->lat = DegMinSecStringToLatLon(tempchar2, nsew[0]);
					data.input.coslat = cos(DegreesToRadians(suadata->lat));
//					HostTraceOutputTL(appErrorClass, "     suadata->lat=|%s|", DblToStr(suadata->lat, 3));

					// Parse the Long from the string
					GetFieldDelim(tempchar, 1, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LON w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Long to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LON=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Long to the structure
					suadata->lon = DegMinSecStringToLatLon(tempchar2, nsew[0]);
//					HostTraceOutputTL(appErrorClass, "     suadata->lon=|%s|", DblToStr(suadata->lon, 3));

					SaveSUAData(false);

				} else if (StrNCompare(tempchar, "AWY", 3) == 0 && (incdata)) {
					// AWY
					if (headerinfo) {
						suaidx->plottype = SUAAWY;
						SaveSUAData(true);
						headerinfo = false;
						MemSet(suadata, sizeof(SUAData), 0);
					}
//					HostTraceOutputTL(appErrorClass, "Got an AWY");
					suadata->plottype = SUAAWY;
					suadata->radius = suaidx->width; // save the airway width to the radius field
					suadata->n = 0;

					// Parse out the Lat & Long together
					MemSet(tempchar, sizeof(tempchar), 0);
					GetFieldDelim(buf, 1, '=', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     LAT/LON=|%s|", tempchar);

					// Parse the Lat from the string
					GetFieldDelim(tempchar, 0, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LAT w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Lat to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LAT=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Lat to the structure
					suadata->lat = DegMinSecStringToLatLon(tempchar2, nsew[0]);
					data.input.coslat = cos(DegreesToRadians(suadata->lat));
//					HostTraceOutputTL(appErrorClass, "     suadata->lat=|%s|", DblToStr(suadata->lat, 3));

					// Parse the Long from the string
					GetFieldDelim(tempchar, 1, ' ', ' ', tempchar2);
//					HostTraceOutputTL(appErrorClass, "     LON w/NSEW=|%s|", tempchar2);
					// Copy the string with the NSEW on the front to variable
					StrCopy(nsew, tempchar2);
					// Set the first character of the Long to a space
					tempchar2[0] = ' ';
//					HostTraceOutputTL(appErrorClass, "     LON=|%s|", tempchar2);
//					HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
					// Convert the Long to the structure
					suadata->lon = DegMinSecStringToLatLon(tempchar2, nsew[0]);
//					HostTraceOutputTL(appErrorClass, "     suadata->lon=|%s|", DblToStr(suadata->lon, 3));
					SaveSUAData(false);

				} else if (StrNCompare(tempchar, "END", 3) == 0) {
					// END
//					HostTraceOutputTL(appErrorClass, "Got an END");
					SUA_parser_tnp(NULL, 0, true);
				}
			}
			next=0;
		}
		skip = false;
	}

	// clear up memory
	MemHandleUnlock(suadata_hand);
	MemHandleFree(suadata_hand);

//	HostTraceOutputTL(appErrorClass, "End SUA Parser TNP Format");
	return;
}

/*
Boolean GetTNPLatLon(Char *coordbuf, Int8 i, Boolean spacedelim, double *lat, double *lon)
{
	Boolean result = true;
	Char tempchar[81];
	Char nsew[12];

	// check input string length
	if (StrLen(coordbuf) > 79) {
		*lat = INVALID_LAT_LON;
		*lon = INVALID_LAT_LON;
		return(false);
	}

	// Parse the Lat from the string
	GetFieldDelim(coordbuf, i, ' ', ' ', tempchar);
//	HostTraceOutputTL(appErrorClass, "     LAT w/NSEW=|%s|", tempchar);
	// Copy the string with the NSEW on the front to variable
	StrCopy(nsew, tempchar);
	// Set the first character of the Lat to a space
	tempchar[0] = ' ';
//	HostTraceOutputTL(appErrorClass, "     LAT=|%s|", tempchar);
//	HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
	// Convert the Lat to the structure
	*lat = DegMinSecStringToLatLon(tempchar, nsew[0]);

	data.input.coslat = cos(DegreesToRadians(*lat));

	// Parse the Long from the string
	i++;
	GetFieldDelim(coordbuf, i, ' ', ' ', tempchar);
//	HostTraceOutputTL(appErrorClass, "     LON w/NSEW=|%s|", tempchar);
	// Copy the string with the NSEW on the front to variable
	StrCopy(nsew, tempchar);
	// Set the first character of the Long to a space
	tempchar[0] = ' ';
//	HostTraceOutputTL(appErrorClass, "     LON=|%s|", tempchar);
//	HostTraceOutputTL(appErrorClass, "     nsew=|%c|", nsew[0]);
	// Convert the Long to the structure
	*lon = DegMinSecStringToLatLon(tempchar, nsew[0]);

	// check lat / lon
	if (Fabs(*lat) > 90.0) result = false;
	if (Fabs(*lon) > 180.0) result = false;
	if (!result) *lat = *lon = INVALID_LAT_LON;

	return(result);
}
*/

void SUA_parser_openair(Char* serinp, UInt32 length, Boolean reset)
{
	UInt32 cur = 0;
	UInt16 strLen = 0;
	static Int16 next = 0;
	static double width = 10.0; // default 10nm width
	Char tempchar[PARSELEN];
	Char newbuf[PARSELEN];
//	Char nsew[12];
	static Boolean skip = false;
	static Boolean incdata = true;
	static Boolean headerinfo = false;
	double arcfromlat, arcfromlon;
	double arctolat, arctolon;
	double rightbrg, leftbrg;
	double tmprange1, tmprange2;
	MemHandle suadata_hand;
	Int16 n_seg;
	Int16 i;
	double d_seg;
	static double clat = INVALID_LAT_LON;
	static double clon = INVALID_LAT_LON;
	static Boolean clockwise = true;
	static Boolean lineoverflow = false;
	
	double radmod;
	double circ_seg = 32.0;

	radmod = 0.5 + 0.5/Cos(PI/circ_seg); // expand the ARC radius slightly to give best approximation.

	if (reset) {
//		HostTraceOutputTL(appErrorClass, " resetting");
		next = 0;
		skip = false;
		MemSet(&buf,sizeof(buf),0);
		width = 10.0;
		rxrecs = 0;
		StrCopy(rxtype, "Records");
		MemSet(selectedSUA, sizeof(SUAIndex) ,0);
		suaidx = selectedSUA;
		StrCopy(suaidx->title, "N/A");	// set default title, class and type
		suaidx->class = SUAallclasses;
		suaidx->type = OTHER;
		incdata = true;
		headerinfo = false;
		skip = false;

		return;
	}

//	HostTraceOutputTL(appErrorClass, "Entering SUA Parser OpenAir Format");
	StrCopy(rxtype, "SUA Items");

	suadata_hand = MemHandleNew(sizeof(SUAData));
	suadata = MemHandleLock(suadata_hand);

	suaidx->width = width;
	while (cur<length) {
		while( (cur<length) && (next<PARSELEN) && (serinp[cur] != '\n')) {
			if (serinp[cur] != '\r') {
				buf[next]=serinp[cur];
				next++;
			}
			cur++;
		}
		buf[next] = '\0';
		ConvertToUpper(buf);

		if (buf[0] == '*' || buf[0] == '#') {
//			HostTraceOutputTL(appErrorClass, "Setting skip to true");
			skip = true;
		}

		if (next >= PARSELEN) {
			/* Parsing error start over */
			lineoverflow = true;
			next=0;
//			HostTraceOutputTL(appErrorClass, "Parse Error - Line Overflow");
		}

		if (StrLen(buf) == 0) {
			cur++;
		} else if (serinp[cur] == '\n' && next <= PARSELEN) {
			cur++;
			StrCopy(buf, trim(buf, '\t', true));

			if (cur <= length && skip == false) {
				GetFieldDelim(buf, 0, ' ', ' ', tempchar);
//				HostTraceOutputTL(appErrorClass, "Initial GetFieldDelim=|%s| : ", tempchar);

				if (lineoverflow) {
					// skip line after line length overflow
//					HostTraceOutputTL(appErrorClass, "Line Over flow %s", buf);
					lineoverflow = false;

				} else if (StrNCompare(tempchar, "AN", 2) == 0) { // && (incdata)) {
//					HostTraceOutputTL(appErrorClass, "********************");
					// NAME
					incdata = true;
					headerinfo = true;
					clockwise = true;
//					HostTraceOutputTL(appErrorClass, "Got a NAME");
					GetFieldDelim(buf, 1, ' ', '=', tempchar);
					StrNCopy(suaidx->title, trim(NoComma(tempchar," "),' ',true), 25);
					suaidx->active = true;
//					HostTraceOutputTL(appErrorClass, "NAME=|%s|", suaidx->title);

				} else if (StrNCompare(tempchar, "AC", 2) == 0) { // && (incdata)) {
					headerinfo = true;
					clockwise = true;
//					HostTraceOutputTL(appErrorClass, "Got a CLASS");
					suaidx->class = SUAallclasses;
					suaidx->type = OTHER;
					GetFieldDelim(buf, 1, ' ', '=', tempchar);
					if (tempchar[0] == 'A') {
						suaidx->class = CLASSA;
						suaidx->type = CLASSA;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS A");
					} else if (tempchar[0] == 'B') {
						suaidx->class = CLASSB;
						suaidx->type = CLASSB;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS B");
					} else if (tempchar[0] == 'C') {
						if (tempchar[1] == 'T') {
							suaidx->class = SUAallclasses;
							suaidx->type = CTACTR;
//							HostTraceOutputTL(appErrorClass, "     Set CTR/CTA");
						} else {
							suaidx->class = CLASSC;
							suaidx->type = CLASSC;
//							HostTraceOutputTL(appErrorClass, "     Set CLASS c");
						}
					} else if (tempchar[0] == 'D') {
						suaidx->class = CLASSD;
						suaidx->type = CLASSD;
//						HostTraceOutputTL(appErrorClass, "     Set CLASS D");
					} else if (tempchar[0] == 'Q') {
						suaidx->class = SUAallclasses;
						suaidx->type = DANGER;
//						HostTraceOutputTL(appErrorClass, "     Set Danger");
					} else if (tempchar[0] == 'R') {
						suaidx->class = SUAallclasses;
						suaidx->type = RESTRICTED;
//						HostTraceOutputTL(appErrorClass, "     Set Restricted");
					} else if (tempchar[0] == 'P') {
						suaidx->class = SUAallclasses;
						suaidx->type = PROHIBITED;
//						HostTraceOutputTL(appErrorClass, "     Set Prohibited");
					} else if (tempchar[0] == 'G') {
						suaidx->class = SUAallclasses;
						suaidx->type = PROHIBITED;
//						HostTraceOutputTL(appErrorClass, "     Set Glider Prohibited");
					} else if (tempchar[0] == 'W') {
						suaidx->class = SUAallclasses;
						suaidx->type = GSEC;
//						HostTraceOutputTL(appErrorClass, "     Set Wave Window");
					}

				} else if (StrNCompare(tempchar, "AH", 2) == 0) { // && (incdata)) {
					// TOPS
					headerinfo = true;
					clockwise = true;
//					HostTraceOutputTL(appErrorClass, "Got a TOPS");
					GetFieldDelim(buf, 1, ' ', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     TOPS=|%s|", tempchar);
					strLen = StrLen(tempchar);
					if (tempchar[0] == 'S' || tempchar[0] == 'G') {
						// SFC, GND
						suaidx->topstype = SUA_SFC;
						suaidx->tops = SFC;
//						HostTraceOutputTL(appErrorClass, "     TOPSSFC=|%s|", DblToStr(suaidx->tops,0));
					} else if (tempchar[0] == 'F' || tempchar[0] == 'U') {
//						HostTraceOutputTL(appErrorClass, "     TOPSFL=|%s|", tempchar);
						if (tempchar[0] == 'U') {
							// unlimited
							suaidx->topstype = SUA_UNL;
							suaidx->tops = UNLIMITED;
						} else {
							// FL
							suaidx->topstype = SUA_FL;
							suaidx->tops = StrToDbl(ToNumeric(tempchar)) * 100.0;
							suaidx->topsunits = NAUTICAL;
						}
//						HostTraceOutputTL(appErrorClass, "     TOPSFL=|%s|", DblToStr(suaidx->tops,0));
					} else {
						// ALT, MSL, AMSL or AGL, AAL, SFC, GND with Value
						if ((strLen > 3) && ((tempchar[strLen-3] == 'S') || (tempchar[strLen-3] == 'G') || ((tempchar[strLen-3] == 'A') && ((tempchar[strLen-2] == 'G') || (tempchar[strLen-2] == 'A'))))) {
							suaidx->topstype = SUA_AGL;
//							HostTraceOutputTL(appErrorClass, "     AGL");
						} else {
							suaidx->topstype = SUA_ALT;
//							HostTraceOutputTL(appErrorClass, "     ALT");
						}
//						HostTraceOutputTL(appErrorClass, "     TOPS=|%s|", tempchar);
						suaidx->topsunits = ChkUnits(tempchar);
						suaidx->tops = StrToDbl(ToNumeric(tempchar));
						if (suaidx->topsunits == METRIC) suaidx->tops = suaidx->tops/ALTMETCONST;
//						HostTraceOutputTL(appErrorClass, "     suaidx->tops=|%s|", DblToStr(suaidx->tops, 1));
					}

				} else if (StrNCompare(tempchar, "AL", 2) == 0) { // && (incdata)) {
					// BASE
					headerinfo = true;
					clockwise = true;
//					HostTraceOutputTL(appErrorClass, "Got a BASE");
					GetFieldDelim(buf, 1, ' ', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     BASE=|%s|", tempchar);
					strLen = StrLen(tempchar);
					if (tempchar[0] == 'S' || tempchar[0] == 'G') {
						// SFC, GND
						suaidx->basetype = SUA_SFC;
						suaidx->base = SFC;
//						HostTraceOutputTL(appErrorClass, "     BASESFC=|%s|", DblToStr(suaidx->base,0));
					} else if (tempchar[0] == 'F'|| tempchar[0] == 'U') {
//						HostTraceOutputTL(appErrorClass, "     TOPSFL=|%s|", tempchar);
						if (tempchar[0] == 'U') {
							// Unlimited
							suaidx->basetype = SUA_UNL;
							suaidx->base = UNLIMITED;
						} else {
							// FL
							suaidx->basetype = SUA_FL;
							suaidx->base = StrToDbl(ToNumeric(tempchar)) * 100.0;
							suaidx->baseunits = NAUTICAL;
						}
//						HostTraceOutputTL(appErrorClass, "     BASEFL=|%s|", DblToStr(suaidx->base,0));
					} else {
						// ALT, MSL, AMSL or AGL, AAL, SFC, GND with Value
						if ((strLen > 3) && ((tempchar[strLen-3] == 'S') || (tempchar[strLen-3] == 'G') || ((tempchar[strLen-3] == 'A') && ((tempchar[strLen-2] == 'G') || (tempchar[strLen-2] == 'A'))))) {
							suaidx->basetype = SUA_AGL;
//							HostTraceOutputTL(appErrorClass, "     AGL");
						} else {
							suaidx->basetype = SUA_ALT;
//							HostTraceOutputTL(appErrorClass, "     ALT");
						}
//						HostTraceOutputTL(appErrorClass, "     BASE=|%s|", tempchar);
						suaidx->baseunits = ChkUnits(tempchar);
						suaidx->base = StrToDbl(ToNumeric(tempchar));
						if (suaidx->baseunits == METRIC) suaidx->base = suaidx->base/ALTMETCONST;
//						HostTraceOutputTL(appErrorClass, "     suaidx->base=|%s|", DblToStr(suaidx->base, 1));
					}

				} else if (StrNCompare(tempchar, "V", 1) == 0) { // && (incdata)) {
//					HostTraceOutputTL(appErrorClass, "Got a VARIABLE");
					GetFieldDelim(buf, 1, ' ', '=', tempchar);
						if (StrNCompare(tempchar, "X", 1) == 0) {
//							HostTraceOutputTL(appErrorClass, "Got a CENTRE");
							GetOpenAirLatLon(buf, 1, false, &clat, &clon);

						} else if (StrNCompare(tempchar, "W", 1) == 0) {
//							HostTraceOutputTL(appErrorClass, "Got a WIDTH");
							GetFieldDelim(buf, 1, '=', '=', tempchar);
//							HostTraceOutputTL(appErrorClass, "     WIDTH=|%s|", tempchar);
							suaidx->width = StrToDbl(ToNumeric(trim(tempchar,' ',true)));
							width = suaidx->width;

						} else if (StrNCompare(tempchar, "D", 1) == 0) {
//							HostTraceOutputTL(appErrorClass, "Got a DIRECTION");
							GetFieldDelim(buf, 1, '=', '=', tempchar);
//							HostTraceOutputTL(appErrorClass, "     DIRECTION=|%s|", tempchar);
							if (StrNCompare(trim(tempchar,' ',true), "-", 1) == 0) {
								clockwise = false;
							} else {
								clockwise = true;
							}
//							HostTraceOutputTL(appErrorClass, "     CLOCKWISE=|%s|", DblToStr((Int8)clockwise,0));
						}

				} else if (StrNCompare(tempchar, "DP", 2) == 0) { // && (incdata)) {
//					HostTraceOutputTL(appErrorClass, "Got a POINT");
					if (headerinfo) {
						suaidx->plottype = SUAPOINT;
						SaveSUAData(true);
						headerinfo = false;
					}
					MemSet(suadata, sizeof(SUAData), 0);
					suadata->plottype = SUAPOINT;

					GetOpenAirLatLon(buf, 1, true, &suadata->lat, &suadata->lon);
					// save point
					SaveSUAData(false);

				} else if ((StrNCompare(tempchar, "DB", 2) == 0 || StrNCompare(tempchar, "DA", 2) == 0)) { // && (incdata)) {
//					HostTraceOutputTL(appErrorClass, "Got a ARC");
					if (headerinfo) {
						suaidx->plottype = SUAARC;
						SaveSUAData(true);
						headerinfo = false;
						MemSet(suadata, sizeof(SUAData), 0);
					}
					if (StrNCompare(tempchar, "DB", 2) == 0) {
//						HostTraceOutputTL(appErrorClass, "DB ARC |%s|", buf);
						suadata->cw = clockwise;
						// get first lat / lon pair
						GetFieldDelim(buf, 0, ',', ',', newbuf);
//						HostTraceOutputTL(appErrorClass, "newbuf=|%s|", newbuf);
						GetOpenAirLatLon(newbuf, 1, true, &arcfromlat, &arcfromlon);
						// get second lat / lon pair
						GetFieldDelim(buf, 1, ',', ',', newbuf);
//						HostTraceOutputTL(appErrorClass, "newbuf=|%s|", newbuf);
						GetOpenAirLatLon(newbuf, 0, true, &arctolat, &arctolon);
						// find radius
						if (suadata->cw) {
							LatLonToRangeBearingEll(clat, clon, arcfromlat, arcfromlon, &tmprange1, &leftbrg);
							LatLonToRangeBearingEll(clat, clon, arctolat, arctolon, &tmprange2, &rightbrg);
						} else {
							LatLonToRangeBearingEll(clat, clon, arctolat, arctolon, &tmprange1, &leftbrg);
							LatLonToRangeBearingEll(clat, clon, arcfromlat, arcfromlon, &tmprange2, &rightbrg);
						}
						suadata->radius = (tmprange1+tmprange2)/2.0;
//						HostTraceOutputTL(appErrorClass, "     arc->radius=|%s|", DblToStr(suadata->radius, 2));
						// Save FROM point
						suadata->plottype = SUAPOINT;
						suadata->lat = arcfromlat;
						suadata->lon = arcfromlon;
						SaveSUAData(false);
					} else {
//						HostTraceOutputTL(appErrorClass, "DA ARC |%s|", buf);
						// get radius
						GetFieldDelim(buf, 1, ' ', ',', tempchar);
//						HostTraceOutputTL(appErrorClass, "     RADIUS=|%s|", tempchar);
						suadata->radius = StrToDbl(ToNumeric(tempchar));
//						HostTraceOutputTL(appErrorClass, "     arc->radius=|%s|", DblToStr(suadata->radius, 2));
						// get left bearing
						GetFieldDelim(buf, 1, ',', ',', tempchar);
//						HostTraceOutputTL(appErrorClass, "     LEFTBRG=|%s|", tempchar);
						leftbrg = StrToDbl(ToNumeric(tempchar));
//						HostTraceOutputTL(appErrorClass, "     leftbrg=|%s|", DblToStr(leftbrg, 2));
						// get right bearing
						GetFieldDelim(buf, 2, ',', ',', tempchar);
//						HostTraceOutputTL(appErrorClass, "     RIGHTBRG=|%s|", tempchar);
						rightbrg = StrToDbl(ToNumeric(tempchar));
//						HostTraceOutputTL(appErrorClass, "     rightbrg=|%s|", DblToStr(rightbrg, 2));
						// calculate direction
						suadata->cw = leftbrg < rightbrg;
//						HostTraceOutputTL(appErrorClass, "     CLOCKWISE=|%s|", DblToStr((Int8)clockwise,0));
						// calculate from and to points
						RangeBearingToLatLon(clat, clon, suadata->radius * radmod, leftbrg, &arcfromlat, &arcfromlon);
						RangeBearingToLatLon(clat, clon, suadata->radius * radmod, rightbrg, &arctolat, &arctolon);
						// Save FROM point
						suadata->plottype = SUAPOINT;
						suadata->lat = arcfromlat;
						suadata->lon = arcfromlon;
						SaveSUAData(false);
					}

					// save centre lat / lon to arc record
					suadata->plottype = SUAARC;
					suadata->lat = clat;
					suadata->lon = clon;
					data.input.coslat = cos(DegreesToRadians(suadata->lat));

     					// left and right bearings for drawing using WinDrawSector
					suadata->leftbrg = leftbrg;
					suadata->rightbrg = rightbrg;

					// Find number of segments to draw and increment
					// check for large arcs
					if (leftbrg > rightbrg) {
						n_seg = (Int16)((360 + rightbrg - leftbrg)/360 * circ_seg);
						if (n_seg < 2) { // minimum 2
							n_seg = 2;
						}
						d_seg = (360 + rightbrg - leftbrg)/n_seg;
					} else {
						n_seg = (Int16)((rightbrg - leftbrg)/360 * circ_seg);
						if (n_seg < 2) { // minimum 2
							n_seg = 2;
						}
						d_seg = (rightbrg - leftbrg)/n_seg;
					}
					suadata->n = n_seg - 1; // save number of points in this ARC
					SaveSUAData(false); // save ARC record, used for drawing, ignored for polygons
//					HostTraceOutputTL(appErrorClass, "DECOMPOSE ARC TO POINTS");
//					HostTraceOutputTL(appErrorClass, "leftbrg =|%s|", DblToStr(leftbrg, 2));
//					HostTraceOutputTL(appErrorClass, "rightbrg=|%s|", DblToStr(rightbrg, 2));
//					HostTraceOutputTL(appErrorClass, "n_seg=|%s|", DblToStr(n_seg, 0));
//					HostTraceOutputTL(appErrorClass, "d_seg=|%s|", DblToStr(d_seg, 2));

					// change SUA type for polygons
					suadata->plottype = SUAPOINT;

					// breakdown ARC into multiple points
					if (suadata->cw) { // clockwise
						for (i=1; i < n_seg; i++) {
//							HostTraceOutputTL(appErrorClass, "i=|%s|", DblToStr(i, 0));
							leftbrg = nice_brg(leftbrg + d_seg);
//							HostTraceOutputTL(appErrorClass, "brg=|%s|", DblToStr(leftbrg, 2));
							RangeBearingToLatLon(clat, clon, suadata->radius * radmod, leftbrg, &suadata->lat, &suadata->lon);
							SaveSUAData(false);
						}
					} else { // anti-clockwise
						for (i=1; i < n_seg; i++) {
//							HostTraceOutputTL(appErrorClass, "i=|%s|", DblToStr(i, 0));
							rightbrg = nice_brg(rightbrg - d_seg);
//							HostTraceOutputTL(appErrorClass, "brg=|%s|", DblToStr(rightbrg, 2));
							RangeBearingToLatLon(clat, clon, suadata->radius * radmod, rightbrg, &suadata->lat, &suadata->lon);
							SaveSUAData(false);
						}
					}

					// Save TO point
					suadata->lat = arctolat;
					suadata->lon = arctolon;
					SaveSUAData(false);

				} else if (StrNCompare(tempchar, "DC", 2) == 0) { // && (incdata)) {
//					HostTraceOutputTL(appErrorClass, "Got a CIRCLE");
					if (headerinfo) {
						suaidx->plottype = SUACIRCLE;
						SaveSUAData(true);
						headerinfo = false;
						MemSet(suadata, sizeof(SUAData), 0);
					}
					suadata->plottype = SUACIRCLE;
					suadata->n = circ_seg;

					GetFieldDelim(buf, 1, ' ', '=', tempchar);
//					HostTraceOutputTL(appErrorClass, "     CIRCLE=|%s|", tempchar);
					suadata->radius = StrToDbl(ToNumeric(tempchar));
					suadata->lat = clat;
					suadata->lon = clon;
//					HostTraceOutputTL(appErrorClass, "     suadata->radius=|%s|", DblToStr(suadata->radius, 3));
					// save circle
					SaveSUAData(false);
					// reset center
					clat = INVALID_LAT_LON;
					clon = INVALID_LAT_LON;

				} else if (StrNCompare(tempchar, "DY", 2) == 0) { // && (incdata)) {
//					HostTraceOutputTL(appErrorClass, "Got a AIRWAY");
					if (headerinfo) {
						suaidx->plottype = SUAAWY;
						SaveSUAData(true);
						headerinfo = false;
						MemSet(suadata, sizeof(SUAData), 0);
					}
					suadata->plottype = SUAAWY;
					suadata->radius = suaidx->width; // save the airway width to the radius field
					suadata->n = 0;
					GetOpenAirLatLon(buf, 1, true, &suadata->lat, &suadata->lon);
					// save point
					SaveSUAData(false);

				} else if ((StrNCompare(tempchar, "SB", 2) == 0 || StrNCompare(tempchar, "SP", 2) == 0)) { // && (incdata)) {
//					HostTraceOutputTL(appErrorClass, "Got a colour table");
					// do nothing
					
				} else if (StrNCompare(tempchar, "END", 3) == 0) {
					// END
//					HostTraceOutputTL(appErrorClass, "Got an END");
					SUA_parser_openair(NULL, 0, true);
				}

			}
			next=0;
		}
		skip = false;
	}

	// clear up memory
	MemHandleUnlock(suadata_hand);
	MemHandleFree(suadata_hand);

//	HostTraceOutputTL(appErrorClass, "End SUA Parser OpenAir Format");
	return;
}

Boolean GetOpenAirLatLon(Char *coordbuf, Int8 i, Boolean spacedelim, double *lat, double *lon)
{
	Char tempchar[81];
	Char nsew[12];
	Boolean result = true;

//	HostTraceOutputTL(appErrorClass, "Lat/Lon=|%s|", coordbuf);

	// check input string length
	if (StrLen(coordbuf) > 79) {
		*lat = INVALID_LAT_LON;
		*lon = INVALID_LAT_LON;
		return(false);
	}

	// get lat
	if (!spacedelim) {
		// find = and replace with space
		StrChr(coordbuf, '=')[0] = ' ';
		i++;
	}
	GetFieldDelim(coordbuf, i, ' ', ' ', tempchar);
//	HostTraceOutputT(appErrorClass, "     LAT=|%s|", tempchar);
	// get NS indicator if required
	StrCopy(nsew, Right(tempchar,1));
//	HostTraceOutputTL(appErrorClass, " -> Chk NSEW=|%s|", nsew);
	if (StrChr("NSns", nsew[0]) == NULL) {
		i++;
		GetFieldDelim(coordbuf, i, ' ', ' ', nsew);
		nsew[1] = '\0';
//		HostTraceOutputTL(appErrorClass, "    NSEW=|%s|", nsew);
		StrCat(tempchar, nsew);
	}
	if (StrChr("NSns", nsew[0]) == NULL) result = false;
//	HostTraceOutputTL(appErrorClass, "   parse lat=|%s|", tempchar);
	*lat = DegMinSecColonStringToLatLon(tempchar);
//	HostTraceOutputTL(appErrorClass, "     lat=|%s|", DblToStr(*lat, 3));
	data.input.coslat = cos(DegreesToRadians(*lat));

	// get lon
	i++;
	GetFieldDelim(coordbuf, i, ' ', ' ', tempchar);
	// get EW indicator if required
	GetFieldDelim(tempchar, 0, ',', ',', tempchar);
	// check for comment
	if (StrChr(tempchar, '*')) {
		StrChr(tempchar, '*')[0] = ' ';
		GetFieldDelim(tempchar, 0, ' ', ' ', tempchar);
	}
//	HostTraceOutputT(appErrorClass, "     LON=|%s|", tempchar);
	StrCopy	(nsew, Right(tempchar,1));
	nsew[1] = '\0';
//	HostTraceOutputTL(appErrorClass, " -> Chk NSEW=|%s|", nsew);
	if (StrChr("EWew", nsew[0]) == NULL) {
		i++;
		GetFieldDelim(coordbuf, i, ' ', ' ', nsew);
		nsew[1] = '\0';
//		HostTraceOutputTL(appErrorClass, "    NSEW=|%s|", nsew);
		StrCat(tempchar, nsew);
	}
	if (StrChr("EWew", nsew[0]) == NULL) result = false;
//	HostTraceOutputTL(appErrorClass, "   parse lon=|%s|", tempchar);
	*lon = DegMinSecColonStringToLatLon(tempchar);
//	HostTraceOutputTL(appErrorClass, "     lon=|%s|", DblToStr(*lon, 3));

	// check lat / lon
	if (Fabs(*lat) > 90.0) result = false;
	if (Fabs(*lon) > 180.0) result = false;
	if (!result) *lat = *lon = INVALID_LAT_LON;

//	HostTraceOutputTL(appErrorClass, "----------");
	return(result);
}

void SaveSUAData(Boolean suaidxinfo)
{
	static Int16 suaidxindex;
	Int16 suadataindex;
	static Boolean firsttime = true;
	double temprange;
	double templat, templon;
	static SUAIndex savesuaidx;
	SUAIndex tempidx;
//	Char tempchar[20];

//	HostTraceOutputTL(appErrorClass, "Inside SaveSUAData");
	if (suaidxinfo) {
		// add last point if required (only for openair files)
		if ((data.config.SUAformat == SUAOPENAIR) && ((OpenAirlastlat != OpenAirfirstlat) || (OpenAirlastlon != OpenAirfirstlon)) && !(suaidx->type & AIRWAYS) && (suaidx->plottype != SUAAWY)) {
//			HostTraceOutputTL(appErrorClass, "Check Last Point");
//			LLToStringDMS(OpenAirfirstlat, tempchar, ISLAT);
//			HostTraceOutputT(appErrorClass, "New Point  = |%s|", tempchar);
//			LLToStringDMS(OpenAirfirstlon, tempchar, ISLON);
//			HostTraceOutputTL(appErrorClass, " |%s|", tempchar);

			// save the new SUA header
			MemMove(&tempidx, suaidx, sizeof(SUAIndex));
			// load previous SUA header
			MemMove(suaidx, &savesuaidx, sizeof(SUAIndex));
			// add last point
			suadata->lat = OpenAirfirstlat;
			suadata->lon = OpenAirfirstlon;
			SaveSUAData(false);
//			HostTraceOutputTL(appErrorClass, "Last Point Header Title=|%s|", suaidx->title);
			// restore the new SUA header
			MemMove(suaidx, &tempidx, sizeof(SUAIndex));
		}
//		HostTraceOutputTL(appErrorClass, "New SUA Header : Title=|%s|", suaidx->title);
//		HostTraceOutputTL(appErrorClass, "Width=|%s|", DblToStr(suaidx->width,0));
//		HostTraceOutputTL(appErrorClass, "Plot =|%s|", DblToStr(suaidx->plottype,0));
//		HostTraceOutputTL(appErrorClass, "Class=|%s|", DblToStr(suaidx->class,0));
//		HostTraceOutputTL(appErrorClass, "Type= |%s|", DblToStr(suaidx->type,0));
//		HostTraceOutputTL(appErrorClass, "     startidx=|%s|", DblToStr(suaidx->startidx, 0));
//		HostTraceOutputTL(appErrorClass, "      stopidx=|%s|", DblToStr(suaidx->stopidx, 0));
//		HostTraceOutputTL(appErrorClass, "    totalrecs=|%s|", DblToStr(totalidxrecs, 0));
//		HostTraceOutputTL(appErrorClass, "              ");
		suaidx->active = true;
		suaidxindex = OpenDBAddRecord(suaidx_db, sizeof(SUAIndex), suaidx, false);

		rxrecs = suaidxindex;
		if (FrmGetActiveFormID() == form_transfer) {
			// update record counter on transferscreen
			field_set_value(form_transfer_records, DblToStr(suaidxindex+1,0));
		}
		// zero width for non airway types
		if (suaidx->plottype != SUAAWY) {
			suaidx->width = 0.0;
		}
		// zero counters
		suaidx->stopidx = 0;
		suaidx->startidx = 0;
		firsttime = true;
		// set default dismissed and active defaults
		suaidx->Dismissed = false;
		suaidx->WarnActive = true;
		suaidx->LastWarned = 0;
		suaidx->ApproachLastWarned = 0;

	} else {
//		HostTraceOutputTL(appErrorClass, "  New SUA Data");
		suadataindex = OpenDBAddRecord(suadata_db, sizeof(SUAData), suadata, false);
		// save last lat / lon value
		SUAdatalastlat = suadata->lat;
		SUAdatalastlon = suadata->lon;

		if (firsttime) {
//			HostTraceOutputTL(appErrorClass, "     New First Item");
			suaidx->startidx = suadataindex;
			firsttime = false;
//			HostTraceOutputTL(appErrorClass, "Setting min/max values to first point values");
			// store first point
			OpenAirfirstlat = suadata->lat;
			OpenAirfirstlon = suadata->lon;
			OpenAirlastlat = INVALID_LAT_LON;
			OpenAirlastlon = INVALID_LAT_LON;
			// set bounding ball defaults
			suaidx->reflat  = suadata->lat;
			suaidx->reflon  = suadata->lon;
			suaidx->maxdist = suadata->radius;
			// Set the min and max values to the first lat and lon
			suaidx->SUAmaxlat = suaidx->SUAminlat = suadata->lat;
			suaidx->SUAmaxlon = suaidx->SUAminlon = suadata->lon;
			// Update min and max values for a circle
			if (suadata->plottype == SUACIRCLE) {
//				HostTraceOutputTL(appErrorClass, "     New Circle (first data item)");
				suaidx->SUAmaxlat = suadata->lat + suadata->radius / 60.0;
				suaidx->SUAminlat = suadata->lat - suadata->radius / 60.0;
				suaidx->SUAmaxlon = suadata->lon + suadata->radius / 60.0 / data.input.coslat;
				suaidx->SUAminlon = suadata->lon - suadata->radius / 60.0 / data.input.coslat;
				// only single point for circle
				OpenAirlastlat = suadata->lat;
				OpenAirlastlon = suadata->lon;
			}
		} else {
			// store last point
			OpenAirlastlat = suadata->lat;
			OpenAirlastlon = suadata->lon;
			// check for update to min and max values
			if (suadata->plottype == SUACIRCLE) { // no need to update max / min for ARC as decomposed to points
//				HostTraceOutputTL(appErrorClass, "     New Circle %s", suaidx->title);
				templat = suadata->lat + suadata->radius / 60.0;
				if (templat > suaidx->SUAmaxlat) suaidx->SUAmaxlat = templat;
				templat = suadata->lat - suadata->radius / 60.0;
				if (templat < suaidx->SUAminlat) suaidx->SUAminlat = templat;
				templon = suadata->lon + suadata->radius / 60.0 / data.input.coslat;
				if (templon > suaidx->SUAmaxlon) suaidx->SUAmaxlon = templon;
				templon = suadata->lon - suadata->radius / 60.0 / data.input.coslat;				
				if (templon < suaidx->SUAminlon) suaidx->SUAminlon = templon;
			} else if ((suadata->plottype == SUAPOINT) || (suadata->plottype == SUAAWY)) {
//				HostTraceOutputTL(appErrorClass, "     New Point / Airway %s", suaidx->title);
				if (suadata->lat > suaidx->SUAmaxlat) suaidx->SUAmaxlat = suadata->lat;
				if (suadata->lat < suaidx->SUAminlat) suaidx->SUAminlat = suadata->lat;
				if (suadata->lon > suaidx->SUAmaxlon) suaidx->SUAmaxlon = suadata->lon;
				if (suadata->lon < suaidx->SUAminlon) suaidx->SUAminlon = suadata->lon;
			} else {
//				HostTraceOutputTL(appErrorClass, "NEW ARC |%s|", suaidx->title);
//				LLToStringDMS(suadata->lat, tempchar, ISLAT);
//				HostTraceOutputT(appErrorClass, "Arc Point  = |%s|", tempchar);
//				LLToStringDMS(suadata->lon, tempchar, ISLON);
//				HostTraceOutputTL(appErrorClass, " |%s|", tempchar);
			}
//			HostTraceOutputTL(appErrorClass, "     Update Bounding Ball");
			// Calculate the distance from the min and max values
			LatLonToRange(suaidx->SUAminlat, suaidx->SUAminlon, suaidx->SUAmaxlat, suaidx->SUAmaxlon, &temprange);
	
			// set reflat / reflon to the centre of the bounding rectangle, radius to half the distance
			suaidx->reflat = (suaidx->SUAminlat + suaidx->SUAmaxlat) / 2.0;
			suaidx->reflon = (suaidx->SUAminlon + suaidx->SUAmaxlon) / 2.0;
			suaidx->maxdist = temprange / 2.0;
//			HostTraceOutputTL(appErrorClass, "     Half range suaidx->reflat=|%s|", DblToStr(suaidx->reflat, 2));
//			HostTraceOutputTL(appErrorClass, "     Half range suaidx->reflon=|%s|", DblToStr(suaidx->reflon, 2));
//			HostTraceOutputTL(appErrorClass, "     temprange is larger, setting maxdist=|%s|", DblToStr(temprange, 2));
			
		}
		suaidx->stopidx = suadataindex;

//		HostTraceOutputTL(appErrorClass, "Update idx");

		OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), suaidx, suaidxindex);

		// save the current SUA header
		MemMove(&savesuaidx, suaidx, sizeof(SUAIndex));

//		HostTraceOutputT(appErrorClass, "type  |%s|", DblToStr(suadata->plottype,0));
//		LLToStringDMS(suadata->lat, tempchar, ISLAT);
//		HostTraceOutputT(appErrorClass, "   Lat |%s|", tempchar);
//		LLToStringDMS(suadata->lon, tempchar, ISLON);
//		HostTraceOutputTL(appErrorClass, "   Lon|%s|", tempchar);
	}

//	HostTraceOutputTL(appErrorClass, "-------------------------------------------------------");
}

//**************************************************************************************************************************************

// Check point is inside or outside a polygon using the winding no. method
Boolean checkinout(double plat, double plong, UInt16 datastart, UInt16 datastop)
{
Boolean exitloop, result;
double  lat0, lon0, lat1, lon1;
Int16   wn;
MemHandle suadata_hand;
MemHandle output_hand;
MemPtr output_ptr;
UInt16 y;
SUAData *suadata;

// link to polygon data
// locate correct SUA polygon in database
//HostTraceOutputTL(appErrorClass, " CHK IO Start=|%s|", DblToStr(datastart,0));
//HostTraceOutputTL(appErrorClass, " CHK IO Stop =|%s|", DblToStr(datastop,0));
y = datastart;

// open suadata database
suadata_hand = MemHandleNew(sizeof(SUAData));
suadata = MemHandleLock(suadata_hand);

// locate first point 
//HostTraceOutputTL(appErrorClass, "First Y=|%s|", DblToStr(y,0));
OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
MemMove(suadata, output_ptr, sizeof(SUAData));
MemHandleUnlock(output_hand);

// initialise variables
exitloop = false;
wn=0;

// set first point lat / lon
lat0 = suadata->lat;
lon0 = suadata->lon;

//HostTraceOutputTL(appErrorClass, "Lat=%s", DblToStr(lat0,2));
//HostTraceOutputTL(appErrorClass, "Lon=%s", DblToStr(lon0,2));

// skip to next record, ignoring SUAARC records
y++;
OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
MemMove(suadata, output_ptr, sizeof(SUAData));
MemHandleUnlock(output_hand);
while ((suadata->plottype == SUAARC) && (y <= datastop)) {
//   HostTraceOutputTL(appErrorClass, "1:ARC Record skip");
   y++;
   OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
   MemMove(suadata, output_ptr, sizeof(SUAData));
   MemHandleUnlock(output_hand);
}
//HostTraceOutputTL(appErrorClass, "Loop Start Y=|%s|", DblToStr(y,0));

// set next point lat / lon
lat1 = suadata->lat;
lon1 = suadata->lon;

// calculate winding number
while ((!exitloop) && (y <= datastop)) {

//	HostTraceOutputTL(appErrorClass, "Y  =%s", DblToStr(y,0));
//	HostTraceOutputTL(appErrorClass, "Lat=%s", DblToStr(lat1,2));
//	HostTraceOutputTL(appErrorClass, "Lon=%s", DblToStr(lon1,2));
   
      // check for crossing and intersect
      if (lat0 <= plat) {
         if (lat1 > plat) {
            if (isleft(lat0,lon0,lat1,lon1,plat,plong) > 0) {
               wn++;
            }
         }
      } else {
         if (lat1 <= plat) {
            if (isleft(lat0,lon0,lat1,lon1,plat,plong) < 0) {
               wn--;
            }
         }   
      }

      // move to next segment, copy first point from previous last point
      lat0 = lat1;
      lon0 = lon1;

      // skip to next record, ignoring SUAARC records, checking for end of polygon
      y++;
      if (y > datastop) { // protect for last record 
//          HostTraceOutputTL(appErrorClass, "y > datastop 1");
          exitloop = true;
      } else {

        OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
        MemMove(suadata, output_ptr, sizeof(SUAData));
        MemHandleUnlock(output_hand);
        while ((suadata->plottype == SUAARC) && (y < datastop)) {
           y++;
           OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
           MemMove(suadata, output_ptr, sizeof(SUAData));
           MemHandleUnlock(output_hand);
        }
      
        if (y > datastop) { // protect for last record 
//            HostTraceOutputTL(appErrorClass, "y > datastop 2");
            exitloop = true;
        } else {
          // set next point lat / lon
          lat1 = suadata->lat;
          lon1 = suadata->lon;
        }
      }
}

// indicate result
if (wn != 0) {
   // Inside
   result = true;
} else {
   // Outside
   result = false;
}

// free memory
//HostTraceOutputTL(appErrorClass, "Freeing Memory");
MemHandleUnlock(suadata_hand);
MemHandleFree(suadata_hand);

return(result);
}

//**************************************************************************************************************************************

double checkdist(double plat, double plong, UInt16 datastart, UInt16 datastop, double warndist)
// Check minimum distance to polygon, returns result in nm squared for speed
// function will exit the loop as soon as any distance < warndist
// if warndist is zero, function calculates the true minimum distance
{
double mindist, newdist;
double SP0lat, SP0long;
double SP1lat, SP1long;
Boolean exitloop;
MemHandle suadata_hand;
MemHandle output_hand;
MemPtr output_ptr;
UInt16 y;
SUAData *suadata;
double templat, templon;

// initialise internal variables
mindist = 999999;
exitloop = false;
warndist = warndist * warndist; // keep as squared for fast comparison

// locate correct SUA polygon in database
//HostTraceOutputTL(appErrorClass, " CHK DST Start=|%s|", DblToStr(datastart,0));
//HostTraceOutputTL(appErrorClass, " CHK DST Stop =|%s|", DblToStr(datastop,0));
y = datastart;

// open suadata database
suadata_hand = MemHandleNew(sizeof(SUAData));
suadata = MemHandleLock(suadata_hand);

// locate first point 
OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
MemMove(suadata, output_ptr, sizeof(SUAData));
MemHandleUnlock(output_hand);
//HostTraceOutputTL(appErrorClass, "1st CHK DST Y=|%s|", DblToStr(y,0));

// set first point lat / lon
SP0lat = suadata->lat;
SP0long = suadata->lon;

// skip to next record, ignoring SUAARC records
y++;
OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
MemMove(suadata, output_ptr, sizeof(SUAData));
MemHandleUnlock(output_hand);
while ((suadata->plottype == SUAARC) && (y < datastop)) {
   y++;
OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
MemMove(suadata, output_ptr, sizeof(SUAData));
MemHandleUnlock(output_hand);
}
//HostTraceOutputTL(appErrorClass, "2nd CHK DST Y=|%s|", DblToStr(y,0));

// set next point lat / lon
SP1lat = suadata->lat;
SP1long = suadata->lon;

// loop through each segment in the polygon
while ((!exitloop) && (y <= datastop)) {

   // Check distance to polygon segment
   newdist = dist2seg(plat,plong,SP0lat,SP0long,SP1lat,SP1long,&templat,&templon);

   // update minimum
   if (newdist < mindist) {
      mindist = newdist;
   }
   
   // check for early exit if any distance inside warning distance
   if (mindist < warndist) {
      exitloop = true;
   }

   // move to next segment SP0 == SP1
   SP0lat = SP1lat;
   SP0long = SP1long;

   // skip to next record, ignoring SUAARC records
   y++;
   if (y > datastop) { // protect for last record 
//        HostTraceOutputTL(appErrorClass, "y > datastop 1");
	exitloop = true;
   } else {
     OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
     MemMove(suadata, output_ptr, sizeof(SUAData));
     MemHandleUnlock(output_hand);
     while ((suadata->plottype == SUAARC) && (y < datastop)) {
        y++;
     OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
     MemMove(suadata, output_ptr, sizeof(SUAData));
     MemHandleUnlock(output_hand);
     }

     if (y > datastop) { // protect for last record 
//       HostTraceOutputTL(appErrorClass, "y > datastop 2");
       exitloop = true;
     } else {
       // set next point lat / lon
       SP1lat = suadata->lat;
       SP1long = suadata->lon;
     }
   }
}

// free memory
MemHandleUnlock(suadata_hand);
MemHandleFree(suadata_hand);

//HostTraceOutputTL(appErrorClass, " CHK DST RES=|%s|", DblToStr(mindist,3));
return(mindist);
}

//**************************************************************************************************************************************

// Isleft fuction for winding number calculation
// is a point on the left hand side of a line
double isleft(double P0lat, double P0lon, double P1lat, double P1lon, double P2lat, double P2lon) 
{
return((P1lon - P0lon) * (P2lat - P0lat)) - ((P2lon - P0lon) * (P1lat - P0lat));
}

// Dot product function
double dot(double P1lat, double P1lon, double P2lat, double P2lon) 
{
return(P1lon * P2lon + P1lat * P2lat);
}

// Check minimum distance squared to polygon segment in nm
double dist2seg(double Plat, double Plong, double SP0lat, double SP0long, double SP1lat, double SP1long, double *ilat, double *ilon)
{
double ulat, ulong, wlat, wlong, b, c1, c2, actdist;
double templat, templon;

// calculate vetors of segment of polygon, and point to first point in segment
ulong = (SP1long - SP0long)*data.input.coslat;
ulat = SP1lat - SP0lat;
wlong = (Plong - SP0long)*data.input.coslat;
wlat = Plat - SP0lat;

c1 = dot(wlat,wlong,ulat,ulong);
if (c1 <=0) { 
   // shortest distance is point to SP0 at one end of line
   actdist = LatLonToRange2(Plat, Plong, SP0lat, SP0long);
   *ilat = SP0lat;
   *ilon = SP0long;

} else {

   c2 = dot(ulat,ulong,ulat,ulong);
   if (c2 <=c1) { 
      // shortest distance is from point to SP1 at other end of line
      actdist = LatLonToRange2(Plat, Plong, SP1lat, SP1long);
      *ilat = SP1lat;
      *ilon = SP1long;

   } else {
      // shortest distance is along the line from SP0 to SP1
      b = c1 / c2;
      templat = SP0lat + b*ulat;
      templon = SP0long + b*ulong/data.input.coslat;
      actdist = LatLonToRange2(Plat, Plong, templat, templon);
      *ilat = templat;
      *ilon = templon;
   }
}
      
return(actdist);
}

void FastBall() // must be called with suaidx data on correct element
{
// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.

// Assume that classes are already given for the objects:
//    Point and Vector with
//        coordinates {float x, y;}
//        operators for:
//            Point  = Point ± Vector
//            Vector = Point - Point
//            Vector = Vector ± Vector
//            Vector = Scalar * Vector    (scalar product)
//            Vector = Vector / Scalar    (scalar division)
//    Ball with a center and radius {Point center; float radius;}
//===================================================================

// fastBall(): a fast approximation of the bounding ball for a point set
//               based on the algorithm given by [Jack Ritter, 1990]
//    Input:  an array V[] of n points
//    Output: a bounding ball = {Point center; float radius;}
 
double rad, rad2, dist, dist2;		
double xmin, xmax, ymin, ymax;      	
Int32 Pxmin, Pxmax, Pymin, Pymax;
Int32 i, y, n;
Point dVx, dVy, C, T, dV;
double dx2, dy2;
Point *V;
MemHandle suadata_hand;
MemHandle output_hand;
MemPtr output_ptr;
Int32 totaldatarecs;
SUAData *suadata;

//HostTraceOutputTL(appErrorClass, "In FastBall Element");
//HostTraceOutputTL(appErrorClass, "Before reflat  %s",DblToStr(suaidx->reflat,2));
//HostTraceOutputTL(appErrorClass, "Before reflon  %s",DblToStr(suaidx->reflon,2));
//HostTraceOutputTL(appErrorClass, "Before maxdist %s",DblToStr(suaidx->maxdist,2));

// open suadata database
suadata_hand = MemHandleNew(sizeof(SUAData));
suadata = MemHandleLock(suadata_hand);

totaldatarecs = (suaidx->stopidx - suaidx->startidx)+1;
if (totaldatarecs > 0) {
	// load points into array V
	// allocate memory to array
	AllocMem((void *)&V, sizeof(Point) * totaldatarecs);
	MemSet(V, sizeof(Point) * totaldatarecs, 0);
	
	// Loop for each SUA data point (need to skip SUAARC types on polygon load
	i = 0;
	for (y=suaidx->startidx; y<(suaidx->stopidx+1); y++) { 
		OpenDBQueryRecord(suadata_db, y, &output_hand, &output_ptr);
		MemMove(suadata, output_ptr, sizeof(SUAData));
		MemHandleUnlock(output_hand);

		if (suadata->plottype != SUAARC) { // ignore actual ARC type, need the decomposed points that follow
			// copy suadata points into poly
			V[i].y = suadata->lat;
			V[i].x = suadata->lon * data.input.coslat;
			i++;
		}
	}
	n = i; // set number of points in array V

	// find a large diameter to start with
	// first get the bounding box and V[] extreme points for it
	xmin = xmax = V[0].x;
	ymin = ymax = V[0].y;
	Pxmin = Pxmax = Pymin = Pymax = 0;
	for (i=1; i<n; i++) {
		if (V[i].x < xmin) {
			xmin = V[i].x;
			Pxmin = i;
		}
		else if (V[i].x > xmax) {
			xmax = V[i].x;
			Pxmax = i;
		}
		if (V[i].y < ymin) {
			ymin = V[i].y;
			Pymin = i;
		}
		else if (V[i].y > ymax) {
			ymax = V[i].y;
			Pymax = i;
		}
	}

	// select the largest extent as an initial diameter for the ball
	//Vector dVx = V[Pxmax] - V[Pxmin]; // diff of Vx max and min
	dVx.x = V[Pxmax].x - V[Pxmin].x;
	dVx.y = V[Pxmax].y - V[Pxmin].y;
	//Vector dVy = V[Pymax] - V[Pymin]; // diff of Vy max and min
	dVy.x = V[Pymax].x - V[Pymin].x;
	dVy.y = V[Pymax].y - V[Pymin].y;

	//float dx2 = norm2(dVx); Vx diff squared
	dx2 = dVx.x * dVx.x + dVx.y * dVx.y;
	//float dy2 = norm2(dVy); Vy diff squared
	dy2 = dVy.x * dVy.x + dVy.y * dVy.y;

	if (dx2 >= dy2) {                     // x direction is largest extent
		//C = V[Pxmin] + (dVx / 2.0);         Center = midpoint of extremes
		C.x = V[Pxmin].x + (dVx.x / 2.0);
		C.y = V[Pxmin].y + (dVx.y / 2.0);
		//rad2 = norm2(V[Pxmax] - C);         radius squared
		T.x = V[Pxmax].x - C.x;
		T.y = V[Pxmax].y - C.y;
		rad2 = T.x * T.x + T.y * T.y;
	}
	else {                                // y direction is largest extent
		//C = V[Pymin] + (dVy / 2.0);          Center = midpoint of extremes
		C.x = V[Pymin].x + (dVy.x / 2.0);
		C.y = V[Pymin].y + (dVy.y / 2.0);
		//rad2 = norm2(V[Pymax] - C);          radius squared
		T.x = V[Pymax].x - C.x;
		T.y = V[Pymax].y - C.y;
		rad2 = T.x * T.x + T.y * T.y;
	}
	rad = Sqrt(rad2);

	// now check that all points V[i] are in the ball
	// and if not, expand the ball just enough to include them
	// Vector dV;
	// float dist, dist2;
	for (i=0; i<n; i++) {
		//dV = V[i] - C;
		dV.x = V[i].x - C.x;
		dV.y = V[i].y - C.y;
		//dist2 = norm2(dV);
		dist2 = dV.x * dV.x + dV.y * dV.y;
		if (dist2 > rad2) {
			// V[i] not in ball, so expand ball to include it
			dist = Sqrt(dist2);
			rad = (rad + dist) / 2.0;         // enlarge radius just enough
			rad2 = rad * rad;
			//C = C + ((dist-rad)/dist) * dV;    shift Center toward V[i]
			C.x = C.x + ((dist-rad)/dist) * dV.x;
			C.y = C.y + ((dist-rad)/dist) * dV.y;
		}	
	}

	// update bounding ball in suaidx
//	HostTraceOutputTL(appErrorClass, "After  reflat  %s",DblToStr(C.y,2));
//	HostTraceOutputTL(appErrorClass, "After  reflon  %s",DblToStr(C.x / data.input.coslat,2));
//	HostTraceOutputTL(appErrorClass, "After  maxdist %s",DblToStr(rad * 60.0,2));
//	HostTraceOutputTL(appErrorClass, " ");

	suaidx->reflat = C.y;
	suaidx->reflon = C.x / data.input.coslat;
	suaidx->maxdist = rad * 60.0;

	// free memory
	FreeMem((void *)&V);

	}

// free memory
MemHandleUnlock(suadata_hand);
MemHandleFree(suadata_hand);		

return;
}

// Convert the current type to a string
// maxlen is the maximum length of the resulting string
// shortstr is a boolean to choose the short or long output format
void GetSUATypeStr(Char *TypeString, Int32 type, Int16 maxlen, Boolean shortstr)
{
		// decode SUA type to text
	switch (type) {
		case CTACTR:
			if (shortstr) 
				StrNCopy(TypeString,"CTA/R", maxlen);
			else 
				StrNCopy(TypeString,"CTA / CTR ", maxlen);
			break;
		case AIRWAYS:
			if (shortstr) 
				StrNCopy(TypeString,"ARWAY", maxlen);
			else 
				StrNCopy(TypeString,"Airway    ", maxlen);
			break;
		case RESTRICTED:
			if (shortstr) 
				StrNCopy(TypeString,"RESTR", maxlen);
			else 
				StrNCopy(TypeString,"Restricted", maxlen);
			break;
		case PROHIBITED:
			if (shortstr) 
				StrNCopy(TypeString,"PROHB", maxlen);
			else 
				StrNCopy(TypeString,"Prohibited", maxlen);
			break;
		case DANGER:
			if (shortstr) 
				StrNCopy(TypeString,"DANGR", maxlen);
			else 
				StrNCopy(TypeString,"Danger    ", maxlen);
			break;
		case OTHER:
			if (shortstr) 
				StrNCopy(TypeString,"OTHER", maxlen);
			else 
				StrNCopy(TypeString,"Other     ", maxlen);
			break;
		case TRAINING:
			if (shortstr) 
				StrNCopy(TypeString,"TRAIN", maxlen);
			else 
				StrNCopy(TypeString,"Training  ", maxlen);
			break;
		case TRAFFIC:
			if (shortstr) 
				StrNCopy(TypeString,"TRAFC", maxlen);
			else 
				StrNCopy(TypeString,"Traffic   ", maxlen);
			break;
		case CLASSB:
			if (shortstr) 
				StrNCopy(TypeString,"CLS-B", maxlen);
			else 
				StrNCopy(TypeString,"Class B   ", maxlen);
			break;
		case CLASSC:
			if (shortstr) 
				StrNCopy(TypeString,"CLS-C", maxlen);
			else 
				StrNCopy(TypeString,"Class C   ", maxlen);
			break;
		case CLASSD:
			if (shortstr) 
				StrNCopy(TypeString,"CLS-D", maxlen);
			else 
				StrNCopy(TypeString,"Class D   ", maxlen);
			break;
		case CLASSE:
			if (shortstr) 
				StrNCopy(TypeString,"CLS-E", maxlen);
			else 
				StrNCopy(TypeString,"Class E   ", maxlen);
			break;
		case ALERT:
			if (shortstr) 
				StrNCopy(TypeString,"ALERT", maxlen);
			else 
				StrNCopy(TypeString,"Alert     ", maxlen);
			break;
		case MOA:
			if (shortstr) 
				StrNCopy(TypeString,"MOA", maxlen);
			else 
				StrNCopy(TypeString,"MOA       ", maxlen);
			break;
		case GSEC:
			if (shortstr) 
				StrNCopy(TypeString,"GSEC", maxlen);
			else 
				StrNCopy(TypeString,"Glider Sec", maxlen);
			break;
		case TMA:
			if (shortstr) 
				StrNCopy(TypeString,"TMA", maxlen);
			else 
				StrNCopy(TypeString,"TMA       ", maxlen);
			break;
		case TEMPOR:
			if (shortstr) 
				StrNCopy(TypeString,"TEMP", maxlen);
			else 
				StrNCopy(TypeString,"TEMPORARY ", maxlen);
			break;
		case WARNING:
			if (shortstr) 
				StrNCopy(TypeString,"WARN", maxlen);
			else 
				StrNCopy(TypeString,"WARNING   ", maxlen);
			break;
		case CLASSA:
			if (shortstr) 
				StrNCopy(TypeString,"CLS-A", maxlen);
			else 
				StrNCopy(TypeString,"Class A   ", maxlen);
			break;
		case CLASSF:
			if (shortstr) 
				StrNCopy(TypeString,"CLS-F", maxlen);
			else 
				StrNCopy(TypeString,"Class F   ", maxlen);
			break;
		case CLASSG:
			if (shortstr) 
				StrNCopy(TypeString,"CLS-G", maxlen);
			else 
				StrNCopy(TypeString,"Class G   ", maxlen);
			break;
		case TMZ:
			if (shortstr) 
				StrNCopy(TypeString,"TMZ", maxlen);
			else 
				StrNCopy(TypeString,"TMZ       ", maxlen);
			break;
		case MATZ:
			if (shortstr) 
				StrNCopy(TypeString,"MATZ", maxlen);
			else 
				StrNCopy(TypeString,"MATZ      ", maxlen);
			break;
		case TFR:
			if (shortstr) 
				StrNCopy(TypeString,"TFR", maxlen);
			else 
				StrNCopy(TypeString,"TFR       ", maxlen);
			break;
		case BOUNDARY:
			if (shortstr) 
				StrNCopy(TypeString,"BOUND", maxlen);
			else 
				StrNCopy(TypeString,"Boundary  ", maxlen);
			break;
		case SUAERROR:
			if (shortstr) 
				StrNCopy(TypeString,"ERROR", maxlen);
			else 
				StrNCopy(TypeString,"SUA ERROR ", maxlen);
			break;
		default:
			if (shortstr) 
				StrNCopy(TypeString,"UNK", maxlen);
			else 
				StrNCopy(TypeString,"Unknown   ", maxlen);
			break;
	}
	return;
}

void GetSUAClassStr(Char *TypeString, Int32 class)
{
		// decode SUA class to text
	switch (class) {
		case CLASSA:
			StrCopy(TypeString,"A");
			break;
		case CLASSB:
			StrCopy(TypeString,"B");
			break;
		case CLASSC:
			StrCopy(TypeString,"C");
			break;
		case CLASSD:
			StrCopy(TypeString,"D");
			break;
		case CLASSE:
			StrCopy(TypeString,"E");
			break;
		case CLASSF:
			StrCopy(TypeString,"F");
			break;
		case CLASSG:
			StrCopy(TypeString,"G");
			break;
		default:
			StrCopy(TypeString," ");
			break;
	}
	return;
}

// decode tops and base types to text
// maxlen should include null terminator
void GetSUAALTTypeStr(Char *TypeString, Int32 type, double alt, Int8 units, Int16 maxlen)
{
	Char tempchar[5];
	switch (type) {
		case SUA_ALT:
			StrCopy(tempchar," ALT");
			break;
		case SUA_FL:
			StrCopy(tempchar," FL ");
			break;
		case SUA_AGL:
			StrCopy(tempchar," AGL");
			break;
		default:
			StrCopy(tempchar," ALT");
			break;
	}			
	if (alt == SFC) {
		StrNCopy(TypeString, "Surface", maxlen);
	} else if (alt == UNLIMITED) {
		StrNCopy(TypeString, "Unlimited", maxlen);
	} else if (type == SUA_FL) {
		StrNCopy(TypeString, tempchar, maxlen);
		StrNCat(TypeString, DblToStr(alt / 100.0, 0), maxlen);
	} else {
		StrNCopy(TypeString, print_SUA_altitude(alt, units), maxlen);
		StrNCat(TypeString, tempchar, maxlen);
	}
	return;
}

Int16 sua_comparison(SUAIndex* rec1, SUAIndex* rec2, Int16 order, SortRecordInfoPtr rec1SortInfo, SortRecordInfoPtr rec2SortInfo, MemHandle h)
{
	double dist1=0, dist2=0;
	//Not sure it should be initialized to zero
	Int16 retval=0;

	if (rec1->WarnType == rec2->WarnType) {
		// same SUA warning level
		switch ( order ) {
			case SUASortByTitleA:
				// compare the title of the two sua items in ascending order
				retval = (StrCompare(rec1->title, rec2->title));
				break;
			case SUASortByTitleD:
				// compare the title of the two sua items in descending order
				retval = (StrCompare(rec2->title, rec1->title));
				break;
			case SUASortByActiveA: // Y before D before N
				if (((Int16)rec1->active == (Int16)rec2->active) && ((Int16)rec1->WarnActive == (Int16)rec2->WarnActive)) {
					retval = (StrCompare(rec1->title, rec2->title));
				} else if ((Int16)rec1->active > (Int16)rec2->active) {
					retval = -1;
				} else if ((Int16)rec1->WarnActive > (Int16)rec2->WarnActive) {
					retval = -1;
				} else {
					retval = 1;
				}
				break;
			case SUASortByActiveD: // N before D before Y
				if (((Int16)rec1->active == (Int16)rec2->active) && ((Int16)rec1->WarnActive == (Int16)rec2->WarnActive)) {
					retval = (StrCompare(rec1->title, rec2->title));
				} else if ((Int16)rec2->active > (Int16)rec1->active) {
					retval = -1;
				} else if ((Int16)rec2->WarnActive > (Int16)rec1->WarnActive) {
					retval = -1;
				} else {
					retval = 1;
				}
				break;
			case SUASortByTypeA:
				if (rec1->type == rec2->type) {
					retval = (StrCompare(rec1->title, rec2->title));
				} else if (rec1->type < rec2->type) {
					retval = -1;
				} else {
					retval = 1;
				}
				break;
			case SUASortByTypeD:
				if (rec1->type == rec2->type) {
					retval = (StrCompare(rec1->title, rec2->title));
				} else if (rec1->type > rec2->type) {
					retval = -1;
				} else {
					retval = 1;
				}
				break;
			case SUASortByDistD:
/*				//LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, rec1->reflat, rec1->reflon, &dist1);
				//LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, rec2->reflat, rec2->reflon, &dist2);
				dist1 = rec1->neardist;
				dist2 = rec2->neardist;

				if (dist1 == dist2) {
					retval = 0;
				} else if (dist2 < dist1) {
					retval = -1;
				} else {
					retval = 1;
				}
				break;
*/
			case SUASortByDistA:
				//LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, rec1->reflat, rec1->reflon, &dist1);
				//LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, rec2->reflat, rec2->reflon, &dist2);
				dist1 = rec1->neardist;
				dist2 = rec2->neardist;

				if (dist1 == dist2) {
					retval = 0;
				} else if (dist1 < dist2) {
					retval = -1;
				} else {
					retval = 1;
				}
				break;
		}
	} else {
		// different SUA warning level
		if (rec1->WarnType > rec2->WarnType) {
			retval = -1;
		} else {
			retval = 1;
		}
	}
	
/*
	// Not needed for a DmInsertionSort
	// Left here for future reference
	if (retval == 0) {
		if ( rec1SortInfo->uniqueID > rec2SortInfo->uniqueID ) {
			retval = 1;
		} else if ( rec1SortInfo->uniqueID < rec2SortInfo->uniqueID ) {
			retval = -1;
		}
	}
*/
	return (retval);
}

Int16 FindSUARecordByName(Char* NameString) 
{
	Int16 suaindex = 0;
	Int16 nrecs;
	SUAIndex *suadata;
	MemHandle suahand;
	MemHandle output_hand;
	MemPtr output_ptr;
	Int16 retval=-1;

	nrecs = OpenDBCountRecords(suaidx_db);

	suahand = MemHandleNew(sizeof(SUAIndex));
	suadata = MemHandleLock(suahand);
	for (suaindex=0; suaindex<nrecs; suaindex++) {
		OpenDBQueryRecord(suaidx_db, suaindex, &output_hand, &output_ptr);
		MemMove(suadata, output_ptr, sizeof(SUAIndex));
		MemHandleUnlock(output_hand);

		if (StrCompare(NameString, suadata->title) == 0) {
			retval = suaindex;
			suaindex = nrecs;
		}
	}
	MemHandleUnlock(suahand);
	MemHandleFree(suahand);
	return(retval);
}

Boolean form_set_sua_event_handler(EventPtr event)
{
	Boolean handled=false;
	Char tempchar[15];

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(FrmGetActiveForm());
			ctl_set_value(form_set_sua_checksua, data.config.CheckSUAWarn);
			if (!data.config.CheckSUAWarn) {
				ctl_set_value(form_set_sua_disponlywarned, false);
				ctl_set_value(form_set_sua_highlightwarned, false);
				ctl_set_enable(form_set_sua_disponlywarned, false);
				ctl_set_enable(form_set_sua_highlightwarned, false);
				ctl_set_enable(form_set_sua_warnbtn, false);
			} else {
				ctl_set_value(form_set_sua_disponlywarned, data.config.SUAdisponlywarned);
				ctl_set_value(form_set_sua_highlightwarned, data.config.SUAhighlightwarned);
				ctl_set_enable(form_set_sua_disponlywarned, true);
				ctl_set_enable(form_set_sua_highlightwarned, true);
				ctl_set_enable(form_set_sua_warnbtn, true);
			}
			field_set_value(form_set_sua_dismisstime, StrIToA(tempchar,(UInt32)data.config.autodismisstime));
			ctl_set_value(form_set_sua_autostart, data.config.autostart);
			ctl_set_value(form_set_sua_finalglide, data.config.FGalert);
			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Set SUA menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Set SUA menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent: 
			data.config.autodismisstime = (UInt32)Fabs(field_get_double(form_set_sua_dismisstime));
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_sua_dispbtn:
					FrmGotoForm(form_set_suadisp);
					handled = true;
					break;
				case form_set_sua_warnbtn:
					FrmGotoForm(form_set_suawarn);
					handled = true;
					break;
				case form_set_sua_checksua:
					data.config.CheckSUAWarn = ctl_get_value(form_set_sua_checksua);
					if (!data.config.CheckSUAWarn) {
						ctl_set_value(form_set_sua_disponlywarned, false);
						ctl_set_value(form_set_sua_highlightwarned, false);
						ctl_set_enable(form_set_sua_disponlywarned, false);
						ctl_set_enable(form_set_sua_highlightwarned, false);
						ctl_set_enable(form_set_sua_warnbtn, false);
					} else {
						ctl_set_value(form_set_sua_disponlywarned, data.config.SUAdisponlywarned);
						ctl_set_value(form_set_sua_highlightwarned, data.config.SUAhighlightwarned);
						ctl_set_enable(form_set_sua_disponlywarned, true);
						ctl_set_enable(form_set_sua_highlightwarned, true);
						ctl_set_enable(form_set_sua_warnbtn, true);
					}
					handled = true;
					break;
				case form_set_sua_disponlywarned:
					data.config.SUAdisponlywarned = ctl_get_value(form_set_sua_disponlywarned);
					handled = true;
					break;
				case form_set_sua_highlightwarned:
					data.config.SUAhighlightwarned = ctl_get_value(form_set_sua_highlightwarned);
					handled = true;
					break;
				case form_set_sua_autostart:
					data.config.autostart = ctl_get_value(form_set_sua_autostart);
					handled = true;
					break;
				case form_set_sua_finalglide:
					data.config.FGalert = ctl_get_value(form_set_sua_finalglide);
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

Boolean form_set_suadisp_event_handler(EventPtr event)
{
	Boolean handled=false;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(FrmGetActiveForm());
			if (data.config.suaactivetypes & CTACTR) ctl_set_value(form_set_suadisp_ctabtn, true);
			if (data.config.suaactivetypes & AIRWAYS) ctl_set_value(form_set_suadisp_airbtn, true);
			if (data.config.suaactivetypes & RESTRICTED) ctl_set_value(form_set_suadisp_resbtn, true);
			if (data.config.suaactivetypes & PROHIBITED) ctl_set_value(form_set_suadisp_probtn, true);
			if (data.config.suaactivetypes & DANGER) ctl_set_value(form_set_suadisp_danbtn, true);
			if (data.config.suaactivetypes & OTHER) ctl_set_value(form_set_suadisp_othbtn, true);
			if (data.config.suaactivetypes & TRAINING) ctl_set_value(form_set_suadisp_trnbtn, true);
			if (data.config.suaactivetypes & TRAFFIC) ctl_set_value(form_set_suadisp_trabtn, true);
			if (data.config.suaactivetypes & CLASSB) ctl_set_value(form_set_suadisp_clbbtn, true);
			if (data.config.suaactivetypes & CLASSC) ctl_set_value(form_set_suadisp_clcbtn, true);
			if (data.config.suaactivetypes & CLASSD) ctl_set_value(form_set_suadisp_cldbtn, true);
			if (data.config.suaactivetypes & CLASSE) ctl_set_value(form_set_suadisp_clebtn, true);
			if (data.config.suaactivetypes & ALERT) ctl_set_value(form_set_suadisp_alrbtn, true);
			if (data.config.suaactivetypes & MOA) ctl_set_value(form_set_suadisp_moabtn, true);
			if (data.config.suaactivetypes & GSEC) ctl_set_value(form_set_suadisp_gsecbtn, true);
			if (data.config.suaactivetypes & TMA) ctl_set_value(form_set_suadisp_tmabtn, true);
			if (data.config.suaactivetypes & CLASSA) ctl_set_value(form_set_suadisp_clabtn, true);
			if (data.config.suaactivetypes & TEMPOR) ctl_set_value(form_set_suadisp_tmpbtn, true);
			if (data.config.suaactivetypes & WARNING) ctl_set_value(form_set_suadisp_warbtn, true);
			if (data.config.suaactivetypes & CLASSF) ctl_set_value(form_set_suadisp_clfbtn, true);

			if (data.config.suaactivetypes & CLASSG) ctl_set_value(form_set_suadisp_clgbtn, true);
			if (data.config.suaactivetypes & TMZ) ctl_set_value(form_set_suadisp_tmzbtn, true);
			if (data.config.suaactivetypes & MATZ) ctl_set_value(form_set_suadisp_mtzbtn, true);
			if (data.config.suaactivetypes & BOUNDARY) ctl_set_value(form_set_suadisp_bndbtn, true);
			//if (data.config.suaactivetypes & TFR) ctl_set_value(form_set_suadisp_tfrbtn, true);

			field_set_value(form_set_suadisp_maxalt, print_altitude(data.config.SUAmaxalt));
			field_set_value(form_set_suadisp_alt, print_altitude(data.config.SUAdispalt));
			ctl_set_value(form_set_suadisp_keepSUA, data.config.keepSUA);
			WinDrawChars(data.input.alttext, 2, 65, 132+3);
			WinDrawChars(data.input.alttext, 2, 145, 132+3);
			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Set SUA Disp menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Set SUA Dis menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent: 
			data.config.suaactivetypes = SUANONE;
			if (ctl_get_value(form_set_suadisp_ctabtn)) data.config.suaactivetypes |= CTACTR;
			if (ctl_get_value(form_set_suadisp_airbtn)) data.config.suaactivetypes |= AIRWAYS;
			if (ctl_get_value(form_set_suadisp_resbtn)) data.config.suaactivetypes |= RESTRICTED;
			if (ctl_get_value(form_set_suadisp_probtn)) data.config.suaactivetypes |= PROHIBITED;
			if (ctl_get_value(form_set_suadisp_danbtn)) data.config.suaactivetypes |= DANGER;
			if (ctl_get_value(form_set_suadisp_othbtn)) data.config.suaactivetypes |= OTHER;
			if (ctl_get_value(form_set_suadisp_trnbtn)) data.config.suaactivetypes |= TRAINING;
			if (ctl_get_value(form_set_suadisp_trabtn)) data.config.suaactivetypes |= TRAFFIC;
			if (ctl_get_value(form_set_suadisp_clbbtn)) data.config.suaactivetypes |= CLASSB;
			if (ctl_get_value(form_set_suadisp_clcbtn)) data.config.suaactivetypes |= CLASSC;
			if (ctl_get_value(form_set_suadisp_cldbtn)) data.config.suaactivetypes |= CLASSD;
			if (ctl_get_value(form_set_suadisp_clebtn)) data.config.suaactivetypes |= CLASSE;
			if (ctl_get_value(form_set_suadisp_alrbtn)) data.config.suaactivetypes |= ALERT;
			if (ctl_get_value(form_set_suadisp_moabtn)) data.config.suaactivetypes |= MOA;
			if (ctl_get_value(form_set_suadisp_gsecbtn)) data.config.suaactivetypes |= GSEC;
			if (ctl_get_value(form_set_suadisp_tmabtn)) data.config.suaactivetypes |= TMA;
			if (ctl_get_value(form_set_suadisp_clabtn)) data.config.suaactivetypes |= CLASSA;
			if (ctl_get_value(form_set_suadisp_tmpbtn)) data.config.suaactivetypes |= TEMPOR;
			if (ctl_get_value(form_set_suadisp_warbtn)) data.config.suaactivetypes |= WARNING;
			if (ctl_get_value(form_set_suadisp_clfbtn)) data.config.suaactivetypes |= CLASSF;

			if (ctl_get_value(form_set_suadisp_clgbtn)) data.config.suaactivetypes |= CLASSG;
			if (ctl_get_value(form_set_suadisp_tmzbtn)) data.config.suaactivetypes |= TMZ;
			if (ctl_get_value(form_set_suadisp_mtzbtn)) data.config.suaactivetypes |= MATZ;
			if (ctl_get_value(form_set_suadisp_bndbtn)) data.config.suaactivetypes |= BOUNDARY;
			//if (ctl_get_value(form_set_suadisp_tfrbtn)) data.config.suaactivetypes |= TFR;
			data.config.suaactivetypes |= TFR;

			// setup SUA actice classes
			SUAdrawclasses = SUANONE;
			if (data.config.suaactivetypes & CLASSA) SUAdrawclasses |= CLASSA;
			if (data.config.suaactivetypes & CLASSB) SUAdrawclasses |= CLASSB;
			if (data.config.suaactivetypes & CLASSC) SUAdrawclasses |= CLASSC;
			if (data.config.suaactivetypes & CLASSD) SUAdrawclasses |= CLASSD;
			if (data.config.suaactivetypes & CLASSE) SUAdrawclasses |= CLASSE;
			if (data.config.suaactivetypes & CLASSF) SUAdrawclasses |= CLASSF;
			if (data.config.suaactivetypes & CLASSG) SUAdrawclasses |= CLASSG;
			SUAdrawclasses |= CLASSX;

			data.config.SUAmaxalt = field_get_double(form_set_suadisp_maxalt)/data.input.altconst;
			data.config.SUAdispalt = field_get_double(form_set_suadisp_alt)/data.input.altconst;
			if (data.config.SUAdispalt < data.config.vert_dist) {
				data.config.SUAdispalt = data.config.vert_dist;
			}
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_suadisp_allon:
					data.config.suaactivetypes = SUAALL;
					ctl_set_value(form_set_suadisp_ctabtn, true);
					ctl_set_value(form_set_suadisp_airbtn, true);
					ctl_set_value(form_set_suadisp_resbtn, true);
					ctl_set_value(form_set_suadisp_probtn, true);
					ctl_set_value(form_set_suadisp_danbtn, true);
					ctl_set_value(form_set_suadisp_othbtn, true);
					ctl_set_value(form_set_suadisp_trnbtn, true);
					ctl_set_value(form_set_suadisp_trabtn, true);
					ctl_set_value(form_set_suadisp_clbbtn, true);
					ctl_set_value(form_set_suadisp_clcbtn, true);
					ctl_set_value(form_set_suadisp_cldbtn, true);
					ctl_set_value(form_set_suadisp_clebtn, true);
					ctl_set_value(form_set_suadisp_alrbtn, true);
					ctl_set_value(form_set_suadisp_moabtn, true);
					ctl_set_value(form_set_suadisp_gsecbtn, true);
					ctl_set_value(form_set_suadisp_tmabtn, true);
					ctl_set_value(form_set_suadisp_clabtn, true);
					ctl_set_value(form_set_suadisp_tmpbtn, true);
					ctl_set_value(form_set_suadisp_warbtn, true);
					ctl_set_value(form_set_suadisp_clfbtn, true);
					ctl_set_value(form_set_suadisp_clgbtn, true);
					ctl_set_value(form_set_suadisp_tmzbtn, true);
					ctl_set_value(form_set_suadisp_mtzbtn, true);
					ctl_set_value(form_set_suadisp_bndbtn, true);
					//ctl_set_value(form_set_suadisp_tfrbtn, true);
					handled = true;
					break;
				case form_set_suadisp_alloff:
					data.config.suaactivetypes = SUANONE;
					ctl_set_value(form_set_suadisp_ctabtn, false);
					ctl_set_value(form_set_suadisp_airbtn, false);
					ctl_set_value(form_set_suadisp_resbtn, false);
					ctl_set_value(form_set_suadisp_probtn, false);
					ctl_set_value(form_set_suadisp_danbtn, false);
					ctl_set_value(form_set_suadisp_othbtn, false);
					ctl_set_value(form_set_suadisp_trnbtn, false);
					ctl_set_value(form_set_suadisp_trabtn, false);
					ctl_set_value(form_set_suadisp_clbbtn, false);
					ctl_set_value(form_set_suadisp_clcbtn, false);
					ctl_set_value(form_set_suadisp_cldbtn, false);
					ctl_set_value(form_set_suadisp_clebtn, false);
					ctl_set_value(form_set_suadisp_alrbtn, false);
					ctl_set_value(form_set_suadisp_moabtn, false);
					ctl_set_value(form_set_suadisp_gsecbtn, false);
					ctl_set_value(form_set_suadisp_tmabtn, false);
					ctl_set_value(form_set_suadisp_clabtn, false);
					ctl_set_value(form_set_suadisp_tmpbtn, false);
					ctl_set_value(form_set_suadisp_warbtn, false);
					ctl_set_value(form_set_suadisp_clfbtn, false);
					ctl_set_value(form_set_suadisp_clgbtn, false);
					ctl_set_value(form_set_suadisp_tmzbtn, false);
					ctl_set_value(form_set_suadisp_mtzbtn, false);
					ctl_set_value(form_set_suadisp_bndbtn, false);
					//ctl_set_value(form_set_suadisp_tfrbtn, false);
					handled = true;
					break;
				case form_set_suadisp_exitbtn:
					FrmGotoForm(form_set_sua);
					handled = true;
					break;
				case form_set_suadisp_keepSUA:
					data.config.keepSUA = ctl_get_value(form_set_suadisp_keepSUA);
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

Boolean form_set_suawarn_event_handler(EventPtr event)
{
	Boolean handled=false;
	Char tempchar[15];

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(FrmGetActiveForm());
			if (data.config.suawarntypes & CTACTR) ctl_set_value(form_set_suawarn_ctabtn, true);
			if (data.config.suawarntypes & AIRWAYS) ctl_set_value(form_set_suawarn_airbtn, true);
			if (data.config.suawarntypes & RESTRICTED) ctl_set_value(form_set_suawarn_resbtn, true);
			if (data.config.suawarntypes & PROHIBITED) ctl_set_value(form_set_suawarn_probtn, true);
			if (data.config.suawarntypes & DANGER) ctl_set_value(form_set_suawarn_danbtn, true);
			if (data.config.suawarntypes & OTHER) ctl_set_value(form_set_suawarn_othbtn, true);
			if (data.config.suawarntypes & TRAINING) ctl_set_value(form_set_suawarn_trnbtn, true);
			if (data.config.suawarntypes & TRAFFIC) ctl_set_value(form_set_suawarn_trabtn, true);
			if (data.config.suawarntypes & CLASSB) ctl_set_value(form_set_suawarn_clbbtn, true);
			if (data.config.suawarntypes & CLASSC) ctl_set_value(form_set_suawarn_clcbtn, true);
			if (data.config.suawarntypes & CLASSD) ctl_set_value(form_set_suawarn_cldbtn, true);
			if (data.config.suawarntypes & CLASSE) ctl_set_value(form_set_suawarn_clebtn, true);
			if (data.config.suawarntypes & ALERT) ctl_set_value(form_set_suawarn_alrbtn, true);
			if (data.config.suawarntypes & MOA) ctl_set_value(form_set_suawarn_moabtn, true);
			if (data.config.suawarntypes & GSEC) ctl_set_value(form_set_suawarn_gsecbtn, true);
			if (data.config.suawarntypes & TMA) ctl_set_value(form_set_suawarn_tmabtn, true);
			if (data.config.suawarntypes & CLASSA) ctl_set_value(form_set_suawarn_clabtn, true);
			if (data.config.suawarntypes & TEMPOR) ctl_set_value(form_set_suawarn_tmpbtn, true);
			if (data.config.suawarntypes & WARNING) ctl_set_value(form_set_suawarn_warbtn, true);
			if (data.config.suawarntypes & CLASSF) ctl_set_value(form_set_suawarn_clfbtn, true);

			if (data.config.suawarntypes & CLASSG) ctl_set_value(form_set_suawarn_clgbtn, true);
			if (data.config.suawarntypes & TMZ) ctl_set_value(form_set_suawarn_tmzbtn, true);
			if (data.config.suawarntypes & MATZ) ctl_set_value(form_set_suawarn_mtzbtn, true);
			if (data.config.suawarntypes & BOUNDARY) ctl_set_value(form_set_suawarn_bndbtn, true);
			//if (data.config.suawarntypes & TFR) ctl_set_value(form_set_suawarn_tfrbtn, true);

			field_set_value(form_set_suawarn_horizdist, print_distance2(data.config.horiz_dist, 2));
			field_set_value(form_set_suawarn_vertdist, print_altitude(data.config.vert_dist));
			field_set_value(form_set_suawarn_rewarn,DblToStr(data.config.SUArewarn*100.0,0));
			field_set_value(form_set_suawarn_lookahead,StrIToA(tempchar,(UInt32)data.config.SUAlookahead));
			WinDrawChars(data.input.distext, 2, 43, 148);
			WinDrawChars(data.input.alttext, 2, 96, 148);
			WinDrawChars("%",1,146,148);
			handled=true;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Set SUA Warn menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Set SUA Warn menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent: 
			data.config.suawarntypes = SUANONE;
			if (ctl_get_value(form_set_suawarn_ctabtn)) data.config.suawarntypes |= CTACTR;
			if (ctl_get_value(form_set_suawarn_airbtn)) data.config.suawarntypes |= AIRWAYS;
			if (ctl_get_value(form_set_suawarn_resbtn)) data.config.suawarntypes |= RESTRICTED;
			if (ctl_get_value(form_set_suawarn_probtn)) data.config.suawarntypes |= PROHIBITED;
			if (ctl_get_value(form_set_suawarn_danbtn)) data.config.suawarntypes |= DANGER;
			if (ctl_get_value(form_set_suawarn_othbtn)) data.config.suawarntypes |= OTHER;
			if (ctl_get_value(form_set_suawarn_trnbtn)) data.config.suawarntypes |= TRAINING;
			if (ctl_get_value(form_set_suawarn_trabtn)) data.config.suawarntypes |= TRAFFIC;
			if (ctl_get_value(form_set_suawarn_clbbtn)) data.config.suawarntypes |= CLASSB;
			if (ctl_get_value(form_set_suawarn_clcbtn)) data.config.suawarntypes |= CLASSC;
			if (ctl_get_value(form_set_suawarn_cldbtn)) data.config.suawarntypes |= CLASSD;
			if (ctl_get_value(form_set_suawarn_clebtn)) data.config.suawarntypes |= CLASSE;
			if (ctl_get_value(form_set_suawarn_alrbtn)) data.config.suawarntypes |= ALERT;
			if (ctl_get_value(form_set_suawarn_moabtn)) data.config.suawarntypes |= MOA;
			if (ctl_get_value(form_set_suawarn_gsecbtn)) data.config.suawarntypes |= GSEC;
			if (ctl_get_value(form_set_suawarn_tmabtn)) data.config.suawarntypes |= TMA;
			if (ctl_get_value(form_set_suawarn_clabtn)) data.config.suawarntypes |= CLASSA;
			if (ctl_get_value(form_set_suawarn_tmpbtn)) data.config.suawarntypes |= TEMPOR;
			if (ctl_get_value(form_set_suawarn_warbtn)) data.config.suawarntypes |= WARNING;
			if (ctl_get_value(form_set_suawarn_clfbtn)) data.config.suawarntypes |= CLASSF;

			if (ctl_get_value(form_set_suawarn_clgbtn)) data.config.suawarntypes |= CLASSG;
			if (ctl_get_value(form_set_suawarn_tmzbtn)) data.config.suawarntypes |= TMZ;
			if (ctl_get_value(form_set_suawarn_mtzbtn)) data.config.suawarntypes |= MATZ;
			if (ctl_get_value(form_set_suawarn_bndbtn)) data.config.suawarntypes |= BOUNDARY;
			//if (ctl_get_value(form_set_suawarn_tfrbtn)) data.config.suawarntypes |= TFR;
			data.config.suawarntypes |= TFR;

			// setup SUA actice classes
			SUAwarnclasses = SUANONE;
			if (data.config.suawarntypes & CLASSA) SUAwarnclasses |= CLASSA;
			if (data.config.suawarntypes & CLASSB) SUAwarnclasses |= CLASSB;
			if (data.config.suawarntypes & CLASSC) SUAwarnclasses |= CLASSC;
			if (data.config.suawarntypes & CLASSD) SUAwarnclasses |= CLASSD;
			if (data.config.suawarntypes & CLASSE) SUAwarnclasses |= CLASSE;
			if (data.config.suawarntypes & CLASSF) SUAwarnclasses |= CLASSF;
			if (data.config.suawarntypes & CLASSG) SUAwarnclasses |= CLASSG;
			SUAwarnclasses |= CLASSX;

			data.config.horiz_dist = field_get_double(form_set_suawarn_horizdist)/data.input.disconst;
			data.config.vert_dist = field_get_double(form_set_suawarn_vertdist)/data.input.altconst;
			data.config.SUArewarn = field_get_double(form_set_suawarn_rewarn)/100.0;
			data.config.SUAlookahead = (UInt32)Fabs(field_get_double(form_set_suawarn_lookahead));
			if (data.config.SUArewarn < 0) {
				data.config.SUArewarn = 0.0;
			}
			if (data.config.SUArewarn > 1.0) {
				data.config.SUArewarn = 1.0;
			}
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_suawarn_allon:
					data.config.suawarntypes = SUAALL;
					ctl_set_value(form_set_suawarn_ctabtn, true);
					ctl_set_value(form_set_suawarn_airbtn, true);
					ctl_set_value(form_set_suawarn_resbtn, true);
					ctl_set_value(form_set_suawarn_probtn, true);
					ctl_set_value(form_set_suawarn_danbtn, true);
					ctl_set_value(form_set_suawarn_othbtn, true);
					ctl_set_value(form_set_suawarn_trnbtn, true);
					ctl_set_value(form_set_suawarn_trabtn, true);
					ctl_set_value(form_set_suawarn_clbbtn, true);
					ctl_set_value(form_set_suawarn_clcbtn, true);
					ctl_set_value(form_set_suawarn_cldbtn, true);
					ctl_set_value(form_set_suawarn_clebtn, true);
					ctl_set_value(form_set_suawarn_alrbtn, true);
					ctl_set_value(form_set_suawarn_moabtn, true);
					ctl_set_value(form_set_suawarn_gsecbtn, true);
					ctl_set_value(form_set_suawarn_tmabtn, true);
					ctl_set_value(form_set_suawarn_tmpbtn, true);
					ctl_set_value(form_set_suawarn_warbtn, true);
					ctl_set_value(form_set_suawarn_clabtn, true);
					ctl_set_value(form_set_suawarn_clfbtn, true);
					ctl_set_value(form_set_suawarn_clgbtn, true);
					ctl_set_value(form_set_suawarn_tmzbtn, true);
					ctl_set_value(form_set_suawarn_mtzbtn, true);
					ctl_set_value(form_set_suawarn_bndbtn, true);
					//ctl_set_value(form_set_suawarn_tfrbtn, true);
					handled = true;
					break;
				case form_set_suawarn_alloff:
					data.config.suawarntypes = SUANONE;
					ctl_set_value(form_set_suawarn_ctabtn, false);
					ctl_set_value(form_set_suawarn_airbtn, false);
					ctl_set_value(form_set_suawarn_resbtn, false);
					ctl_set_value(form_set_suawarn_probtn, false);
					ctl_set_value(form_set_suawarn_danbtn, false);
					ctl_set_value(form_set_suawarn_othbtn, false);
					ctl_set_value(form_set_suawarn_trnbtn, false);
					ctl_set_value(form_set_suawarn_trabtn, false);
					ctl_set_value(form_set_suawarn_clbbtn, false);
					ctl_set_value(form_set_suawarn_clcbtn, false);
					ctl_set_value(form_set_suawarn_cldbtn, false);
					ctl_set_value(form_set_suawarn_clebtn, false);
					ctl_set_value(form_set_suawarn_alrbtn, false);
					ctl_set_value(form_set_suawarn_moabtn, false);
					ctl_set_value(form_set_suawarn_gsecbtn, false);
					ctl_set_value(form_set_suawarn_tmabtn, false);
					ctl_set_value(form_set_suawarn_tmpbtn, false);
					ctl_set_value(form_set_suawarn_warbtn, false);
					ctl_set_value(form_set_suawarn_clabtn, false);
					ctl_set_value(form_set_suawarn_clfbtn, false);
					ctl_set_value(form_set_suawarn_clgbtn, false);
					ctl_set_value(form_set_suawarn_tmzbtn, false);
					ctl_set_value(form_set_suawarn_mtzbtn, false);
					ctl_set_value(form_set_suawarn_bndbtn, false);
					//ctl_set_value(form_set_suawarn_tfrbtn, false);
					handled = true;
					break;
				case form_set_suawarn_exitbtn:
					FrmGotoForm(form_set_sua);
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

void Update_QNHEvent()
{
//	HostTraceOutputTL(appErrorClass, "Update QFE/QNH");

	// show QNH value
	if ((data.config.pressaltsrc == C302ALT || data.config.pressaltsrc == SN10ALT) && recv_palt) {
		// use C302 output for QNH value
		data.input.QNHpressure = data.input.compqnh; 
	}
	field_set_value(form_set_qnh_pressure, DblToStr(data.input.QNHpressure * data.input.QNHconst, QNHnumdigits));

	// calculate QNH alititude correction
	data.input.QNHaltcorrect = presstoalt2(data.input.QNHpressure);

	// show altitude values - depends on pressure source and use pressure alt settings
	if ((data.config.pressaltsrc == GPSALT) || !recv_palt) {
		// GPS is the master, or no pressure data received
		field_set_value(form_set_qnh_msl, print_altitude(data.logger.gpsalt));
		field_set_value(form_set_qnh_fl,  DblToStr(data.logger.gpsalt + data.input.QNHaltcorrect,0));
		if (terrainvalid) {
			field_set_value(form_set_qnh_agl, print_altitude(data.logger.gpsalt - data.input.terelev));
		} else {
			field_set_value(form_set_qnh_agl, "N/A");
		}							
		field_set_value(form_set_qnh_qfe, print_altitude(data.logger.gpsalt - data.config.qfealt));
		if (data.config.pressaltsrc == GPSALT) {
			field_set_value(form_set_qnh_required, "QNH used for FL");
		} else {
			if (data.config.pressaltsrc == C302ALT) {
				field_set_value(form_set_qnh_required, "No C302 Data!");
			} else if (data.config.pressaltsrc == SN10ALT) {
				field_set_value(form_set_qnh_required, "No SN10 Data!");
			} else {
				field_set_value(form_set_qnh_required, "No Pressure Data!");
			}
		}
	} else 	if (data.config.usepalt) { // assumes usepalt is false for C302 setting
			// Pressure is the master, MSL calculated from pressure alt with QNH correction
			field_set_value(form_set_qnh_msl, print_altitude(data.logger.pressalt - data.input.QNHaltcorrect));
			field_set_value(form_set_qnh_fl,  DblToStr(data.logger.pressalt,0));
			if (terrainvalid) {
				field_set_value(form_set_qnh_agl, print_altitude((data.logger.pressalt - data.input.QNHaltcorrect) - data.input.terelev));
			} else {
				field_set_value(form_set_qnh_agl, "N/A");
			}							
			field_set_value(form_set_qnh_qfe, print_altitude((data.logger.pressalt - data.input.QNHaltcorrect) - data.config.qfealt));
			field_set_value(form_set_qnh_required, "QNH used for MSL");
		} else {
			// Mixed (QNH not used)
			if (data.config.pressaltsrc == C302ALT) {
				// use C302 output for MSL
				field_set_value(form_set_qnh_msl, print_altitude(data.input.comptruealt));
				field_set_value(form_set_qnh_fl,  DblToStr(data.logger.pressalt,0));
				if (terrainvalid) {
					field_set_value(form_set_qnh_agl, print_altitude(data.input.comptruealt - data.input.terelev));
				} else {
					field_set_value(form_set_qnh_agl, "N/A");
				}							
				field_set_value(form_set_qnh_qfe, print_altitude(data.input.comptruealt - data.config.qfealt));
				field_set_value(form_set_qnh_required, "QNH data from C302");
			} else {
				field_set_value(form_set_qnh_msl, print_altitude(data.logger.gpsalt));
				field_set_value(form_set_qnh_fl,  DblToStr(data.logger.pressalt,0));
				if (terrainvalid) {
					field_set_value(form_set_qnh_agl, print_altitude(data.logger.gpsalt - data.input.terelev));
				} else {
					field_set_value(form_set_qnh_agl, "N/A");
				}							
				field_set_value(form_set_qnh_qfe, print_altitude(data.logger.gpsalt - data.config.qfealt));
				field_set_value(form_set_qnh_required, "QNH not used");
			}
		}	
}

Boolean form_set_qnh_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
	static double QNHinc = 1.0;

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(pfrm);
			menuopen = false;

			// set decimal places for QNH value
			if (data.config.QNHunits == MILLIBARS) {
				QNHnumdigits = 0;
				QNHinc = 1.0;
			} else {
				QNHnumdigits = 2;
				QNHinc = MBPERIN/100.0;
			}

			// check for C302
			if (data.config.pressaltsrc == C302ALT || data.config.pressaltsrc == SN10ALT) {
				// hide buttons to change QNH
				ctl_set_visible(form_set_qnh_plus1, false);
				ctl_set_visible(form_set_qnh_plus10, false);
				ctl_set_visible(form_set_qnh_minus1, false);
				ctl_set_visible(form_set_qnh_minus10, false);
				ctl_set_visible(form_set_qnh_reset, false);
			}

			// draw height units
			WinDrawChars(data.input.alttext, 2, 68, 15);
			WinDrawChars("ft", 2, 68, 35);
			WinDrawChars(data.input.alttext, 2, 149, 15);
			WinDrawChars(data.input.alttext, 2, 149, 35);
			WinDrawChars(data.input.alttext, 2, 149, 57);

			// show QNH units
			WinDrawChars(data.input.QNHtext, 2, 47, 121);			

			// show field elevation value
			field_set_value(form_set_qnh_fieldelev, print_altitude(data.config.qfealt));

			// show the auto zero QFE setting
			ctl_set_value(form_set_qnh_autozeroQFE, data.config.autozeroQFE);

			WinDrawLine (0, 90, 160, 90);
			Update_QNHEvent();

			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
				// get field elevation value from field (maybe changed by pen input)
				data.config.qfealt = field_get_double(form_set_qnh_fieldelev)/data.input.altconst;
				Update_QNHEvent();
			}
			handled=false;
			break;	
		case frmCloseEvent:
			data.config.qfealt = field_get_double(form_set_qnh_fieldelev)/data.input.altconst;
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "Set QNH menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "Set QNH menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case ctlSelectEvent:  // A control button was pressed and released.
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_set_qnh_reset:
					data.input.QNHpressure = 1013.25;
					break;
				case form_set_qnh_plus1:
					data.input.QNHpressure += QNHinc;
					break;
				case form_set_qnh_plus10:
					data.input.QNHpressure += QNHinc*10;
					break;
				case form_set_qnh_minus1:
					data.input.QNHpressure -= QNHinc;
					break;
				case form_set_qnh_minus10:
					data.input.QNHpressure -= QNHinc*10;
					break;
				case form_set_qnh_zeroqfe:
					if ((((data.config.pressaltsrc != GPSALT) && data.config.usepalt) || (data.config.pressaltsrc == C302ALT)) && recv_palt) {
						if (data.config.pressaltsrc == C302ALT) {
							data.config.qfealt = data.input.comptruealt;
						} else {
							data.config.qfealt = data.logger.pressalt - data.input.QNHaltcorrect;
						}
					} else {
						data.config.qfealt = data.logger.gpsalt;
					}
					field_set_value(form_set_qnh_fieldelev, print_altitude(data.config.qfealt));
					break;
				case form_set_qnh_autozeroQFE:
					data.config.autozeroQFE = ctl_get_value(form_set_qnh_autozeroQFE);
					handled = true;
					break;
				default:
					break;
			}
			Update_QNHEvent();
			handled = true;
			break;
		default:
			break;
	}
	return handled;
}

void refresh_sua_list(Int16 action)
{
	#define FIELDLEN 20

	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst, lst2, lst3, lst4;
	Int16 x=0, SUAIdx=0;
	MemHandle mem_hand;
	MemPtr mem_ptr;
	static Char **status = NULL;
	static Char **title = NULL;
	static Char **type = NULL;
	static Char **dist = NULL;
	static Int16 prevNumRecs = 0;
	Char tempchar[20];
	Char pageString[20];
	Int16 start=0, end=0, nrecs;
	Int16 statuslen=1;
	Int16 titlelen=15;
	Int16 typelen=5;
//	Int16 distlen=5;
	Boolean act;
	double neardist;

//	HostTraceOutputTL(appErrorClass, "action =|%hd|", action);

	// Get the number of SUA items in the database
	numSUARecs = DmNumRecords(suaidx_db);
//	HostTraceOutputTL(appErrorClass, "numSUARecs-|%hd|", numSUARecs);					

	switch (action) {
		case SUADISP:
			break;
		case SUAUPNUM:
			OpenDBQueryRecord(suaidx_db, selectedSUAListIdx, &mem_hand, &mem_ptr);
			MemMove(selectedSUA, mem_ptr, sizeof(SUAIndex));
			MemHandleUnlock(mem_hand);
			break;
		case SUAACTV:
			if (SUAselectall) {
				act = selectedSUA->active;
				// Toggle all the active & WarnActive statuses
				for (x = 0; x < numSUARecs; x++) { 
					OpenDBQueryRecord(suaidx_db, x, &mem_hand, &mem_ptr);
					MemMove(selectedSUA, mem_ptr, sizeof(SUAIndex));
					MemHandleUnlock(mem_hand);
					if (selectedSUA->type != SUAERROR) {
						if (!act) {
							selectedSUA->active = true;
							selectedSUA->WarnActive = true;
						} else {
							selectedSUA->active = false;
							selectedSUA->WarnActive = false;
						}
						OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), selectedSUA, x);
					}
				}
			} else {
				if ((selectedSUAListIdx >= (currentSUAPage*10)) && (selectedSUAListIdx < ((currentSUAPage*10) + 10))) {
					if (selectedSUA->type != SUAERROR) {
						// Toggle the active & WarnActive statuses of the currently selected SUA item. 
						selectedSUA->active = !selectedSUA->active;
						selectedSUA->WarnActive = !selectedSUA->WarnActive;
						OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), selectedSUA, selectedSUAListIdx);
					}
				}
			}
			break;
		case SUAACTVD:
			// Toggle the active & WarnActive statuses of the currently selected SUA item. 
			if (selectedSUA->type != SUAERROR) {
				selectedSUA->active = !selectedSUA->active;
				selectedSUA->WarnActive = selectedSUA->active;
				OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), selectedSUA, selectedSUAListIdx);
			}
			break;			
		case SUAVIEW:
			if (numSUARecs > 0) {
				dispsuareadonly = false;
				FrmGotoForm(form_disp_sua);
			}
			break;
		case SUAPGDWN:
//			HostTraceOutputTL(appErrorClass, "SUAPGDWN-currentSUAPage-|%hd|",currentSUAPage);					
			if (((currentSUAPage + 1) * 10) <  numSUARecs) {
				// If there are more waypoints to display, move down one page
				currentSUAPage++;
			} else {
				// If at the bottom, wrap to the first page
				currentSUAPage = 0;
			}
			break;
		case SUAPGUP:
			if (currentSUAPage > 0) {
				// If not on the first page of waypoints, move up one page 
				currentSUAPage--;
			} else {
				// If at the top, wrap to the last page
				if (numSUARecs == 0) {
					currentSUAPage = 0;
				} else if (Fmod((double)numSUARecs,(double)10) == 0.0) {
					currentSUAPage = (Int16)(numSUARecs/10) - 1;
				} else {
					currentSUAPage = (Int16)(numSUARecs/10);
				}
			}
			break;
		case SUASORT:
			DmInsertionSort(suaidx_db, (DmComparF*)sua_comparison, suasortType);
			if (selectedSUAListIdx != -1) {
				selectedSUAListIdx = FindSUARecordByName(selectedSUA->title);
			}
			currentSUAPage = 0;
			break;
		case SUAFREE:
			// Free up each of the previous strings and then free up
			// the array of pointers itself.
			for (x = 0; x < prevNumRecs; x++) {
				MemPtrFree(status[x]);
				MemPtrFree(title[x]);
				MemPtrFree(type[x]);
				MemPtrFree(dist[x]);
			}
			if (status) {
				MemPtrFree(status);
				status = NULL;
			}
			if (title) {
				MemPtrFree(title);
				title = NULL;
			}

			if (type) {
				MemPtrFree(type);
				type = NULL;
			}
			if (dist) {
				MemPtrFree(dist);
				dist = NULL;
			}
			prevNumRecs = 0;
			return;
			break;
		default:
			break;
	}

	// Update the screen
	if ( FrmGetActiveFormID() == form_list_sua && action != SUAUPNUM && action != SUAVIEW) {
		// Get the List pointers
		lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_sua_list));
		lst2 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_sua_list2));
		lst3 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_sua_list3));
		lst4 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_sua_list4));

		// Free up each of the previous strings and then free up
		// the array of pointers itself.
		for (x = 0; x < prevNumRecs; x++) {
			MemPtrFree(status[x]);
			MemPtrFree(title[x]);
			MemPtrFree(type[x]);
			MemPtrFree(dist[x]);
		}
		if (status) {
			MemPtrFree(status);
			status = NULL;
		}
		if (title) {
			MemPtrFree(title);
			title = NULL;
		}

		if (type) {
			MemPtrFree(type);
			type = NULL;
		}
		if (dist) {
			MemPtrFree(dist);
			dist = NULL;
		}

		if (numSUARecs > 0) {
			// Compute page to display based on selectedSUAListIdx; 
//			currentSUAPage = (Int16)(selectedSUAListIdx / 10);

			// Compute start and end indexes of the page to display
			start = currentSUAPage * 10;
			end = ((start + 10) > numSUARecs) ? numSUARecs : (start + 10);
			nrecs = end - start;
		
			// We got at least one record so allocate enough 
			// memory to hold nrecs pointers-to-pointers
			status = (Char **) MemPtrNew((nrecs) * (sizeof(Char *)));
			title = (Char **) MemPtrNew((nrecs) * (sizeof(Char *)));
			type = (Char **) MemPtrNew((nrecs) * (sizeof(Char *)));
			dist = (Char **) MemPtrNew((nrecs) * (sizeof(Char *)));
			prevNumRecs = nrecs;

			// Loop through each SUA Record
			for (x = 0; x < numSUARecs; x++) { 
				if (x >= start && x < end) {
					OpenDBQueryRecord(suaidx_db, x, &mem_hand, &mem_ptr);
					MemMove(selectedSUA, mem_ptr, sizeof(SUAIndex));
					MemHandleUnlock(mem_hand);

					status[SUAIdx] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
					title[SUAIdx] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
					type[SUAIdx] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
					dist[SUAIdx] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
					MemSet(status[SUAIdx], FIELDLEN, 0);
					MemSet(title[SUAIdx], FIELDLEN, 0);
					MemSet(type[SUAIdx], FIELDLEN, 0);
					MemSet(dist[SUAIdx], FIELDLEN, 0);
					if (selectedSUA->active) {
						if (selectedSUA->WarnActive) {
							StrNCopy(status[SUAIdx], "Y", statuslen);
						} else {
							StrNCopy(status[SUAIdx], "D", statuslen);
						}
					} else {
						StrNCopy(status[SUAIdx], "N", statuslen);
					}
					StrNCopy(title[SUAIdx], selectedSUA->title, titlelen);

					// Convert the current type to a string
					// typelen is the maximum length of the resulting string
					// true means to use the short form of the string for output
					GetSUATypeStr(type[SUAIdx], selectedSUA->type, typelen, true);

					if (selectedSUA->WarnType == SUAWARN_NONE) {
						//LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, selectedSUA->reflat, selectedSUA->reflon, &neardist);
						neardist = disttoSUAitem(selectedSUA);
						StrNCopy(dist[SUAIdx], print_distance2(neardist, 0), 5);
						StrCopy(dist[SUAIdx], leftpad(dist[SUAIdx], '\031', 5));
					} else if (((selectedSUA->WarnType >= SUAURGENT_NEAR_BELOW) && (selectedSUA->WarnType <= SUAURGENT_IN)) ||
						  ((selectedSUA->WarnType >= SUAURGENT_LOW) && (selectedSUA->WarnType <= SUAURGENT_HIGH_EDGE))) {
						// urgent warning
						StrCopy(dist[SUAIdx], "WARN!");
					} else {
						// normal warning
						StrCopy(dist[SUAIdx], "Warn");
					}

					SUAIdx++;
				}
			}
						
			// Reform the list
			LstSetListChoices(lst, status, nrecs);
			LstSetListChoices(lst2, title, nrecs);
			LstSetListChoices(lst3, type, nrecs);
			LstSetListChoices(lst4, dist, nrecs);

			// Call once more to re-populate for current selected SUA
			OpenDBQueryRecord(suaidx_db, selectedSUAListIdx, &mem_hand, &mem_ptr);
			MemMove(selectedSUA, mem_ptr, sizeof(SUAIndex));
			MemHandleUnlock(mem_hand);
		} else {
			status = (Char **) MemPtrNew(1 * (sizeof(Char *)));
			title = (Char **) MemPtrNew(1 * (sizeof(Char *)));
			type = (Char **) MemPtrNew(1 * (sizeof(Char *)));
			dist = (Char **) MemPtrNew(1 * (sizeof(Char *)));
			prevNumRecs = 1;		
			status[0] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
			title[0] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
			type[0] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
			dist[0] = (Char *) MemPtrNew(FIELDLEN * (sizeof(Char)));
			MemSet(status[0],FIELDLEN,0);
			MemSet(title[0],FIELDLEN,0);
			MemSet(type[0],FIELDLEN,0);
			MemSet(dist[0],FIELDLEN,0);
			StrNCopy(title[0], "No SUA Data", 11);
			LstSetListChoices(lst, status, 1);
			LstSetListChoices(lst2, title, 1);
			LstSetListChoices(lst3, type, 1);
			LstSetListChoices(lst4, dist, 1);
			
			selectedSUAListIdx = -1;
		}

		//redraw the list
		if (data.config.listlinesvert > 1) {
			LstDrawList(lst);
			LstDrawList(lst2);
			LstDrawList(lst3);
			LstDrawList(lst4);
		} else {
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_sua_list));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_sua_list2));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_sua_list3));
			DrawFormWithNoListBorder(pfrm, FrmGetObjectIndex(pfrm, form_list_sua_list4));
		}

		if (!SUAselectall && ((selectedSUAListIdx >= (currentSUAPage*10)) &&  (selectedSUAListIdx < ((currentSUAPage*10) + 10)))) {
			LstSetSelection(lst, (selectedSUAListIdx % 10));
			LstSetSelection(lst2, (selectedSUAListIdx % 10));
			LstSetSelection(lst3, (selectedSUAListIdx % 10));
			LstSetSelection(lst4, (selectedSUAListIdx % 10));
		} else {
			LstSetSelection(lst, -1);
			LstSetSelection(lst2, -1);
			LstSetSelection(lst3, -1);
			LstSetSelection(lst4, -1);
		}
		DrawHorizListLines(10, 36, 11);

		ctl_set_visible(form_list_sua_acttbtn, true);
		ctl_set_visible(form_list_sua_nametbtn, true);
		ctl_set_visible(form_list_sua_typetbtn, true);
		ctl_set_visible(form_list_sua_disttbtn, true);
		switch (suasortType) {
			case SUASortByActiveA:
			case SUASortByActiveD:
				WinDrawLine(2,22,18,22);
				break;
			case SUASortByTitleA:
			case SUASortByTitleD:
				WinDrawLine(38,22,60,22);
				break;
			case SUASortByTypeA:
			case SUASortByTypeD:
				WinDrawLine(100,22,122,22);
				break;
			case SUASortByDistA:
			case SUASortByDistD:
				WinDrawLine(135,22,154,22);
				break;
			default:
				break;
		}
	}
	
	if ( FrmGetActiveFormID() == form_list_sua && action != SUAVIEW ) {
		// Display the number of SUA Items in the database
		MemSet(pageString, 20, 0);
		if (SUAselectall) {
			StrCopy(pageString, "All ");
			StrCat(pageString,StrIToA(tempchar, numSUARecs));
		} else {
			StrCopy(pageString,StrIToA(tempchar, selectedSUAListIdx+1));
			StrCat(pageString, " of ");
			StrCat(pageString,StrIToA(tempchar, numSUARecs));
		}
		field_set_value(form_list_sua_page, pageString);
		if ( FrmGetActiveFormID() == form_list_sua) {
			if (selectedSUA->active) {
				ctl_set_label(form_list_sua_actbtn, "DeAct");
			} else {
				ctl_set_label(form_list_sua_actbtn, "Act");
			}
		}
	}
}

Boolean form_list_sua_event_handler(EventPtr event)
{
	FormType *pfrm = FrmGetActiveForm();
	ListPtr lst, lst2, lst3, lst4;
	Boolean handled=false;
	Boolean atexit=false;
	Int16 SUAidx, selectedIdx = 0;
	Char tempchar[25];
	static Int16 saveSUAidx, saveSUAidx2;
	UInt16 x;
	MemHandle output_hand;
	MemPtr output_ptr;
	static Boolean tfradd = false;
	
	lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_sua_list));
	lst2 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_sua_list2));
	lst3 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_sua_list3));
	lst4 = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm,form_list_sua_list4));

	switch (event->eType) {
		case frmOpenEvent:
		case frmUpdateEvent:
			origform = form_list_sua;

			FrmDrawForm(pfrm);

			saveSUAidx = -1;
			saveSUAidx2 = -1;

			StrCopy(tempchar, "SUA List - ");			

			switch (suasortType) {
				case SUASortByActiveA:
					StrCat(tempchar, "Active");
					WinDrawLine(4,26,20,26);
					break;
				case SUASortByActiveD:
					StrCat(tempchar, "Deactivated");
					break;
				case SUASortByTitleA:
					StrCat(tempchar, "Title A");
					break;
				case SUASortByTitleD:
					StrCat(tempchar, "Title D");
					break;
				case SUASortByTypeA:
					StrCat(tempchar, "Type A");
					break;
				case SUASortByTypeD:
					StrCat(tempchar, "Type D");
					break;
				case SUASortByDistD:
//					StrCat(tempchar, "Dist D");
//					break;
				case SUASortByDistA:
					StrCat(tempchar, "Dist A");
					break;

				default:
					break;
			}
			frm_set_title(tempchar);

			// allocate memory
			AllocMem((void *)&suaidx, sizeof(SUAIndex));

			// Loop for each SUA element
			for (x=0; x < data.input.SUAnumrecs; x++) {
				OpenDBQueryRecord(suaidx_db, x, &output_hand, &output_ptr);
				MemMove(suaidx, output_ptr, sizeof(SUAIndex));
				MemHandleUnlock(output_hand);

				// set nearest distance
				suaidx->neardist = disttoSUAitem(suaidx);

				OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), suaidx, x);
			}

			// free memory
			FreeMem((void *)&suaidx);

			// keep manually added TFR on screen
			if (tfradd) {
				tfradd = false;
				currentSUAPage = selectedSUAListIdx / 10;
			} else {
				DmInsertionSort(suaidx_db, (DmComparF*)sua_comparison, suasortType);
			}

			refresh_sua_list(SUADISP);

			handled=true;
			break;
		case fldEnterEvent:
			if (event->data.ctlEnter.controlID == form_list_sua_page) {
				PlayKeySound();
				SUAselectall = !SUAselectall;
				refresh_sua_list(SUADISP);
				handled=true;
				break;
			}
		case ctlSelectEvent:
			PlayKeySound();
			switch ( event->data.ctlEnter.controlID ) {
				case form_list_sua_addmanual:
					// add manual TFR to SUA database
					SUAselectall = false;
					MemSet(selectedSUA, sizeof(SUAIndex), 0);
					StrCopy(selectedSUA->title, "Manual TFR");
					selectedSUA->type = TFR;
					selectedSUA->class = SUAallclasses;
					selectedSUA->plottype = SUATFR;
					selectedSUA->base = SFC;
					selectedSUA->basetype = SUA_SFC;
					selectedSUA->tops = UNLIMITED;
					selectedSUA->topstype = SUA_UNL;
					selectedSUA->active = true;
					selectedSUA->WarnActive = true;
					selectedSUA->WarnType = SUAWARN_NONE;
					selectedSUAListIdx = OpenDBAddRecord(suaidx_db, sizeof(SUAIndex), selectedSUA, false);
					dispsuareadonly = false;
					tfradd = true;
					FrmGotoForm( form_disp_sua );
					break;
				case form_list_sua_selectall:
					SUAselectall = !SUAselectall;
					refresh_sua_list(SUADISP);
					break;
				case form_list_sua_viewbtn:
					if (!SUAselectall) refresh_sua_list(SUAVIEW);
					break;
				case form_list_sua_actbtn:
					refresh_sua_list(SUAACTV);
					break;
				case form_list_sua_acttbtn:
					if (suasortType == SUASortByActiveA) {
						suasortType = SUASortByActiveD;
					} else {
						suasortType = SUASortByActiveA;
					}
					refresh_sua_list(SUASORT);
					break;
				case form_list_sua_nametbtn:
					if (suasortType == SUASortByTitleA) {
						suasortType = SUASortByTitleD;
					} else {
						suasortType = SUASortByTitleA;
					}
					refresh_sua_list(SUASORT);
					break;
				case form_list_sua_typetbtn:
					if (suasortType == SUASortByTypeA) {
						suasortType = SUASortByTypeD;
					} else {
						suasortType = SUASortByTypeA;
					}
					refresh_sua_list(SUASORT);
					break;
				case form_list_sua_disttbtn:
					if (suasortType == SUASortByDistA) {
						suasortType = SUASortByDistD;
					} else {
						suasortType = SUASortByDistA;
					}
					refresh_sua_list(SUASORT);
					break;
				default:
					break;
			}
			StrCopy(tempchar, "SUA List - ");			
			switch (suasortType) {
				case SUASortByActiveA:
					StrCat(tempchar, "Active");
					break;
				case SUASortByActiveD:
					StrCat(tempchar, "Deactivated");
					break;
				case SUASortByTitleA:
					StrCat(tempchar, "Title A");
					break;
				case SUASortByTitleD:
					StrCat(tempchar, "Title D");
					break;
				case SUASortByTypeA:
					StrCat(tempchar, "Type A");
					break;
				case SUASortByTypeD:
					StrCat(tempchar, "Type D");
					break;
				case SUASortByDistD:
//					StrCat(tempchar, "Dist D");
//					break;
				case SUASortByDistA:
					StrCat(tempchar, "Dist A");
					break;
				default:
					break;
			}
			frm_set_title(tempchar);
			handled=true;
			break;
		case lstSelectEvent:
		{
			switch (event->data.lstSelect.listID) {

				case form_list_sua_list:
					if (numSUARecs > 0) {
						PlayKeySound();
						selectedIdx = LstGetSelection(lst);
//						HostTraceOutputTL(appErrorClass, "selectedIdx =|%hd|", selectedIdx);
						SUAidx = selectedIdx + (currentSUAPage * 7);
						if (SUAidx == saveSUAidx) {
							// toggle active flag
							refresh_sua_list(SUAACTVD);
							saveSUAidx = -1; // forces double click again
						} else {
							saveSUAidx = SUAidx;
						}
					}
					break;
				case form_list_sua_list2:
					if (numSUARecs > 0) {
						PlayKeySound();
						selectedIdx = LstGetSelection(lst2);
//						HostTraceOutputTL(appErrorClass, "selectedIdx =|%hd|", selectedIdx);
						SUAidx = selectedIdx + (currentSUAPage * 7);
						if (SUAidx == saveSUAidx2) {
							// go to SUA item display
							refresh_sua_list(SUAVIEW);
							saveSUAidx2 = -1; // forces double click again
						} else {
							saveSUAidx2 = SUAidx;
						}
					}
					break;
				case form_list_sua_list3:
					if (numSUARecs > 0) {
						PlayKeySound();
						selectedIdx = LstGetSelection(lst3);
//						HostTraceOutputTL(appErrorClass, "selectedIdx =|%hd|", selectedIdx);
					}
					break;
				case form_list_sua_list4:
					if (numSUARecs > 0) {
						PlayKeySound();
						selectedIdx = LstGetSelection(lst4);
//						HostTraceOutputTL(appErrorClass, "selectedIdx =|%hd|", selectedIdx);
					}
					break;
				default:
					break;
			}
			SUAselectall = false;
			LstSetSelection(lst, selectedIdx);
			LstSetSelection(lst2, selectedIdx);
			LstSetSelection(lst3, selectedIdx);
			LstSetSelection(lst4, selectedIdx);
			if (selectedSUAListIdx != -1) {
				selectedSUAListIdx = selectedIdx + (currentSUAPage * 10);
				refresh_sua_list(SUAUPNUM); // Just update the waypoint X of X display
			}
			DrawHorizListLines(10, 36, 11);
			handled=true;
			break;
		}
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "SUA List menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "SUA List menuopen = true");
			atexit = true;
			menuopen = true;
			//handled = false;
			//break;
		case frmCloseEvent:
			if (!allowExit && !atexit) {
				refresh_sua_list(SUAFREE);
			}
			handled=false;
			break;
		default:
			break;
	}
	return(handled);
}

Boolean form_disp_sua_event_handler(EventPtr event)
{
	Boolean handled=false;
	FormType *pfrm = FrmGetActiveForm();
	Char tempchar[26];
	ControlPtr ctl;
	ListPtr lst;
	RectangleType rectP;
	DateTimeType timenow;
        
	switch (event->eType)
	{
		case frmOpenEvent:
		case frmUpdateEvent:
			FrmDrawForm(pfrm);

			field_set_value(form_disp_sua_title, selectedSUA->title);
			field_set_value(form_disp_sua_daysactive, selectedSUA->daysactive);
			field_set_value(form_disp_sua_radiofreq, selectedSUA->radiofreq);

			GetSUATypeStr(tempchar, selectedSUA->type, 11, false);
			if (selectedSUA->type & AIRWAYS) {
				StrCopy(tempchar, Left(tempchar,6));
				StrCat(tempchar, " W");
				StrCat(tempchar, print_distance2(selectedSUA->width, 1));
			}
			field_set_value(form_disp_sua_type, tempchar);
			GetSUAClassStr(tempchar, selectedSUA->class);
			field_set_value(form_disp_sua_class, tempchar);

			// tops
//			HostTraceOutputTL(appErrorClass, "SUA Tops %s", DblToStr(selectedSUA->tops, 0));
			StrCopy(tempchar, print_SUA_altitude(selectedSUA->tops, selectedSUA->topsunits));
			if ((selectedSUA->topstype == SUA_SFC) || (selectedSUA->tops == SFC)) StrCopy(tempchar, "Surface");
			else if ((selectedSUA->topstype == SUA_UNL) || (selectedSUA->tops == UNLIMITED)) StrCopy(tempchar, "Unlimit");
			field_set_value(form_disp_sua_tops, tempchar);
			ctl = (ControlPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm, form_disp_sua_topspop1));
			lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm, form_disp_sua_topspop2));
			LstSetSelection (lst, selectedSUA->topstype);
//			CtlSetLabel (ctl, LstGetSelectionText(lst, selectedSUA->topstype));
			ctl_set_label(form_disp_sua_topspop1, LstGetSelectionText(lst, selectedSUA->topstype));
			field_set_enable(form_disp_sua_tops, false);
			ctl_set_enable(form_disp_sua_topspop1, false);

			// base
//			HostTraceOutputTL(appErrorClass, "SUA Base %s", DblToStr(selectedSUA->base, 0));
			StrCopy(tempchar, print_SUA_altitude(selectedSUA->base, selectedSUA->baseunits));
			if ((selectedSUA->basetype == SUA_SFC) || (selectedSUA->base == SFC)) StrCopy(tempchar, "Surface");
			else if ((selectedSUA->basetype == SUA_UNL) || (selectedSUA->base == UNLIMITED)) StrCopy(tempchar, "Unlimit");
			field_set_value(form_disp_sua_base, tempchar);
			ctl = (ControlPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm, form_disp_sua_basepop1));
			lst = (ListPtr)FrmGetObjectPtr(pfrm, FrmGetObjectIndex(pfrm, form_disp_sua_basepop2));
			LstSetSelection (lst, selectedSUA->basetype);
//			CtlSetLabel (ctl, LstGetSelectionText(lst, selectedSUA->basetype));
			ctl_set_label(form_disp_sua_basepop1, LstGetSelectionText(lst, selectedSUA->basetype));
			field_set_enable(form_disp_sua_base, false);
			ctl_set_enable(form_disp_sua_basepop1, false);

			if (!dispsuareadonly && (selectedSUA->type & TFR)) {
				// enable fields for manually entered TFR
				//ctl_set_visible(form_disp_sua_tfrlbl, true);
				ctl_set_visible(form_disp_sua_radlbl, true);
				ctl_set_visible(form_disp_sua_radius, true);
				ctl_set_visible(form_disp_sua_latlbl, true);
				ctl_set_visible(form_disp_sua_lat, true);
				ctl_set_visible(form_disp_sua_lonlbl, true);
				ctl_set_visible(form_disp_sua_lon, true);
				ctl_set_visible(form_disp_sua_lldm, true);
				ctl_set_visible(form_disp_sua_lldms, true);
				ctl_set_visible(form_disp_sua_llutm, true);
				ctl_set_visible(form_disp_sua_delete, true);
				field_set_enable(form_disp_sua_tops, true);
				field_set_enable(form_disp_sua_base, true);
				ctl_set_enable(form_disp_sua_topspop1, true);
				ctl_set_enable(form_disp_sua_basepop1, true);
				ctl_set_enable(form_disp_sua_class, true);

				switch (data.config.llfmt) {
					case LLUTM:
						frm_set_label(form_disp_sua_latlbl, "Est:");
						frm_set_label(form_disp_sua_lonlbl, "Nth:");
						ctl_set_value(form_disp_sua_llutm, true);
						ctl_set_value(form_disp_sua_lldm, false);
						ctl_set_value(form_disp_sua_lldms, false);
						ctl_set_visible(form_disp_sua_utmzone, true);
						LLToStringUTM(selectedSUA->reflat, selectedSUA->reflat, tempchar, EASTING);
						field_set_value(form_disp_sua_lat, tempchar);
						LLToStringUTM(selectedSUA->reflat, selectedSUA->reflat, tempchar, NORTHING);
						field_set_value(form_disp_sua_lon, tempchar);
						LLToStringUTM(selectedSUA->reflat, selectedSUA->reflat, tempchar, ZONE);
						field_set_value(form_disp_sua_utmzone, tempchar);
						break;
					case LLDM:
						frm_set_label(form_disp_sua_latlbl, "Lat:");
						frm_set_label(form_disp_sua_lonlbl, "Lon:");
						ctl_set_value(form_disp_sua_llutm, false);
						ctl_set_value(form_disp_sua_lldm, true);
						ctl_set_value(form_disp_sua_lldms, false);
						ctl_set_visible(form_disp_sua_utmzone, false);
						LLToStringDM(selectedSUA->reflat, tempchar, ISLAT, true, true, 3);
						field_set_value(form_disp_sua_lat, tempchar);
						LLToStringDM(selectedSUA->reflon, tempchar, ISLON, true, true, 3);
						field_set_value(form_disp_sua_lon, tempchar);
						break;
					case LLDMS:
					default:
						frm_set_label(form_disp_sua_latlbl, "Lat:");
						frm_set_label(form_disp_sua_lonlbl, "Lon:");
						ctl_set_value(form_disp_sua_llutm, false);
						ctl_set_value(form_disp_sua_lldm, false);
						ctl_set_value(form_disp_sua_lldms, true);
						ctl_set_visible(form_disp_sua_utmzone, false);
						LLToStringDMS(selectedSUA->reflat, tempchar, ISLAT);
						field_set_value(form_disp_sua_lat, tempchar);
						LLToStringDMS(selectedSUA->reflon, tempchar, ISLON);
						field_set_value(form_disp_sua_lon, tempchar);
						break;
				}
				field_set_value(form_disp_sua_radius, print_distance2(selectedSUA->maxdist,2));
				WinDrawChars(data.input.distext, 2, 145, 101);
			}

			if (!dispsuareadonly) {
				// normal display with save button
				ctl_set_visible(form_disp_sua_save, true);
				ctl_set_visible(form_disp_sua_closebtn, true);
				ctl_set_visible(form_disp_sua_actbtn, true);
			} else {
				// field to display SUA warning message
				frm_set_title("SUA Item Warning");
				ctl_set_visible(form_disp_sua_warning, true);
				ctl_set_visible(form_disp_sua_5mins, true);
				ctl_set_visible(form_disp_sua_1hour, true);
				ctl_set_visible(form_disp_sua_today, true);
				field_set_value(form_disp_sua_warning, trim(WarnString, ' ', true));
			}

			ctl_set_value(form_disp_sua_warnonexit, selectedSUA->warnonexit);

			if (selectedSUA->type == SUAERROR) {
				// display reason for error in warning fields
				ctl_set_visible(form_disp_sua_warning, true);
				ctl_set_visible(form_disp_sua_warning2, true);
				ctl_set_visible(form_disp_sua_save, false);
				ctl_set_visible(form_disp_sua_actbtn, false);
				if (selectedSUA->WarnType != SUAWARN_NONE) {
					if (selectedSUA->WarnType & SUAlatlonerror) {
						field_set_value(form_disp_sua_warning, "Invalid Lat or Lon Value");
						if (selectedSUA->WarnType & SUAfirstlasterror) {
							field_set_value(form_disp_sua_warning2, "Last Point Error");
						}
					} else if (selectedSUA->WarnType & SUAfirstlasterror) {
						field_set_value(form_disp_sua_warning, "Last Point Error");
							if (selectedSUA->WarnType & SUAtopsbaseerror) {
								field_set_value(form_disp_sua_warning2, "Base > Tops Error");
							}
					} else if (selectedSUA->WarnType & SUAtopsbaseerror) {
						field_set_value(form_disp_sua_warning, "Base > Tops Error");
					}
				}
			}

			handled=true;
			break;
		case nilEvent:
			if (!menuopen) {
				// display active YES / NO fields
				if (selectedSUA->active) {
					field_set_value(form_disp_sua_active, "Yes");
					ctl_set_label(form_disp_sua_actbtn, "DeAct");
				} else {
					field_set_value(form_disp_sua_active, "No");
					ctl_set_label(form_disp_sua_actbtn, "Act");
				}
				if (selectedSUA->WarnActive) {
					field_set_value(form_disp_sua_warnactive, "Yes");
				} else {
					field_set_value(form_disp_sua_warnactive, "No");
				}
			}
			handled=false;
			break;
		case winExitEvent:
//			HostTraceOutputTL(appErrorClass, "SUA Disp menuopen = true");
			menuopen = true;
			handled = false;
			break;
		case winEnterEvent:
			if (event->data.winEnter.enterWindow == (WinHandle) FrmGetFirstForm ()) {
//				HostTraceOutputTL(appErrorClass, "SUA Disp menuopen = false");
				menuopen = false;
			}
			handled=false;
			break;
		case frmCloseEvent:
			handled=false;
			break;
		case popSelectEvent:  // A popup button was pressed and released.
			PlayKeySound();
			switch (event->data.popSelect.controlID) {
				case form_disp_sua_topspop1:
					selectedSUA->topstype = event->data.popSelect.selection;
//					HostTraceOutputTL(appErrorClass, "Tops Type %s", DblToStr(selectedSUA->topstype, 0));
					if (selectedSUA->topstype == SUA_SFC) {
						field_set_value(form_disp_sua_tops, "Surface");
						selectedSUA->tops = SFC;
					} else if (StrCompare(field_get_str(form_disp_sua_tops), "Surface") == 0) {
						field_set_value(form_disp_sua_tops, "0");
						selectedSUA->tops = 0;
					}
					if (selectedSUA->topstype == SUA_UNL) {
						field_set_value(form_disp_sua_tops, "Unlimit");
						selectedSUA->tops = UNLIMITED;
					} else if (StrCompare(field_get_str(form_disp_sua_tops), "Unlimit") == 0) {
						field_set_value(form_disp_sua_tops, "0");
						selectedSUA->tops = 0;
					}
					handled = false; //Must be set to false for popup to work
					break;
				case form_disp_sua_basepop1:
					selectedSUA->basetype = event->data.popSelect.selection;
//					HostTraceOutputTL(appErrorClass, "Base Type %s", DblToStr(selectedSUA->basetype, 0));
					if (selectedSUA->basetype == SUA_SFC) {
						field_set_value(form_disp_sua_base, "Surface");
						selectedSUA->base = SFC;
					} else if (StrCompare(field_get_str(form_disp_sua_base), "Surface") == 0) {
						field_set_value(form_disp_sua_base, "0");
						selectedSUA->base = 0;
					}
					if (selectedSUA->basetype == SUA_UNL) {
						field_set_value(form_disp_sua_base, "Unlimit");
						selectedSUA->base = UNLIMITED;
					} else if (StrCompare(field_get_str(form_disp_sua_base), "Unlimit") == 0) {
						field_set_value(form_disp_sua_base, "0");
						selectedSUA->base = 0;
					}
					handled = false; //Must be set to false for popup to work
					break;
				default:
					break;
			}
			break;
		case penDownEvent:
			// close display if tapped in window
			RctSetRectangle (&rectP, 2, 2, 140, 158); 
			if (dispsuareadonly && (RctPtInRectangle (event->screenX, event->screenY, &rectP))) {
				 // immediately active again (new warning when more serious)
//				HostTraceOutputTL(appErrorClass, "SUA immediate");
				selectedSUA->LastWarned = cursecs;
				if (selectedSUA->WarnType == SUAWARN_APPROACHING) {
					selectedSUA->ApproachLastWarned = cursecs + (1 - data.config.SUArewarn) * data.config.SUAlookahead;
				}
				OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), selectedSUA, selectedSUAListIdx); 
				FrmGotoForm( origform );
				handled=true;
			}
			break;
		case fldEnterEvent:
			switch (event->data.ctlEnter.controlID) {
				case form_disp_sua_active:
					PlayKeySound();
					if (selectedSUA->type != SUAERROR) {
						selectedSUA->active = !selectedSUA->active;
						selectedSUA->WarnActive = !selectedSUA->WarnActive;
						if (!selectedSUA->active) selectedSUA->WarnActive = false;
					}
					handled=true;
					break;
				case form_disp_sua_warnactive:
					PlayKeySound();
					selectedSUA->WarnActive = !selectedSUA->WarnActive;
					if (!selectedSUA->active) selectedSUA->WarnActive = false;
					handled=true;
					break;
				default:
					break;
			}
			break;
		case ctlSelectEvent:
			PlayKeySound();
			switch (event->data.ctlEnter.controlID) {
				case form_disp_sua_warnonexit:
					selectedSUA->warnonexit = !selectedSUA->warnonexit;
					break;
				case form_disp_sua_lldm:
					frm_set_label(form_disp_sua_latlbl, "Lat:");
					frm_set_label(form_disp_sua_lonlbl, "Lon:");
					ctl_set_visible(form_disp_sua_utmzone, false);
					LLToStringDM(selectedSUA->reflat, tempchar, ISLAT, true, true, 3);
					field_set_value(form_disp_sua_lat, tempchar);
					LLToStringDM(selectedSUA->reflon, tempchar, ISLON, true, true, 3);
					field_set_value(form_disp_sua_lon, tempchar);
					data.config.llfmt = LLDM;
					handled=true;
					break;
				case form_disp_sua_lldms:
					frm_set_label(form_disp_sua_latlbl, "Lat:");
					frm_set_label(form_disp_sua_lonlbl, "Lon:");
					ctl_set_visible(form_disp_sua_utmzone, false);
					LLToStringDMS(selectedSUA->reflat, tempchar, ISLAT);
					field_set_value(form_disp_sua_lat, tempchar);
					LLToStringDMS(selectedSUA->reflon, tempchar, ISLON);
					field_set_value(form_disp_sua_lon, tempchar);
					data.config.llfmt = LLDMS;
					handled=true;
					break;
				case form_disp_sua_llutm:
					frm_set_label(form_disp_sua_latlbl, "Est:");
					frm_set_label(form_disp_sua_lonlbl, "Nth:");
					ctl_set_visible(form_disp_sua_utmzone, true);
					LLToStringUTM(selectedSUA->reflat, selectedSUA->reflon, tempchar, EASTING);
					field_set_value(form_disp_sua_lat, tempchar);
					LLToStringUTM(selectedSUA->reflat, selectedSUA->reflon, tempchar, NORTHING);
					field_set_value(form_disp_sua_lon, tempchar);
					LLToStringUTM(selectedSUA->reflat, selectedSUA->reflon, tempchar, ZONE);
					field_set_value(form_disp_sua_utmzone, tempchar);
					data.config.llfmt = LLUTM;
					handled=true;
					break;
				case form_disp_sua_actbtn:
					if (selectedSUA->type != SUAERROR) {
						selectedSUA->active = !selectedSUA->active;
						selectedSUA->WarnActive = !selectedSUA->WarnActive;
						if (!selectedSUA->active) selectedSUA->WarnActive = false;
					}
					handled=true;
					break;
				case form_disp_sua_save:
					saveSUAitem();
					handled=true;
					break;
				case form_disp_sua_delete:
					question->type = QdelSUA;
					FrmPopupForm(form_question);
					handled=true;
					break;
				case form_disp_sua_closebtn:
					if ((selectedSUA->type & TFR) && (selectedSUA->stopidx == 0)) {
						// TFR never saved so delete record
						OpenDBDeleteRecord(suaidx_db, selectedSUAListIdx); 
						selectedSUAListIdx = 0;
					}
					FrmGotoForm( origform );
					handled=true;
					break;
				case form_disp_sua_5mins: // 5 mins
//					HostTraceOutputTL(appErrorClass, "SUA 5 mins");
					selectedSUA->LastWarned = cursecs + 300;
					if (selectedSUA->WarnType == SUAWARN_APPROACHING) {
						selectedSUA->ApproachLastWarned = selectedSUA->LastWarned;		
					}
					selectedSUA->WarnType = SUAWARN_SNOOZE;
					OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), selectedSUA, selectedSUAListIdx); 
					FrmGotoForm( origform );
					handled=true;
					break;
				case form_disp_sua_1hour: // 1 hour
//					HostTraceOutputTL(appErrorClass, "SUA 1 hour");
					selectedSUA->LastWarned = cursecs + 3600;
					if (selectedSUA->WarnType == SUAWARN_APPROACHING) {
						selectedSUA->ApproachLastWarned = selectedSUA->LastWarned;		
					}
					selectedSUA->WarnType = SUAWARN_SNOOZE;
					OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), selectedSUA, selectedSUAListIdx); 
					FrmGotoForm( origform );
					handled=true;
					break;
				case form_disp_sua_today: // Today (24 hrs or until SoarPilot starts again)
//					HostTraceOutputTL(appErrorClass, "SUA midnight");
					TimSecondsToDateTime(cursecs, &timenow);
					timenow.hour = 23;
					timenow.minute = 59;
					timenow.second = 59;
					selectedSUA->LastWarned = TimDateTimeToSeconds(&timenow);
					//selectedSUA->LastWarned = cursecs + (UInt32)3600*24;
					if (selectedSUA->WarnType == SUAWARN_APPROACHING) {
						selectedSUA->ApproachLastWarned = selectedSUA->LastWarned;		
					}
					selectedSUA->WarnType = SUAWARN_NONE;
					OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), selectedSUA, selectedSUAListIdx); 
					FrmGotoForm( origform );
					handled=true;
					break;
				default:
					break;
			}
		default:
			break;
	}
	return handled;
}

void saveSUAitem()
{
	char *str;
	Char SUAtitle[30];
	double UTMn, UTMe;
	Char UTMz[5];

	// ignore for read only items
	if (dispsuareadonly) {
//		HostTraceOutputTL(appErrorClass, "SUA Read Only Save");
		FrmGotoForm( origform );
		return;
	}
	
	// copy fields from screen
//	StrCopy(selectedSUA->title, field_get_str(form_disp_sua_title));
	StrCopy(selectedSUA->daysactive, field_get_str(form_disp_sua_daysactive));
	StrCopy(selectedSUA->radiofreq, NoComma(field_get_str(form_disp_sua_radiofreq),"."));
	if (selectedSUA->type & TFR) {
//		HostTraceOutputTL(appErrorClass, "SUA TFR Save");
		// update info for TFR only
		if (data.config.llfmt == LLUTM) {
			UTMe = field_get_double(form_disp_sua_lat);
			UTMn = field_get_double(form_disp_sua_lon);
			StrCopy(UTMz, field_get_str(form_disp_sua_utmzone));
			UTMtoLL(UTMWGS84, UTMn, UTMe, UTMz, &selectedSUA->reflat,  &selectedSUA->reflon);
		} else {
			str = field_get_str(form_disp_sua_lat);
			if (str) {
				if (data.config.llfmt == LLDM) {
					selectedSUA->reflat = DegMinColonStringToLatLon(str);
				} else {
					selectedSUA->reflat = DegMinSecColonStringToLatLon(str);
				}
			}
			str = field_get_str(form_disp_sua_lon);
			if (str) {
				if (data.config.llfmt == LLDM) {
					selectedSUA->reflon = DegMinColonStringToLatLon(str);
				} else {
					selectedSUA->reflon = DegMinSecColonStringToLatLon(str);
				}
			}
		}
		selectedSUA->maxdist = Fabs(field_get_double(form_disp_sua_radius)/data.input.disconst);
		selectedSUA->SUAmaxlat = selectedSUA->reflat + selectedSUA->maxdist / 60.0;
		selectedSUA->SUAminlat = selectedSUA->reflat - selectedSUA->maxdist / 60.0;
		selectedSUA->SUAmaxlon = selectedSUA->reflon + selectedSUA->maxdist / 60.0 / data.input.coslat;
		selectedSUA->SUAminlon = selectedSUA->reflon - selectedSUA->maxdist / 60.0 / data.input.coslat;
		selectedSUA->neardist = disttoSUAitem(selectedSUA);
		if (selectedSUA->basetype == SUA_SFC) {
			selectedSUA->base = SFC;
		} else if (selectedSUA->basetype == SUA_UNL) {
			selectedSUA->base = UNLIMITED;
		} else {
			selectedSUA->base = Fabs(field_get_double(form_disp_sua_base));	
		}
		if (selectedSUA->topstype == SUA_SFC) {
			selectedSUA->tops = SFC;
		} else if (selectedSUA->topstype == SUA_UNL) {
			selectedSUA->tops = UNLIMITED;
		} else {
			selectedSUA->tops = Fabs(field_get_double(form_disp_sua_tops));
		}
		// flag to save record
		selectedSUA->stopidx = 1;		
	}
	
	// check for empty name
	StrCopy(SUAtitle, field_get_str(form_disp_sua_title));
	if (StrLen(trim(NoComma(SUAtitle," "),' ',true)) == 0) {
		warning->type = Wgeneric;
		StrCopy(warning->line1, "SUA Title");
		StrCopy(warning->line2, "cannot be blank");
		FrmPopupForm(form_warning);
	} else {
//		HostTraceOutputTL(appErrorClass, "SUA Save");
		StrCopy(selectedSUA->title, trim(NoComma(SUAtitle," "),' ',true));
		// update record
		OpenDBUpdateRecord(suaidx_db, sizeof(SUAIndex), selectedSUA, selectedSUAListIdx);
		FrmGotoForm( origform );
	}
	return;
}

double disttoSUAitem(SUAIndex *selectedSUA)
{	// calculate accurate nearest distance to SUA element
	// requires selectedSUA to point to existing SUAIndex data

	double neardist, chkdist;

	switch (selectedSUA->plottype) {
		case SUATFR:
		case SUACIRCLE: 
			LatLonToRange(data.input.gpslatdbl, data.input.gpslngdbl, selectedSUA->reflat, selectedSUA->reflon, &neardist); 
			if (neardist <= selectedSUA->maxdist) {
				// inside circle
				neardist = 0.0;
			} else { 
				// subtract radius for distance to circle
				neardist = neardist - selectedSUA->maxdist;
			}
			break;

		case SUAAWY:
			// check using checkdist function with width to get inside or distance
			chkdist = selectedSUA->width / 2.0; 
			neardist = checkdist(data.input.gpslatdbl, data.input.gpslngdbl, selectedSUA->startidx, selectedSUA->stopidx, 0);
			if (neardist <= (chkdist * chkdist)) {
				// inside airway
				neardist = 0.0;
			} else { 
				// subtract half airway width to get distance to airway
				neardist = Sqrt(neardist) - chkdist;
			}
			break;

		case SUAARC: // should never have SUAARC as first type but included here just in case
		case SUAPOINT:
			if (checkinout(data.input.gpslatdbl, data.input.gpslngdbl, selectedSUA->startidx, selectedSUA->stopidx)) {
				// inside polygon
				neardist = 0.0;
			} else {
				// find distance to polygon
				neardist = Sqrt(checkdist(data.input.gpslatdbl, data.input.gpslngdbl, selectedSUA->startidx, selectedSUA->stopidx, 0));
			}
			break;
		default:
			neardist = 0.0;
			break;
		}

	return(neardist);
}

Char* print_SUA_altitude(double val, Int8 units)
{
	static Char tempchar[10];
	double altconst = ALTMPHNAUCONST;;
        
        if (units == METRIC) altconst = ALTMETCONST;

	if (pround(val * altconst, 0) > 99999) {
		StrCopy(tempchar, "99999");
	} else if (pround(val * altconst, 0) < -99999) {
		StrCopy(tempchar,"-99999");
	} else {
	 	StrCopy(tempchar, DblToStr(pround(val * altconst,0),0));
	}
	if (units == METRIC) StrCat(tempchar, "m");
	return(tempchar);
}
