#include <PalmOS.h>	// all the system toolbox headers
#include "soaring.h"
#include "soarSTF.h"
#include "soarMath.h"
#include "soarForm.h"
#include "soarDB.h"
#include "soarUtil.h"
#include "soarUMap.h"
#include "soarWay.h"
#include "soarWLst.h"
#include "soarWind.h"
#include "soarTask.h"
#include "soarMem.h"
#include "soarTer.h"
#include "soarComp.h"
#include "soarCAI.h"

#define FINE_MC // controls if the "*" to indicate the MC value moves by whole lines only or not

Int16 MCSelYVal = 0;
double MCCurVal = 0.0;
double MCCurDispVal = 0.0;
Boolean forceupdate = false;
Boolean chgstartaltref = false;
double calcstfhwind;

extern UInt8 glidingtoheight;
extern Boolean clearrect;
extern double MCMult;
extern Int16 MCXVal;
extern Int16 MCYVal;
extern Int16 selectedFltindex;
extern Boolean graphing;
extern Boolean ManualDistChg;
extern Boolean inflight;
extern Boolean recv_data;
extern UInt32 thermalcount;
extern Boolean tasknotfinished;
extern UInt8 thmode;
extern double avgatthstart;
extern Int16 activetskway;
extern IndexedColorType  indexGreen, indexRed,indexBlack;
extern Boolean taskonhold;
extern FlightData *fltdata;
extern UInt32 utcsecs;
extern Char timeinfo[15];
extern UInt32 nogpstime;
extern Boolean skipmc;
extern Boolean FltInfoshowMC;

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
extern Boolean gps_sim;

static void final_glide_mc(double inputSi, UInt16 speedFieldID, UInt16 altFieldID)
{
	double Vstf, altitude, bearing;
	double inSi = 0;
	Char tempchar[15];
	double wptaalt;
	Boolean mcwptcrash = false;

	if (data.input.bearing_to_destination.valid==VALID) {
		// bearing to current waypoint
		bearing = data.input.bearing_to_destination.value;
	} else {
		// current track
		bearing = data.input.magnetic_track.value;
	}
	
//	HostTraceOutputTL(appErrorClass, "---------------");
//	HostTraceOutputTL(appErrorClass, "Input %s",DblToStr(inputSi,2));
//	HostTraceOutputTL(appErrorClass, "const %s",DblToStr(data.input.lftconst,2));

	// convert to correct units
	inSi = inputSi * MCMult;

	// Have to convert inSi to knots as that is what CalcSTFSpedAlt expects
	if (CalcSTFSpdAlt(inSi/data.input.lftconst, data.input.distance_to_destination.value,
				bearing, data.input.destination_elev,
				data.input.inusealt, true, &Vstf, &altitude) == false) {
		field_set_value(speedFieldID,"XX");
		field_set_value(altFieldID,"XX");
		return;
	}

//	HostTraceOutputTL(appErrorClass, "FG Spd %s",DblToStr(Vstf,2));
	if (data.config.showrgs) {
		Vstf = Vstf - calcstfhwind;
	}
	// display speed value
	field_set_value(speedFieldID, print_horizontal_speed2(Vstf, 0));

	// check for terrain crash en-route to destination at various MC values
	if (terrainpresent && terrainvalid && data.config.usefgterrain && (data.input.bearing_to_destination.valid==VALID) && !offterrain) {
		CalcSTFSpdAlt2(inSi/data.input.lftconst, data.input.distance_to_destination.value, bearing, &Vstf, &wptaalt);
		wptaalt = ConvertAltType(data.input.destination_elev, data.input.inusealt, true, ARVALT, wptaalt);
		mcwptcrash = terraincrash(wptterheights, numwptter, data.input.gpslatdbl, data.input.gpslngdbl, data.input.destination_lat, data.input.destination_lon,
					data.input.inusealt, wptaalt+data.input.destination_elev, true);
		if (mcwptcrash && !offterrain) {
			switch (data.config.alttype) {
				case REQALT:
					altitude = data.input.inusealt - crashalt;
					break;
				case ARVALT:
					// if crash is more than safety height, adjust AAlt figure
					if (crashalt + data.config.safealt < 0) {
						altitude = crashalt + data.config.safealt;
					}
					break;
				case DLTALT:
					altitude = crashalt;
					break;
				default:
					break;
			}
		}
	} else {
		crash = false;
		crash1 = false;
		crashalt = 0.0;
	}

	// display altitude value
	field_set_value(altFieldID, print_altitude(altitude));

	if (inputSi == 0.0) { // do once once

		// show warning triangle for off terrain under Alt Ref Label
		if (offterrain && data.config.usefgterrain) {
//			HostTraceOutputTL(appErrorClass, "Triangle on FG");
			FntSetFont((FontID)WAYSYMB11);
			if (device.colorCapable) {
				WinSetTextColor(indexRed);
			}

			tempchar[0] = 11; // fell off terrain map!
			tempchar[1] = '\0';
			// under altitude field label
			WinDrawChars(tempchar, 1, WIDTH_MIN+63, HEIGHT_MIN+25);	

			if (device.colorCapable) {
				WinSetTextColor(indexBlack);
			}
			FntSetFont(stdFont);
		}
	}

	// Calculations for "*" placement
	MCSelYVal = (Int16)(inputSi/data.config.mcrngmult) * 12 + MCYVal;

	// draw warning triangle for terrain crash
	if (mcwptcrash && !offterrain) {
//		HostTraceOutputTL(appErrorClass, "Triangle on FG");
		FntSetFont((FontID)WAYSYMB11);
		if (device.colorCapable) {
			WinSetTextColor(indexRed);
		}
		tempchar[0] = 9;
		tempchar[1] = '\0';
		// between speed and altitude value
		WinDrawChars(tempchar, 1, WIDTH_MIN+50, HEIGHT_MIN+1+MCSelYVal);	
		if (device.colorCapable) {
			WinSetTextColor(indexBlack);
		}
		FntSetFont(stdFont);
	} else {
		StrCopy(tempchar, "      ");
		WinDrawChars(tempchar, 6, WIDTH_MIN+50, HEIGHT_MIN+1+MCSelYVal);
	}

#ifndef FINE_MC
//	HostTraceOutputTL(appErrorClass, "MCsel %s",DblToStr(MCSelYVal,2));
	if ((MCCurDispVal >= 0.0) && (inputSi == MCCurDispVal)) {
		FntSetFont(largeBoldFont);
		WinDrawChars("*", 1, MCXVal-8, MCSelYVal+1);
		FntSetFont(stdFont);
	} else {
		FntSetFont(largeBoldFont);
		WinEraseChars("*", 1, MCXVal-8, MCSelYVal+1);
		FntSetFont(stdFont);
	}
#endif
}

void final_glide_event()
{
	Char tempchar[20];
	double WaySTF=0.0;
	double fgatpdist=0.0, fgatpbear=0.0;
	double fgatpalt1=0.0;
	double fgatpalt2=0.0;
	double startalt=0.0;
	double firstaltloss;
	double secondaltloss;
	static RectangleType rectP;
	Int16 TAKEOFFSET, LANDOFFSET;
//	double refRng, refBrg;

//	HostTraceOutputTL(appErrorClass, "Main Final Glide Event");

	if (data.config.usepalmway) {
		if (data.input.destination_valid) {
			// Find the STF altitude for current distance and bearing
			// This function fills in the bearing and range for the waypoints
			//    the altitude is ignored and recalculated below.
			startalt = data.input.inusealt;

			// Calculates the STF Speed and Altitude values for the current Waypoint
			// CalcSTFSpdAlt expects the first parameter to be in knots
			CalcSTFSpdAlt2(MCCurVal, data.input.distance_to_destination.value, data.input.bearing_to_destination.value, &WaySTF, &data.input.destination_aalt);
			firstaltloss = data.input.destination_aalt;
			fgatpalt1 = ConvertAltType(data.input.destination_elev, startalt, true, REQALT, firstaltloss);
//			HostTraceOutputTL(appErrorClass, "fgatpalt1 = %s", DblToStr(fgatpalt1,0));

			// Find the STF altitude from current waypoint to:
			//      The Finish Point if a task is active or
			//      The HOME waypoint if no task is active
			if (data.task.numwaypts > 0) {
				if (data.task.hastakeoff) {
				 	TAKEOFFSET = 1;
				} else {
					TAKEOFFSET = 0;
				}
				if (data.task.haslanding) {
					LANDOFFSET = data.task.numwaypts - 1;
				} else {
					LANDOFFSET = data.task.numwaypts;
				}
				// get FGA altitude
				fgatpalt2 = data.input.FGAdispalt;

				// check task rules
				if (data.task.rulesactive && !taskonhold) {
					// display info above waypoint arrow
					if ((StrLen(timeinfo) > 0) && tasknotfinished) {
						if ((data.task.startlocaltime > 0) && (activetskway <= TAKEOFFSET) && inflight) {
							// center start time
							StrCopy(tempchar, "  ");
							if (StrCompare(Left(timeinfo,1), "(") != 0) StrCat(tempchar, "  "); 
							StrCat(tempchar, Left(timeinfo,7)); 
						} else {
							// center TOT
							StrCopy(tempchar, " ");
							StrCat(tempchar, Left(timeinfo,7));
						}
						field_set_value(form_final_glide_fgto1000m, tempchar);
						ctl_set_visible(form_final_glide_fgto1000m, true);
					} else if (data.task.fgto1000m || (data.task.finishheight > 0.0)) {
						// display original finish elevation if changed by task rules
						if (!taskonhold && (activetskway == (LANDOFFSET-1)) && (data.task.elevations[LANDOFFSET-1] != data.activetask.finishheight)) {
							// indicate final gliding to 1000m below start height
							// by showinf original waypoint elevation
							StrCopy(tempchar, "(");
							StrCat(tempchar, print_altitude(data.task.elevations[LANDOFFSET-1]));
							StrCat(tempchar, ")");
							field_set_value(form_final_glide_fgto1000m, tempchar);
							ctl_set_visible(form_final_glide_fgto1000m, true);
						} else {
							ctl_set_visible(form_final_glide_fgto1000m, false);
						}
					} else {
						ctl_set_visible(form_final_glide_fgto1000m, false);
					}
				} else if (taskonhold) {
					field_set_value(form_final_glide_fgto1000m, timeinfo);
					ctl_set_visible(form_final_glide_fgto1000m, true);
				} else {
					ctl_set_visible(form_final_glide_fgto1000m, false);
				}
				
				// show warning triangle for task crash or off terrain under FGA Label
				if (tskcrash || (tskoffterrain && data.config.usefgterrain)) {
//					HostTraceOutputTL(appErrorClass, "Triangle on FG");
					FntSetFont((FontID)WAYSYMB11);
					if (device.colorCapable) {
						WinSetTextColor(indexRed);
					}
					if (tskoffterrain) {
						tempchar[0] = 11; // fell off terrain map!
					} else {
						tempchar[0] = 9;
					}
					tempchar[1] = '\0';
					// under FGA field label
					WinDrawChars(tempchar, 1, WIDTH_MIN+147, HEIGHT_MIN+117);	

					if (device.colorCapable) {
						WinSetTextColor(indexBlack);
					}
					FntSetFont(stdFont);
				}
			} else {
//				HostTraceOutputTL(appErrorClass, "Inside GOTO Waypoint FGA Case");
				// Setting fgatpalt2 to fgatpalt1 to cover the case where
				// the current waypoint/turnpoint is the HOME/Finish waypoint/turnpoint
//				tskcrash = false;
		 		if (data.config.alttype == REQALT) {
					fgatpalt2 = fgatpalt1;
				} else {
					fgatpalt2 = ConvertAltType(data.input.homeElev, startalt, true, data.config.alttype, firstaltloss);
				}

				if (data.input.destination_lat != data.input.homeLat || data.input.destination_lon != data.input.homeLon) {
					LatLonToRangeBearing(data.input.destination_lat, data.input.destination_lon, data.input.homeLat, data.input.homeLon, &fgatpdist, &fgatpbear);

					// Convert to Magnetic
					fgatpbear = nice_brg(fgatpbear + data.input.deviation.value);

					// CalcSTFSpdAlt expects the first parameter to be in knots
					CalcSTFSpdAlt2(MCCurVal, fgatpdist, fgatpbear, &WaySTF, &secondaltloss);
					fgatpalt2 = ConvertAltType(data.input.homeElev, startalt, true, REQALT, (firstaltloss+secondaltloss));
					if (data.config.alttype != REQALT) {
						fgatpalt2 = ConvertAltType(data.input.homeElev, startalt, true, data.config.alttype,  (firstaltloss+secondaltloss));
					}
				}
			}
//			HostTraceOutputTL(appErrorClass, "------------------------");

		} else if (!ManualDistChg) {
			// clear inuseWaypoint
			data.input.destination_name[0] = 0;
			data.input.destination_elev = 0.0;
			data.input.distance_to_destination.valid = VALID;
			data.input.bearing_to_destination.valid = NOT_VALID;
			data.input.bearing_to_destination.value = 000.0;
			data.input.destination_valid = false;
		} else {
			// manual distance change
			data.input.distance_to_destination.valid = VALID;
			data.input.bearing_to_destination.valid = NOT_VALID;
			data.input.bearing_to_destination.value = 000.0;
			data.input.destination_valid = false;
		}

/*		if ((data.input.refwpttype != REFNONE) && (data.input.refLat != INVALID_LAT_LON) && (data.input.refLon != INVALID_LAT_LON)) {
			// display range and bearing from/to ref point
			LatLonToRangeBearing(data.input.gpslatdbl, data.input.gpslngdbl, data.input.refLat, data.input.refLon, &refRng, &refBrg);
			refBrg = nice_brg(refBrg + data.input.deviation.value);
			StrCopy(tempchar, "  ");
			if (data.config.RefWptRadial) {
				refBrg = RecipCse(refBrg);
				StrCat(tempchar, print_direction2(refBrg));
				StrCat(tempchar, "R");
			} else {
				StrCat(tempchar, print_direction2(refBrg));
				StrCat(tempchar, "°");
			}
			if (pround(refRng*data.input.disconst,0) > 999.0) {
				StrCat(tempchar, "999");
			} else {
				StrCat(tempchar, print_distance2(refRng,0));
			}
			field_set_value(form_final_glide_fgto1000m, Right(tempchar,7));
			ctl_set_visible(form_final_glide_fgto1000m, true);
		}
*/
	}

	// Updates the Final Glide Around Turnpoint Value
	field_set_value(form_final_glide_fga, print_altitude(fgatpalt2));

	// Remark the print functions check the validity of the data
	if (!ManualDistChg) {
		if ((data.input.distance_to_destination.value*data.input.disconst) < 10.0) {
			StrCopy(tempchar, print_distance2(data.input.distance_to_destination.value, 1));
		} else if ((data.input.distance_to_destination.value*data.input.disconst) < 1000.0) {
			StrCopy(tempchar, print_distance2(data.input.distance_to_destination.value, 0));
		} else {
			StrCopy(tempchar, "999");
		}
		field_set_value(form_final_glide_dis, tempchar);
	}

	ManualDistChg = false;

	field_set_value(form_final_glide_gs, print_horizontal_speed(data.input.ground_speed));
	field_set_value(form_final_glide_bearing, print_direction(data.input.bearing_to_destination));
	field_set_value(form_final_glide_track, print_direction(data.input.magnetic_track));

	// Draw the control point bearing and distance if valid and meets criteria
	if (((data.config.shownextwpt == NEXTALL) || ((data.config.shownextwpt == NEXTCTL) && data.input.isctlpt)) && (data.input.nextwpt_dist >= 0.0)) {
		StrCopy(tempchar,"  ");
		StrCat(tempchar, print_direction2(data.input.nextwpt_bear));
		StrCat(tempchar, "°  ");
	
		if ((data.input.nextwpt_dist*data.input.disconst) < 10.0) {
			StrCat(tempchar, print_distance2(data.input.nextwpt_dist, 1));
//		} else if ((data.input.nextwpt_dist*data.input.disconst) < 100.0) {
//			StrCat(tempchar, print_distance2(data.input.nextwpt_dist, 1));
		} else if ((data.input.nextwpt_dist*data.input.disconst) < 1000.0) {
			StrCat(tempchar, print_distance2(data.input.nextwpt_dist, 0));
		} else {
			StrCat(tempchar, "999");
		}
		field_set_value(form_final_glide_bearrng, tempchar);
	} else {
		field_set_value(form_final_glide_bearrng, " ");
	}

	if (data.input.destination_type & AREA) {
		field_set_value(form_final_glide_wptlabel, "Area:");
	} else	if (data.input.destination_type & CONTROL) {
		field_set_value(form_final_glide_wptlabel, "Ctl:");
	} else {
		field_set_value(form_final_glide_wptlabel, "Wpt:");
	}

	if (data.config.altreftype == AGL) {
		if (terrainvalid) {
			field_set_value(form_final_glide_curalt, print_altitude(data.input.inusealt-data.input.terelev));
		} else {
			field_set_value(form_final_glide_curalt, "N/A");
		}
	} else if (data.config.altreftype == QFE) {
		field_set_value(form_final_glide_curalt, print_altitude(data.input.inusealt-data.config.qfealt));
	} else if (data.config.altreftype == PALT) {
		// FL always in ft
		field_set_value(form_final_glide_curalt, DblToStr(pround(data.logger.pressalt,0),0));
	} else {
		field_set_value(form_final_glide_curalt, print_altitude(data.input.inusealt));
	}

	// msr: now displaying thermal average in place of current lift
	field_set_value(form_final_glide_lift, DblToStr(pround(data.input.thavglift*data.input.lftconst, data.input.lftprec), data.input.lftprec));
	field_set_value(form_final_glide_alift, DblToStr(pround(data.input.avglift*data.input.lftconst, data.input.lftprec), data.input.lftprec));

#ifdef FINE_MC
	// clear the place for the * marker for the current MC value
	rectP.topLeft.x = MCXVal-8;
	rectP.topLeft.y = 36;
	rectP.extent.x = 8;
	rectP.extent.y = 73;
	WinEraseRectangle(&rectP, 0);
	tempchar[1] = 0;
	
	// calculate position of "*" in MC / STF grid
	FntSetFont(largeBoldFont);
	MCCurDispVal = pround(MCCurVal*data.input.lftconst/data.config.mcrngmult/MCMult, 1) * data.config.mcrngmult;
	if (MCCurDispVal < 0.0) {
		MCCurDispVal = 0.0;
		WinDrawChars("*", 1, MCXVal-8, MCYVal+1);
	} else if (MCCurDispVal > 5.0*data.config.mcrngmult) {
		MCCurDispVal = 5.0*data.config.mcrngmult;
		WinDrawChars("*", 1, MCXVal-8, 60 + MCYVal+1);
	} else {
		WinDrawChars("*", 1, MCXVal-8, (Int16)(MCCurVal*data.input.lftconst/data.config.mcrngmult/MCMult*12) + MCYVal+1);
	}
	FntSetFont(stdFont);
#else
	// calculate position of "*" in MC / STF grid
	MCCurDispVal = pround(MCCurVal*data.input.lftconst/data.config.mcrngmult/MCMult, 0) * data.config.mcrngmult;
	if (MCCurDispVal < 0.0) {
		MCCurDispVal = 0.0;
	}
	if (MCCurDispVal > 5.0*data.config.mcrngmult) {
		MCCurDispVal = 5.0*data.config.mcrngmult;
	}
#endif

	rectP.topLeft.x = GPSX;
	rectP.topLeft.y = GPSY;
	rectP.extent.x = GPSXOFF+1;
	rectP.extent.y = GPSYOFF;
	if (!recv_data || (nogpstime > 0)) {
		WinEraseRectangle(&rectP, 0);
		FntSetFont(boldFont);
		WinDrawInvertedChars(" NO GPS", 7, GPSX, GPSY);
		FntSetFont(stdFont);
		clearrect = true;
	} else if (StrCompare(data.logger.gpsstat, "V") == 0) {
		WinEraseRectangle(&rectP, 0);
		FntSetFont(boldFont);
		WinDrawInvertedChars("NOSATS", 6, GPSX, GPSY);
		FntSetFont(stdFont);
		clearrect = true;
	} else {
		if (clearrect) {
			WinEraseRectangle(&rectP, 0);
			clearrect = false;
		}
		if (!gps_sim) {
			StrCopy(tempchar, "G");
			StrCat(tempchar, data.input.gpsnumsats);
			field_set_value(form_final_glide_gpsstat, tempchar);
		} else {
			FntSetFont(boldFont);
			WinDrawInvertedChars(" SIM ", 5, GPSX, GPSY);
			FntSetFont(stdFont);		
		}
	}

	// Update the Headwind Value when it has changed
	if (data.config.hwposlabel) {
//		HostTraceOutputTL(appErrorClass, "data.input.headwind1 =|%s|", DblToStr(data.input.headwind, 1));
		field_set_value(form_final_glide_headwind, DblToStr(pround(data.input.headwind*data.input.wndconst,1),1));
	} else {
//		HostTraceOutputTL(appErrorClass, "data.input.headwind2 =|%s|", DblToStr(data.input.headwind, 1));
		field_set_value(form_final_glide_headwind, DblToStr(pround(data.input.headwind*data.input.wndconst*(-1),1),1));
	}

	// Update the Course to the Waypoint Arrow
	DrawFGWayLine(ARROWCENTX, ARROWCENTY);
}

void final_glide_event0()
{
	if (data.input.distance_to_destination.valid == VALID) {
		final_glide_mc(0.0*data.config.mcrngmult,form_final_glide_0_speed, form_final_glide_0_alt);
	} else {
		field_set_value(form_final_glide_0_speed, "");
		field_set_value(form_final_glide_0_alt, "");
	}
	if ((StrCompare(data.input.destination_name, ",") == 0 || StrLen(data.input.destination_name) == 0)) {
		field_set_value(form_final_glide_waypt, "Not Selected");
	} else {
		switch (glidingtoheight) {
			case STARTHEIGHT:
				// gliding to max start height so change name
				field_set_value(form_final_glide_waypt, "Start Height");
				break;
			case FINISHHEIGHT:
				// gliding to min finish height so change name
				field_set_value(form_final_glide_waypt, "Finish Height");
				break;
			case ELEVATION:
			default:
				// display target name normally
				field_set_value(form_final_glide_waypt, data.input.destination_name);
				break;
		}
		if (data.config.usepalmway) {
			field_set_value(form_final_glide_wayelev, print_altitude(data.input.destination_elev));
			WinDrawChars(data.input.alttext, 2, 51, 123);
		} else {
			field_set_value(form_final_glide_wayelev, print_altitude(data.input.destination_elev));
		}
	}
}

void final_glide_event1()
{
	double tempdbl, remainder;
	if (data.input.distance_to_destination.valid == VALID) {
		final_glide_mc(1.0*data.config.mcrngmult,form_final_glide_1_speed,form_final_glide_1_alt);
	} else {
		field_set_value(form_final_glide_1_speed, "");
		field_set_value(form_final_glide_1_alt, "");
	}
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
		field_set_value(form_final_glide_basemc,
			DblToStr(pround(data.input.basemc*data.input.lftconst, data.input.lftprec), 
			data.input.lftprec));
	}
}

void final_glide_event2()
{
	if (data.input.distance_to_destination.valid == VALID) {
		final_glide_mc(2.0*data.config.mcrngmult,form_final_glide_2_speed, form_final_glide_2_alt);
	} else {
		field_set_value(form_final_glide_2_speed, "");
		field_set_value(form_final_glide_2_alt, "");
	}

}

void final_glide_event3()
{
	if (data.input.distance_to_destination.valid == VALID) {
		final_glide_mc(3.0*data.config.mcrngmult,form_final_glide_3_speed, form_final_glide_3_alt);
	} else {
		field_set_value(form_final_glide_3_speed, "");
		field_set_value(form_final_glide_3_alt, "");
	}
}

void final_glide_event4()
{
	if (data.input.distance_to_destination.valid == VALID) {
		final_glide_mc(4.0*data.config.mcrngmult,form_final_glide_4_speed, form_final_glide_4_alt);
	} else {
		field_set_value(form_final_glide_4_speed, "");
		field_set_value(form_final_glide_4_alt, "");
	}
}

void final_glide_event5()
{
	if (data.input.distance_to_destination.valid == VALID) {
		final_glide_mc(5.0*data.config.mcrngmult,form_final_glide_5_speed, form_final_glide_5_alt);
	} else {
		field_set_value(form_final_glide_5_speed, "");
		field_set_value(form_final_glide_5_alt, "");
	}
}

void Update_Fltinfo(Int16 fltindex, Boolean listactive, Boolean fltinfo)
{
	Char tempchar[30];
	DateTimeType strtdt, stopdt;
	UInt32 startsecs, stopsecs, lengthsecs, elapsedsecs;
	MemHandle flt_hand;
	MemPtr flt_ptr;
	double ld, speed;
	UInt16 TAKEOFFSET;
	Int16 n, nrecs;

//	HostTraceOutputTL(appErrorClass, "Update_Fltinfo");
	nrecs = OpenDBCountRecords(flight_db);

	if (listactive && (nrecs > 0) && (data.task.numwaypts <= 0)) {
		frm_set_title("Flight/Task Info");
		// flight selector
		ctl_set_enable(form_flt_info_pop1, true);
		ctl_set_visible(form_flt_info_flightlbl, true);
		ctl_set_visible(form_flt_info_pop1, true);
		// graphing buttons
		ctl_set_visible(form_flt_info_graphbtn, true);
		ctl_set_visible(form_flt_info_graphbtn2, true);
		ctl_set_visible(form_flt_info_graphbtn3, true);
		// task label / button
		ctl_set_visible(form_flt_info_taskbtnlbl, true);
		ctl_set_visible(form_flt_info_taskbtn, false);
		// glide ratios
		ctl_set_visible(form_flt_info_ldilabel, false);
		ctl_set_visible(form_flt_info_ldi, false);
		ctl_set_visible(form_flt_info_ldalabel, false);
		ctl_set_visible(form_flt_info_lda, false);
		ctl_set_visible(form_flt_info_ldrlabel, false);
		ctl_set_visible(form_flt_info_ldr, false);
		// speed fields
		ctl_set_visible(form_flt_info_avgtskspd, false);
		ctl_set_visible(form_flt_info_speedreqdlbl2, false);
		ctl_set_visible(form_flt_info_speedreqd, false);
		ctl_set_visible(form_flt_info_speedreqdlbl, false);
		// old items
//		ctl_set_visible(form_flt_info_resetlbl, false);
//		ctl_set_visible(form_flt_info_ldlabel, false);
//		ctl_set_visible(form_flt_info_tsketa_label, false);
//		ctl_set_visible(form_flt_info_tskstop_label, true);
		frm_set_label(form_flt_info_tskspdlbl, "Avg:");
		frm_set_label(form_flt_info_spdlbl, data.input.tskspdtext);
		FltInfoshowMC = false;
	} else {
		// current waypoint info
		if (data.task.numwaypts <= 0) {
			frm_set_title("Flight/Task Info");
		} else if ((StrCompare(data.input.destination_name, ",") == 0 || StrLen(data.input.destination_name) == 0)) {
			frm_set_title("Not Selected");
		} else {
			frm_set_title(data.input.destination_name);
		}
		// flight selectors
		if (nrecs > 0) ctl_set_label(form_flt_info_pop1, "Current");
		ctl_set_enable(form_flt_info_pop1, false);
		ctl_set_visible(form_flt_info_flightlbl, false);
		ctl_set_visible(form_flt_info_pop1, false);
		// graphing buttons
		ctl_set_visible(form_flt_info_graphbtn, false);
		ctl_set_visible(form_flt_info_graphbtn2, false);
		ctl_set_visible(form_flt_info_graphbtn3, false);
		// task label / button
		ctl_set_visible(form_flt_info_taskbtn, false);
		ctl_set_visible(form_flt_info_taskbtnlbl, true);
		// glide ratios
		ctl_set_visible(form_flt_info_ldilabel, true);
		ctl_set_visible(form_flt_info_ldi, true);
		ctl_set_visible(form_flt_info_ldalabel, true);
		ctl_set_visible(form_flt_info_lda, true);
		ctl_set_visible(form_flt_info_ldrlabel, true);
		ctl_set_visible(form_flt_info_ldr, true);
		// speed fields
//		ctl_set_visible(form_flt_info_avgtskspd, false);
//		ctl_set_visible(form_flt_info_speedreqdlbl2, false);
//		ctl_set_visible(form_flt_info_speedreqd, false);
//		ctl_set_visible(form_flt_info_speedreqdlbl, false);
		// old times
//		ctl_set_visible(form_flt_info_resetlbl, true);
//		ctl_set_visible(form_flt_info_ldlabel, true);
//		if (nrecs > 0) {
//			ctl_set_visible(form_flt_info_tskstop_label, false);
//			ctl_set_visible(form_flt_info_tsketa_label, true);
//		}
		frm_set_label(form_flt_info_tskspdlbl, "MC:");
		frm_set_label(form_flt_info_spdlbl, data.input.lfttext);
		FltInfoshowMC = true;
	}

	if (nrecs > 0) {
		OpenDBQueryRecord(flight_db, fltindex, &flt_hand, &flt_ptr);
		MemMove(fltdata, flt_ptr, sizeof(FlightData));
		MemHandleUnlock(flt_hand);
	} else {
		MemSet(fltdata, sizeof(FlightData), 0);
	}

//****************************************************************************
// FLIGHT INFO
//****************************************************************************
	if (fltinfo) { // Update the flight information

// calculate glide angles

/*		// instant
		if ((data.input.ground_speed.valid == VALID) && (data.input.curlift != 0.0)) {
			ld = -data.input.ground_speed.value / data.input.curlift;
			n = 1;
			if (Fabs(ld) >= 100) n = 0;
			if (Fabs(ld) >= 999) ld = 999;
			field_set_value(form_flt_info_ldi, DblToStr(ld,n));
		} else {
			field_set_value(form_flt_info_ldi, "XX");		
		}
*/
		// since last thermal
		if (thmode == THERMAL) {
			ld = data.input.lastglide;
			n = 1;
			if (Fabs(ld) >= 100) n = 0;
			if (Fabs(ld) >= 999) ld = 999;
			field_set_value(form_flt_info_ldi, DblToStr(ld,n));
		} else if ((data.input.distance_to_destination.valid == VALID) && ((data.input.startcruisealt - data.input.inusealt) != 0.0)) {
			ld = (data.input.startcruisedist - data.input.distance_to_destination.value)*DISTFTCONST / (data.input.startcruisealt - data.input.inusealt);
			n = 1;
			if (Fabs(ld) >= 100) n = 0;
			if (Fabs(ld) >= 999) ld = 999;
			field_set_value(form_flt_info_ldi, DblToStr(ld,n));
		} else {
			field_set_value(form_flt_info_ldi, "XX");		
		}		

		// average
		if ((data.input.distdone != 0.0) && (data.input.avglift != 0.0)) {
			ld = -data.input.distdone*2*60 / data.input.avglift; // assuming 30 sec average
			n = 1;
			if (Fabs(ld) >= 100) n = 0;
			if (Fabs(ld) >= 999) ld = 999;
			field_set_value(form_flt_info_lda, DblToStr(ld,n));
		} else {
			field_set_value(form_flt_info_lda, "XX");		
		}

		// required (including safety height)
		if ((data.input.distance_to_destination.valid == VALID) && (data.input.inusealt - (data.input.destination_elev + data.config.safealt) != 0.0)) {
			ld = data.input.distance_to_destination.value*DISTFTCONST / (data.input.inusealt - (data.input.destination_elev + data.config.safealt));
			n = 1;
			if (Fabs(ld) >= 100) n = 0;
			if (Fabs(ld) >= 999) ld = 999;
			field_set_value(form_flt_info_ldr, DblToStr(ld,n));
		} else {
			field_set_value(form_flt_info_ldr, "XX");		
		}
// flight start
		// Copy the flight DTG(MMDDYY) into the Start DateTime Structure
		// Copy the Start time UTC (HHMMSS) into the Start DateTime Structure
		StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &strtdt);
		startsecs = TimDateTimeToSeconds(&strtdt);
		SecondsToDateOrTimeString(startsecs, tempchar, 1, (Int32)data.config.timezone);
		field_set_value(form_flt_info_start, tempchar);
// flight stop
		// Copy the flight Stop DTG(MMDDYY) into the Stop DateTime Structure and
		// Copy the flight Stop time UTC (HHMMSS) into the Stop DateTime Structure
		// If the flight is not in progress.
		// If the flight is in progress, use the current GPS DTG and UTC
		if (inflight) {
			//StringToDateAndTime(data.logger.gpsdtg, data.logger.gpsutc, &stopdt);
			//stopsecs = TimDateTimeToSeconds(&stopdt);
			stopsecs = utcsecs;
		} else {
			StringToDateAndTime(fltdata->stopdtg, fltdata->stoputc, &stopdt);
			stopsecs = TimDateTimeToSeconds(&stopdt);
		}
		SecondsToDateOrTimeString(stopsecs, tempchar, 1, (Int32)data.config.timezone);
		field_set_value(form_flt_info_stop, tempchar);
// flight length
		// Calculate flt length & convert HH:MM:SS for display
		if (startsecs > stopsecs) lengthsecs = 0; else lengthsecs = stopsecs - startsecs;
		SecondsToDateOrTimeString(lengthsecs, tempchar, 1, 0);
		field_set_value(form_flt_info_length,tempchar);
// max alt
		if (fltdata->maxalt == -999.9) {
			field_set_value(form_flt_info_maxalt,print_altitude(data.input.maxalt));
		} else {
			field_set_value(form_flt_info_maxalt,print_altitude(fltdata->maxalt));
		}
// % thermal
		if (inflight) {
			if (data.flight.stopidx > data.flight.startidx) {
				field_set_value(form_flt_info_pctthermal, DblToStr((((double)thermalcount/(double)(data.flight.stopidx - data.flight.startidx))*100.0), 1));
			}else {
				field_set_value(form_flt_info_pctthermal, "0.0");
			}
		} else {
			field_set_value(form_flt_info_pctthermal, DblToStr((fltdata->pctthermal*100.0), 1));
		}
// terrain height
		if (terrainvalid) {
			field_set_value(form_flt_info_terelev, print_altitude(data.input.terelev));
		} else {
			field_set_value(form_flt_info_terelev, "N/A");
		}

//****************************************************************************
// TASK INFO
//****************************************************************************
	} else { // Update the task information
//		HostTraceOutputTL(appErrorClass, "Update_Fltinfo - Logs");
//		HostTraceOutputTL(appErrorClass, "                 fltdata->tskstartutc-|%s|", fltdata->tskstartutc);
//		HostTraceOutputTL(appErrorClass, "                 nrecs-|%hd|", nrecs);
//		HostTraceOutputTL(appErrorClass, "                 fltdata->flttask.numwaypts-|%hu|", fltdata->flttask.numwaypts);

		if ((StrCompare(fltdata->tskstartutc, "NT") != 0) && (nrecs > 0) && (fltdata->flttask.numwaypts > 0)) {
//			HostTraceOutputTL(appErrorClass, "Update_Fltinfo - Task set so update task info");
//********************
// task started
//********************
			// show task button or label when the logger is not running
			if (listactive && (data.task.numwaypts <= 0)) {
				ctl_set_visible(form_flt_info_taskbtnlbl, false);
				ctl_set_visible(form_flt_info_taskbtn, true);
			}
// task start time
			// Copy the flight DTG(MMDDYY) into the Start & Stop DateTime Structure
			// Copy the task Start time utc (HHMMSS) into the Start DateTime Structure
			if (tasknotfinished && inflight) {
				startsecs = data.activetask.tskstartsecs;
			} else {
				StringToDateAndTime(fltdata->tskstartdtg, fltdata->tskstartutc, &strtdt);
				startsecs = TimDateTimeToSeconds(&strtdt);
			}
			SecondsToDateOrTimeString(startsecs, tempchar, 1, (Int32)data.config.timezone);
			field_set_value(form_flt_info_tskstart, tempchar);

// start height
			// Check for request to change start height alt ref
			if (chgstartaltref) {
				if (fltdata->flttask.startaltref == MSL) {
					fltdata->flttask.startaltref = AGL;
				} else {
					fltdata->flttask.startaltref = MSL;
				}
			}
			// Copy the task start height
			if (fltdata->flttask.startaltref == AGL) {
				// display above start point value
				if (fltdata->flttask.hastakeoff) TAKEOFFSET = 1; else TAKEOFFSET = 0;
				StrCopy(tempchar, print_altitude(fltdata->startheight - fltdata->flttask.elevations[TAKEOFFSET]));
				ctl_set_visible(form_flt_info_tskstartaltMSL, false);
				ctl_set_visible(form_flt_info_tskstartaltAGL, true);
			} else {
				// display MSL value
				StrCopy(tempchar, print_altitude(fltdata->startheight));
				ctl_set_visible(form_flt_info_tskstartaltAGL, false);
				ctl_set_visible(form_flt_info_tskstartaltMSL, true);
			}
			// check start height
			if ((data.task.numwaypts > 0) && (data.activetask.tskstartsecs == 0)) {
				StrCopy(tempchar, "0");
			}
			field_set_value(form_flt_info_tskstartalt, tempchar);

// task stop
			// Copy the task Stop DTG(MMDDYY) into the Stop DateTime Structure and
			// Copy the task Stop time UTC (HHMMSS) into the Stop DateTime Structure
			// If the task is not in progress.
			// If the task is in progress, use the current GPS DTG and UTC
			// Copy the task Stop time UTC (HHMMSS) into the Stop DateTime Structure
			if (tasknotfinished && inflight) {
				fltdata->tskdist = data.flight.tskdist;
				fltdata->flttask.ttldist = data.task.ttldist;
				fltdata->tskspeed = data.flight.tskspeed;
				stopsecs = data.activetask.tskstopsecs;
			} else {
				StringToDateAndTime(fltdata->tskstopdtg, fltdata->tskstoputc, &stopdt);
				stopsecs = TimDateTimeToSeconds(&stopdt);
			}
			SecondsToDateOrTimeString(stopsecs, tempchar, 1, (Int32)data.config.timezone);
// task elapsed
			// Calculate task elapsed time & convert HH:MM:SS for display
			if (startsecs > stopsecs) lengthsecs = 0; else lengthsecs = stopsecs - startsecs;
			if (startsecs > 0) {
				elapsedsecs = lengthsecs;
			} else {
				elapsedsecs = 0;
			}
			SecondsToDateOrTimeString(elapsedsecs, tempchar, 1, 0);
			field_set_value(form_flt_info_tskelapsed, tempchar);
// task dist done
			field_set_value(form_flt_info_tskdist, print_distance2(fltdata->tskdist,1));
// task dist to go
			field_set_value(form_flt_info_disttogo, print_distance2(fltdata->flttask.ttldist - fltdata->tskdist,1));
// task total dist or MC
				field_set_value(form_flt_info_tskttldist, print_distance2(fltdata->flttask.ttldist,1));
// task speed
			if (FltInfoshowMC) {
				field_set_value(form_flt_info_tskspd, DblToStr(pround(data.input.basemc*data.input.lftconst, data.input.lftprec), data.input.lftprec));
			} else {
				field_set_value(form_flt_info_tskspd, DblToStr(pround(fltdata->tskspeed*data.input.tskspdconst,1),1));
			}
//********************
// logger not running
//********************
			if (listactive) {
				// don't show required task speeds or time to go
				ctl_set_visible(form_flt_info_speedreqd, false);
				ctl_set_visible(form_flt_info_speedreqdlbl, false);
				ctl_set_visible(form_flt_info_avgtskspd, false);
				ctl_set_visible(form_flt_info_speedreqdlbl2, false);
				field_set_value(form_flt_info_timetogo, " ");
// current time on task
				SecondsToDateOrTimeString(lengthsecs, tempchar, 1, 0);
				field_set_value(form_flt_info_tskarrive,tempchar);
// task finish time
				SecondsToDateOrTimeString(stopsecs, tempchar, 1, (Int32)data.config.timezone);
				field_set_value(form_flt_info_tskstop, tempchar);
// show variance to specified task time
				if (fltdata->flttask.rulesactive && (fltdata->flttask.mintasktime > 0)) {
					field_set_value(form_flt_info_early, CalcTOTvar(lengthsecs/60, fltdata->flttask.mintasktime));
					field_set_value(form_flt_info_speedtype, "OTA");
				} else {
					field_set_value(form_flt_info_early, " ");
					field_set_value(form_flt_info_speedtype, " ");
				}
			} else {
//********************
// in flight
//********************
// estimated total time on task
				if (!taskonhold) {
					if (!tasknotfinished) {
						// task finished so show actual task stop time.
						SecondsToDateOrTimeString(lengthsecs, tempchar, 1, 0);
						field_set_value(form_flt_info_tskarrive, tempchar);
					} else if ((data.flight.tskspeed > 0.0) || (data.input.Vxc > 0.0)) {
						if (data.activetask.TOTsecs > 0) lengthsecs = data.activetask.TOTsecs; else lengthsecs = 0;
						SecondsToDateOrTimeString(lengthsecs, tempchar, 1, 0);
						field_set_value(form_flt_info_tskarrive,tempchar);
					} else {
						// task speed not valid, cannot calculate TOT
						field_set_value(form_flt_info_tskarrive,"N/A");
					}
				} else {
					// task on hold
					field_set_value(form_flt_info_tskarrive, "On Hold");
				}
// estimated task finish
				if (!taskonhold) {
					if (data.flight.tskspeed > 0.0) {
						// Use task speed
						SecondsToDateOrTimeString(lengthsecs+startsecs, tempchar, 1, (Int32)data.config.timezone);
						field_set_value(form_flt_info_tskstop, tempchar);
					} else {
						field_set_value(form_flt_info_tskstop, "N/A");
					}
				} else {
					// task on hold
					field_set_value(form_flt_info_tskstop, "On Hold");
				}
// show variance to specified task time and speed required
				if (!taskonhold) {
					// speed units
					field_set_value(form_flt_info_speedreqdlbl, data.input.tskspdtext);
					field_set_value(form_flt_info_speedreqdlbl2, data.input.tskspdtext);
					if ((data.flight.tskspeed <= 0.0) && (data.input.Vxc <= 0.0)) {
						// no valid task speed or Vxc
						field_set_value(form_flt_info_early, " ");
						field_set_value(form_flt_info_speedtype, " ");
						field_set_value(form_flt_info_timetogo, "N/A");
						ctl_set_visible(form_flt_info_speedreqd, false);
						ctl_set_visible(form_flt_info_speedreqdlbl, false);
						ctl_set_visible(form_flt_info_avgtskspd, false);
						ctl_set_visible(form_flt_info_speedreqdlbl2, false);
						field_set_value(form_flt_info_speedtype, " ");
					} else if (data.task.rulesactive && (data.task.mintasktime > 0)) {
						// min task time set
						ctl_set_visible(form_flt_info_speedreqd, true);
						ctl_set_visible(form_flt_info_speedreqdlbl, true);
						ctl_set_visible(form_flt_info_avgtskspd, true);
						ctl_set_visible(form_flt_info_speedreqdlbl2, true);
						field_set_value(form_flt_info_early, CalcTOTvar(lengthsecs/60, data.task.mintasktime));
						field_set_value(form_flt_info_speedtype, "OTA");
						// speed required to arrive on time
						if (data.task.mintasktime*60 > elapsedsecs) {
							speed = (fltdata->flttask.ttldist - fltdata->tskdist)/(data.task.mintasktime*60 - elapsedsecs)*3600.0;
							if (speed > 150.0) {
								StrCopy(tempchar, "Vne");
							} else if (speed < 0.0) {
								StrCopy(tempchar, "0.0");
							} else {
								StrCopy(tempchar, DblToStr(pround(speed*data.input.tskspdconst, 1), 1));
							}
							field_set_value(form_flt_info_speedreqd, tempchar);
							field_set_value(form_flt_info_avgtskspd, DblToStr(pround(fltdata->tskspeed*data.input.tskspdconst,1),1));
							SecondsToDateOrTimeString(data.task.mintasktime*60 - elapsedsecs, tempchar, 1, 0);
							field_set_value(form_flt_info_timetogo, tempchar);
						} else {
							field_set_value(form_flt_info_speedreqd, "N/A");
							field_set_value(form_flt_info_avgtskspd, DblToStr(pround(fltdata->tskspeed*data.input.tskspdconst,1),1));
							field_set_value(form_flt_info_timetogo, "Expired");
						}
					} else {
						// no time limit
						field_set_value(form_flt_info_early, " ");
						field_set_value(form_flt_info_speedtype, "Est.");
						ctl_set_visible(form_flt_info_speedreqd, true);
						ctl_set_visible(form_flt_info_speedreqdlbl, true);
						ctl_set_visible(form_flt_info_avgtskspd, true);
						ctl_set_visible(form_flt_info_speedreqdlbl2, true);
						SecondsToDateOrTimeString(lengthsecs - elapsedsecs, tempchar, 1, 0);
						field_set_value(form_flt_info_timetogo,tempchar);
						speed = fltdata->flttask.ttldist/lengthsecs*3600.0;
						StrCopy(tempchar, DblToStr(pround(speed*data.input.tskspdconst, 1), 1));
						field_set_value(form_flt_info_speedreqd, tempchar);
						field_set_value(form_flt_info_avgtskspd, DblToStr(pround(fltdata->tskspeed*data.input.tskspdconst,1),1));
					}
				} else {
					// task on hold
					field_set_value(form_flt_info_early, " ");
					field_set_value(form_flt_info_speedtype, " ");
					field_set_value(form_flt_info_timetogo, "On Hold");
					ctl_set_visible(form_flt_info_speedreqd, false);
					ctl_set_visible(form_flt_info_speedreqdlbl, false);
					ctl_set_visible(form_flt_info_avgtskspd, false);
					ctl_set_visible(form_flt_info_speedreqdlbl2, false);
				}
			}

//********************
// zero task values
//********************
		} else { // No task so reset values
//			HostTraceOutputTL(appErrorClass, "Update_Fltinfo - No task so reset values");
			ctl_set_visible(form_flt_info_taskbtn, false);
			ctl_set_visible(form_flt_info_taskbtnlbl, true);
			if (chgstartaltref) {
				ctl_set_visible(form_flt_info_tskstartaltAGL, false);
				ctl_set_visible(form_flt_info_tskstartaltMSL, true);
			} else {
				ctl_set_visible(form_flt_info_tskstartaltMSL, false);
				ctl_set_visible(form_flt_info_tskstartaltAGL, true);
			}
			field_set_value(form_flt_info_tskstartalt, "0");
			field_set_value(form_flt_info_tskstart, "00:00:00");

			field_set_value(form_flt_info_tskelapsed, "00:00:00");
			field_set_value(form_flt_info_tskdist, "0.0");
			if (FltInfoshowMC) {
				field_set_value(form_flt_info_tskspd, DblToStr(pround(data.input.basemc*data.input.lftconst, data.input.lftprec), data.input.lftprec));
			} else {
				field_set_value(form_flt_info_tskspd, "0.0");
			}
			field_set_value(form_flt_info_avgtskspd, " ");
			if (inflight && tasknotfinished && data.task.numwaypts > 0) {
//				HostTraceOutputTL(appErrorClass, "Update_Fltinfo-ttldist3");
				field_set_value(form_flt_info_tskttldist, print_distance2(data.task.ttldist, 1));
				field_set_value(form_flt_info_disttogo, print_distance2(data.task.ttldist, 1));
			} else if ((nrecs <=0) && (data.task.numwaypts > 0)) {
				// active task total distance
				field_set_value(form_flt_info_tskttldist, print_distance2(data.task.ttldist, 1));
				field_set_value(form_flt_info_disttogo, print_distance2(data.task.ttldist, 1));
			} else {
//				HostTraceOutputTL(appErrorClass, "Update_Fltinfo-ttldist4");
				field_set_value(form_flt_info_tskttldist, "0.0");
				field_set_value(form_flt_info_disttogo, "0.0");
			}
			// speed units
			field_set_value(form_flt_info_speedreqdlbl, data.input.tskspdtext);
			field_set_value(form_flt_info_speedreqdlbl2, data.input.tskspdtext);
			// check if task is active with min task time
			if (taskonhold) {
				// task on hold
				field_set_value(form_flt_info_tskarrive, "On Hold");
				field_set_value(form_flt_info_timetogo, "On Hold");
				field_set_value(form_flt_info_tskstop, "On Hold");
			} else if (data.task.rulesactive && (data.task.mintasktime > 0) && (data.task.numwaypts > 0)) {
				// task with time limit
				if ((data.flight.tskspeed > 0.0) || (data.input.Vxc > 0.0)) {
					// use task speed or Vxc
					if (data.activetask.TOTsecs > 0) lengthsecs = data.activetask.TOTsecs; else lengthsecs = 0;
					SecondsToDateOrTimeString(lengthsecs, tempchar, 1, 0);
					field_set_value(form_flt_info_tskarrive,tempchar);
					field_set_value(form_flt_info_early, CalcTOTvar(lengthsecs/60, data.task.mintasktime));
	                                field_set_value(form_flt_info_speedtype, "OTA");
				} else {
					// task speed not valid, cannot calculate TOT
					field_set_value(form_flt_info_tskarrive,"N/A");
					field_set_value(form_flt_info_early, " ");
	                                field_set_value(form_flt_info_speedtype, " ");
				}
				// min task time
				SecondsToDateOrTimeString(data.task.mintasktime*60, tempchar, 1, 0);
				field_set_value(form_flt_info_timetogo, tempchar);
				// required average speed
				field_set_value(form_flt_info_speedreqdlbl, data.input.tskspdtext);
				speed = (data.task.ttldist)/(data.task.mintasktime*60)*3600.0;
				if (speed > 150.0) {
					StrCopy(tempchar, "Vne");
					ctl_set_visible(form_flt_info_speedreqdlbl, false);
				} else if (speed < 0.0) {
					StrCopy(tempchar, "0.0");
				} else {
					StrCopy(tempchar, DblToStr(pround(speed*data.input.tskspdconst, 1), 1));
				}
				field_set_value(form_flt_info_speedreqd, tempchar);
				ctl_set_visible(form_flt_info_speedreqd, true);
				ctl_set_visible(form_flt_info_speedreqdlbl, true);
			} else if (data.task.numwaypts > 0) {
				// task with no time limit
				if ((data.flight.tskspeed > 0.0) || (data.input.Vxc > 0.0)) {
					// use task speed or Vxc
					field_set_value(form_flt_info_speedtype, "Est.");
					if (data.activetask.TOTsecs > 0) lengthsecs = data.activetask.TOTsecs; else lengthsecs = 0;
					SecondsToDateOrTimeString(lengthsecs, tempchar, 1, 0);
					field_set_value(form_flt_info_tskarrive,tempchar);
					field_set_value(form_flt_info_timetogo,tempchar);
					speed = data.task.ttldist/lengthsecs*3600.0;
					StrCopy(tempchar, DblToStr(pround(speed*data.input.tskspdconst, 1), 1));
					field_set_value(form_flt_info_speedreqd, tempchar);
					ctl_set_visible(form_flt_info_speedreqd, true);
					ctl_set_visible(form_flt_info_speedreqdlbl, true);
				} else {
					// task speed not valid, cannot calculate TOT
					field_set_value(form_flt_info_tskarrive,"N/A");
					field_set_value(form_flt_info_early, " ");
	                                field_set_value(form_flt_info_speedtype, " ");
				}
			} else {
				// no task
				field_set_value(form_flt_info_tskarrive, "00:00:00");
				field_set_value(form_flt_info_timetogo, "00:00:00");
				field_set_value(form_flt_info_tskstop, "00:00:00");
			}
		}
	}

//	HostTraceOutputTL(appErrorClass, "Update_Fltinfo-return");
//	HostTraceOutputTL(appErrorClass, "--------------------------------");
	return;
}

void Update_Fltinfo_List(Boolean cleanup)
{
	Char tempchar[10];
	Char tempchar2[30];
	static ListPtr lst;
	FormPtr frm;
	Int16 x;
	static Int16 nrecs = 0;
	MemHandle flt_hand;
	MemPtr flt_ptr;
	static Char **items = NULL;
	DateTimeType strtdt;
	FlightData *fltdata;

	if (!cleanup) {
		frm = FrmGetActiveForm();
		lst = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,form_flt_info_pop2));
		nrecs = OpenDBCountRecords(flight_db);
		if (nrecs > 0) {
			AllocMem((void *)&fltdata, sizeof(FlightData));
//			MemSet(fltdata,sizeof(FlightData),0);
			if (items == NULL) {
//				HostTraceOutputTL(appErrorClass, "Make Flight List");
				items = (Char **) MemPtrNew(nrecs * (sizeof(Char *)));
				for (x = 0; x < nrecs; x++) { 
					items[x] = (Char *) MemPtrNew(20 * (sizeof(Char)));
					OpenDBQueryRecord(flight_db, (nrecs - 1 - x), &flt_hand, &flt_ptr);
					MemMove(fltdata, flt_ptr, sizeof(FlightData));
					MemHandleUnlock(flt_hand);
	
					StringToDateAndTime(fltdata->startdtg, fltdata->startutc, &strtdt);
//					HostTraceOutputTL(appErrorClass, "startdtg:|%s|", fltdata->startdtg);
//					HostTraceOutputTL(appErrorClass, "startutc:|%s|", fltdata->startutc);
//					HostTraceOutputTL(appErrorClass, "strtdt.hour:|%hd|", strtdt.hour);
//					HostTraceOutputTL(appErrorClass, "strtdt.minute:|%hd|", strtdt.minute);
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
			}
			LstSetListChoices(lst, items, nrecs);
			if (nrecs < 5) {
				LstSetHeight(lst, nrecs);
			} else {
				LstSetHeight(lst, 5);
			}
			ctl_set_enable(form_flt_info_pop1, true);
			ctl_set_visible(form_flt_info_graphbtn, true);
			ctl_set_visible(form_flt_info_graphbtn2, true);
			ctl_set_visible(form_flt_info_graphbtn3, true);
			if (!graphing) {
				LstSetTopItem(lst, 0);
				ctl_set_label(form_flt_info_pop1, "Current");
				selectedFltindex = nrecs-1;
			} else {
				LstSetTopItem(lst, ((nrecs-1)-selectedFltindex));
				LstSetSelection(lst, ((nrecs-1)-selectedFltindex));
				ctl_set_label(form_flt_info_pop1, LstGetSelectionText (lst, ((nrecs-1)-selectedFltindex)));
			}
			FreeMem((void *)&fltdata);
		} else {
			ctl_set_enable(form_flt_info_pop1, true);
			ctl_set_label (form_flt_info_pop1, "No Flights");
			ctl_set_enable(form_flt_info_pop1, false);
			ctl_set_visible(form_flt_info_graphbtn, false);
			ctl_set_visible(form_flt_info_graphbtn2, false);
			ctl_set_visible(form_flt_info_graphbtn3, false);
			LstSetHeight(lst, 0);
		}
		graphing = false;
	} else {
		if ((nrecs > 0) && items != NULL) {
			for (x = 0; x < nrecs; x++) { 
				MemPtrFree(items[x]);
			}
			MemPtrFree(items);
			items = NULL;
		}
	}
	return;
}

void Update_Fltinfo_Event()
{
	Int16 nrecs;
	static Boolean oncemore = false;

//	HostTraceOutputTL(appErrorClass, "Update Flight Info");
	// This ensures that there is an active flight going on before
	// updating the flt info screen 
	nrecs = OpenDBCountRecords(flight_db);
	if ((nrecs > 0) && !graphing) {
		if (inflight) {
			selectedFltindex = nrecs-1;
			Update_Fltinfo(selectedFltindex, false, true);
			oncemore = true;
		} else if (oncemore) {
			selectedFltindex = nrecs-1;
			Update_Fltinfo(selectedFltindex, true, true);
			Update_Fltinfo_List(true);
			Update_Fltinfo_List(false);
			oncemore = false;
		} else if (forceupdate) {
			Update_Fltinfo(selectedFltindex, true, true);
//			oncemore = true;
		}
	} else {
		Update_Fltinfo_List(false);
		ctl_set_visible(form_flt_info_ldilabel, true);
		ctl_set_visible(form_flt_info_ldi, true);
		ctl_set_visible(form_flt_info_ldalabel, true);
		ctl_set_visible(form_flt_info_lda, true);
		ctl_set_visible(form_flt_info_ldrlabel, true);
		ctl_set_visible(form_flt_info_ldr, true);
		field_set_value(form_flt_info_start, "00:00:00");
		field_set_value(form_flt_info_stop, "00:00:00");
		field_set_value(form_flt_info_length, "00:00:00");
		field_set_value(form_flt_info_maxalt, "0");
		field_set_value(form_flt_info_pctthermal, "0.0");
		if (terrainvalid) {
			field_set_value(form_flt_info_terelev, print_altitude(data.input.terelev));
		} else {
			field_set_value(form_flt_info_terelev, "N/A");
		}
		field_set_value(form_flt_info_ldi,"XX");
		field_set_value(form_flt_info_lda,"XX");
		field_set_value(form_flt_info_ldr,"XX");
	}
	return;
}

void Update_Tskinfo_Event()
{
	Int16 nrecs;
	static Boolean oncemore = false;

//	HostTraceOutputTL(appErrorClass, "Update Task Info");
	// This ensures that there is an active flight going on before
	// updating the flt info screen 
	nrecs = OpenDBCountRecords(flight_db);
	if (((nrecs > 0) && !graphing) || (data.task.numwaypts > 0)) {
		if (inflight || (data.task.numwaypts > 0)) {
			selectedFltindex = nrecs-1;
			Update_Fltinfo(selectedFltindex, false, false);
			oncemore = true;
		} else if (oncemore) {
			selectedFltindex = nrecs-1;
			Update_Fltinfo(selectedFltindex, true, false);
			oncemore = false;
		} else if (forceupdate) {
			Update_Fltinfo(selectedFltindex, true, false);
			forceupdate = false;
		}
	} else {
		field_set_value(form_flt_info_tskstartalt, "0");
		if (FltInfoshowMC) {
			field_set_value(form_flt_info_tskspd, DblToStr(pround(data.input.basemc*data.input.lftconst, data.input.lftprec), data.input.lftprec));
		} else {
			field_set_value(form_flt_info_tskspd, "0.0");
		}
		field_set_value(form_flt_info_tskstart, "00:00:00");
		field_set_value(form_flt_info_tskstop, "00:00:00");
		field_set_value(form_flt_info_tskdist, "0.0");
		field_set_value(form_flt_info_disttogo, "0.0");
		field_set_value(form_flt_info_tskttldist, "0.0");
		field_set_value(form_flt_info_tskelapsed, "00:00:00");
		field_set_value(form_flt_info_timetogo, "00:00:00");
		field_set_value(form_flt_info_tskarrive, "00:00:00");
		ctl_set_visible(form_flt_info_taskbtn, false);
		ctl_set_visible(form_flt_info_taskbtnlbl, true);
		ctl_set_visible(form_flt_info_tskstartaltAGL, true);
		ctl_set_visible(form_flt_info_tskstartaltMSL, false);
	}
	return;
}

// inputSi should be in knots
Boolean CalcSTFSpdAlt(double inputSi, double distance, double bearing, 
			double elevation, double startalt, Boolean usesafealt, 
			double *stfspd, double *stfalt)
{
	// calculate STF and altitude lost
	if (!CalcSTFSpdAlt2(inputSi, distance, bearing, stfspd, stfalt)) return(false);

	// change altitude reference
	*stfalt = ConvertAltType(elevation, startalt, usesafealt, data.config.alttype, *stfalt);

	return(true);
}

// inputSi should be in knots
Boolean CalcSTFSpdAlt2(double inputSi, double distance, double bearing,
			double *stfspd, double *stfalt)
{
	double a = data.polar.a;
	double b = data.polar.b;
	double c = data.polar.c;
	double Vstf, Wsstf;
	double Si = -inputSi;
	double tempdbl;

	if (a==0.0 || b==0.0 || c==0.0) return(false);;

//	HostTraceOutputTL(appErrorClass, "Si =|%s|", DblToStr(Si, 1));

	// This recalculates the headwind component for the actual direction to the destination waypoint
	if (data.config.usecalchw && (data.input.bearing_to_destination.valid == VALID)) {
		CalcHWind(bearing, data.input.wnddir, data.input.wndspd, &calcstfhwind);
	} else {
		calcstfhwind = data.input.headwind;
	}

	// Now use the new Wm(airmass) value to calculate Vstf instead
	// of just the Overall SinkValue
	tempdbl = 2*a*calcstfhwind;
	if (!data.config.optforspd) {
		Vstf = ((tempdbl-b)-Sqrt(b*b + tempdbl*tempdbl - 2*b*tempdbl + 8*a*(Si+calcstfhwind*b)))/(4*a);
	} else {
		Vstf = calcstfhwind-1/(2*a)*Sqrt(tempdbl*tempdbl - 4*a*(-Si-calcstfhwind*b-c));
	}

	// calculate STF
	if (Vstf < data.polar.Vmin) {
		// no point to fly slower than Vmin
		Vstf = data.polar.Vmin;
	}
	if (Vstf > 150.0) {
		// above Vne for most gliders
		Vstf = 150.0;
	}
	*stfspd = Vstf;
//	HostTraceOutputTL(appErrorClass, "Vstf=|%s|", DblToStr(Vstf, 1));

	// calculate sink at STF
	Wsstf = a*Vstf*Vstf + b*Vstf + c;
//	HostTraceOutputTL(appErrorClass, "Wsstf=|%s|", DblToStr(Wsstf, 1));

	// calculate altitude lost in feet
	*stfalt = distance/(Vstf-calcstfhwind) * Wsstf * (FPNM * -1.0);

	return(true);
}

double ConvertAltType(double elevation, double startalt, Boolean usesafealt, Int8 alttype, double neededalt)
{
	double safealt=0.0;
	double altitude=0.0;

	if (usesafealt) {
		safealt = data.config.safealt;
	} else {
		safealt = 0.0;
	}

	altitude = neededalt + elevation;

	switch (alttype) {
		case REQALT:
			// Calculate Require Altitude
			altitude += safealt;
			break;
		case ARVALT:
			// Calculate Arrival Altitude
			altitude = startalt - altitude;
			break;
		case DLTALT:
			// Calculate Delta Altitude (Arrival - Required)
			altitude = startalt - (altitude + safealt);
			break;
		default:
			break;
	}

	return(altitude);
}

void SetMCCurVal()
{
	double Vstf;
	if (data.config.optforspd) {
		MCCurVal = data.input.basemc;
	} else {
		if (thmode == CRUISE) {
//			HostTraceOutputTL(appErrorClass, "data.input.avglift-|%s|", DblToStr(data.input.avglift, 1));
//			HostTraceOutputTL(appErrorClass, "data.input.basemc-|%s|", DblToStr(data.input.basemc, 1));
			MCCurVal = data.input.basemc - data.input.avglift;
//			HostTraceOutputTL(appErrorClass, "MCCurVal-|%s|", DblToStr(MCCurVal, 1));
		} else {
			MCCurVal = data.input.basemc - avgatthstart;
		}
		if (MCCurVal < 0.0) {
			MCCurVal = 0.0;
		}
	}
	// calculate expected cross country speed.
	data.input.Vxc = CalcVxc(MCCurVal, &Vstf);
//	HostTraceOutputTL(appErrorClass, "MCCurVal-|%s|", DblToStr(MCCurVal, 1));
//	HostTraceOutputTL(appErrorClass, "--------------------");

	return;
}

void incMC(double MCVal)
{
	double remainder, maxval, newmc;
	
//	MCVal = data.input.basemc * data.input.lftconst;
	remainder = pround(Fmod(MCVal, data.input.lftincr), data.input.lftprec);
	if (remainder < data.input.lftincr) {
		MCVal = pround(MCVal - remainder + data.input.lftincr, data.input.lftprec);
	} else {
		MCVal = pround(MCVal - remainder + (2.0 * data.input.lftincr), data.input.lftprec);
	}
	if (data.config.lftunits == STATUTE) {
		maxval = 3000.0;
	} else if (data.config.lftunits == METRIC) {
		maxval = 15.0;
	} else {
		maxval = 30.0;
	}
	if (MCVal <= maxval) {
		// This converts tempdbl to knots for storing in basemc
		data.input.basemc = MCVal/data.input.lftconst;
		newmc = data.input.basemc;
		if (data.config.flightcomp == C302COMP) {
			Output302GRec(GMC);
			skipmc = true;
		} else {
			data.input.compmc = data.input.basemc;
		}
		SetMCCurVal();
	}
	data.application.changed = 1;
	return;
}

void decMC(double MCVal)
{
 	double remainder, newmc;

//	MCVal = data.input.basemc * data.input.lftconst;
	remainder = pround(Fmod(MCVal, data.input.lftincr), data.input.lftprec);
	if (remainder == 0.0) {
		MCVal = pround(MCVal - data.input.lftincr, data.input.lftprec);
	} else if (remainder <= data.input.lftincr) {
		MCVal = pround(MCVal - remainder, data.input.lftprec);
	} else {
		MCVal = pround(MCVal - remainder + data.input.lftincr, data.input.lftprec);
	}
	if (data.input.basemc > 0.0) {
		// This converts tempdbl to knots for storing in basemc
		data.input.basemc = MCVal/data.input.lftconst;
		newmc = data.input.basemc;
		if (data.config.flightcomp == C302COMP) {
			Output302GRec(GMC);
			skipmc = true;
		} else {
			data.input.compmc = data.input.basemc;
		}
		SetMCCurVal();
	}
	data.application.changed = 1;
	return;
}
