/******************************************************************************
 *                                                                            *
 *                 (C) Copyright 2001, Sony Corporation                       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *    <IDENTIFICATION>                                                        *
 *       file name    : $Workfile: SonySndLib.h $
 *                                                                            *
 *    <PROFILE>                                                               *
 *       Sony Sound Library function prototype                                *
 *                                                                            *
 *    <HISTORY>                                                               *
 *       Started on   : 01/08/25                                              *
 *       Last Modified: $Date: 2003/01/06 23:02:03 $ 
 *                                                                            *
 ******************************************************************************/

#ifndef _SNDMGRLIB_H_INCLUDED_
#define _SNDMGRLIB_H_INCLUDED_

#include <SonyErrorBase.h>
#include <SonySystemResources.h>
#include <SoundMgr.h>

// BUILDING_APPLICATION
#if BUILDING_APP_OR_LIB_SOUNDLIB
	// direct link to library code
	#define	SonySnd_LIB_TRAP(trapNum)
#else
	// else someone else is including this public header file; use traps
	#define SonySnd_LIB_TRAP(trapNum)	SYS_TRAP(trapNum)
#endif


#define SonySnd_LIB_APIVersion		0x00000001

/****************************************************************************/

#define	NewSoundMgr_FtrID	0x8000
#define	sndPcmNameLength	32

/****************************************************************************
	Sony Sound Library	Error Codes
****************************************************************************/
#define	SonySndErrBase				(sndErrorClass)
#define	errOpenError				sndErrOpen
#define errAlreadyOpen				(SonySndErrBase | 10)
#define errCloseError				(SonySndErrBase | 11)
#define	errAlreadyClose				(SonySndErrBase | 12)

/****************************************************************************
		Struct
****************************************************************************/
typedef struct _tagSndPcmCallbacksType {
	SndCallbackInfoType	completion;
	SndCallbackInfoType	continuous;
	SndCallbackInfoType	blocking;
	SndCallbackInfoType	reserced;
	} SndPcmCallbacksType;

typedef struct _tagSndPcmFormatType {
	UInt16				formatTag;
	UInt16				numOfChan;
	UInt32				samplePerSec;
	UInt16				bitPerSample;
	UInt32				dataSize;
} SndPcmFormatType;
	
typedef struct _tagSndPcmOptionsType {
	UInt16				amplitude;
	UInt16				pan;
	Boolean				interruptible;
	UInt8				reserved;
	UInt32				dwStartMilliSec;
	UInt32				dwEndMilliSec;
} SndPcmOptionsType;

typedef struct _tagSndPcmListItemType {
	Char				name[sndPcmNameLength];
	UInt32				uniqueRecID;
	LocalID				dbID;
	UInt16				cardNo;
} SndPcmListItemType;

typedef struct _tagSndPcmRecHdrType {
	UInt32				signature;
	SndPcmFormatType	format;
	UInt16				numOfSubRec;
	UInt8				bDataOffset;
	UInt8				reserved;
} SndPcmRecHdrType;

typedef struct _tagSndPcmRecType {
	SndPcmRecHdrType	hdr;
	Char				name[1];
} SndPcmRecType;

typedef struct _tagSndPcmChunkInfoType {
	UInt32				uniqueRecID;
	UInt32				subRecSize;
} SndPcmChunkInfoType;

typedef struct _tagSndPcmSubRecHdrType {
	UInt32				signature;
	UInt16				serialNum;
	UInt8				bDataOffset;
	UInt8				reserved;
} SndPcmSubRecHdrType;

typedef struct _tagSndPcmSubRecType {
	SndPcmSubRecHdrType	hdr;
	Char	name[1];
} SndPcmSubRecType;

typedef enum _tagSndPcmCmdEnum {
	sndPcmCmdPlay  = 1,
	sndPcmCmdPlayList,
	sndPcmCmdEndOfList,
	sndPcmCmdDuration
} SndPcmCmdEnum;


/************************************************************************/
/* 																		*/
/************************************************************************/
typedef enum {
	SonySndTrapLibAPIVersion = sysLibTrapCustom,
	SonySndTrapEnableNewSoundMgr,
	SonySndTrapSndPlayPcm,
	SonySndTrapSndPlayPcmResource,
	SonySndTrapSndCreatePcmList,

	SonySndMaxSelector = SonySndTrapSndCreatePcmList
} SonySndLibSelector;



/********************************************************************
 * API Prototypes
 ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
 * 		Custom library API functions
 ************************************************************************/
unsigned long SonySndLibGetVersion(unsigned short refNum)
				SonySnd_LIB_TRAP(SonySndTrapLibAPIVersion);

Err SndPlayPcm(UInt16 refNum, void* chanP, SndPcmCmdEnum cmd, UInt8* pcmP, 
               SndPcmFormatType* formatP, SndPcmOptionsType* selP, 
               SndPcmCallbacksType* callbacksP, Boolean bNoWait )
				SonySnd_LIB_TRAP(SonySndTrapSndPlayPcm);

Err SndPlayPcmResource(UInt16 refNum, UInt32 resType, Int16 resID, 
                       SystemPreferencesChoice volumeSelector, SndPcmFormatType* formatP )
				SonySnd_LIB_TRAP(SonySndTrapSndPlayPcmResource);

Boolean SndCreatePcmList(UInt16 refNum, UInt32 creator, Boolean multipleDbs,
                         UInt16 *wCountP, MemHandle* entHP )
				SonySnd_LIB_TRAP(SonySndTrapSndCreatePcmList);


#ifdef __cplusplus 
}
#endif


#endif 	/* _SNDMGRLIB_H_INCLUDED_ */
