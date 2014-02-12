#ifndef SOARCOL_H
#define SOARCOL_H
/**
* \file soarCOL.h
* \brief SoarPilot Colibri header file
*/

#define SCMP2 __attribute__ ((section ("scmp2")))

#define CRC_POLY				0x69
#define TPNUM					12
#define FLIGHTBLOCKSIZE				8192

#define LX_EOF					2			// end of file

// command characters on serial port
#define LX_SYN					0x16		// LX_SYN character on serial port
#define LX_ACK					0x06		// LX_ACK character on serial port
#define LX_STX					0x02		// LX_STX character on serial port
#define LX_RCHAR				0x52		// 'R' character on serial port
#define LX_SECRETKEY				0xC7
#define LX_DELETE				0xC8
#define LX_PCREAD				0xC9
#define LX_PCWRITE				0xCA
#define LX_DECRYPT				0xCB		// decrypt block
#define LX_BSIZES				0xCC
#define LX_FLLIST				0xCD		// read list of flights
#define LX_FLTPOS				0xCE		// selection of download position
#define LX_CCREAD				0xCF
#define LX_CCWRITE				0xD0
#define LX_MEMINFO				0xD1		// get memory info of the instrument

#define LX_BLOCK0				0xD2
#define LX_BLOCK1				0xD3
#define LX_BLOCK2				0xD4
#define LX_BLOCK3				0xD5
#define LX_BLOCK4				0xD6
#define LX_BLOCK5				0xD7
#define LX_BLOCK6				0xD8
#define LX_BLOCK7				0xD9
#define LX_BLOCK15				0xE1

#define LX_FLB0					0xE6		// flight-log blocks
#define LX_FLB1					0xE7
#define LX_FLB2					0xE8
#define LX_FLB3					0xE9
#define LX_FLB4					0xEA
#define LX_FLB5					0xEB
#define LX_FLB6					0xEC
#define LX_FLB7					0xED

// LX Data Types
#define LX_FLIGHTDATA				20
#define LX_WAYPOINTDATA				21
#define LX_GLIDERDATA				22
#define LX_PILOTDATA				23

// IGC constants
#define LX_MAX_LSTRING				63
#define LX_START				128
#define LX_DATUM				251
#define LX_SER_NR				246
#define LX_FLIGHT_INFO				253
#define LX_COMPETITION_CLASS			241
#define LX_SHVERSION				127
#define LX_FIXEXT_INFO				255
#define LX_EXTEND_INFO				254
#define LX_FIXEXT				249
#define LX_EXTEND				250
#define LX_TASK					247
#define LX_EVENT				244
#define LX_POSITION_OK				191
#define LX_POSITION_BAD				195
#define LX_ORIGIN				160
#define LX_END					64
#define LX_SECURITY_OLD				245
#define LX_SECURITY				240
#define LX_LOW_SECURITY				0x0D
#define LX_MED_SECURITY				0x0E
#define LX_HIGH_SECURITY			0x0F

// 3 letter codes
#define LX_FXA					0x0001
#define LX_VXA					0x0002
#define LX_RPM					0x0004
#define LX_GSP					0x0008
#define LX_IAS					0x0010
#define LX_TAS					0x0020
#define LX_HDM					0x0040
#define LX_HDT					0x0080
#define LX_TRM					0x0100
#define LX_TRT					0x0200
#define LX_TEN					0x0400
#define LX_WDI					0x0800
#define LX_WVE					0x1000
#define LX_ENL					0x2000
#define LX_VAR					0x4000
#define LX_XX3					0x8000


// Memory information for Colibri log transfers
typedef struct
{
	UInt16	startpos;
	UInt8	startpage;
	UInt16	endpos;
	UInt8	endpage;
} MemInfo;

// header of the flight-log
typedef struct
{
	UInt16		ser_nr;
	Int16		flt_nr;
	UInt8		day;
	UInt8		month;
	UInt16		year;
} LX_headerInfo;

// information about one flight
typedef struct
{
	UInt16	oo_id;
	Char	pilot[19];
	Char	glider[12];
	Char	reg_num[8];
	Char	cmp_num[4];
	UInt8	razred;
	Char	observer[10];
	UInt8	gpsdatum;
	UInt8	fix_accuracy;
	Char	gps[60];
} LX_flightInfo;

// position data from the log
typedef	struct
{
	UInt8	st;
	Int32	lat;
	Int32	lon;
	Int32	t;
	Int32	dt;
	Int16	aa;
	Int16	ga;
	UInt16	enl;
} LX_fixData;

// origin data from the log
typedef struct s_orig
{
	UInt8	flag;
	Int32	time;
	Int32	lat;
	Int32	lon;
} LX_originData;


// ******************************************************************************
// * Exported functions for the main SoarPilot program
// ******************************************************************************
Boolean COLDeclareTask() SCMP2;
Boolean COLGetFlightList(Int16 cmd) SCMP2;
Boolean COLDownloadFlight() SCMP2;

// ******************************************************************************
// * Colibri internal functions
// ******************************************************************************
void COLCopyWaypoint(Char *sendBuffer, UInt8 from, UInt8 to) SCMP2;
Char Calc_CRC(Char c) SCMP2;
Boolean COLSync() SCMP2;
Boolean COLPing() SCMP2;
Boolean COLSendBlock(UInt8 *block, UInt16 size) SCMP2;
Boolean COLReadBlock(UInt8 *block, UInt16 size, UInt16 timeout) SCMP2;
UInt16 COLGetFlightBlock(UInt32 blockStart, UInt32 blockEnd, UInt8 *buffer) SCMP2;
Boolean COLGetDecryptBlock(UInt8 *buffer) SCMP2;
Boolean COLDownloadFlightIGC() SCMP2;
Boolean COLDownloadFlightLXN() SCMP2;
void COLStoreFlightHeader(UInt8* buffer, UInt16 ind) SCMP2;
Boolean COLGetMemInfo() SCMP2;
Boolean COLGetBinaryDataForIGC(UInt8 *data, UInt32 numBytes) SCMP2;

#endif

