#ifndef SOARWAY_H
#define SOARWAY_H

#define SWAY __attribute__ ((section ("sway")))

#define ARROWCENTX 80.0
#define ARROWCENTY 145.0
#define ARROWCENTXOFF 13.0
#define ARROWCENTYOFF 13.0
#define ARROWAREAWIDTH 26.0

#define WPCAI 		0
#define WPCU  		1
#define WPCUN   	2
#define WPGPWPL		3
#define WPGPWPLALT	4
#define WPOZI		5

// Waypoint Types
#define   AIRPORT   0x0001
#define   TURN      0x0002
#define   LAND      0x0004
#define   START     0x0008
#define   FINISH    0x0010
#define   MARK      0x0020
#define   HOME      0x0040
#define   AIRLAND   0x0080
#define   AREA      0x0100
#define   THRML     0x0200
#define   CONTROL   0x0400
#define   TARGET    0x0800
#define   TERRAIN   0x1000
#define   CRASH     0x2000
#define   REFWPT    0x4000
#define   AREAEXIT  0x8000

#define   MAXPENPT  0x10000

#define REFNONE    0
#define REFBEARING 1
#define REFRADIAL  2

// actions for handling waypoint selection
#define WAYSELECT	0
#define WAYNEXT		1
#define WAYPREV		2
#define WAYADD		3
#define WAYTEMP		4
#define WAYDEACT	5
#define WAYRESUME	6
#define WAYLAND		7
#define WAYGO		8
#define WAYNORM		9

/*****************************************************************************
 * protos
 *****************************************************************************/
void SaveWaypoint(double lat, double lon, double alt, Boolean thrml) SWAY;
void waypoint_parser(Char* serinp, UInt32 length, Boolean reset) SWAY;
void OutputWayHeader(Int8 wpformat) SWAY;
void OutputWayCAI() SWAY;
void OutputWayCU() SWAY;
void OutputWayGPWPL() SWAY;
//void OutputWayGDN() SWAY;
Int16 FindWayptRecordByName(Char* NameString) SWAY;
Int16 FindWayptRecordByNamePartial(Char* NameString) SWAY;
void ClearWayptHOMEStatus() SWAY;
void ClearWayptREFStatus() SWAY;
void calcallwaypoints(double gpslat, double gpslon) SWAY;
void HomeWayptInitialize(WaypointData *waydata) SWAY;
Boolean FindHomeWayptInitialize() SWAY;
Boolean FindRefWayptInitialize() SWAY;
void DrawFGWayLine(Int16 centerX, Int16 centerY) SWAY;
void cuwaypoint_parser(Char* serinp, UInt32 length, Boolean reset) SWAY;
void GPWPLwaypoint_parser(Char* serinp, UInt32 length, Boolean reset) SWAY;
void draw_waypoints(double gliderLat, double gliderLon, double mapscale, double maxdist, Boolean gliderpos) SWAY;
void OutputWayOZI() SWAY;
void OZIwaypoint_parser(Char* serinp, UInt32 length, Boolean reset) SWAY;
void DeleteThermalWaypts() SWAY;

#endif
