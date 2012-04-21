#ifndef SOARGAR_H
#define SOARGAR_H

#define SGAR __attribute__ ((section ("sgar")))

#define iQue3000  1 //3000
#define iQue3200  2 //3200
#define iQue3600  3 //3600
#define iQue3600a 4 //3700

/*****************************************************************************
 * prototypes
 *****************************************************************************/
Int16 iQueEnabledDevice() SGAR;
Boolean XferInitiQue() SGAR;
void XferCloseiQue() SGAR;
Err GpsLibEventCallback( SysNotifyParamType *notifyParamsP ) SGAR;
void GetiQueInfo() SGAR;

#endif
