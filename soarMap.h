#ifndef SOARMAP_H
#define SOARMAP_H

#define SMAP3 __attribute__ ((section ("smap3")))

#include "soaring.h"

#ifndef	COORDS_AXIS
#define	COORDS_AXIS	((double)6378137.0)
#endif

#ifndef	COORDS_RECF
#define	COORDS_RECF	((double)298.257223563)
#endif

#ifndef	DEG2RAD
#define	DEG2RAD	((double)0.01745329251994)
#endif

#ifndef	RAD2DEG
#define	RAD2DEG	((double)57.29577951308)
#endif

// screen defines for ease and central location
#define  WIDTH_MIN		0
#define  HEIGHT_MIN		0
#define  WIDTH_BASE		160
#define  HEIGHT_BASE		160
#define  HEIGHT_BASE_DIA	240
#define  GLIDERX_BASE		80
#define  GLIDERY_BASE		100
#define  LABEL_HEIGHT           20

// options for right hand field on non-DIA palms
#define MAPFGA		0
#define MAPGLIDE	1
#define MAPLFT		2
#define MAPTLFT		3
#define MAPAVG		4

// invalid lat lon value
#define	INVALID_LAT_LON  999.0

#define ISLAT 1
#define ISLON 0

// Linear Interpolation projections maintain minimal data
typedef struct {
	int	pixWidth;		// width of map in pixels
	int	pixHeight;		// height of map in pixels
	double	scaleWidth;		// width of map in Nautical Miles
	double	scaleHeight;	// height of map in Nautical Miles
	double	ulLat;			// latitude at map upper left corner
	double	ulLon;			// longitude at map upper left corner
	double	lrLat;			// latitude at map lower right corner
	double	lrLon;			// longitude at map lower right corner
	double	deltaLat;		// delta of top & bottom latitiudes - was sp1
	double	deltaLon;		// delta of left & right longitudes - was sp2
} LinearInterp;

/*****************************************************************************
 * protos
 *****************************************************************************/
void UpdateMap2(Boolean cleanup, double gliderLat, double gliderLon, double mapscale, Boolean ismap, TaskData *tsk, Boolean gliderpos) SMAP3;

#endif
