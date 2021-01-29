/* ===========================================================================
  Clarasoft Core definitions

  cslib.h
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
=========================================================================== */

#ifndef __CLARASOFT_CSLIB_H__
#define __CLARASOFT_CSLIB_H__

#include <errno.h>
#include <inttypes.h>

#define CS_SUCCESS                     (0x00000000)
#define CS_FAILURE                     (0x10000000)

#define CS_MASK_OPER                   (0x0FFF0000)
#define CS_MASK_DIAG                   (0x0000FFFF)

#define CS_SUCCEED(x)                  (((x) & (0xF0000000)) == (0))
#define CS_FAIL(x)                     ((x) & (CS_FAILURE))
#define CS_OPER(x)                     ((x) & (CS_MASK_OPER))
#define CS_DIAG(x)                     ((x) & (CS_MASK_DIAG))

#define CS_OPER_NOVALUE                (0x00000000)
#define CS_DIAG_NOVALUE                (0x00000000)
#define CS_DIAG_UNKNOWN                (0x0000FFFF)

typedef long CSRESULT;

#define CSLIST_TOP                     (0x00000000)   // at the beginning of the list
#define CSLIST_BOTTOM                  (0x7FFFFFFF)   // at the end of the list

#define CSSTR_B64_LINEBREAK_OFFSET     (0x0000004D)
#define CSSTR_B64_MASK_LINEBREAK       (0x0000000F)
#define CSSTR_B64_LINEBREAK_NONE       (0x00000000)
#define CSSTR_B64_LINEBREAK_LF         (0x00000001)
#define CSSTR_B64_LINEBREAK_CRLF       (0x00000002)
#define CSSTR_B64_IGNOREINVALIDCHAR    (0x00000100)

#define CSSTR_INPUT_ASCII              (0x00000001)
#define CSSTR_INPUT_EBCDIC             (0x00000002)
#define CSSTR_OUTPUT_ASCII             (0x00000004)
#define CSSTR_OUTPUT_EBCDIC            (0x00000008)

#define CSSTR_URLENCODE_SPACETOPLUS    (0x00000100)
#define CSSTR_URLENCODE_XLATERESERVED  (0x00000200)

#define CS_OPER_CSSTRCV                (0x00020000)

#define CSSYS_UUID_BUFFERSIZE          (37)
#define CSSYS_UUID_UPPERCASE           (0x00000000)
#define CSSYS_UUID_LOWERCASE           (0x00000001)
#define CSSYS_UUID_DASHES              (0x00000002)


typedef void* CSLIST;
typedef void* CSMAP;
typedef void* CSSTRCV;

/* --------------------------------------------------------------------------
  Linked List
-------------------------------------------------------------------------- */

CSLIST
  CSLIST_Constructor
    (void);

void
  CSLIST_Destructor
    (CSLIST*);

CSRESULT
  CSLIST_Insert
    (CSLIST This,
     void* value,
     long  valueSize,
     long  index);

CSRESULT
  CSLIST_Remove
    (CSLIST This,
     long   index);

CSRESULT
  CSLIST_Get
    (CSLIST This,
     void*  value,
     long   index);

CSRESULT
  CSLIST_Set
    (CSLIST This,
     void*  value,
     long   valueSize,
     long   index);

long
  CSLIST_Count
    (CSLIST This);

void
  CSLIST_Clear
    (CSLIST This);

long
  CSLIST_ItemSize
    (CSLIST This,
     long   index);

CSRESULT
  CSLIST_GetDataRef
    (CSLIST This,
     void** value,
     long   index);

/* --------------------------------------------------------------------------
  Hash Map
-------------------------------------------------------------------------- */

CSMAP
  CSMAP_Constructor
    (void);

void
  CSMAP_Destructor
    (CSMAP*);

CSRESULT
  CSMAP_Insert
    (CSMAP This,
     char* key,
     void* value,
     long  valueSize);

CSRESULT
  CSMAP_InsertKeyRef
    (CSMAP This,
     char* key,
     void* value,
     long  valueSize);

CSRESULT
  CSMAP_Remove
    (CSMAP This,
     char* key);

CSRESULT
  CSMAP_Clear
    (CSMAP This);

CSRESULT
  CSMAP_Lookup
    (CSMAP  This,
     char*  key,
     void** value,
     long*  valueSize);

CSRESULT
  CSMAP_IterStart
    (CSMAP This);

CSRESULT
  CSMAP_IterNext
    (CSMAP  This,
     char** key,
     void** value,
     long*  valueSize);

/* --------------------------------------------------------------------------
  String
-------------------------------------------------------------------------- */

CSSTRCV
  CSSTRCV_Constructor
    (void);

CSRESULT
  CSSTRCV_Destructor
    (CSSTRCV* This);

CSRESULT
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

uint64_t
  CSSTR_FromBase64
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     uint64_t   outSize);

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
     unsigned char* outBuffer);

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
    (char* szSource,
     char* szTarget);

long
  CSSTR_UrlDecode
    (unsigned char* in,
     unsigned char* out,
     long InSize,
     int flags);

long
  CSSTR_UrlEncode
    (unsigned char* in,
     unsigned char* out,
     long InSize,
     int flags);

/* --------------------------------------------------------------------------
  System
-------------------------------------------------------------------------- */

CSRESULT
  CSSYS_GetCurJobInfo
    (char szJobName[11],
     char szJobNumber[7],
     char szJobUser[11]);

CSRESULT
  CSSYS_MakeUUID
    (char* szUUID,
     int mode);


#endif

 
