#ifndef SOARBT_H
#define SOARBT_H

#include <BtCommVdrv.h> // AGM: for prefered BT GPS connection feature

#define BT __attribute__ ((section ("bt")))

/*****************************************************************************
 * prototypes
 *****************************************************************************/
Err BT_FindDevice() BT;

#endif

