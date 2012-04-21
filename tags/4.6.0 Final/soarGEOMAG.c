#include <PalmOS.h>  // all the system toolbox headers
#include "soaring.h"
#include "soarGEOMAG.h"
#include "soarMAGCOFF.h"
#include "soarMath.h"
#include "soarUtil.h"
#include "Mathlib.h"

#define MAXDEG 12

/*************************************************************************/

double CalcDeviation(float alt, float glat, float glon, float time)
{
	int n,m,D3,D4;
	float tc[13][13],dp[13][13],
			sp[13],cp[13],pp[13],pi,dtr,a,b,re,
			a2,b2,c2,a4,b4,c4,otime,oalt,
			olat,olon,dt,rlon,rlat,srlon,srlat,crlon,crlat,srlat2,
			crlat2,q,q1,q2,ct=0.0,st=0.0,r2,r=0.0,d,ca=0.0,sa=0.0,aor,ar,br,bt,bpp,
			par,temp1,temp2,parp,bx;
	float by, bp;
	float *p = snorm;
	double dec=0.0;

	/* INITIALIZE CONSTANTS */
	sp[0] = 0.0;
	cp[0] = *p = pp[0] = 1.0;
	dp[0][0] = 0.0;
	a = (float)6378.137;
	b = (float)6356.7523142;
	re = (float)6371.2;
	a2 = a*a;
	b2 = b*b;
	c2 = a2-b2;
	a4 = a2*a2;
	b4 = b2*b2;
	c4 = a4 - b4;

	otime = oalt = olat = olon = -1000.0;
	dt = (float)(time - epoch);
	pi = (float)(3.14159265359);
	dtr = (float)(pi/180.0);
	rlon = (float)(glon*dtr);
	rlat = (float)(glat*dtr);
	srlon = (float)(sin(rlon));
	srlat = (float)(sin(rlat));
	crlon = (float)(cos(rlon));
	crlat = (float)(cos(rlat));
	srlat2 = srlat*srlat;
	crlat2 = crlat*crlat;
	sp[1] = srlon;
	cp[1] = crlon;

	/* CONVERT FROM GEODETIC COORDS. TO SPHERICAL COORDS. */
	if (alt != oalt || glat != olat) {
		q = (float)(Sqrt(a2-c2*srlat2));
		q1 = (float)(alt*q);
		q2 = ((q1+a2)/(q1+b2))*((q1+a2)/(q1+b2));
		ct = (float)(srlat/Sqrt(q2*crlat2+srlat2));
		st = (float)(Sqrt(1.0-(ct*ct)));
		r2 = (float)((alt*alt)+2.0*q1+(a4-c4*srlat2)/(q*q));
		r = (float)(Sqrt(r2));
		d = (float)(Sqrt(a2*crlat2+b2*srlat2));
		ca = (float)((alt+d)/r);
		sa = c2*crlat*srlat/(r*d);
	}
	if (glon != olon) { 
		for (m=2; m<=MAXDEG; m++) { 
			sp[m] = sp[1]*cp[m-1]+cp[1]*sp[m-1]; 
			cp[m] = cp[1]*cp[m-1]-sp[1]*sp[m-1]; 
		} 
	} 
	aor = re/r; 
	ar = aor*aor; 
	br = bt = bp = bpp = 0.0; 
	for (n=1; n<=MAXDEG; n++) { 
		ar = ar*aor; 
		for (m=0,D3=1,D4=(n+m+D3)/D3; D4>0; D4--,m+=D3) { 
			/*
				COMPUTE UNNORMALIZED ASSOCIATED LEGENDRE POLYNOMIALS
				AND DERIVATIVES VIA RECURSION RELATIONS
			*/
			if (alt != oalt || glat != olat) { 
				if (n == m) { 
					*(p+n+m*13) = st**(p+n-1+(m-1)*13); 
					dp[m][n] = st*dp[m-1][n-1]+ct**(p+n-1+(m-1)*13); 
				} else if (n == 1 && m == 0) {
					*(p+n+m*13) = ct**(p+n-1+m*13); 
					dp[m][n] = ct*dp[m][n-1]-st**(p+n-1+m*13); 
				} else if (n > 1 && n != m) { 
					if (m > n-2) *(p+n-2+m*13) = 0.0; 
					if (m > n-2) dp[m][n-2] = 0.0; 
					*(p+n+m*13) = ct**(p+n-1+m*13)-k[m][n]**(p+n-2+m*13); 
					dp[m][n] = ct*dp[m][n-1] - st**(p+n-1+m*13)-k[m][n]*dp[m][n-2]; 
				} 
			} 

			/*
				TIME ADJUST THE GAUSS COEFFICIENTS
			*/
			if (time != otime) { 
				tc[m][n] = c[m][n]+dt*cd[m][n]; 
				if (m != 0) tc[n][m-1] = c[n][m-1]+dt*cd[n][m-1]; 
			} 
			/*
				ACCUMULATE TERMS OF THE SPHERICAL HARMONIC EXPANSIONS
			*/
			par = ar**(p+n+m*13); 
			if (m == 0) { 
				temp1 = tc[m][n]*cp[m]; 
				temp2 = tc[m][n]*sp[m]; 
			} else { 
				temp1 = tc[m][n]*cp[m]+tc[n][m-1]*sp[m]; 
				temp2 = tc[m][n]*sp[m]-tc[n][m-1]*cp[m]; 
			} 
			bt = bt-ar*temp1*dp[m][n]; 
			bp += (fm[m]*temp2*par); 
			br += (fn[n]*temp1*par); 
			/*
				SPECIAL CASE:  NORTH/SOUTH GEOGRAPHIC POLES
			*/
			if (st == 0.0 && m == 1) { 
				if (n == 1) { pp[n] = pp[n-1]; 
				} else { 
					pp[n] = ct*pp[n-1]-k[m][n]*pp[n-2]; 
				} 
				parp = ar*pp[n]; 
				bpp += (fm[m]*temp2*parp); 
			} 
		}
	}
	if (st == 0.0) { 
		bp = bpp; 
	} else { 
		bp /= st; 
	} 
	/*
		ROTATE MAGNETIC VECTOR COMPONENTS FROM SPHERICAL TO
		GEODETIC COORDINATES
	*/
	bx = -bt*ca-br*sa; 
	by = bp; 
	/*
		COMPUTE DECLINATION (DEC), INCLINATION (DIP) AND
		TOTAL INTENSITY (TI)
	*/
	dec = Atan2(by,bx)/dtr;
	return(dec);
}

double GetDeviation()
{
	DateType dt;
	double pctyear;
	UInt32 days;

	// Find current days since 1904
	DateSecondsToDate(TimGetSeconds(), &dt);
	days = DateToDays(dt); // Gives me days since 1904 for current date

	// Find days since 1904 for Jan 1 of the current year
	dt.day = 1;
	dt.month = 1;
	days -= DateToDays(dt); // Gives me days since 1904 for Jan 1

	// Find the percentage of the current year for the current date
	pctyear = (double)(days) / 365.0;
//	HostTraceOutputTL(appErrorClass, "pctyear(partial)-|%s|", DblToStr(pctyear, 3));
	pctyear = pctyear + (double)dt.year + 1904;
//	HostTraceOutputTL(appErrorClass, "pctyear(complete)-|%s|", DblToStr(pctyear, 3));

//	HostTraceOutputTL(appErrorClass, "data.input.gpslatdbl-|%s|", DblToStr(data.input.gpslatdbl, 5));
//	HostTraceOutputTL(appErrorClass, "data.input.gpslngdbl-|%s|", DblToStr(data.input.gpslngdbl, 5));
	// Have to multiply the calculated value by -1.0.  I need East to be negative and West to be positive.
	data.input.deviation.value = CalcDeviation(0.0, data.input.gpslatdbl, data.input.gpslngdbl, pctyear) * -1.0;
	data.input.deviation.valid = VALID;
//	HostTraceOutputTL(appErrorClass, "data.input.deviation.value-|%s|", DblToStr(data.input.deviation.value, 4));

	return(data.input.deviation.value);
	
}

/*
void TestDeviation()
{
	DateType dt;
	double pctyear;
	UInt32 days;
	float alt=0.0;

	// Find current days since 1904
	DateSecondsToDate(TimGetSeconds(), &dt);
	days = DateToDays(dt); // Gives me days since 1904 for current date

	// Find days since 1904 for Jan 1 of the current year
	dt.day = 1;
	dt.month = 1;
	days -= DateToDays(dt); // Gives me days since 1904 for Jan 1

	// Find the percentage of the current year for the current date
	pctyear = (double)(days) / 365.0;
//	HostTraceOutputTL(appErrorClass, "pctyear(partial)-|%s|", DblToStr(pctyear, 3));
	pctyear = pctyear + (double)dt.year + 1904;
//	HostTraceOutputTL(appErrorClass, "pctyear(complete)-|%s|", DblToStr(pctyear, 3));
//	HostTraceOutputTL(appErrorClass, "50.88/6.89   0.1589-|%s|",
							DblToStr((CalcDeviation(alt, 50.88, 6.89, (float)pctyear)), 4));

//	pctyear = 2007.5;
//	HostTraceOutputTL(appErrorClass, "pctyear(test)-|%s|", DblToStr(pctyear, 3));
//	HostTraceOutputTL(appErrorClass, "alt(test)-|%s|", DblToStr(alt, 3));
//	HostTraceOutputTL(appErrorClass, "80/00       -6.92-|%s|",
							DblToStr((CalcDeviation(alt, 80.0, 0.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "80/60       35.84-|%s|",
							DblToStr((CalcDeviation(alt, 80.0, 60.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "80/120       0.55-|%s|",
							DblToStr((CalcDeviation(alt, 80.0, 120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "80/180       8.65-|%s|",
							DblToStr((CalcDeviation(alt, 80.0, 180.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "80/-120     31.25-|%s|",
							DblToStr((CalcDeviation(alt, 80.0, -120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "80/-60     -54.44-|%s|",
							DblToStr((CalcDeviation(alt, 80.0, -60.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "40/00       -0.98-|%s|",
							DblToStr((CalcDeviation(alt, 40.0, 0.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "40/60        5.08-|%s|",
							DblToStr((CalcDeviation(alt, 40.0, 60.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "40/120      -7.40-|%s|",
							DblToStr((CalcDeviation(alt, 40.0, 120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "40/180       5.79-|%s|",
							DblToStr((CalcDeviation(alt, 40.0, 180.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "40/-120     14.75-|%s|",
							DblToStr((CalcDeviation(alt, 40.0, -120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "40/-60     -17.79-|%s|",
							DblToStr((CalcDeviation(alt, 40.0, -60.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "00/00       -6.44-|%s|",
							DblToStr((CalcDeviation(alt, 00.0, 0.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "00/60       -4.13-|%s|",
							DblToStr((CalcDeviation(alt, 00.0, 60.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "00/120       1.08-|%s|",
							DblToStr((CalcDeviation(alt, 00.0, 120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "00/180       9.62-|%s|",
							DblToStr((CalcDeviation(alt, 00.0, 180.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "00/-120      9.13-|%s|",
							DblToStr((CalcDeviation(alt, 00.0, -120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "00/-60     -14.63-|%s|",
							DblToStr((CalcDeviation(alt, 00.0, -60.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-40/00     -23.43-|%s|",
							DblToStr((CalcDeviation(alt, -40.0, 0.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-40/60     -45.31-|%s|",
							DblToStr((CalcDeviation(alt, -40.0, 60.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-40/120     -3.36-|%s|",
							DblToStr((CalcDeviation(alt, -40.0, 120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-40/180     21.80-|%s|",
							DblToStr((CalcDeviation(alt, -40.0, 180.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-40/-120    22.46-|%s|",
							DblToStr((CalcDeviation(alt, -40.0, -120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-40/-60     -2.65-|%s|",
							DblToStr((CalcDeviation(alt, -40.0, -60.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-80/00     -21.77-|%s|",
							DblToStr((CalcDeviation(alt, -80.0, 0.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-80/60     -74.33-|%s|",
							DblToStr((CalcDeviation(alt, -80.0, 60.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-80/120   -140.75-|%s|",
							DblToStr((CalcDeviation(alt, -80.0, 120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-80/180    131.56-|%s|",
							DblToStr((CalcDeviation(alt, -80.0, 180.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-80/-120    70.39-|%s|",
							DblToStr((CalcDeviation(alt, -80.0, -120.0, (float)pctyear)), 4));
//	HostTraceOutputTL(appErrorClass, "-80/-60     23.86-|%s|",
							DblToStr((CalcDeviation(alt, -80.0, -60.0, (float)pctyear)), 4));
	return;
}
*/
