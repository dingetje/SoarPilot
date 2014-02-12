#ifndef SOARWIND_H
#define SOARWIND_H

#define SMAP2 __attribute__ ((section ("smap2")))

#define LEFT 0
#define RIGHT 1

#define CRUISE 0
#define THERMAL 1

#define MAXTHNUM 6
#define MINTHNUM 0
#define MAXTURNIDX 10
#define MINTURNIDX 0
#define BEGINTHIDX 5

#define MAXWINDIDX 32

#define THZOOMOFF 0
#define THZOOMVAR 1
#define THZOOMFIX 2

#define THZOOMSCALELO 0.5
#define THZOOMSCALEHI 0.2

#define WINDPROFILESCALES 10
#define WINDPROFILEPOINTS 600 // 15000m or 50000ft profile depth
#define WINDPROFILECONST 4	  // which number to keep constant when scaling

#define WINDCALCSP	0
#define WINDCALCCOMP	1

#define THERMALSTOAVG	5
#define THERMALPROFILEDIVISOR 2
#define THERMALPROFILEWIDTH 20
#define THERMALPROFILEHEIGHT 50

typedef struct WindProfileType {
	double direction;
	double speed;
	double lift;
	UInt32 timestamp;
} WindProfileType;

typedef struct ThermalData {
	double avglift;
	double startalt;
	double endalt;
	UInt32 starttime;
	UInt32 endtime;
	Int16 turns;
} ThermalData;

/*****************************************************************************
 * protos
 *****************************************************************************/
Boolean CalcWind(double curdir, double curspd, double *awnddir, double *awndspd, Boolean usecompvalues, Boolean reset) SMAP2;
void CalcHWind(double curdir, double wnddir, double wndspd, double *hdwnd) SMAP2;
void DrawWind(Int16 centerX, Int16 centerY, double xrat, double yrat, double scale) SMAP2;
void Draw3DWind(double bearing, double speed, Int16 centerX, Int16 centerY, Int8 maporient) SMAP2;
void CalcWindProfile(Boolean reset, Boolean clear) SMAP2;
Boolean form_thermal_history_event_handler(EventPtr event) SMAP2;
double glAngleAverage(double average, double value, int samples) SMAP2;

#endif
