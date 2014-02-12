#ifndef SOARGEOMAG_H
#define SOARGEOMAG_H

#define SMTH __attribute__ ((section ("smth")))

/*****************************************************************************
 * protos
 *****************************************************************************/
double CalcDeviation(float alt, float glat, float glon, float time) SMTH;
double GetDeviation() SMTH;
//void TestDeviation() SMTH;

#endif
