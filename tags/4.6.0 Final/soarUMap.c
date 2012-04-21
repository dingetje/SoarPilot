#include <PalmOS.h>
#include "soarMap.h"
#include "soarUMap.h"
#include "Mathlib.h"
#include "soaring.h"
#include "soarUtil.h"
#include "soarMem.h"
#include "soarMath.h"
#include "soarForm.h"
#include "soarWay.h"
#include "soarDB.h"
#include "soarIO.h"
#include "soarWind.h"
#include "soarGPSInfo.h"
#include "soarTask.h"

/*****************************************************************************
 * globals
 *****************************************************************************/

TrkTrail *trail = NULL;
Int16 trailidx = -1;
Int16 idxmax = -1;
double mapscales[MAXMAPSCALEIDX+1] = { 0.2, 0.5, 1.0, 2.0, 3.0, 5.0, 8.0, 10.0, 12.0, 15.0, 20.0, 30.0, 50.0, 80.0, 100.0, 120.0, 150.0, 200.0, 250.0, 300.0};
UInt32 trkplotds[MAXMAPSCALEIDX+1] = { 1,   1,   1,   1,   2,   2,   2,   2,    2,    5,    5,    10,   10,   10,   20,    25,    50,    100,   200,   200};

// UTM Coords
static Ellipsoid ellipsoid[] =
{
	{-1, "Placeholder", 0, 0},
	{1, "Airy", 6377563, 0.00667054},
	{2, "Australian National", 6378160, 0.006694542},
	{3, "Bessel 1841", 6377397, 0.006674372},
	{4, "Bessel 1841 (Nambia) ", 6377484, 0.006674372},
	{5, "Clarke 1866", 6378206, 0.006768658},
	{6, "Clarke 1880", 6378249, 0.006803511},
	{7, "Everest", 6377276, 0.006637847},
	{8, "Fischer 1960 (Mercury) ", 6378166, 0.006693422},
	{9, "Fischer 1968", 6378150, 0.006693422},
	{10, "GRS 1967", 6378160, 0.006694605},
	{11, "GRS 1980", 6378137, 0.00669438},
	{12, "Helmert 1906", 6378200, 0.006693422},
	{13, "Hough", 6378270, 0.00672267},
	{14, "International", 6378388, 0.00672267},
	{15, "Krassovsky", 6378245, 0.006693422},
	{16, "Modified Airy", 6377340, 0.00667054},
	{17, "Modified Everest", 6377304, 0.006637847},
	{18, "Modified Fischer 1960", 6378155, 0.006693422},
	{19, "South American 1969", 6378160, 0.006694542},
	{20, "WGS 60", 6378165, 0.006693422},
	{21, "WGS 66", 6378145, 0.006694542},
	{22, "WGS-72", 6378135, 0.006694318},
	{23, "WGS-84", 6378137, 0.00669438},
	{24, "FAI",    6371000, 0.000000002} // Not sure this is right!
};

extern double bigratio;
extern IndexedColorType indexGreen, indexRed, indexBlack, indexGrey, indexOrange;
extern IndexedColorType indexSink, indexWeak, indexStrong, indexWaypt;
extern double  curmapscale;
extern double xdist, ydistul, ydistll, ulRng; // screen bounds in nm
extern double crashlat, crashlon;
extern Boolean offterrain;
extern UInt8 mapmode;
extern double mapmaxlat, mapminlat;
extern double mapmaxlon, mapminlon;
extern double truecse;
extern Boolean draw_task;
extern Boolean IsMap;
extern Int16 profilemaxalt;
extern Int16 profileminalt;
extern WindProfileType *windprofile;
extern double profilestep;
extern double MCCurVal;
extern double MCMult;
extern UInt32 cursecs;
extern Int16 inareasector;

/******************************************************************************
 * Conversions
 *****************************************************************************/
double MetersToNautical(double	meters)
{
	return (meters / 1852.0);
}

double NauticalToMeters(double	miles)
{
	return (miles * 1852.0);
}

double MetersToFeet(double	meters)
{
	return (meters * 3.2808333);
}

double FeetToMeters(double feet)
{
	return (feet / 3.2808333);
}

double RelToTrueBrg(double relbrg, double truecse)
{
	double truebrg;
	truebrg = relbrg + truecse;
	if (truebrg >= 360.0) {
		truebrg -= 360.0;
	}
	return(truebrg);
}

double TrueToRelBrg(double oldbearing, double truecse)
{
	double newbearing;

	/* Shift Bearing of POI by True Course Amount */
	newbearing = oldbearing - truecse;
	if (newbearing < 0.0) {
		newbearing += 360.0; 
	}
	return(newbearing);
}

double RecipCse(double curcse)
{
	double recipcse;

	recipcse = curcse - 180.0;
	if (recipcse < 0.0) {
		recipcse += 360.0;
	}
	return(recipcse);
}

double modcrs(double x)
{
	return(Fmod(x, 2.0*PI));
}
// Given two bearing, calculate the bisector angle
double MidCse(double leftbrg, double rightbrg) 
{
	double diff, angle;
	Boolean opposite = false;

	if (leftbrg > rightbrg) {
		diff = leftbrg - rightbrg;
	} else {
		diff = rightbrg - leftbrg;
	}

	if (diff < 180.0) {
		diff = 360.0 - diff;
		opposite = true;
	}

	diff = diff/2.0;
	if (leftbrg > rightbrg) {
		if (opposite) {
			angle = nice_brg(rightbrg - diff);
		} else {
			angle = nice_brg(diff + rightbrg);
		}
	} else {
		if (opposite) {
			angle = nice_brg(leftbrg - diff);
		} else {
			angle = nice_brg(diff + leftbrg);
		}
	}
	return(RecipCse(angle));
}

Boolean PointOutOfBounds (double ul_lat, double ul_lon, double lr_lat, 
				double lr_lon, double lat, double lon)
{
//	if (convP->crosses_pole != (char)0) {
//		/*Pole crossing supported - do a concentric check*/
//		double cos_c  = Sin(convP->latOrigin)* Sin(lat)+
//				Cos(convP->latOrigin)* Cos(lat) * Cos(lon - convP->lonOrigin);
//		double bounds = Cos(convP->ul_lat - convP->latOrigin)*
//				Cos(convP->lr_lon - convP->lonOrigin);
//
//		if (cos_c < bounds)
//			return true;
//		else
//			return false;
//	}
//	else if (lon > convP->lr_lon || lon < convP->ul_lon || lat < convP->lr_lat ||
//			lat > convP->ul_lat)
//
	if (lon > lr_lon || lon < ul_lon || lat < lr_lat || lat > ul_lat)
		return(true);
	else
		return(false);
}

/*****************************************************************************
 * LLToStringDM - 	for LON, print "DDD:MM.MM(MM) X" where 'X' is E or W.
 *			for LAT, print "DD:MM.MM(MM) X" where 'X' is N or S.
 *			Now supports 2-4 decimal places with 3 or 4 being the available
 *			options for numaddplaces and whether the colon & period are added.
 *****************************************************************************/
Char LLToStringDM(double coord, Char *coordStr, Boolean lat, Boolean colon, Boolean period, Int8 numaddplaces)
{
	double	absCoord;
	double	fraction;
	Int16	degrees;
	Int16	minutes;
	Int16	decMinutes;		// decimal minutes
	Char	nsew;
	double	precmult=1.0;

	if (!coordStr)
		return(' ');


	if (numaddplaces == 3) {
		precmult = 10.0;
	} else if (numaddplaces == 4) {
		precmult = 100.0;
	}
	// break the double down to parts
	absCoord = Fabs(coord);
	degrees = (Int16)absCoord;
	fraction = absCoord - (double)degrees;
	minutes = (Int16)(fraction * 60.0);
	fraction = (fraction * 60.0 - (double)minutes);
	decMinutes = (Int16)(pround((fraction * (Int16)(100.0*precmult)), 0));

	// catch decimal minute overflow
	if (decMinutes >= (Int16)(100.0*precmult)) {
		decMinutes -= (Int16)(100.0*precmult);
		minutes++;
	}

	// catch minute overflow
	if (minutes > 59) {
		minutes -= 60;
		degrees++;
	}

	// put the digits in the string, padding w/ zeros if necessary
	if (lat) {
		if (degrees < 10)
			StrPrintF(coordStr, "0%d", degrees);
		else
			StrPrintF(coordStr, "%d", degrees);
		if (colon) {
			StrCat(coordStr, ":");
			coordStr += 3;
		} else {
			coordStr += 2;
		}
	} else {
		if (degrees < 10)
			StrPrintF(coordStr, "00%d:", degrees);
		else if (degrees < 100)
			StrPrintF(coordStr, "0%d:", degrees);
		else
			StrPrintF(coordStr, "%d:", degrees);
		if (colon) {
			StrCat(coordStr, ":");
			coordStr += 4;
		} else {
			coordStr += 3;
		}
	}

	if (minutes < 10)
		StrPrintF(coordStr, "0%d", minutes);
	else
		StrPrintF(coordStr, "%d", minutes);
	if (period) {
		StrCat(coordStr, ".");
		coordStr += 3;
	} else {
		coordStr += 2;
	}

	if (lat)
		nsew = (coord >= 0.0) ? 'N' : 'S';
	else
		nsew = (coord >= 0.0 && coord <= 180.0) ? 'E' : 'W';

	if (numaddplaces == 3) {
		if (decMinutes < 10) {
			StrPrintF(coordStr, "00%d%c", decMinutes, nsew);
		} else if (decMinutes < 100) {
			StrPrintF(coordStr, "0%d%c", decMinutes, nsew);
		} else {
			StrPrintF(coordStr, "%d%c", decMinutes, nsew);
		}
	} else if (numaddplaces == 4) {
		if (decMinutes < 10) {
			StrPrintF(coordStr, "000%d%c", decMinutes, nsew);
		} else if (decMinutes < 100) {
			StrPrintF(coordStr, "00%d%c", decMinutes, nsew);
		} else if (decMinutes < 1000) {
			StrPrintF(coordStr, "0%d%c", decMinutes, nsew);
		} else {
			StrPrintF(coordStr, "%d%c", decMinutes, nsew);
		}
	} else {
		if (decMinutes < 10) {
			StrPrintF(coordStr, "0%d%c", decMinutes, nsew);
		} else {
			StrPrintF(coordStr, "%d%c", decMinutes, nsew);
		}
	}

	return(nsew);
}

/*****************************************************************************
 * LLToStringDMS -	For LON, print "DDD:MM:SS.SS X" where 'X' is E or W.
 *			for LAT, print "DD:MM:SS.SS X" where 'X' is N or S.
 *****************************************************************************/
Char LLToStringDMS(double coord, Char *coordStr, Boolean lat)
{
	double	absCoord;
	double	fraction;
	Int16		degrees;
	Int16		minutes;
	Int16		seconds;
	Int16		decSeconds;		// decimal seconds
	Char		nsew;

	if (!coordStr)
		return(' ');

	// break the double down to parts
	absCoord = Fabs(coord);
	degrees = (Int16)absCoord;
	fraction = absCoord - (double)degrees;
	minutes = (Int16)(fraction * 60.0);
	fraction = (fraction * 60.0 - (double)minutes);
	seconds = (Int16)(fraction * 60.0);
	fraction = (fraction * 60.0 - (double)seconds);
	decSeconds = (Int16)((fraction * 100.0) + .5);

	// catch decimal second overflow
	if (decSeconds > 99) {
		decSeconds -= 100;
		seconds++;
	}

	// catch second overflow
	if (seconds > 59) {
		seconds -= 60;
		minutes++;
	}

	// catch minute overflow
	if (minutes > 59) {
		minutes -= 60;
		degrees++;
	}

	// put the digits in the string, padding w/ zeros if necessary
	if (lat) {
		if (degrees < 10)
			StrPrintF(coordStr, "0%d:", degrees);
		else
			StrPrintF(coordStr, "%d:", degrees);
		coordStr += 3;
	} else {
		if (degrees < 10) {
			StrPrintF(coordStr, "00%d:", degrees);
		 } else if (degrees < 100) {
			StrPrintF(coordStr, "0%d:", degrees);
	 	} else {
			StrPrintF(coordStr, "%d:", degrees);
		}
		coordStr += 4;
	}

	if (minutes < 10) {
		StrPrintF(coordStr, "0%d:", minutes);
	} else {
		StrPrintF(coordStr, "%d:", minutes);
	}
	coordStr += 3;

	if (seconds < 10) {
		StrPrintF(coordStr, "0%d.", seconds);
	} else {
		StrPrintF(coordStr, "%d.", seconds);
	}
	coordStr += 3;

	if (lat) {
		nsew = (coord >= 0.0) ? 'N' : 'S';
	} else {
		nsew = ((coord >= 0.0) && (coord <= 180.0)) ? 'E' : 'W';
	}

	if (decSeconds < 10) {
		StrPrintF(coordStr, "0%d%c", decSeconds, nsew);
	} else {
		StrPrintF(coordStr, "%d%c", decSeconds, nsew);
	}

	return(nsew);
}

/*****************************************************************************
 * DegMinSecToLatLon - convert decimal degrees, minutes, fractions of minutes, 
 * seconds and fractions of seconds to a double.  2.7778e-4 is 1/3600
 *****************************************************************************/
double DegMinSecToLatLon(Int16	deg, Int16 min, double fracMin,
				Int16 sec, double fracSec, Char nsew)
{
	double	latLon;

	latLon = (double)deg + ((double)min / 60.0) + (fracMin / 60.0) + ((double)sec / 3600.0) +
			(fracSec * 2.7778e-4);

	if (nsew == 'w' || nsew == 'W' || nsew == 's' || nsew == 'S')
		return(latLon * -1.0);
	else
		return(latLon);
}


/*****************************************************************************
 * DegMinStringToLatLon - convert degrees, minutes, decimal mins to a double. 
 * String is of the form DD(D)MM.MMC where C is N, S, E or W
 * If a NULL is passed in for nsew, assume the N,S,E or W are in latLonstring
 *****************************************************************************/
double DegMinStringToLatLon(Char *latLonstring, Char nsew)
{
	double latLondbl;
	double latLon;
	double deg;
	double min;
	Int16  i;
	Boolean result = true;

	if (nsew == NULL) {
		// find the N, S, E, or W before we destroy it
		for (i = StrLen(latLonstring)-1; i > 0 && latLonstring[i] < 'A'; i--)
			;	// empty for body

		if (i <= 0)
			result = false;
		else {
			nsew = latLonstring[i];

			// make sure of a valid nsew character
			switch (nsew) {
				case 'N':
				case 'n':
				case 'S':
				case 's':
				case 'E':
				case 'e':
				case 'W':
				case 'w':
					break;

				default:
					result = false;
					break;
			}

			// drop in a NULL
			latLonstring[i] = '\0'; 
		} 
	}
	latLondbl = StrToDbl(latLonstring);
	deg = trunc(latLondbl/100.0);
	min = Fmod(latLondbl,100.0);
	latLon = deg + (min / 60.0);

	if (result == false) {
		return(INVALID_LAT_LON);
	}
	if (nsew == 'w' || nsew == 'W' || nsew == 's' || nsew == 'S') {
		return (latLon * -1.0);
	} else {
		return (latLon);
	}

}

/*****************************************************************************
 * DegMinSecStringToLatLon - convert degrees, minutes, decimal mins to a double. 
 * String is of the form DD(D)MM.MMC where C is N, S, E or W
 *****************************************************************************/
double DegMinSecStringToLatLon(Char *latLonstring, Char nsew)
{
	double latLondbl;
	double latLon;
	double deg;
	double min;
	double sec;
	Char tempchar[2];

	tempchar[0] = nsew;
	tempchar[1] = '\0';
	if (StrStr("NSEWnsew", tempchar) == NULL) return(INVALID_LAT_LON);

	latLondbl = StrToDbl(latLonstring);
	deg = trunc(latLondbl/10000.0);
	latLondbl = latLondbl - (deg*10000.0);
	min = trunc(latLondbl/100.0);
	sec = Fmod(latLondbl,100.0);
	latLon = deg + (min / 60.0) + (sec / 3600.0);

	if (Fabs(latLon) > 180.0) return (INVALID_LAT_LON);

	if (nsew == 'w' || nsew == 'W' || nsew == 's' || nsew == 'S') {
		return (latLon * -1.0);
	} else {
		return (latLon);
	}

}

/*****************************************************************************
 * DegMinColonStringToLatLon - parse a lat or lon string in DD:MM(.MMM)C format
 *		where 'C' is N, S, E, or W.  Only does three places of fractions of
 *		seconds.
 *****************************************************************************/
double DegMinColonStringToLatLon(Char *inP)
{
	Int32 	degrees=0, minutes=0, seconds=0;
	double	fracSeconds= .0;
	double 	fracMinutes=0.0;
	char	*startP, *charP=NULL, *coordP;
	char	nsew=NULL;
	Int16	len, i;
	Boolean	result = true;

	// create string for copying
	len = StrLen(inP);
	if (len < 1 || !AllocMem((void *)&coordP, len))
		return INVALID_LAT_LON;
	
	// copy the incoming string
	MemMove(coordP, inP, len);
	startP = coordP;

	// find the N, S, E, or W before we destroy it
	for (i = len - 1; i > 0 && coordP[i] < 'A'; i--)
		;	// empty for body

	if (i <= 0)
		result = false;
	else {
		nsew = coordP[i];

		// make sure of a valid nsew character
		switch (nsew) {
		case 'N':
		case 'n':
		case 'S':
		case 's':
		case 'E':
		case 'e':
		case 'W':
		case 'w':
			break;

		default:
			result = false;
			break;
		}

		// drop in a NULL
		coordP[i] = NULL;
	} 

	// back to the start - find degree
	if (result) {
		if ((charP = StrChr(startP, ':')) != NULL) 
			*charP = NULL;
		degrees = StrAToI(startP);
	}
	else
		result = false;

	// find minutes if last search succeeded
	if (result && charP) {
		startP = charP + 1;
		if ((startP - coordP) < len) {
			// may not be any decimal seconds
			if ((charP = StrChr(startP, '.')) != NULL) 
				*charP = NULL;
			minutes = StrAToI(startP);
		}
		else
			result = false;
	}

	// find decimal minutes if last search succeeded
	if (result && charP)
		result = RightHandToD(charP + 1, &fracMinutes);

	// free local string
	FreeMem((void *)&coordP);

	// convert values to lat/lon
	if (result) {
		if (degrees > 180 || degrees < 0) {
			return INVALID_LAT_LON;
		}
		if (minutes > 60 || minutes < 0) {
			return INVALID_LAT_LON;
		}
		if (seconds > 60 || seconds < 0) {
			return INVALID_LAT_LON;
		}
		if (fracMinutes >= 1.0) {
			return INVALID_LAT_LON;
		}
		if (fracSeconds >= 1.0) {
			return INVALID_LAT_LON;
		}

		return(DegMinSecToLatLon(degrees, minutes, fracMinutes, seconds, fracSeconds, nsew));
	} else {
		return(INVALID_LAT_LON);
	}
}

/*****************************************************************************
 * DegMinSecColonStringToLatLon - parse a lat or lon string in DD:MM:SS(.SSS)C format
 *		where 'C' is N, S, E, or W.  Only does three places of fractions of
 *		seconds.
 *****************************************************************************/
double DegMinSecColonStringToLatLon(Char* inP)
{
	Int32	degrees=0, minutes=0, seconds=0;
	double	fracSeconds=0.0;
	double	fracMinutes=0.0;
	char	*startP, *charP=NULL, *coordP;
	char	nsew=NULL;
	Int16	len, i;
	Boolean	result = true;
	Boolean nosecs = false;

	// create string for copying
	len = StrLen(inP);
	if (len < 1 || !AllocMem((void *)&coordP, len))
		return INVALID_LAT_LON;

	// copy the incoming string
	MemMove(coordP, inP, len);
	startP = coordP;

	// find the N, S, E, or W before we destroy it
	for (i = len - 1; i > 0 && coordP[i] < 'A'; i--)
		;	// empty for body

	if (i <= 0) {
		result = false;
	} else {
		nsew = coordP[i];

		// make sure of a valid nsew character
		switch (nsew) {
			case 'N':
			case 'n':
			case 'S':
			case 's':
			case 'E':
			case 'e':
			case 'W':
			case 'w':
				result = true;
				break;
			default:
				result = false;
				break;
		}

		// drop in a NULL
		coordP[i] = NULL;
	} 

	// back to the start - find degree
	if (result) {
		if ((charP = StrChr(startP, ':')) != NULL)
			*charP = NULL;
		degrees = StrAToI(startP);
	}
	else
		result = false;

	// find minutes if last search succeeded
	if (result && charP) {
		startP = charP + 1;
		if ((startP - coordP) < len && (charP = StrChr(startP, ':')) != NULL) {
			*charP = NULL;
			minutes = StrAToI(startP);
		} else if ((startP - coordP) < len) {
			if ((charP = StrChr(startP, ':')) != NULL) 
				*charP = NULL;
			minutes = StrAToI(startP);
			if (charP) {
				result = RightHandToD(charP + 1, &fracMinutes);
			}
			nosecs = true;
		} else {
			result = false;
		}
	}

	// find seconds if last search succeeded & there are seconds
	if (!nosecs) {
		if (result && charP) {
			startP = charP + 1;
			if ((startP - coordP) < len && (charP = StrChr(startP, '.')) != NULL)
				*charP = NULL;
			// may not be any decimal seconds
			seconds = StrAToI(startP);
		}

		// find decimal seconds if last search succeeded
		if (result && charP)
			result = RightHandToD(charP + 1, &fracSeconds);
	}

	// free local string
	FreeMem((void *)&coordP);

	// convert values to lat/lon
	if (result) {
		if (degrees > 180 || degrees < 0) {
			return INVALID_LAT_LON;
		}
		if (minutes > 60 || minutes < 0) {
			return INVALID_LAT_LON;
		}
		if (seconds > 60 || seconds < 0) {
			return INVALID_LAT_LON;
		}
		if (fracMinutes >= 1.0) {
			return INVALID_LAT_LON;
		}
		if (fracSeconds >= 1.0) {
			return INVALID_LAT_LON;
		}

		return(DegMinSecToLatLon(degrees, minutes, fracMinutes, seconds, fracSeconds, nsew));
	} else {
		return(INVALID_LAT_LON);
	}
}


/*****************************************************************************
 * PositToPixelLinearInterp - convert a Position Lat,Lon to a Pixel X,Y via
 *		Linear Interpolation
 *****************************************************************************/
void PositToPixelLinearInterp(LinearInterp *liP, double lat, double lon, Int16 *xP, Int16 *yP)
{
	double	relLat, relLon, ratioX, ratioY;

	if (! liP)
		return;

	// hope we're not stradling a hemisphere line

	// offset of posit from upper left
	relLat = liP->ulLat - lat;
	relLon = lon - liP->ulLon;

	// ratio is rel / delta
	ratioX = relLon / liP->deltaLon;
	*xP = (Int16)(liP->pixWidth * ratioX);
	ratioY = relLat / liP->deltaLat;
	*yP = (Int16)(liP->pixHeight * ratioY);
}


/*****************************************************************************
 * PixelToPositLinearInterp - x and y must be relative to the raster, NOT
 *		the screen
 *****************************************************************************/
void PixelToPositLinearInterp(LinearInterp *liP, Int32 x, Int32 y, double *latP, double *lonP)
{
	double	ratioX, ratioY;

	// + 0.001 at end fixes rounding problems
	ratioX = (double)x / (double)(liP->pixWidth) + 0.001;
	ratioY = (double)y / (double)(liP->pixHeight) + 0.001;

	*lonP = liP->ulLon + (ratioX * liP->deltaLon);
	*latP = liP->ulLat - (ratioY * liP->deltaLat);
}


/*****************************************************************************
 * CoordDeltaLinearInterp - get the delta (difference) between two coordinates
 *****************************************************************************/
double CoordDeltaLinearInterp(double coord1, double coord2)
{
	if (coord1 > 0.0 && coord2 < 0.0)
		// coords in different hemispheres
		return coord1 - coord2;
	else if (coord1 < 0.0 && coord2 > 0.0)
		// coords in different hemispheres
		return coord2 - coord1;
	else if (coord2 > coord1)
		// coords in same hemispheres
		return coord2 - coord1;
	else
		// coords in same hemispheres
		return coord1 - coord2;
}


/*****************************************************************************
 * CoordDeltaModLinearInterp - change coord2 so it is closer to or further
 *		from coord1 according to the ratio argument.
 *****************************************************************************/
double CoordDeltaModLinearInterp(double coord1, double coord2, double ratio)
{
	if (coord1 > 0.0 && coord2 < 0.0)
		// coords in different hemispheres
		return (coord1 - ((coord1 - coord2) * ratio));
	else if (coord1 < 0.0 && coord2 > 0.0)
		// coords in different hemispheres
		return (coord1 + ((coord2 - coord1) * ratio));
	else if (coord2 > coord1)
		// coords in same hemispheres
		return (coord1 + ((coord2 - coord1) * ratio));
	else
		// coords in same hemispheres
		return (coord1 - ((coord1 - coord2) * ratio));
}


/*****************************************************************************
 * MidCoordLinearInterp - get the coordinte between two coordinates
 *****************************************************************************/
double MidCoordLinearInterp(double coord1, double coord2)
{
	double divCoord;

	divCoord = CoordDeltaLinearInterp(coord1, coord2) / 2.0;
	return(divCoord + ((coord1 < coord2) ? coord1 : coord2));
}

/*****************************************************************************
 * RangeBearingToLatLon - get the coordintes at the end of the
 *		range/bearing line
 *   THIS IS NOT WORKING CORRECTLY!!!!!!!!!!
 *   NOW WITH TEMP FIX
 *****************************************************************************/
void RangeBearingToLatLon(double fromLat, double fromLon, double rng, double truebrg, double *toLat, double *toLon)
{
/*
	double dLon;
	double plat = 0.0, plon = 0.0;
	double tlat = 0.0, tlon = 0.0;
	double radtruebrg;
	double radrng;

	plat = DegreesToRadians(fromLat);
	plon = DegreesToRadians(fromLon);

	radtruebrg = DegreesToRadians(nice_brg(truebrg));
	radrng = rng/NMPR;

	tlat = Asin(Sin(plat) * Cos(radrng) + Cos(plat) * Sin(radrng) * Cos(radtruebrg));
	dLon = Atan2(Sin(radtruebrg) * Sin(radrng) * Cos(plat), Cos(radrng) - Sin(plat) * Sin(*toLat));
	tlon = Fmod(plon-dLon+PI, TWOPI) - PI;

	*toLat = RadiansToDegrees (tlat);
	*toLon = RadiansToDegrees (tlon);
*/
	double	plat = 0.0, plon = 0.0;
	double	tlat = 0.0, tlon = 0.0;
	double	x_ad;
	double	y_ad;
	double  temprng, tempbrg;
/*
// NOT WORKING!
	// calculate an endpoint given a startpoint, bearing and distance
	// Vincenty 'Direct' formula based on the formula as described at http://www.movable-type.co.uk/scripts/latlong-vincenty-direct.html
	// original JavaScript implementation © 2002-2006 Chris Veness
	double a = 6378137, b = 6356752.3142, f = 1/298.257223563;  // WGS-84 ellipsiod

	double lat1 = fromLat;
	double lon1 = fromLon;

	double s = rng;
	double alpha1 = DegreesToRadians(truebrg);
	double sinAlpha1 = sin(alpha1);
	double cosAlpha1 = cos(alpha1);

	double tanU1 = (1 - f) * tan(lat1);
	double cosU1 = 1 / sqrt((1 + tanU1 * tanU1));
	double sinU1 = tanU1 * cosU1;
	double sigma1 = atan2(tanU1, cosAlpha1);
	double sinAlpha = cosU1 * sinAlpha1;
	double cosSqAlpha = 1 - sinAlpha * sinAlpha;
	double uSq = cosSqAlpha * (a * a - b * b) / (b * b);
	double A = 1 + uSq / 16384 * (4096 + uSq * (-768 + uSq * (320 - 175 * uSq)));
	double B = uSq / 1024 * (256 + uSq * (-128 + uSq * (74 - 47 * uSq)));

	double sigma = s / (b * A);
	double sigmaP = 2.0 * PI;

	double cos2SigmaM = 0.0;
	double sinSigma = 0.0;
	double cosSigma = 0.0;
        double deltaSigma = 0.0;
	double tmp;
	double lat2;
	double lambda;
	double C;
	double L;
	double lon2;

	while (abs(sigma - sigmaP) > 1e-12) {
		cos2SigmaM = cos(2 * sigma1 + sigma);
		sinSigma = sin(sigma);
		cosSigma = cos(sigma);
		deltaSigma = B * sinSigma * (cos2SigmaM + B / 4 * (cosSigma * (-1 + 2 * cos2SigmaM * cos2SigmaM) - B / 6 * cos2SigmaM * (-3 + 4 * sinSigma * sinSigma) * (-3 + 4 * cos2SigmaM * cos2SigmaM)));
		sigmaP = sigma;
		sigma = s / (b * A) + deltaSigma;
	}

	tmp = sinU1 * sinSigma - cosU1 * cosSigma * cosAlpha1;
	lat2 = atan2(sinU1 * cosSigma + cosU1 * sinSigma * cosAlpha1, (1 - f) * sqrt(sinAlpha * sinAlpha + tmp * tmp));
	lambda = atan2(sinSigma * sinAlpha1, cosU1 * cosSigma - sinU1 * sinSigma * cosAlpha1);
	C = f / 16 * cosSqAlpha * (4 + f * (4 - 3 * cosSqAlpha));
	L = lambda - (1 - C) * f * sinAlpha * (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma * (-1 + 2 * cos2SigmaM * cos2SigmaM)));

	lon2 = lon1 + L;

	*toLat = RadiansToDegrees(lat2);
	*toLon = RadiansToDegrees(lon2);
	HostTraceOutputT(appErrorClass, "New =|%s|", DblToStr(*toLat, 5));
	HostTraceOutputTL(appErrorClass, ": |%s|", DblToStr(*toLon, 5));
*/
// current method
	if (isnan(fromLat) || isnan(fromLon))
		return;

	plat = DegreesToRadians(fromLat);
	plon = DegreesToRadians(fromLon);

	truebrg = nice_brg(truebrg);

	x_ad = Fabs(Sin(DegreesToRadians(truebrg)) * rng / NMPR) / data.input.coslat;
	y_ad = Fabs(Cos(DegreesToRadians(truebrg)) * rng / NMPR);

	// temp fix to improve accuracy
	LatLonToRangeBearingEll(fromLat, fromLon, RadiansToDegrees(plat + y_ad), RadiansToDegrees(plon + x_ad), &temprng, &tempbrg);
	x_ad *= rng/temprng;
	y_ad *= rng/temprng;

	if (truebrg <= 90.0 || truebrg >= 270.0)
		tlat = plat + y_ad;
	else
		tlat = plat - y_ad;
	if (truebrg > 0.0 && truebrg < 180.0)
		tlon = plon + x_ad;
	else
		tlon = plon - x_ad;

	*toLat = RadiansToDegrees (tlat);
	*toLon = RadiansToDegrees (_correct(tlon));

//	HostTraceOutputT(appErrorClass, "Old =|%s|", DblToStr(*toLat, 5));
//	HostTraceOutputTL(appErrorClass, ": |%s|", DblToStr(*toLon, 5));
//	HostTraceOutputTL(appErrorClass, "--------------------");
}


/*****************************************************************************
 * RangeBearingToLatLonLinearInterp - get the coordintes at the end of the
 *		range/bearing line
 *****************************************************************************/
void RangeBearingToLatLonLinearInterp(double fromLat, double fromLon, double rng, double brg, double *toLatP, double *toLonP)
{
	double	plat = 0.0, plon = 0.0;
	double	tlat = 0.0, tlon = 0.0;
	double	x_ad;
	double	y_ad;

	if (isnan(fromLat) || isnan(fromLon))
		return;

	plat = DegreesToRadians(fromLat);
	plon = DegreesToRadians(fromLon);

	brg = nice_brg(brg);

	x_ad = Fabs(Sin(DegreesToRadians(brg)) * rng / NMPR) / data.input.coslat;
	y_ad = Fabs(Cos(DegreesToRadians(brg)) * rng / NMPR);

	if (brg <= 90.0 || brg >= 270.0)
		tlat = plat + y_ad;
	else
		tlat = plat - y_ad;
	if (brg > 0.0 && brg < 180.0)
		tlon = plon + x_ad;
	else
		tlon = plon - x_ad;

	*toLatP = RadiansToDegrees (tlat);
	*toLonP = RadiansToDegrees (_correct(tlon));
}

/*****************************************************************************
 * LatLonToRangeBearing : returns nautical miles and degrees
 *  Returns bearing in True
 *****************************************************************************/
void LatLonToRangeBearing(double from_lat, double from_lon, double to_lat, double to_lon, double *rangeP, double *bearingP )
{
	double bearing;
	double range;
	double lat, lon;

	lat = to_lat - from_lat;
	lon = (to_lon - from_lon) * data.input.coslat;

	range = 60.0 * Sqrt(lat*lat + lon*lon);

	if (lon == 0.0) {
		if (lat < 0.0) {
			bearing  = PI;
		} else { 
			bearing = 0.0;
		}
	} else if (lat == 0.0) {
			bearing = (PI / 2.0);
		} else {
			bearing = (PI / 2.0) - Atan(lat / lon);
		}

	if (lon < 0.0) {
		bearing += PI;
	}

	*bearingP = nice_brg(RadiansToDegrees(bearing));
	*rangeP = range;

	return;
}


/*****************************************************************************
 * LatLonToRangeBearingEll
 * glat1 initial geodetic latitude in radians N positive 
 * glon1 initial geodetic longitude in radians E positive 
 * glat2 final geodetic latitude in radians N positive 
 * glon2 final geodetic longitude in radians E positive 
 *****************************************************************************/
void LatLonToRangeBearingEll(double from_lat, double from_lon, double to_lat, double to_lon, double *rangeP, double *bearingP )
{
	double a=0.0, f=0.0;
	double r, tu1, tu2, cu1, su1, cu2, s1, b1, f1;
	double x, sx, cx, sy=0.0, cy=0.0,y=0.0, sa, c2a=0.0, cz=0.0, e=0.0, c, d;
	double EPS= 0.00000000005;
	Int16 iter=1;
	Int16 MAXITER=100;
	double glat1, glon1, glat2, glon2;
	glat1 = DegreesToRadians (from_lat);
	glon1 = DegreesToRadians (from_lon);
	glat2 = DegreesToRadians (to_lat);
	glon2 = DegreesToRadians (to_lon);

	if (from_lat == to_lat && from_lon == to_lon) {
		*rangeP = 0.0;
		*bearingP = 0.0;
		return;
	}

	switch (data.config.earthmodel) {
		case EMWGS84:
			// WGS84 Flattening Values
			a = 6378.137/1.852;
			f = 1.0/298.257223563;
			break;
		case EMFAI:
			// FAI Sphere Flattening Values
			a = 6371.0/1.852;
			f = 1.0/1000000000.0;
			break;
		default:
			// WGS84 Flattening Values
			a = 6378.137/1.852;
			f = 1.0/298.257223563;
			break;
	}

	if ((glat1+glat2==0.0) && (Fabs(glon1-glon2)==PI)) {
//		alert("Course and distance between antipodal points is undefined")
		glat1=glat1+0.00001; // allow algorithm to complete
	}

	if (glat1==glat2 && (glon1==glon2 || Fabs(Fabs(glon1-glon2)-2.0*PI) < EPS)) {
//		alert("Course and distance between antipodal points is undefined")
		glat1=glat1+0.00001; // allow algorithm to complete
//		alert("Points 1 and 2 are identical- course undefined")
//		out=new MakeArray(0)
//		out.d=0
//		out.crs12=0
//		out.crs21=Math.PI
//		return out
	}
	r = 1.0 - f;
	tu1 = r * tan(glat1);
	tu2 = r * tan(glat2);
	cu1 = 1.0 / sqrt(1.0 + tu1 * tu1);
	su1 = cu1 * tu1;
	cu2 = 1.0 / sqrt (1.0 + tu2 * tu2);
	s1 = cu1 * cu2;
	b1 = s1 * tu2;
	f1 = b1 * tu1;
	x = glon2 - glon1;
	d = x + 1.0; // force one pass
	while ((Fabs(d - x) > EPS) && (iter < MAXITER)) {
		iter++;
		sx = sin(x);
		cx = cos(x);
		tu1 = cu2 * sx;
		tu2 = b1 - su1 * cu2 * cx;
		sy = sqrt(tu1 * tu1 + tu2 * tu2);
		cy = s1 * cx + f1;
		y = atan2(sy, cy);
		sa = s1 * sx / sy;
		c2a = 1.0 - sa * sa;
		cz = f1 + f1;
		if (c2a > 0.0) {
			cz = cy - cz / c2a;
		}
		e = cz * cz * 2.0 - 1.0;
		c = ((-3.0 * c2a + 4.0) * f + 4.0) * c2a * f / 16.0;
		d = x;
		x = ((e * cy * c + cz) * sy * c + y) * sa;
		x = (1.0 - c) * x * f + glon2 - glon1;
	}
	*bearingP = nice_brg(RadiansToDegrees(modcrs(atan2(tu1, tu2))));
	x = sqrt ((1.0 / (r * r) - 1.0) * c2a + 1.0);
	x +=1.0;
	x = (x - 2.0) / x;
	c = 1.0 - x;
	c = (x * x / 4.0 + 1.0) / c;
	d = (0.375 * x * x - 1.0) * x;
	x = e * cy;
	*rangeP = ((((sy*sy*4.0-3.0)*(1.0-e-e)*cz*d/6.0-x)*d/4.0+cz)*sy*d+y)*c*a*r;
	if ((Fabs(iter-MAXITER)) < EPS){
//		alert("Algorithm did not converge")
	}
	return;
}

/*****************************************************************************
 * LatLonToRange : approximate function returns nautical miles
 *****************************************************************************/
void LatLonToRange( double from_lat, double from_lon, double to_lat, double to_lon, double *rangeP )
{
	double range;
	double lat, lon;

	lat = to_lat - from_lat;
	lon = (to_lon - from_lon) * data.input.coslat;

	range = 60.0 * Sqrt(lat*lat + lon*lon);
	*rangeP = range;

	return;
}

// returns distance in nm squared for speed
double LatLonToRange2( double from_lat, double from_lon, double to_lat,  double to_lon)
{
	double lat, lon;

	lat = to_lat - from_lat;
	lon = (to_lon - from_lon) * data.input.coslat;
	return(3600.0 * (lat*lat + lon*lon));
}

/*****************************************************************************
 * LatLonToBearing : approximate function : returns degrees
 *  Returns bearing in True
 *****************************************************************************/
void LatLonToBearing( double from_lat, double from_lon, double to_lat, double to_lon, double *bearingP )
{
	double bearing;
	double lat, lon;

	lat = to_lat - from_lat;
	lon = (to_lon - from_lon) * data.input.coslat;

	if (lon == 0.0) {
		if (lat >= 0.0) {
			bearing  = 0.0;
		} else { 
			bearing = PI;
		}
	} else if (lat == 0.0) {
			bearing = (PI / 2.0);
		} else {
			bearing = (PI / 2.0) - Atan(lat / lon);
		}

	if (lon < 0.0) {
		bearing += PI;
	}

	*bearingP = nice_brg(RadiansToDegrees(bearing));

 return;
}

/****************************************************************************/
/* LonDeltaToNM                                                             */
/****************************************************************************/
double LonDeltaToNM(double lonDelta, double lat)
{
	return (lonDelta * 60.0 * Cos(lat));
}

/****************************************************************************/
/* nice_brg                                                                 */
/****************************************************************************/
double nice_brg (double brg)
{
	Int8 ct = 0;

	if (brg < 0.0) {
		brg += 360.0;
		while (brg < 0.0) {
			brg += 360.0;
			ct++;
			if (ct & 2) return(0.0);
		}
	} else {
		while (brg >= 360.0 ) {
			brg -= 360.0;
			ct++;
			if (ct & 2) return(0.0);
		}
	}
	return(brg);
} /* end nice_brg */

/****************************************************************************/
/* RadiansToDegrees                                                         */
/****************************************************************************/
double RadiansToDegrees(double value)
{
	return value * (180.0 / PI);
}


/****************************************************************************/
/* DegreesToRadians                                                         */
/****************************************************************************/
double DegreesToRadians(double value)
{
	return value * (PI / 180.0);
}


/****************************************************************************/
/* _correct                                                                 */
/****************************************************************************/
double _correct(double value)
{
	value -= (2.0 * PI) * (double)((Int32) (value / (2.0 * PI)));
	if ( value < (-1.0 * PI))
		value += 2.0 * PI;
	else if (value > PI)
		value -= 2.0 * PI;
	return value;
}

Boolean PixelOnScreen(Int16 plotX, Int16 plotY)
{
	return(	(plotX<=SCREEN.WIDTH ) && (plotX>=0) && (plotX<MAXINT16) &&
		(plotY<=SCREEN.HEIGHT+(device.DIACapable?SCREEN.SRES*80:0)) && (plotY>=0) && (plotY<MAXINT16) );
}

double PixelToDist(double scale, double pixelval)
{
	double distval;
	distval = ((double)pixelval/(double)SCREEN.WIDTH)*scale;
	return (distval);
}

double XYDistToRng(double xdist, double ydist)
{
	double rngval;
	rngval = Sqrt(xdist*xdist + ydist*ydist);
	return(rngval);
}

double XYDistToBrg(double xdist, double ydist)
{
	double brgval;
	brgval = RadiansToDegrees(Atan2(ydist, xdist));
	return(brgval);
}

void DrawGlider(Int8 maporient, Int16 centerx, Int16 centery)
{
	Int16 plotX = 0, plotY = 0;
	Int16 plotX2 = 0, plotY2 = 0;
	Int8 width = 0;
	double curcse = data.input.magnetic_track.value;

	if (device.HiDensityScrPresent) width = 1;

	if (maporient == TRACKUP) {
		// Vertical line for fuselage
		BresLine(centerx, centery-(SCREEN.SRES*3), centerx, centery+(SCREEN.SRES*7), width);
		// Horizontal line for tail
		BresLine(centerx-(SCREEN.SRES*2), centery+(SCREEN.SRES*7), centerx+(SCREEN.SRES*2), centery+(SCREEN.SRES*7), width);
		// Horizontal line for Wings
		BresLine(centerx-(SCREEN.SRES*7), centery, centerx+(SCREEN.SRES*7), centery, width);
	} else {
		// Draw fuselage
		RangeBearingToPixel(1, 1, centerx, centery, (SCREEN.SRES*3), curcse, NOFORCE, maporient, &plotX, &plotY);
		RangeBearingToPixel(1, 1, centerx, centery, (SCREEN.SRES*7), RecipCse(curcse), NOFORCE, maporient, &plotX2, &plotY2);
		BresLine(plotX, plotY, plotX2, plotY2, width);
		// Draw elevator
		RangeBearingToPixel(1, 1, plotX2, plotY2, (SCREEN.SRES*2), nice_brg(curcse+90.0), NOFORCE, maporient, &plotX, &plotY);
		RangeBearingToPixel(1, 1, plotX2, plotY2, (SCREEN.SRES*2), nice_brg(curcse-90.0), NOFORCE, maporient, &plotX2, &plotY2);
		BresLine(plotX, plotY, plotX2, plotY2, width);
		// Draw Wings
		RangeBearingToPixel(1, 1, centerx, centery, (SCREEN.SRES*7), nice_brg(curcse+90.0), NOFORCE, maporient, &plotX, &plotY);
		RangeBearingToPixel(1, 1, centerx, centery, (SCREEN.SRES*7), nice_brg(curcse-90.0), NOFORCE, maporient, &plotX2, &plotY2);
		BresLine(plotX, plotY, plotX2, plotY2, width);
	}

	return;
}

void DrawPOI(Int16 pixelx, Int16 pixely, char *label, Int16 offset, Int8 maxlen, Boolean reachable, Boolean erasefirst, Int32 type, Boolean forcelabel, double mapscale, Boolean isairport, Boolean islandable, Boolean justreachable)
{
	RectangleType rectP;
	Int8 labellen=StrLen(label);
	Char tempchar[2];
	Boolean colorGreen=false;
	Boolean colorOrange=false;
	Boolean bold=false;
	Boolean uppercase=false;

	rectP.topLeft.x = pixelx-(offset/2);
	rectP.topLeft.y = pixely-(offset/2);
	rectP.extent.x = offset;
	rectP.extent.y = offset;

	// set up highlighting
	if (draw_task) reachable = false;
	if (device.colorCapable) {
		// colour Palms
		colorGreen 	= reachable;
		colorOrange    = justreachable;
//		colorOrange    = justreachable && (isairport || islandable);
		bold 		= reachable && islandable;
		uppercase 	= reachable && isairport;
	} else {
		// black and white Palms
		colorGreen 	= false;	
		colorOrange    = false;
		bold 		= reachable;
		uppercase	= reachable && islandable;
	}

	if (PixelOnScreen(pixelx, pixely)) {
		if (labellen != 0) {
			if (uppercase) ConvertToUpper(label);

			if (erasefirst) {
				if (labellen <= maxlen) { 
					WinEraseChars(label, (Int16)labellen, pixelx + (offset/2+(SCREEN.SRES*2)), pixely - (offset+(SCREEN.SRES*7)));
					FntSetFont(boldFont);
					WinEraseChars(label, (Int16)labellen, pixelx + (offset/2+(SCREEN.SRES*2)), pixely - (offset+(SCREEN.SRES*7)));
					FntSetFont(stdFont);
				} else {
					WinEraseChars(label, (Int16)maxlen, pixelx + (offset/2+(SCREEN.SRES*2)), pixely - (offset+(SCREEN.SRES*7)));
					FntSetFont(boldFont);
					WinEraseChars(label, (Int16)maxlen, pixelx + (offset/2+(SCREEN.SRES*2)), pixely - (offset+(SCREEN.SRES*7)));
					FntSetFont(stdFont);
				}
			}

			// colour reachable waypoints in green
			if (device.colorCapable) {
				if (colorOrange) {
					WinSetTextColor(indexOrange);
				} else if (colorGreen) {
					WinSetTextColor(indexGreen);
				} else {
					WinSetTextColor(indexRed);
				}
			}

			// draw waypoint symbol
			FntSetFont((FontID)WAYSYMB11);
			if (type&AIRPORT) {
				tempchar[0] = 0;
				tempchar[1] = '\0';
				WinDrawChars(tempchar, 1, (pixelx-(5*SCREEN.SRES)), (pixely-(5*SCREEN.SRES)));
			} else if (type&LAND) {
				tempchar[0] = 4;
				tempchar[1] = '\0';
				WinDrawChars(tempchar, 1, (pixelx-(5*SCREEN.SRES)), (pixely-(5*SCREEN.SRES)));
			} else if (type&MARK) {
				tempchar[0] = 47;
				tempchar[1] = '\0';
				WinDrawChars(tempchar, 1, (pixelx-(5*SCREEN.SRES)), (pixely-(5*SCREEN.SRES)));
			} else if (type&TARGET) {
				if (IsMap) {
					tempchar[0] = 7;
				} else {
					tempchar[0] = 5;
				}
				tempchar[1] = '\0';
				if (device.colorCapable) WinSetTextColor(indexBlack);
				WinDrawChars(tempchar, 1, (pixelx-(5*SCREEN.SRES)), (pixely-(5*SCREEN.SRES)));
			} else if (type&THRML) {
				tempchar[0] = 3;
				tempchar[1] = '\0';
				WinDrawChars(tempchar, 1, (pixelx-(5*SCREEN.SRES)), (pixely-(5*SCREEN.SRES)));
			} else if (type&TERRAIN) {
				if (offterrain) {
					tempchar[0] = 11; // fell off terrain map!
				} else {	
					tempchar[0] = 9;
				}
				tempchar[1] = '\0';
				WinDrawChars(tempchar, 1, (pixelx-(5*SCREEN.SRES)), (pixely-(5*SCREEN.SRES)));
			} else if (type&CRASH) {
				tempchar[0] = 10;
				tempchar[1] = '\0';
				WinDrawChars(tempchar, 1, (pixelx-(5*SCREEN.SRES)), (pixely-(5*SCREEN.SRES)));
			} else if (type&MAXPENPT) {
				if (IsMap) {
					tempchar[0] = 54;
				} else {
					tempchar[0] = 55;
				}
				tempchar[1] = '\0';
				if (device.colorCapable) WinSetTextColor(indexBlack);
				WinDrawChars(tempchar, 1, (pixelx-(5*SCREEN.SRES)), (pixely-(5*SCREEN.SRES)));
			} else {
				if (device.colorCapable) {
					WinPushDrawState();
					if (colorOrange) {
						WinSetForeColor(indexOrange);
					} else if (colorGreen) {
						WinSetForeColor(indexGreen);
					} else {
						WinSetForeColor(indexRed);
					}
						
				}
				WinDrawRectangle(&rectP, 0);
				if (device.colorCapable) {
					WinPopDrawState();
				}
			}

			// if landable (or reachable for b&w Palms) use bold text
			if (bold) {
				FntSetFont(boldFont);
			} else {
				FntSetFont(stdFont);
			}

			if (device.colorCapable) {
				WinSetTextColor(indexBlack);
			}

			if (maxlen > 0) { // skip label of length set to zero

				if ((data.config.declutter == 0.0) || (mapscale < data.config.declutter) || forcelabel) {
					if (labellen <= maxlen) { 
						WinDrawChars(label, (Int16)labellen, pixelx + (offset/2+(SCREEN.SRES*2)), pixely - (offset+(SCREEN.SRES*7)));
					} else {
						WinDrawChars(label, (Int16)maxlen, pixelx + (offset/2+(SCREEN.SRES*2)), pixely - (offset+(SCREEN.SRES*7)));
					}
				}
			}

			if (bold) {
				FntSetFont(stdFont);
			}
		}
	}
	return;
}


//  CalcPlotValues
//  Returns bearing in Magnetic
Boolean CalcPlotValues(double origlat, double origlon, double tgtlat, double tgtlon,
			double xratio, double yratio, Int16 *plotX, Int16 *plotY, 
			double *poirange, double *poibearing, Int8 forcecalc, Int8 maporient, Boolean CalcBearing) 
{
	double plotrange;
	double shiftbearing;
	double arcangle;
	Int16 poixdist, poiydist;

//	HostTraceOutputTL(appErrorClass, "Calc-origlat=|%s|", DblToStr(origlat, 5));
//	HostTraceOutputTL(appErrorClass, "Calc-origlon=|%s|", DblToStr(origlon, 5));
//	HostTraceOutputTL(appErrorClass, "Calc-tgtlat=|%s|", DblToStr(tgtlat, 5));
//	HostTraceOutputTL(appErrorClass, "Calc-tgtlon=|%s|", DblToStr(tgtlon, 5));
//	HostTraceOutputTL(appErrorClass, "Calc-latdiff=|%s|", DblToStr(latdiff, 5));
//	HostTraceOutputTL(appErrorClass, "Calc-londiff=|%s|", DblToStr(londiff, 5));
//	HostTraceOutputTL(appErrorClass, "Calc-plotX=|%hd|", *plotX);
//	HostTraceOutputTL(appErrorClass, "Calc-plotY=|%hd|", *plotY);
//	HostTraceOutputTL(appErrorClass, "Calc-poibearing=|%s|", DblToStr(*poibearing, 1));
//	HostTraceOutputTL(appErrorClass, "Calc-poirange=|%s|", DblToStr(*poirange, 1));
//	HostTraceOutputTL(appErrorClass, "============================");

//	if ((origlat == tgtlat) && (origlon == tgtlon)) {
//		*plotX = SCREEN.GLIDERX;
//		*plotY = SCREEN.GLIDERY;
//		*poibearing = 0.0;
//		*poirange = 0.0;

//	Circle based on distance to top corner of screen
//	} else if (forcecalc ||  (latdiff*latdiff + londiff*londiff <= ulRng*ulRng)) {

//	Based in max/min lat/lon of screen corners
//	} else 
	if (forcecalc || ((tgtlat < mapmaxlat) && (tgtlat > mapminlat) && (tgtlon < mapmaxlon) && (tgtlon > mapminlon))) {

		// Find Range & Bearing from glider to POI
		// Returns bearing in true
		LatLonToRangeBearing(origlat, origlon, tgtlat, tgtlon, poirange, poibearing);

		plotrange = *poirange;

		// ensure value does not overflow signed 16bit value divided by two
		if (Fabs(plotrange * bigratio) > 16384.0) {
			plotrange = 16384.0 / bigratio;
		}

		// Shift Bearing of POI by True Course Amount
		shiftbearing = nice_brg(TrueToRelBrg(*poibearing, truecse));

		//MFH Check this logic
		if (shiftbearing >= 270.0) {
			arcangle = shiftbearing - 270.0;
		} else if (shiftbearing >= 180.0) {
			arcangle = 90.0 - (shiftbearing - 180.0);
		} else if (shiftbearing >= 90.0) {
			arcangle = shiftbearing - 90.0;
		} else {
			arcangle = 90.0 - shiftbearing;
		}
		
		if (arcangle == 0.0) {
			poixdist = (Int16)(plotrange * xratio);
			poiydist = 0;
		} else if (arcangle == 90.0) {
			poixdist = 0;
			poiydist = (Int16)(plotrange * yratio);
		} else {
			poixdist = (Int16)(Cos(DegreesToRadians(arcangle)) * plotrange * xratio);
			poiydist = (Int16)(Sin(DegreesToRadians(arcangle)) * plotrange * yratio);
		}
		if (shiftbearing > 270.0 || shiftbearing < 90.0) {
			poiydist = poiydist * -1;
		}
		if (shiftbearing < 360.0 && shiftbearing > 180.0) {
			poixdist = poixdist * -1;
		}
		*plotX = SCREEN.GLIDERX + poixdist;
		*plotY = SCREEN.GLIDERY + poiydist; 

		// Convert to magnetic to return
		if (CalcBearing) *poibearing = nice_brg(*poibearing + data.input.deviation.value);
	} else {
		*plotX = -1;
		*plotY = -1;
		return(false);
	}
	return(true);
}
// end of calcplot

// Expects the inputted bearing to be magnetic
void RangeBearingToPixel(double xratio, double yratio, Int16 centerX, Int16 centerY, double plotrange, double plotbearing, Int8 forcecalc, Int8 maporient, Int16 *plotX, Int16 *plotY) 
{
	double truecse;
	double shiftbearing;
	double arcangle;
	double poixdist, poiydist;

//	HostTraceOutputTL(appErrorClass, "RBTP-plotrange=|%s|", DblToStr(plotrange, 1));
//	HostTraceOutputTL(appErrorClass, "RBTP-plotbearing=|%s|", DblToStr(plotbearing, 1));
	if (maporient == TRACKUP) {
		truecse = data.input.true_track.value;
	} else if ((data.input.curmaporient == COURSEUP) && (data.input.bearing_to_destination.valid==VALID)) {
		truecse = nice_brg(data.input.bearing_to_destination.value - data.input.deviation.value);
	} else {
		truecse = nice_brg(0.0 - data.input.deviation.value);
	}

	// Converts the inputted bearing (magnetic) to True for the calculations
	plotbearing = nice_brg(plotbearing - data.input.deviation.value);

//	if ((forcecalc == FORCE) && (plotrange > (curmapscale * 1.25))) {
//		plotrange = curmapscale * 1.25;
//	}

	/* Shift Bearing of POI by True Course Amount */
//	HostTraceOutputTL(appErrorClass, "RBTP-truecse=|%s|", DblToStr(truecse, 1));
	shiftbearing = TrueToRelBrg(plotbearing, truecse);
//	HostTraceOutputTL(appErrorClass, "RBTP-shiftbearing=|%s|", DblToStr(shiftbearing, 1));

	//MFH Check this logic
	if (shiftbearing >= 270.0) {
		arcangle = nice_brg(shiftbearing - 270.0);
	} else if (shiftbearing >= 180.0) {
		arcangle = nice_brg(90.0 - (shiftbearing - 180.0));
	} else if (shiftbearing >= 90.0) {
		arcangle = nice_brg(shiftbearing - 90.0);
	} else {
		arcangle = nice_brg(90.0 - shiftbearing);
	}
//	HostTraceOutputTL(appErrorClass, "RBTP-arcangle=|%s|", DblToStr(arcangle, 1));
		
	if (arcangle == 0.0) {
		poixdist = pround(plotrange * xratio, 0);
		poiydist = 0.0;
	} else if (arcangle == 90.0) {
		poixdist = 0.0;
		poiydist = pround(plotrange * yratio, 0);
	} else {
		poixdist = pround(Cos(DegreesToRadians(arcangle)) * (plotrange * xratio),0);
		poiydist = pround(Sin(DegreesToRadians(arcangle)) * (plotrange * yratio),0);
	}
//	HostTraceOutputTL(appErrorClass, "RBTP-poixdist=|%s|", DblToStr(poixdist, 1));
//	HostTraceOutputTL(appErrorClass, "RBTP-poiydist=|%s|", DblToStr(poiydist, 1));

//	HostTraceOutputTL(appErrorClass, "RBTP-xratio=|%s|", DblToStr(xratio, 1));
//	HostTraceOutputTL(appErrorClass, "RBTP-yratio=|%s|", DblToStr(yratio, 1));

	if (shiftbearing > 270.0 || shiftbearing < 90.0) {
		poiydist = poiydist * -1.0;
//		HostTraceOutputTL(appErrorClass, "RBTP-poiydist=|%s|", DblToStr(poiydist, 1));
	}
	if (shiftbearing < 360.0 && shiftbearing > 180.0) {
		poixdist = poixdist * -1.0;
//		HostTraceOutputTL(appErrorClass, "RBTP-poixdist=|%s|", DblToStr(poixdist, 1));
	}
	*plotX = centerX + (Int16)poixdist;
	*plotY = centerY + (Int16)poiydist;

//	HostTraceOutputTL(appErrorClass, "RBTP-plotX=|%hd|", *plotX);
//	HostTraceOutputTL(appErrorClass, "RBTP-plotY=|%hd|", *plotY);
//	HostTraceOutputTL(appErrorClass, "RBTP-plotbearing=|%s|", DblToStr(plotbearing, 1));
//	HostTraceOutputTL(appErrorClass, "-----------------------------");
	return;
}

void DrawWayLine(Int16 WayX, Int16 WayY, double gliderLat, double gliderLon, double xratio, double yratio, Boolean ismap)
{
	Int16 centerX=WIDTH_MIN+(SCREEN.SRES*51), centerY=HEIGHT_MIN+(SCREEN.SRES*7);
	Int16 plotX=centerX, plotY=centerY;
	Int16 oppplotX=centerX, oppplotY=centerY;
	Int16 arrowlendX=centerX, arrowlendY=centerY;
	Int16 arrowrendX=centerX, arrowrendY=centerY;
	double xrat=0, yrat=0;
	RectangleType rectP;
	double boldline = 1.0;
	double poirange, poibearing;

	// check for bold line
	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(kCoordinatesStandard);
		boldline = SCREEN.SRES;
	}

	// draw main waypoint line
	if (device.colorCapable) WinSetForeColor(indexWaypt);
	if (ismap) {
		// draw waypoint line on moving map screen
		WinDrawClippedLine(SCREEN.GLIDERX/boldline, SCREEN.GLIDERY/boldline, WayX/boldline, WayY/boldline, (data.config.BoldWaypt?SOLID:DASHED));
	} else {
		// draw waypoint on waypoint sector screen
		CalcPlotValues(gliderLat, gliderLon, data.input.destination_lat, data.input.destination_lon, xratio, yratio, &plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, true);
		CalcPlotValues(gliderLat, gliderLon, data.input.gpslatdbl, data.input.gpslngdbl, xratio, yratio, &oppplotX, &oppplotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
		WinDrawClippedLine(oppplotX/boldline, oppplotY/boldline, plotX/boldline, plotY/boldline, (data.config.BoldWaypt?SOLID:DASHED));
	}
	if (device.colorCapable) WinSetForeColor(indexBlack);

	// reset bold line
	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(device.HiDensityScrPresent);
		boldline = 1.0;
	}

	// draw direction to next waypoint is distance is valid and meets criteria
	if (((data.config.shownextwpt == NEXTALL) || ((data.config.shownextwpt == NEXTCTL) && data.input.isctlpt)) && (data.input.nextwpt_dist >= 0.0)) {
		if (ismap) {
			// draw arrows on moving map
			xrat = ARROWCENTXOFF / (ARROWCENTXOFF / ARROWAREAWIDTH) / (2/SCREEN.SRES);
			yrat = ARROWCENTYOFF / (ARROWCENTYOFF / ARROWAREAWIDTH) / (2/SCREEN.SRES);

			rectP.topLeft.x = (Coord)(centerX - ARROWCENTXOFF/(2/SCREEN.SRES));
			rectP.topLeft.y = (Coord)(centerY - ARROWCENTYOFF/(2/SCREEN.SRES));
			rectP.extent.x = (Coord)((ARROWAREAWIDTH/(2/SCREEN.SRES)));
			rectP.extent.y = (Coord)(ARROWAREAWIDTH/(2/SCREEN.SRES));
			WinEraseRectangle(&rectP, 0);

			RangeBearingToPixel(xrat, yrat, centerX, centerY, 0.45, data.input.nextwpt_bear, NOFORCE, data.input.curmaporient, &plotX, &plotY);
			RangeBearingToPixel(xrat, yrat, centerX, centerY, 0.45, RecipCse(data.input.nextwpt_bear), NOFORCE, data.input.curmaporient, &oppplotX, &oppplotY);
			RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(data.input.nextwpt_bear - 150.0), NOFORCE, data.input.curmaporient, &arrowlendX, &arrowlendY);
			RangeBearingToPixel(xrat, yrat, plotX, plotY, 0.3, nice_brg(data.input.nextwpt_bear + 150.0), NOFORCE, data.input.curmaporient, &arrowrendX, &arrowrendY);
			WinDrawLine(plotX, plotY, oppplotX, oppplotY);
			WinDrawLine(plotX, plotY, arrowlendX, arrowlendY);
			WinDrawLine(plotX, plotY, arrowrendX, arrowrendY);

			if (device.HiDensityScrPresent) { // only if lines can be seen differently!
				// draw next wpt line on moving map
				// check if next wpt is a Target or Max point in an AREA
				if ((StrCompare(data.input.nextwpt_text, "Tgt") == 0) || (StrCompare(data.input.nextwpt_text, "Max") == 0)) {
					if (device.colorCapable) WinSetForeColor(indexWaypt);
					CalcPlotValues(gliderLat, gliderLon, data.input.nextwpt_lat, data.input.nextwpt_lon, xratio, yratio, &plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
					WinDrawClippedLine(SCREEN.GLIDERX/boldline, SCREEN.GLIDERY/boldline, plotX/boldline, plotY/boldline, (data.config.BoldWaypt?SOLID:DASHED));
					if (device.colorCapable) WinSetForeColor(indexBlack);
				}
			}
		} else {
			if (device.HiDensityScrPresent) { // only if lines can be seen differently!
				// draw next wpt line on waypoint sector screen
				// check if next wpt is a Target or Max point in an AREA
				if ((StrCompare(data.input.nextwpt_text, "Tgt") == 0) || (StrCompare(data.input.nextwpt_text, "Max") == 0)) {
					if (device.colorCapable) WinSetForeColor(indexWaypt);
					CalcPlotValues(gliderLat, gliderLon, data.input.nextwpt_lat, data.input.nextwpt_lon, xratio, yratio, &plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
					WinDrawClippedLine(oppplotX/boldline, oppplotY/boldline, plotX/boldline, plotY/boldline, (data.config.BoldWaypt?SOLID:DASHED));
					if (device.colorCapable) WinSetForeColor(indexBlack);
				}
			}
		}
	}
	return;
}

void DrawTrkTrail(double gliderLat, double gliderLon, double xratio, double yratio, double ulRng)
{
	Int16 x, maxx;
	Int16 drawidx;
	Int16 plotX=0, plotY=0;
	Int16 prevplotX=SCREEN.GLIDERX, prevplotY=SCREEN.GLIDERY;
	double poirange, poibearing;
	Boolean plotvalstat=false, prevplotvalstat=true;
	double boldline = 1.0;
	double TEfactor = 0.0;
	
//	static double E1, E2;
//	static UInt32 T1, T2;
//	double speed, speedms, heightm, polarlossm, Eloss, netto, vario;
	
	// can't draw trail as not enough memory
	if (trail == NULL) return;

//	HostTraceOutputTL(appErrorClass, "Draw-idxmax++=%hd", idxmax);
//	HostTraceOutputTL(appErrorClass, "Draw-trailidx++=%hd", trailidx);
	drawidx = trailidx;
//	HostTraceOutputTL(appErrorClass, "Draw-drawidx++=%hd", drawidx);

	// check for bold line
	if (device.HiDensityScrPresent && data.config.BoldTrack) {
		WinSetCoordinateSystem(kCoordinatesStandard);
		boldline = SCREEN.SRES;
	}

	maxx = idxmax;
	if (mapmode == THERMAL) {
		if (maxx > data.config.thnumtrkpts) maxx = data.config.thnumtrkpts;
	} else {
		if (maxx > data.config.numtrkpts) maxx = data.config.numtrkpts;
	}
//	HostTraceOutputTL(appErrorClass, "Draw maxx=%s", DblToStr(maxx,0));

	if (maxx > 0) 
	for (x=0; x < maxx; x++) {
		//if (trail[drawidx].lat != data.input.gpslatdbl || trail[drawidx].lon != data.input.gpslngdbl) {
		//	plotvalstat = CalcPlotValues(gliderLat, gliderLon, trail[drawidx].lat, trail[drawidx].lon, xratio, yratio, &plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
		//}

		// filter points on lat/lon to reduce number of calculations
		if ((trail[drawidx].lat < mapmaxlat) && (trail[drawidx].lat > mapminlat) && (trail[drawidx].lon < mapmaxlon) && (trail[drawidx].lon > mapminlon)) {
			plotvalstat = CalcPlotValues(gliderLat, gliderLon, trail[drawidx].lat, trail[drawidx].lon, xratio, yratio, &plotX, &plotY, &poirange, &poibearing, NOFORCE, data.input.curmaporient, false);
		} else {
			plotvalstat = false;
		}

		// force calculation if current point on-screen and previous point off-screen or visa-versa
		if (plotvalstat != prevplotvalstat) {
//			HostTraceOutputTL(appErrorClass, "Force Calc");
			if (!plotvalstat) plotvalstat = CalcPlotValues(gliderLat, gliderLon, trail[drawidx].lat, trail[drawidx].lon, xratio, yratio, &plotX, &plotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
			if (!prevplotvalstat) prevplotvalstat = CalcPlotValues(gliderLat, gliderLon, trail[drawidx+1].lat, trail[drawidx+1].lon, xratio, yratio, &prevplotX, &prevplotY, &poirange, &poibearing, FORCEACTUAL, data.input.curmaporient, false);
		}

		if (plotvalstat && prevplotvalstat) {

			// set colour based on lift value;
			if (device.colorCapable) {

/* total energy calc - not stable enough on GPS data to be useful
				if (data.config.totalenergy) {
					// Code for total energy calculation
					// G is gravitational constant (9.81 m/s)
					// mass is excluded as it is constant
					// Potential Energy (height) = G*(h2-h1)
					// Kinetic Energy (speed) = (v1*v1-v2*v2)/2
					// 	h in meters and v in meters/second
					// 	(or h in feet and v in feet/second)
					//	v needs to be true airspeed
					//	taken from vario or calculated using ground speed and wind
					// Polar losses = G*(Av*v + B*v + C)*t
					// 	A,B,C are the polar constants
					// 	t is time interval in secs between samples
					//
					// Energy gain = E2 - E1 - Polar Losses
					//
					
					speed = trail[drawidx].speed;
					speedms = speed * SPDKPHCONST / 3.6; // meters per sec
					heightm = trail[drawidx].height * ALTMETCONST;  // meters

					if (x == 0) {
						T2 = trail[drawidx].time;

						// expected height lost due to polar
						polarlossm = ((T2-T1) * (data.polar.a*speed*speed + data.polar.b*speed + data.polar.c)) * SPDMPSCONST;

						// current total energy
						E2 = GRAVITY_CONST*(heightm - polarlossm) + (speedms*speedms)/2.0;
						Eloss = E1-E2;

                                                //netto = -(Eloss)/(T2-T1)/GRAVITY_CONST;
						netto = data.input.curlift*SPDMPSCONST - polarlossm;						 
                                                
//						HostTraceOutputTL(appErrorClass, "---------------------");
//						HostTraceOutputTL(appErrorClass, "Speed (kmh) %s", DblToStr(speedms*3.6, 0));
//						HostTraceOutputTL(appErrorClass, "Height (m) %s", DblToStr(heightm, 0));
//						HostTraceOutputTL(appErrorClass, "Total Energy %s", DblToStr(E2,3));
//						HostTraceOutputTL(appErrorClass, "Time Diff %lu", T2-T1);
//						HostTraceOutputTL(appErrorClass, "Polar Loss (m) %s", DblToStr(polarlossm, 3));
//						HostTraceOutputTL(appErrorClass, "Energy Loss %s", DblToStr(Eloss, 3));
//						HostTraceOutputTL(appErrorClass, "Vario %s", DblToStr(vario, 3));
//						HostTraceOutputTL(appErrorClass, "Curlift %s", DblToStr(data.input.curlift*SPDMPSCONST, 3));
//						HostTraceOutputTL(appErrorClass, "Netto %s", DblToStr(netto, 3));

					} else if (x == 1) {
						// previous total energy
						E1 = GRAVITY_CONST*heightm + (speedms*speedms)/2.0;
						T1 = trail[drawidx].time;
					}
				}
*/				
/*			if (x == 0) {
				// display data from last track point				
//				HostTraceOutputTL(appErrorClass, "Airspeed %s", print_horizontal_speed2(data.input.true_airspeed.value, 0));
//				HostTraceOutputTL(appErrorClass, "Current lift %s", print_vertical_speed2(data.input.curlift, 3));
//				HostTraceOutputTL(appErrorClass, "Netto lift %s", print_vertical_speed2(data.input.nettolift, 3));
//				HostTraceOutputTL(appErrorClass, "lift %s", print_vertical_speed2(trail[drawidx].lift, 3));
			}
			if (x == 1) {
//				HostTraceOutputTL(appErrorClass, "lift %s", print_vertical_speed2(trail[drawidx].lift, 3));
			}
*/
			if (trail[drawidx].lift >= data.input.newmc - TEfactor) {
					WinSetForeColor(indexStrong);
				} else 	if (trail[drawidx].lift >= TEfactor) {
						WinSetForeColor(indexWeak);
					} else {
						WinSetForeColor(indexSink);
					}
			}

			WinDrawClippedLine(plotX/boldline, plotY/boldline, prevplotX/boldline, prevplotY/boldline, SOLID);

			if (device.colorCapable) {
				WinSetForeColor(indexBlack);
			}
		}
			
		drawidx--;
		if (drawidx < 0) { 
			drawidx = idxmax-1;
		}

		if (plotvalstat) {
			prevplotX = plotX;
			prevplotY = plotY;
			prevplotvalstat = plotvalstat;
		}
	}

	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(device.HiDensityScrPresent);
	}
	return;
}

void InitTrkTrail(Boolean reset)
{
	Int16 numtrkpts;
	
	if (data.config.numtrkpts > data.config.thnumtrkpts) {
		numtrkpts = data.config.numtrkpts;
	} else {
		numtrkpts = data.config.thnumtrkpts;
	}
	if ((data.config.numtrkpts > 0) || (data.config.thnumtrkpts > 0)) {
		data.config.trktrail = true;
	} else {
		data.config.trktrail = false;
	}
//	HostTraceOutputTL(appErrorClass, "max trk pts %s", DblToStr(numtrkpts,0));

	if (reset) {
//		HostTraceOutputTL(appErrorClass, "Free trail");
		if (trail) MemPtrFree(trail);
		trail = NULL;
		trailidx = -1;
		idxmax = -1;
	} else if (data.config.trktrail) {
//		HostTraceOutputTL(appErrorClass, "Allocating trail %s", DblToStr(numtrkpts,0));
		trail = (TrkTrail *)MemPtrNew((UInt32)numtrkpts * sizeof(TrkTrail));
//		HostTraceOutputTL(appErrorClass, "trail %s", DblToStr((UInt32)trail,0));
		trailidx = -1;
		idxmax = 0;
	}
}

void SaveTrkTrail()
{
	Int16 numtrkpts;

	if (data.config.numtrkpts > data.config.thnumtrkpts) {
		numtrkpts = data.config.numtrkpts;
	} else {
		numtrkpts = data.config.thnumtrkpts;
	}
//	HostTraceOutputTL(appErrorClass, "max trk pts %s", DblToStr(numtrkpts,0));

	if (trail != NULL) {
		trailidx++;
		if (trailidx == numtrkpts) trailidx = 0;
//		HostTraceOutputTL(appErrorClass, "   if trailidx++=%hu", trailidx);
//		HostTraceOutputTL(appErrorClass, "Inside SaveTrkTrail trailidx=%hu", trailidx);
//		HostTraceOutputTL(appErrorClass, "Inside SaveTrkTrail idxmax=%hu", idxmax);
		trail[trailidx].lat = data.input.gpslatdbl;
		trail[trailidx].lon = data.input.gpslngdbl;
		if (data.config.netto) {
			trail[trailidx].lift = data.input.nettolift;
		} else if (data.config.totalenergy) {
			trail[trailidx].lift = data.input.curlift; // waiting for total energy calcs
		} else {
			trail[trailidx].lift = data.input.curlift;
		}
		trail[trailidx].speed = data.input.true_airspeed.value;
		trail[trailidx].height = data.logger.pressalt;
		trail[trailidx].time = cursecs;
//		HostTraceOutputTL(appErrorClass, "   trailidx++=%hu", trailidx);
		if (idxmax < numtrkpts) idxmax++;
//		HostTraceOutputTL(appErrorClass, "   idxmax++=%hu", idxmax);
	}
	return;
}

/*
 * Circle drawing algorithms.
 */
static void Set4Pixels(Int32 dx, Int32 dy, Int32 xc, Int32 yc)
{

	Int32 plotx, ploty;

	plotx = xc+dx;
	ploty = yc+dy;
	if (PixelOnScreen(plotx, ploty)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx, (Int16)ploty, (Int16)plotx, (Int16)ploty);
		} else {
			WinDrawPixel((Int16)plotx, (Int16)ploty);
		}
	}

	plotx = xc+dx;
	ploty = yc-dy;
	if (PixelOnScreen(plotx, ploty)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx, (Int16)ploty, (Int16)plotx, (Int16)ploty);
		} else {
			WinDrawPixel((Int16)plotx, (Int16)ploty);
		}
	}

	plotx = xc-dx;
	ploty = yc+dy;
	if (PixelOnScreen(plotx, ploty)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx, (Int16)ploty, (Int16)plotx, (Int16)ploty);
		} else {
			WinDrawPixel((Int16)plotx, (Int16)ploty);
		}
	}

	plotx = xc-dx;
	ploty = yc-dy;
	if (PixelOnScreen(plotx, ploty)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx, (Int16)ploty, (Int16)plotx, (Int16)ploty);
		} else {
			WinDrawPixel((Int16)plotx, (Int16)ploty);
		}
	}

}

static void Set8Pixels(Int32 dx, Int32 dy, Int32 xc, Int32 yc)
{
	Int32 plotx1, ploty1;
	Int32 plotx2, ploty2;

	// +x, +y 135-180d
	plotx1 = xc+dx;
	ploty1 = yc+dy;
	if (PixelOnScreen(plotx1, ploty1)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx1, (Int16)ploty1, (Int16)plotx1, (Int16)ploty1);
		} else {
			WinDrawPixel((Int16)plotx1, (Int16)ploty1);
		}
	}

	// -x, -y 315-360d
	plotx2 = xc-dx;
	ploty2 = yc-dy;
	if (PixelOnScreen(plotx2, ploty2)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx2, (Int16)ploty2, (Int16)plotx2, (Int16)ploty2);
		} else {
			WinDrawPixel((Int16)plotx2, (Int16)ploty2);
		}
	}

	// +y, +x 090-135c
	plotx1 = xc+dy;
	ploty1 = yc+dx;
	if (PixelOnScreen(plotx1, ploty1)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx1, (Int16)ploty1, (Int16)plotx1, (Int16)ploty1);
		} else {
			WinDrawPixel((Int16)plotx1, (Int16)ploty1);
		}
	}

	// -y, -x 270-315c
	plotx2 = xc-dy;
	ploty2 = yc-dx;
	if (PixelOnScreen(plotx2, ploty2)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx2, (Int16)ploty2, (Int16)plotx2, (Int16)ploty2);
		} else {
			WinDrawPixel((Int16)plotx2, (Int16)ploty2);
		}
	}

	// y, x 045-090b
	plotx1 = xc+dy;
	ploty1 = yc-dx;
	if (PixelOnScreen(plotx1, ploty1)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx1, (Int16)ploty1, (Int16)plotx1, (Int16)ploty1);
		} else {
			WinDrawPixel((Int16)plotx1, (Int16)ploty1);
		}
	}

	// -y, -x 225-270b
	plotx2 = xc-dy;
	ploty2 = yc+dx;
	if (PixelOnScreen(plotx2, ploty2)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx2, (Int16)ploty2, (Int16)plotx2, (Int16)ploty2);
		} else {
			WinDrawPixel((Int16)plotx2, (Int16)ploty2);
		}
	}

	// +x, -y 000-045a
	plotx1 = xc+dx;
	ploty1 = yc-dy;
	if (PixelOnScreen(plotx1, ploty1)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx1, (Int16)ploty1, (Int16)plotx1, (Int16)ploty1);
		} else {
			WinDrawPixel((Int16)plotx1, (Int16)ploty1);
		}
	}

	// -x, +y 180-225a
	plotx2 = xc-dx;
	ploty2 = yc+dy;
	if (PixelOnScreen(plotx2, ploty2)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine((Int16)plotx2, (Int16)ploty2, (Int16)plotx2, (Int16)ploty2);
		} else {
			WinDrawPixel((Int16)plotx2, (Int16)ploty2);
		}
	}
}

void WinDrawEllipse(Int32 xc, Int32 yc, Int32 a0, Int32 b0)
{
	Int32 x = 0;
	Int32 y = b0;

	Int32 a = a0;
	Int32 b = b0;

	Int32 Asquared = a * a;
	Int32 TwoAsquared = 2 * Asquared;
	Int32 Bsquared = b * b;
	Int32 TwoBsquared = 2 * Bsquared;

	Int32 d;
	Int32 dx,dy;

	d = Bsquared - Asquared * b + Asquared/4;
	dx = 0;
	dy = TwoAsquared * b;

	while (dx < dy) {
		Set4Pixels(x, y, xc, yc);

		if (d > 0) {
			--y;
			dy -= TwoAsquared;
			d -= dy;
		}

		++x;
		dx += TwoBsquared;
		d += Bsquared + dx;
	}

	d += (3 * (Asquared - Bsquared)/2 - (dx+dy)) / 2;

	while (y >= 0) {
		Set4Pixels(x, y, xc, yc);

		if (d < 0) {
			++x;
			dx += TwoBsquared;
			d += dx;
		}

		--y;
		dy -= TwoAsquared;
		d += Asquared - dy;
	}
}

/*
void WinDrawCircle(Int32 xc, Int32 yc, Int32 radius, Int8 lineType)
{
	WinDrawEllipse(xc, yc, radius, radius);
}
*/

//static const CustomPatternType pattern = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
//static const CustomPatternType pattern = {0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00};
//static const CustomPatternType pattern = {0x88, 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 0x00};
//static const CustomPatternType pattern = {0x55, 0x00, 0x55, 0x00, 0x55, 0x00, 0x55, 0x00};
//static const CustomPatternType pattern = {0xEE, 0x00, 0xEE, 0x00, 0xEE, 0x00, 0xEE, 0x00};
//static const CustomPatternType pattern = {0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00};
//static const CustomPatternType pattern = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
//static const CustomPatternType pattern = {0x00, 0xAA, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00};
//static const CustomPatternType pattern = {0x55, 0x00, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00};
//static const CustomPatternType pattern = {0x55, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x55};

/*
void WinDrawCirclePalm(Int32 xc, Int32 yc, Int32 r, Int8 lineType)
{
	RectangleType R1;

	R1.topLeft.x = xc-r;
	R1.topLeft.y = yc-r;
	R1.extent.x = 2*r;
	R1.extent.y = 2*r;


//	WinDrawRectangle(&R1, r);
//	R1.topLeft.x = xc-r+1;
//	R1.topLeft.y = yc-r+1;
//	R1.extent.x = 2*r-2;
//	R1.extent.y = R1.extent.x;
//	WinEraseRectangle(&R1, (r-1));
//	WinSetPattern(&pattern);
//	WinFillRectangle(&R1, r-1);

//	WinDrawCircle(xc, yc, r, SOLID);
//	R1.topLeft.x = xc-r+2;
//	R1.topLeft.y = yc-r+2;
//	R1.extent.x = 2*r-3;
//	R1.extent.y = R1.extent.x;
//	WinEraseRectangle(&R1, (r-2));
//	WinSetPattern(&pattern);
//	WinFillRectangle(&R1, r-1);

//	WinDrawCircle(xc-1, yc-1, r, SOLID);
	WinSetPattern(&pattern);
	WinFillRectangle(&R1, r);
}
*/

void WinDrawCircle(Int32 xc, Int32 yc, Int32 radius, Int8 lineType)
{
	Int32 x = 0, y, d;
	Boolean drawpixel=true;

	y = radius;
	d = 3 - (2 * radius);

	while (x < y) {
		if (drawpixel) {
			Set8Pixels(x, y, xc, yc);
			if (lineType == SOLID) {
				drawpixel = true;
			} else {
				drawpixel = false;
			}
		} else {
			drawpixel = true;
		}

		if (d < 0) {
			d += (x<<2) + 6;
		} else {
			d += ((x - y) << 2) + 10;
			y--;
		}

		x++;

		if (x == y) {
			Set8Pixels(x, y, xc, yc);
		}
	}
}

void WinDrawSector(double xrat, double yrat, Int16 centerX, Int16 centerY, double innerrad, double outerrad, Int16 leftbrg, Int16 rightbrg, Int8 sectortype)
{
//	if (device.HiDensityScrPresent) {
		WinDrawSectorOld(xrat, yrat, centerX, centerY, innerrad, outerrad, leftbrg, rightbrg, sectortype);
//	} else {
//		WinDrawSectorNew(xrat, yrat, centerX, centerY, innerrad, outerrad, leftbrg, rightbrg, sectortype);
//	}
	return;
}

// Expects the inputted bearings to be in Magnetic
void WinDrawSectorSimple(double xrat, double yrat, Int16 centerX, Int16 centerY, 
			double radius,  Int16 leftbrg, Int16 rightbrg,
			Int16 *leftX, Int16 *leftY, Int16 *rightX, Int16 *rightY)
{
	Int16 plotX=0, plotY=0;
	Int16 prevplotX=-1, prevplotY=-1;
	Boolean plotstat=false, prevplotstat=false;
	Int16 x=0;
	Boolean firsttime = true;

	if (leftbrg > rightbrg) {
		rightbrg += 360;
//		HostTraceOutputTL(appErrorClass, "rightbrg+360:|%hd|", rightbrg);
	}

	for (x=leftbrg; x<=rightbrg; (x+=3)) {
		// Expects the inputted bearing to be magnetic
		RangeBearingToPixel(xrat, yrat, centerX, centerY, radius, nice_brg((double)x), FORCEACTUAL, data.input.curmaporient, &plotX, &plotY);
		if (firsttime) {
			*leftX = plotX;
			*leftY = plotY;
			firsttime = false;
		}

	if (PixelOnScreen(plotX, plotY)) {
			plotstat = true;
		} else {
			plotstat = false;
		}
		if (plotstat && prevplotstat) {
			WinDrawLine(plotX, plotY, prevplotX, prevplotY);
		} else if ((plotstat || prevplotstat) && prevplotX > -1) {
			WinDrawClippedLine(plotX, plotY, prevplotX, prevplotY, SOLID);
		}
		prevplotX = plotX;
		prevplotY = plotY;
		prevplotstat = plotstat;
	}

	RangeBearingToPixel(xrat, yrat, centerX, centerY, radius, nice_brg((double)rightbrg), FORCEACTUAL, data.input.curmaporient, &plotX, &plotY);
	*rightX = plotX;
	*rightY = plotY;

	if (PixelOnScreen(plotX, plotY)) {
		plotstat = true;
	} else {
		plotstat = false;
		}
	if (plotstat && prevplotstat) {
		WinDrawLine(plotX, plotY, prevplotX, prevplotY);
	} else if ((plotstat || prevplotstat) && prevplotX > -1) {
		WinDrawClippedLine(plotX, plotY, prevplotX, prevplotY, SOLID);
	}
	return;
}

// Expects the inputted bearings to be in Magnetic
// hi-res version
void WinDrawSectorOld(double xrat, double yrat, Int16 centerX, Int16 centerY, double innerrad, double outerrad, Int16 leftbrg, Int16 rightbrg, Int8 sectortype)
{
	Int16 sectend1X, sectend1Y;
	Int16 sectend2X, sectend2Y;
	Int16 sectend3X, sectend3Y;
	Int16 sectend4X, sectend4Y;

//	HostTraceOutputTL(appErrorClass, "leftbrg:|%hd|", leftbrg);
//	HostTraceOutputTL(appErrorClass, "rightbrg:|%hd|", rightbrg);
	if (leftbrg > rightbrg) {
		rightbrg += 360;
//		HostTraceOutputTL(appErrorClass, "rightbrg+360:|%hd|", rightbrg);
	}

	// If both bearings are the same, simply draw two circles around the center
	if (leftbrg != rightbrg) {
		// Draw Outer Arc of the Sector
		if (sectortype == ALLSECTOR || sectortype == NOBOTTOM || sectortype == NOENDS) {
			// If drawings whole sector, find x,y for all values at outer rad, by 2, from left bearing
			// to right bearing and plot a pixel at each point.
			WinDrawSectorSimple(xrat, yrat, centerX, centerY, outerrad,  leftbrg, rightbrg, &sectend1X, &sectend1Y, &sectend2X, &sectend2Y);
		} else {
			// Find x,y for left bearing and outer radius
			// Expects the inputted bearing to be magnetic
			RangeBearingToPixel(xrat, yrat, centerX, centerY, outerrad, nice_brg((double)leftbrg), NOFORCE, data.input.curmaporient, &sectend1X, &sectend1Y);
			if ((sectend1X <= SCREEN.WIDTH) && (sectend1X >= 0) && (sectend1Y <= SCREEN.HEIGHT) && (sectend1Y >= 0)) {
				// This instead of WinDrawPixel which is only available in PalmOS 3.5 and above
				WinDrawLine(sectend1X, sectend1Y, sectend1X, sectend1Y);
			}

			// If only drawing end lines only, find x,y for right bearing.
			// Expects the inputted bearing to be magnetic
			RangeBearingToPixel(xrat, yrat, centerX, centerY, outerrad, nice_brg((double)rightbrg), NOFORCE, data.input.curmaporient, &sectend2X, &sectend2Y);
		}

		// If Inner Radius is 0, just draw end lines from the Center
		// MFH Should check to ensure that innerrad is not less than outerrad
		if (innerrad > 0.0) {
			// Draw Inner Arc of the Sector
			if (sectortype == ALLSECTOR || sectortype == NOENDS) {
				// If drawings whole sector, find x,y for all values at inner radius, by 2, from left bearing
				// to right bearing and plot a pixel at each point.
				WinDrawSectorSimple(xrat, yrat, centerX, centerY, innerrad,  leftbrg, rightbrg, &sectend3X, &sectend3Y, &sectend4X, &sectend4Y);
			} else {
				// Find x,y for left bearing and inner radius
				// Expects the inputted bearing to be magnetic
				RangeBearingToPixel(xrat, yrat, centerX, centerY, innerrad, nice_brg((double)leftbrg), NOFORCE, data.input.curmaporient, &sectend3X, &sectend3Y);
				if ((sectend3X <= SCREEN.WIDTH) && (sectend3X >= 0) && (sectend3Y <= SCREEN.WIDTH) && (sectend3Y >= 0)) {
					WinDrawLine(sectend3X, sectend3Y, sectend3X, sectend3Y);
				}

				// If only drawing end lines, find x,y for right bearing.
				// Expects the inputted bearing to be magnetic
				RangeBearingToPixel(xrat, yrat, centerX, centerY, innerrad, nice_brg((double)rightbrg), NOFORCE, data.input.curmaporient, &sectend4X, &sectend4Y);
			}
	
			if (sectortype != NOENDS) {
//				HostTraceOutputTL(appErrorClass, "sectend1X:|%hd|  sectend1Y:|%hd|", sectend1X, sectend1Y);
//				HostTraceOutputTL(appErrorClass, "sectend2X:|%hd|  sectend2Y:|%hd|", sectend2X, sectend2Y);
//				HostTraceOutputTL(appErrorClass, "sectend3X:|%hd|  sectend3Y:|%hd|", sectend3X, sectend3Y);
//				HostTraceOutputTL(appErrorClass, "sectend4X:|%hd|  sectend4Y:|%hd|", sectend4X, sectend4Y);
//				HostTraceOutputTL(appErrorClass, "--------------------------------");
				// Draw End Connecting Lines 
				WinDrawClippedLine(sectend1X, sectend1Y, sectend3X, sectend3Y, SOLID);
				WinDrawClippedLine(sectend2X, sectend2Y, sectend4X, sectend4Y, SOLID);
			}
		} else {
			if (sectortype != NOENDS) {
				// Draw End Connecting Lines from the Center
				WinDrawClippedLine(sectend1X, sectend1Y, centerX, centerY, SOLID);
				WinDrawClippedLine(sectend2X, sectend2Y, centerX, centerY, SOLID);
			}
		}
	} else {
		// Both bearings are equal
		if (innerrad > 0.0) {
			// Inner radius is not zero, drawing two circles
			WinDrawCircle((Int32)centerX, (Int32)centerY, (Int32)(outerrad*xrat), SOLID);
			WinDrawCircle((Int32)centerX, (Int32)centerY, (Int32)(innerrad*xrat), SOLID);
		} else {
			// Inner radius is zero, drawing one circle at the outer radius
			WinDrawCircle((Int32)centerX, (Int32)centerY, (Int32)(outerrad*xrat), SOLID);
		}
	}
	return;
}

/*
// Expects the inputted bearings to be in Magnetic
// lo-res version
void WinDrawSectorNew(double xrat, double yrat, Int16 centerX, Int16 centerY, double innerrad, double outerrad, Int16 leftbrg, Int16 rightbrg, Int8 sectortype)
{
	Int16 sectend1X=0, sectend1Y=0;
	Int16 sectend2X=0, sectend2Y=0;
	Int16 sectend3X=0, sectend3Y=0;
	Int16 sectend4X=0, sectend4Y=0;
	Boolean foundleftbrg=false;
	Boolean foundrightbrg=false;
	Int16 clipleftbrg=leftbrg, cliprightbrg=rightbrg;

//	HostTraceOutputTL(appErrorClass, "leftbrg:|%hd|", leftbrg);
//	HostTraceOutputTL(appErrorClass, "rightbrg:|%hd|", rightbrg);
	// Ensure the rightbrg is always greater than leftbrg
	if (leftbrg > rightbrg) {
		rightbrg += 360;
//		HostTraceOutputTL(appErrorClass, "rightbrg+360:|%hd|", rightbrg);
	}

	// If both bearings are the same, simply draw two circles around the center
	if (leftbrg != rightbrg) {
		// Plot the Outer Radius Data
		if (curmapscale >= 1.0) {
			RangeBearingToPixel(xrat, yrat, centerX, centerY, outerrad, (double)leftbrg, FORCEACTUAL, data.input.curmaporient, &sectend1X, &sectend1Y);
			RangeBearingToPixel(xrat, yrat, centerX, centerY, outerrad, (double)rightbrg, FORCEACTUAL, data.input.curmaporient, &sectend2X, &sectend2Y);
			if (sectortype == ALLSECTOR || sectortype == NOBOTTOM || sectortype == NOENDS) {
				EllipseArc2D(centerX, centerY, outerrad, (double)(sectend2X), (double)(sectend2Y), (double)(sectend1X), (double)(sectend1Y), xrat, yrat); 
			}
		} else {
			// Temp Values
			foundleftbrg = foundrightbrg = true;
			clipleftbrg=leftbrg;
			cliprightbrg=rightbrg;
//			HostTraceOutputTL(appErrorClass, "clipleftbrg:|%hd|", clipleftbrg);
//			HostTraceOutputTL(appErrorClass, "cliprightbrg:|%hd|", cliprightbrg);
			if (sectortype == ALLSECTOR || sectortype == NOBOTTOM || sectortype == NOENDS) {
				if (foundleftbrg && foundrightbrg) { 
					WinDrawSectorSimple(xrat, yrat, centerX, centerY, outerrad,  clipleftbrg, cliprightbrg, &sectend1X, &sectend1Y, &sectend2X, &sectend2Y);
				}
			} else {
				RangeBearingToPixel(xrat, yrat, centerX, centerY, outerrad, (double)leftbrg, FORCEACTUAL, data.input.curmaporient, &sectend1X, &sectend1Y);
				RangeBearingToPixel(xrat, yrat, centerX, centerY, outerrad, (double)rightbrg, FORCEACTUAL, data.input.curmaporient, &sectend2X, &sectend2Y);
			}
		}

		// ---------------------------------------------------------------------------
		// Plot the Inner Radius Data
		// If Inner Radius is 0, just draw end lines from the Center
		if (innerrad > 0.0 && innerrad < outerrad) {
			if (curmapscale >= 1.0) {
				RangeBearingToPixel(xrat, yrat, centerX, centerY, innerrad, (double)leftbrg, FORCEACTUAL, data.input.curmaporient, &sectend3X, &sectend3Y);
				RangeBearingToPixel(xrat, yrat, centerX, centerY, innerrad, (double)rightbrg, FORCEACTUAL, data.input.curmaporient, &sectend4X, &sectend4Y);
				if (sectortype == ALLSECTOR || sectortype == NOENDS) {
					EllipseArc2D(centerX, centerY, outerrad, (double)(sectend4X), (double)(sectend4Y), (double)(sectend3X), (double)(sectend3Y), xrat, yrat); 
				}
			} else {
				// Temp Values
				foundleftbrg = foundrightbrg = true;
				clipleftbrg=leftbrg;
				cliprightbrg=rightbrg;
//				HostTraceOutputTL(appErrorClass, "clipleftbrg:|%hd|", clipleftbrg);
//				HostTraceOutputTL(appErrorClass, "cliprightbrg:|%hd|", cliprightbrg);
				if (sectortype == ALLSECTOR || sectortype == NOENDS) {
					if (foundleftbrg && foundrightbrg) { 
							WinDrawSectorSimple(xrat, yrat, centerX, centerY, innerrad,  clipleftbrg, cliprightbrg, &sectend3X, &sectend3Y, &sectend4X, &sectend4Y);
					}
				} else {
					RangeBearingToPixel(xrat, yrat, centerX, centerY,innerrad, (double)leftbrg, FORCEACTUAL, data.input.curmaporient, &sectend3X, &sectend3Y);
					RangeBearingToPixel(xrat, yrat, centerX, centerY, innerrad, (double)rightbrg, FORCEACTUAL, data.input.curmaporient, &sectend4X, &sectend4Y);
				}
			}

			if (sectortype != NOENDS) {
//				HostTraceOutputTL(appErrorClass, "sectend1X:|%hd|  sectend1Y:|%hd|", sectend1X, sectend1Y);
//				HostTraceOutputTL(appErrorClass, "sectend2X:|%hd|  sectend2Y:|%hd|", sectend2X, sectend2Y);
//				HostTraceOutputTL(appErrorClass, "sectend3X:|%hd|  sectend3Y:|%hd|", sectend3X, sectend3Y);
//				HostTraceOutputTL(appErrorClass, "sectend4X:|%hd|  sectend4Y:|%hd|", sectend4X, sectend4Y);
//				HostTraceOutputTL(appErrorClass, "--------------------------------");
				// Draw End Connecting Lines 
				WinDrawClippedLine(sectend1X, sectend1Y, sectend3X, sectend3Y, SOLID);
				WinDrawClippedLine(sectend2X, sectend2Y, sectend4X, sectend4Y, SOLID);
			}
		} else {
			if (sectortype != NOENDS) {
				// Draw End Connecting Lines from the Center
				WinDrawClippedLine(sectend1X, sectend1Y, centerX, centerY, SOLID);
				WinDrawClippedLine(sectend2X, sectend2Y, centerX, centerY, SOLID);
			}
		}
	} else {
		// Both bearings are equal
		if (innerrad > 0.0) {
			// Inner radius is not zero, drawing two circles
			WinDrawCircle((Int32)centerX, (Int32)centerY, (Int32)(outerrad*xrat), SOLID);
			WinDrawCircle((Int32)centerX, (Int32)centerY, (Int32)(innerrad*xrat), SOLID);
		} else {
			// Inner radius is zero, drawing one circle at the outer radius
			WinDrawCircle((Int32)centerX, (Int32)centerY, (Int32)(outerrad*xrat), SOLID);
		}
	}
	return;
}
*/

//---------------------------------------------------------------------------
static void DrawPixel(Int16 plotx, Int16 ploty)
{
	if (PixelOnScreen(plotx, ploty)) {
		if (device.romVersion < SYS_VER_35) {
			WinDrawLine(plotx, ploty, plotx, ploty);
		} else {
			WinDrawPixel(plotx, ploty);
		}
	}
	return;
}

/*
//---------------------------------------------------------------------------
static void SelectEllipsePoint (Int32 rad2, double x, double y, Int32 *ix, Int32 *iy)
{
	Int32 xfloor = (Int32)(Floor(x));
	Int32 yfloor = (Int32)(Floor(y));
	Int32 xincr = rad2*(2*xfloor+1);
	Int32 yincr = rad2*(2*yfloor+1);
	Int32 base = rad2*xfloor*xfloor+rad2*yfloor*yfloor-rad2*rad2;
	Int32 a00 = (Int32)Fabs(base);
	Int32 a10 = (Int32)Fabs(base+xincr);
	Int32 a01 = (Int32)Fabs(base+yincr);
	Int32 a11 = (Int32)Fabs(base+xincr+yincr);
	Int32 min = a00;

	*ix = xfloor;
	*iy = yfloor;
	if ( a10 < min )
	{
		min = a10;
		*ix = xfloor+1;
		*iy = yfloor;
	}
	if ( a01 < min )
	{
		min = a01;
		*ix = xfloor;
		*iy = yfloor+1;
	}
	if ( a11 < min )
	{
		min = a11;
		*ix = xfloor+1;
		*iy = yfloor+1;
	}
}

//---------------------------------------------------------------------------
static Int16 WhichArc (Int32 x, Int32 y)
{
	if (x > 0) {
		if (y > 0)
			return(x <  y ? 0 : 1);
		else if ( y < 0 )
			return(x > (-1*y) ? 2 : 3);
		else
			return(2);
	} else if ( x < 0 ) {
		if ( y < 0 )
			return(y <  x ? 4 : 5);
		else if (y > 0)
			return(y < (-1*x) ? 6 : 7);
		else
			return(6);
	} else {
		return(y > 0 ? 0 : 4);
	}
}
//---------------------------------------------------------------------------
void EllipseArc2D (Int16 xc, Int16 yc, double rad, double fx0, double fy0,
						 double fx1, double fy1, double xrat, double yrat)
{
	// Assert (within floating point roundoff errors):
	//   (fx0-xc)^2/a^2 + (fy0-yc)^2/b^2 = 1
	//   (fx1-xc)^2/a^2 + (fy1-yc)^2/b^2 = 1
	// Assume if (fx0,fy0) == (fx1,fy1), then entire ellipse should be drawn.
	//
	// Integer points on arc are guaranteed to be traversed clockwise.

	Int32 rad2;
	Int16 plotx, ploty;
	Int16 arc;
	Int32 sigma;
	Int32 x0=0, y0=0, x1=0, y1=0;
	Int32 dx = 0;
	Int32 dy = 0;
	Int32 sqrlen;
	Int32 counter = 0;
	Boolean firsttime = true;

//	HostTraceOutputTL(appErrorClass, "EA2D-rad:|%s|", DblToStr(rad, 2));
	rad2 = (Int32)((rad*xrat)*(rad*yrat));
//	HostTraceOutputTL(appErrorClass, "EA2D-rad2:|%ld|", rad2);

	// get integer end points for arc
	SelectEllipsePoint(rad2,fx0-(double)xc,fy0-(double)yc,&x0,&y0);
	SelectEllipsePoint(rad2,fx1-(double)xc,fy1-(double)yc,&x1,&y1);
//	HostTraceOutputTL(appErrorClass, "EA2D-x0:|%ld|  y0:|%ld|", x0, y0);
//	HostTraceOutputTL(appErrorClass, "EA2D-x1:|%ld|  y1:|%ld|", x1, y1);
	dx = x0 - x1;
	dy = y0 - y1;
	sqrlen = dx*dx+dy*dy;
//	HostTraceOutputTL(appErrorClass, "EA2D-sqrlen:|%ld|", sqrlen);

	if ( sqrlen == 1 || ( sqrlen == 2 && Fabs(dx) == 1 ) ) {
		plotx = xc+(Int16)x0;
		ploty = yc+(Int16)y0;
//		HostTraceOutputTL(appErrorClass, "EA2D-Special If plotx1:|%hd|", plotx);
//		HostTraceOutputTL(appErrorClass, "EA2D-           ploty1:|%hd|", ploty);
		DrawPixel(plotx, ploty);
		plotx = xc+(Int16)x1;
		ploty = yc+(Int16)y1;
//		HostTraceOutputTL(appErrorClass, "EA2D-Special If plotx2:|%hd|", plotx);
//		HostTraceOutputTL(appErrorClass, "EA2D-           ploty2:|%hd|", ploty);
		DrawPixel(plotx, ploty);
		return;
	}

	// determine initial case for arc drawing
	arc = WhichArc(x0,y0);
//	HostTraceOutputTL(appErrorClass, "EA2D-arc=|%hd|", arc);
	while ( 1 ) {
		// process the pixel
		plotx = xc+(Int16)x0;
		ploty = yc+(Int16)y0;
//		HostTraceOutputTL(appErrorClass, "EA2D-Inside While plotx:|%hd|", plotx);
//		HostTraceOutputTL(appErrorClass, "EA2D-             ploty:|%hd|", ploty);
//		HostTraceOutputTL(appErrorClass, "EA2D-Inside While arc=|%hd|", arc);
		if (firsttime) {
			WinDrawLine((short int)fx0, (short int)fy0, plotx, ploty);
			firsttime = false;
		}
		DrawPixel(plotx, ploty);

		sigma = 0;
		// Determine next pixel to process.  Notation <(x,y),dy/dx>
		// indicates point on ellipse and slope at that point.
		switch ( arc ) {
			case 0:  // <(0,b),0> to <(u0,v0),-1>
				x0 ++;
				dx ++;
				sigma = rad2*x0*x0+rad2*(y0-1)*(y0-1)-rad2*rad2;
				if ( sigma >= 0 ) {
					y0--;
					dy--;
				}
				if ( rad2*x0 >= rad2*y0 ) {
					// Slope dy/dx is no longer between 0 and -1.  Switch to next
					// arc drawer.  For large a and b, you expect to go to
					// 'arc = 1'.  But for small a or b, it is possible that the
					// next arc is so small (on the discrete raster) that it is
					// skipped.
					if ( y0 > 0 )
						arc = 1;
					else
						arc = 2;
				}
				break;
			case 1:  // <(u0,v0),-1> to <(a,0),infinity>
				y0--;
				dy--;
				sigma = rad2*x0*x0+rad2*y0*y0-rad2*rad2;
				if ( sigma < 0 ) {
					x0++;
					dx++;
				}
				if ( y0 == 0 )
					arc = 2;
				break;
			case 2:  // <(a,0),infinity> to <(u1,v1),+1>
				y0--;
				dy--;
				sigma = rad2*(x0-1)*(x0-1)+rad2*y0*y0-rad2*rad2;
				if ( sigma >= 0 ) {
					x0--;
					dx--;
				}
				if ( rad2*x0 <= -rad2*y0 ) {
					// Slope dy/dx is no longer between 0 and +1.  Switch to next
					// arc drawer.  For large a and b, you expect to go to
					// 'arc = 3'.  But for small a or b, it is possible that the
					// next arc is so small (on the discrete raster) that it is
					// skipped.
					if ( x0 > 0 )
						arc = 3;
					else
						arc = 4;
				}
				break;
			case 3:  // <(u1,v1),+1> to <(0,-b),0>
				x0--;
				dx--;
				sigma = rad2*x0*x0+rad2*y0*y0-rad2*rad2;
				if ( sigma < 0 ) {
					y0--;
					dy--;
				}
				if ( x0 == 0 )
					arc = 4;
				break;
			case 4:  // <(0,-b),0> to <(u2,v2,-1)>
				x0--;
				dx--;
				sigma = rad2*x0*x0+rad2*(y0+1)*(y0+1)-rad2*rad2;
				if ( sigma >= 0 ) {
					y0++;
					dy++;
				}
				if ( rad2*y0 >= rad2*x0 ) {
					// Slope dy/dx is no longer between 0 and -1.  Switch to next
					// arc drawer.  For large a and b, you expect to go to
					// 'arc = 5'.  But for small a or b, it is possible that the
					// next arc is so small (on the discrete raster) that it is
					// skipped.
					if ( y0 < 0 )
						arc = 5;
					else
						arc = 6;
				}
				break;
			case 5:  // <(u2,v2,-1)> to <(-a,0),infinity>
				y0++;
				dy++;
				sigma = rad2*x0*x0+rad2*y0*y0-rad2*rad2;
				if ( sigma < 0 ) {
					x0--;
					dx--;
				}
				if ( y0 == 0 )
					arc = 6;
				break;
			case 6:  // <(-a,0),infinity> to <(u3,v3),+1>
				y0++;
				dy++;
				sigma = rad2*(x0+1)*(x0+1)+rad2*y0*y0-rad2*rad2;
				if ( sigma >= 0 ) {
					x0++;
					dx++;
				}
				if ( rad2*y0 >= -rad2*x0 ) {
					// Slope dy/dx is no longer between 0 and +1.  Switch to next
					// arc drawer.  For large a and b, you expect to go to
					// 'arc = 7'.  But for small a or b, it is possible that the
					// next arc is so small (on the discrete raster) that it is
					// skipped.
					if ( x0 < 0 )
						arc = 7;
					else
						arc = 8;
				}
				break;
			case 7:  // <(u3,v3),+1> to <(0,b),0>
				x0++;
				dx++;
				sigma = rad2*x0*x0+rad2*y0*y0-rad2*rad2;
				if ( sigma < 0 ) {
					y0++;
					dy++;
				}
				if ( x0 == 0 )
					arc = 0;
				break;
			case 8: 
				break;
		}

		if (arc == 8) {
			WinDrawLine((short int)fx1, (short int)fy1, plotx, ploty);
			return;
//			HostTraceOutputTL(appErrorClass, "EA2D-arc = 8");
		}
			
		sqrlen = dx*dx+dy*dy;
//		HostTraceOutputTL(appErrorClass, "EA2D-sqrlen:|%ld|", sqrlen);
		counter++;
		
		if (sqrlen <= 2 || counter >= 5000) {
//			HostTraceOutputTL(appErrorClass, "EA2D EXITING!!!!!-counter:|%ld|", counter);
			break;
		}
	}
	WinDrawLine((short int)fx1, (short int)fy1, plotx, ploty);
//	HostTraceOutputTL(appErrorClass, "-------------------------------------------------");
	return;
}

//---------------------------------------------------------------------------
*/

void WinDrawClippedLine(Int16 x1, Int16 y1, Int16 x2, Int16 y2, Int8 lineType)
{
	// allow Palm OS line routines to handle clipping the line to visible screen
	if (lineType == SOLID) {
		WinDrawLine(x1, y1, x2, y2);
	} else {
		WinDrawGrayLine(x1, y1, x2, y2);
	}
}

/*
Boolean clipTest(Int16 p, Int16 q, double *u1, double *u2) 
{ 
	double r; 
	Boolean retVal = true;

	if (p < 0) { 
		r = (double)q / (double)p; 
		if (r > *u2) {
			retVal = false;
		} else if (r > *u1) {
			*u1 = r; 
		}
	} else if (p > 0) { 
		r = (double)q / (double)p; 
		if (r < *u1) {
			retVal = false;
		} else if (r < *u2) { 
			*u2 = r; 
		}
	} else if (q < 0) { 
		retVal = false;
	}
	return(retVal);
} 

void WinDrawClippedLine(Int16 x1, Int16 y1, Int16 x2, Int16 y2, Int8 lineType) 
{
	Int16 dx = x2 - x1;
	Int16 dy;
	double u1 = 0.0;
	double u2 = 1.0;
	if (clipTest(-dx, x1-WIDTH_MIN, &u1, &u2)) {
		if (clipTest(dx, SCREEN.WIDTH-x1, &u1, &u2)) {
			dy = y2 - y1;
			if (clipTest(-dy, y1-HEIGHT_MIN, &u1, &u2)) {
				if (clipTest(dy, SCREEN.HEIGHT*((device.DIACapable && !data.config.btmlabels)?1.5:1)-y1, &u1, &u2)) {
					if (u2 < 1.0) {
						x2 = x1 + (Int16)(u2 * dx); 
						y2 = y1 + (Int16)(u2 * dy); 
					}
					if (u1 > 0.0) {
						x1 += (Int16)(u1 * dx);
						y1 += (Int16)(u1 * dy); 
					}
					if (lineType == SOLID) {
						WinDrawLine(x1, y1, x2, y2);
//						BresLine(x1, y1, x2, y2,0);
					} else {
						WinDrawGrayLine(x1, y1, x2, y2); 
					}
					
//					HostTraceOutputTL(appErrorClass, "WDCL-LB Clipping");
//					HostTraceOutputTL(appErrorClass, "     WDCL-x1:|%hd|  y1:|%hd|", x1, y1);
//					HostTraceOutputTL(appErrorClass, "     WDCL-x2:|%hd|  y2:|%hd|", x2, y2);
//					HostTraceOutputTL(appErrorClass, "------------------------------------------");
				}
			}
		}
	}
}
*/
//---------------------------------------------------------------------------

double GetMapScale(Int8 mapscaleidx)
{
	double retval = 2.0;

	if (mapscaleidx <= MAXMAPSCALEIDX && mapscaleidx >= 0) {
		retval = mapscales[mapscaleidx];
	}

	return(retval);
}

Int8 FindNextMapScale(double *mapscale, Boolean down)
{
	Int8 x=0;
	Int8 scaleidx= 0;
	double scale;

	scale = *mapscale;
	if (scale > (double)mapscales[MAXMAPSCALEIDX]) scale = mapscales[MAXMAPSCALEIDX];
	for (x=0; x< MAXMAPSCALEIDX+1; x++) {
		if(scale <= (double)mapscales[x]) {
			scaleidx = x;
			break;
		}
	}

	if (down) {
		if (scaleidx == 0) {
			scale = mapscales[0];
		} else {
			scaleidx--;
			scale = mapscales[scaleidx];
		}
	} else {
		if (scaleidx == MAXMAPSCALEIDX) {
			scale = mapscales[MAXMAPSCALEIDX];
		} else {//if (scale == (double)mapscales[scaleidx]) {
			scaleidx++;
			scale = mapscales[scaleidx];
		}
//		 else {
//			scale = mapscales[scaleidx];
//		}
	}
	*mapscale = scale;
	return(scaleidx);
}

UInt32 GetMapDownsampleScale(Int8 mapscaleidx)
{
	UInt32 retval = 2;

	if (mapscaleidx <= MAXMAPSCALEIDX && mapscaleidx >= 0) {
		retval = trkplotds[mapscaleidx];
	}

	return(retval);
}

//============================================================================
// Fills the intermediate points along a line between the two given endpoints
// using Bresenham's line drawing algorithm. NOTE: this routine does no clipping
// so the coordinate values must be within the FrameBuffer bounds.
// NOTE: code may be slightly longer since efficiency was the most important
// consideration. Care was taken to ensure only integer additions, subtractions,
// and bit-shifts, and comparisons to zero. Inner loop was replicated to
// handle X and Y as independent variables separately with only one comparison
// of overhead that is outside the loop.
//============================================================================
void BresLine(Int16 Ax, Int16 Ay, Int16 Bx, Int16 By, Int8 width)
{
	//------------------------------------------------------------------------
	// INITIALIZE THE COMPONENTS OF THE ALGORITHM THAT ARE NOT AFFECTED BY THE
	// SLOPE OR DIRECTION OF THE LINE
	//------------------------------------------------------------------------
	Int16 dX = abs(Bx-Ax);// store the change in X and Y of the line endpoints
	Int16 dY = abs(By-Ay);

	Int16 CurrentX = Ax;// store the starting point (just point A)
	Int16 CurrentY = Ay;

//	Boolean drawpixel=true;

	//------------------------------------------------------------------------
	// DETERMINE "DIRECTIONS" TO INCREMENT X AND Y (REGARDLESS OF DECISION)
	//------------------------------------------------------------------------
	Int16 Xincr, Yincr;
	if (Ax > Bx) { Xincr=-1; } else { Xincr=1; }// which direction in X?
	if (Ay > By) { Yincr=-1; } else { Yincr=1; }// which direction in Y?

	//------------------------------------------------------------------------
	// DETERMINE INDEPENDENT VARIABLE (ONE THAT ALWAYS INCREMENTS BY 1 (OR -1) )
	// AND INITIATE APPROPRIATE LINE DRAWING ROUTINE (BASED ON FIRST OCTANT
	// ALWAYS). THE X AND Y'S MAY BE FLIPPED IF Y IS THE INDEPENDENT VARIABLE.
	//------------------------------------------------------------------------
	if (dX >= dY)// if X is the independent variable
	{           
		Int16 dPr = dY<<1;   // amount to increment decision if right is chosen (always)
		Int16 dPru = dPr - (dX<<1);// amount to increment decision if up is chosen
		Int16 P = dPr - dX;// decision variable start value

		for (; dX>=0; dX--)// process each point in the line one at a time (just use dX)
		{
			DrawPixel(CurrentX, CurrentY);// plot the pixel
			if (P > 0)                              // is the pixel going right AND up?
			{ 
				if (width > 0) {
					DrawPixel(CurrentX+width, CurrentY);// plot the pixel
					DrawPixel(CurrentX-width, CurrentY);// plot the pixel
				}
				CurrentX+=Xincr;// increment independent variable
				CurrentY+=Yincr; // increment dependent variable
				P+=dPru;// increment decision (for up)
			}
			else// is the pixel just going right?
			{
				if (width > 0) {
					DrawPixel(CurrentX, CurrentY-width);// plot the pixel
					DrawPixel(CurrentX, CurrentY+width);// plot the pixel
				}
				CurrentX+=Xincr;// increment independent variable
				P+=dPr;// increment decision (for right)
			}
		}
	}
	else// if Y is the independent variable
	{
		Int16 dPr = dX<<1;   // amount to increment decision if right is chosen (always)
		Int16 dPru = dPr - (dY<<1);    // amount to increment decision if up is chosen
		Int16 P = dPr - dY;// decision variable start value

		for (; dY>=0; dY--)// process each point in the line one at a time (just use dY)
		{
			DrawPixel(CurrentX, CurrentY);// plot the pixel
			if (P > 0)                              // is the pixel going up AND right?
			{
				if (width > 0) {
					DrawPixel(CurrentX+width, CurrentY);// plot the pixel
					DrawPixel(CurrentX-width, CurrentY);// plot the pixel
				}
				CurrentX+=Xincr; // increment dependent variable
				CurrentY+=Yincr;// increment independent variable
				P+=dPru;// increment decision (for up)
			}
			else// is the pixel just going up?
			{
				if (width > 0) {
					DrawPixel(CurrentX+width, CurrentY);// plot the pixel
					DrawPixel(CurrentX-width, CurrentY);// plot the pixel
				}
				CurrentY+=Yincr;// increment independent variable
				P+=dPr;// increment decision (for right)
			}
		}
	}
}

/*
void FindMirrorPointRight(Int16 startX, Int16 startY, Int16 curX, Int16 curY, 
				Int16 *mirrorX, Int16 *mirrorY, Int16 mirrormult)
{
	Int16 diffX, diffY;
	Int16 tempdiff;
	Int16 sector=0;
	Int16  i;

	
	*mirrorX = curX;
	*mirrorY = curY;

	for (i=0; i<mirrormult; i++) {
//		HostTraceOutputTL(appErrorClass, "FMPR-Input start X-|%hd| Y-|%hd|", startX, startY);
//		HostTraceOutputTL(appErrorClass, "FMPR-Input cur   X-|%hd| Y-|%hd|", *mirrorX, *mirrorY);
		// Find difference between start and current X & Y values
		diffX = startX - *mirrorX;
		diffY = startY - *mirrorY;
//		HostTraceOutputTL(appErrorClass, "FMPR-Input diff  X-|%hd| Y-|%hd|", diffX, diffY);
		// Find difference between start and current X & Y values
		// Determine current sector
		// If it is the same point, just return cur X & y in mirror X & Y
		if (diffX == 0 && diffY == 0) {
			// mirrorX and mirrorY already contain the correct value, just return;
			return;
		} else if (diffX == 0) {
			sector = -1; // -1 means current point is on the Y axis
		} else if (diffY == 0) {
			sector = -2; // -2 means current point is on the X axis
		} else if (diffX > 0 && diffY > 0) {
			sector = 1;
		} else if (diffX < 0 && diffY < 0) {
			sector = 3;
		} else if (diffX < 0) {
			sector = 4;
		} else {
			sector = 2;
		}

		// Find new mirror point based on current sector & number of sectors to mirror through
		switch (sector) {
			case -1:
				tempdiff = diffX;
				diffX = diffY;
				diffY = tempdiff;
				break;
			case -2:
				tempdiff = diffX;
				diffX = diffY;
				diffY = tempdiff;
				diffY *= -1;
				break;
			case 1:
			case 3:
				diffY *= -1;
				break;
			case 2:
			case 4:
				diffX *= -1;
				break;
		}

		*mirrorX = startX + diffX;
		*mirrorY = startY + diffY;
//		HostTraceOutputTL(appErrorClass, "FMPR-Output mirror X-|%hd| Y-|%hd|", *mirrorX, *mirrorY);
//		HostTraceOutputTL(appErrorClass, "FMPR--------------------------------------");
	}
//	HostTraceOutputTL(appErrorClass, "FMPR======================================");

	return;
}
*/

void getfirstloggedpoint(Int16 selectedFltindex)
{
	FlightData *fltdata;
	LoggerData *logdata;
	MemHandle output_hand;
	MemPtr output_ptr;

	// allocate memory
	AllocMem((void *)&fltdata, sizeof(FlightData));
	AllocMem((void *)&logdata, sizeof(LoggerData));

	// get flight record
	OpenDBQueryRecord(flight_db, selectedFltindex, &output_hand, &output_ptr);
	MemMove(fltdata,output_ptr,sizeof(FlightData));
	MemHandleUnlock(output_hand);
				
	// read first logged point
	OpenDBQueryRecord(logger_db, fltdata->startidx, &output_hand, &output_ptr);
	MemMove(logdata, output_ptr, sizeof(LoggerData));
	MemHandleUnlock(output_hand);

	// set map centre to lat/lon of first logged point
	data.input.gpslatdbl = DegMinStringToLatLon(logdata->gpslat, *logdata->gpslatdir);
	data.input.gpslngdbl = DegMinStringToLatLon(logdata->gpslng, *logdata->gpslngdir);

	// free memory
	FreeMem((void *)&logdata);
	FreeMem((void *)&fltdata);
}

void updateposition(double deltalat, double deltalng)
{
	Char    tempchar[20];
	Char    nsew[2];

	data.input.gpslngdbl += deltalng;
	StrPrintF(nsew, "%c", LLToStringDM(data.input.gpslngdbl, tempchar, ISLON, false, true, 3));
	tempchar[StrLen(tempchar)-1] = '\0';
	StrCopy(data.logger.gpslng, tempchar);
	StrCopy(data.logger.gpslngdir, nsew);
	data.input.gpslngdbl = DegMinStringToLatLon(data.logger.gpslng, data.logger.gpslngdir[0]);

	data.input.gpslatdbl += deltalat;
	StrPrintF(nsew, "%c", LLToStringDM(data.input.gpslatdbl, tempchar, ISLAT, false, true, 3));
	tempchar[StrLen(tempchar)-1] = '\0';
	StrCopy(data.logger.gpslat, tempchar);
	StrCopy(data.logger.gpslatdir, nsew);
	data.input.gpslatdbl = DegMinStringToLatLon(data.logger.gpslat, data.logger.gpslatdir[0]);
	data.input.coslat = cos(DegreesToRadians(data.input.gpslatdbl));
}

 /**[OPTIONAL]:Returns the color index for given r,g,b values
 *
 *
 * @param  r,g,b values for the color
 * @return index for the color in the color table
 *
 * <br>
 * <br><b>NOTE</b>: To get colors that are close to what you wish it to be, you can
 * download any color utility that shows the R,G,B components for a color
 * and pass the values to this function. You can use the return values
 * in the Palm color functions.
*/
IndexedColorType RGB(UInt8 r, UInt8 g, UInt8 b)
{
	RGBColorType rgbt;
	rgbt.r = r;
	rgbt.g = g;
	rgbt.b = b;
	rgbt.index = 0;
	return WinRGBToIndex(&rgbt);
}

void DrawThermalProfile()
{
	Int16 j, thprofmax, thprofmin, thprofpos;
	Int16 vpos, hpos;
	double thprofscale;
	RectangleType rectP;
	Char tempchar[5];
	double boldline = 1.0;

	// Draw thermal strength with height profile
	// clear area
	RctSetRectangle(&rectP, WIDTH_MIN,HEIGHT_MIN+SCREEN.SRES*25, SCREEN.SRES*THERMALPROFILEWIDTH, SCREEN.SRES*(THERMALPROFILEHEIGHT+9));
	WinEraseRectangle(&rectP, 0);
	// Horiz scale numbers
	FntSetFont((FontID)WAYSYMB11);
	tempchar[0] = 36;
	tempchar[1] = 0;
	vpos = HEIGHT_MIN+(SCREEN.SRES*25);
	WinDrawChars(tempchar,1, WIDTH_MIN,vpos);
	if (data.config.lftunits == METRIC) {
		if (data.config.mcrngmult == 1) {
			// 2.5
			tempchar[0] = 52;
		} else {
			// 5
			tempchar[0] = 37;
		}
	} else {
		// 5 or 10
		tempchar[0] = 36+data.config.mcrngmult;
	}
	if (data.config.lftunits==METRIC) {
		WinDrawChars(tempchar,1, WIDTH_MIN+SCREEN.SRES*THERMALPROFILEWIDTH-(data.config.mcrngmult==2?5:9),vpos);
	} else {
		WinDrawChars(tempchar,1, WIDTH_MIN+SCREEN.SRES*THERMALPROFILEWIDTH-(data.config.mcrngmult==2?9:5),vpos);
	}
	// current altitude markers
	vpos = HEIGHT_MIN+(SCREEN.SRES*(28+THERMALPROFILEHEIGHT/2));
	//tempchar[0] = 48;
	//WinDrawChars(tempchar,1, 1,vpos);
	tempchar[0] = 49;
	WinDrawChars(tempchar,1, WIDTH_MIN+SCREEN.SRES*(THERMALPROFILEWIDTH-9),vpos);
	FntSetFont(stdFont);
	// axis lines
	hpos = HEIGHT_MIN+(SCREEN.SRES*32);
	vpos = HEIGHT_MIN+(SCREEN.SRES*(34+THERMALPROFILEHEIGHT));
	WinDrawLine(WIDTH_MIN,hpos, WIDTH_MIN,vpos);
	WinDrawLine(WIDTH_MIN,hpos, WIDTH_MIN+SCREEN.SRES*THERMALPROFILEWIDTH,hpos);
	WinDrawLine(WIDTH_MIN,vpos, WIDTH_MIN+SCREEN.SRES*THERMALPROFILEWIDTH,vpos);
	// draw profile
	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(kCoordinatesStandard);
		boldline = SCREEN.SRES;
	}
	if (device.colorCapable) {
		WinSetForeColor(indexGrey);
	} 
	thprofscale = data.input.lftconst*THERMALPROFILEWIDTH/5/MCMult/data.config.mcrngmult;
	thprofpos = Floor(data.input.inusealt / profilestep);
	thprofmin = thprofpos - THERMALPROFILEHEIGHT/2;
	if (thprofmin < profileminalt) thprofmin = profileminalt;
	thprofmax = thprofpos + THERMALPROFILEHEIGHT/2;
	if (thprofmax > profilemaxalt) thprofmax = profilemaxalt;
	for (j=thprofmin; j<=thprofmax; j++) {
		if (windprofile[j].direction >= 0.0) {
			vpos = (HEIGHT_MIN+(SCREEN.SRES*(33+THERMALPROFILEHEIGHT/2+(thprofpos-j))))/boldline;
			hpos = WIDTH_MIN+SCREEN.SRES*(Int16)(windprofile[j].lift*thprofscale);
			WinDrawLine(WIDTH_MIN, vpos, hpos/boldline, vpos);
		}
	}
	// set as bold
	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(kCoordinatesStandard);
		boldline = SCREEN.SRES;
	}
	if (device.colorCapable) {
		WinSetForeColor(indexBlack);
	}
	// draw MC value
	hpos = WIDTH_MIN+SCREEN.SRES*(Int16)(MCCurVal*thprofscale);
	if (hpos > SCREEN.SRES*THERMALPROFILEWIDTH) hpos = SCREEN.SRES*THERMALPROFILEWIDTH; 
	WinDrawClippedLine(hpos/boldline, (HEIGHT_MIN+(SCREEN.SRES*32))/boldline, hpos/boldline, (HEIGHT_MIN+(SCREEN.SRES*(34+THERMALPROFILEHEIGHT)))/boldline, DASHED);
	// draw TLft value
	hpos = WIDTH_MIN+SCREEN.SRES*(Int16)(data.input.thavglift*thprofscale);
	if (hpos > SCREEN.SRES*THERMALPROFILEWIDTH) hpos = SCREEN.SRES*THERMALPROFILEWIDTH; 
	WinDrawClippedLine(hpos/boldline, (HEIGHT_MIN+(SCREEN.SRES*32))/boldline, hpos/boldline, (HEIGHT_MIN+(SCREEN.SRES*(34+THERMALPROFILEHEIGHT)))/boldline, SOLID);
	// restore settings
	if (device.HiDensityScrPresent) {
		WinSetCoordinateSystem(device.HiDensityScrPresent);
		boldline = 1.0;
	}
	// draw current altitude line
	vpos = HEIGHT_MIN+(SCREEN.SRES*(33+THERMALPROFILEHEIGHT/2));
	WinDrawClippedLine(WIDTH_MIN, vpos, WIDTH_MIN+SCREEN.SRES*(THERMALPROFILEWIDTH-3),vpos , DASHED);

	return;
}

/*****************************************************************************
 * UTM COORD FUNCTIONS
 *****************************************************************************/

/*Reference ellipsoids derived from Peter H. Dana's website-
http://www.utexas.edu/depts/grg/gcraft/notes/datum/elist.html
Department of Geography, University of Texas at Austin
Internet: pdana@mail.utexas.edu
3/22/95

Source
Defense Mapping Agency. 1987b. DMA Technical Report: Supplement to Department of Defense World Geodetic System
1984 Technical Report. Part I and II. Washington, DC: Defense Mapping Agency
*/

void LLtoUTM(Int8 ReferenceEllipsoid, double Lat, double Long, double* UTMNorthing, double* UTMEasting, Char* UTMZone)
{
//converts lat/long to UTM coords.  Equations from USGS Bulletin 1532
//East Longitudes are positive, West longitudes are negative.
//North latitudes are positive, South latitudes are negative
//Lat and Long are in decimal degrees
//Written by Chuck Gantz- chuck.gantz@globalstar.com

	double a = ellipsoid[ReferenceEllipsoid].EquatorialRadius;
	double eccSquared = ellipsoid[ReferenceEllipsoid].eccentricitySquared;
	double k0 = 0.9996;

	double LongOrigin;
	double eccPrimeSquared;
	double N, T, C, A, M;

	//Make sure the longitude is between -180.00 .. 179.9
	double LongTemp = nice_brg(Long);

	double LatRad = Lat*degToRad;
	double LongRad = LongTemp*degToRad;
	double LongOriginRad;
	Int8   ZoneNumber;

	ZoneNumber = (LongTemp + 180)/6 + 1;
	if( Lat >= 56.0 && Lat < 64.0 && LongTemp >= 3.0 && LongTemp < 12.0 )
		ZoneNumber = 32;

	// Special zones for Svalbard
	if( Lat >= 72.0 && Lat < 84.0 )
	{
	  if(      LongTemp >= 0.0  && LongTemp <  9.0 ) ZoneNumber = 31;
	  else if( LongTemp >= 9.0  && LongTemp < 21.0 ) ZoneNumber = 33;
	  else if( LongTemp >= 21.0 && LongTemp < 33.0 ) ZoneNumber = 35;
	  else if( LongTemp >= 33.0 && LongTemp < 42.0 ) ZoneNumber = 37;
	 }
	LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  //+3 puts origin in middle of zone
	LongOriginRad = LongOrigin * degToRad;

	//compute the UTM Zone from the latitude and longitude
	StrPrintF(UTMZone, "%d%c", ZoneNumber, UTMLetterDesignator(Lat));

	eccPrimeSquared = (eccSquared)/(1-eccSquared);

	N = a/sqrt(1-eccSquared*sin(LatRad)*sin(LatRad));
	T = tan(LatRad)*tan(LatRad);
	C = eccPrimeSquared*cos(LatRad)*cos(LatRad);
	A = cos(LatRad)*(LongRad-LongOriginRad);

	M = a*((1 - eccSquared/4 - 3*eccSquared*eccSquared/64 - 5*eccSquared*eccSquared*eccSquared/256)*LatRad
		- (3*eccSquared/8 + 3*eccSquared*eccSquared/32 + 45*eccSquared*eccSquared*eccSquared/1024)*sin(2*LatRad)
		+ (15*eccSquared*eccSquared/256 + 45*eccSquared*eccSquared*eccSquared/1024)*sin(4*LatRad)
		- (35*eccSquared*eccSquared*eccSquared/3072)*sin(6*LatRad));

	*UTMEasting = (double)(k0*N*(A+(1-T+C)*A*A*A/6
			+ (5-18*T+T*T+72*C-58*eccPrimeSquared)*A*A*A*A*A/120)
			+ 500000.0);

	*UTMNorthing = (double)(k0*(M+N*tan(LatRad)*(A*A/2+(5-T+9*C+4*C*C)*A*A*A*A/24
			+ (61-58*T+T*T+600*C-330*eccPrimeSquared)*A*A*A*A*A*A/720)));

	if (Lat < 0) *UTMNorthing += 10000000.0; //10000000 meter offset for southern hemisphere
}

Char UTMLetterDesignator(double Lat)
{
//This routine determines the correct UTM letter designator for the given latitude
//returns 'Z' if latitude is outside the UTM limits of 84N to 80S
//Written by Chuck Gantz- chuck.gantz@globalstar.com

	Char LetterDesignator;

	if((84 >= Lat) && (Lat >= 72)) LetterDesignator = 'X';
	else if((72 > Lat) && (Lat >= 64)) LetterDesignator = 'W';
	else if((64 > Lat) && (Lat >= 56)) LetterDesignator = 'V';
	else if((56 > Lat) && (Lat >= 48)) LetterDesignator = 'U';
	else if((48 > Lat) && (Lat >= 40)) LetterDesignator = 'T';
	else if((40 > Lat) && (Lat >= 32)) LetterDesignator = 'S';
	else if((32 > Lat) && (Lat >= 24)) LetterDesignator = 'R';
	else if((24 > Lat) && (Lat >= 16)) LetterDesignator = 'Q';
	else if((16 > Lat) && (Lat >= 8)) LetterDesignator = 'P';
	else if(( 8 > Lat) && (Lat >= 0)) LetterDesignator = 'N';
	else if(( 0 > Lat) && (Lat >= -8)) LetterDesignator = 'M';
	else if((-8> Lat) && (Lat >= -16)) LetterDesignator = 'L';
	else if((-16 > Lat) && (Lat >= -24)) LetterDesignator = 'K';
	else if((-24 > Lat) && (Lat >= -32)) LetterDesignator = 'J';
	else if((-32 > Lat) && (Lat >= -40)) LetterDesignator = 'H';
	else if((-40 > Lat) && (Lat >= -48)) LetterDesignator = 'G';
	else if((-48 > Lat) && (Lat >= -56)) LetterDesignator = 'F';
	else if((-56 > Lat) && (Lat >= -64)) LetterDesignator = 'E';
	else if((-64 > Lat) && (Lat >= -72)) LetterDesignator = 'D';
	else if((-72 > Lat) && (Lat >= -80)) LetterDesignator = 'C';
	else LetterDesignator = 'Z'; //This is here as an error flag to show that the Latitude is outside the UTM limits

	return LetterDesignator;
}

void UTMtoLL(Int8 ReferenceEllipsoid, double UTMNorthing, double UTMEasting, Char* UTMZone, double* Lat,  double* Long )
{
//converts UTM coords to lat/long.  Equations from USGS Bulletin 1532
//East Longitudes are positive, West longitudes are negative.
//North latitudes are positive, South latitudes are negative
//Lat and Long are in decimal degrees.
//Written by Chuck Gantz- chuck.gantz@globalstar.com

	double k0 = 0.9996;
	double a = ellipsoid[ReferenceEllipsoid].EquatorialRadius;
	double eccSquared = ellipsoid[ReferenceEllipsoid].eccentricitySquared;
	double eccPrimeSquared;
	double e1 = (1-sqrt(1-eccSquared))/(1+sqrt(1-eccSquared));
	double N1, T1, C1, R1, D, M;
	double LongOrigin;
	double mu, phi1, phi1Rad;
	double x, y;
	Int16 ZoneNumber;
	Int16 NorthernHemisphere; //1 for northern hemispher, 0 for southern
	Char ZoneLetter[2];
	
	x = UTMEasting - 500000.0; //remove 500,000 meter offset for longitude
	y = UTMNorthing;

	ZoneNumber = StrAToI(UTMZone); // get zone number

	StrCopy(ZoneLetter, Right(UTMZone, 1)); // get zone l
	ConvertToUpper(ZoneLetter);
	
	if (StrCompare(ZoneLetter, "N") >= 0)
		NorthernHemisphere = 1;//point is in northern hemisphere
	else
	{
		NorthernHemisphere = 0;//point is in southern hemisphere
		y -= 10000000.0;//remove 10,000,000 meter offset used for southern hemisphere
	}

	LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  //+3 puts origin in middle of zone

	eccPrimeSquared = (eccSquared)/(1-eccSquared);

	M = y / k0;
	mu = M/(a*(1-eccSquared/4-3*eccSquared*eccSquared/64-5*eccSquared*eccSquared*eccSquared/256));

	phi1Rad = mu + (3*e1/2-27*e1*e1*e1/32)*sin(2*mu)
		+ (21*e1*e1/16-55*e1*e1*e1*e1/32)*sin(4*mu)
			+(151*e1*e1*e1/96)*sin(6*mu);
	phi1 = phi1Rad*radToDeg;

	N1 = a/sqrt(1-eccSquared*sin(phi1Rad)*sin(phi1Rad));
	T1 = tan(phi1Rad)*tan(phi1Rad);
	C1 = eccPrimeSquared*cos(phi1Rad)*cos(phi1Rad);
	R1 = a*(1-eccSquared)/pow(1-eccSquared*sin(phi1Rad)*sin(phi1Rad), 1.5);
	D = x/(N1*k0);

	*Lat = phi1Rad - (N1*tan(phi1Rad)/R1)*(D*D/2-(5+3*T1+10*C1-4*C1*C1-9*eccPrimeSquared)*D*D*D*D/24
		+(61+90*T1+298*C1+45*T1*T1-252*eccPrimeSquared-3*C1*C1)*D*D*D*D*D*D/720);
	*Lat = *Lat * radToDeg;

	*Long = (D-(1+2*T1+C1)*D*D*D/6+(5-2*C1+28*T1-3*C1*C1+8*eccPrimeSquared+24*T1*T1)
		*D*D*D*D*D/120)/cos(phi1Rad);
	*Long = LongOrigin + *Long * radToDeg;
}

double NewtonRaphson(double initEstimate)
{
	double Estimate = initEstimate;
	double tol = 0.00001;
	double corr;

	double eccSquared = ellipsoid[3].eccentricitySquared;
	double ecc = sqrt(eccSquared);

	double LatOrigin = 46.95240556; //N46d57'8.660"
	double LatOriginRad = LatOrigin*degToRad;

	double c = sqrt(1+((eccSquared * pow(cos(LatOriginRad), 4)) / (1-eccSquared)));

	double equivLatOrgRadPrime = Asin(sin(LatOriginRad) / c);

	//eqn. 1
	double K = log(tan(FOURTHPI + equivLatOrgRadPrime/2))
		-c*(log(tan(FOURTHPI + LatOriginRad/2))
		- ecc/2 * log((1+ecc*sin(LatOriginRad)) / (1-ecc*sin(LatOriginRad))));
	double C = (K - log(tan(FOURTHPI + initEstimate/2)))/c;

	do
	{
		corr = CorrRatio(Estimate, C);
		Estimate = Estimate - corr;
	}
	while (fabs(corr) > tol);

	return Estimate;
}

double CorrRatio(double LatRad, double C)
{
	double eccSquared = ellipsoid[3].eccentricitySquared;
	double ecc = sqrt(eccSquared);
	double corr = (C + log(tan(FOURTHPI + LatRad/2)) - ecc/2 * log((1+ecc*sin(LatRad)) / (1-ecc*sin(LatRad)))) * (((1-eccSquared*sin(LatRad)*sin(LatRad)) * cos(LatRad)) / (1-eccSquared));

	return corr;
}

void LLToStringUTM(double lat, double lon, Char  *coordStr, Int8 type)
{
// converts a lat / lon pair into a UTM format string
	double 	UTMn = 0.0;
	double	UTMe = 0.0;
	Char 	UTMz[5];

	if (!coordStr) return;

        LLtoUTM(UTMWGS84, lat, lon, &UTMn, &UTMe, UTMz);

	coordStr[0] = '\0';
	switch (type) {
	        case NORTHING:
			AppendIntToStr((Int32)pround(UTMn, 0), 7, coordStr);
			//StrCopy(coordStr, DblToStr(UTMn, 0));
			break;
		case EASTING:
			AppendIntToStr((Int32)pround(UTMe, 0), 7, coordStr);
			//StrCopy(coordStr, DblToStr(UTMe, 0));
			break;
		case ZONE:
			StrCopy(coordStr, Left(UTMz,2));
			StrCat(coordStr, " ");
			StrCat(coordStr, Right(UTMz,1));
			break;
		default:
		        break;
	}
	return;
}

/*****************************************************************************
 * REPLACED FUNCTIONS
 *****************************************************************************/

/*****************************************************************************
 * LatLonToBearing 
 *		Returns bearing in True
 *****************************************************************************/
/*
void LatLonToBearing(double	from_lat,
						double	from_lon,
						double	to_lat,
						double	to_lon,
						double   range,
						double	*bearingP)
{
	double deltalon = 0.0;
	double lat1, lon1, lat2, lon2;
	double slat1, clat1, slat2;
	double bearing;

	lat1 = DegreesToRadians (from_lat);
	lon1 = DegreesToRadians (from_lon);
	lat2 = DegreesToRadians (to_lat);
	lon2 = DegreesToRadians (to_lon);

	deltalon = lon1 - lon2;

//	slat1 = sin_121(lat1);
	slat1 = Sin(lat1);
//	clat1 = cos_121(lat1);
	clat1 = Cos(lat1);
//	slat2 = sin_121(lat2);
	slat2 = Sin(lat2);
//	bearing = RadiansToDegrees(acos(((slat2-cos_121(range/NMPR)*slat1)/(sin_121(range/NMPR)*clat1))));
	bearing = RadiansToDegrees(acos(((slat2-Cos(range/NMPR)*slat1)/(Sin(range/NMPR)*clat1))));

	if (lon2 < lon1) bearing = 360.0 - bearing;

	*bearingP = bearing;
	
	return;
}
*/

/*****************************************************************************
 * LatLonToRangeBearing 
 *		Returns bearing in True
 *****************************************************************************/
/*
void LatLonToRangeBearing(double	from_lat,
						double	from_lon,
						double	to_lat,
						double	to_lon,
						double	*rangeP,
						double	*bearingP)
{
	double deltalon = 0.0;
	double lat1, lon1, lat2, lon2;
	double slat1, clat1, slat2;
	double range, bearing;

	lat1 = DegreesToRadians(from_lat);
	lon1 = DegreesToRadians(from_lon);
	lat2 = DegreesToRadians(to_lat);
	lon2 = DegreesToRadians(to_lon);

	deltalon = lon1 - lon2;

//	slat1 = sin_121(lat1);
	slat1 = Sin(lat1);
//	clat1 = cos_121(lat1);
	clat1 = Cos(lat1);
//	slat2 = sin_121(lat2);
	slat2 = Sin(lat2);
//	range = acos((slat1 * slat2 + clat1 * cos_121(lat2) * cos_121(deltalon)));
	range = acos((slat1 * slat2 + clat1 * Cos(lat2) * Cos(deltalon)));
//	bearing = RadiansToDegrees(acos(((slat2-cos_121(range)*slat1)/(sin_121(range)*clat1))));
	bearing = RadiansToDegrees(acos(((slat2-Cos(range)*slat1)/(Sin(range)*clat1))));

	if (lon2 < lon1) {
		bearing = 360.0 - bearing;
	}

	*rangeP = range * NMPR;
	*bearingP = nice_brg(bearing);
	
	return;
}
*/

/*****************************************************************************
 * LatLonToRange 
 *****************************************************************************/
/*
void LatLonToRange(double	from_lat,
						double	from_lon,
						double	to_lat,
						double	to_lon,
						double	*rangeP)
{
	double deltalon = 0.0;
	double lat1, lon1, lat2, lon2;
	double slat1, clat1, slat2;
	double range;

	lat1 = DegreesToRadians(from_lat);
	lon1 = DegreesToRadians(from_lon);
	lat2 = DegreesToRadians(to_lat);
	lon2 = DegreesToRadians(to_lon);

	deltalon = lon1 - lon2;

	if (deltalon == 0.0) {
		range = Fabs(lat2 - lat1);
	} else {
//		slat1 = sin_121(lat1);
		slat1 = Sin(lat1);
//		clat1 = cos_121(lat1);
		clat1 = Cos(lat1);
//		slat2 = sin_121(lat2);
		slat2 = Sin(lat2);
//		range = acos((slat1 * slat2 + clat1 * cos_121(lat2) * cos_121(deltalon)));
		range = acos((slat1 * slat2 + clat1 * Cos(lat2) * Cos(deltalon)));
	} 

	*rangeP = range * NMPR;
	
	return;
}
*/

