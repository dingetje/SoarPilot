/******************************************************************************
 *                                                                            *
 *                 (C) Copyright 2000, Sony Corporation                       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *    <IDENTIFICATION>                                                        *
 *       file name    : $Workfile: SonyMsaLib.h $
 *                                                                            *
 *    <PROFILE>                                                               *
 *       MSA Library function prototype                                                *
 *                                                                            *
 *    <HISTORY>                                                               *
 *       Started on   : 00/10/05                                  		   *
 *       Last Modified: $Date: 2003/01/06 23:02:02 $ 
 *                                                                            *
 ******************************************************************************/

#ifndef __MSA_LIB_H__
#define __MSA_LIB_H__

#include <SonyErrorBase.h>
#include <SonySystemResources.h>

#include <DateTime.h>
//#include "MsaOut.h"
// Palm common definitions

// BUILDING_APPLICATION
#if BUILDING_APP_OR_LIB_MSA
	// direct link to library code
	#define MSA_LIB_TRAP(trapNum) 
#else
	// else someone else is including this public header file; use traps
	#define MSA_LIB_TRAP(trapNum)	SYS_TRAP(trapNum)
#endif


/********************************************************************
 * Type and creator of MSA Library database
 ********************************************************************/
#define msLibDbCreator		sonyLibDbCIdMsalib
#define msLibDbNum		0


#define		msaLibCreatorID	sonySysFileCMsaLib
									// MSA Library database creator
#define		msaLibTypeID		sonySysFileTMsaLib
									// Standard library database type

#define		msaLibAlbumTypeID		'albm'		// Album database type

/********************************************************************
 * Internal library name which can be passed to SysLibFind()
 ********************************************************************/
//#define		msaLibName			"Msa Library"		

#define msalibAPIVertion		0x00000002

/************************************************************
 * Msa Library result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *************************************************************/

#define msaErrParam				(sonyMsaErrorClass | 1)	// invalid parameter
#define msaErrNotOpen			(sonyMsaErrorClass | 2)	// library is not open
#define msaErrStillOpen			(sonyMsaErrorClass | 3)	// returned from MsaLibClose() if
														// the library is still open by others
#define msaErrMemory			(sonyMsaErrorClass | 4)	// memory error occurred
#define msaErrNoVFSMgr			(sonyMsaErrorClass | 5)	// VFS Mgr error
#define msaErrAlreadyOpen		(sonyMsaErrorClass | 6)	// library is already open
#define msaErrNotImplemented	(sonyMsaErrorClass | 7) // API is Unsupported
#define msaErrSecurity			(sonyMsaErrorClass | 8)	// MgSysLib returns errorcd
#define msaErrPBListSet			(sonyMsaErrorClass | 9)	// PBList error
#define msaErrNotShuffleMode	(sonyMsaErrorClass | 10)// ShuffleList error
#define msaErrNoAlbum			(sonyMsaErrorClass | 11)// AlbumDB doesn't exist error
#define msaErrNoMedia			(sonyMsaErrorClass | 12)// Media doesn't exist error
#define msaErrInvalidMedia		(sonyMsaErrorClass | 13)// Valid Media doesn't exist error
#define msaErrDifferentMode		(sonyMsaErrorClass | 14)// Different mode error
#define msaErrEnumerationEmpty	(sonyMsaErrorClass | 15)// AlbumEnumerate error
#define msaErrEnumerationdetail	(sonyMsaErrorClass | 16)// AlbumEnumerate Get albumInfo error
#define msaErrNotConnected		(sonyMsaErrorClass | 17)// Audio device is not connected.

#define msaErrDataReadFail	(sonyMsaErrorClass | 18)// MP3 file read error occurred  
#define msaErrNotEnoughSpace	(sonyMsaErrorClass | 19)// Can't allocate memory for MP3 file   
#define msaErrInvalidFormat	(sonyMsaErrorClass | 20)// MP3 file Invalid format error   
#define msaErrNotMP3File		(sonyMsaErrorClass | 21)// Not MP3 file   

//-----------------------------------------------------------------------------
// Msa library function trap ID's. Each library call gets a trap number:
//   msaLibTrapXXXX which serves as an index into the library's dispatch table.
//   The constant sysLibTrapCustom is the first available trap number after
//   the system predefined library traps Open,Close,Sleep & Wake.
//
// WARNING!!! The order of these traps MUST match the order of the dispatch
//  table in MsaLibDispatch.c!!!
//-----------------------------------------------------------------------------

typedef enum {
	/* MsaIf */
	msaTrapPlay = sysLibTrapCustom,
	msaTrapStop,
	msaTrapGetPBPosition,
	msaTrapGetTrackInfo,
	msaTrapGetPBStatus,
	msaTrapSetPBRate,
	msaTrapSetPBPosition,
	msaTrapGetPBRate,
	msaTrapGetPBMode,
	msaTrapGetPBList,
	msaTrapSetPBList,
	msaTrapSetPBStatus,
	msaTrapSetPBMode,
	msaTrapSuToTime,
	msaTrapTimeToSu,
	msaTrapPBListIndexToTrackNo,
	msaTrapGetShufflePlayedList,
	msaTrapGetAlbum,
	msaTrapSetAlbum,
	msaTrapSetControlKey,
	msaTrapEdit,
	msaTrapGetCapability,
	msaTrapGetAPIVersion,
	msaTrapGetTrackRestrictionInfo,
	/* MsaOut */
	msaTrapOutGetCapability,
	msaTrapOutInit,
	msaTrapOutSetOutputMode,
	msaTrapOutSetVolume,
	msaTrapOutVolumeUp,
	msaTrapOutVolumeDown,
	msaTrapOutSetVolumeLimit,
	msaTrapOutSetMute,
	msaTrapOutSetEQ,
	msaTrapOutSetEQLevel,
	msaTrapOutSetBBLevel,
	msaTrapOutSetBeepLevel,
	msaTrapOutStartBeep,
	msaTrapOutStopBeep,
	msaTrapOutGetVolume,
	msaTrapOutGetVolumeLimit,
	msaTrapOutGetMute,
	msaTrapOutGetEQ,
	msaTrapOutGetEQLevel,
	msaTrapOutGetBBLevel,
	msaTrapOutGetBeepLevel,
	msaTrapOutGetOutputMode,
	msaTrapOutGetInfo,
	msaTrapOutGetLevel,
	msaTrapOutGetSpectrum,
	msaTrapOutGetBeepStatus,
	/* MsaStream */
	msaTrapStreamOpenStream,
	msaTrapStreamCloseStream,
	msaTrapStreamSetFormat,
	msaTrapStreamPut,
	msaTrapStreamSetCallback,
	msaTrapStreamStartPlay,
	msaTrapStreamStopPlay,
	msaTrapStreamFlush,
	msaTrapStreamGetStatus,
	msaTrapStreamGetTime,
	msaTrapStreamSetFeatureValue,
	msaTrapStreamGetFeatureValue,
	msaTrapStreamReservedFunction,
	msaTrapEnforceOpen,
	msaTrapAlbumEnumarate,
	msaLibLastTrap  /* for calculating the number of traps */
} MsaLibTrapNumberEnum;

/********************************************************************
 * Data Structure
 ********************************************************************/
/* MsaUInt64 */
typedef unsigned long long MsaUInt64;

/* MsaLib OpenMode */
#define msaLibOpenModeNormal	(0)
#define msaLibOpenModeAlbum		(msaLibOpenModeNormal)
#define msaLibOpenModeStream	(1)
//#define msaLibOpenModeFile		// This is reserved.

/* Play Speed */
#define msa_PBRATE_SP			0x800c000c
#define msa_PBRATE_SP_x1_5		0x80060009
#define msa_PBRATE_SP_x2		0x8006000c
#define msa_PBRATE_SP_x5		0x8006001e
#define msa_PBRATE_SP_x10		0x8006003c
#define msa_PBRATE_SP_x20		0x80040050
#define msa_PBRATE_SP_x50		0x80020064
#define msa_PBRATE_SP_x100		0x800200c8

/* Language code of Track info */
#define msa_LANG_CODE_ASCII			0x0100
#define msa_LANG_CODE_MODIFIED8859	0x0300
#define msa_LANG_CODE_MS_JIS		0x8100
#define msa_LANG_CODE_S_JIS			0x9000

/* Infomation type of Track info */
#define msa_INF_NOTGETSU		0x01		// if 1,Not calculate total size
#define msa_INF_ALBUM			0x08
#define msa_INF_COMMENT			0x10
#define msa_INF_GENRE 			0x20
#define msa_INF_ARTIST			0x40
#define msa_INF_TITLE			0x80
#define msa_INF_INFALL		(msa_INF_ALBUM | msa_INF_COMMENT|msa_INF_GENRE|msa_INF_ARTIST|msa_INF_TITLE)

/* Total SU of Album */
#define msa_TOTALSU_UNKNOWN	0xffffffff

/* codecmode bit rate kbps */
#define msa_BITRATE0	33
#define msa_BITRATE1	47
#define msa_BITRATE2	66
#define msa_BITRATE3	94
#define msa_BITRATE4	105
#define msa_BITRATE5	132
#define msa_BITRATE6	146
#define msa_BITRATE7	176

/* Codec Type */
typedef enum{
	msa_CODEC_ATRAC,
	msa_CODEC_MP3
}MsaCodecType;

/* Sampling Frequency */

#define msa_SAMPLE_FREQ0	44100
#define msa_SAMPLE_FREQ1	48000
#define msa_SAMPLE_FREQ2	32000

/* Limit play count(trackinfo.limittime) */
#define msa_LIMIT_PLAY_DATE_EXIST	0x8000
#define msa_LIMIT_PLAY_YES			0x0008
#define msa_LIMIT_PLAY_EXPIRED		0x0004

/* For Album Enumerate */
#define albumIteratorStart		0L
#define albumIteratorStop		0xffffffffL

#define msa_ALBUMREF_BASE		0x0001

/* PBList */
typedef struct{
	UInt16	format;
	UInt16	reserve1;
	UInt32	creatorID;
	UInt32	appinfo;
	UInt32	reserve2;
	UInt16	pblistindex[1];
}MsaPBList,*MsaPBListPtr;

/* Type of Status */
typedef enum{
	msa_PLAYSTATUS,
	msa_STOPSTATUS,
	msa_OTHERSTATUS
}MsaPlayStatus;

/* PB Status */
typedef struct{
	MsaPlayStatus	status;
	UInt32	pbrate;
	UInt16	currentpblistindex;
	UInt32	currentSU;
}MsaPBStatus, *MsaPBStatusPtr;

/* Type of Play Loop */
typedef enum{
	msa_PLAY_NOLOOP,
	msa_PLAY_LOOP,
	msa_PLAY_NOLIMIT = 0xffff	// Not Implemented
}MsaPlayloop;

/* Type of Scope */
typedef enum{
	msa_SCOPE_ALL,
	msa_SCOPE_ONETRACK,
	msa_SCOPE_ARB
}MsaScope;

/* Type of PB List */
typedef enum{
	msa_PBLIST_ALBUM,
	msa_PBLIST_PROGRAM
} MsaPbListType;

/* Type of Sequence */
typedef enum{
	msa_SEQUENCE_CONTINUE,
 	msa_SEQUENCE_REVERSE,
	msa_SEQUENCE_SHUFFLE
} MsaSequence;

/* Type of Conformation */
typedef enum{
	msa_CONFIRM_AUTO,
	msa_CONFIRM_PASS,	// Not Implemented
	msa_CONFIRM_STOP
} MsaConfirm;

/* PB Mode */
typedef struct{
	MsaPlayloop	loop;
	MsaScope	scope;
	MsaPbListType	pblisttype;
	MsaSequence	seq;
	MsaConfirm	confirm;
	UInt8		reserve; 
	UInt16		pblistindex1;
	UInt32		startTime;
	UInt16		pblistindex2;
	UInt32		endTime;
} MsaPBMode, *MsaPBModePtr;

/* TrackInfo */
typedef struct{
	UInt32	titleoffset;		//offset from trackinfo[1]
	UInt32	artistoffset;		//offset from trackinfo[1]
	UInt32	genreoffset;		//offset from trackinfo[1]
	UInt32	commentoffset;	//offset from trackinfo[1]
	UInt32	albumoffset;		//offset from trackinfo[1]
	UInt32	totalsu;	
	UInt16	tracknum;
	UInt16	limitinfo;			// bit 15  datetime info flag
							// bit 7-0 LT
	UInt16	codecmode;
	MsaCodecType codectype;
	UInt16	frequency;
	Char		trackinfo[1];
} MsaTrackInfo, *MsaTrackInfoPtr;

/* Track Restriction Info */
typedef struct{
	DateTimeType pbstartdatetime;
	DateTimeType pbfinishdatetime;
	UInt8		maxplaytime;
	UInt8		curplaytime;
	UInt16		reserved;
}MsaTrackRestrictionInfo,*MsaTrackRestrictionInfoPtr;

/* Control key type */

typedef enum{
	msaControlkeyNoKey,
	msaControlkeyPlayPause,
	msaControlkeyFRPlay,
	msaControlkeyFFPlay,
	msaControlkeyPause,
	msaControlkeyStop,
	msaControlkeyVolm,
	msaControlkeyVolp,
	msaControlkeyPlay,
	msaControlkeyCue,
	msaControlkeyRev,
	msaControlkeyAMSp,
	msaControlkeyAMSm,
	msaControlkeyFF,
	msaControlkeyFR,
	msaControlkeyRepeat,
	msaControlkeyPlay1Track,
	msaControlkeyPlayAllTrack,
	msaControlkeyPlaySection,
	msaControlkeySetSection,
	msaControlkeyOrderNormal,
	msaControlkeyOrderReverse,
	msaControlkeyOrderShuffle,
	msaControlkeyHold,
	msaControlkey_NUMCODE
}MsaControlKey;

/* Type of Key */
typedef enum{
	msaControlKeySet,
	msaControlKeyRelease,
	msaControlKeyLong
} MsaControlKeyState;

/* Msa Time */
typedef struct{
	UInt16 minute;
	UInt16 second;
	UInt16 frame;	// mili sec
}MsaTime,*MsaTimePtr;


/*
 * MsaOut
 */
/* MsaOut error code */
#if 0
typedef enum {
	msaOutErrNone = 0,
	msaOutErrInvalidParam = msaOutErrClass | 1,
	msaOutErrBandOutOfRange,
	msaOutErrLevelOutOfRange,
	msaOutErrFreqOutOfRange,
	msaOutErrPatternOutOfRange,
	msaOutErrAlreadyStopped,
	msaOutErrAlreadyOpened,
	msaOutErrAlreadyClosed,
	msaOutErrClosed,
	msaOutErrHwr,
	msaOutErrNotSupported
} MsaOutErr;
#else
typedef Err MsaOutErr;
#define msaOutErrClass                  (sonyMsaErrorClass|0x40)
#define msaOutErrNone                   (0)
#define msaOutErrInvalidParam           (msaOutErrClass| 1)
#define msaOutErrBandOutOfRange         (msaOutErrClass| 2)
#define msaOutErrLevelOutOfRange        (msaOutErrClass| 3)
#define msaOutErrFreqOutOfRange         (msaOutErrClass| 4)
#define msaOutErrPatternOutOfRange      (msaOutErrClass| 5)
#define msaOutErrAlreadyStopped         (msaOutErrClass| 6)
#define msaOutErrAlreadyOpened          (msaOutErrClass| 7)
#define msaOutErrAlreadyClosed          (msaOutErrClass| 8)
#define msaOutErrClosed                 (msaOutErrClass| 9)
#define msaOutErrHwr                    (msaOutErrClass|10)
#define msaOutErrNotSupported           (msaOutErrClass|11)
#endif

/* Output mode */
typedef enum {
	msaOutOutputStereo = 0,
	msaOutOutputMonoral,
	msaOutOutputMain,
	msaOutOutputSub,
	msaOutOutputDual
} MsaOutOutputMode;

/* Muting switch */
typedef enum {
	msaOutMuteOFF = 0,
	msaOutMuteON
} MsaOutMuteSwitch;

/* Equalizer switch */
typedef enum {
	msaOutEQOFF = 0,
	msaOutEQON
} MsaOutEQSwitch;

/* Beep status */
typedef enum {
	msaOutBeepStop = 0,
	msaOutBeepPlay
} MsaOutBeepStatus;

/* Beep pattern */
typedef enum {
	msaOutBeepPatternPlay = 0,
	msaOutBeepPatternStop,
	msaOutBeepPatternPause,
	msaOutBeepPatternAMSp,
	msaOutBeepPatternAMSm,
	msaOutBeepPatternFirst,
	msaOutBeepPatternWarn,
	msaOutBeepPatternErr,
	msaOutBeepPatternSkip,
	msaOutBeepPatternOK,
	msaOutBeepPatternCancel,
	msaOutBeepPatternClick,
	msaOutBeepPatternReset,
	msaOutBeepPattern13,
	msaOutBeepPattern14,
	msaOutBeepPattern15
} MsaOutBeepPattern;

/* information */
typedef struct {
	MsaOutOutputMode outputMode;         /* output mode                        */
	UInt16 volumeL;                      /* value of Lch volume                */
	UInt16 volumeR;                      /* value of Rch volume                */
	UInt16 volumeLimitL;                 /* value of Lch volume limit          */
	UInt16 volumeLimitR;                 /* value of Rch volume limit          */
	MsaOutMuteSwitch muteSwitch;         /* state of mute switch               */
	MsaOutEQSwitch EQSwitch;             /* state of EQualizer switch          */ 
	UInt16 *EQvalueP;                    /* pointer to the value table of
                                           EQualizer level                    */
	UInt16 BBLevel;                      /* bassboost level                    */
	UInt16 beepLevel;                    /* beep level                         */
} MsaOutInfoType, *MsaOutInfoPtr;

/* capability */
typedef struct {
#define msaOutIncapable                 (0)
#define msaOutCapable                   (1)
	UInt32 monoral:1;
	UInt32 bilingual:1;
	UInt32 volumeL:1;
	UInt32 volumeR:1;
	UInt32 volumeLLimit:1;
	UInt32 volumeRLimit:1;
	UInt32 deEmphasis:1;
	UInt32 mute:1;
	UInt32 EQ:1;
	UInt32 EQL:1;
	UInt32 EQR:1;
	UInt32 BB:1;
	UInt32 beep:1;
	UInt32 levelL:1;
	UInt32 levelR:1;
	UInt32 spectrumL:1;
	UInt32 spectrumR:1;
	UInt32 reservedFlag:15;
	UInt16 volumeReso;
	UInt16 volumeLimitReso;
	UInt16 volumeLimitForAVLS;
	UInt16 volumeDefault;
	UInt16 EQReso;
	UInt16 EQNumBand;
	UInt16 BBMaxLevel;
	UInt16 beepMaxLebel;
	UInt16 beepMaxFreq;
	UInt16 beepMaxPattern;
	UInt16 levelReso;
	UInt16 spectrumReso;
	UInt16 spectrumNumBand;
} MsaOutCapabilityType, *MsaOutCapabilityPtr;

/*
 * MsaStream
 */
/* MsaStream error code */
#if 0
typedef enum {
	msaStreamErrNone = 0,
	msaStreamErrInvalidParam = msaStreamErrClass | 1,
	msaStreamErrLevelOutOfRange,
	msaStreamErrAlreadyStarted,
	msaStreamErrAlreadyStopped,
	msaStreamErrHwr,
	msaStreamErrNotSupported,
	msaStreamErrAlreadyClosed,
	msaStreamErrNotOpen
} MsaStreamErr;
#else
typedef Err MsaStreamErr;
#define msaStreamErrClass               (sonyMsaErrorClass|0x80)
#define msaStreamErrNone                (0)
#define msaStreamErrInvalidParam        (msaStreamErrClass| 1)
#define msaStreamErrLevelOutOfRange     (msaStreamErrClass| 2)
#define msaStreamErrAlreadyStarted		(msaStreamErrClass| 3)
#define msaStreamErrAlreadyStopped      (msaStreamErrClass| 4)
#define msaStreamErrHwr                 (msaStreamErrClass| 5)
#define msaStreamErrNotSupported        (msaStreamErrClass| 6)
#define	msaStreamErrAlreadyClosed		(msaStreamErrClass| 7)
#define msaStreamErrNotOpen             (msaStreamErrClass| 8)
#endif

/* codec */
#define msaStreamCodecATRAC3            (0x270)

/* channel mode (for ATRAC3) */
#define msaStreamAT3ChanModeMono        (0x0001) /* (Joint)Mono                   */
#define msaStreamAT3ChanModeStereo      (0x0002) /* DualStereo                    */
#define msaStreamAT3ChanModeJoint       (0x0012) /* JointStereo                   */

/* bitrate (for ATRAC3) */
#define msaStreamAT3BitRate12           (12)
#define msaStreamAT3BitRate16           (16)
#define msaStreamAT3BitRate24           (24)
#define msaStreamAT3BitRate33           (33)
#define msaStreamAT3BitRate47           (47)
#define msaStreamAT3BitRate66           (66)
#define msaStreamAT3BitRate94           (94)
#define msaStreamAT3BitRate105          (105)
#define msaStreamAT3BitRate132          (132)
#define msaStreamAT3BitRate146          (146)
#define msaStreamAT3BitRate176          (176)

/* feature */
typedef enum {
	msaStreamFeatureVolume = 1,
	msaStreamFeatureBalance,
	msaStreamFeatureOutputMode,
	msaStreamFeatureMax = msaStreamFeatureOutputMode
} msaStreamFeature;

/* codec information */
typedef struct {
	UInt16 chanMode;                     /* channel mode (mono,stereo,joint)   */
	UInt16 bitRate;                      /* bitrate by Kbps                    */
} MsaStreamCodecInfoAT3Type, *MsaStreamCodecInfoAT3Ptr;

typedef union {
	MsaStreamCodecInfoAT3Type at3;       /* ATRAC3                             */
} MsaStreamCodecInfoType, *MsaStreamCodecInfoPtr;

/* status */
typedef struct {
	UInt16 codec;                        /* codec (ATRAC3,...)                 */
	UInt32 sampleRate;                   /* sampling rate by Hz                */
	MsaStreamCodecInfoPtr codecInfoP;    /* codec rerated information          */
	UInt16 status;                       /* playing status                     */
#define msaStreamStateStop              (1)  /* Stop                          */
#define msaStreamStatePreStop           (2)  /* PreStop                       */
#define msaStreamStatePlay              (3)  /* Play                          */
#define msaStreamStatePrePlay           (4)  /* PrePlay                       */
#define msaStreamStatePause             (5)  /* Pause                         */
#define msaStreamStatePrePause          (6)  /* PrePause                      */
	MemPtr dataPtr;                      /* pointer to current buffer          */
	UInt32 dataSize;                     /* data size of current buffer        */
	MsaUInt64 atTime;                    /* attime of current buffer           */
	MsaUInt64 curTime;                   /* current playing time               */
} MsaStreamStatusType, *MsaStreamStatusPtr;



typedef struct {
	UInt16 albumtype;		// [OUT]codec
	UInt16 albumRefNum;		// [OUT]Reference Number of Album
	UInt16 volRef;			// [OUT]Volume Reference of Album
	Char   *nameP;
	UInt16 fileNameLength;	// [IN]size of buffer for name
	UInt8  maskflag;		// [IN/OUT]bit field of Info to get
	UInt8  reserve1;
//	UInt8 *maskP;
	UInt16 code;			// [IN]Charcter code to get
	MemHandle infoH;			// [IN/*OUT] Handle of Info to get
//	MemHandle *hdlP;	
	UInt32 reserve2;
}AlbumInfoType,*AlbumInfoPtr;

typedef void (*MsaStreamCallbackFunc)(UInt32 param, MemPtr dataPtr);

/********************************************************************
 * API Prototypes
 ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


//--------------------------------------------------
// Standard library open, close, sleep and wake functions
//--------------------------------------------------

extern Err MsaLibOpen(UInt16 msaLibrefNum, UInt16 mode)
				MSA_LIB_TRAP(sysLibTrapOpen);
				
extern Err MsaLibClose(UInt16 msaLibrefNum, UInt16 mode)
				MSA_LIB_TRAP(sysLibTrapClose);

extern Err MsaLibSleep(UInt16 msaLibrefNum)
				MSA_LIB_TRAP(sysLibTrapSleep);

extern Err MsaLibWake(UInt16 msaLibrefNum)
				MSA_LIB_TRAP(sysLibTrapWake);


//--------------------------------------------------
// Custom library API functions
//--------------------------------------------------
	
// Custom routine 0
extern Err MsaPlay(UInt16 msaLibrefNum, UInt16 currenttrack,
                    UInt32 currentposition, UInt32 pbrate)
				MSA_LIB_TRAP(msaTrapPlay);
	
// Custom routine 1
extern Err MsaStop(UInt16 msaLibrefNum, Boolean reset)
				MSA_LIB_TRAP(msaTrapStop);
	
// Custom routine 2
extern Err MsaGetPBPosition(UInt16 msaLibrefNum,
                    UInt16 *currenttrackno, UInt32 *currentposition)
				MSA_LIB_TRAP(msaTrapGetPBPosition);

// Custom routine 3
Err MsaGetTrackInfo(UInt16 msaLibrefNum, UInt16 trackNo, UInt8 *maskP,
			 UInt16 code, MemHandle *hdlP)
				MSA_LIB_TRAP(msaTrapGetTrackInfo);
// Custom routine 4
extern Err MsaGetPBStatus(UInt16 msaLibrefNum, MsaPBStatusPtr pbstatusP)
				MSA_LIB_TRAP(msaTrapGetPBStatus);

// Custom routine 5
extern Err MsaSetPBRate(UInt16 msaLibrefNum, UInt32 pbrate)
				MSA_LIB_TRAP(msaTrapSetPBRate);

// Custom routine 6
extern Err MsaSetPBPosition(UInt16 msaLibrefNum, UInt16 currenttrackno,
                      UInt32 currentposition)
				MSA_LIB_TRAP(msaTrapSetPBPosition);

// Custom routine 7
extern Err MsaGetPBRate(UInt16 msaLibrefNum, UInt32 *pbrateP)
				MSA_LIB_TRAP(msaTrapGetPBRate);

// Custom routine 8
extern Err MsaGetPBMode(UInt16 msaLibrefNum, MsaPBModePtr pbmodeP)
				MSA_LIB_TRAP(msaTrapGetPBMode);

// Custom routine 9
extern Err MsaGetPBList(UInt16 msaLibrefNum, MsaPBListPtr pblistP, UInt16 *tracknum)
				MSA_LIB_TRAP(msaTrapGetPBList);

// Custom routine 10
extern Err MsaSetPBList(UInt16 msaLibrefNum, MsaPBListPtr pblistP, UInt16 tracknum)
				MSA_LIB_TRAP(msaTrapSetPBList);

// Custom routine 11
extern Err MsaSetPBStatus(UInt16 msaLibrefNum, MsaPBStatus *pbstatus)
				MSA_LIB_TRAP(msaTrapSetPBStatus);

// Custom routine 12
extern Err MsaSetPBMode(UInt16 msaLibrefNum, MsaPBModePtr pbmodeP)
				MSA_LIB_TRAP(msaTrapSetPBMode);
				
// Custom routine 13
extern Err MsaSuToTime(UInt16 msaLibrefNum,UInt32 SU, MsaTimePtr timeP)
				MSA_LIB_TRAP(msaTrapSuToTime);
				
// Custom routine 14
extern Err MsaTimeToSu(UInt16 msaLibrefNum, MsaTimePtr timeP, UInt32 *SU)
				MSA_LIB_TRAP(msaTrapTimeToSu);

// Custom routine 15
extern Err MsaPBListIndexToTrackNo(UInt16 msaLibrefNum, 
								UInt16 pblistindex,UInt16 *trackno)
				MSA_LIB_TRAP(msaTrapPBListIndexToTrackNo);

// Custom routine 16
extern Err MsaGetShufflePlayedList(UInt16 msaLibrefNum, UInt32 *shuffleplayedlist)
				MSA_LIB_TRAP(msaTrapGetShufflePlayedList);

// Custom routine 17
Err MsaGetAlbum(UInt16 msaLibrefNum, UInt16 *albumrefNum, UInt32 *dummy)
//extern Err MsaGetAlbum(UInt16 msaLibrefNum, UInt16 *cardNo, LocalID *dbIDP)
				MSA_LIB_TRAP(msaTrapGetAlbum);

// Custom routine 18
//extern Err MsaSetAlbum(UInt16 msaLibrefNum, UInt16 cardNo, LocalID *dbIDP)
Err MsaSetAlbum(UInt16 msaLibrefNum, UInt16 albumrefNum, UInt32 *dummy)
				MSA_LIB_TRAP(msaTrapSetAlbum);

// Custom routine 19
extern Err MsaSetControlKey(UInt16 msaLibrefNum, 
						MsaControlKey controlkey, MsaControlKeyState keystatus)
				MSA_LIB_TRAP(msaTrapSetControlKey);

// Custom routine 20
extern Err MsaEdit(UInt16 msaLibrefNum, UInt8 command, UInt16 track1, UInt16 track2, UInt32 su)
				MSA_LIB_TRAP(msaTrapEdit);

// Custom routine 21
extern Boolean MsaGetCapability(UInt16 msaLibrefNum, MsaCodecType codectype, UInt32 pbrate)
				MSA_LIB_TRAP(msaTrapGetCapability);

// Custom routine 22
extern UInt32 MsaGetAPIVersion(UInt16 msaLibrefNum)
				MSA_LIB_TRAP(msaTrapGetAPIVersion);

// Custom routine 23
Err MsaGetTrackRestrictionInfo(UInt16 msaLibrefNum, UInt16 trackNo,
						MsaTrackRestrictionInfoPtr resrictionP)
				MSA_LIB_TRAP(msaTrapGetTrackRestrictionInfo);
				
// Custom routine 24
extern MsaOutErr MsaOutGetCapability(UInt16 msaLibrefNum,
												 MsaOutCapabilityType *capabilityP)
				MSA_LIB_TRAP(msaTrapOutGetCapability);

// Custom routine 25
extern MsaOutErr MsaOutInit(UInt16 msaLibrefNum)
				MSA_LIB_TRAP(msaTrapOutInit);

// Custom routine 26
extern MsaOutErr MsaOutSetOutputMode(UInt16 msaLibrefNum, MsaOutOutputMode mode)
				MSA_LIB_TRAP(msaTrapOutSetOutputMode);

// Custom routine 27
extern MsaOutErr MsaOutSetVolume(UInt16 msaLibrefNum, UInt16 lValue, UInt16 rValue)
				MSA_LIB_TRAP(msaTrapOutSetVolume);

// Custom routine 28
extern MsaOutErr MsaOutVolumeUp(UInt16 msaLibrefNum)
				MSA_LIB_TRAP(msaTrapOutVolumeUp);

// Custom routine 29
extern MsaOutErr MsaOutVolumeDown(UInt16 msaLibrefNum)
				MSA_LIB_TRAP(msaTrapOutVolumeDown);

// Custom routine 30
extern MsaOutErr MsaOutSetVolumeLimit(UInt16 msaLibrefNum,
												  UInt16 lValue, UInt16 rValue)
				MSA_LIB_TRAP(msaTrapOutSetVolumeLimit);

// Custom routine 31
extern MsaOutErr MsaOutSetMute(UInt16 msaLibrefNum, MsaOutMuteSwitch muteSwitch)
				MSA_LIB_TRAP(msaTrapOutSetMute);

// Custom routine 32
extern MsaOutErr MsaOutSetEQ(UInt16 msaLibrefNum, MsaOutEQSwitch EQSwitch)
				MSA_LIB_TRAP(msaTrapOutSetEQ);

// Custom routine 33
extern MsaOutErr MsaOutSetEQLevel(UInt16 msaLibrefNum, UInt16 band,
											 UInt16 lValue, UInt16 rValue)
				MSA_LIB_TRAP(msaTrapOutSetEQLevel);

// Custom routine 34
extern MsaOutErr MsaOutSetBBLevel(UInt16 msaLibrefNum, UInt16 level)
				MSA_LIB_TRAP(msaTrapOutSetBBLevel);

// Custom routine 35
extern MsaOutErr MsaOutSetBeepLevel(UInt16 msaLibrefNum, UInt16 level)
				MSA_LIB_TRAP(msaTrapOutSetBeepLevel);

// Custom routine 36
extern MsaOutErr MsaOutStartBeep(UInt16 msaLibrefNum, UInt16 freq,
											MsaOutBeepPattern pattern)
				MSA_LIB_TRAP(msaTrapOutStartBeep);

// Custom routine 37
extern MsaOutErr MsaOutStopBeep(UInt16 msaLibrefNum)
				MSA_LIB_TRAP(msaTrapOutStopBeep);

// Custom routine 38
extern MsaOutErr MsaOutGetVolume(UInt16 msaLibrefNum, UInt16 *lValue, UInt16 *rValue)
				MSA_LIB_TRAP(msaTrapOutGetVolume);

// Custom routine 39
extern MsaOutErr MsaOutGetVolumeLimit(UInt16 msaLibrefNum,
												  UInt16 *lLimitP, UInt16 *rLimitP)
				MSA_LIB_TRAP(msaTrapOutGetVolumeLimit);

// Custom routine 40
extern MsaOutErr MsaOutGetMute(UInt16 msaLibrefNum, MsaOutMuteSwitch *muteSwitchP)
				MSA_LIB_TRAP(msaTrapOutGetMute);

// Custom routine 41
extern MsaOutErr MsaOutGetEQ(UInt16 msaLibrefNum, MsaOutEQSwitch *EQSwitchP)
				MSA_LIB_TRAP(msaTrapOutGetEQ);

// Custom routine 42
extern MsaOutErr MsaOutGetEQLevel(UInt16 msaLibrefNum, UInt16 band,
											 UInt16 *lValueP, UInt16 *rValueP)
				MSA_LIB_TRAP(msaTrapOutGetEQLevel);

// Custom routine 43
extern MsaOutErr MsaOutGetBBLevel(UInt16 msaLibrefNum, UInt16 *levelP)
				MSA_LIB_TRAP(msaTrapOutGetBBLevel);

// Custom routine 44
extern MsaOutErr MsaOutGetBeepLevel(UInt16 msaLibrefNum, UInt16 *levelP)
				MSA_LIB_TRAP(msaTrapOutGetBeepLevel);

// Custom routine 45
extern MsaOutErr MsaOutGetOutputMode(UInt16 msaLibrefNum, MsaOutOutputMode *modeP)
				MSA_LIB_TRAP(msaTrapOutGetOutputMode);

// Custom routine 46
extern MsaOutErr MsaOutGetInfo(UInt16 msaLibrefNum, MsaOutInfoType *infoP)
				MSA_LIB_TRAP(msaTrapOutGetInfo);

// Custom routine 47
extern MsaOutErr MsaOutGetLevel(UInt16 msaLibrefNum, UInt16 *lValueP, UInt16 *rValueP)
				MSA_LIB_TRAP(msaTrapOutGetLevel);

// Custom routine 48
extern MsaOutErr MsaOutGetSpectrum(UInt16 msaLibrefNum,
											  UInt16 *lValueP, UInt16 *rValueP)
				MSA_LIB_TRAP(msaTrapOutGetSpectrum);

// Custom routine 49
extern MsaOutErr MsaOutGetBeepStatus(UInt16 msaLibrefNum, MsaOutBeepStatus *statusP)
				MSA_LIB_TRAP(msaTrapOutGetBeepStatus);
				
// Custom routine 50
extern MsaStreamErr MsaStreamOpenStream(UInt16 msaLibrefNum, UInt32 *msaStreamRefnumP)
				MSA_LIB_TRAP(msaTrapStreamOpenStream);

// Custom routine 51
extern MsaStreamErr MsaStreamCloseStream(UInt16 msaLibrefNum, UInt32 msaStreamRefnum)
				MSA_LIB_TRAP(msaTrapStreamCloseStream);

// Custom routine 52
extern MsaStreamErr MsaStreamSetFormat(UInt16 msaLibrefNum, UInt32 msaStreamRefnum,
										UInt16 codec, UInt32 sampleRate,
										UInt16 numChannel, void *codecInfo)
				MSA_LIB_TRAP(msaTrapStreamSetFormat);

// Custom routine 53
extern MsaStreamErr MsaStreamPut(UInt16 msaLibrefNum, UInt32 msaStreamRefnum, MemPtr dataPtr,
							UInt32 dataSize, MsaUInt64 atTime)
				MSA_LIB_TRAP(msaTrapStreamPut);

// Custom routine 54
extern MsaStreamErr MsaStreamSetCallback(UInt16 msaLibrefNum, UInt32 msaStreamRefnum,
									MsaStreamCallbackFunc func, UInt32 param)
				MSA_LIB_TRAP(msaTrapStreamSetCallback);

// Custom routine 55
extern MsaStreamErr MsaStreamStartPlay(UInt16 msaLibrefNum, UInt32 msaStreamRefnum,
										MsaUInt64 startTime)
				MSA_LIB_TRAP(msaTrapStreamStartPlay);

// Custom routine 56
extern MsaStreamErr MsaStreamStopPlay(UInt16 msaLibrefNum, UInt32 msaStreamRefnum)
				MSA_LIB_TRAP(msaTrapStreamStopPlay);

// Custom routine 57
extern MsaStreamErr MsaStreamFlush(UInt16 msaLibrefNum, UInt32 msaStreamRefnum)
				MSA_LIB_TRAP(msaTrapStreamFlush);

// Custom routine 58
extern MsaStreamErr MsaStreamGetStatus(UInt16 msaLibrefNum, UInt32 msaStreamRefnum,
										MsaStreamStatusType *msaStreamStatusP)
				MSA_LIB_TRAP(msaTrapStreamGetStatus);

// Custom routine 59
extern MsaStreamErr MsaStreamGetTime(UInt16 msaLibrefNum, UInt32 msaStreamRefnum,
										MsaUInt64 *curTimeP, UInt32 *palmTimeP)
				MSA_LIB_TRAP(msaTrapStreamGetTime);

// Custom routine 60
extern MsaStreamErr MsaStreamSetFeatureValue(UInt16 msaLibrefNum, UInt32 msaStreamRefnum,
												UInt32 feature, void *featureP)
				MSA_LIB_TRAP(msaTrapStreamSetFeatureValue);

// Custom routine 61	
extern MsaStreamErr MsaStreamGetFeatureValue(UInt16 msaLibrefNum, UInt32 msaStreamRefnum,
												UInt32 feature, void *featureP)
				MSA_LIB_TRAP(msaTrapStreamGetFeatureValue);

// Custom routine 62	
/* reserved */
MsaStreamErr MsaStreamReservedFunction(UInt16 msaLibrefNum)
				MSA_LIB_TRAP(msaTrapStreamReservedFunction);

// Custom routine 63
extern Err MsaLibEnforceOpen(UInt16 msaLibrefNum, UInt16 mode, UInt32 creator)
				MSA_LIB_TRAP(msaTrapEnforceOpen);

// Custom routine 64
Err MsaAlbumEnumerate(UInt16 msaLibrefNum, UInt32 *albumIteraterP, AlbumInfoType *infoP)
				MSA_LIB_TRAP(msaTrapAlbumEnumarate);
				
#ifdef __cplusplus 
}
#endif


#endif	// __MSA_LIB_H__
