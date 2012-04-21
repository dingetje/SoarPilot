#ifndef SOARSTF_H
#define SOARSTF_H

#define SCMP __attribute__ ((section ("scmp")))
#define SCMP2 __attribute__ ((section ("scmp2")))

/*****************************************************************************
 * protos
 *****************************************************************************/
void final_glide_event() SCMP;
void final_glide_event0() SCMP;
void final_glide_event1() SCMP;
void final_glide_event2() SCMP;
void final_glide_event3() SCMP;
void final_glide_event4() SCMP;
void final_glide_event5() SCMP;
Boolean CalcSTFSpdAlt(double inputSi, double distance, double bearing, double elevation, double startalt, Boolean usesafealt, double *stfspd, double *stfalt) SCMP;
Boolean CalcSTFSpdAlt2(double inputSi, double distance, double bearing, double *stfspd, double *stfalt) SCMP;
double ConvertAltType(double elevation, double startalt, Boolean usesafealt, Int8 alttype, double neededalt) SCMP;
void SetMCCurVal() SCMP;
void Update_Fltinfo(Int16 fltindex, Boolean listactive, Boolean fltinfo) SCMP;
void Update_Tskinfo_Event() SCMP;
void Update_Fltinfo_Event() SCMP;
void Update_Fltinfo_List(Boolean cleanup) SCMP;
void incMC(double OldMCVal) SCMP2;
void decMC(double OldMCVal) SCMP2;
#endif
