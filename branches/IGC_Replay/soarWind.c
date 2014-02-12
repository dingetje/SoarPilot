#include <PalmOS.h>
#include "soaring.h"
#include "soarWind.h"
#include "soarUMap.h"
#include "soarUtil.h"
#include "soarForm.h"
#include "soarMath.h"
#include "soarNMEA.h"
#include "soarWay.h"
#include "soarComp.h"

// Wind profile
WindProfileType *windprofile;
Int16 profilescale_ft[WINDPROFILESCALES] = {3, 6, 9, 12, 18, 24, 30, 36, 48, 60};
Int16 profilescale_m[WINDPROFILESCALES]  = {4, 8, 12, 16, 20, 24, 32, 40, 60, 80};
Int16 *profilescale;
Int16 profilebase = 0;
double profilestep;
Boolean profilegraph = true;
Int16 profilemaxalt = 0;
Int16 profileminalt = WINDPROFILEPOINTS;

// Thermal averaging
UInt8 thmode = CRUISE;
Boolean wasinthermal = false;
ThermalData lastthermal[THERMALSTOAVG];
double thermalavg = 0.0;
Boolean showthermalalt = true;
double avgatthstart=0.0;

// Wind Info screen
Boolean fieldupd = false;
double windinc;

// Externals
extern double curmapscale;
extern double actualmapscale;
extern double savmapscale;
extern Boolean menuopen;
extern UInt32 cursecs;
extern Boolean updatemap;
extern UInt8 mapmode;

/*****************************************************************************
 * CalcHWind - Calculate Head Wind Component
 *****************************************************************************/
void CalcHWind(double curdir, double wnddir, double wndspd, double *hdwnd)
{
	
	if (data.config.useinputwinds) {
		// PG :  special C302 option deleted as only valid for current direction of flight
		//if (data.config.flightcomp == C302COMP) {
			// Use the headwind component value from an attached flight computer
			//	*hdwnd = data.input.compheadwind;
		//} else {
			// Use the wind speed and direction values from an attached flight computer
			*hdwnd = data.input.compwndspd * Cos(DegreesToRadians(data.input.compwnddir - curdir));
		//}
//		HostTraceOutputTL(appErrorClass, "     Headwind:usecompvalues=true|%s|", DblToStr(*hdwnd, 1));
	} else {
		// Calculate headwind component from SoarPilot calculated values
		*hdwnd = wndspd * Cos(DegreesToRadians(wnddir - curdir));
//		HostTraceOutputTL(appErrorClass, "     Headwind:usecompvalues=false|%s|", DblToStr(*hdwnd, 1));
	}
}

/*****************************************************************************
 * glAngleAverage - Average a series of angles
 *                  Adapted from Marc Ramsey's GlideLib
 *****************************************************************************/
// Normalize angle to range 0 <= a < 360
double glAngleNormalize(double a)
{
	return a < 0.0 ? a + 360.0 : (a >= 360.0 ? a - 360.0 : a);
}
// Add/subtract two angles
#define glAngleAdd(a1, a2) glAngleNormalize((a1)+(a2))
#define glAngleSub(a1, a2) glAngleNormalize((a1)-(a2))
// Average a series of angles
double glAngleAverage(double average, double value, int samples)
{
	double diff = glAngleSub(average, value);
	double div = 1.0/samples;
	if (samples == 1)
		return value;
	else if (diff < 180.0)
		return glAngleSub(average, diff*div);
	else
		return glAngleAdd(average, (360.0 - diff)*div);
}

/*****************************************************************************
 * CalcWind - Calculate Wind Speed & Direction using the min/max
 *                     speeds in a thermalling turn.
 *****************************************************************************/
Boolean CalcWind(double curdir, double curspd, double *awnddir, double *awndspd, Boolean usecompvalues, Boolean reset)
{
	static double olddir=0.0, maxspd, minspd;
	double turndiff=0.0; 
	static double totalturndiff=0.0;
	static Int16 turnincr=1, rhincr=1, lhincr=-1, turndir=RIGHT;
	static Boolean begin_thermal=true;
	static Int16 turnidx=MINTURNIDX, thnum=MINTHNUM;
	static double wndspds[MAXTHNUM+1];
	static double wnddirs[MAXTHNUM+1];
	static Boolean thmaxnum_reached = false;
//	static double savmapscale;
	static double wnddir=0.0;
	static double wndspd=0.0;
	static double minwnddir=0.0, maxwnddir=0.0; 
	static double thavgstartalt=0.0; // msr: thermal start altitude
	static UInt32 thavgstarttime=0;	// msr: thermal start time
	Int16 x=0, thmaxnum=0, i;
	double sumwnddir=0.0, sumwndspd=0.0;
//	double sumlift=0.0;
//	Int16, ct=0;
	Char tempchar[15];
	Boolean fwd_active=false;
	Boolean left_thermal=false;
//	DateTimeType curdt;
	UInt32 diffsecs;
	Int16 profileidx = 0;
	Int16 totalturns;

//	HostTraceOutputTL(appErrorClass, "Inside CalcWind");
	if (FrmGetActiveFormID() == form_wind_disp) {
		fwd_active = true;
	}

	if (reset) {
//		HostTraceOutputTL(appErrorClass, "     Resetting Wind Calculations");
		olddir = 0.0;
		totalturndiff = 0.0;
		turndir = RIGHT;
		thmode = CRUISE;
		// This is to save the current mode into the logger
		data.logger.thermal = CRUISE;
		begin_thermal = true;
		left_thermal = false;
		turnidx = MINTURNIDX;
		thnum = MINTHNUM;
		thmaxnum_reached = false;
		maxspd = minspd = curspd;	// msr: fix for using gs - as
		wnddir = 0.0;
		minwnddir = maxwnddir = curdir;
		wndspd = 0.0;
		*awnddir = 0.0;
		*awndspd = 0.0;
		data.input.thavglift = 0.0;

		// msr: reset thermal start altitude and time
//		if (data.config.usepalt) {
//			thavgstartalt = data.logger.pressalt;
//		} else {
			thavgstartalt = data.input.inusealt;
//		}
		thavgstarttime = cursecs;
		if (data.config.calcwind) {
			ctl_set_visible(form_wind_disp_mancalc, false);
			ctl_set_visible(form_wind_disp_calcSP, true);
			field_set_value(form_wind_disp_mode, "CRUISE");
			field_set_value(form_wind_disp_turndir, "RIGHT");
		} else {
			field_set_value(form_wind_disp_mode, "MANUAL");
			field_set_value(form_wind_disp_turndir, "N/A");
			ctl_set_visible(form_wind_disp_calcSP, false);
			ctl_set_visible(form_wind_disp_calccomp, false);
			ctl_set_visible(form_wind_disp_mancalc, true);
		}
		field_set_value(form_wind_disp_winddir, print_direction2(wnddir));
		field_set_value(form_wind_disp_windspd, print_horizontal_wndspeed(wndspd,1));
		field_set_value(form_wind_disp_awnddir, print_direction2(*awnddir));
		field_set_value(form_wind_disp_awndspd, print_horizontal_wndspeed(*awndspd,1));
		field_set_value(form_wind_disp_thavglft, print_vertical_speed2(data.input.thavglift, 1));
		return(true);
	}

//	HostTraceOutputTL(appErrorClass, "     curspd:|%s|", DblToStr(curspd, 1));

	if (data.config.calcwind && usecompvalues) {
		wnddir = data.input.compwnddir;
		wndspd = data.input.compwndspd;
		*awnddir = data.input.compwnddir;
		*awndspd = data.input.compwndspd;
	}

	// This section determines if we are turning and which direction
	turndiff = curdir - olddir;
//	HostTraceOutputTL(appErrorClass, "     turndiff1:|%s|", DblToStr(turndiff, 1));
//	HostTraceOutputTL(appErrorClass, "     curdir:|%s|", DblToStr(curdir, 1));
//	HostTraceOutputTL(appErrorClass, "     olddir:|%s|", DblToStr(olddir, 1));

	if (turndiff != 0.0) {
		if (turndiff < 0.0 ) {
			if (curdir < 45.0 && olddir > 315.0) {
				//Right Turn
				turndiff = 360.0 + turndiff;
				turnincr = rhincr;
				turndir = RIGHT;
			} else {
				//Left Turn
				turndiff = turndiff * -1.0;
				turnincr = lhincr;
				turndir = LEFT;
			}
		} else {
			if (curdir > 315.0 && olddir < 45.0) {
				turndiff = 360.0 - turndiff;
				//Left Turn
				turnincr = lhincr;
				turndir = LEFT;
			} else {
				//Right Turn
				turnincr = rhincr;
				turndir = RIGHT;
			}
		}
	} else {
		turnincr = -1;
	}
	olddir = curdir;
//	HostTraceOutputTL(appErrorClass, "     turndiff2:|%s|", DblToStr(turndiff, 1));

	if (fwd_active && !menuopen) {
//		HostTraceOutputTL(appErrorClass, "     WindDir1:|%s|", DblToStr(wnddir, 1));
		field_set_value(form_wind_disp_winddir, print_direction2(wnddir));
		field_set_value(form_wind_disp_windspd, print_horizontal_wndspeed(wndspd,1));
		if (turndir == RIGHT) {
			StrCopy(tempchar, "RIGHT");
		} else {
			StrCopy(tempchar, "LEFT");
		}
		field_set_value(form_wind_disp_turndir, tempchar);
	}

	// This section maintains the turn increments used to increment/decrement turnidx.
	// Must figure out that we are turning in the same direction for 5 consecutive times
	// before shifting into THERMAL mode.
	// Allows for 5 sample samples to be in the opposite direction (centering) before it shifts
	// out of THERMAL mode and into CRUISE.
	if (turndiff >= 10.0) {
		if (turnidx == MINTURNIDX) {
			if (turndir == RIGHT) {
				rhincr = 1; 
				lhincr = -1;
				turnincr = rhincr;
			} else {
				rhincr = -1;
				lhincr = 1;
				turnincr = lhincr;
			}
		}
		turnidx += turnincr;
	} else {
		if (turnidx > MINTURNIDX) {
			turnidx += -1;
		}
	}
//	HostTraceOutputTL(appErrorClass, "     turnidx1:|%hd|", turnidx);

	if (turnidx > MAXTURNIDX) {
		turnidx = MAXTURNIDX;
	}

	// This really just for protection
	if (turnidx < MINTURNIDX) {
		turnidx = MINTURNIDX;
	}

//	HostTraceOutputTL(appErrorClass, "     turnidx2:|%hd|", turnidx);

	// Shift Into Thermal Mode
	if (turnidx >= BEGINTHIDX) {
		thmode = THERMAL;
		// This is to save the current mode into the logger for later analysis
		data.logger.thermal = THERMAL;
		if (begin_thermal) {
			// save last glide info and reset altitude and distance values
			if (data.input.startcruisealt != data.input.inusealt) {
				data.input.lastglide = (data.input.startcruisedist - data.input.distance_to_destination.value)*DISTFTCONST / (data.input.startcruisealt - data.input.inusealt);
			} else {
				data.input.lastglide = 0.0;
			}
			data.input.startcruisealt = -1;
			data.input.startcruisedist = -1;
//			HostTraceOutputTL(appErrorClass, "Last Glide : %s", DblToStr(data.input.lastglide,1));

			// Need to set min/max speed values equal to current speed
			maxspd = minspd = curspd;	// msr: fix for using gs - as
			maxwnddir = minwnddir = curdir;
			// msr: reset wind average 
			thnum = MINTHNUM;
			thmaxnum_reached = false;
			begin_thermal = false;
			if (data.config.thzoom != THZOOMOFF) {
				// change map settings
				if (mapmode != THERMAL) savmapscale = curmapscale;
				if (data.config.thzoom != THZOOMVAR) {
					// goto fixed thermal scale as set in Map Settings screen
					curmapscale = data.config.thzoomscale;
				} else {
					// goto variable thermal scale based on last thermal mode
					curmapscale = GetMapScale(data.config.thmapscaleidx) / data.input.disconst;
				}
				actualmapscale = curmapscale * data.input.disconst;
				mapmode = THERMAL;
			}
			// set up thermal data
			data.input.thavglift = 0.0;
			// PG : inusealt already decides which altitude reference to use
			// msr: save thermal start altitude and time
			//if (data.config.usepalt) {
			//	thavgstartalt = data.logger.pressalt;
			//} else {
				thavgstartalt = data.input.inusealt;
			//}
			thavgstarttime = cursecs;
			// thermal history
			if (lastthermal[THERMALSTOAVG-1].turns > data.config.thermal_turns) {
				// save thermals
				for (i = 0; i < (THERMALSTOAVG-1); i++) {
					MemMove(&lastthermal[i], &lastthermal[i+1], sizeof(ThermalData));
				}
			}
			lastthermal[THERMALSTOAVG-1].starttime = cursecs;
			lastthermal[THERMALSTOAVG-1].turns = 1;	// already turning to trigger thermal mode
			lastthermal[THERMALSTOAVG-1].startalt = data.input.inusealt;
			lastthermal[THERMALSTOAVG-1].avglift = 0.0;

			// Save the current avglift for use in determining MCCurVal
			// when in Distance Optimization Mode
			avgatthstart = data.input.avglift;
			// This is to reset the averager for shifting into THERMAL mode
			CalcLift(0.0, NULL, -9999.9, RESET);
		}
		// Start checking to see if speeds are slower/faster than stored values.
		// If current is slower, also save current direction as the wind direction.
		// msr: I changed one of these tests to include =, as that yields
		// a more accurate result...
		if (curspd >= maxspd) {
			maxspd = curspd;
			maxwnddir = RecipCse(curdir);
//			HostTraceOutputTL(appErrorClass, "     maxwnddir:|%s|", DblToStr(maxwnddir, 1));
//			HostTraceOutputTL(appErrorClass, "     maxspd1:|%s|", DblToStr(maxspd, 1));
		}
		if (curspd < minspd) {
			minspd = curspd;
			minwnddir = curdir;
//			HostTraceOutputTL(appErrorClass, "     minwnddir:|%s|", DblToStr(minwnddir, 1));
//			HostTraceOutputTL(appErrorClass, "     minspd1:|%s|", DblToStr(minspd, 1));
		}

		// msr: update thermal average lift
		lastthermal[THERMALSTOAVG-1].endtime = cursecs;
		lastthermal[THERMALSTOAVG-1].endalt = data.input.inusealt;
		diffsecs = lastthermal[THERMALSTOAVG-1].endtime - thavgstarttime;
//		HostTraceOutputTL(appErrorClass, "STARTalt %s", DblToStr(thavgstartalt, 1));
//		HostTraceOutputTL(appErrorClass, "pressalt %s", DblToStr(data.logger.pressalt, 1));
//		HostTraceOutputTL(appErrorClass, "diffsecs %s", DblToStr(diffsecs, 0));
		if (diffsecs != 0) {
			// PG : inusealt already decides which altitude reference to use
			//if (data.config.usepalt) {
			//	data.input.thavglift = (data.logger.pressalt - thavgstartalt) / ((double)diffsecs / 60.0) / AIRFPMCONST;
			//} else {
				data.input.thavglift = (data.input.inusealt - thavgstartalt) / ((double)diffsecs / 60.0) / AIRFPMCONST;
				lastthermal[THERMALSTOAVG-1].avglift = data.input.thavglift;
			//}
		}
//		HostTraceOutputTL(appErrorClass, "Tlft %s", DblToStr(data.input.thavglift, 1));
		// find thermal average
		thermalavg = 0.0;
		totalturns = 0;
		for (i = THERMALSTOAVG-1; i >= 0; i--) {
//			HostTraceOutputT(appErrorClass, " %s", DblToStr(lastthermal[i].avglift,2));
			thermalavg += lastthermal[i].avglift * lastthermal[i].turns;
			totalturns += lastthermal[i].turns;
		}
		if (totalturns > 0) thermalavg = thermalavg / totalturns;

		// msr: if wind form active, update thermal avg field 
		if (fwd_active && !menuopen) {
			field_set_value(form_wind_disp_thavglft, print_vertical_speed2(data.input.thavglift, 1));
		}
		// Start adding turndiffs
		totalturndiff += turndiff; //how to allow for small opposite turns?
//		HostTraceOutputTL(appErrorClass, "     totalturndiff:|%s|", DblToStr(totalturndiff, 1));

		// Start checking when sum of turndiffs >= 360.0
		if (totalturndiff >= 360.0) {

			if (!usecompvalues) {
			//	Calculate speed as half the difference between the min and max speeds. 
				wndspd = (maxspd - minspd) / 2.0;
//				HostTraceOutputTL(appErrorClass, "     minspd1:|%s|", DblToStr(minspd, 1));
//				HostTraceOutputTL(appErrorClass, "     maxspd2:|%s|", DblToStr(maxspd, 1));
//				HostTraceOutputTL(appErrorClass, "     WindSpeed:|%s|", DblToStr(wndspd, 1));

				//	Calculate direction using the average of the minspd direction and the 
				// reciprical of the maxspd direction. 
				wnddir = glAngleAverage(maxwnddir, minwnddir, 2);
//				HostTraceOutputTL(appErrorClass, "     minwnddir:|%s|", DblToStr(minwnddir, 1));
//				HostTraceOutputTL(appErrorClass, "     maxwnddir:|%s|", DblToStr(maxwnddir, 1));
//				HostTraceOutputTL(appErrorClass, "     WindDir2:|%s|", DblToStr(wnddir, 1));

			}
			// Save wind direction and speed into arrays.
			// Allows for calculating average wind speed and direction from
			// last 7 turns.
			wnddirs[thnum] = wnddir;
			wndspds[thnum] = wndspd;
			if (thmaxnum_reached) {
				thmaxnum = MAXTHNUM;
			} else {
				thmaxnum = thnum;
			}
			for (x=MINTHNUM; x<=thmaxnum; x++) {
				sumwnddir = glAngleAverage(sumwnddir, wnddirs[x], x + 1);
				sumwndspd += wndspds[x];
			}
			if (data.config.calcwind && !usecompvalues) {
				*awnddir = sumwnddir;
				*awndspd = sumwndspd / (double)(thmaxnum + 1);
			}

			// update wind profile
//			HostTraceOutputTL(appErrorClass, "Updating Wind Profile");
//			HostTraceOutputTL(appErrorClass, "  Altitude  |%s|", print_altitude(data.input.inusealt));
			profileidx = Floor(data.input.inusealt / profilestep);
//			HostTraceOutputTL(appErrorClass, "  index     |%hd|", profileidx);
//			HostTraceOutputTL(appErrorClass, "  AWind Dir:|%s|", print_direction2(sumwnddir));
//			HostTraceOutputTL(appErrorClass, "  AWind Spd:|%s|", print_horizontal_wndspeed(sumwndspd / (double)(thmaxnum + 1), 1));
//			HostTraceOutputTL(appErrorClass, "  Avg Lift: |%s|", print_vertical_speed2(data.input.avglift, 1));
			windprofile[profileidx].direction = sumwnddir;;
			windprofile[profileidx].speed     = sumwndspd / (double)(thmaxnum + 1);;
			windprofile[profileidx].lift      = data.input.avglift;
			windprofile[profileidx].timestamp = cursecs;

			// update max / min alt used in profile
			if (profileidx > profilemaxalt) profilemaxalt = profileidx;
			if (profileidx < profileminalt) profileminalt = profileidx;

			// Increment Thermal Number
			thnum++;
			if (thnum > MAXTHNUM) {
				thnum = MINTHNUM;
				thmaxnum_reached = true;
			}
			if (fwd_active && !menuopen) {
//				HostTraceOutputTL(appErrorClass, "     WindDir3:|%s|", DblToStr(wnddir, 1));
				field_set_value(form_wind_disp_winddir, print_direction2(wnddir));
				field_set_value(form_wind_disp_windspd, print_horizontal_wndspeed(wndspd,1));
//				HostTraceOutputTL(appErrorClass, "     AWindDir2:|%s|", print_direction2(*awnddir));
				field_set_value(form_wind_disp_awnddir, print_direction2(*awnddir));
				field_set_value(form_wind_disp_awndspd, print_horizontal_wndspeed(*awndspd,1));
			}

			totalturndiff -= 360.0;
			maxspd = curspd;
			minspd = curspd;
			minwnddir = curdir;
			maxwnddir = curdir;
			lastthermal[THERMALSTOAVG-1].turns++;
//			HostTraceOutputTL(appErrorClass, "Turns %s", DblToStr(lastthermal[THERMALSTOAVG-1].turns,0));
		}
	} else if (thmode == THERMAL) {
		thmode = CRUISE;
		// log altitude + distance to target when exiting thermal mode
		data.input.startcruisealt = data.input.inusealt;
		data.input.startcruisedist = data.input.distance_to_destination.value;
//		HostTraceOutputTL(appErrorClass, "Start Cruise Alt : %s", DblToStr(data.input.startcruisealt,1));
//		HostTraceOutputTL(appErrorClass, "Start Cruise Dist: %s", DblToStr(data.input.startcruisedist,1));

		// This is to save the current mode into the logger for later analysis
		data.logger.thermal = CRUISE;
		turnidx = MINTURNIDX;
//		HostTraceOutputTL(appErrorClass, "     Setting turnidx to zero");
		begin_thermal = true;
		totalturndiff = 0.0;
		left_thermal = true;
		if (data.config.thzoom != THZOOMOFF) {
			// change map settings
			curmapscale = savmapscale;
			actualmapscale = curmapscale * data.input.disconst;
			mapmode = CRUISE;
		}
		// This is to reset the averager for shifting into CRUISE mode
		CalcLift(avgatthstart, NULL, -9999.9, PREFILL);
	}
	if (fwd_active && !menuopen) {
		if (thmode == THERMAL) {
			StrCopy(tempchar, "THERMAL");
		} else {
			StrCopy(tempchar, "CRUISE");
		}
		field_set_value(form_wind_disp_mode, tempchar);
	}
//	HostTraceOutputTL(appErrorClass, "Leaving CalcWind");
//	HostTraceOutputTL(appErrorClass, "----------------------------------------------------");

	return(left_thermal);
}

void DrawWind(Int16 centerX, Int16 centerY, double xrat, double yrat, double scale)
{
	Int16 innerplotX=centerX, innerplotY=centerY;
	Int16 outerplotX1=centerX, outerplotY1=centerY;
	Int16 outerplotX2=centerX, outerplotY2=centerY;
	Int16 textX=centerX, textY=centerY;
	Char tempchar[8];

	RangeBearingToPixel(xrat, yrat, centerX, centerY, (scale*.1), data.input.wnddir, NOFORCE, data.input.curmaporient, &innerplotX, &innerplotY);
	RangeBearingToPixel(xrat, yrat, centerX, centerY, (scale*.2), data.input.wnddir-5, NOFORCE, data.input.curmaporient, &outerplotX1, &outerplotY1);
	RangeBearingToPixel(xrat, yrat, centerX, centerY, (scale*.2), data.input.wnddir+5, NOFORCE, data.input.curmaporient, &outerplotX2, &outerplotY2);
	WinDrawLine(innerplotX, innerplotY, outerplotX1, outerplotY1);
	WinDrawLine(innerplotX, innerplotY, outerplotX2, outerplotY2);
	textX = (Int16)Fabs((outerplotX1 - outerplotX2)/2) + (outerplotX1 <= outerplotX2 ? outerplotX1: outerplotX2);
	textY = (Int16)Fabs((outerplotY1 - outerplotY2)/2) + (outerplotY1 <= outerplotY2 ? outerplotY1: outerplotY2);
	StrCopy(tempchar, " ");
	StrCat(tempchar, print_horizontal_wndspeed(data.input.wndspd,1));
	FntSetFont(stdFont);
	WinDrawInvertedChars(tempchar, StrLen(tempchar), textX-(SCREEN.SRES*6), textY-(SCREEN.SRES*6));
	return;
}

void Draw3DWind(double bearing, double speed, Int16 centerX, Int16 centerY, Int8 maporient)
{
	Int16 plotX=centerX, plotY=centerY;
	Int16 oppplotX=centerX, oppplotY=centerY;
	Int16 arrowlendX=centerX, arrowlendY=centerY;
	Int16 arrowrendX=centerX, arrowrendY=centerY;
	double xrat, yrat;
	double sf = 1.3;

	// set scale factor for speed
	if (speed < 1.0) speed = 1.0;
	sf *= 20.0 / pround(speed, 0);
	if (speed > 20.0) sf = 1.3;

	xrat = ARROWCENTXOFF / (ARROWCENTXOFF / ARROWAREAWIDTH);
	yrat = ARROWCENTYOFF / (ARROWCENTYOFF / ARROWAREAWIDTH);

	// draw arrow - length depends on wind strength
	RangeBearingToPixel(xrat/sf, yrat/sf, centerX, centerY, 0.45, bearing, NOFORCE, maporient, &plotX, &plotY);
	RangeBearingToPixel(xrat/sf, xrat/sf, centerX, centerY, 0.45, RecipCse(bearing), NOFORCE, maporient, &oppplotX, &oppplotY);
	RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(bearing - 150), NOFORCE, maporient, &arrowlendX, &arrowlendY);
	RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(bearing + 150), NOFORCE, maporient, &arrowrendX, &arrowrendY);
	WinDrawLine(plotX, plotY, oppplotX, oppplotY);
	WinDrawLine(plotX, plotY, arrowlendX, arrowlendY);
	WinDrawLine(plotX, plotY, arrowrendX, arrowrendY);

	// draw double arrow head if wind over 20 kts
	if (speed > 20.0) {
		sf *= 1.5;
		RangeBearingToPixel(xrat/sf, yrat/sf, centerX, centerY, 0.45, bearing, NOFORCE, TRACKUP, &plotX, &plotY);
		RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(bearing - 150), NOFORCE, TRACKUP, &arrowlendX, &arrowlendY);
		RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(bearing + 150), NOFORCE, TRACKUP, &arrowrendX, &arrowrendY);
		WinDrawLine(plotX, plotY, arrowlendX, arrowlendY);
		WinDrawLine(plotX, plotY, arrowrendX, arrowrendY);
	}
}

void CalcWindProfile(Boolean reset, Boolean clear)
{
	Int16 i,j;
	Int16 ct;
	Int16 start;
	RectangleType rectP;
	Char windstr[10];
	double spdscale, sumwnddir, sumwndspd;
//	double latdiff, londiff;
	Int16 plotX, plotY, prevplotX, prevplotY;
	double windalt[7];
	double winddir[7];
	double windspd[7];
	Int16 samples[7];
	Char recent[10][7];
	UInt32 latest;
	Char tempchar[12];
	UInt16 fldID;

	if (reset) {
		// set all values to invalid
		if (clear) {
			for (i=0; i<WINDPROFILEPOINTS; i++) {
				windprofile[i].direction = -999.9;
				windprofile[i].speed = -999.9;
				windprofile[i].lift = 0.0;
				windprofile[i].timestamp = 0;
			}
		}
		// set up height scales
		if (data.config.altunits == NAUTICAL) {
			// feet
			profilescale = profilescale_ft;
			profilestep = 250.0/3.0; // 83.33ft
		} else {
			// meters 
			profilescale = profilescale_m;
			profilestep = 25.0/ALTMETCONST; // 25m
		}
		return;
	}

	// check profilebase is in range
//	HostTraceOutputTL(appErrorClass, "profilebase %hd", profilebase);
	if (((7+profilebase) * profilescale[data.config.profilescaleidx]) >= WINDPROFILEPOINTS) {
//		HostTraceOutputTL(appErrorClass, "   out of range %hd", ((7+profilebase) * profilescale[data.config.profilescaleidx]));
		profilebase = (WINDPROFILEPOINTS / profilescale[data.config.profilescaleidx]) - 8;
//		HostTraceOutputTL(appErrorClass, "   profilebase fixed %hd", profilebase);
	} else if (profilebase < 0) profilebase = 0;

	// calculate average wind for each band
	for (i=0; i<7; i++) {
		// altitude at middle of band
		windalt[i] = (i+1+profilebase) * profilescale[data.config.profilescaleidx] * profilestep;
//		HostTraceOutputTL(appErrorClass, "Band Altitude %s", print_altitude(windalt[i]));

		// initialise position in windprofile
		start = (i+1+profilebase)*profilescale[data.config.profilescaleidx] - profilescale[data.config.profilescaleidx]/2;
//		HostTraceOutputTL(appErrorClass, "   Start Idx %hd", start);
		sumwnddir = 0.0;
		sumwndspd = 0.0;
		ct = 0;
		latest = 0;
//		latdiff = 0.0;
//		londiff = 0.0;

		// calculate average wind and direction for band
		for (j=0; j<profilescale[data.config.profilescaleidx]; j++) {
			// average valid wind values
			if (windprofile[start+j].direction >= 0.0) {
				// average method
				sumwnddir = glAngleAverage(sumwnddir, windprofile[start+j].direction, ct + 1);
				sumwndspd += windprofile[start+j].speed;
				ct++;
				if (windprofile[start+j].timestamp > latest) latest = windprofile[start+j].timestamp;
				// vector method
//				latdiff += Cos(DegreesToRadians(windprofile[start+j].direction)) * windprofile[start+j].speed;
//				londiff += Sin(DegreesToRadians(windprofile[start+j].direction)) * windprofile[start+j].speed;// * data.input.coslat;
			}
		}
//		HostTraceOutputTL(appErrorClass, "   End   Idx %hd", start+j-1);
//		HostTraceOutputTL(appErrorClass, "Lat %s", DblToStr(latdiff,2));
//		HostTraceOutputTL(appErrorClass, "Lon %s", DblToStr(londiff,2));

		// calculate average wind
		if (ct > 0) {
			// no. of samples and latest sample
			samples[i] = ct;
			if (samples[i] > 999) samples[i] = 999;
			SecondsToDateOrTimeString(latest, tempchar, 1, 0);
			StrCopy(recent[i], Left(tempchar, 5));

			// average method
			winddir[i] = sumwnddir;
			windspd[i] = sumwndspd / (double)ct;
//			HostTraceOutputTL(appErrorClass, "Avg Dir %s", DblToStr(winddir[i],1));
//			HostTraceOutputTL(appErrorClass, "Avg Spd %s", DblToStr(windspd[i],1));

/*			// vector method (slow)
			windspd[i] = Sqrt(latdiff*latdiff + londiff*londiff) / (double)ct;
			if (londiff == 0.0) {
				if (latdiff < 0.0) {
					winddir[i] = PI;
				} else { 
					winddir[i] = 0.0;
				}
			} else if (latdiff == 0.0) {
					winddir[i] = (PI / 2.0);
				} else {
					winddir[i] = (PI / 2.0) - Atan(latdiff / londiff);
				}
			if (londiff < 0.0) {
				winddir[i] += PI;
			}
			winddir[i] = nice_brg(RadiansToDegrees(winddir[i]));
//			HostTraceOutputTL(appErrorClass, "Vec Dir %s", DblToStr(winddir[i],1));
//			HostTraceOutputTL(appErrorClass, "Vec Spd %s", DblToStr(windspd[i],1));
//			HostTraceOutputTL(appErrorClass, "-----------------------------");
*/
		} else {
			// no valid values in band
			winddir[i] = -999.9;
			windspd[i] = -999.9;
			samples[i] = 0;
			recent[i][0] = 0; 
		}
	}

	// fill altitide / direction / speed table
	for (i=0; i<7; i++) {
		fldID = form_wind_3dinfo_wind1 + i*10;
		field_set_value(fldID+1, print_altitude(windalt[6-i]));
		if (winddir[6-i] < 0.0) {
			field_set_value(fldID+2, "");
			field_set_value(fldID, "");
		} else {
			StrCopy(windstr, print_direction2(winddir[6-i]));
			StrCat(windstr, "\260");
			field_set_value(fldID+2, windstr);
			field_set_value(fldID, print_horizontal_wndspeed(windspd[6-i], 0));
		}
	}

	// clear profile area
	rectP.topLeft.x = 107;
	rectP.topLeft.y = 12;
	rectP.extent.x = 159;
	rectP.extent.y = 159;
	WinEraseRectangle(&rectP, 0);

	if (profilegraph) {
		// hide time of lastest sample and number of samples
		for (i=0; i<7; i++) {
			fldID = form_wind_3dinfo_wind1 + i*10;
			ctl_set_visible(fldID+3, false);
			ctl_set_visible(fldID+4, false);
		}

		// draw axis
		FntSetFont((FontID)WAYSYMB11);
		windstr[0] = 36;
		WinDrawChars(windstr, 1, 126, 12);
		WinDrawChars(windstr, 1, 126, 153);
		switch (data.config.wndunits) {
			// speed scale factor = 30kts * speed const / full scale value
			case STATUTE:
				windstr[0] = 40;
				spdscale = 30*SPDMPHCONST/30.0;
				break;
			case NAUTICAL:
				windstr[0] = 40;
				spdscale = 30*SPDNAUCONST/30.0;;
				break;
			case METRIC:
				windstr[0] = 41;
				spdscale = 30*SPDKPHCONST/60.0;
				break;
			case METRICMPS:
				windstr[0] = 39;
				spdscale = 30*SPDMPSCONST/15.0;
				break;
			default:
				windstr[0] = 40;
				spdscale = 1.0;
				break;
		}
		WinDrawChars(windstr, 1, 150, 12);
		WinDrawChars(windstr, 1, 150, 153);
		FntSetFont(stdFont);
		WinDrawGrayLine(129, 19, 129, 152);
		WinDrawGrayLine(159, 19, 159, 152);

		// draw wind arrows
		for (i=0; i<7; i++) {
			if (winddir[6-i] >= 0.0) Draw3DWind(RecipCse(winddir[6-i]), windspd[6-i]*spdscale, 117, 24+i*21, data.config.windprofileorient);
		}

		// draw wind profile, arrows to show off scale values
		prevplotX = (Int16)pround(129.0+windspd[6]*spdscale, 0);
		prevplotY = 24;
		if (windspd[6] > 30.0/spdscale) Draw3DWind(90, 0, 158, 24, NORTHUP);
		for (i=1; i<7; i++) {
			plotX = (Int16)pround(129.0+windspd[6-i]*spdscale, 0);
			plotY = 24+i*21;
			if ((winddir[7-i] >= 0.0) && (winddir[6-i] >= 0.0)) {
				WinDrawLine(plotX, plotY, prevplotX, prevplotY);
				if (windspd[6-i] > 30.0/spdscale) Draw3DWind(90, 0, 158, 24+i*21, NORTHUP);	
			}
			prevplotX = plotX;
			prevplotY = plotY;
		}

	} else {
		// show time of lastest sample and number of samples
		for (i=0; i<7; i++) {
			fldID = form_wind_3dinfo_wind1 + i*10;

			ctl_set_visible(fldID+3, true);
			ctl_set_visible(fldID+4, true);

			if (samples[6-i] > 0) {
				field_set_value(fldID+4, DblToStr(samples[6-i],0));
				field_set_value(fldID+3, recent[6-i]);
			} else {
				field_set_value(fldID+4, "");
				field_set_value(fldID+3, "");
			}
		}
	}

	return;
}

Boolean form_thermal_history_event_handler(EventPtr event) 
{ 
	Boolean handled=false; 
	Int8 i;
	UInt16 fldID;
	Char tempchar[15];
	static Boolean validpendown = false;
	static RectangleType rectP;
	double Vstf;

	switch (event->eType) { 
		case frmOpenEvent: 
		case frmUpdateEvent: 
			menuopen = false; 
			FrmDrawForm(FrmGetActiveForm()); 
			// draw units
			for (i=0; i<THERMALSTOAVG; i++) {
				WinDrawChars(data.input.alttext, 2, 104, 26+18*i);
				WinDrawChars(data.input.lfttext, 3, 144, 26+18*i);
			}
			WinDrawChars(data.input.lfttext, 3, 144, 26+18*5);
			WinDrawChars(data.input.lfttext, 3, 144, 24+18*6);
			WinDrawChars(data.input.tskspdtext, 3, 56, 26+18*5);
			WinDrawChars(data.input.tskspdtext, 3, 56, 24+18*6);
			WinDrawChars(data.input.tskspdtext, 3, 56, 22+18*7);
			// touch area on screen
			rectP.topLeft.x = 1;
			rectP.topLeft.y = 12;
			rectP.extent.x = 100;
			rectP.extent.y = 100;
			// change visible screen fields
			if (!showthermalalt) {
				ctl_set_visible(form_thermal_history_alt_lbl, showthermalalt);
				ctl_set_visible(form_thermal_history_base_lbl, showthermalalt);
			}
			ctl_set_visible(form_thermal_history_start_lbl, !showthermalalt);
			ctl_set_visible(form_thermal_history_mins_lbl, !showthermalalt);
			ctl_set_visible(form_thermal_history_turns_lbl, !showthermalalt);
			if (showthermalalt) {
				ctl_set_visible(form_thermal_history_alt_lbl, showthermalalt);
				 ctl_set_visible(form_thermal_history_base_lbl, showthermalalt);
			}
			for (i=0; i<THERMALSTOAVG; i++) {
				fldID = form_thermal_history_start1 + 10*i;
				if (!showthermalalt) ctl_set_visible(fldID+5, showthermalalt);
				ctl_set_visible(fldID+1, !showthermalalt);
				ctl_set_visible(fldID+2, !showthermalalt);
				if (showthermalalt) ctl_set_visible(fldID+5, showthermalalt);
			}
			handled=true;
			break;
		case nilEvent: 
			if (!menuopen) {
				// update thermal history fields
				for (i=0; i<THERMALSTOAVG; i++) {
					fldID = form_thermal_history_start1 + 10*i;	// requires each line incremented by 10 in soarForm.h defines
					if (lastthermal[THERMALSTOAVG-1-i].turns > 0) {
						if (showthermalalt) {
							// start altitude
							field_set_value(fldID+0, print_altitude(lastthermal[THERMALSTOAVG-1-i].startalt));
							// end altitude
							field_set_value(fldID+5, print_altitude(lastthermal[THERMALSTOAVG-1-i].endalt));
						} else {
							// start time
							SecondsToDateOrTimeString(lastthermal[THERMALSTOAVG-1-i].starttime, tempchar, 1, 0);
							field_set_value(fldID+0, Left(tempchar,5));
							// length
							field_set_value(fldID+1, DblToStr((lastthermal[THERMALSTOAVG-1-i].endtime - lastthermal[THERMALSTOAVG-1-i].starttime)/60,0));
							// number of turns
							field_set_value(fldID+2, DblToStr(lastthermal[THERMALSTOAVG-1-i].turns,0));
						}
						// altitude gain
						field_set_value(fldID+4, print_altitude(lastthermal[THERMALSTOAVG-1-i].endalt - lastthermal[THERMALSTOAVG-1-i].startalt));
						// average lift
						field_set_value(fldID+3, DblToStr(pround(lastthermal[THERMALSTOAVG-1-i].avglift*data.input.lftconst, data.input.lftprec), data.input.lftprec));
					} else {
						// no data recorded
						field_set_value(fldID+0, " ");
						field_set_value(fldID+1, " ");
						field_set_value(fldID+2, " ");
						field_set_value(fldID+4, " ");
						field_set_value(fldID+3, " ");
					}
				}
				// update speeds
				field_set_value(form_thermal_history_Vavglift, DblToStr(pround(CalcVxc(thermalavg, &Vstf)*data.input.tskspdconst,0),0));
				field_set_value(form_thermal_history_avglift, print_vertical_speed2(thermalavg, data.input.lftprec));
				field_set_value(form_thermal_history_VMCval, DblToStr(pround(data.input.Vxc*data.input.tskspdconst,0),0));
				field_set_value(form_thermal_history_MCval, print_vertical_speed2(data.input.basemc, data.input.lftprec));
				if (data.flight.tskspeed > 0.0) {
					field_set_value(form_thermal_history_Vtask, DblToStr(pround(data.flight.tskspeed*data.input.tskspdconst,0),0));
				} else {
					field_set_value(form_thermal_history_Vtask, "XX");
				}
				// update wind
				StrCopy(tempchar, print_direction2(data.input.wnddir));
				StrCat(tempchar, " @ ");
				StrCat(tempchar, print_horizontal_wndspeed(data.input.wndspd,0));
				StrCat(tempchar, " ");
				StrCat(tempchar, data.input.wndtext);
				field_set_value(form_thermal_history_wind, tempchar);
			}	
			handled=false; 
			break;
		case frmCloseEvent: 
			handled=false; 
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
		case penDownEvent:
			validpendown = false;
			if (!menuopen && RctPtInRectangle (event->screenX, event->screenY, &rectP)) {
				//WinInvertRectangle (&rectP, 0);
				validpendown = true;
				handled=true;
			}
			break;
		case penUpEvent:
			if (validpendown && RctPtInRectangle (event->screenX, event->screenY, &rectP) && (validpendown)) {
				PlayKeySound();
				// switch from profile graphics to numbers
				showthermalalt = !showthermalalt;
				// change visible screen fields
				if (!showthermalalt) {
					ctl_set_visible(form_thermal_history_alt_lbl, showthermalalt);
					ctl_set_visible(form_thermal_history_base_lbl, showthermalalt);
				}
				ctl_set_visible(form_thermal_history_start_lbl, !showthermalalt);
				ctl_set_visible(form_thermal_history_mins_lbl, !showthermalalt);
				ctl_set_visible(form_thermal_history_turns_lbl, !showthermalalt);
				if (showthermalalt) {
					ctl_set_visible(form_thermal_history_alt_lbl, showthermalalt);
					ctl_set_visible(form_thermal_history_base_lbl, showthermalalt);
				}
				for (i=0; i<THERMALSTOAVG; i++) {
					fldID = form_thermal_history_start1 + 10*i;
					if (!showthermalalt) ctl_set_visible(fldID+5, showthermalalt);
					ctl_set_visible(fldID+1, !showthermalalt);
					ctl_set_visible(fldID+2, !showthermalalt);
					if (showthermalalt) ctl_set_visible(fldID+5, showthermalalt);
				}
				handled = true;					
			}
			validpendown = false;
			break;
		case fldEnterEvent:
			switch (event->data.ctlEnter.controlID) {
				case form_thermal_history_Vavglift:
				case form_thermal_history_VMCval:
				case form_thermal_history_Vtask:
					PlayKeySound();
					WinEraseChars(data.input.tskspdtext, 3, 56, 26+18*5);
					WinEraseChars(data.input.tskspdtext, 3, 56, 24+18*6);
					WinEraseChars(data.input.tskspdtext, 3, 56, 22+18*7);
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
					WinDrawChars(data.input.tskspdtext, 3, 56, 26+18*5);
					WinDrawChars(data.input.tskspdtext, 3, 56, 24+18*6);
					WinDrawChars(data.input.tskspdtext, 3, 56, 22+18*7);
					updatemap = true;
					break;
				case form_thermal_history_wind:
					//PlayKeySound();
					//FrmGotoForm(form_wind_disp);
					break;
				case form_thermal_history_MCval:
					//if (data.config.MCbutton == POPUP)FrmPopupForm(form_set_mc);
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
