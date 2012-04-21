#ifndef SOARTER_H
#define SOARTER_H

#define SMTH __attribute__ ((section ("smth")))

#define GCONST (double)(50.0/6000.0)

// controls fineness of final glide terrain grid
// 2 = about every km, bigger number = corser grid
// terrain heights are only every kilometer
#define TERGRID 2

typedef struct TerHeader {
	float ullat;
	float ullon;
	Int16  numrows;
	Int16  numcols;
} TerHeader;

/*****************************************************************************
 * protos
 *****************************************************************************/
void InitTerrainData() SMTH;
double GetTerrainElev(double curlat, double curlon) SMTH;
Boolean GetTerrainBounds(double *pointlat, double *pointlon, Int8 corner) SMTH;
void CleanUpTerrainData() SMTH;
Boolean loadterrain(double *terheights, Int32 numter, double origlat, double origlon, double origelev, double tgtlat, double tgtlon, double tgtelev) SMTH;
Boolean terraincrash(double *terheights, Int32 numter, double origlat, double origlon, double tgtlat, double tgtlon, double origalt, double tgtaalt, Boolean reset) SMTH;
Boolean tskterraincrash(double *terheights, Int16 activetskway, double startalt) SMTH;
Boolean loadtskterrain(TaskData *tertsk) SMTH;
#endif
