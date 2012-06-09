#ifndef SOARBT_H
#define SOARBT_H
/**
* \file soarBT.h
* \brief Bluetooth header file for SoarPilot
*/

#include <BtCommVdrv.h> // AGM: for prefered BT GPS connection feature

#define BT __attribute__ ((section ("bt")))

/*****************************************************************************
 * prototypes
 *****************************************************************************/
Err BT_FindDevice() BT;

#endif

