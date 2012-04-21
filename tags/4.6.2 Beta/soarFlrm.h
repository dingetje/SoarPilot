#ifndef SOARFLRM_H
#define SOARFLRM_H

#define SFLRM __attribute__ ((section ("sflrm")))

#define ByteSwap16(n) ( ((((unsigned int) n) << 8) & 0xFF00) | ((((unsigned int) n) >> 8) & 0x00FF) )

#define ByteSwap32(n) ( ((((unsigned long) n) << 24) & 0xFF000000) |  ((((unsigned long) n) <<  8) & 0x00FF0000) | ((((unsigned long) n) >>  8) & 0x0000FF00) | ((((unsigned long) n) >> 24) & 0x000000FF) )

// Flarm Data Types
#define FLRM_FLIGHTDATA    40
#define FLRM_WAYPOINTDATA  41	// not used
#define FLRM_GLIDERDATA    42	// not used
#define FLRM_PILOTDATA     43	// not used

typedef struct FlarmConfigData {
	Char DeviceID[7];
	Int8 RFFreq;
	Int8 RFGndTX;
	Int8 NMEAout;
	Int8 Baud;
	Int8 UserIF;
	Int8 Private;
	Int8 Type;
	Int8 Logging;
	Int16 Logint;
	Char Pilot[26];
	Char GliderType[16];
	Char GliderID[11];
	Char CompID[6];
	Char CompClass[13];
} FlarmConfigData;

/*****************************************************************************
 * protos
 *****************************************************************************/
Boolean FlarmSendConfig() SFLRM;
Boolean FlarmGetConfig() SFLRM;
Boolean FlarmGetFlightList(Int16 cmd) SFLRM;
Boolean FlarmDownloadFlight() SFLRM;
Boolean form_config_flarminst_event_handler(EventPtr event) SFLRM;
Boolean DeclareFlarmTask(Boolean declaretoSD) SFLRM;
Boolean ClearFlarmDeclaration(Boolean declaretoSD) SFLRM;

Boolean FlarmSendIGCInfo(Boolean declaretoSD) SFLRM;
UInt16 Calc_crc(UInt8 *data, UInt16 len) SFLRM;
UInt16 crc_update(UInt16 crc, UInt8 data) SFLRM;
Boolean Send_command(UInt8 *cmdarray, UInt16 len) SFLRM;
UInt16 Wait_Frame_Start() SFLRM;
Boolean Read_Frame(UInt8 *dataarray, UInt16 len) SFLRM;
Boolean Flarm_reset() SFLRM;
Boolean Flarm_ping() SFLRM;
Boolean Flarm_Version() SFLRM;
Boolean Flarm_binary_connect() SFLRM;
Boolean FlarmSetPortSpeed(UInt8 requestedspeed) SFLRM;
Boolean FlarmDataErr(Boolean declaretoSD) SFLRM;

#endif
