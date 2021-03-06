#ifndef SOARCAI_H
#define SOARCAI_H
/**
* \file soarCAI.h
* \brief SoarPilot Cambridge 302 header file
*/

#define SCAI __attribute__ ((section ("scai")))

#define C302speed 2

/*****************************************************************************
 * Cambridge "G" Record defines 
 *****************************************************************************/
#define GMC      0
#define GBALLAST 1
#define GBUGS    2
#define GALL     3

// CAI Unit Word Mask Constant
#define CAI_UNIT_VARIO_MS    0
#define CAI_UNIT_VARIO_KTS   1
#define CAI_UNIT_ALT_METERS  0
#define CAI_UNIT_ALT_FEET    2
#define CAI_UNIT_TEMP_C      0
#define CAI_UNIT_TEMP_F      4
#define CAI_UNIT_BARO_MILL   0
#define CAI_UNIT_BARO_INHG   8
#define CAI_UNIT_DIST_KM     0
#define CAI_UNIT_DIST_NM    16
#define CAI_UNIT_DIST_SM    32
#define CAI_UNIT_SPEED_KPH   0
#define CAI_UNIT_SPEED_KTS  64
#define CAI_UNIT_SPEED_MPH 128
#define CAI_VARIO_NETTO    247
#define CAI_VARIO_TE       251
#define CAI_VARIO_SNETTO   255

// CAI Waypoint Mask Constants
#define   CAITURN      0x01
#define   CAIAIRPORT   0x02
#define   CAIMARK      0x04
#define   CAILAND      0x08
#define   CAISTART     0x10
#define   CAIFINISH    0x20
#define   CAIHOME      0x40
#define   CAITHRML     0x80
#define   CAIWAYPOINT  0x100
#define   CAIAIRSPACE  0x200

// CAI 302 Data Types
#define CAI_FLIGHTDATA    0
#define CAI_WAYPOINTDATA  1
#define CAI_GLIDERDATA    2
#define CAI_PILOTDATA     3

// CAI GPSNAV Data Types
#define GPN_FLIGHTDATA    30
#define GPN_WAYPOINTDATA  31
#define GPN_GLIDERDATA    32
#define GPN_PILOTDATA     33

// CAI 302 Defines
#define GL_CAI_RETRY_CMD 3
#define GL_CAI_NAME_SIZE 24
#define CAI_SIGNATURE_SIZE 201
#define GL_CAI_BUFFER_SIZE 320
#define CAI_ID_SIZE        3
#define CAI_OSTYPE_SIZE    1
#define CAI_VER_SIZE       5
#define CAIDate DateType
#define CAIFISTART 0
#define CAIFIFREE 9999

#define CAINOTFOUND -1

/// CAI states
typedef enum {
	GL_CAI_UNKNOWN, 		///< unknown mode
	GL_CAI_PNP, 			///< Pocket-Nav mode
	GL_CAI_CMD, 			///< command mode
	GL_CAI_DOW, 			///< download mode
	GL_CAI_UPL				///< upload mode
} CAIState;

/// function typedef for input callback routine
typedef Boolean (*CAIInputCallback)(void *userData, Char *data, Int16 size);
/// function typedef for output callback routine
typedef Boolean (*CAIOutputCallback)(void *userData, Char *data, Int16 size);
/// function typedef for flush callback routine
typedef void (*CAIFlushCallback)(void *userData);

/// General CAI Data Structure
typedef struct {
	CAIState state;						///< state
	Int16 index;						///< index
	short blockSize;					///< block size
	Char checksum;						///< checksum
	unsigned short longChecksum;		///< long checksum
	Char buffer[GL_CAI_BUFFER_SIZE];	///< buffer
	void *userData;						///< user data
	CAIInputCallback input;				///< input callback routine
	CAIOutputCallback output;			///< output callback routine
	CAIFlushCallback flush;				///< flush callback routine
} CAIData;

/// CAI Time Data Structure
typedef struct {
	Int16 hour;							///< hour
	Int16 min;							///< minute
	Int16 sec;							///< second
	UInt32 totalsecs;					///< total number of seconds
} CAITime;

/// CAI Flight Log Structure
typedef struct {
   Int16 index;							///< index
   CAIDate startDate;					///< start date
   CAITime startTime;					///< start time
   CAIDate endDate;						///< end date
   CAITime endTime;						///< end time
   Char pilotName[GL_CAI_NAME_SIZE + 1];///< pilot name
   Int16 FlightOfDay;					///< flight number
   UInt16 StartTape;				 	///< for Colibri / Flarm
   UInt8  StartPage; 					///< for Colibri
   UInt16 EndTape; 						///< for Colibri
   UInt8  EndPage; 						///< for Colibri
   UInt16 SerialNo; 					///< for Colibri
   Char IGCname[16];					///< for Flarm
} CAILogData;

/// CAI Addition Data
typedef struct CAIAddData {
	double   stfdb;						///< ???
	double	 arvrad;					///< ???
	double	 apprad;					///< ???
	Int16	 tbtwn;						///<
	Boolean	 dalt;						///< destination altitude
	Boolean	 sinktone;					///< sink tone
	Boolean	 tefg;						///<
	Int8	 tempunits;					///< temperature unit
	Int8	 barounits;					///< pressure unit
	Int16    variotype;					///< vario type
	Boolean  pilotinfo;					///< pilot info
	Boolean  gliderinfo;				///< glider info
} CAIAddData;

/// CAI Generic Info
typedef struct {
	Char id[CAI_ID_SIZE+1];				///< identifier string
	Char ostype[CAI_OSTYPE_SIZE+1];		///< OS type
	Char ver[CAI_VER_SIZE+1];			///< version
} CAIGenInfo;

/*****************************************************************************
 * function prototypes
 *****************************************************************************/
void Output302GRec(Int8 item) SCAI;
Boolean DeclareCAITask() SCAI;
Boolean SendCAICommandStart() SCAI;
Boolean SendCAIFlightModeStart(Boolean dialog) SCAI;
Boolean SendCAIDownloadStart() SCAI;
Boolean SendCAIUploadStart() SCAI;
Boolean SendCAIDeclareEnd() SCAI;
Boolean SendCAITaskpoints() SCAI;
Boolean SendCAITaskpoint(UInt16 caiidx, UInt16 wayidx) SCAI;
Boolean SendCAIPilotInfo() SCAI;
Boolean SendCAIGliderInfo() SCAI;
Boolean UploadCAIWaypoints(Boolean ShowFinished) SCAI;
Boolean SendCAIWaypoints() SCAI;
Boolean SendCAIClearWaypoints() SCAI;
Boolean SendCAIWaypointEnd() SCAI;
Boolean SendCAISetSpeed(CAIData *caidata, Int32 newspeed) SCAI;
Boolean UploadCAIGliderInfo() SCAI;
Boolean UploadCAIPilotInfo() SCAI;
Boolean GetCAINumLogs(CAIData *data, Int16 *numLogs) SCAI;
Boolean GetGPNNumLogs(CAIData *data, Int16 *numLogs) SCAI;
Boolean GetCAILogInfoNext(CAIData *data, CAILogData *logData) SCAI;
Boolean GetCAIGenInfo(CAIData *data, CAIGenInfo *geninfo) SCAI;
Boolean DownloadCAILogInfo(Int16 cmd) SCAI;
Boolean DownloadCAISelectedLog() SCAI;
Int16 putString(CAIData *data, const Char *str) SCAI;
Boolean CAILogStart(CAIData *data, Int16 logIndex, Int16 *blockSize) SCAI;
Int16 CAILogRead(CAIData *data, Char *buffer, Int16 *valid) SCAI;
Int16 CAILogSignature(CAIData *data, Char *buffer, Int16 *valid) SCAI;
Int16 CAIFlightMode(CAIData *data) SCAI;
Int8 FindCAIFltNumOfDay(CAIDate *fltdate, CAITime *flttime) SCAI;
void GenerateCAIIGCName(CAIDate *fltdate, CAITime *flttime, Char *igcname) SCAI;
Boolean SendGPSNAVConfigInfo() SCAI;
Boolean ClearCAIWaypoints() SCAI;
Boolean ClearCAIFlights() SCAI;
Boolean SendCAIClearFlights() SCAI;
Boolean form_config_caiinst_event_handler(EventPtr event) SCAI;
Boolean form_config_gpsnavinst_event_handler(EventPtr event) SCAI;

#endif
