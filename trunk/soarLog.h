#ifndef SOARLOG_H
#define SOARLOG_H

#define SLOG __attribute__ ((section ("slog")))

/*****************************************************************************
 * protos
 *****************************************************************************/
void PerformLogging(Boolean progexit) SLOG;
void MakeNewFlight() SLOG;
void OutputIGCHeader(FlightData *fltdata) SLOG;
void OutputIGCFltCRecs(FlightData *fltdata) SLOG;
void OutputIGCFltLRecs(FlightData *fltdata) SLOG;
void OutputIGCFltBRecs(Int32 startidx, Int32 stopidx, Int32 totalrecs) SLOG;
void OutputIGCFltFRec(LoggerData *logdata, Boolean firsttime) SLOG;
void OutputIGCInvalidFltLRec() SLOG;
Boolean CopyTaskToFlight() SLOG;
void CopyTaskRulesToFlight() SLOG;
Int16 FindFltNumOfDay(Char* fltdtg, Int16 flthour, Int16 fltminute, Int16 fltsecond) SLOG;
void StopLogging() SLOG;
double estlogtimeleft() SLOG;
void OutputSingleFlight(Int16 selectedFlt, Boolean dispAlert) SLOG;

#endif
