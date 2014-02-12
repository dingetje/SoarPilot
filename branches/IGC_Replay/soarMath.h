#ifndef SOARMATH_H
#define SOARMATH_H

#define SMTH __attribute__ ((section ("smth")))
#define SCRPT __attribute__ ((section ("scrpt")))

#define max( a, b )	( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#define min( a, b )	( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )

// Used to force all math functions to use the MatLib Equivalents
//#define MATHLIB_FUNCS
#define TABLE_FUNCS
//#define FUNCS_121
//#define FUNCS_32

#define INT_TO_FIXED(x)         ((x) << 16)
#define DOUBLE_TO_FIXED(x)      ((long)(x * 65536.0 + 0.5))
#define FIXED_TO_INT(x)         ((x) >> 16)
#define FIXED_TO_DOUBLE(x)      (((double)x) / 65536.0)
#define ROUND_FIXED_TO_INT(x)   (((x) + 0x8000) >> 16)

#define PI              ((double)3.1415926535897932384626433)
#define TWOPI           ((double)2.0*PI)      // pi times 2
#define TWOOVERPI       ((double)2.0/PI)      // 2/pi
#define HALFPI          ((double)PI/2.0)      // pi divided by 2
#define PIOVERTWO       ((double)PI/2.0)      // pi divided by 2
#define FOURTHPI		((double)PI/4.0)	// pi divided by 4
#define ONE             INT_TO_FIXED(1)
#define FIXED_PI        205887L
#define FIXED_2PI       411775L
#define FIXED_E         178144L
#define FIXED_ROOT2      74804L
#define FIXED_ROOT3     113512L
#define FIXED_GOLDEN    106039L
#define degToRad		(PI/180.0)			// degrees to radians
#define radToDeg		(180.0/PI)			// radians to degrees

typedef long int32_t;
typedef unsigned long u_int32_t; 

double Sqrt(double x) SMTH;
double Sin(double x) SMTH;
double Cos(double x) SMTH;
double Tan(double x) SMTH;
double Atan(double x) SMTH;
double Atan2(double y, double x) SMTH;
double Fabs(double x)SMTH;
double Fnabs (double value) SMTH;
double Fmod(double x, double y) SMTH;
double Floor(double x) SMTH;
double Ceil(double x) SMTH;
double Asin(double x) SMTH;
double Acos(double x) SMTH;
#ifdef FUNCS_32
float cos_32s(float x) SMTH;
//float cos_32(float x) SMTH;
//float sin_32(float x) SMTH;
double cos_121s(double x) SMTH;
//double cos_121(double x) SMTH;
//double sin_121(double x) SMTH;
#endif
double acos(double x) SMTH;
Boolean LoadUnloadMathLib(Boolean load) SMTH;

/* Type definitions. */

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

typedef UINT4 NN_DIGIT;
typedef UINT2 NN_HALF_DIGIT;

/* Constants.

   Note: MAX_NN_DIGITS is long enough to hold any RSA modulus, plus
   one more digit as required by R_GeneratePEMKeys (for n and phiN,
   whose lengths must be even). All natural numbers have at most
   MAX_NN_DIGITS digits, except for double-length intermediate values
   in NN_Mult (t), NN_ModMult (t), NN_ModInv (w), and NN_Div (c).
*/

/* Length of digit in bits */
#define NN_DIGIT_BITS 32
#define NN_HALF_DIGIT_BITS 16
/* Length of digit in bytes */
#define NN_DIGIT_LEN (NN_DIGIT_BITS / 8)
/* Maximum length in digits */
#define MAX_NN_DIGITS ((MAX_RSA_MODULUS_LEN + NN_DIGIT_LEN - 1) / NN_DIGIT_LEN + 1)
/* Maximum digits */
#define MAX_NN_DIGIT 0xffffffff
#define MAX_NN_HALF_DIGIT 0xffff

#define NN_LT   -1
#define NN_EQ   0
#define NN_GT 1

/* Macros. */

#define LOW_HALF(x) ((x) & MAX_NN_HALF_DIGIT)
#define HIGH_HALF(x) (((x) >> NN_HALF_DIGIT_BITS) & MAX_NN_HALF_DIGIT)
#define TO_HIGH_HALF(x) (((NN_DIGIT)(x)) << NN_HALF_DIGIT_BITS)
#define DIGIT_MSB(x) (unsigned int)(((x) >> (NN_DIGIT_BITS - 1)) & 1)
#define DIGIT_2MSB(x) (unsigned int)(((x) >> (NN_DIGIT_BITS - 2)) & 3)

/* CONVERSIONS
   NN_Decode (a, digits, b, len)   Decodes character string b into a.
   NN_Encode (a, len, b, digits)   Encodes a into character string b.

   ASSIGNMENTS
   NN_Assign (a, b, digits)        Assigns a = b.
   NN_ASSIGN_DIGIT (a, b, digits)  Assigns a = b, where b is a digit.
   NN_AssignZero (a, b, digits)    Assigns a = 0.
   NN_Assign2Exp (a, b, digits)    Assigns a = 2^b.

   ARITHMETIC OPERATIONS
   NN_Add (a, b, c, digits)        Computes a = b + c.
   NN_Sub (a, b, c, digits)        Computes a = b - c.
   NN_Mult (a, b, c, digits)       Computes a = b * c.
   NN_LShift (a, b, c, digits)     Computes a = b * 2^c.
   NN_RShift (a, b, c, digits)     Computes a = b / 2^c.
   NN_Div (a, b, c, cDigits, d, dDigits)  Computes a = c div d and b = c mod d.

   NUMBER THEORY
   NN_Mod (a, b, bDigits, c, cDigits)  Computes a = b mod c.
   NN_ModMult (a, b, c, d, digits) Computes a = b * c mod d.
   NN_ModExp (a, b, c, cDigits, d, dDigits)  Computes a = b^c mod d.
   NN_ModInv (a, b, c, digits)     Computes a = 1/b mod c.
   NN_Gcd (a, b, c, digits)        Computes a = gcd (b, c).

   OTHER OPERATIONS
   NN_EVEN (a, digits)             Returns 1 iff a is even.
   NN_Cmp (a, b, digits)           Returns sign of a - b.
   NN_EQUAL (a, digits)            Returns 1 iff a = b.
   NN_Zero (a, digits)             Returns 1 iff a = 0.
   NN_Digits (a, digits)           Returns significant length of a in digits.
   NN_Bits (a, digits)             Returns significant length of a in bits.
*/
NN_DIGIT subdigitmult (NN_DIGIT *, NN_DIGIT *, NN_DIGIT, NN_DIGIT *, unsigned int) SCRPT;
void dmult (NN_DIGIT, NN_DIGIT, NN_DIGIT *, NN_DIGIT *) SCRPT;
unsigned int NN_DigitBits (NN_DIGIT) SCRPT;
void NN_Decode (NN_DIGIT *, unsigned int, unsigned char *, unsigned int) SCRPT;
void NN_Encode (unsigned char *, unsigned int, NN_DIGIT *, unsigned int) SCRPT;
void NN_Assign (NN_DIGIT *, NN_DIGIT *, unsigned int) SCRPT;
void NN_AssignZero (NN_DIGIT *, unsigned int) SCRPT;
void NN_Assign2Exp (NN_DIGIT *, unsigned int, unsigned int) SCRPT;

NN_DIGIT NN_Add (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned int) SCRPT;
NN_DIGIT NN_Sub (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned int) SCRPT;
void NN_Mult (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned int) SCRPT;
void NN_Div (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned int, NN_DIGIT *, unsigned int) SCRPT;
NN_DIGIT NN_LShift (NN_DIGIT *, NN_DIGIT *, unsigned int, unsigned int) SCRPT;
NN_DIGIT NN_RShift (NN_DIGIT *, NN_DIGIT *, unsigned int, unsigned int) SCRPT;
NN_DIGIT NN_LRotate (NN_DIGIT *, NN_DIGIT *, unsigned int, unsigned int) SCRPT;

void NN_Mod (NN_DIGIT *, NN_DIGIT *, unsigned int, NN_DIGIT *, unsigned int) SCRPT;
void NN_ModMult (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned int) SCRPT;
void NN_ModExp (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned int, NN_DIGIT *, unsigned int) SCRPT;
void NN_ModInv (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned int) SCRPT;
void NN_Gcd (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned int) SCRPT;

int NN_Cmp (NN_DIGIT *, NN_DIGIT *, unsigned int) SCRPT;
int NN_Zero (NN_DIGIT *, unsigned int) SCRPT;
unsigned int NN_Bits (NN_DIGIT *, unsigned int) SCRPT;
unsigned int NN_Digits (NN_DIGIT *, unsigned int) SCRPT;

#define NN_ASSIGN_DIGIT(a, b, digits) {NN_AssignZero (a, digits); a[0] = b;}
#define NN_EQUAL(a, b, digits) (! NN_Cmp (a, b, digits))
#define NN_EVEN(a, digits) (((digits) == 0) || ! (a[0] & 1))

#endif
