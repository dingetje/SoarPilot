#ifndef SOARBT_H
#define SOARBT_H
/**
* \file soarBT.h
* \brief SoarPilot Bluetooth header file
*/

#include <BtCommVdrv.h> // AM: for prefered BT GPS connection feature

#define BT __attribute__ ((section ("bt")))

/*****************************************************************************
 * prototypes
 *****************************************************************************/
Err BT_FindDevice() BT;

#endif

