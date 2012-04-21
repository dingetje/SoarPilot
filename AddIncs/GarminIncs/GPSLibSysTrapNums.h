/*********************************************************************
*
*   MODULE NAME:
*       GPSLibSysTrapNums.h - Contains the SYS_TRAP numbers for the 
*           GPSLib library that can be used in both the 68K code that 
*           uses the library, and the ARM shim code inside the 
*           library.
*
*   Copyright 2002 by GARMIN Corporation.
*
*********************************************************************/

#ifndef __GPSLIBSYSTRAPNUMS_H__
#define __GPSLIBSYSTRAPNUMS_H__

#ifndef __MC68K__
//   #include <LibTraps68K.h>
#endif

/********************************************************************
 * Traps
 ********************************************************************/
enum
    {
    gpsLibTrapClose = sysLibTrapCustom,
    gpsLibTrapGetLibAPIVersion,
    gpsLibTrapGetMaxSatellites,
    gpsLibTrapGetPosition,
    gpsLibTrapGetPVT,
    gpsLibTrapGetSatellites,
    gpsLibTrapGetStatus,
    gpsLibTrapGetTime,
    gpsLibTrapGetVelocity,
    gpsLibTrapOpen
    };

#endif  //__GPSLIBSYSTRAPNUMS_H__
