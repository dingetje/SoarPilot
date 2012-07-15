#ifndef SOARCONF_H
#define SOARCONF_H
/**
* \file soarConf.h
* \brief SoarPilot configuration header file
*/
#define SCFG __attribute__ ((section ("scfg")))
/*****************************************************************************
 * defines 
 *****************************************************************************/

/*****************************************************************************
 * prototypes
 *****************************************************************************/
void OutputConfig() SCFG;
void config_parser(Char* serinp, UInt32 length, Boolean reset) SCFG;

#endif
