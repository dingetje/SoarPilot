/*
	RSA.C - RSA routines for RSAEURO

	Copyright (c) J.S.A.Kapp 1994 - 1996.

	RSAEURO - RSA Library compatible with RSAREF(tm) 2.0.

	All functions prototypes are the Same as for RSAREF(tm).
	To aid compatiblity the source and the files follow the
	same naming comventions that RSAREF(tm) uses.  This should aid
	direct importing to your applications.

	All Trademarks Acknowledged.

	RSA encryption performed as defined in the PKCS (#1) by RSADSI.

	Revision history
		0.90 First revision, code produced very similar to that
		of RSAREF(tm), still it worked fine.

	0.91 Second revision, code altered to aid speeding up.
		Used pointer accesses to arrays to speed up some parts,
		mainly during the loops.

	1.03 Third revision, Random Structure initialization
	double check, RSAPublicEncrypt can now return RE_NEED_RANDOM.
*/

#include <PalmOS.h>
#include "soarRSA.h"
#include "soarMath.h"
#include "soarMem.h"

/* RSA decryption, according to RSADSI's PKCS #1. */

int RSAPublicDecrypt(output, outputLen, input, inputLen, publicKey)
unsigned char *output;		/* output block */
unsigned int *outputLen;	/* length of output block */
unsigned char *input;		/* input block */
unsigned int inputLen;		/* length of input block */
R_RSA_PUBLIC_KEY *publicKey;	/* RSA public key */
{
	int status;
	unsigned char *pkcsBlock;
	unsigned int i, modulusLen, pkcsBlockLen;

	AllocMem((void *)&pkcsBlock, sizeof(unsigned char)*MAX_RSA_MODULUS_LEN);
	modulusLen = (publicKey->bits + 7) / 8;

	if(inputLen > modulusLen)
		return(RE_LEN);

	status = rsapublicfunc(pkcsBlock, &pkcsBlockLen, input, inputLen, publicKey);
	if(status)
		return(status);

	if(pkcsBlockLen != modulusLen)
		return(RE_LEN);

	/* Require block type 1. */

	if((pkcsBlock[0] != 0) || (pkcsBlock[1] != 1))
	 return(RE_DATA);

	for(i = 2; i < modulusLen-1; i++)
		if(*(pkcsBlock+i) != 0xff)
			break;

	/* separator check */

	if(pkcsBlock[i++] != 0)
		return(RE_DATA);

	*outputLen = modulusLen - i;

	if(*outputLen + 11 > modulusLen)
		return(RE_DATA);

	MemMove((POINTER)output, (POINTER)&pkcsBlock[i], *outputLen);

	/* Clear sensitive information. */

	MemSet((POINTER)pkcsBlock, 0, sizeof(pkcsBlock));
	FreeMem((void *)&pkcsBlock);

	return(ID_OK);
}

/* RSA encryption, according to RSADSI's PKCS #1. */

int RSAPrivateEncrypt(output, outputLen, input, inputLen, privateKey)
unsigned char *output;		/* output block */
unsigned int *outputLen;	/* length of output block */
unsigned char *input;		/* input block */
unsigned int inputLen;		/* length of input block */
R_RSA_PRIVATE_KEY *privateKey;	/* RSA private key */
{
	int status;
	unsigned char *pkcsBlock;
	unsigned int i, modulusLen;


	AllocMem((void *)&pkcsBlock, sizeof(unsigned char)*MAX_RSA_MODULUS_LEN);
	modulusLen = (privateKey->bits + 7) / 8;

	if(inputLen + 11 > modulusLen)
		return (RE_LEN);

	*pkcsBlock = 0;
	/* block type 1 */
	*(pkcsBlock+1) = 1;

	for (i = 2; i < modulusLen - inputLen - 1; i++)
		*(pkcsBlock+i) = 0xff;

	/* separator */
	pkcsBlock[i++] = 0;

	MemMove((POINTER)&pkcsBlock[i], (POINTER)input, inputLen);

	status = rsaprivatefunc(output, outputLen, pkcsBlock, modulusLen, privateKey);

	/* Clear sensitive information. */

	MemSet((POINTER)pkcsBlock, 0, sizeof(pkcsBlock));
	FreeMem((void *)&pkcsBlock);

	return(status);
}

/* RSA decryption, according to RSADSI's PKCS #1. */

int RSAPrivateDecrypt(output, outputLen, input, inputLen, privateKey)
unsigned char *output;		/* output block */
unsigned int *outputLen;	/* length of output block */
unsigned char *input;		/* input block */
unsigned int inputLen;		/* length of input block */
R_RSA_PRIVATE_KEY *privateKey;	/* RSA private key */
{
	int status;
	unsigned char *pkcsBlock;
	unsigned int i, modulusLen, pkcsBlockLen;

	AllocMem((void *)&pkcsBlock, sizeof(unsigned char)*MAX_RSA_MODULUS_LEN);
	modulusLen = (privateKey->bits + 7) / 8;

	if(inputLen > modulusLen)
		return (RE_LEN);

	status = rsaprivatefunc(pkcsBlock, &pkcsBlockLen, input, inputLen, privateKey);
	if(status)
		return (status);

	if(pkcsBlockLen != modulusLen)
		return (RE_LEN);

	/* We require block type 2. */

	if((*pkcsBlock != 0) || (*(pkcsBlock+1) != 2))
	 return (RE_DATA);

	for(i = 2; i < modulusLen-1; i++)
		/* separator */
		if (*(pkcsBlock+i) == 0)
			break;

	i++;
	if(i >= modulusLen)
		return(RE_DATA);

	*outputLen = modulusLen - i;

	if(*outputLen + 11 > modulusLen)
		return(RE_DATA);

	MemMove((POINTER)output, (POINTER)&pkcsBlock[i], *outputLen);

	/* Clear sensitive information. */
	MemSet((POINTER)pkcsBlock, 0, sizeof(pkcsBlock));
	FreeMem((void *)&pkcsBlock);

	return(ID_OK);
}

/* Raw RSA public-key operation. Output has same length as modulus.

	 Requires input < modulus.
*/
int rsapublicfunc(output, outputLen, input, inputLen, publicKey)
unsigned char *output;		/* output block */
unsigned int *outputLen;	/* length of output block */
unsigned char *input;		/* input block */
unsigned int inputLen;		/* length of input block */
R_RSA_PUBLIC_KEY *publicKey;	/* RSA public key */
{
	NN_DIGIT c[MAX_NN_DIGITS], e[MAX_NN_DIGITS], m[MAX_NN_DIGITS],
		n[MAX_NN_DIGITS];
	unsigned int eDigits, nDigits;


		/* decode the required RSA function input data */
	NN_Decode(m, MAX_NN_DIGITS, input, inputLen);
	NN_Decode(n, MAX_NN_DIGITS, publicKey->modulus, MAX_RSA_MODULUS_LEN);
	NN_Decode(e, MAX_NN_DIGITS, publicKey->exponent, MAX_RSA_MODULUS_LEN);

	nDigits = NN_Digits(n, MAX_NN_DIGITS);
	eDigits = NN_Digits(e, MAX_NN_DIGITS);

	if(NN_Cmp(m, n, nDigits) >= 0)
		return(RE_DATA);

	*outputLen = (publicKey->bits + 7) / 8;

	/* Compute c = m^e mod n.  To perform actual RSA calc.*/

	NN_ModExp (c, m, e, eDigits, n, nDigits);

	/* encode output to standard form */
	NN_Encode (output, *outputLen, c, nDigits);

	/* Clear sensitive information. */

	MemSet((POINTER)c, 0, sizeof(c));
	MemSet((POINTER)m, 0, sizeof(m));

	return(ID_OK);
}

/* Raw RSA private-key operation. Output has same length as modulus.

	 Requires input < modulus.
*/

int rsaprivatefunc(output, outputLen, input, inputLen, privateKey)
unsigned char *output;		/* output block */
unsigned int *outputLen;	/* length of output block */
unsigned char *input;		/* input block */
unsigned int inputLen;		/* length of input block */
R_RSA_PRIVATE_KEY *privateKey;	/* RSA private key */
{
	NN_DIGIT *c, *cP, *cQ, *dP, *dQ, 
				*mP, *mQ, *n, *p, *q, *qInv, *t;
	unsigned int cDigits, nDigits, pDigits;

	AllocMem((void *)&c, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&cP, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&cQ, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&dP, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&dQ, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&mP, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&mQ, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&n, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&p, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&q, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&qInv, sizeof(NN_DIGIT)*MAX_NN_DIGITS);
	AllocMem((void *)&t, sizeof(NN_DIGIT)*MAX_NN_DIGITS);

	/* decode required input data from standard form */
	NN_Decode(c, MAX_NN_DIGITS, input, inputLen);		/* input */

					/* private key data */
	NN_Decode(p, MAX_NN_DIGITS, privateKey->prime[0], MAX_RSA_PRIME_LEN);
	NN_Decode(q, MAX_NN_DIGITS, privateKey->prime[1], MAX_RSA_PRIME_LEN);
	NN_Decode(dP, MAX_NN_DIGITS, privateKey->primeExponent[0], MAX_RSA_PRIME_LEN);
	NN_Decode(dQ, MAX_NN_DIGITS, privateKey->primeExponent[1], MAX_RSA_PRIME_LEN);
	NN_Decode(n, MAX_NN_DIGITS, privateKey->modulus, MAX_RSA_MODULUS_LEN);
	NN_Decode(qInv, MAX_NN_DIGITS, privateKey->coefficient, MAX_RSA_PRIME_LEN);
		/* work out lengths of input components */

	cDigits = NN_Digits(c, MAX_NN_DIGITS);
	pDigits = NN_Digits(p, MAX_NN_DIGITS);
	nDigits = NN_Digits(n, MAX_NN_DIGITS);


	if(NN_Cmp(c, n, nDigits) >= 0)
		return(RE_DATA);

	*outputLen = (privateKey->bits + 7) / 8;

	/* Compute mP = cP^dP mod p  and  mQ = cQ^dQ mod q. (Assumes q has
		 length at most pDigits, i.e., p > q.)
	*/

	NN_Mod(cP, c, cDigits, p, pDigits);
	NN_Mod(cQ, c, cDigits, q, pDigits);

	NN_AssignZero(mP, nDigits);
	NN_ModExp(mP, cP, dP, pDigits, p, pDigits);

	NN_AssignZero(mQ, nDigits);
	NN_ModExp(mQ, cQ, dQ, pDigits, q, pDigits);

	/* Chinese Remainder Theorem:
			m = ((((mP - mQ) mod p) * qInv) mod p) * q + mQ.
	*/

	if(NN_Cmp(mP, mQ, pDigits) >= 0) {
		NN_Sub(t, mP, mQ, pDigits);
	}else{
		NN_Sub(t, mQ, mP, pDigits);
		NN_Sub(t, p, t, pDigits);
	}

	NN_ModMult(t, t, qInv, p, pDigits);
	NN_Mult(t, t, q, pDigits);
	NN_Add(t, t, mQ, nDigits);

	/* encode output to standard form */
	NN_Encode (output, *outputLen, t, nDigits);

	/* Clear sensitive information. */
	MemSet((POINTER)c, 0, sizeof(c));
	MemSet((POINTER)cP, 0, sizeof(cP));
	MemSet((POINTER)cQ, 0, sizeof(cQ));
	MemSet((POINTER)dP, 0, sizeof(dP));
	MemSet((POINTER)dQ, 0, sizeof(dQ));
	MemSet((POINTER)mP, 0, sizeof(mP));
	MemSet((POINTER)mQ, 0, sizeof(mQ));
	MemSet((POINTER)p, 0, sizeof(p));
	MemSet((POINTER)q, 0, sizeof(q));
	MemSet((POINTER)qInv, 0, sizeof(qInv));
	MemSet((POINTER)t, 0, sizeof(t));

	FreeMem((void *)&c);
	FreeMem((void *)&cP);
	FreeMem((void *)&cQ);
	FreeMem((void *)&dP);
	FreeMem((void *)&dQ);
	FreeMem((void *)&mP);
	FreeMem((void *)&mQ);
	FreeMem((void *)&n);
	FreeMem((void *)&p);
	FreeMem((void *)&q);
	FreeMem((void *)&qInv);
	FreeMem((void *)&t);

	return(ID_OK);
}
