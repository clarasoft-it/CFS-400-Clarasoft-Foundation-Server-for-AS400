
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

#ifndef __CSSTR_H__
#define __CSSTR_H__

#include "qcsrc/cscore.h"

#define CCSID_JOBDEFAULT   0
#define CCSID_UTF8         1208
#define CCSID_ASCII        819

#define CS_OPER_CSSTRCV    0x00020000

typedef void* CSSTRCV;

CSSTRCV
  CSSTRCV_Constructor
    (void);

CSRESULT
  CSSTRCV_Destructor
    (CSSTRCV* This);

CSRESULT
  CSSTRCV_SetConversion
    (CSSTRCV This,
     long iFromCCSID,
     long iToCCSID);

CSRESULT
  CSSTRCV_StrCpy
    (CSSTRCV This,
     char*  inBuff,
     long   size);

CSRESULT
  CSSTRCV_StrCat
    (CSSTRCV This,
     char*  inBuff,
     long   size);

long
  CSSTRCV_Size
    (CSSTRCV);

long
  CSSTRCV_Get
    (CSSTRCV This,
     char*  outBuffer);

char*
  CSSTR_StrTok
    (char* szBuffer,
     char*szDelimiter);

long
  CSSTR_Trim
    (char* szTarget,
     char* szSource);

int
  CSSTR_ToUpperCase
    (char* buffer,
     int size);

int
  CSSTR_ToLowerCase
    (char* buffer,
     int size);

long
  CSSTR_ToBase64
    (char* inBuffer,
     int   inSize,
     char* outBuffer,
     int   outSize);

#endif
