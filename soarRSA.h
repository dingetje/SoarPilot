#ifndef SOARRSA_H
#define SOARRSA_H
/*
	soarRSA.H - header file for soarRSA.C

	RSA Routines Header File.

*/

#define SCRPT __attribute__ ((section ("scrpt")))

/* RSA key lengths. */

#define MIN_RSA_MODULUS_BITS 508
#define MAX_RSA_MODULUS_BITS 1024
#define MAX_RSA_MODULUS_LEN ((MAX_RSA_MODULUS_BITS + 7) / 8)
#define MAX_RSA_PRIME_BITS ((MAX_RSA_MODULUS_BITS + 1) / 2)
#define MAX_RSA_PRIME_LEN ((MAX_RSA_PRIME_BITS + 7) / 8)

#define MAX_DIGEST_LEN 256
#define MAX_SIGNATURE_LEN MAX_RSA_MODULUS_LEN

/* Error codes. */

#define RE_CONTENT_ENCODING 0x0400
#define RE_DATA 0x0401
#define RE_DIGEST_ALGORITHM 0x0402
#define RE_ENCODING 0x0403
#define RE_KEY 0x0404
#define RE_KEY_ENCODING 0x0405
#define RE_LEN 0x0406
#define RE_MODULUS_LEN 0x0407
#define RE_NEED_RANDOM 0x0408
#define RE_PRIVATE_KEY 0x0409
#define RE_PUBLIC_KEY 0x040a
#define RE_SIGNATURE 0x040b
#define RE_SIGNATURE_ENCODING 0x040c
#define RE_ENCRYPTION_ALGORITHM 0x040d
#define RE_FILE 0x040e

/* Internal Error Codes */

/* IDOK and IDERROR changed to ID_OK and ID_ERROR */

#define ID_OK    0
#define ID_ERROR 1

/* RSAEuro Info Structure */

typedef struct {
    unsigned short int Version;                 /* RSAEuro Version */
    unsigned int flags;                         /* Version Flags */
    unsigned char ManufacturerID[32];           /* Toolkit ID */
    unsigned int Algorithms;                    /* Algorithms Supported */
} RSAEUROINFO;

/* Random structure. */

typedef struct {
  unsigned int bytesNeeded;                    /* seed bytes required */
  unsigned char state[16];                     /* state of object */
  unsigned int outputAvailable;                /* number byte available */
  unsigned char output[16];                    /* output bytes */
} R_RANDOM_STRUCT;

/* RSA public and private key. */

typedef struct {
  unsigned short int bits;                     /* length in bits of modulus */
  unsigned char modulus[MAX_RSA_MODULUS_LEN];  /* modulus */
  unsigned char exponent[MAX_RSA_MODULUS_LEN]; /* public exponent */
} R_RSA_PUBLIC_KEY;

typedef struct {
  unsigned short int bits;                     /* length in bits of modulus */
  unsigned char modulus[MAX_RSA_MODULUS_LEN];  /* modulus */
  unsigned char publicExponent[MAX_RSA_MODULUS_LEN];     /* public exponent */
  unsigned char exponent[MAX_RSA_MODULUS_LEN]; /* private exponent */
  unsigned char prime[2][MAX_RSA_PRIME_LEN];   /* prime factors */
  unsigned char primeExponent[2][MAX_RSA_PRIME_LEN];     /* exponents for CRT */
  unsigned char coefficient[MAX_RSA_PRIME_LEN];          /* CRT coefficient */
} R_RSA_PRIVATE_KEY;

/* RSA prototype key. */

typedef struct {
  unsigned int bits;                           /* length in bits of modulus */
  int useFermat4;                              /* public exponent (1 = F4, 0 = 3) */
} R_RSA_PROTO_KEY;

int RSAPrivateEncrypt (unsigned char *, unsigned int *, unsigned char *, unsigned int, R_RSA_PRIVATE_KEY *) SCRPT;
int RSAPublicDecrypt (unsigned char *, unsigned int *, unsigned char *, unsigned int, R_RSA_PUBLIC_KEY *) SCRPT;
int RSAPrivateDecrypt (unsigned char *, unsigned int *, unsigned char *, unsigned int, R_RSA_PRIVATE_KEY *) SCRPT;
int rsapublicfunc (unsigned char *, unsigned int *, unsigned char *, unsigned int, R_RSA_PUBLIC_KEY *) SCRPT;
int rsaprivatefunc (unsigned char *, unsigned int *, unsigned char *, unsigned int, R_RSA_PRIVATE_KEY *) SCRPT;


#endif /* SOARRSA_H */

