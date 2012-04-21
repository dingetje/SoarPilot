#include <PalmOS.h>
#include "soaring.h"
#include "soarSHA.h"
#include "soarSHAAdd.h"
#include "soarRSA.h"
#include "soarKeys.h"
#include "soarUtil.h"
#include "soarMem.h"
#include "soarIO.h"

char* hexd = "0123456789ABCDEF";
//char hexsig[500];

void ConvertHexStr(Char *inStr, unsigned int inStrLen, Char *outStr, unsigned int *outStrLen)
{
	Char char2[2];
   unsigned int   i, t=0;
	int count=0;

   for(i = 0; i < inStrLen; ++i) {
//		HostTraceOutputTL(appErrorClass, "i-%hu", i);
		count++;
//		HostTraceOutputTL(appErrorClass, "count-%hd", count);
		char2[count-1] = inStr[i];
		if (count == 2) {
			char2[2] = '\0';
//			HostTraceOutputTL(appErrorClass, "char2-|%s|", char2);
			outStr[t++] = Hex2Dec2(char2);
//			HostTraceOutputTL(appErrorClass, "outStr[t]-%c", outStr[t-1]);
//			HostTraceOutputTL(appErrorClass, "t-%hu", t);
			count = 0;
		}
	}
	outStr[t] = '\0';
	*outStrLen = StrLen(outStr);
//	HostTraceOutputTL(appErrorClass, "outStr-|%s|", outStr);
//	HostTraceOutputTL(appErrorClass, "*outStrLen-|%hu|", *outStrLen);

	return;
}

/*
void hash_out_old(unsigned char h[], unsigned int len)
{
   unsigned char  s[200];
   unsigned char  tempchar[100];
   unsigned int   i, j, d, t;
	unsigned int   x=0;

//	HostTraceOutputTL(appErrorClass, "hash_out_old");
//	HostTraceOutputTL(appErrorClass, "StrLen(h):|%hu|", StrLen(h));
	d = 0;
	StrCopy(tempchar, "G");
	for (j=0; j<(len/4); ++j) {
		for (i=(4*j); i<(4*j+4); ++i) {
			t = h[i];
			s[d++] = hexd[t >> 4];
			hexsig[x++] = s[d-1];
			s[d++] = hexd[t & 15];
			hexsig[x++] = s[d-1];
		}

		// This is to break the encrypted data into two "G" strings 65 characters long
		if (j==(len/8) || j==((len/4)-1)) {
			s[d++] = '\0';
//			HostTraceOutputTL(appErrorClass, "s:|%s|", s);
			StrCat(tempchar, s);
			StrCatEOL(tempchar, data.config.xfertype);
//			HostTraceOutputTL(appErrorClass, "s:|%s|", tempchar);
			d = 0;
			StrCopy(tempchar, "G");
		}
	}
	hexsig[x++] = '\0';
//	HostTraceOutputTL(appErrorClass, "StrLen(hexsig):|%hu|", StrLen(hexsig));
//	HostTraceOutputTL(appErrorClass, "hexsig:|%s|", hexsig);
}
*/


void hash_out(unsigned char h[], unsigned int len)
{
	unsigned char  *s;
	unsigned char *tempchar;
	unsigned int i, d, t, j;
	unsigned int glen;
	R_RSA_PRIVATE_KEY privateKey = PRIVATE_KEY1;

	// Outputted values
	unsigned char *signature;
	unsigned int  signatureLen=0;

	// Inputted values
	unsigned char *digest;
	unsigned int digestLen;

	AllocMem((void *)&s, sizeof(Char)*200);
	AllocMem((void *)&tempchar, sizeof(Char)*100);
	AllocMem((void *)&signature, sizeof(Char)*MAX_SIGNATURE_LEN);
	AllocMem((void *)&digest, (sizeof(Char)*SHA256_DIGEST_SIZE+1));

	StrCopy(digest, h);
	digestLen = StrLen(digest);
//	MemMove(digest, h, (sizeof(Char)*SHA256_DIGEST_SIZE));
//	digestLen = len;
//	HostTraceOutputTL(appErrorClass, "Digest in-|%s|", digest);
//	HostTraceOutputTL(appErrorClass, "       StrLen(h)-%hu", StrLen(h));
//	HostTraceOutputTL(appErrorClass, "       len-%hu", len);
//	HostTraceOutputTL(appErrorClass, "       StrLen(digest)-%hu", StrLen(digest));
//	HostTraceOutputTL(appErrorClass, "       digestLen-%hu", digestLen);
//	HostTraceOutputTL(appErrorClass, "---------------------------------------------------");

	MemSet(s, 0, sizeof(s));
//	HostTraceOutputTL(appErrorClass, "Encoding Digest to Hex");
	d = 0;
	for (j=0; j<(digestLen/4); ++j) {
		for (i=(4*j); i<(4*j+4); ++i) {
			t = digest[i];
			s[d++] = hexd[t >> 4];
			s[d++] = hexd[t & 15];
		}
	}
	s[d] = '\0';
//	HostTraceOutputTL(appErrorClass, "       StrLen(s)-%hu", StrLen(s));
//	HostTraceOutputTL(appErrorClass, "       s-|%s|", s);
//	HostTraceOutputTL(appErrorClass, "---------------------------------------------------");

	MemSet(signature, 0, sizeof(signature));
//	HostTraceOutputTL(appErrorClass, "Encrypting with Private Key");

	j = sizeof(privateKey);
	RSAPrivateEncrypt(signature, &signatureLen, digest, StrLen(digest), &privateKey);

//	HostTraceOutputTL(appErrorClass, "       signatureLen-%hu", signatureLen);
//	HostTraceOutputTL(appErrorClass, "       signature-|%s|", signature);
//	HostTraceOutputTL(appErrorClass, "---------------------------------------------------");

	MemSet(s, 0, sizeof(s));
//	HostTraceOutputTL(appErrorClass, "Encoding signature to Hex");
	d = 0;
	for (j=0; j<(signatureLen/4); ++j) {
		for (i=(4*j); i<(4*j+4); ++i) {
			t = signature[i];
			s[d++] = hexd[t >> 4];
			s[d++] = hexd[t & 15];
		}
	}
	s[d] = '\0';
//	HostTraceOutputTL(appErrorClass, "       StrLen(s)-%hu", StrLen(s));
//	HostTraceOutputTL(appErrorClass, "       s-|%s|", s);
//	HostTraceOutputTL(appErrorClass, "---------------------------------------------------");

//	HostTraceOutputTL(appErrorClass, "Building the G Lines");

	StrCopy(tempchar, "G");
	glen = StrLen(s);
	for (j=0; j<(glen/2); ++j) {
		tempchar[j+1] = s[j];
	}
	tempchar[glen/2+1] = '\0';
//	HostTraceOutputTL(appErrorClass, "       cgline1-|%s|", tempchar);
//	HostTraceOutputTL(appErrorClass, "       StrLen(cgline1)-%hu", StrLen(tempchar));
	StrCatEOL(tempchar, data.config.xfertype);
	TxData(tempchar, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "tempchar:|%s|", tempchar);

	for (j=(glen/2); j<glen; ++j) {
		tempchar[j-(glen/2)+1] = s[j];
	}
	tempchar[glen/2+1] = '\0';
//	HostTraceOutputTL(appErrorClass, "       cgline2-|%s|", tempchar);
//	HostTraceOutputTL(appErrorClass, "       StrLen(cgline2)-%hu", StrLen(tempchar));
	StrCatEOL(tempchar, data.config.xfertype);
	TxData(tempchar, data.config.xfertype);
//	HostTraceOutputTL(appErrorClass, "tempchar:|%s|", tempchar);
//	HostTraceOutputTL(appErrorClass, "---------------------------------------------------");


	FreeMem((void *)&s);
	FreeMem((void *)&tempchar);
	FreeMem((void *)&signature);
	FreeMem((void *)&digest);

	return;
}

int HandleSHA(const unsigned char *s, Int8 action)
{
 	static sha256_ctx     *ht;
	static unsigned char  h[32];
	int retval = 1;
	static int hashcount = 0;

	switch(action) {
		case SHAINIT: 
			AllocMem((void *)&ht, sizeof(sha256_ctx));
			sha256_begin(ht);
			break;
		case SHAHASH:
			hashcount++;
			sha256_hash(s, StrLen((const char*)s), ht);
			break;
		case SHAEND:
			sha256_end(h, ht);
			break;
		case SHAOUTPUT:
			hash_out(h, 32);
			FreeMem((void *)&ht);
			break;
	}
	return(retval);
}

void EncodeHexOutput(unsigned char *inputstr, int length)
{
	int i, d, j, t;
	unsigned char  s[200];

	MemSet(s, 0, sizeof(s));
	d = 0;
	for (j=0; j<(length/4); ++j) {
		for (i=(4*j); i<(4*j+4); ++i) {
			t = inputstr[i];
			s[d++] = hexd[t >> 4];
			s[d++] = hexd[t & 15];
		}
	}
	s[d] = '\0';
//	HostTraceOutputTL(appErrorClass, "hexout(%hd)-|%s|", length, s);

	return;
}

void DumpCTX(sha256_ctx *ctx)
{
//	HostTraceOutputTL(appErrorClass, "ctx->count[0]-|%lu|", ctx->count[0]);
//	HostTraceOutputTL(appErrorClass, "ctx->count[1]-|%lu|", ctx->count[1]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[0]-|%lu|", ctx->wbuf[0]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[1]-|%lu|", ctx->wbuf[1]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[2]-|%lu|", ctx->wbuf[2]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[3]-|%lu|", ctx->wbuf[3]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[4]-|%lu|", ctx->wbuf[4]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[5]-|%lu|", ctx->wbuf[5]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[6]-|%lu|", ctx->wbuf[6]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[7]-|%lu|", ctx->wbuf[7]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[8]-|%lu|", ctx->wbuf[8]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[9]-|%lu|", ctx->wbuf[9]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[10]-|%lu|", ctx->wbuf[10]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[11]-|%lu|", ctx->wbuf[11]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[12]-|%lu|", ctx->wbuf[12]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[13]-|%lu|", ctx->wbuf[13]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[14]-|%lu|", ctx->wbuf[14]);
//	HostTraceOutputTL(appErrorClass, "ctx->wbuf[15]-|%lu|", ctx->wbuf[15]);
//	HostTraceOutputTL(appErrorClass, "ctx->hash[0]-|%lu|", ctx->hash[0]);
//	HostTraceOutputTL(appErrorClass, "ctx->hash[1]-|%lu|", ctx->hash[1]);
//	HostTraceOutputTL(appErrorClass, "ctx->hash[2]-|%lu|", ctx->hash[2]);
//	HostTraceOutputTL(appErrorClass, "ctx->hash[3]-|%lu|", ctx->hash[3]);
//	HostTraceOutputTL(appErrorClass, "ctx->hash[4]-|%lu|", ctx->hash[4]);
//	HostTraceOutputTL(appErrorClass, "ctx->hash[5]-|%lu|", ctx->hash[5]);
//	HostTraceOutputTL(appErrorClass, "ctx->hash[6]-|%lu|", ctx->hash[6]);
//	HostTraceOutputTL(appErrorClass, "ctx->hash[7]-|%lu|", ctx->hash[7]);

	return;
}
/*
void loadctx(sha256_ctx *ctx)
{
   ctx->count[0] = (uint32_t)123105;
   ctx->count[1] = (uint32_t)0;
   ctx->wbuf[0] = (uint32_t)875966769;
   ctx->wbuf[1] = (uint32_t)809055822;
   ctx->wbuf[2] = (uint32_t)808530740;
   ctx->wbuf[3] = (uint32_t)858927668;
   ctx->wbuf[4] = (uint32_t)1161900080;
   ctx->wbuf[5] = (uint32_t)825766448;
   ctx->wbuf[6] = (uint32_t)808532018;
   ctx->wbuf[7] = (uint32_t)808530224;
   ctx->wbuf[8] = (uint32_t)889192448;
   ctx->wbuf[9] = (uint32_t)2162096523;
   ctx->wbuf[10] = (uint32_t)393194924;
   ctx->wbuf[11] = (uint32_t)3109953767;
   ctx->wbuf[12] = (uint32_t)886122484;
   ctx->wbuf[13] = (uint32_t)3680844869;
   ctx->wbuf[14] = (uint32_t)3886500634;
   ctx->wbuf[15] = (uint32_t)2692924941;
   ctx->hash[0] = (uint32_t)2194452386;
   ctx->hash[1] = (uint32_t)3712624302;
   ctx->hash[2] = (uint32_t)1759553943;
   ctx->hash[3] = (uint32_t)2749668608;
   ctx->hash[4] = (uint32_t)681832178;
   ctx->hash[5] = (uint32_t)89800019;
   ctx->hash[6] = (uint32_t)3872881832;
   ctx->hash[7] = (uint32_t)1922317576;

	return;

}
*/
