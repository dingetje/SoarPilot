#ifndef SOARSHAADD_H
#define SOARSHAADD_H

/*
 -------------------------------------------------------------------------
 Copyright (c) 2001, Dr Brian Gladman <brg@gladman.me.uk>, Worcester, UK.
 All rights reserved.

 TERMS

 Redistribution and use in source and binary forms, with or without 
 modification, are permitted subject to the following conditions:

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer. 

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution. 

  3. The copyright holder's name must not be used to endorse or promote 
     any products derived from this software without his specific prior 
     written permission. 

 This software is provided 'as is' with no express or implied warranties 
 of correctness or fitness for purpose.
 -------------------------------------------------------------------------
 Issue Date: 17/01/2002
*/

#define SCRPT __attribute__ ((section ("scrpt")))

#define SHAINIT     0
#define SHAHASH     1
#define SHAEND      2
#define SHAOUTPUT   3

void hash_out(unsigned char h[], unsigned int len) SCRPT;
int HandleSHA(const unsigned char *s, Int8 action) SCRPT;

void ConvertHexStr(Char *inStr, unsigned int inStrLen, Char *outStr, unsigned int *outStrLen) SCRPT;
void EncodeHexOutput(unsigned char *inputstr, int length) SCRPT;
void DumpCTX(sha256_ctx *ctx) SCRPT;

#endif
