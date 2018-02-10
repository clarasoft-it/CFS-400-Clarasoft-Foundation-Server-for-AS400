/* ==========================================================================
  Clarasoft Core Tools

  csstr.h
  string utilities

  Version 1.0.0
 
  Distributed under the MIT license

  Copyright (c) 2013 Clarasoft I.T. Solutions Inc.

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify,
  merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
  THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
========================================================================== */

#ifndef __CLARASOFT_CT_CSSTR_H__
#define __CLARASOFT_CT_CSSTR_H__

#include "qcsrc/cscore.h"

#define CCSID_JOBDEFAULT               (0)
#define CCSID_UTF8                     (1208)
#define CCSID_ASCII                    (819)
#define CCSID_WINDOWS_1251             (1251)
#define CCSID_ISO_8859_1               (819)

#define CSSTR_B64_LINEBREAK_OFFSET     (0x0000004D)
#define CSSTR_B64_MASK_LINEBREAK       (0x0000000F)
#define CSSTR_B64_LINEBREAK_NONE       (0x00000000)
#define CSSTR_B64_LINEBREAK_LF         (0x00000001)
#define CSSTR_B64_LINEBREAK_CRLF       (0x00000002)
#define CSSTR_B64_IGNOREINVALIDCHAR    (0x00000100)

#define CSSTR_URLENCODE_SPACETOPLUS    (0x00000100)
#define CSSTR_URLENCODE_XLATERESERVED  (0x00000200)

#define CS_OPER_CSSTRCV                (0x00020000)

typedef void* CSSTRCV;

uint64_t
  CSSTR_FromBase64Ex
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     uint64_t   outSize,
     int flags);

char*
  CSSTR_StrTok
    (char* szBuffer,
     char*szDelimiter);

uint64_t
  CSSTR_ToBase64
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     uint64_t   outSize);

uint64_t
  CSSTR_ToBase64Ex
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     uint64_t   outSize,
     int flags);

int
  CSSTR_ToLowerCase
    (char* buffer,
     int size);

int
  CSSTR_ToUpperCase
    (char* buffer,
     int size);

long
  CSSTR_Trim
    (char* szTarget,
     char* szSource);

uint64_t
  CSSTR_UrlDecode
    (unsigned char* in,
     unsigned char* out,
     uint64_t size);

uint64_t
  CSSTR_UrlEncode
    (unsigned char* in,
     uint64_t inSize,
     unsigned char* out,
     uint64_t size,
     int flags);

CSSTRCV
  CSSTRCV_Constructor
    (void);

CSRESULT
  CSSTRCV_Destructor
    (CSSTRCV* This);

long
  CSSTRCV_Get
    (CSSTRCV This,
     char*  outBuffer);

CSRESULT
  CSSTRCV_SetConversion
    (CSSTRCV This,
     char* szFromCCSID,
     char* szToCCSID);

long
  CSSTRCV_Size
    (CSSTRCV);

CSRESULT
  CSSTRCV_StrCat
    (CSSTRCV This,
     char*  inBuff,
     long   size);

CSRESULT
  CSSTRCV_StrCpy
    (CSSTRCV This,
     char*  inBuff,
     long   size);

#endif
