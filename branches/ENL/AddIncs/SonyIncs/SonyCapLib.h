/******************************************************************************
 *                                                                            *
 *                 (C) Copyright 2001, Sony Corporation                       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *    <IDENTIFICATION>                                                        *
 *       file name    : $Workfile: SonyCapLib.h $
 *                                                                            *
 *    <PROFILE>                                                               *
 *       Capture library prototype                                            *
 *                                                                            *
 *                                                                            *
 *    <HISTORY>                                                               *
 *       Started on   : 01/09/11                                              *
 *       Last Modified: $Date: 2003/01/06 23:02:02 $
 *                                                                            *
 ******************************************************************************/
#ifndef __CAP_LIB_H__
#define __CAP_LIB_H__

#include <SonyErrorBase.h>

/****************************************************************************
 * If we're actually compiling the library code, then we need to
 * eliminate the trap glue that would otherwise be generated from
 * this header file in order to prevent compiler errors in CW Pro 2.
 ***************************************************************************/
#ifdef BUILDING_CAP_LIB
        #define CAP_LIB_TRAP(trapNum)
#else
        #define CAP_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif

/***************************************************************************
 * version number
 ***************************************************************************/
#define capLibAPIVersion	(1)

/***************************************************************************
 * Error codes
 ***************************************************************************/
#define capLibErrorBase					(0x00)
#define capLibErrorClass				(sonyCapErrorClass | capLibErrorBase)

#define capLibErrNone					(errNone)
#define capLibErrBadParam				(capLibErrorClass | 1)  // invalid parameter
#define capLibErrNotOpen				(capLibErrorClass | 2)  // library is not open
#define capLibErrStillOpen				(capLibErrorClass | 3)  // library still in used
#define capLibErrNoMemory				(capLibErrorClass | 4)
#define capLibErrNotSupported			(capLibErrorClass | 5)
#define capLibErrDrvNotFound			(capLibErrorClass | 6)	// driver is not found
#define capLibErrDevAlreadyOpened	(capLibErrorClass | 7)	// device has already been opened

/***************************************************************************
 * Misc definition
 ***************************************************************************/
/* for CapDevInfoType */
#define capDevInfoStringMaxLen (31)

/***************************************************************************
 * Structure
 ***************************************************************************/
typedef enum {
	cameraClass = 0
} CapDevClass;

typedef struct CapDevInfoTag {
	UInt32 devPortID;
	CapDevClass devClass;
	Char 			manufactureStr[capDevInfoStringMaxLen + 1];
	Char 			productStr[capDevInfoStringMaxLen + 1];
	Char 			deviceUniquieIDStr[capDevInfoStringMaxLen + 1];
} CapDevInfoType;

typedef enum {
	capParamIndDefault = 0,
	capParamIndCurrent
} CapParamIndicator;

typedef enum {
	capExposureDirect = 0,
	capExposureAuto
} CapExposureEnum;

typedef struct CapExposureTag {
/* direct parameter for exposure */
#define	capExposureM2	(-2)
#define	capExposureM1	(-1)
#define	capExposure00	(0)
#define	capExposureP1	(1)
#define	capExposureP2	(2)
	CapExposureEnum	exposureEnum;
	Int16					exposure;
} CapExposureType;


typedef enum {
	capWBDirect = 0,
	capWBAuto,
	capWBIndoor,
	capWBOutdoor,
	capWBUnderLight
} CapWBEnum;

typedef struct CapWBTag {
	CapWBEnum	wbEnum;
	Int16			wb;
} CapWBType;

typedef enum {
	capFocusDirect = 0,
	capFocusAuto
}	CapFocusEnum;

typedef struct CapFocusTag {
	CapFocusEnum	focusEnum;
	Int16				focus;
} CapFocusType;

typedef enum {
	capInputModeDirect = 0,
	capInputModeColor,
	capInputModeBlackWhite,
	capInputModeNegative,
	capInputModeSepia,
	capInputModeSolarization
} CapInputModeEnum;

typedef struct CapInputModeTag {
	CapInputModeEnum	inputModeEnum;
	Int16					inputMode;
} CapInputModeType;

typedef enum {
	capZoomDirect = 0
} CapZoomEnum;

typedef struct {
	CapZoomEnum 	zoomEnum;
	Int16				zoom;
} CapZoomType;

typedef enum {
	w80xh60 = 0,
	w160xh120,
	w176xh144,
	QCIF = w176xh144,
	w320xh240,
	QVGAsize = w320xh240,
	w352xh288,
	CIFsize = w352xh288,
	w640xh480,
	VGAsize = w640xh480
} CapFrameSize;

typedef enum {
/*	blackWhite = 0, */
/*	grayScale2bit, */
/*	grayScale4bit, */
/*	color8bit, */
	color16bit = 4
} CapCaptureFormat;

typedef struct {
	UInt16 power:1;			/* 0:off, 1:on */
	UInt16 initialize:1;
	UInt16 preview:1;
	UInt16 captureReady:1;
	UInt16 capturing:1;
	UInt16 rsvFlag:3;
	UInt16 mirror:1;			/* 0:normal, 1:mirror */
	UInt16 rsvFlag2:7;
} CapDevStatusType, *CapDevStatusPtr;

/***************************************************************************
 *   Capture library function trap ID's. Each library call gets a trap number:
 *   capLibTrapXXXX which serves as an index into the library's dispatch
 *   table. The constant sysLibTrapCustom is the first available trap number
 *   after the system predefined library traps Open,Close,Sleep & Wake.
 *
 * WARNING!!! The order of these traps MUST match the order of the dispatch
 *  table in LibDispatch.c!!!
 ****************************************************************************/
typedef enum {
	capLibTrapGetLibAPIVersion = sysLibTrapCustom,

	capLibTrapInit,						//capLibInit,
	capLibTrapDevSelect,					//CapLibDevSelect  ,
	capLibTrapDevOpen,					//CapLibDevOpen  ,
	capLibTrapDevClose,					//CapLibDevClose ,
	capLibTrapDevPowerOn,				//CapLibDevPowerOn ,
	capLibTrapDevPowerOff,				//CapLibDevPowerOff ,
	capLibTrapSetExposure,				//CapLibSetExposure,
	capLibTrapSetWB,						//CapLibSetWB,
	capLibTrapSetFocus,					//CapLibSetFocus,
	capLibTrapSetInputMode,				//CapLibSetInputMode,
	capLibTrapSetZoom,					//CapLibSetZoom   ,
	capLibTrapSetFrame,					//CapLibSetFrame,
	capLibTrapSetPreviewArea,			//CapLibSetPreviewArea,
	capLibTrapSetCaptureArea,			//CapLibSetCaptureArea ,
	capLibTrapSetCaptureFormat,		//CapLibSetCaptureFormat,
	capLibTrapPreviewStart,    		//CapLibPreviewStart,
	capLibTrapPreviewStop,				//CapLibPreviewStop,
	capLibTrapCaptureImage,				//CapLibCaptureImage,
	capLibTrapGetExposure,				//CapLibGetExposure,
	capLibTrapGetWB,						//CapLibGetWB,
	capLibTrapGetFocus,					//CapLibGetFocus,
	capLibTrapGetInputMode,				//CapLibGetInputMode,
	capLibTrapGetZoom,					//CapLibGetZoom,
	capLibTrapGetCaptureFormat,		//CapLibGetCaptureFormat,
	capLibTrapDevGetStatus,				//CapLibDevGetStatus,
	capLibTrapLast							//capLibTrapLast
} capLibTrapNumberEnum;					//capLibTrapNumberEnum;


/********************************************************************
 *              Library API Prototypes
 ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * Standard library open, close, sleep and wake functions
 *-------------------------------------------------------------------------*/


/* open the library */
extern Err CapLibOpen(UInt16 capLibRefNum)
					CAP_LIB_TRAP(sysLibTrapOpen);

/* close the library */
extern Err CapLibClose(UInt16 capLibRefNum)
					CAP_LIB_TRAP(sysLibTrapClose);

/* library sleep */
extern Err CapLibSleep(UInt16 capLibRefNum)
					CAP_LIB_TRAP(sysLibTrapSleep);

/* library wakeup */
extern Err CapLibWake(UInt16 capLibRefNum)
					CAP_LIB_TRAP(sysLibTrapWake);

/*--------------------------------------------------------------------------
 * Custom library API functions
 *--------------------------------------------------------------------------*/

/* Get our library API version */
extern UInt32 CapLibGetLibAPIVersion(UInt16 capLibRefNum)
					CAP_LIB_TRAP(capLibTrapGetLibAPIVersion);

extern Err CapLibInit(UInt16 capLibRefNum)
					CAP_LIB_TRAP(capLibTrapInit);

extern Err CapLibDevSelect (UInt16 capLibRefNum, CapDevClass devType, UInt32 reserved, CapDevInfoType *capDevInfoP)
					CAP_LIB_TRAP(capLibTrapDevSelect);

extern Err CapLibDevOpen (UInt16 capLibRefNum, UInt32 devPortID)
					CAP_LIB_TRAP(capLibTrapDevOpen);

extern Err CapLibDevClose (UInt16 capLibRefNum, UInt32 devPortID)
					CAP_LIB_TRAP(capLibTrapDevClose);

extern Err CapLibDevPowerOn (UInt16 capLibRefNum, UInt32 devPortID)
					CAP_LIB_TRAP(capLibTrapDevPowerOn);

extern Err CapLibDevPowerOff (UInt16 capLibRefNum, UInt32 devPortID)
					CAP_LIB_TRAP(capLibTrapDevPowerOff);
				
extern Err CapLibSetExposure (UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapExposureType *exposure)
					CAP_LIB_TRAP(capLibTrapSetExposure);

extern Err CapLibSetWB (UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapWBType *wb)
					CAP_LIB_TRAP(capLibTrapSetWB);
				
extern Err CapLibSetFocus (UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapFocusType *focus)
					CAP_LIB_TRAP(capLibTrapSetFocus);
					
extern Err CapLibSetInputMode (UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapInputModeType *mode)
					CAP_LIB_TRAP(capLibTrapSetInputMode);
					
extern Err CapLibSetZoom (UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapZoomType *zoom)
					CAP_LIB_TRAP(capLibTrapSetZoom);
					
extern Err CapLibSetFrame (UInt16 capLibRefNum, UInt32 devPortID, PointType topLeft, CapFrameSize size)
					CAP_LIB_TRAP(capLibTrapSetFrame);

extern Err CapLibSetPreviewArea (UInt16 capLibRefNum, UInt32 devPortID, RectangleType *rP)
					CAP_LIB_TRAP(capLibTrapSetPreviewArea);

extern Err CapLibSetCaptureArea (UInt16 capLibRefNum, UInt32 devPortID, RectangleType *rP)
					CAP_LIB_TRAP(capLibTrapSetCaptureArea);
					
extern Err CapLibSetCaptureFormat (UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapCaptureFormat format)
					CAP_LIB_TRAP(capLibTrapSetCaptureFormat);
					
extern Err CapLibPreviewStart (UInt16 capLibRefNum, UInt32 devPortID)
					CAP_LIB_TRAP(capLibTrapPreviewStart);
					
extern Err CapLibPreviewStop (UInt16 capLibRefNum, UInt32 devPortID)
					CAP_LIB_TRAP(capLibTrapPreviewStop);

extern Err CapLibCaptureImage (UInt16 capLibRefNum, UInt32 devPortID, UInt16 *imageP)
					CAP_LIB_TRAP(capLibTrapCaptureImage);

extern Err CapLibGetExposure(UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapExposureType *exposure)
					CAP_LIB_TRAP(capLibTrapGetExposure);

extern Err CapLibGetWB(UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapWBType *wb)
					CAP_LIB_TRAP(capLibTrapGetWB);

extern Err CapLibGetFocus(UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapFocusType *focus)
					CAP_LIB_TRAP(capLibTrapGetFocus);

extern Err CapLibGetInputMode(UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapInputModeType *mode)
					CAP_LIB_TRAP(capLibTrapGetInputMode);

extern Err CapLibGetZoom(UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapZoomType *zoom)
					CAP_LIB_TRAP(capLibTrapGetZoom);

extern Err CapLibGetCaptureFormat (UInt16 capLibRefNum, UInt32 devPortID, CapParamIndicator ind, CapCaptureFormat *format)
					CAP_LIB_TRAP(capLibTrapGetCaptureFormat);

extern Err CapLibGetStatus (UInt16 capLibRefNum, UInt32 devPortID, CapDevStatusPtr status)
					CAP_LIB_TRAP(capLibTrapDevGetStatus);

#ifdef __cplusplus
}
#endif


#endif	// __CAP_LIB_H__
