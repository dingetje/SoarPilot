#ifndef SOARSUA_H
#define SOARSUA_H

#define SSUA __attribute__ ((section ("ssua")))
#define SSUA2 __attribute__ ((section ("ssua2")))

// SUA Plot Types
#define SUAPOINT   0
#define SUACIRCLE  1
#define SUAARC     2
#define SUAAWY     3
#define SUATFR     4

#define SFC        -9999.00
#define UNLIMITED  99999.00

// SUA Altitude Types
#define SUA_ALT    0	// Altitude above mean sea level (GPS)
#define SUA_FL     1	// Altitude above pressure surface 1013.25mb
#define SUA_AGL    2	// Altitude above ground level
#define SUA_SFC    3    // At ground level
#define SUA_UNL    4    // Unlimited

// SUA priority level
#define SUAlow		0
#define SUAhigh		1

// SUA Airspace Types
// Done like this so they can be AND'd with a mask value
#define   SUANONE    0x00000000

#define   CTACTR     0x00000001
#define   AIRWAYS    0x00000002
#define   RESTRICTED 0x00000004
#define   PROHIBITED 0x00000008
#define   DANGER     0x00000010
#define   OTHER      0x00000020
#define   TRAINING   0x00000040
#define   TRAFFIC    0x00000080
#define   CLASSB     0x00000100
#define   CLASSC     0x00000200
#define   CLASSD     0x00000400
#define   CLASSE     0x00000800
#define   ALERT      0x00001000
#define   MOA        0x00002000
#define   GSEC       0x00004000
#define   TMA        0x00008000
#define   WARNING    0x00010000		// was TIA
#define   TEMPOR     0x00020000		// was TIZ
#define   CLASSA     0x00040000
#define   CLASSF     0x00080000
#define   TMZ        0x00100000	
#define   MATZ       0x00200000
#define   CLASSG     0x00400000
#define   BOUNDARY   0x00800000
#define   TFR        0x01000000
#define   CLASSX     0x02000000

#define   SUAERROR   0xF0000000
#define   SUAALL     0xFFFFFFFF

// SUA warning alert types defined in soaring.h

// SUA error types
#define SUAlatlonerror		1
#define SUAfirstlasterror	2
#define SUAtopsbaseerror	4

// Commands for refresh_sua_list function
#define SUADISP       0  // Display selected SUA item
#define SUAACTV       1  // Activate or Deactivate the selected SUA Item (toggle) from list screen
#define SUAVIEW       2  // View the selected SUA data in a separate window
#define SUAFREE       3  // Free the list points memory
#define SUAPGUP       4  // Move up one page worth of sua items
#define SUAPGDWN      5  // Move down one page worth of sua items
#define SUAUPNUM      6  // Update the SUA Selected Number
#define SUASORT       7  // Sort the SUA List 
#define SUAACTVD      8  // Activate or Deactivate the selected SUA Item (toggle) from display screen

// Various SUA List sort order types
#define SUASortByTitleA    0
#define SUASortByTitleD    1
#define SUASortByTypeA     2
#define SUASortByTypeD     3
#define SUASortByActiveA   4
#define SUASortByActiveD   5
#define SUASortByDistA     6
#define SUASortByDistD     7

// SUA file format
#define SUATNP		0
#define SUAOPENAIR	1

// used in vectors functions
typedef struct Point {
        double x,y; 
} Point;

/*****************************************************************************
 * protos
 *****************************************************************************/
void check_sua_elements() SSUA;
Boolean WarnSUA(UInt16 SUAelement, double curlat, double curlon, double curalt, double curFL, double curAGL, double vert_dist, double horiz_dist, double SUArewarn) SSUA;
void WarnSet(UInt16 SUAelement, Int16 relalt, Boolean inside, Boolean rewarn, Boolean wave) SSUA;
void WarnClear() SSUA;
void WarnAllSUA(double curlat, double curlon, double curalt, double curFL, double curAGL, double vert_dist, double horiz_dist, double SUArewarn) SSUA;
void ProcessSUA(Boolean afterparsing, Boolean reset) SSUA;
void DrawSUAElement(SUAIndex *suaidx, double gliderLat, double gliderLon, double xratio, double yratio, double ulRng, Boolean highlight, UInt8 offset) SSUA;
void DrawSUA(double gliderLat, double gliderLon, double xratio, double yratio, double ulRng) SSUA;
void SUA_parser_tnp(Char *serinp, UInt32 length, Boolean reset) SSUA;
Boolean GetTNPLatLon(Char *buf, Int8 i, Boolean spacedelim, double *lat, double *lon) SSUA2;
void SUA_parser_openair(Char *serinp, UInt32 length, Boolean reset) SSUA2;
Boolean GetOpenAirLatLon(Char *buf, Int8 i, Boolean spacedelim, double *lat, double *lon) SSUA2;
void SaveSUAData(Boolean idxdata) SSUA;
Boolean checkinout(double plat, double plong, UInt16 datastart, UInt16 datastop) SSUA;
double checkdist(double plat, double plong, UInt16 datastart, UInt16 datastop, double warndist) SSUA;
double isleft(double P0lat, double P0lon, double P1lat, double P1lon, double P2lat, double P2lon) SSUA;
double dot(double P1lat, double P1lon, double P2lat, double P2lon) SSUA;
double dist2seg(double Plat, double Plong, double SP0lat, double SP0long, double SP1lat, double SP1long, double *ilat, double* ilon) SSUA;
void FastBall() SSUA;
void GetSUATypeStr(Char * TypeString, Int32 type, Int16 maxlen, Boolean shortstr) SSUA;
void GetSUAClassStr(Char *TypeString, Int32 class) SSUA;
void GetSUAALTTypeStr(Char *TypeString, Int32 type, double alt, Int8 units, Int16 maxlen) SSUA;
Int16 sua_comparison(SUAIndex* rec1, SUAIndex* rec2, Int16 order, SortRecordInfoPtr rec1SortInfo, SortRecordInfoPtr rec2SortInfo, MemHandle h) SSUA2;
Int16 FindSUARecordByName(Char* NameString) SSUA;
Boolean form_set_sua_event_handler(EventPtr event) SSUA2;
void refresh_sua_list(Int16 action) SSUA2;
Boolean form_disp_sua_event_handler(EventPtr event) SSUA2;
Boolean form_set_suadisp_event_handler(EventPtr event) SSUA2;
Boolean form_set_suawarn_event_handler(EventPtr event) SSUA2;
void Update_QNHEvent() SSUA2;
Boolean form_set_qnh_event_handler(EventPtr event) SSUA2;
Boolean form_list_sua_event_handler(EventPtr event) SSUA2;
void Update_Altitudes() SSUA2;
double disttoSUAitem(SUAIndex *selectedSUA) SSUA2;
void saveSUAitem() SSUA;
Char* print_SUA_altitude(double val, Int8 units) SSUA;

#endif
