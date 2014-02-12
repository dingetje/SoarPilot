/******************************************************************************
 *                                                                            *
 *                 (C) Copyright 2000, Sony Corporation                       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *    <IDENTIFICATION>                                                        *
 *       file name    : $Workfile: SonySystemFtr.h $
 *                                                                            *
 *    <PROFILE>                                                               *
 *       Feature related definitions for Sony System                          *
 *                                                                            *
 *    <HISTORY>                                                               *
 *       Started on   : 00/11/28                                              *
 *       Last Modified: $Date: 2003/01/06 23:02:02 $
 *                                                                            *
 ******************************************************************************/
/* this file is best viewed by setting TAB-stop as 3 */

#ifndef __SONYSYSTEMFTR_H__
#define __SONYSYSTEMFTR_H__

/******************************************************************************
 *    Includes                                                                *
 ******************************************************************************/
#include <SystemMgr.h>
#include <SonySystemResources.h>


/******************************************************************************
 *    Features                                                                *
 ******************************************************************************/

/*** Sony Ftr Creator ***/
#define sonySysFtrCreator				sonySysFileCSony

/*** Ftr Number ***/
#define sonySysFtrNumSysInfoP			(1)		/* ptr to SysInfo */
#define sonySysFtrNumStringInfoP		(2)		/* ptr to StringInfo */
#define sonySysFtrNumJogAstMaskP		(3)		/* ptr to JogAstMask */
#define sonySysFtrNumJogAstMOCardNoP (4)		/* ptr to JogAstMaskOwnerCardNo */
#define sonySysFtrNumJogAstMODbIDP	(5)		/* ptr to JogAstMaskOwnerDbID */


/******************************************************************************
 *    Structures for Featrures                                                *
 ******************************************************************************/

/*** SysInfoP ***/
typedef struct S_SonySysFtrSysInfo {
	UInt16 revision;
	UInt16 rsv16_00;
	UInt32 extn;			/* loaded extension */
	UInt32 libr;			/* loaded libr */
	UInt32 rsv32_00;
	UInt32 rsv32_01;

	void *rsvP;
	UInt32 status;			/* current system status */
	UInt32 msStatus;		/* current MemoryStick status */
	UInt32 rsv32_10;

	UInt16 msSlotNum;		/* number of slot of MemoryStick */
	UInt16 jogType;
	UInt16 rmcType;
} SonySysFtrSysInfoType;
typedef SonySysFtrSysInfoType *SonySysFtrSysInfoP;

	/* revision field */
#define sonySysFtrSysInfoRevision		(1)

	/* extn field */
#define sonySysFtrSysInfoExtnJog		(0x00000001L)	/* vchrJogEvent usable */
#define sonySysFtrSysInfoExtnRmc		(0x00000002L)	/* vchrRmcEvent usable */
#define sonySysFtrSysInfoExtnHold	(0x00000004L)	/* Hold switch usable */
#define sonySysFtrSysInfoExtnJogAst	(0x00000008L)	/* JogAssist usable */
#define sonySysFtrSysInfoExtnSilk	(0x00000010L)	/* Software silk usable */
#define sonySysFtrSysInfoExtnCapBtn	(0x00000020L)	/* vchrCapEvent usable */
#define sonySysFtrSysInfoExtnKB		(0x00000040L)	/* Hardware Keyboard usable */

	/* libr field */
#define sonySysFtrSysInfoLibrHR		(0x00000001L)	/* HR-Lib usable */
#define sonySysFtrSysInfoLibrMsa		(0x00000002L)	/* Msa-Lib usable */
#define sonySysFtrSysInfoLibrRmc		(0x00000004L)	/* Rmc-Lib usable */
#define sonySysFtrSysInfoLibrFm		(0x00000020L)	/* Sound-Lib usable */
#define sonySysFtrSysInfoLibrCap		(0x00000040L)	/* Capture-Lib usable */
#define sonySysFtrSysInfoLibrJpeg	(0x00000080L)	/* Jpeg-Lib usable */
#define sonySysFtrSysInfoLibrSilk	(0x00000100L)	/* Silk-Lib usable */

	/* status field */	/* 1: on(inserted/enabled), 0: off(removed/disabled) */
#define sonySysFtrSysInfoStatusHP			(0x00000001)	/* HeadPhone */
#define sonySysFtrSysInfoStatusHoldOn		(0x00000002)	/* Hold switch */
#define sonySysFtrSysInfoStatusLcdRotate	(0x00000004)	/* Lcd Rotate status */
#define sonySysFtrSysInfoStatusLcdFlip		(0x00000008)	/* Lcd Flip status*/
#define sonySysFtrSysInfoStatusCamRotate	(0x00000010)	/* Cam Rotate status */

	/* msStatus field */	/* 1: inserted, 0: removed */
#define sonySysFtrSysInfoMsStatus1MS	(0x00000001)	/* MS in Slot 1*/
#define sonySysFtrSysInfoMsStatus1StrgMS	(0x00000002) /* StorageMS in Slot 1*/
#define sonySysFtrSysInfoMsStatus1MGMS	(0x00000004)	/* MG-MS in Slot 1*/
#define sonySysFtrSysInfoMsStatus1WP	(0x00000008)	/* WriteProtected */
#define sonySysFtrSysInfoMsStatus1Mask	(0x000000FF)	/* Mask for Slot 1 */

	/* jogType field */
#define sonySysFtrSysInfoJogTypeNone	(0)	/* No Jog available */
#define sonySysFtrSysInfoJogType1		(1)	/* 2D Jog (PEG-S300/500) */
#define sonySysFtrSysInfoJogType2		(2)	/* 2D Jog with Back */

	/* rmcType field */
#define sonySysFtrSysInfoRmcTypeNone	(0)	/* No Rmc available */
#define sonySysFtrSysInfoRmcType1		(1)	/* 6 buttons w/o display */
#define sonySysFtrSysInfoRmcType2		(2)	/* Audio Adapter */


/*** StringInfoP ***/
typedef struct S_SonySysFtrStringInfo {
	/* All chars are described with ASCII */	/* must be null-terminated */
								/* offset: ex. */
	Char maker[16];		/*   0/0x0000: ex. "Sony Corp." */
	Char model[16];		/*  16/0x0010: ex. "PEG-S300" */
	Char ship[16];			/*  32/0x0020: ex. "Japan" */
	Char os[32];			/*  48/0x0030: ex. "Palm OS 3.5" */
	Char cpu[32];			/*  80/0x0050: ex. "Motorola DragonBall-VZ(33MHz)" */
	Char comment[128];	/* 112/0x0070: ex. "Personal Entertainment..." */
	UInt16 code;			/* 240/0x00F0: code for comment2 */
	Char comment2[254];	/* 242/0x00F2: ex. "パーソナル・エンタテイメント..." */
								/* 496/0x01F0: */
} SonySysFtrStringInfoType;
typedef SonySysFtrStringInfoType *SonySysFtrStringInfoP;
	/* CAUTION: those strings is not guaranteed to be correct by Sony. Null
	     strings are possible. */
	/* code for 'code' field' */
#define sonySysFtrStingInfoCodeASCII	(0x0001)
#define sonySysFtrStingInfoCode8859		(0x0003)
#define sonySysFtrStingInfoCodeMSJIS	(0x0081)


/*** JogAstMaskP ***/
typedef void *JogAstMaskP;
	/* related specs are defined in JogAst.h */


#endif	// __SONYSYSTEMFTR_H__

