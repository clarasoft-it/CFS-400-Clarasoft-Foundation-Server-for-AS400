/* ==========================================================================
  Clarasoft JSON object

  csjson.h
  JSON object definitions
  Version 1.0.0

  Distributed under the MIT license

  Copyright (c) 2020 Clarasoft I.T. Solutions Inc.

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

#ifndef __CLARASOFT_CSCORE_CSJSON_H__
#define __CLARASOFT_CSCORE_CSJSON_H__

#include "qcsrcx/cslist.h"

#define JSON_PARSE_STANDARD   0

#define JSON_TYPE_BOOL_FALSE  0
#define JSON_TYPE_BOOL_TRUE   1
#define JSON_TYPE_NULL        3
#define JSON_TYPE_OBJECT     10
#define JSON_TYPE_ARRAY      20
#define JSON_TYPE_NUMERIC    51
#define JSON_TYPE_STRING     52
#define JSON_TYPE_UNKNOWN    99

typedef void* CSJSON;

typedef struct tagCSJSON_DIRENTRY
{
  int    type;
  long   numItems;

} CSJSON_DIRENTRY;

typedef struct tagCSJSON_LSENTRY
{
  int   type;
  char* szKey;
  long  keySize;
  char* szValue;
  long  valueSize;

} CSJSON_LSENTRY;

CSJSON
  CSJSON_Constructor
    (void);

void
  CSJSON_Destructor
    (CSJSON*);

CSRESULT
  CSJSON_Parse
    (CSJSON This,
     char* pJsonString,
     int parseMode);

CSRESULT
  CSJSON_LookupDir
    (CSJSON This,
     char* szPath,
     CSJSON_DIRENTRY* pdire);

CSRESULT
  CSJSON_LookupKey
    (CSJSON This,
     char* szPath,
     char* szKey,
     CSJSON_LSENTRY* plse);

CSRESULT
  CSJSON_LookupIndex
    (CSJSON This,
     char* szPath,
     long index,
     CSJSON_LSENTRY* plse);

CSRESULT
  CSJSON_Init
    (CSJSON This,
     long type);

CSRESULT
  CSJSON_InsertBool
    (CSJSON This,
     char*   szPath,
     char*   szKey,
     int     boolValue);

CSRESULT
  CSJSON_InsertNull
    (CSJSON This,
     char*   szPath,
     char*   szKey);

CSRESULT
  CSJSON_InsertNumeric
    (CSJSON This,
     char*   szPath,
     char*   szKey,
     char*   szValue);

CSRESULT
  CSJSON_InsertString
    (CSJSON This,
     char*   szPath,
     char*   szKey,
     char*   szValue);

CSRESULT
  CSJSON_Ls
    (CSJSON This,
     char*  szPath,
     CSLIST listing);

CSRESULT
  CSJSON_MkDir
    (CSJSON This,
     char*  szPath,
     char*  szKey,
     int    type);

long
  CSJSON_Serialize
    (CSJSON This,
     char* szPath,
     char** szOutStream,
     int mode);

CSRESULT
  CSJSON_Dump
    (CSJSON This,
     CSLIST values);


#endif
 
