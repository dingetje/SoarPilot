/**
 * \file 68K/Libraries/Telephony/HsPhoneIOTA.h
 *
 * Header File for Phone Library API ---- IOTA CATEGORY
 *
 * All implementations of the Handspring Phone Library support a common API. 
 * This API is broken up into various categories for easier management.  This file
 * defines the IOTA category.  These API calls are used for features specific to
 * IOTA operation on CDMA networks.
 *
 * \license
 *
 *    Copyright (c) 2003 Handspring Inc., All Rights Reserved
 *
 * \author Arun Mathias
 *
 * $Id: HsPhoneIOTA.h,v 1.1 2004/03/08 02:36:57 Administrator Exp $
 *
 **************************************************************/

#ifndef HS_PHONEIOTA_H
#define HS_PHONEIOTA_H
#include <PalmOS.h>
#include <PalmTypes.h>
#ifndef __CORE_COMPATIBILITY_H__
#include <PalmCompatibility.h>
	// workaround for differing header files in sdk-3.5 and sdk-internal
	#ifndef __CORE_COMPATIBILITY_H__
	#define __CORE_COMPATIBILITY_H__
	#endif 
#endif 
#include <Common/Libraries/Telephony/HsPhoneTraps.h>     // trap table definition for phone library calls
#include <Common/Libraries/Telephony/HsPhoneErrors.h>    // error codes returned by phone library functions
#include <Common/Libraries/Telephony/HsPhoneTypes.h>
  

/**
 * Item description here
 **/
typedef enum 
{
  phnIOTAUnknown = 0,
	phnIOTAStart,
	phnIOTAEnd,
	phnIOTACommit,
	phnIOTANAMSelect
}
PhnIOTAStatus;


#define isValidPhnIOTAStatus(i) ((i >= phnIOTAUnknown) && (i <= phnIOTANAMSelect))

typedef struct
{
  PhnIOTAStatus		status;			// the status that the modem changes into
  UInt8			      namInUse;		// namInUse4IOTA currently
  PHNErrorCode		error;			// == 0 normally, only != 0 for phnIOTAEnd and phnIOTACommit if Commit failed.
} PhnIOTAReportType;

#endif
