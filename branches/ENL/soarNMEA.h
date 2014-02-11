#ifndef SOARNMEA_H
#define SOARNMEA_H

#define SCMP __attribute__ ((section ("scmp")))
#define SCMP2 __attribute__ ((section ("scmp2")))

/*****************************************************************************
 * defines 
 *****************************************************************************/
#define NORESET 0
#define RESET   1
#define PREFILL 2

/*****************************************************************************
 * prototypes
 *****************************************************************************/
void nmea_parser(Char* serinp, UInt32 length, Boolean reset) SCMP;
Boolean gpsvalidate(Char* gpssentence) SCMP2;
void CalcLift(double curalt, Char *utc, double inputsecs, Int8 command) SCMP2;
void SetFixStatus() SCMP2;

#endif
