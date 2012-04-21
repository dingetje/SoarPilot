/***************************************************************
 *
 *  Project:
 *	  Carrier Customization Library
 *
 * Copyright info:
 *
 *	  Copyright (c) Handspring 2002 -- All Rights Reserved
 *
 *
 *  FileName:
 *	  CarrierCustomizationLibraryTypes.h
 *
 *  Description:
 *	  This file contains public data types exported by the
 *    Carrier Customization Library
 *
 *	Note:
 *
 *	History:
 *	  11-Dec-2002	dla		Created
 *
 ****************************************************************/

#ifndef __CARRIER_CUSTOMIZATION_LIBRARY_TYPES_H__
#define __CARRIER_CUSTOMIZATION_LIBRARY_TYPES_H__


#if 0
#pragma mark -------- Constants -------- 
#endif

#define kCCLibType					  'libr'
#define kCarrierCustomizationCLibName "CarrierCustomization.lib"


// Current library API version number
//  Returned by NetPrefLibVersionGet()
#define CCSMVersionMajor	  0
#define	CCSMVersionMinor	  5
#define CCSMVersionBugFix	  0


typedef void *CCSMCarrierHandle;  /* pointer type to hold the carrier object */

/**
 * Enumeration for the type of carrier settings.
 * The following are applicable to CDMA as well.
 *
 *
 * We want to keep the groups together.
 * This helps keep track of which structure the setting resides in.
 */
enum
{
  kCCSMSettingsSystemReserved,         /**< system settings place holder */

  //                                         <Data type returned> description
  // phone settings
  kCCSMSettingsShowNetworkSelect,      /**< <Boolean> enable/disable network select */
  kCCSMSettingsIndicateRoaming,        /**< <Boolean> enable/disable roaming indicator */
  kCCSMSettingsEditableMSISDN,         /**< <Boolean> User editable MSISDN */
  kCCSMSettingsShowEmergencyMode,      /**< <Boolean> enable/disable emergency mode indicator */
  kCCSMSettingsCarrierMSISDN,          /**< <char *>  carrier MSISDN */
  kCCSMSettingsRadioBand,              /**< <UInt32>  radio band to start search with */
  kCCSMSettingsVoiceMailNumber,        /**< <char *>  carriers voice mail number */

  // Messaging settings
  kCCSMSettingsPOP3Server,             /**< <char *>  pop3 server */
  kCCSMSettingsSMTPServer,             /**< <char *>  smtp server */
  kCCSMSettingsSMSCNumber,             /**< <char *>  smsc number */
  kCCSMSettingsSMSEmailGateway,        /**< <char *>  sms email gateway */
  kCCSMSettingsMMSURL,                 /**< <char *>  MMS URL */
  kCCSMSettingsMMSWAPGateway,          /**< <char *>  MMS WAP gateway */
  kCCSMSettingsMMSPort,                /**< <UInt32>  MMS port number */
  kCCSMSettingsMMSMaxMsgSize,          /**< <UInt32>  max MMS message size */

  // Browser settings
  kCCSMSettingsPrimaryProxyServer,          /**< <char *>  Proxy server */
  kCCSMSettingsPrimaryProxyPort,            /**< <UInt32>  Proxy port */
  kCCSMSettingsPrimarySecureProxyServer,    /**< <char *>  Secure proxy server */
  kCCSMSettingsPrimarySecureProxyPort,      /**< <UInt32>  Secure proxy port */
  kCCSMSettingsSecondaryProxyServer,        /**< <char *>  Proxy server */
  kCCSMSettingsSecondaryProxyPort,          /**< <UInt32>  Proxy port */
  kCCSMSettingsSecondarySecureProxyServer,  /**< <char *>  Secure proxy server */
  kCCSMSettingsSecondarySecureProxyPort,    /**< <UInt32>  Secure proxy port */
  kCCMSSettingsPrimaryTrustedDomain,        /**< <char *>  GSM:  Primary trusted domain */
                                            /**< <char *>  CDMA: phone:downloadserver.url */

  kCCMSSettingsSecondaryTrustedDomain,      /**< <char *>  GSM Only: Secondary trusted domain */
  kCCMSSettingsUAProfilingURL,              /**< <char *>  UA Profiling URL */
  kCCSMSettingsHTTPHeaders,                 /**< <char *>  carrier specific HTTP headers to send */
  kCCSMSettingsNAIHeaderFormat,             /**< <char *>  header format to send NAI (see sprintf format) */
  kCCSMSettingsMSISDNHeaderFormat,          /**< <char *>  header format to send MSISDN (see sprintf format) */
  kCCSMSettingsEncryptMSISDN,               /**< <Boolean> encrypt MSISDN header */
  kCCSMSettingsHomepageURL,                 /**< <char *> home page string */

  // network settings
  kCCSMSettingsGPRSAutoAttach,              /**< flag whether or not to display the GPRS attached icon  */

  // NVRAM settings.
  kCCSMSettingsNAI,                         /**< Slot 1 username.  CDMA only */


  kCCSMSettingsReserved = 0x8000      /**< reserved */
};
typedef UInt16 CCSMSettingsType;  // used for the settingsTypeEnum


/** system settings
 *
 */
typedef UInt32 CCSMSystemFlagsType;
typedef enum
{
  kCCSMSystemSettingReserved  = 0x00000000  /**< reserved setting */
}
CCSMSystemFlagsEnum;

typedef struct tagSystemSettings
{
  CCSMSystemFlagsType flags;  /**< app flags - undefined */
  UInt32 projectUpdate;       /**< project flags - undefined */
  UInt32 reserved;            /**< reserved */
}
CCSMSystemSettingsType;


/** enumeration for the phone settings flags
 *
 */
typedef UInt32 CCSMPhoneFlagsType;
typedef enum
{
  kCCSMPhoneSettingsReserved          = 0x00000000,  /**< reserved setting */
  kCCSMPhoneSettingsNetworkSelect     = 0x00000001,  /**< Network Select disable/enable */
  kCCSMPhoneSettingsRoamingIndicator  = 0x00000002,  /**< Roaming indicator yes/no */
  kCCSMPhoneSettingsEditMSISDN        = 0x00000004,  /**< Editable MSISDN */
  kCCSMPhoneSettingsShowEmergencyMode = 0x00000008,  /**< use Emergency mode */
  kCCSMPhoneSettingsEnableTTY         = 0x00000010   /**< enable TTY */
}
CCSMPhoneFlagsEnum;


/** enumeration for the radio band
 *  Used to optimize which band the radio starts scanning on.
 */
typedef UInt32 CCSMRadioBandType;
typedef enum
{
  kCCSMRadioBandUnknown,        /**< unknown radio band */
  kCCSMRadioBandEurope,         /**< european radio */
  kCCSMRadioBandNorthAmerica    /**< north american radio */
}
CCSMRadioBandEnum;


/** phone settings
 *
 */
typedef struct tagPhoneSettings
{
  CCSMPhoneFlagsType flags;      /**< app flags - see CCSMPhoneFlagsEnum */
  CCSMRadioBandType radioBand;   /**< radio band - see CCSMRadioBandEnum */
  char *msisdnP;                 /**< MSISDN */
  char *voiceMailNumberP;        /**< voice mail number */
  UInt32 reserved;               /**< reserved */
}
CCSMPhoneSettingsType;


/** enumeration for the messaging settings flags
 *
 */
typedef UInt32 CCSMMessagingFlagsType;
typedef enum
{
  kCCSMEmailSettingsReserved = 0x00000000  /**< reserved setting */
}
CCSMMessagingSettingsEnum;


/** Messaging settings
 *
 */
typedef struct tagMessagingSettings
{
  CCSMMessagingFlagsType flags;  /**< app flags - see CCSMEmailFlagsEnum */

  // Email
  char *pop3P;                          /**< pop3 server for incoming mail */
  char *smtpP;                          /**< smtp server for outgoing mail */

  // SMS 
  char *smscNumberP;                    /**< SMSC Number */
  char *smsEmailGatewayP;               /**< SMS email center */

  // MMS
  char   *mmsURLP;                      /**< MMS URL */
  char   *mmsWAPGatewayP;               /**< MMS WAP Gateway */
  UInt32 mmsPort;                       /**< MMS Port number */
  UInt32 mmsMessageSizeLimit;           /**< Max MMS essage size in kB */
}
CCSMMessagingSettingsType;



/** enumeration for the browser settings flags
 *
 */
typedef UInt32 CCSMBrowserFlagsType;
typedef enum
{
  kCCSMBrowserSettingsReserved      = 0x00000000,  /**< reserved setting */
  kCCSMBrowserSettingsEncryptMSISDN = 0x00000001   /**< send encrypted MSISDN */
}
CCSMBrowserSettingsEnum;


/** browser settings
 *
 */
typedef struct tagBrowserSettings
{
  CCSMBrowserFlagsType flags;         /**< bitfield for app flags */
  char *homepageP;                    /**< homepage URL */

  char *primaryProxyP;                /**< Primary proxy address */
  UInt32 primaryProxyPort;            /**< Primary proxy port number */
  char *primarySecureProxyP;          /**< Primary proxy address */
  UInt32 primarySecureProxyPort;      /**< Primary proxy port number */

  char *secondaryProxyP;              /**< Secondary proxy address */
  UInt32 secondaryProxyPort;          /**< Secondary proxy port number */
  char *secondarySecureProxyP;        /**< Secondary proxy address */
  UInt32 secondarySecureProxyPort;    /**< Secondary proxy port number */

  char *staticHTTPHeadersP;           /**< static headers to send with an HTTP request */
  char *naiHeaderFormatStringP;       /**< header format to send the NAI */
  char *msisdnHeaderFormatStringP;    /**< header format to send the MSISDN */

  char *uaProfURLP;                   /**< Profiling URL */
  char *primaryTrustedDomainURLP;     /**< Primary trusted domain */
  char *secondaryTrustedDomainURLP;   /**< Secondary trusted domain */
}
CCSMBrowserSettingsType;


struct CCSMContextTypeTag;

#endif // __CARRIER_CUSTOMIZATION_LIBRARY_TYPES_H__
