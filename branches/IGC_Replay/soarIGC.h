#ifndef SOARIGC_H
#define SOARIGC_H

#define SIGC __attribute__ ((section ("igc")))

/*****************************************************************************
 * protos
 *****************************************************************************/
void igc_parser(Char* serinp, UInt32 length, Boolean reset) SIGC;

#endif
