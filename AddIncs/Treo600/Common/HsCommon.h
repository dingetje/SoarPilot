/**
 * \file Common/Hs.h
 *	Main Handspring Palm OS API Header file
 * 
 *	Most functions or API's are specific to either the 68K environment or the
 *	ARM environment but both of them sometimes share the same structures, types,
 *	constants etc.. The common header files are for those shared components that
 *	are used by both the 68K and ARM parts of a library, extention etc. In
 *	general, a developer would never have to include this file seperataely. It
 *	is already included in both the 68K/Hs.h and the ARM/Hs.h. 
 *
 * \license
 *
 *    Copyright (c) 2002 Handspring Inc., All Rights Reserved
 *
 * \author Kiran Prasad
 *
 * $Id: HsCommon.h,v 1.1 2004/03/08 02:37:44 Administrator Exp $
 *
 *****************************************************************************/

#ifndef __HS_COMMON_H__

#define __HS_COMMON_H__

#include <PalmTypes.h>
#include <PalmCompatibility.h>
#include <PalmOS.h>

#include <Common/System/HsAppLaunchCmd.h>
#include <Common/System/HsCreators.h>
#include <Common/System/HsErrorClasses.h>
#include <Common/System/HsExtCommon.h>
#include <Common/System/HsNavCommon.h>
#include <Common/System/HsKeyTypes.h>
#include <Common/System/HsKeyCodes.h>
#include <Common/System/HsExgMgrCommon.h>
#include <Common/System/HsErrors.h>
#include <Common/System/HsExtUtilBigButtonRsc.h>
#include <Common/System/HsExtUtilBigButtonCommon.h>
#include <Common/System/HsKeyCommon.h>


#include <Common/Libraries/TonesLibrary/TonesLibTypes.h>
#include <Common/Libraries/DefaultHelperLibrary/DefaultHelperLibTypes.h>
#include <Common/Libraries/FavoritesDBLibrary/FavoritesDBLibTypes.h>

#include <Common/Libraries/Imaging/ImageLibCommon.h>
#include <Common/Libraries/Imaging/ImageLibTraps.h>

#include <Common/Libraries/Camera/CameraLibCommon.h>

#include <Common/Libraries/HsSoundLib/HsSoundLibCommon.h>
#include <Common/Libraries/HsSoundLib/HsSoundLibTraps.h>


#include <Common/Libraries/HTTP/HS_HTTPLibApp.h>
#include <Common/Libraries/HTTP/HS_HTTPLibConst.h>

#include <Common/Libraries/MMS/MmsHelperCommon.h>

#include <Common/Libraries/NetMaster/NetMasterLibErrors.h>
#include <Common/Libraries/NetMaster/NetMasterLibTarget.h>
#include <Common/Libraries/NetMaster/NetMasterLibTraps.h>

#include <Common/Libraries/SmartTextEngine/SmartTextEngineDef.h>
#include <Common/Libraries/SmartTextEngine/SmartTextEngineErrors.h>
#include <Common/Libraries/SmartTextEngine/SmartTextEngineTraps.h>

#include <Common/Libraries/NetPref/NetPrefLibErrors.h>
#include <Common/Libraries/NetPref/NetPrefLibTarget.h>
#include <Common/Libraries/NetPref/NetPrefLibTraps.h>

#include <Common/Libraries/ComChannelProvider/ComChannelProviderTypes.h>
#include <Common/Libraries/TransparencyLibrary/TransparencyLibTypes.h>



#endif /* __HS_COMMON_H__ */
