#ifndef SOARCOMP_H
#define SOARCOMP_H

#define SCMP __attribute__ ((section ("scmp")))
#define SCMP2 __attribute__ ((section ("scmp2")))

// Logger Commands
#define DECNOCMD	0
#define DECCLEAR	1
#define DECSEND		2
#define GETINFO		3
#define GETFLT		4
#define GETCONF		5
#define SENDCONF	6
#define GETWPTS		7
#define SENDWPTS	8
#define TOOFEWWPTS	9
#define TOOMANYWPTS	10
#define DATASPDERR	11
#define CONNECTERR	12
#define MEMORYERR	13
#define WPTNOTFOUND	14
#define DECQUESTION	15
#define CLRQUESTION	16

// Pressure Altitude Source Types
#define GPSALT		0
#define VOLKSALT	1
#define C302ALT		2
#define GARALT		3
#define LXALT		4
#define RECOALT		5
#define TASALT		6
#define GPSNAVALT	7
#define FLARMALT	8
#define SN10ALT		9
#define ZANDERALT	10
#define POSIALT		11
#define C302AALT	12
#define EWMRALT		13
#define B500ALT		14
#define WESTBALT	15

// Flight Computer types
#define NOCOMP		0
#define B50COMP		1
#define C302COMP	2
#define LXCOMP		3
#define EWCOMP		4
#define RECOCOMP	5
#define B50LXCOMP	6
#define IQUECOMP	7
#define TASCOMP		8
#define VOLKSCOMP	9
#define GPSNAVCOMP	10
#define FLARMCOMP	11
#define SN10COMP	12
#define FILSERCOMP	13
#define C302ACOMP	14
#define B50VLKCOMP	15
#define EWMRCOMP	16
#define EWMRSDCOMP	17
#define EWMRSDTASCOMP	18
#define LXVARCOMP	19
#define WESTBCOMP	20

/*****************************************************************************
 * prototypes
 *****************************************************************************/
Boolean form_list_flts_event_handler(EventPtr event) SCMP2;
void DeclareTaskToLogger(Int8 CompDecCmd) SCMP2;
Boolean compmsg(Int8 msgtype, Boolean success) SCMP2;
void refresh_flts_list(Int16 scr) SCMP2;
Boolean SetSelectedFlt(Int16 lstselection) SCMP2;

#endif
