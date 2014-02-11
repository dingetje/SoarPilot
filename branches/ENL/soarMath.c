#include <PalmOS.h>
#include "soaring.h"
#include "soarMath.h"
#include "soarMap.h"
#include "soarUtil.h"
#include "soarRSA.h"
#include "Mathlib.h"

// Get two 32 bit ints from a double.
#define EXTRACT_WORDS(ix0,ix1,d) { (ix0)=__HI32(d); (ix1)=__LO32(d); }

// Get the more significant 32 bit int from a double.
#define GET_HIGH_WORD(i,d) (i)=__HI32(d)

// Get the less significant 32 bit int from a double.
#define GET_LOW_WORD(i,d) (i)=__LO32(d)

// Set a double from two 32 bit ints.
#define INSERT_WORDS(d,ix0,ix1) { __HI32(d)=(ix0); __LO32(d)=(ix1); }

// Set the more significant 32 bits of a double from an int.
#define SET_HIGH_WORD(d,v) __HI32(d)=(v)

// Set the less significant 32 bits of a double from an int.
#define SET_LOW_WORD(d,v) __LO32(d)=(v)

static const double one = 1.0, tiny=1.0e-300; 

#ifdef MATHLIB_FUNCS
double Sqrt(double x) {
	return(sqrt(x));
}
#else
double Sqrt(double x) {
    double z;
    const int32_t sign = (Int16)0x80000000;
    int32_t ix0,s0,q,m,t,i;
    u_int32_t r,t1,s1,ix1,q1;

    EXTRACT_WORDS(ix0,ix1,x);   

    /* take care of Inf and NaN */
    if((ix0&0x7ff00000)==0x7ff00000) {
		return x*x+x;               /* sqrt(NaN)=NaN, sqrt(+inf)=+inf
                                           sqrt(-inf)=sNaN */
    }
    /* take care of zero */
    if(ix0<=0) {
		if(((ix0&(~sign))|ix1)==0) return x;/* sqrt(+-0) = +-0 */
		else if(ix0<0)
			return (x-x)/(x-x);             /* sqrt(-ve) = sNaN */
    }
    /* normalize x */
    m = (ix0>>20);
    if(m==0) {                              /* subnormal x */
		while(ix0==0) {
			m -= 21;
			ix0 |= (ix1>>11); ix1 <<= 21;
		}
		for(i=0;(ix0&0x00100000)==0;i++) ix0<<=1;
		m -= i-1;
		ix0 |= (ix1>>(32-i));                                      
		ix1 <<= i;
    }
    m -= 1023;      /* unbias exponent */
    ix0 = (ix0&0x000fffff)|0x00100000;
    if(m&1){        /* odd m, double x to make it even */
		ix0 += ix0 + ((ix1&sign)>>31);
		ix1 += ix1;
    }
    m >>= 1;        /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
    ix0 += ix0 + ((ix1&sign)>>31);
    ix1 += ix1;
    q = q1 = s0 = s1 = 0;   /* [q,q1] = sqrt(x) */
    r = 0x00200000;         /* r = moving bit from right to left */

    while(r!=0) {
		t = s0+r;
		if(t<=ix0) {
			s0   = t+r;
			ix0 -= t;                       
			q   += r;
		}
		ix0 += ix0 + ((ix1&sign)>>31);
		ix1 += ix1;
		r>>=1;
    }

    r = sign;
    while(r!=0) {
		t1 = s1+r;
		t  = s0;
		if((t<ix0)||((t==ix0)&&(t1<=ix1))) {
			s1  = t1+r;
			if(((t1&sign)==sign)&&(s1&sign)==0) s0 += 1;
			ix0 -= t;
			if (ix1 < t1) ix0 -= 1;
			ix1 -= t1;
			q1  += r;
		}
		ix0 += ix0 + ((ix1&sign)>>31);
		ix1 += ix1;
		r>>=1;                            
    }

    /* use floating add to find out rounding direction */
    if((ix0|ix1)!=0) {
		z = one-tiny; /* trigger inexact flag */
		if (z>=one) {
			z = one+tiny;
			if (q1==(u_int32_t)0xffffffff) { q1=0; q += 1;}
			else if (z>one) {
				if (q1==(u_int32_t)0xfffffffe) q+=1;
				q1+=2;
			} else
				q1 += (q1&1);
		}
    }
    ix0 = (q>>1)+0x3fe00000;
    ix1 =  q1>>1;
    if ((q&1)==1) ix1 |= sign;
    ix0 += (m <<20);
    INSERT_WORDS(z,ix0,ix1);

    return z;               
}
#endif

#ifdef MATHLIB_FUNCS
double Sin(double x) {
	return(sin(x));
}

double Cos(double x) {
	return(cos(x));
}
#endif

#ifdef TABLE_FUNCS
static const float sin_tab[] = {
	0.000000, 0.012272, 0.024541, 0.036807,
	0.049068, 0.061321, 0.073565, 0.085797,
	0.098017, 0.110222, 0.122411, 0.134581,
	0.146730, 0.158858, 0.170962, 0.183040,
	0.195090, 0.207111, 0.219101, 0.231058,
	0.242980, 0.254866, 0.266713, 0.278520,
	0.290285, 0.302006, 0.313682, 0.325310,
	0.336890, 0.348419, 0.359895, 0.371317,
	0.382683, 0.393992, 0.405241, 0.416430,
	0.427555, 0.438616, 0.449611, 0.460539,
	0.471397, 0.482184, 0.492898, 0.503538,
	0.514103, 0.524590, 0.534998, 0.545325,
	0.555570, 0.565732, 0.575808, 0.585798,
	0.595699, 0.605511, 0.615232, 0.624860,
	0.634393, 0.643832, 0.653173, 0.662416,
	0.671559, 0.680601, 0.689541, 0.698376,
	0.707107, 0.715731, 0.724247, 0.732654,
	0.740951, 0.749136, 0.757209, 0.765167,
	0.773010, 0.780737, 0.788346, 0.795837,
	0.803208, 0.810457, 0.817585, 0.824589,
	0.831470, 0.838225, 0.844854, 0.851355,
	0.857729, 0.863973, 0.870087, 0.876070,
	0.881921, 0.887640, 0.893224, 0.898674,
	0.903989, 0.909168, 0.914210, 0.919114,
	0.923880, 0.928506, 0.932993, 0.937339,
	0.941544, 0.945607, 0.949528, 0.953306,
	0.956940, 0.960431, 0.963776, 0.966976,
	0.970031, 0.972940, 0.975702, 0.978317,
	0.980785, 0.983105, 0.985278, 0.987301,
	0.989177, 0.990903, 0.992480, 0.993907,
	0.995185, 0.996313, 0.997290, 0.998118,
	0.998795, 0.999322, 0.999699, 0.999925,
	1.000000
};

double Sin(double x) {
	const double percent = x/TWOPI;
	const Int32 idx_untrimmed = (percent+200)*512;
	const Int16 idx = idx_untrimmed % 512;
	double retval = 0;

	if (idx < 128)
		retval = sin_tab[idx];
	else if (idx < 256)
		retval = sin_tab[256 - idx];
	else if (idx < 384)
		retval = -sin_tab[idx - 256];
	else
		retval = -sin_tab[512 - idx];

	return(retval);
}

double Cos(double x) {
	const double percent = x/TWOPI;
	const Int32 idx_untrimmed = (percent+200)*512;
	const Int16 idx = idx_untrimmed % 512;
	double retval = 0;

	if (idx < 128)
		retval = sin_tab[128 - idx];
	else if (idx < 256)
		retval = -sin_tab[idx - 128];
	else if (idx < 384)
		retval = -sin_tab[384 - idx];
	else
		retval = sin_tab[idx - 384];

	return(retval);
}
#endif

#ifdef FUNCS_121
// *********************************************************
// ***
// ***   Routines to compute sine and cosine to 12.1 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		cos_121s computes cosine (x)
//
//  Accurate to about 12.1 decimal digits over the range [0, pi/2].
//  The input argument is in radians.
//
//  Algorithm:
//		cos(x)= c1 + c2*x**2 + c3*x**4 + c4*x**6 + c5*x**8 + c6*x**10 + c7*x**12
//   which is the same as:
//		cos(x)= c1 + x**2(c2 + c3*x**2 + c4*x**4 + c5*x**6 + c6*x**8 + c7*x**10)
//		cos(x)= c1 + x**2(c2 + x**2(c3 + c4*x**2 + c5*x**4 + c6*x**6 + c7*x**8 ))
//		cos(x)= c1 + x**2(c2 + x**2(c3 + x**2(c4 + c5*x**2 + c6*x**4 + c7*x**6 )))
//		cos(x)= c1 + x**2(c2 + x**2(c3 + x**2(c4 + x**2(c5 + c6*x**2 + c7*x**4 ))))
//		cos(x)= c1 + x**2(c2 + x**2(c3 + x**2(c4 + x**2(c5 + x**2(c6 + c7*x**2 )))))
//
double cos_121s(double x)
{
	const double c1= 0.99999999999925182;
	const double c2=-0.49999999997024012;
	const double c3= 0.041666666473384543;
	const double c4=-0.001388888418000423;
	const double c5= 0.0000248010406484558;
	const double c6=-0.0000002752469638432;
	const double c7= 0.0000000019907856854;

	double x2;							// The input argument squared

	x2=x * x;
	return (c1 + x2*(c2 + x2*(c3 + x2*(c4 + x2*(c5 + x2*(c6 + c7*x2))))));
}

//
//  This is the main cosine approximation "driver"
// It reduces the input argument's range to [0, pi/2],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
//
//double cos_121(double x){
double Cos(double x){
	Int16 quad;						// what quadrant are we in?

	x=Fmod(x, TWOPI);				// Get rid of values > 2* pi
	if(x<0)x=-x;					// cos(-x) = cos(x)
	quad=(Int16)(x * TWOOVERPI);			// Get quadrant # (0 to 3) we're in
	switch (quad){
		case 0: return  cos_121s(x);
		case 1: return -cos_121s(PI-x);
		case 2: return -cos_121s(x-PI);
		case 3: return  cos_121s(TWOPI-x);
		default: return  cos_121s(x);
	}
}

//
//   The sine is just cosine shifted a half-pi, so
// we'll adjust the argument and call the cosine approximation.
//
//double sin_121(double x){
double Sin(double x){
	return(Cos(HALFPI-x));
}
#endif

#ifdef FUNCS_32
// *********************************************************
// ***
// ***   Routines to compute sine and cosine to 3.2 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		cos_32s computes cosine (x)
//
//  Accurate to about 3.2 decimal digits over the range [0, pi/2].
//  The input argument is in radians.
//
//  Algorithm:
//		cos(x)= c1 + c2*x**2 + c3*x**4
//   which is the same as:
//		cos(x)= c1 + x**2(c2 + c3*x**2)
//
float cos_32s(float x)
{
	const float c1= 0.99940307;
	const float c2=-0.49558072;
	const float c3= 0.03679168;

	float x2;							// The input argument squared

	x2=x * x;
	return(c1 + x2*(c2 + c3 * x2));
}

//
//  This is the main cosine approximation "driver"
// It reduces the input argument's range to [0, pi/2],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
//
double Cos(double inputx)
{
	Int32 quad;						// what quadrant are we in?
	float x;

	
	x=(float)(Fmod(inputx, TWOPI));				// Get rid of values > 2* pi
	if(x<0)x=-x;					// cos(-x) = cos(x)
	quad=(Int32)(x * TWOOVERPI);			// Get quadrant # (0 to 3) we're in
	switch (quad){
		case 0: return((double)cos_32s(x));
		case 1: return((double)-cos_32s(PI-x));
		case 2: return((double)-cos_32s(x-PI));
		case 3: return((double)cos_32s(TWOPI-x));
		default: return((double)cos_32s(x));
	}
}

//
//   The sine is just cosine shifted a half-pi, so
// we'll adjust the argument and call the cosine approximation.
//
//float sin_32(float x){
double Sin(double x){
	return(Cos(HALFPI-x));
}

#endif

#ifdef MATHLIB_FUNCS
double Fabs(double x) {
	return(fabs(x));
}
#else
double Fabs(double x) {
	u_int32_t high;

	GET_HIGH_WORD(high,x);
	SET_HIGH_WORD(x,high&0x7fffffff);
	return x;
} 
#endif

double Fnabs (double value) {
	if (Fabs (value) > 1.0) return (( value < 0.0) ? -1.0 : 1.0);
	return(value);
}

#ifdef MATHLIB_FUNCS
double Fmod(double x, double y) {
	return(fmod(x, y));
}
#else
double Fmod(double x, double y) {
	return(x - y * Floor(x/y));
} 
#endif

static const double   
two54   =  1.80143985094819840000e+16, /* 0x43500000, 0x00000000 */
twom54  =  5.55111512312578270212e-17, /* 0x3C900000, 0x00000000 */
huge   = 1.0e+300;

#ifdef MATHLIB_FUNCS
double Floor(double x) {
	return(floor(x));
}
#else
double Floor(double x) {
    int32_t i0,i1,j0;
    u_int32_t i,j;
    EXTRACT_WORDS(i0,i1,x);
    j0 = ((i0>>20)&0x7ff)-0x3ff;
    if(j0<20) {
		if(j0<0) {  /* raise inexact if x != 0 */
		   if(huge+x>0.0) {/* return 0*sign(x) if |x|<1 */
				if(i0>=0) {
					i0=i1=0;
				} else if(((i0&0x7fffffff)|i1)!=0) { 
					i0=0xbff00000;i1=0;
				}
			}
		} else {
			i = (0x000fffff)>>j0;
			if(((i0&i)|i1)==0) return x; /* x is integral */
			if(huge+x>0.0) {        /* raise inexact flag */
				if(i0<0) i0 += (0x00100000)>>j0;
				i0 &= (~i); i1=0;
			}
		}
    } else if (j0>51) {
		if(j0==0x400) return x+x;   /* inf or NaN */
		else return x;              /* x is integral */
    } else {
		i = ((u_int32_t)(0xffffffff))>>(j0-20);
		if((i1&i)==0) return x;     /* x is integral */
		if(huge+x>0.0) {            /* raise inexact flag */
			if(i0<0) {
				if(j0==20) i0+=1;
				else {
					j = i1+(1<<(52-j0));
					if(j<i1) i0 +=1 ;       /* got a carry */
					i1=j;
				}
			}
			i1 &= (~i);
		}
    }
    INSERT_WORDS(x,i0,i1);
    return x;
} 
#endif

#ifdef MATHLIB_FUNCS
double Ceil(double x) {
	return(ceil(x));
}
#else
double Ceil(double x) {
	if (Floor(x) < x) {
		return(Floor(x)+1.0);
	} else {
		return(x);
	}
}	
#endif

#ifdef MATHLIB_FUNCS
double Atan(double x) {
	return(atan(x));
}
#else
double Atan(double x) {
    const double M_PI = 3.141592654;
    const double A0 = 0.9999992083;
    const double A1 = 0.3332870775;
    const double B1 = 0.5985998078;
    const double A2 = 0.0635500089;
    const double B2 = 0.3953544718;
    const double A0X = 0.2388229612;
    const double A1X = 2.445205396;
    const double B1X = 3.943529789;
    const double A2X = 1.314747223;
    const double B2X = 1.798249626;

    // correct to 3e-7. 4 multiplies/divides
    double x2;
    double v;

    x2 = x*x;
    if (x > 1.0) v = M_PI/2 - (A0 - A1/(x2 + B1 - A2/(x2 + B2)))/x;
    else if (x >= 0.0) v = x*(A0X + A1X/(x2 + B1X - A2X/(x2 + B2X)));
    else v = -Atan(-x);
    return v;
}
#endif

#ifdef MATHLIB_FUNCS
double Atan2(double y, double x) {
	return(atan2(y, x));
}
#else
double Atan2(double y, double x) {
	if (x > 0.0) {
		return(Atan(y/x));  
	} else if (x < 0.0 && y >= 0.0) {
		return(Atan(y/x) + PI);  
	} else if (x == 0.0 && y > 0.0) {
		return(HALFPI);  
	} else if (x < 0.0 && y < 0.0) {
		return(Atan(y/x) - PI);  
	} else if (x == 0.0 && y < 0.0) {
		return(HALFPI * -1.0);  
	} else if (x == 0.0 && y == 0.0) {
		return(0);
	} else {
		return(0);
	}
}
#endif

#ifdef MATHLIB_FUNCS
double Asin(double x) {
	return(asin(x));
}
#else
double Asin(double x) {
	return(2.0*Atan(x/(1.0+Sqrt(1.0-x*x))));  
}
#endif

#ifdef MATHLIB_FUNCS
double Acos(double x) {
	return(acos(x));
}
#else
double Acos(double x) {
	if (x >= 0.0) {
		return(2.0*Atan(Sqrt((1.0-x)/(1.0+x))));   
	} else {
		return(PI - 2.0*Atan(Sqrt((1.0+x)/(1.0-x))));
	}
//	return(PI/2.0 - Asin(x));
}
#endif

double acos(double x) {
	return(PI/2.0 - asin(x));
}

Boolean LoadUnloadMathLib(Boolean load)
{
	Err error;
	UInt16 usecount;

	if (load) {
		/* Load the MathLib System Library */
		error = SysLibFind(MathLibName, &MathLibRef);
		if (error)
			error = SysLibLoad(LibType, MathLibCreator, &MathLibRef);
		ErrFatalDisplayIf(error, "Can't find MathLib"); // Just an example; handle it gracefully
		error = MathLibOpen(MathLibRef, MathLibVersion);
		ErrFatalDisplayIf(error, "Can't open MathLib");
	} else {
		error = MathLibClose(MathLibRef, &usecount);
		ErrFatalDisplayIf(error, "Can't close MathLib");
		if (usecount == 0)
			SysLibRemove(MathLibRef);
	}
	return(true);
}



/*
	NN.C - natural numbers routines

*/

//#include <PalmOS.h>
//#include "soarRSA.h"
//#include "soarNN.h"

#ifndef USEASM
/* Decodes character string b into a, where character string is ordered
	 from most to least significant.

	 Lengths: a[digits], b[len].
	 Assumes b[i] = 0 for i < len - digits * NN_DIGIT_LEN. (Otherwise most
	 significant bytes are truncated.)
 */
void NN_Decode (a, digits, b, len)
NN_DIGIT *a;
unsigned char *b;
unsigned int digits, len;
{
  NN_DIGIT t;
  unsigned int i, u;
  int j;
  
            /* @##$ unsigned/signed bug fix added JSAK - Fri  31/05/96 18:09:11 */
  for (i = 0, j = len - 1; i < digits && j >= 0; i++) {
    t = 0;
    for (u = 0; j >= 0 && u < NN_DIGIT_BITS; j--, u += 8)
			t |= ((NN_DIGIT)b[j]) << u;
		a[i] = t;
  }
  
  for (; i < digits; i++)
    a[i] = 0;
}

/* Encodes b into character string a, where character string is ordered
   from most to least significant.

	 Lengths: a[len], b[digits].
	 Assumes NN_Bits (b, digits) <= 8 * len. (Otherwise most significant
	 digits are truncated.)
 */
void NN_Encode (a, len, b, digits)
NN_DIGIT *b;
unsigned char *a;
unsigned int digits, len;
{
	NN_DIGIT t;
    unsigned int i, u;
    int j;

            /* @##$ unsigned/signed bug fix added JSAK - Fri  31/05/96 18:09:11 */
    for (i = 0, j = len - 1; i < digits && j >= 0; i++) {
		t = b[i];
        for (u = 0; j >= 0 && u < NN_DIGIT_BITS; j--, u += 8)
			a[j] = (unsigned char)(t >> u);
	}

    for (; j >= 0; j--)
		a[j] = 0;
}

/* Assigns a = 0. */

void NN_AssignZero (a, digits)
NN_DIGIT *a;
unsigned int digits;
{
	if(digits) {
		do {
			*a++ = 0;
		}while(--digits);
	}
}

#endif

/* Assigns a = 2^b.

   Lengths: a[digits].
	 Requires b < digits * NN_DIGIT_BITS.
 */
void NN_Assign2Exp (a, b, digits)
NN_DIGIT *a;
unsigned int b, digits;
{
  NN_AssignZero (a, digits);

	if (b >= digits * NN_DIGIT_BITS)
    return;

  a[b / NN_DIGIT_BITS] = (NN_DIGIT)1 << (b % NN_DIGIT_BITS);
}

/* Computes a = b - c. Returns borrow.

	 Lengths: a[digits], b[digits], c[digits].
 */
NN_DIGIT NN_Sub (a, b, c, digits)
NN_DIGIT *a, *b, *c;
unsigned int digits;
{
	NN_DIGIT temp, borrow = 0;

	if(digits)
		do {
            /* Bug fix 16/10/95 - JSK, code below removed, caused bug with
               Sun Compiler SC4.

			if((temp = (*b++) - borrow) == MAX_NN_DIGIT)
                temp = MAX_NN_DIGIT - *c++;
            */

            temp = *b - borrow;
            b++;
            if(temp == MAX_NN_DIGIT) {
                temp = MAX_NN_DIGIT - *c;
                c++;
            }else {      /* Patch to prevent bug for Sun CC */
                if((temp -= *c) > (MAX_NN_DIGIT - *c))
					borrow = 1;
				else
					borrow = 0;
                c++;
            }
			*a++ = temp;
		}while(--digits);

	return(borrow);
}

/* Computes a = b * c.

	 Lengths: a[2*digits], b[digits], c[digits].
	 Assumes digits < MAX_NN_DIGITS.
*/

void NN_Mult (a, b, c, digits)
NN_DIGIT *a, *b, *c;
unsigned int digits;
{
	NN_DIGIT t[2*MAX_NN_DIGITS];
	NN_DIGIT dhigh, dlow, carry;
	unsigned int bDigits, cDigits, i, j;

	NN_AssignZero (t, 2 * digits);

	bDigits = NN_Digits (b, digits);
	cDigits = NN_Digits (c, digits);

	for (i = 0; i < bDigits; i++) {
		carry = 0;
		if(*(b+i) != 0) {
			for(j = 0; j < cDigits; j++) {
				dmult(*(b+i), *(c+j), &dhigh, &dlow);
				if((*(t+(i+j)) = *(t+(i+j)) + carry) < carry)
					carry = 1;
				else
					carry = 0;
				if((*(t+(i+j)) += dlow) < dlow)
					carry++;
				carry += dhigh;
			}
		}
		*(t+(i+cDigits)) += carry;
	}


	NN_Assign(a, t, 2 * digits);
}

/* Computes a = b * 2^c (i.e., shifts left c bits), returning carry.

	 Requires c < NN_DIGIT_BITS. */

NN_DIGIT NN_LShift (a, b, c, digits)
NN_DIGIT *a, *b;
unsigned int c, digits;
{
	NN_DIGIT temp, carry = 0;
	unsigned int t;

	if(c < NN_DIGIT_BITS)
		if(digits) {

			t = NN_DIGIT_BITS - c;

			do {
				temp = *b++;
				*a++ = (temp << c) | carry;
				carry = c ? (temp >> t) : 0;
			}while(--digits);
		}

	return (carry);
}

/* Computes a = c div 2^c (i.e., shifts right c bits), returning carry.

	 Requires: c < NN_DIGIT_BITS. */

NN_DIGIT NN_RShift (a, b, c, digits)
NN_DIGIT *a, *b;
unsigned int c, digits;
{
	NN_DIGIT temp, carry = 0;
	unsigned int t;

	if(c < NN_DIGIT_BITS)
		if(digits) {

			t = NN_DIGIT_BITS - c;

			do {
				digits--;
				temp = *(b+digits);
				*(a+digits) = (temp >> c) | carry;
				carry = c ? (temp << t) : 0;
			}while(digits);
		}

	return (carry);
}

/* Computes a = c div d and b = c mod d.

	 Lengths: a[cDigits], b[dDigits], c[cDigits], d[dDigits].
	 Assumes d > 0, cDigits < 2 * MAX_NN_DIGITS,
					 dDigits < MAX_NN_DIGITS.
*/

void NN_Div (a, b, c, cDigits, d, dDigits)
NN_DIGIT *a, *b, *c, *d;
unsigned int cDigits, dDigits;
{
	NN_DIGIT ai, cc[2*MAX_NN_DIGITS+1], dd[MAX_NN_DIGITS], s;
	NN_DIGIT t[2], u, v, *ccptr;
	NN_HALF_DIGIT aHigh, aLow, cHigh, cLow;
	int i;
	unsigned int ddDigits, shift;

	ddDigits = NN_Digits (d, dDigits);
	if(ddDigits == 0)
		return;

	shift = NN_DIGIT_BITS - NN_DigitBits (d[ddDigits-1]);
	NN_AssignZero (cc, ddDigits);
	cc[cDigits] = NN_LShift (cc, c, shift, cDigits);
	NN_LShift (dd, d, shift, ddDigits);
	s = dd[ddDigits-1];

	NN_AssignZero (a, cDigits);

	for (i = cDigits-ddDigits; i >= 0; i--) {
		if (s == MAX_NN_DIGIT)
			ai = cc[i+ddDigits];
		else {
			ccptr = &cc[i+ddDigits-1];

			s++;
			cHigh = (NN_HALF_DIGIT)HIGH_HALF (s);
			cLow = (NN_HALF_DIGIT)LOW_HALF (s);

			*t = *ccptr;
			*(t+1) = *(ccptr+1);

			if (cHigh == MAX_NN_HALF_DIGIT)
				aHigh = (NN_HALF_DIGIT)HIGH_HALF (*(t+1));
			else
				aHigh = (NN_HALF_DIGIT)(*(t+1) / (cHigh + 1));
			u = (NN_DIGIT)aHigh * (NN_DIGIT)cLow;
			v = (NN_DIGIT)aHigh * (NN_DIGIT)cHigh;
			if ((*t -= TO_HIGH_HALF (u)) > (MAX_NN_DIGIT - TO_HIGH_HALF (u)))
				t[1]--;
			*(t+1) -= HIGH_HALF (u);
			*(t+1) -= v;

			while ((*(t+1) > cHigh) ||
						 ((*(t+1) == cHigh) && (*t >= TO_HIGH_HALF (cLow)))) {
				if ((*t -= TO_HIGH_HALF (cLow)) > MAX_NN_DIGIT - TO_HIGH_HALF (cLow))
					t[1]--;
				*(t+1) -= cHigh;
				aHigh++;
			}

			if (cHigh == MAX_NN_HALF_DIGIT)
				aLow = (NN_HALF_DIGIT)LOW_HALF (*(t+1));
			else
				aLow =
			(NN_HALF_DIGIT)((TO_HIGH_HALF (*(t+1)) + HIGH_HALF (*t)) / (cHigh + 1));
			u = (NN_DIGIT)aLow * (NN_DIGIT)cLow;
			v = (NN_DIGIT)aLow * (NN_DIGIT)cHigh;
			if ((*t -= u) > (MAX_NN_DIGIT - u))
				t[1]--;
			if ((*t -= TO_HIGH_HALF (v)) > (MAX_NN_DIGIT - TO_HIGH_HALF (v)))
				t[1]--;
			*(t+1) -= HIGH_HALF (v);

			while ((*(t+1) > 0) || ((*(t+1) == 0) && *t >= s)) {
				if ((*t -= s) > (MAX_NN_DIGIT - s))
					t[1]--;
				aLow++;
			}

			ai = TO_HIGH_HALF (aHigh) + aLow;
			s--;
		}

		cc[i+ddDigits] -= subdigitmult(&cc[i], &cc[i], ai, dd, ddDigits);

		while (cc[i+ddDigits] || (NN_Cmp (&cc[i], dd, ddDigits) >= 0)) {
			ai++;
			cc[i+ddDigits] -= NN_Sub (&cc[i], &cc[i], dd, ddDigits);
		}

		a[i] = ai;
	}

	NN_AssignZero (b, dDigits);
	NN_RShift (b, cc, shift, ddDigits);
}


/* Computes a = b mod c.

	 Lengths: a[cDigits], b[bDigits], c[cDigits].
	 Assumes c > 0, bDigits < 2 * MAX_NN_DIGITS, cDigits < MAX_NN_DIGITS.
*/
void NN_Mod (a, b, bDigits, c, cDigits)
NN_DIGIT *a, *b, *c;
unsigned int bDigits, cDigits;
{
    NN_DIGIT t[2 * MAX_NN_DIGITS];
  
	NN_Div (t, a, b, bDigits, c, cDigits);
}

/* Computes a = b * c mod d.

   Lengths: a[digits], b[digits], c[digits], d[digits].
   Assumes d > 0, digits < MAX_NN_DIGITS.
 */
void NN_ModMult (a, b, c, d, digits)
NN_DIGIT *a, *b, *c, *d;
unsigned int digits;
{
    NN_DIGIT t[2*MAX_NN_DIGITS];

	NN_Mult (t, b, c, digits);
    NN_Mod (a, t, 2 * digits, d, digits);
}

/* Computes a = b^c mod d.

   Lengths: a[dDigits], b[dDigits], c[cDigits], d[dDigits].
	 Assumes d > 0, cDigits > 0, dDigits < MAX_NN_DIGITS.
 */
void NN_ModExp (a, b, c, cDigits, d, dDigits)
NN_DIGIT *a, *b, *c, *d;
unsigned int cDigits, dDigits;
{
    NN_DIGIT bPower[3][MAX_NN_DIGITS], ci, t[MAX_NN_DIGITS];
    int i;
	unsigned int ciBits, j, s;

	/* Store b, b^2 mod d, and b^3 mod d.
	 */
	NN_Assign (bPower[0], b, dDigits);
	NN_ModMult (bPower[1], bPower[0], b, d, dDigits);
    NN_ModMult (bPower[2], bPower[1], b, d, dDigits);
  
    NN_ASSIGN_DIGIT (t, 1, dDigits);

	cDigits = NN_Digits (c, cDigits);
    for (i = cDigits - 1; i >= 0; i--) {
		ci = c[i];
		ciBits = NN_DIGIT_BITS;

		/* Scan past leading zero bits of most significant digit.
		 */
		if (i == (int)(cDigits - 1)) {
			while (! DIGIT_2MSB (ci)) {
				ci <<= 2;
				ciBits -= 2;
			}
        }

        for (j = 0; j < ciBits; j += 2, ci <<= 2) {
        /* Compute t = t^4 * b^s mod d, where s = two MSB's of ci. */
            NN_ModMult (t, t, t, d, dDigits);
            NN_ModMult (t, t, t, d, dDigits);
            if ((s = DIGIT_2MSB (ci)) != 0)
            NN_ModMult (t, t, bPower[s-1], d, dDigits);
        }
    }
  
	NN_Assign (a, t, dDigits);
}

/* Compute a = 1/b mod c, assuming inverse exists.
   
   Lengths: a[digits], b[digits], c[digits].
	 Assumes gcd (b, c) = 1, digits < MAX_NN_DIGITS.
 */
void NN_ModInv (a, b, c, digits)
NN_DIGIT *a, *b, *c;
unsigned int digits;
{
    NN_DIGIT q[MAX_NN_DIGITS], t1[MAX_NN_DIGITS], t3[MAX_NN_DIGITS],
		u1[MAX_NN_DIGITS], u3[MAX_NN_DIGITS], v1[MAX_NN_DIGITS],
		v3[MAX_NN_DIGITS], w[2*MAX_NN_DIGITS];
    int u1Sign;

    /* Apply extended Euclidean algorithm, modified to avoid negative
       numbers.
    */
    NN_ASSIGN_DIGIT (u1, 1, digits);
	NN_AssignZero (v1, digits);
    NN_Assign (u3, b, digits);
	NN_Assign (v3, c, digits);
    u1Sign = 1;

	while (! NN_Zero (v3, digits)) {
        NN_Div (q, t3, u3, digits, v3, digits);
        NN_Mult (w, q, v1, digits);
		NN_Add (t1, u1, w, digits);
        NN_Assign (u1, v1, digits);
		NN_Assign (v1, t1, digits);
		NN_Assign (u3, v3, digits);
		NN_Assign (v3, t3, digits);
		u1Sign = -u1Sign;
	}

    /* Negate result if sign is negative. */
	if (u1Sign < 0)
		NN_Sub (a, c, u1, digits);
	else
		NN_Assign (a, u1, digits);
}

/* Computes a = gcd(b, c).

	 Assumes b > c, digits < MAX_NN_DIGITS.
*/

#define iplus1  ( i==2 ? 0 : i+1 )      /* used by Euclid algorithms */
#define iminus1 ( i==0 ? 2 : i-1 )      /* used by Euclid algorithms */
#define g(i) (  &(t[i][0])  )

void NN_Gcd(a ,b ,c, digits)
NN_DIGIT *a, *b, *c;
unsigned int digits;
{
	short i;
	NN_DIGIT t[3][MAX_NN_DIGITS];

	NN_Assign(g(0), c, digits);
	NN_Assign(g(1), b, digits);

	i=1;

	while(!NN_Zero(g(i),digits)) {
		NN_Mod(g(iplus1), g(iminus1), digits, g(i), digits);
		i = iplus1;
	}

	NN_Assign(a , g(iminus1), digits);
}

/* Returns the significant length of a in bits.

	 Lengths: a[digits]. */

unsigned int NN_Bits (a, digits)
NN_DIGIT *a;
unsigned int digits;
{
	if ((digits = NN_Digits (a, digits)) == 0)
		return (0);

	return ((digits - 1) * NN_DIGIT_BITS + NN_DigitBits (a[digits-1]));
}

#ifndef USEASM

/* Returns sign of a - b. */

int NN_Cmp (a, b, digits)
NN_DIGIT *a, *b;
unsigned int digits;
{

	if(digits) {
		do {
			digits--;
			if(*(a+digits) > *(b+digits))
				return(1);
			if(*(a+digits) < *(b+digits))
				return(-1);
		}while(digits);
	}

	return (0);
}

/* Returns nonzero iff a is zero. */

int NN_Zero (a, digits)
NN_DIGIT *a;
unsigned int digits;
{
	if(digits) {
		do {
			if(*a++)
				return(0);
		}while(--digits);
	}

	return (1);
}

/* Assigns a = b. */

void NN_Assign (a, b, digits)
NN_DIGIT *a, *b;
unsigned int digits;
{
	if(digits) {
		do {
			*a++ = *b++;
		}while(--digits);
	}
}

/* Returns the significant length of a in digits. */

unsigned int NN_Digits (a, digits)
NN_DIGIT *a;
unsigned int digits;
{

	if(digits) {
		digits--;

		do {
			if(*(a+digits))
				break;
		}while(digits--);

		return(digits + 1);
	}

	return(digits);
}

/* Computes a = b + c. Returns carry.

	 Lengths: a[digits], b[digits], c[digits].
 */
NN_DIGIT NN_Add (a, b, c, digits)
NN_DIGIT *a, *b, *c;
unsigned int digits;
{
	NN_DIGIT temp, carry = 0;

	if(digits)
		do {
			if((temp = (*b++) + carry) < carry)
				temp = *c++;
            else {      /* Patch to prevent bug for Sun CC */
                if((temp += *c) < *c)
					carry = 1;
				else
					carry = 0;
                c++;
            }
			*a++ = temp;
		}while(--digits);

	return (carry);
}

#endif

NN_DIGIT subdigitmult(a, b, c, d, digits)
NN_DIGIT *a, *b, c, *d;
unsigned int digits;
{
	NN_DIGIT borrow, thigh, tlow;
	unsigned int i;

	borrow = 0;

	if(c != 0) {
		for(i = 0; i < digits; i++) {
			dmult(c, d[i], &thigh, &tlow);
			if((a[i] = b[i] - borrow) > (MAX_NN_DIGIT - borrow))
				borrow = 1;
			else
				borrow = 0;
			if((a[i] -= tlow) > (MAX_NN_DIGIT - tlow))
				borrow++;
			borrow += thigh;
		}
	}

	return (borrow);
}

/* Returns the significant length of a in bits, where a is a digit. */

unsigned int NN_DigitBits (a)
NN_DIGIT a;
{
	unsigned int i;

	for (i = 0; i < NN_DIGIT_BITS; i++, a >>= 1)
		if (a == 0)
			break;

	return (i);
}

/* Computes a * b, result stored in high and low. */
 
void dmult( a, b, high, low)
NN_DIGIT          a, b;
NN_DIGIT         *high;
NN_DIGIT         *low;
{
	NN_HALF_DIGIT al, ah, bl, bh;
	NN_DIGIT m1, m2, m, ml, mh, carry = 0;

	al = (NN_HALF_DIGIT)LOW_HALF(a);
	ah = (NN_HALF_DIGIT)HIGH_HALF(a);
	bl = (NN_HALF_DIGIT)LOW_HALF(b);
	bh = (NN_HALF_DIGIT)HIGH_HALF(b);

	*low = (NN_DIGIT) al*bl;
	*high = (NN_DIGIT) ah*bh;

	m1 = (NN_DIGIT) al*bh;
	m2 = (NN_DIGIT) ah*bl;
	m = m1 + m2;

	if(m < m1)
        carry = 1L << (NN_DIGIT_BITS / 2);

	ml = (m & MAX_NN_HALF_DIGIT) << (NN_DIGIT_BITS / 2);
	mh = m >> (NN_DIGIT_BITS / 2);

	*low += ml;

	if(*low < ml)
		carry++;

	*high += carry + mh;
}

/* Tangent function, independent of trig routines used */

double Tan(double x)
{
	return(Sin(x)/Cos(x));
}
