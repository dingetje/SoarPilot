#ifndef SOARING_H
#define SOARING_H

#define SFM __attribute__ ((section ("sfm")))
#define SFM2 __attribute__ ((section ("sfm2")))
#define SFM3 __attribute__ ((section ("sfm3")))
#define SFM4 __attribute__ ((section ("sfm4")))
#define SIO __attribute__ ((section ("sio")))
#define SIO2 __attribute__ ((section ("sio2")))

// must have writeable SD card in slot!
// cannot use other VFS file operations!
// only one log file at a time!
// used for logging serial input for testing
//#define NMEALOG
// used for SUA parsing debugging
//#define SUALOG
// used to output simple lat / lon / alt data
//#define POSITIONLOG

#define VISORSUPPORT
#define TREOSUPPORT

#define STNONE		0
#define STWINMOBILE	1
#define STIPHONE	2

#define GPSMAXSATS	12

#define LOWBATWARNLEVEL	15
#define LOWBATSTOPLEVEL	5

#define ISLAT		1
#define ISLON		0

#define LLDM		0 // Lat/Long ddd:mm.mmmc format
#define LLDMS		1 // Lat/Long ddd:mm:ss.ssc format
#define LLUTM		2 // UTM format

#define MAX_EVENT_SZ	10
#define MAXINT16	32767

#define NOT_VALID	0
#define VALID		1
#define ESTIMATED	2

#define NFC		0
#define HFC		1

#define OK		0
#define CANCEL		1
#define ADD		0
#define DELETE		2
#define YES		0
#define NO		1
#define CONTINUE	0
#define EXIT		1
#define QUIT		1

#define TASKADD		0
#define TASKREMOVE	1
#define TASKEDIT	2

#define TRACKUP		0
#define NORTHUP		1
#define COURSEUP	2

#define NEXTALL		2
#define NEXTCTL		1
#define NEXTOFF		0

#define NORMAL		0
#define POPUP		1

//Values used to define the bounds of speeds to sample
//for STF and Final Glide Calcs.  Also used for the beginning
//min and max speeds for wind calculations.
#define MIN_SPEED	20.0
#define MAX_SPEED	200.0

//Use these to convert from knots to different units
#define SPDMPHCONST	1.151
#define SPDNAUCONST	1.0
#define SPDKPHCONST	1.852
#define SPDMPSCONST	0.5144444

#define DISTMICONST	1.151
#define DISTNAUCONST	1.0
#define DISTKMCONST	1.852
#define DISTFTCONST	6076.12

#define ALTMPHNAUCONST	1.0
//Multiply feet by constant to get meters
#define ALTMETCONST 	0.3048

#define AIRFPMCONST	101.2686
#define AIRKTSCONST	1.0
#define AIRMPSCONST	0.5144444

#define AIRFPMINCR	50.0 /* 50 feet/sec in knots */
#define AIRKTSINCR	0.5
#define AIRMPSINCR	0.25 /* 0.25 m/sec in knots */
#define AIRC302INCR	(double)0.20000000000

#define WGTLBSCONST	1.0
#define WGTKGCONST	0.4535924

#define WTRGALCONST	1.0
#define WTRLTRCONST	3.785412
#define WTRIMPCONST	0.8326738

#define STATUTE		0
#define NAUTICAL	1
#define METRIC		2
#define METRICMPS	3

#define MILLIBARS	0
#define INHG		1

#define FTPERMB		27.96 // ft per mb near sea level for QNH corrections
#define MBPERIN		33.86388 // mb per in of mercury
#define INPERMB		0.029529989 // inches of mercury per millibar

#define GRAVITY_CONST	9.80665 // standard gravity constant

// Defined like this based on the 302 Dataport Guide
#define FARENHEIT	0
#define CELCIUS		1

#define BATX		145
#define BATY		12
#define LOGX		131
#define LOGY		12
#define GPSX		104
#define GPSY		12
#define GPSXOFF		40
#define GPSYOFF		12
#define BLX		93
#define BLY		12

// Start, Turnpoint & Finish Sector Types
#define CYLINDER	0
#define FAI		1
#define TSKLINE		2
#define BOTH		3
#define ARC		4

// Earth Model Types
#define EMSPHERE	0
#define EMWGS84		1
#define EMFAI		2

// Altitude Reference Types
#define MSL		0
#define QFE		1
#define AGL		2
#define PALT		3

// GPS Altitude Reference Types
#define GPSMSL		0
#define GPSWGS84	1

/* Pounds Per Gallon of Water */
#define PPGW		8.3453

// defines for user configurable find button chain
#define SCREENCHAINMAX	10	// number of items in the chain

// max length of line for parsing
#define PARSELEN	512

#define appcreator		'Soar'

#define config_db_name		"SoaringPilot_config_db\0"
#define logger_db_name		"SoaringPilot_logger_db\0"
#define flight_db_name		"SoaringPilot_flight_db\0"
#define waypoint_db_name	"SoaringPilot_waypoint_db\0"
#define polar_db_name		"SoaringPilot_polar_db\0"
#define task_db_name		"SoaringPilot_task_db\0"
#define suaidx_db_name		"SoaringPilot_suaidx_db\0"
#define suadata_db_name		"SoaringPilot_suadata_db\0"
#define terrain_db_name		"SoaringPilot_terrain_db\0"

#define config_db_type		'SCfg'
#define logger_db_type		'SLog'
#define flight_db_type		'SFlt'
#define waypoint_db_type	'SWay'
#define polar_db_type		'SPol'
#define task_db_type		'STsk'
#define suaidx_db_type		'SSdx'
#define suadata_db_type		'SSua'
#define terrain_db_type		'STer'

#define memoDBName		"MemoDB\0"
#define memoDBType		'DATA'
#define docDBType		'TEXt'
#define docCreator		'REAd'

// Database Version Numbers
#define CONFIGDBVER	11.0

#define LOGGERDBVER	6.0	// should keep task, logger and flight databases the same
#define FLIGHTDBVER	6.0	//
#define TASKDBVER	6.0	//

#define WAYPOINTDBVER   4.0

#define SUAIDXDBVER	4.0	// should keep both SUA databases the same
#define SUADBVER	4.0	//

#define POLARDBVER	2.0

#define TERRAINDBVER	1.0

#define DOCDBVER	0

#define CARD_NO		0
#define POLAR_REC	0
#define CONFIG_REC	0
#define IGCHINFO_REC	1
#define CAIINFO_REC	2
#define RECOINFO_REC	3
#define EWMRINFO_REC	4
#define TASKACTIVE_REC	0
#define DOCZERO_REC	0

// Altitude Display Types
#define   REQALT	0
#define   ARVALT	1
#define   DLTALT	2

// Alert Types that use the genalert form
#define NULL_ALERT		0
#define FINALGLIDE_ALERT	30
#define SUAWARNING_ALERT	50
#define TASKSTART_ALERT		70
#define LOWBATWARN_ALERT	90

// Alert Warning Types : Ordered in priority
#define SUAWARN_NONE		0
#define SUAWARN_SNOOZE		10

#define SUAWARN_APPROACHING	30	// SUA warnings
#define SUAWARN_NEAR_BELOW	40
#define SUAWARN_BELOW		41
#define SUAWARN_NEAR_ABOVE	42
#define SUAWARN_ABOVE		43
#define SUAWARN_NEAR		44
#define SUAWARN_IN		45

#define SUAURGENT_NEAR_BELOW	50	// Urgent SUA warnings
#define SUAURGENT_BELOW		51
#define SUAURGENT_NEAR_ABOVE	52
#define SUAURGENT_ABOVE		53
#define SUAURGENT_NEAR		54
#define SUAURGENT_IN		55

#define SUAWARN_LOW		60	// SUA warnings for wave windows
#define SUAWARN_HIGH		61
#define SUAWARN_EDGE		62
#define SUAWARN_LOW_EDGE	63
#define SUAWARN_HIGH_EDGE	64

#define SUAURGENT_LOW		70	// Urgent SUA warnings for wave windows
#define SUAURGENT_HIGH		71
#define SUAURGENT_EDGE		72
#define SUAURGENT_LOW_EDGE	73
#define SUAURGENT_HIGH_EDGE	74

#define SUAWARN_ALL		75

#define GOTOSTART		80	// Higher priority than all SUA warnings to ensure display
#define RESTARTTASK		81
#define INVALIDSTART		82

#define LOWBATWARN		90	// Highest alert to force display
#define LOWBATSTOP		99

// Question Type Defines
#define Qnone		0
#define Qmanualstart	1
#define Qnewflight	2
#define QexitSP		3
#define Qdeacttask	4
#define Qaddnewwpt	5
#define Qdelwpt		6
#define QAAToverwrite	7
#define Qdelpolar	8
#define Qdeltask	9
#define Qclrtask	10
#define Qdeclaretask	11
#define Qcleardec	12
#define QdelSUA		13
#define Qacttaskchg	14
#define Qsetactivewpt	15
#define Qrulesoff	16
#define QturnAAT	17
#define QturnOLC	18
#define Qwindcalcoff	19

// Warning Type Defines
#define Wgeneric	0
#define Wfinished	1
#define Werror		2
#define Wlogtime	3
#define Winfo		4
#define Wcardwrite	5

// SMS Output Type Defines
#define SMSOUTGEN	0
#define SMSOUTPW	1
#define SMSSENDTO	0x0001
#define SMSSENDLAND	0x0002
#define SMSSENDPERIOD	0x0004
#define SMSSENDPROGEND	0x0008
#define SMSSENDNOW	0x0010

// OS defines
#define SYS_VER_30		sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0)
#define SYS_VER_32		sysMakeROMVersion(3,2,0,sysROMStageDevelopment,0)
#define SYS_VER_35		sysMakeROMVersion(3,5,0,sysROMStageDevelopment,0)
#define SYS_VER_40		sysMakeROMVersion(4,0,0,sysROMStageDevelopment,0)
#define SYS_VER_50		sysMakeROMVersion(5,0,0,sysROMStageDevelopment,0)
#define hsMinVersionSupported	0x3523040

#define reloadAppEvents		firstUserEvent

typedef void (*ParserType)(Char* string, UInt32 length, Boolean reset);
typedef void (*EventHandlerType)();

typedef struct DataEvent {
	EventHandlerType	event;
	int			valid;
} DataEvent;

typedef struct DataValue {
	double	value;
	int	valid;
} DataValue;

typedef struct PolarData{
	Char	name[16];
	double	v1;
	double	w1;
	double	v2;
	double	w2;
	double	v3;
	double	w3;
	double	a;
	double	b;
	double	c;
	double	maxdrywt;
	double	maxwater;
	double	Wsmin;
	double	Vmin;
} PolarData;

typedef struct Parser {
	ParserType	parser_func;
	Char		name[32];
} Parser; 

typedef struct TaskRules {
	Boolean	rulesactive;
	double	startwarnheight;
	double	maxstartheight;
	Int32	timebelowstart;
	Int32	mintasktime;
	double	finishheight;
	Boolean	fgto1000m;
	Int8	startaltref;
	Int8	finishaltref;
	Int32	startlocaltime;
	Boolean	startonentry;
	Boolean	warnbeforestart;
	Boolean	inclcyldist;
} TaskRules;

/* Configuration Information which will be stored */    
typedef struct ConfigFlight {
	double      bugfactor;
	double      pctwater;
	double      safealt;
	Int8        spdunits;
	Int8        disunits;
	Int8        altunits;
	Int8        lftunits;
	Int8        wgtunits;
	Int8        wtrunits;
	double      logstartspd;
	double      logstopspd;
	Int32       logstoptime;
	Boolean     logonoff;
	Boolean     logautooff;
	Int8	    mapscaleidx;
	Int8        timezone;
	UInt16	    nmeaspeed;
	UInt16	    dataspeed;
	Boolean	    flowctrl;
	Int8        alttype;
	Boolean     defaulttoFG;
	Boolean     usecalchw;
	Boolean     usepalmway;
	Boolean     setmcval;
	Boolean     optforspd;
	Int8        ldforcalcs;
	Boolean     btmlabels;
	Boolean     trktrail;
	Int8        thzoom;
	Boolean     thonoff; 
	Boolean     wayonoff;
	Boolean     wayline;
	Int8        waymaxlen;
	UInt16      numtrkpts;
	Int8        pressaltsrc;
	Boolean     usepalt;
	Int8        altreftype;
	Boolean     taskonoff;
	Int8	    taskdrawtype;
	Boolean     FlrmCopyIGC;
	Boolean     usechksums;
	Int8        starttype;
	double      startrad;
	double      startdir;
	Boolean     startdirmag;
	Boolean     startdirauto;
	Int8        turntype;
	double      turnfairad;
	double      turncircrad;
	Int8        finishtype;
	double      finishrad;
	double      finishdir;
	Boolean     finishdirmag;
	Boolean     finishdirauto;
	double	    qfealt;
	Boolean     usetas;
	Int8        flightcomp;
	Int8        earthmodel; 
	Boolean     usegpstime;
	double      SUAmaxalt;
	Int32       suaactivetypes;
	Boolean     windarrow;
	Boolean     inrngcalc;
	Int8        xfertype;
	Boolean     hwposlabel;
	double      mapcircrad1;
	double      mapcircrad2;
	Boolean     tskzoom;
	Int32       nodatatime;
	Int32       slowlogint;
	Int32       fastlogint;
	Boolean     keysoundon;
	Int16       mcrngmult;
	Int8        nmeaxfertype;
	Int8        llfmt;
	Int8        maporient;
	Int8        thmaporient;
	Boolean     useiquesim;
	Boolean     useiqueser;
	double      horiz_dist;
	double      vert_dist;
	Boolean     CheckSUAWarn;
	Int32       suawarntypes;  
	Int8        wndunits;
	double	    SUArewarn;
	UInt32      stcurdevcreator; 
	double      thzoomscale;
	UInt32      autodismisstime;
	double	    declutter;
	Boolean	    keepSUA;
	RGBColorType TaskColour;
	RGBColorType SUAColour;
	RGBColorType SectorColour;
	Boolean     BoldTask;
	Boolean	    BoldSUA;
	Boolean     BoldSector;
	Int8	    QNHunits;
	Boolean	    BoldTrack;
	Boolean	    autozeroQFE;
	double	    SUAdispalt;
	Int8	    shownextwpt;
	Int8        AATmode;
	Int8	    wpformat;
	Int8	    tskspdunits;
	UInt32	    SUAlookahead;
	Int8        sectormaporient;
	Int8	    thmapscaleidx;
	Int8        listlinesvert;
	Int8        listlineshoriz;
	double	    gpsmsladj;
	Int8        gpsaltref;
	Boolean	    ctllinevis;
	Boolean	    gpscalcdev;
	Int8	    Flarmscaleidx;
	Boolean     showrgs;
	Boolean	    calcwind;
	Boolean	    tskrestarts;
	Boolean	    usefgterrain;
	Boolean	    defaulthome;
	RGBColorType SUAwarnColour;
	Boolean     SUAdisponlywarned;
	Boolean     SUAhighlightwarned;
	Boolean     RefWptRadial;
	UInt16      thnumtrkpts;
	Int8	    windprofileorient;
	RGBColorType SinkColour;
	RGBColorType WeakColour;
	RGBColorType StrongColour;
	Boolean     dynamictrkcolour;
	Boolean     useinputwinds;
	Char        config_file[21];
	Char        waypt_file[21];
	Char        SUA_file[21];
	UInt16	   screenchain[SCREENCHAINMAX];
	Int16 	   profilescaleidx;
	Boolean    thermalprofile;
	Boolean	   showQFEQNHonstartup;
	Boolean	   outputSMS;
	Int8	   SMSouttype;
	Int16      SMSsendtype;
	Int32      SMSsendint;
	Char       SMSaddress[41];
	TaskRules  defaultrules;
	Boolean    totalenergy;
	Boolean    output_wypts;
	Boolean    output_tasks;
	Int8       leftaction;
	Boolean    autostart;
	Boolean    declaretasks;
	Boolean    fgtostartalt;
	Boolean    accuratedistcalc;
	RGBColorType WayptColour;
	Boolean    BoldWaypt;
	Int8       thermal_turns;
	Boolean    netto;
	Int8       SUAformat;
	Int8       mapRHfield;
	Boolean    FGalert;
	Boolean    echoNMEA;
	Boolean    autoIGCxfer;
	Boolean    declaretoSD;
	Int8       MCbutton;
} ConfigFlight;

/* GPS, Calculated & Transient Global Data*/
typedef struct InputData {
	DataValue  magnetic_track;
	DataValue  deviation;
	DataValue  true_track;
	DataValue  ground_speed;
	DataValue  distance_to_destination;
	DataValue  bearing_to_destination;
	Boolean    destination_valid;
	Char       destination_name[16];
	double     destination_elev;
	UInt16     destination_type;
	double     destination_lat;
	double     destination_lon;
	double     destination_aalt;
	double     headwind;
	double     basemc;
	double     curlift;
	double     nettolift;
	double     avglift;
	double     thavglift;
	double     avgnettolift;
	double     maxalt;
	double     minalt;
	Char       gpsnumsats[5];
	double     wndspd;
	double     wnddir;
	double     spdconst;
	double     disconst;
	double     altconst;
	double     lftconst;
	double     wgtconst;
	double     wtrconst;
	Char       spdtext[4];
	Char       distext[3];
	Char       alttext[5];
	Char       lfttext[4];
	Char       wgttext[4];
	Char       wtrtext[4];
	Int8       lftprec;
	double     lftincr;
	UInt32     logpolldate;
	DataValue  true_airspeed;
	double     ind_airspeed;
	double     compmc;
	double     vario;
	double     varioavg;
	double     variorel;
	double     bugs;
	double     ballast;
	Boolean    cruise;
	double	   inusealt;
	double	   comptruealt;
	double     compqnh;
	double     compwndspd;
	double     compwnddir;
	double     compheadwind;
	double     avgld;
	double     gpslatdbl;
	double     gpslngdbl;
	Int32      logpollint;
	double     terelev;
	Int8       siu; // Satellite in Use
	double     eph; // Horizontal Estimated Precision-2sigma
	Int8       svns[GPSMAXSATS]; // Space Vehicle Numbers being used
	Int8       curmaporient;
	UInt32     SUAnumrecs;
	double     wndconst;
	Char       wndtext[4];
	UInt32     SUAwarnrecs; 
	double     coslat;
	double	   QNHpressure;
	double	   QNHaltcorrect;
	double	   QNHconst;
	Char	   QNHtext[2];
	double     distdone;
	double	   startcruisealt;
	double	   startcruisedist;
	double	   lastglide;
	Boolean    isctlpt;
	double     nextwpt_dist;
	double     nextwpt_bear;
	Char	   nextwpt_text[5];
	double     areamaxlat;
	double     areamaxlon;
	double     areaminlat;
	double     areaminlon;
	double     startrad;
	double     startsect;
	double     startlat;
	double     startlon;	
//	double     finishrad;	// used for ARC finish (not implemented)
//	double     finishsect;	//
//	double     finishlat;	//
//	double     finishlon;	//
	double     tskspdconst;
	Char       tskspdtext[4];	
	double     gpsgeoidaldiff;
	Boolean    showterbox;
	UInt32     logtrkdate;
	double 	   homeLat;
	double	   homeLon;
	double 	   homeElev;
	double 	   refLat;
	double	   refLon;
	Int8	   refwpttype;
	UInt32     logsmsdate;
	double     startendlat1;
	double     startendlon1;
	double     startendlat2;
	double     startendlon2;
	double     finishendlat1;
	double     finishendlon1;
	double     finishendlat2;
	double     finishendlon2;
	UInt32     logfltupd;
	double     nextwpt_lat;
	double     nextwpt_lon;
	double     FGAdispalt;
	double     Vxc;
	double     newmc;
} InputData;

typedef struct ApplicationScreen {
	int		form_id;
	DataEvent	display_events[MAX_EVENT_SZ];
	int		changed;
} ApplicationScreen;

#define MAXWAYSPERTASK	25
#define TSKTEMPSLOT	25

typedef struct TaskData {
	Char	name[13];
	UInt16	numwaypts;
	Boolean	hastakeoff;
	Boolean	haslanding;
	Char	wayptnames[MAXWAYSPERTASK+1][13];
	double	wayptlats[MAXWAYSPERTASK+1];
	double	wayptlons[MAXWAYSPERTASK+1];
	double	elevations[MAXWAYSPERTASK+1];
	double	distances[MAXWAYSPERTASK+1];
	double	bearings[MAXWAYSPERTASK+1];
	Char	remarks[MAXWAYSPERTASK+1][13];
	UInt16	sectbear1[MAXWAYSPERTASK+1];
	UInt16	sectbear2[MAXWAYSPERTASK+1];
	double	arearadii[MAXWAYSPERTASK+1];
	Int16	wayptidxs[MAXWAYSPERTASK+1];
	double	ttldist;
	UInt16	waypttypes[MAXWAYSPERTASK+1];
	UInt16	numctlpts;
	double	arearadii2[MAXWAYSPERTASK+1];
	double	targetlats[MAXWAYSPERTASK+1];
	double	targetlons[MAXWAYSPERTASK+1];
	double	distlats[MAXWAYSPERTASK+1];
	double	distlons[MAXWAYSPERTASK+1];
	Boolean	rulesactive;
	double	startwarnheight;
	double	maxstartheight;
	Int32	timebelowstart;
	Int32	mintasktime;
	double	finishheight;
	Boolean	fgto1000m;
	Int8	startaltref;
	Int8	finishaltref;
	Int32	terrainidx[MAXWAYSPERTASK+1];
	Int32	startlocaltime;
	Boolean	startonentry;
	Boolean	warnbeforestart;
	Boolean	inclcyldist;
	Int8	aataimtype;
} TaskData;

typedef struct ActiveTaskData {
	Char	declaredtg[7];
	Char	declareutc[10];
	double	maxareadist[MAXWAYSPERTASK+1];
	double	arearng1;
	double	arearng2;
	double	distlats[MAXWAYSPERTASK+1];
	double	distlons[MAXWAYSPERTASK+1];
	double	alts[MAXWAYSPERTASK+1];
	double	stfs[MAXWAYSPERTASK+1];
	double	hwinds[MAXWAYSPERTASK+1];
	UInt32	times[MAXWAYSPERTASK+1];
	UInt32	etas[MAXWAYSPERTASK+1];
	UInt32	tskstartsecs;
	UInt32	tskstopsecs;
	UInt32	TOTsecs;
	double	FGAalt;
	double	FGAdist;
	double	tasktgtlat;
	double	tasktgtlon;
	double	AATtgtdist;
	Int8	FGstatus;
	double	startheight;
	double	finishheight;
} ActiveTaskData;

typedef struct LoggerData {
	Char	gpsutc[10];
	Char	gpslat[10];
	Char	gpslatdir[5];
	Char	gpslng[11];
	Char	gpslngdir[5];
	Char	gpsstat[5];
	double	pressalt;
	double	gpsalt;
	Char	gpsdtg[7];
	Boolean	thermal;
	Int16	taskleg;
	double	terelev;
	Int8	siu; 			// Satellites In Use
	double	eph;			// Horizontal Estimated Precision-2sigma
	Int8	svns[GPSMAXSATS];	// Space Vehicle Numbers being used
	Int16	enl;			// Engine noise level
	Boolean	event;		// pilot event
} LoggerData;

typedef struct IGCHInfo {
	Char	name[26];
	Char	type[16];
	Char	gid[11];
	Char	cid[6];
	Char	cls[13];
	Char	site[26];
	Char	gpsmodel[26];
	Char	gpsser[26];
	Char	ooid[26];
} IGCHInfo;

typedef struct FlightData {
	Char		startutc[10];
	Char		stoputc[10];
	Char		startdtg[7];
	Char		stopdtg[7];
	Int32		startidx;
	Int32		stopidx;
	double		maxalt;
	double		minalt;
	double		tskspeed;
	double		pctthermal;
	double		tskdist;
	TaskData	flttask;
	Char		tskstartutc[10];
	Char		tskstoputc[10];
	Char		tskstartdtg[7];
	Char		tskstopdtg[7];
	Int8		flightcomp;
	Char		fcfirmwarever[16];
	Char		fcserial[11];
	Boolean		valid;
	IGCHInfo	igchinfo;
	double		tskairspeed;
	UInt32		timeatturn[MAXWAYSPERTASK+1];
	Boolean		invalidturn[MAXWAYSPERTASK+1];
	double		startheight;
	Char		declaredtg[7];
	Char		declareutc[10];
} FlightData;

typedef struct WaypointData {
	double	lat;
	double	lon;
	double	distance;
	double	bearing;
	double	elevation;
	double	alt;
	UInt16	type;
	Char	name[13];
	Char	rmks[13];
	UInt16	arearadial1;
	Boolean	arearadial1mag;
	UInt16	arearadial2;
	Boolean	arearadial2mag;
	double	arearadius;
	Int16	rwdir;
	double	rwlen;
	Char	freq[10];
	Char	geninfo[64];
	double	arearadius2;
	Char	UsedTime[13];
} WaypointData;

typedef struct SUAIndex {
	Boolean	active;		// active for drawing?
	Char	title[26];
	Int32	startidx;	// index into SUAData file
	Int32	stopidx;
	Int32	type;		// used for type and class fields in new SUA file spec
	double	width;
	double	base;
	double	tops;
	double	reflat;		// used for the SUA element bounding ball
	double	reflon;
	double	maxdist;
	double	SUAmaxlat;	// used for the SUA element bounding box
	double	SUAminlat;
	double	SUAmaxlon;
	double	SUAminlon;
	Int32	basetype;	// define type of height value
	Int32	topstype;
	Int16	plottype;	// store type here to for quick access
	Int16	WarnType;
	Boolean	Dismissed;
	Boolean	WarnActive;	// active for warnings?
	UInt32	LastWarned;
	UInt32	ApproachLastWarned;
	double	neardist;
	Char	daysactive[26];	// new fields fields for latest SUA file spec
	Char	radiofreq[26];
	Int32	class;
	Boolean	warnonexit;
	Int8	topsunits;
	Int8	baseunits;
} SUAIndex;

typedef struct SUAData {
	Int16	plottype;
	double	lat;
	double	lon;
	double	radius;
	Boolean	cw;
	Int16	n;
	double	leftbrg;
	double	rightbrg;
} SUAData;

typedef struct SUAAlertData {
	Char	title[26];
	Char	displaytext[257];
	Char	btn0text[11];
	Char	btn1text[11];
	Char	btn2text[11];
	Int16	numbtns;
	Int16	priority;
	Int16	alertidx;
	Int16	alerttype;
} SUAAlertData;

typedef struct SUAAlertRet {
	Boolean	valid;
	Int16	btnselected;
	Int16	alertidxret;
	Int16	alerttype;
	Int16	priority;
} SUAAlertRet;

typedef struct QuestionData {
	Int16	type;
	Boolean	default_answer;
	Boolean	autodismiss;
} QuestionData;

typedef struct WarningData {
	Int16	type;
	Char	line1[40];
	Char	line2[40];
} WarningData;

typedef struct PalmData {
	UInt32	romVersion;
	Char	appVersion[32];
	Boolean	TreoPDA;
	Boolean	CliePDA;
	Boolean	StyleTapPDA;
	Boolean	VFSCapable;
	Boolean	BTCapable;
	Boolean	USBCapable;
	Int16	iQueCapable;
	Boolean	DIACapable;
	Boolean	CFCapable;
	Boolean	CardPresent;
	Boolean	CardRW;
	Boolean	NewSerialPresent;
	Boolean	NewSerialV2Present;
	UInt32	HiDensityScrPresent;
	Boolean	colorCapable;
	Boolean	CableAttachStatus;
	Int8	BTreconnects;
	Boolean	EXCapable;
	UInt8	lowbatlevel;
	UInt8	batpercent;
	UInt8	lowbatcount;
	Boolean	ENLCapable;
} PalmData;

typedef struct ApplicationData {
	Parser			parser;
	ConfigFlight		config;
	InputData		input;
	ApplicationScreen	application;
	TaskData		task;
	LoggerData		logger;
	FlightData		flight;
	WaypointData		inuseWaypoint;
	PolarData		polar;
	IGCHInfo		igchinfo;
	ActiveTaskData		activetask;
} ApplicationData;

typedef struct ScreenGlobals {
	double	SRES;
	Int16	WIDTH;
	Int16	HEIGHT;
	double	TOPCORNER;
	double	BOTTOMCORNER;
	double	BOTTOMCORNERLBL;
	double	TOPANGLE;
	double	BOTTOMANGLE;
	double	BOTTOMANGLELBL;
	Int16	GLIDERX;
	Int16	GLIDERY;
} ScreenGlobals;

// global externals
extern ApplicationData data;
extern PalmData device;
extern ScreenGlobals SCREEN;

// databases
extern DmOpenRef config_db;
extern DmOpenRef logger_db;
extern DmOpenRef flight_db;
extern DmOpenRef waypoint_db;
extern DmOpenRef polar_db;
extern DmOpenRef task_db;
extern DmOpenRef suaidx_db;
extern DmOpenRef suadata_db;
extern DmOpenRef terrain_db;

// variables for new alert dialogs
extern QuestionData *question;
extern WarningData *warning;

/*****************************************************************************
 * protos
 *****************************************************************************/
Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags);
Boolean CardEnabledDevice() SIO; 
Boolean BTEnabledDevice() SIO; 
Boolean DIAEnabledDevice() SIO;
void RegisterVolumeNotification(Boolean register) SIO;
void RegisterWinResizedEventNotification(Boolean reg) SIO;
void RegisterCableAttachDetachNotification(Boolean reg) SIO;
Err DisplayResizedEventCallback( SysNotifyParamType *notifyParamsP ) SIO;
Err CableAttachHandler( SysNotifyParamType *notifyParamsP ) SIO;
Err CableDetachHandler( SysNotifyParamType *notifyParamsP ) SIO;
Boolean ApplicationHandleEvent(EventPtr event) SIO;
Boolean PreprocessEvent(EventPtr event) SIO2;
Boolean form_genalert_event_handler(EventPtr event) SIO;

Boolean form_final_glide_event_handler(EventPtr event) SFM;
Boolean form_moving_map_event_handler(EventPtr event) SFM;
Boolean form_flt_info_event_handler(EventPtr event) SFM3;
Boolean form_graph_flt_event_handler(EventPtr event) SFM;
Boolean form_wind_disp_event_handler(EventPtr event) SFM2;
Boolean form_wind_spddir_event_handler(EventPtr event) SFM3;
Boolean form_wind_3dinfo_event_handler(EventPtr event) SFM2;
Boolean form_list_waypt_event_handler(EventPtr event) SFM;
Boolean form_av_waypt_event_handler(EventPtr event) SFM;
Boolean form_waypt_addinfo_event_handler(EventPtr event) SFM2;
Boolean form_set_task_event_handler(EventPtr event) SFM2;
Boolean save_task_data(Int16 next_form) SFM2;
Boolean form_list_polar_event_handler(EventPtr event) SFM3;
Boolean form_av_polar_event_handler(EventPtr event) SFM3;
Boolean form_set_fg_event_handler(EventPtr event) SFM2;
Boolean form_set_units_event_handler(EventPtr event) SFM3;
Boolean form_set_logger_event_handler(EventPtr event) SFM;
Boolean form_config_task_event_handler(EventPtr event) SFM;
Boolean form_set_port_event_handler(EventPtr event) SFM4;
Boolean form_set_map_event_handler(EventPtr event) SFM2;
Boolean form_set_map_colours_event_handler(EventPtr event) SFM2;
Boolean form_set_map_track_event_handler(EventPtr event) SFM2;
Boolean form_set_pilot_event_handler(EventPtr event) SFM2;
void form_transfer_hide_fields(Boolean all) SFM;
Boolean form_transfer_event_handler(EventPtr event) SFM3;
void select_fg_waypoint(Int16 action) SFM3;
Boolean form_set_mc_event_handler(EventPtr event) SFM2;
Boolean form_wayselect_tr_event_handler(EventPtr event) SFM3;
Boolean form_wayselect_ta_event_handler(EventPtr event) SFM3;
Boolean form_wayselect_te_event_handler(EventPtr event) SFM3;
Boolean form_set_scrorder_event_handler(EventPtr event) SFM2;
Boolean form_set_sms_event_handler(EventPtr event) SFM2;
Boolean form_question_event_handler(EventPtr event) SFM4;
Boolean form_warning_event_handler(EventPtr event) SFM4;

#endif

