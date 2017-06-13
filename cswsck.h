/* ==========================================================================
  Clarasoft Foundation Server 400

  cswsck.h
  Web Socket Protocol Implementation definitions
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

#ifndef __CSWSCK_H__
#define __CSWSCK_H__

#include "qcsrc/cscore.h"


#define CSWSCK_MASK_OPERATION        (0x0FFF0000)

#define CSWSCK_OPER_CONTINUATION     (0x00000000)
#define CSWSCK_OPER_TEXT             (0x00010000)
#define CSWSCK_OPER_BINARY           (0x00020000)
#define CSWSCK_OPER_CLOSE            (0x00080000)
#define CSWSCK_OPER_PING             (0x00090000)
#define CSWSCK_OPER_PONG             (0x000A0000)

#define CSWSCK_FIN_OFF               (0x00)
#define CSWSCK_FIN_ON                (0x01)

#define CSWSCK_E_NODATA              (0x00000001)
#define CSWSCK_E_PARTIALDATA         (0x00000002)
#define CSWSCK_E_ALLDATA             (0x00000003)

#define CSWSCK_MOREDATA              (0x00000002)
#define CSWSCK_ALLDATA               (0x00000003)

#define CSWSCK_RCV_ALL               (0x00000001)
#define CSWSCK_RCV_PARTIAL           (0x00000002)

#define CSWSCK_OPERATION(x)          ((x) & CSWSCK_MASK_OPERATION)

typedef void* CSWSCK;

CSWSCK
  CSWSCK_OpenClient
    (int   connfd,
     char* szAppID,
     void* data,
     long  dataSize,
     void* sessionInfo,
     int   sessionInfoFmt);

CSWSCK
  CSWSCK_OpenServer
    (int   connfd,
     char* szAppID,
     char* szURL,
     long  port,
     void* sessionInfo,
     int   sessionInfoFmt);

CSRESULT
  CSWSCK_Close
    (CSWSCK    This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_Ping
    (CSWSCK    This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_Receive
    (CSWSCK    This,
     uint64_t* iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_Send
    (CSWSCK    This,
     long      operation,
     char*     szData,
     uint64_t  iDataSize,
     char      fin,
     int       timeout);

CSRESULT
  CSWSCK_GetData
    (CSWSCK    This,
     char*     szBuffer,
     uint64_t  offset,
     uint64_t  iMaxDataSize);

CSWSCK
  CSWSCK_SecureOpenClient
    (int   connfd,
     char* szAppID,
     char* szURL,
     long  port,
     void* sessionInfo,
     int   sessionInfoFmt);

CSWSCK*
  CSWSCK_SecureOpenServer
    (int   connfd,
     char* szAppID,
     void* sessionInfo,
     int   sessionInfoFmt);

CSRESULT
  CSWSCK_SecureClose
    (CSWSCK    This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_SecurePing
    (CSWSCK    This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_SecureReceive
    (CSWSCK    This,
     uint64_t* iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_SecureSend
    (CSWSCK    This,
     long      operation,
     char*     szData,
     uint64_t  iDataSize,
     char      fin,
     int       timeout);

CSRESULT
  CSWSCK_SecureGetData
    (CSWSCK    This,
     char*     szBuffer,
     uint64_t  offset,
     uint64_t  iMaxDataSize);

#endif