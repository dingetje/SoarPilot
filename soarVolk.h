#ifndef SOARVOLK_H
#define SOARVOLK_H

#define SVLK __attribute__ ((section ("svlk")))

#include "soarCAI.h"

// VLK Data Types
#define VLK_FLIGHTDATA    10
#define VLK_WAYPOINTDATA  11
#define VLK_GLIDERDATA    12
#define VLK_PILOTDATA     13

// data speed for initial connection (9600) used in soarComp.c
#define VLKspeed 3

// memory sizes
#define VLK_DBB_MEMSIZE 0x4000	
#define VLK_LOG_MEMSIZE 0x14000
#define MAX_MEMSIZE 0xFF00

#define VLK_LOGSTART  0x0000
#define VLK_DECLSTART 0x3000
#define VLK_DECLEND   0x4000

/* DS-Haupttypen */
#define rectyp_msk   0xE0 //Haupttypmaske
#define	rectyp_vrt   0x00 //Variabel, timed
#define	rectyp_vrb   0x20 //Variabel, untimed
#define	rectyp_sep   0x40 //separator (head)
#define rectyp_end   0x60 //Security
#define	rectyp_pos   0x80 //Pos-DS (Fix)
#define	rectyp_tnd   0xA0 //Time&Date
#define rectyp_fil   0xC0 //Fillzeichen
#define rectyp_poc   0xE0 //komprimierter Pos-DS

// field identifiers
#define FLDPLT   0x01 // pilots  16
#define FLDPLT1  0x01 // pilot 1 16
#define FLDPLT2  0x02 // pilot 2 16
#define FLDPLT3  0x03 // pilot 3 16
#define FLDPLT4  0x04 // pilot 4 16
#define FLDGTY   0x05 // glider type 12
#define FLDGID   0x06 // glider id 7
#define FLDCID   0x07 // comp id 3
#define FLDCCL   0x08 // comp class 12
#define FLDTZN   0x09 // time zone offset
#define FLDNTP   0x10 // number of turnpoints
#define FLDFDT   0x11 // flight date
#define FLDTID   0x12 // task id
#define FLDTKF   0x20 // takeoff
#define FLDSTA   0x21 // start
#define FLDFIN   0x22 // finish
#define FLDLDG   0x23 // landing
#define FLDTP1   0x31 // turnpoints 1 to 12
#define FLDTP2   0x32
#define FLDTP3   0x33
#define FLDTP4   0x34
#define FLDTP5   0x35
#define FLDTP6   0x36
#define FLDTP7   0x37
#define FLDTP8   0x38
#define FLDTP9   0x39
#define FLDTP10  0x3A
#define FLDTP11  0x3B
#define FLDTP12  0x3C
#define FLDHDR   0x50 // serial number
#define FLDEPEV  0x60 // event button pressed
#define FLDETKF  0x61 // takeoff detected

// flow- and format- control
#define	STX  0x02
#define	ETX  0x03
#define	ENQ  0x05
#define	ACK  0x06
#define	DLE  0x10
#define	NAK  0x15
#define	CAN  0x18

// commands PC -> Logger
#define	cmd_INF  0x00	// Information
#define	cmd_DIR  0x01	// Verzeichnis lesen
#define	cmd_GFL  0x02	// Flug lesen mit MD4
#define	cmd_GFS  0x03	// Flug lesen mit Signatur
#define	cmd_RDB  0x04	// Datenbank lesen
#define	cmd_WPR  0x05	// Parameter schreiben
#define	cmd_CFL  0x06	// Flugspeicher loeschen
#define	cmd_PDB  0x07	// Datenbank schreiben
#define	cmd_SIG  0x08	// Signatur berechnen und ausgeben
#define	cmd_ERO  0x09	// Emergency readout (Memorydump lesen)
#define	cmd_RST  0x0c 	// Restart Logger

// other defines
#define max_bfv 1
#define MFR_ID "GCS"   // manufacturer three-letter-code
#define MFR_ID2  "A"   // manufacturer letter

// Volkslogger data structures
// info and database records
typedef struct VLKinfo {
    	Char serialno[4];
	UInt8 fwmajor;
	UInt8 fwminor;
	UInt8 build;
	Int16 wptstart;
	Int16 wptend;
	UInt8 wptlen;
	Int16 numwpt; // max 500
	Int16 pltstart;
	Int16 pltend;
	UInt8 pltlen;
	Int16 numplt; // max 25
	Int16 rtestart;
	Int16 rteend;
	UInt8 rtelen;
	Int16 numrte; // max 25
	UInt16 declstart;
} VLKinfo; 

// waypoint
typedef struct VLKwpt {
	Char name[7];
	double lat;
	double lon;
	Int32 ilat;
	Int32 ilon;
	UInt8 typ;
} VLKwpt;

// declaration waypoint
typedef struct VLKdeclwpt {
	VLKwpt wpt; 
	Int16 lw; // line width
	Int16 rz; // cylinder radius
	Int16 rs; // sector radius
	Int16 ws; // sector direction
	UInt8 oztyp; // observation zone 0=Cylinder 1=Line
} VLKdeclwpt;

// route
typedef struct VLKroute {
	Char name[15];
	VLKwpt wpt[10];
} VLKroute;

// IGC header information
typedef struct VLKIGCheader {
  Char A[10],
       DTE[15],
       FXA[10],
       PLT[65],
       GTY[50],
       GID[50],
       RFW[30],
       RHW[30],
       FTY[60],
       DTM[25],
       CID[50],
       CCL[50],
       TZN[15];
} VLKIGCheader;

/*****************************************************************************
 * protos
 *****************************************************************************/
// links into main soarpilot program
Boolean VLKgetloginfo(Int16 cmd) SVLK;
Boolean VLKDownloadSelectedLog() SVLK;
Boolean DeclareVLKTask() SVLK;
Boolean ClearVLKDeclaration() SVLK;
Boolean DeleteVLKLogs() SVLK;

// volkslogger internal functions
UInt16 UpdateCRC(UInt8 Octet, UInt16 CRC) SVLK;
Boolean VLKconnect() SVLK;
UInt8 findVLKportspeed(UInt8 palmspeed) SVLK;
Boolean VLKsetportspeed(UInt8 portspeed) SVLK;
Int16 VLKsendcommand(UInt8 cmd, UInt8 param1, UInt8 param2) SVLK;
Boolean VLKReadDatabase() SVLK;
Boolean VLKwriteDatabase() SVLK;
Boolean VLKgetinfo(VLKinfo *volks) SVLK;
UInt8 readbuffer(Int32 position) SVLK;
void writebuffer(Int32 position, UInt8 data) SVLK;
Int32 VLKreadBIGblock(Int32 startpos, Int32 maxlen) SVLK;
Int32 VLKreadblock(UInt8 *buffer, Int32 startpos, Int32 maxlen) SVLK;
Int32 VLKwriteblock(UInt8 *buffer, Int32 maxlen) SVLK;
void VLKdbbinit(VLKinfo *volks) SVLK;
Int16 VLKAddDeclarationField(Int16 id, Int16 pos, UInt8 *data, Int16 datalen) SVLK;
Int16 VLKputdeclwp(Int16 wptidx, Int16 pos, UInt8 FID) SVLK;
void VLKsector(Int8 type, double fairad, double circrad, double sectdir, Int16 pos) SVLK;
void VLKputwp(VLKwpt *wpt, Int16 pos) SVLK;
Int16 VLKGetDeclarationField(Int16 id) SVLK;
void VLKgetwp(Int16 wpnum, VLKwpt *wpt) SVLK;
void VLKgetpilot(Char *pilot, Int16 pltnum) SVLK;
Int16 VLKconvdir(CAILogData *logdata, Int16 countonly) SVLK;
Boolean VLKconvIGC() SVLK;

#endif
