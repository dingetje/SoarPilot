#ifndef SOARPLST_H
#define SOARPLST_H

#define SPOL __attribute__ ((section ("spol")))

/*****************************************************************************
 * protos
 *****************************************************************************/
Int16 polar_comparison(PolarData* rec1, PolarData* rec2, Int16 order,
					        SortRecordInfoPtr rec1SortInfo, SortRecordInfoPtr rec2SortInfo,
					        MemHandle h) SPOL;
void refresh_polar_list(Int16 scr) SPOL;
Boolean save_polar_fields(double SinkConst, Boolean checkupd) SPOL;
Int16 FindPolarRecordByName(Char* NameString) SPOL;
void polar_parser(Char* serinp, UInt32 length, Boolean reset) SPOL;
void OutputPolarHeader() SPOL;
void OutputPolarRecords() SPOL;
Boolean CalcPolarABC(PolarData *polar, double bugfactor) SPOL;

#endif
