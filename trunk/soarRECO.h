#ifndef SOARRECO_H
#define SOARRECO_H

#define SCMP __attribute__ ((section ("scmp")))

/*
Example: $RECOA,264,103,0,v3.1,B0008,S1032870,uBlox-TIM*09
first value is 10* On-Chip temperature (31.8C)
second is decimal value of register, multiply by .11758 to get power voltage
third is Engine Noise Level (ENL)
fourth is ReCo firmware version
fifth is ReCo serial # (5 characters)
sixth is GPS serial #
seventh is GPS model
*/
typedef struct RECOData {
	double temp;
	double voltage;
	Int16  enl;
	Char   recofirmwarever[16];
	Char   recoserial[11];
	double cursecs;
} RECOData;

/*****************************************************************************
 * prototypes
 *****************************************************************************/
double RECOB2Alt(Char *altstr) SCMP;
void RECOEvent() SCMP;
Boolean form_config_recoinst_event_handler(EventPtr event) SCMP;

#endif
