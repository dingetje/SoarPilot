#ifndef SOARIGC_H
#define SOARIGC_H

#define SIGC __attribute__ ((section ("igc")))

typedef int t_daytime; /* at least 60*60*24=86400 seconds */
typedef int t_index; /* point counter */ 

typedef struct _point
{
	double	lat,	// latitude
		lon,	// longitude
		alt,	// GPS or pressure altitude
		speed,	// speed
		dist;	// distance
	t_daytime	seconds;	// time stamp
	struct _point *next;
} t_point;

/*****************************************************************************
 * protos
 *****************************************************************************/
void igc_parser(Char* serinp, UInt32 length, Boolean reset) SIGC;

#endif
