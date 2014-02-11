/******************************************************************************
 *                                                                            *
 *                 (C) Copyright 2000, Sony Corporation                       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *    <IDENTIFICATION>                                                        *
 *       file name    : $Workfile: SonyNotify.h $
 *                                                                            *
 *    <PROFILE>                                                               *
 *       Type/DataStructure of Notifications for Sony System                  *
 *                                                                            *
 *    <HISTORY>                                                               *
 *       Started on   : 00/12/04                                              *
 *       Last Modified: $Date: 2003/01/06 23:02:03 $
 *                                                                            *
 ******************************************************************************/
/* this file is best viewed by setting TAB-stop as 3 */

#ifndef __SONYNOTIFY_H__
#define __SONYNOTIFY_H__

/******************************************************************************
 *    Includes                                                                *
 ******************************************************************************/
#include <NotifyMgr.h>
#include <SonySystemResources.h>

/******************************************************************************
 *    Type/DataStructure of Notifications                                     *
 ******************************************************************************/
#define sonySysNotifyBroadcasterCode      sonySysFileCSony

/*** MsaLib ***/
#define sonySysNotifyMsaStatusChangeEvent 'SnSc' /* Msa-Lib status change     */
				/* This notification is broadcasted by Deferred */
#define sonySysNotifyMsaEnforceOpenEvent 'SnEo'

typedef struct {
  UInt32 creator; /* creator who calls EnforceOpen() */
  UInt16 mode; /* new mode */
} SonySysNotifyMsaEnforceOpenDetailsType,
					*SonySysNotifyMsaEnforceOpenDetailsPtr;

typedef SonySysNotifyMsaEnforceOpenDetailsPtr 
					SonySysNotifyMsaEnforceOpenDetailsP;

#define msaHandledEnforceOpenRejected 0x01

/* Status change class */
typedef enum {
	sonySysNotifyMsaStatusChangeStopped = 1, /* Stopped                        */
	sonySysNotifyMsaStatusChangeSwitched,    /* TrackChange                    */
	sonySysNotifyMsaStatusChangeWarning      /* Warning                        */
} SonySysNotifyMsaStatusChangeClass;

/* Cause of stop */
typedef enum {
	sonySysNotifyMsaStopCauseNormal,        /* normal stop                     */
	sonySysNotifyMsaStopCauseRequest,       /* stop by request                 */
	sonySysNotifyMsaStopCauseError,         /* error occurred                  */
	sonySysNotifyMsaStopCauseSecurityError, /* security error occurred         */
	sonySysNotifyMsaStopCausePowerOff,      /* system power off                */
	sonySysNotifyMsaStopCauseRestrictTimes, /* use times restricted            */
	sonySysNotifyMsaStopCauseLowBattery,    /* low battery                     */
	sonySysNotifyMsaStopCauseTimesExpired,  /* use times expired               */
	sonySysNotifyMsaStopCauseTermExpired,   /* term expired                    */
	sonySysNotifyMsaStopCauseSystem,        /* system cause                    */
	sonySysNotifyMsaStopCauseUnknown,       /* unknown cause                   */
	sonySysNotifyMsaStopCauseConnectionLost /* connection has lost             */
} SonySysNotifyMsaStopCause;

/* Cause of switch(track change) */
typedef enum {
	sonySysNotifyMsaSwitchCauseNormal,      /* normal switch                   */
	sonySysNotifyMsaSwitchCauseRepeat,      /* switch by repeat                */
	sonySysNotifyMsaSwitchCauseRequest,     /* switch by request               */
	sonySysNotifyMsaSwitchCauseUnknown      /* unknown cause                   */
} SonySysNotifyMsaSwitchCause;

/* Cause of switch(track change) */
typedef enum {
	sonySysNotifyMsaWarningCauseDataError   /* data error                      */
} SonySysNotifyMsaWarningCause;

/* Union of cause */
typedef union {
	SonySysNotifyMsaStopCause    stopped;
	SonySysNotifyMsaSwitchCause  switched;
	SonySysNotifyMsaWarningCause warning;
} SonySysNotifyMsaCause;

/* Details of status change */
typedef struct {
	SonySysNotifyMsaStatusChangeClass clas;
	SonySysNotifyMsaCause cause;
} SonySysNotifyMsaStatusChangeDetailsType,
					*SonySysNotifyMsaStatusChangeDetailsPtr;
typedef SonySysNotifyMsaStatusChangeDetailsPtr 
					SonySysNotifyMsaStatusChangeDetailsP;


/*** HoldExtn ***/
#define sonySysNotifyHoldStatusChangeEvent	'SnHd' /* Hold status change */
				/* This notification is broadcasted by Deferred */
				/* Caution: Hold-notificaiton is broadcasted only when device is
				     powered On, so every On/Off notificaiton may not be paird with
				     each other. */ 

typedef struct {
	Boolean holdOn;	/* true when On, false when Off */
	UInt32 lock;		/* bitfields, valid when holdOn is true */
	UInt32 rsv;
} SonySysNotifyHoldStatusChangeDetailsType;
typedef SonySysNotifyHoldStatusChangeDetailsType
					*SonySysNotifyHoldStatusChangeDetailsPtr;
typedef SonySysNotifyHoldStatusChangeDetailsPtr
					SonySysNotifyHoldStatusChangeDetailsP;

#define sonySysNotifyHoldLockKey				(0x000001)
#define sonySysNotifyHoldLockPen				(0x000002)
#define sonySysNotifyHoldLockScreen			(0x000004)


#endif	// __SONYNOTIFY_H__

