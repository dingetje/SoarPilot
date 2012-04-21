/******************************************************************************
 *                                                                            *
 *                 (C) Copyright 2002, Sony Corporation                       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *    <IDENTIFICATION>                                                        *
 *       file name    : $Workfile: SonySilkLib.h $
 *                                                                            *
 *    <PROFILE>                                                               *
 *       Audio Remote Controller Lib prototype                                *
 *                                                                            *
 *    <HISTORY>                                                               *
 *       Started on   : 00/11/01                                  		   	*
 *       Last Modified: $Date: 2003/01/06 23:02:03 $ 
 *                                                                            *
 ******************************************************************************/
/* this file is best viewed by setting TAB-stop as 3 */


#ifndef __SILK_LIB_H__
#define __SILK_LIB_H__

#include <SonyErrorBase.h>


#if BUILDING_APP_OR_LIB
	#define SILK_LIB_TRAP(trapNum) 	// direct link to library code
#else
	#define SILK_LIB_TRAP(trapNum)	SYS_TRAP(trapNum)
#endif


/* ------------------------------------- */
/*				Constant def					  */
/* ------------------------------------- */
#define silkLibErrParam					(sonySilkErrorClass | 1)	// invalid parameter
#define silkLibErrNotOpen				(sonySilkErrorClass | 2)	// library is not open
#define silkLibErrStillOpen			(sonySilkErrorClass | 3)	// returned from SampleLibClose() if
																					// the library is still open by others
#define silkLibErrNotAvailable		(sonySilkErrorClass | 4)	// memory error occurred
#define silkLibErrResizeDisabled		(sonySilkErrorClass | 5)	// cannot resize
// up to										(sonySilkErrorClass | 15)

#define hrSilkHeight						(130)
#define hrSilkWidth						(320)
#define stdSilkHeight					(65)
#define stdSilkWidth						(160)

#define hrStatusHeight					(30)
#define hrStatusWidth					(320)
#define stdStatusHeight					(15)
#define stdStatusWidth					(160)

#define silkResizeNormal				(0)
#define silkResizeToStatus				(1)
#define silkResizeMax					(2)

#define silkLibAPIVertion				(0x00000001)

/* ------------------------------------- */
/*				Type def							  */
/* ------------------------------------- */
typedef enum {
	silkLibTrapResizeDispWin = sysLibTrapCustom,
	silkLibTrapEnableResize,
	silkLibTrapDisableResize,
	silkLibTrapGetAPIVersion,
	silkLibLastTrap
} SilkLibTrapNumberEnum;

/* ------------------------------------- */
/*				API Prototypes			 		  */
/* ------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

extern Err SilkLibOpen(UInt16 refNum)
				SILK_LIB_TRAP(sysLibTrapOpen);
				
extern Err SilkLibClose(UInt16 refNum)
				SILK_LIB_TRAP(sysLibTrapClose);

extern Err SilkLibSleep(UInt16 refNum)
				SILK_LIB_TRAP(sysLibTrapSleep);

extern Err SilkLibWake(UInt16 refNum)
				SILK_LIB_TRAP(sysLibTrapWake);

extern Err SilkLibResizeDispWin(UInt16 refNum, UInt8 win)
				SILK_LIB_TRAP(silkLibTrapResizeDispWin);

extern Err SilkLibEnableResize(UInt16 refNum)
				SILK_LIB_TRAP(silkLibTrapEnableResize);

extern Err SilkLibDisableResize(UInt16 refNum)
				SILK_LIB_TRAP(silkLibTrapDisableResize);

extern UInt32 SilkLibGetAPIVersion(UInt16 refNum)
				SILK_LIB_TRAP(silkLibTrapGetAPIVersion);

#ifdef __cplusplus 
}
#endif


#endif	// __SLK_LIB_H__
