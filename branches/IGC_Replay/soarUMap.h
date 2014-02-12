#ifndef SOARUMAP_H
#define SOARUMAP_H

#define SMAP __attribute__ ((section ("smap")))
#define SMAP2 __attribute__ ((section ("smap2")))

#include "soarMap.h"

#define MAXMAPSCALEIDX 19

#define NOFORCE 0
#define FORCE   1
#define FORCEACTUAL 2

//Line Types
#define SOLID 0
#define DASHED 1

//Sector Drawing Types
#define ALLSECTOR 0
#define ENDSONLY  1
#define NOBOTTOM  2
#define NOENDS    3

// UTM Coord
#define NORTHING 0
#define EASTING  1
#define ZONE     2
#define UTMWGS84 23

typedef	struct {
	Int8 id;
	Char ellipsoidName[25];
	double EquatorialRadius;
	double eccentricitySquared;
} Ellipsoid;

typedef struct TrkTrail {
    double lat;
    double lon;
    double lift;
    double speed;
    double height;
    UInt32 time;
} TrkTrail;

/*****************************************************************************
 * protos
 *****************************************************************************/
double MetersToNautical(double meters) SMAP2;
double NauticalToMeters(double miles) SMAP2;
double MetersToFeet(double	meters) SMAP2;
double FeetToMeters(double feet) SMAP2;
double modcrs(double x) SMAP2;
double MidCse(double leftbrg, double rightbrg) SMAP2;
double RelToTrueBrg(double relbrg, double truecse) SMAP2;
double TrueToRelBrg(double oldbearing, double truecse) SMAP2;
Boolean PointOutOfBounds (double ul_lat, double ul_lon, double lr_lat, double lr_lon, double lon, double lat) SMAP2;
Char LLToStringDM(double coord, Char *coordStr, Boolean lat, Boolean colon, Boolean period, Int8 numaddplaces) SMAP2;
Char LLToStringDMS(double coord, Char  *coordStr, Boolean  lat) SMAP2;
double PixelToDist(double scale, double pixelval) SMAP2;
Boolean PixelOnScreen(Int16 plotX, Int16 plotY) SMAP2;
double DegMinSecToLatLon(Int16	deg, Int16 min, double fracMin, Int16 sec, double fracSec, Char	nsew) SMAP2;
double DegMinColonStringToLatLon(Char	*inP) SMAP2;
double DegMinSecColonStringToLatLon(Char* inP) SMAP2;
double DegMinStringToLatLon(Char *latLonstring, Char nsew) SMAP2;
double DegMinSecStringToLatLon(Char *latLonstring, Char nsew) SMAP2;
double DegMinSecStringToLatLon(Char *latLonstring, Char nsew) SMAP2;
void PixelToPositLinearInterp(LinearInterp *liP, Int32 x, Int32 y, double *latP, double *lonP) SMAP2;
void PositToPixelLinearInterp(LinearInterp *liP, double lat, double lon, Int16 *xP, Int16 *yP) SMAP2;
double CoordDeltaLinearInterp(double	coord1, double	coord2) SMAP2;
double MidCoordLinearInterp(double	coord1, double	coord2) SMAP2;
double CoordDeltaModLinearInterp(double	coord1, double	coord2, double	ratio) SMAP2;
void RangeBearingToLatLon(double fromLat, double fromLon, double rng, double truebrg, double *toLat, double *toLon) SMAP2;
void RangeBearingToLatLonLinearInterp(double fromLat, double fromLon, double rng, double brg, double *toLatP, double *toLonP) SMAP2;
void LatLonToRangeBearing(double from_lat, double from_lon, double to_lat, double to_lon, double *rangeP, double *bearingP) SMAP;
void LatLonToRangeBearingEll(double from_lat, double   from_lon, double   to_lat, double   to_lon, double   *rangeP, double   *bearingP) SMAP;
void LatLonToRange(double from_lat, double from_lon, double to_lat, double to_lon, double *rangeP) SMAP;
void LatLonToBearing(double from_lat, double from_lon, double to_lat, double to_lon, double *bearingP) SMAP;
double LatLonToRange2( double from_lat, double from_lon, double to_lat,  double to_lon) SMAP;
double LonDeltaToNM(double lonDelta, double lat) SMAP;
double nice_brg (double brg) SMAP;
double RadiansToDegrees(double value) SMAP;
double DegreesToRadians(double value) SMAP;
double _correct(double value) SMAP;
double XYDistToRng(double xdist, double ydist) SMAP;
double XYDistToBrg(double xdist, double ydist) SMAP;
void DrawGlider(Int8 maporient, Int16 centerx, Int16 centery) SMAP;
void DrawPOI(Int16 pixelx, Int16 pixely, char *label, Int16 offset, Int8 maxlen, Boolean reachable, Boolean erasefirst, 
Int32 type, Boolean forcelabel, double mapscale, Boolean isairport, Boolean islandable, Boolean justreachable) SMAP;
void DrawWayLine(Int16 WayX, Int16 WayY, double gliderLat, double gliderLon, double xratio, double yratio, Boolean ismap) SMAP;
Boolean CalcPlotValues(double origlat, double origlon, double tgtlat, double tgtlon, double xratio, double yratio, Int16 *plotX, Int16 *plotY, double *poirange, double *poibearing, Int8 forcecalc, Int8 maporient, Boolean CalcBearing) SMAP;
void RangeBearingToPixel(double xratio, double yratio, Int16 centerX, Int16 centerY, double plotrange, double plotbearing, Int8 forcecalc, Int8 maporient, Int16 *plotX, Int16 *plotY) SMAP;
void WinDrawSector(double xrat, double yrat, Int16 centerX, Int16 centerY, double innerrad, double outerrad, Int16 leftbrg, Int16 rightbrg, Int8 sectortype) SMAP2;
void WinDrawSectorSimple(double xrat, double yrat, Int16 centerX, Int16 centerY, double radius,  Int16 leftbrg, Int16 rightbrg, Int16 *leftX, Int16 *leftY, Int16 *rightX, Int16 *rightY) SMAP2;
void WinDrawSectorOld(double xrat, double yrat, Int16 centerX, Int16 centerY, double innerrad, double outerrad, Int16 leftbrg, Int16 rightbrg, Int8 sectortype) SMAP2;
void WinDrawSectorNew(double xrat, double yrat, Int16 centerX, Int16 centerY, double innerrad, double outerrad, Int16 leftbrg, Int16 rightbrg, Int8 sectortype) SMAP2;
void DrawTrkTrail(double gliderLat, double gliderLon, double xratio, double yratio, double ulRng) SMAP;
void InitTrkTrail(Boolean allocate) SMAP;
void SaveTrkTrail() SMAP;
double RecipCse(double curcse) SMAP;
void WinDrawCirclePalm(Int32 xc, Int32 yc, Int32 radius, Int8 lineType) SMAP;
void WinDrawCircle(Int32 xc, Int32 yc, Int32 radius, Int8 lineType) SMAP;
void WinDrawEllipse(Int32 xc, Int32 yc, Int32 a0, Int32 b0) SMAP;
void EllipseArc2D (Int16 xc, Int16 yc,  double rad, double fx0, double fy0, double fx1, double fy1, double xrat, double yrat) SMAP2;
void WinDrawClippedLine(Int16 x1, Int16 y1, Int16 x2, Int16 y2, Int8 lineType) SMAP;
double GetMapScale(Int8 mapscaleidx) SMAP2;
Int8 FindNextMapScale(double *mapscale, Boolean down) SMAP2;
UInt32 GetMapDownsampleScale(Int8 mapscaleidx) SMAP2;
void FindMirrorPointRight(Int16 startX, Int16 startY, Int16 curX, Int16 curY, Int16 *mirrorX, Int16 *mirrorY, Int16 mirrormult) SMAP2;
void BresLine(Int16 Ax, Int16 Ay, Int16 Bx, Int16 By, Int8 width) SMAP2;
void getfirstloggedpoint(Int16 selectedFltindex) SMAP2;
void updateposition(double deltalat, double deltalng) SMAP2;
IndexedColorType RGB(UInt8 r, UInt8 g, UInt8 b) SMAP2;
void DrawThermalProfile() SMAP2;

// UTM Coords
void LLtoUTM(Int8 ReferenceEllipsoid, double Lat, double Long, double* UTMNorthing, double* UTMEasting, Char* UTMZone) SMAP;
void UTMtoLL(Int8 ReferenceEllipsoid, double UTMNorthing, double UTMEasting, Char* UTMZone, double* Lat,  double* Long) SMAP;
Char UTMLetterDesignator(double Lat) SMAP;
void LLtoSwissGrid(double Lat, double Long, double* SwissNorthing, double* SwissEasting) SMAP;
void SwissGridtoLL(double SwissNorthing, double SwissEasting, double* Lat, double* Long) SMAP;
double NewtonRaphson(double initEstimate) SMAP;
double CorrRatio(double LatRad, double C) SMAP;
void LLToStringUTM(double lat, double lon, Char  *coordStr, Int8 type) SMAP;

#endif
