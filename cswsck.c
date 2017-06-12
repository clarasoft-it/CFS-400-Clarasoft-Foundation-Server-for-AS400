/* ==========================================================================
  Clarasoft Foundation Server 400

  cswsck.c
  Web Socket Protocol Implementation
  Version 1.0.0

  Compile module with:

     CRTCMOD MODULE(CSWSCK) SRCFILE(QCSRC) DBGVIEW(*ALL)

  To build this as a loadable module:

     CRTSRVPGM SRVPGM(CSWSCK)
        MODULE(CSWSCK CSLIST CFSAPI CSSTR) EXPORT(*ALL)

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

#include <errno.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "QSYSINC/H/QC3HASH"

#include "qcsrc/cscore.h"
#include "qcsrc/cslist.h"
#include "qcsrc/csstr.h"
#include "qcsrc/cfsapi.h"

#define CSWSCK_MASK_OPERATION        (0x0FFF0000)

#define CSWSCK_OP_CONTINUATION       (0x00)
#define CSWSCK_OP_TEXT               (0x01)
#define CSWSCK_OP_BINARY             (0x02)
#define CSWSCK_OP_CLOSE              (0x08)
#define CSWSCK_OP_PING               (0x09)
#define CSWSCK_OP_PONG               (0x0A)

#define CSWSCK_FIN_OFF               (0x00)
#define CSWSCK_FIN_ON                (0x01)

#define CSWSCK_OPER_CONTINUATION     (0x00000000)
#define CSWSCK_OPER_TEXT             (0x00010000)
#define CSWSCK_OPER_BINARY           (0x00020000)
#define CSWSCK_OPER_CLOSE            (0x00080000)
#define CSWSCK_OPER_PING             (0x00090000)
#define CSWSCK_OPER_PONG             (0x000A0000)
#define CSWSCK_OPER_CFSAPI           (0x0F010000)

#define CSWSCK_E_NODATA              (0x00000001)
#define CSWSCK_E_PARTIALDATA         (0x00000002)
#define CSWSCK_E_ALLDATA             (0x00000003)
#define CSWSCK_E_NOTSUPPORTED        (0x000000F1)
#define CSWSCK_E_READ                (0x000000F2)
#define CSWSCK_E_WRITE               (0x000000F3)

#define CSWSCK_MOREDATA              (0x00000002)
#define CSWSCK_ALLDATA               (0x00000003)

#define CSWSCK_RCV_ALL               (0x00000001)
#define CSWSCK_RCV_PARTIAL           (0x00000002)

#define CSWSCK_Fin(x)                (((x) & (0x80)) >> 7 )
#define CSWSCK_OpCode(x)             ((x)  & (0x0F))
#define CSWSCK_MaskCode(x)           (((x) & (0x80)) >> 7 )
#define CSWSCK_BaseLength(x)         (((x) & (0x7F)) )

#define CSWSCK_OPERATION(x)          ((x) & CSWSCK_MASK_OPERATION)


typedef struct tagCSWSCK {

  char* dataBuffer;

  uint64_t dataSize;

  CFS_INSTANCE* Connection;
  CSLIST internalData;
  CSSTRCV cvt;

} CSWSCK;

CSRESULT
  CSWSCK_Close
    (CSWSCK*  This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_GetData
    (CSWSCK*   This,
     char*     szBuffer,
     uint64_t  offset,
     uint64_t  iMaxDataSize);

CSWSCK*
  CSWSCK_OpenClient
    (int   connfd,
     char* szAppID,
     void* data,
     long  dataSize,
     void* sessionInfo,
     int   sessionInfoFmt);

CSWSCK*
  CSWSCK_OpenServer
    (int   connfd,
     char* szAppID,
     void* sessionInfo,
     int   sessionInfoFmt);

CSRESULT
  CSWSCK_Ping
    (CSWSCK*   This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_Receive
    (CSWSCK*   This,
     uint64_t* iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_SecureClose
    (CSWSCK*  This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

CSWSCK*
  CSWSCK_SecureOpenClient
    (int   connfd,
     char* szAppID,
     void* data,
     long  dataSize,
     void* sessionInfo,
     int   sessionInfoFmt);

CSWSCK*
  CSWSCK_SecureOpenServer
    (int   connfd,
     char* szAppID,
     void* sessionInfo,
     int   sessionInfoFmt);

CSRESULT
  CSWSCK_SecurePing
    (CSWSCK*   This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_SecureReceive
    (CSWSCK*   This,
     uint64_t* iDataSize,
     int       timeout);

CSRESULT
  CSWSCK_SecureSend
    (CSWSCK*   This,
     long      operation,
     char*     szData,
     uint64_t  iDataSize,
     char      fin,
     int       timeout);

CSRESULT
  CSWSCK_Send
    (CSWSCK*   This,
     long      operation,
     char*     szData,
     uint64_t  iDataSize,
     char      fin,
     int       timeout);

uint64_t ntohll(const uint64_t value);
uint64_t htonll(const uint64_t value);

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_Close
//
// Closes the websocket session: this sends the CLOSE websocket operation
// to the peer. The connection is then closed.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CSWSCK. Use
//       the CSWSCK_OpenClient() or the CSWSCK_OpenServer()
//       function to initialise this instance.
//
// szBuffer: A pointer to the data that is to be sent alonside the CLOSE
//           operation.
//
// iDataSize: The number of bytes to send. This is the size of the buffer
//            pointed to by the szBuffer parameter.
//
// timeout: The maximum number of seconds to wait before returning
//          from the attempt to send the CLOSE operation.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CSWSCK_Close(CSWSCK*  This,
                      char*    szBuffer,
                      uint64_t iDataSize,
                      int      timeout)
{
   CSRESULT hResult;

   hResult = CS_FAILURE;

   if (This) {

      // Send Websocket close to client
      hResult = CSWSCK_Send(This, CSWSCK_OPER_CLOSE,
                            szBuffer, iDataSize, CSWSCK_FIN_ON, timeout);

      CFS_Close(This->Connection);

      CSSTRCV_Destructor(&(This->cvt));
      CSLIST_Destructor(&This->internalData);

      free(This);
   }

   return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_GetData
//
// Copies the data received from a peer to a supplied buffer. This function
// can be used on either secure or non-secure connections.
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CSWSCK. Use
//       one the CSWSCK_*Open*() functions to initialise this instance.
//
// szBuffer: A pointer to a buffer that will receive the data.
//
// offset: Am integer representing the number of bytes from which to
//         start the copy from the internal data buffer. Data is always
//         copied to the first byte of the szBuffer parameter but the
//         copy operation may start from any byte from the internal
//         buffer as specified by this parameter.
//
// iMaxDataSize: The maximum number of bytes to copy into the supplied buffer.
//               If the internal buffer is larger, then the data is partially
//               copied up to the supplied buffer size.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CSWSCK_GetData(CSWSCK*   This,
                        char*     szBuffer,
                        uint64_t  offset,
                        uint64_t  iMaxDataSize) {

  if (iMaxDataSize <= This->dataSize - offset) {
     memcpy(szBuffer, This->dataBuffer + offset, iMaxDataSize - offset);
  }
  else {
     memcpy(szBuffer, This->dataBuffer + offset, This->dataSize - offset);
  }

  return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_OpenServer
//
// Opens a non-secure websocket server session.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// connfd: A socket descriptor to a TCP connection. This socket must be
//         already connected.
//
// szAppId: This parameter is ignored in this version.
//
// szOverflow: A pointer to a buffer that will receive any extra data
//             received from a web socket client. This should never
//             happen in practice and is just a precautionary measure.
//             If this parameter is NULL, then overflow data is ignored.
//
// iOverflowMax: The maximum number of overflow bytes to copy into the
//               szOverflow buffer, if not NULL.
//
// sessionInfo: The address of a data structure that will hold additional
//              data needed to establish a server session. At present, this
//              parameter is ignored.
//
// sessionInfoFmt: An integer identifying the format of the sessionInfo
//                 data structure. At present, this parameter is ignored.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    A pointer to a CSWSCK instance or NULL if the operation failed.
//
//
//////////////////////////////////////////////////////////////////////////////

CSWSCK* CSWSCK_OpenServer(int   connfd,
                          char* szAppID,
                          void* sessionInfo,
                          int   sessionInfoFmt)
{
  char szHTTPRequest[65536];
  char szHTTPHeader[1025];
  char szHTTPResponse[1024];
  char szMethod[1025];
  char szService[1025];
  char szHttpVersion[1025];
  char szKeyHash[1025];
  char szWebSocketVersion[1025];
  char szConnection[1025];
  char szUpgrade[1025];
  char szTempBuff[1025];
  char szTempBuff_2[1025];
  char szChallenge[129];
  char InpuFmt[9];
  char AlgDesc[9];
  char szHash[21];

  // Constants

  char szSpace[10];
  char szDot[10];
  char szComma[10];
  char szColon[10];
  char szNewline[10];
  char szDoubleNewline[10];

  char HashProvider;

  char* headers[120];
  char* szHTTPOverflow;
  char* token;

  int iIndex;
  int i;
  int n;
  int iSSLResult;
  int iHeaderOffsets[33];
  int iSize;

  long position;
  long iOverflowSize;

  union {
    char bError[48];
    int iBytesProvided;
  } error;

  union {
    char padding[20];
    int Algorithm;
  }HashAlgorithm;

  uint64_t size;

  CSWSCK*       This;
  CSRESULT      hResult;
  CFS_INSTANCE* Connection;
  CSSTRCV       cvts;

  This = (CSWSCK*)malloc(sizeof(CSWSCK));

  ///////////////////////////////////////////////////////////////////////////
  // Create conversion objects
  ///////////////////////////////////////////////////////////////////////////

  This->Connection = CFS_Open(connfd,
                              szAppID,
                              CFS_SESSIONTYPE_SERVER,
                              0,
                              0);

  if (This->Connection != 0) {

     This->cvt = CSSTRCV_Constructor();
     This->internalData = CSLIST_Constructor();

     size = 65535;
     hResult = CFS_Read(This->Connection,
                        szHTTPRequest,
                        &size,
                        -1);

     if (CS_SUCCEED(hResult)) {

        /////////////////////////////////////////////////////////////////////
        // Convert from ASCII to EBCDIC
        /////////////////////////////////////////////////////////////////////

        CSSTRCV_SetConversion(This->cvt, CCSID_ASCII, CCSID_JOBDEFAULT);
        CSSTRCV_StrCpy(This->cvt, szHTTPRequest, size);
        size = CSSTRCV_Size(This->cvt);
        CSSTRCV_Get(This->cvt, szHTTPRequest);

        // null terminate what was read from client.
        szHTTPRequest[size] = 0;

        strcpy(szSpace, " ");
        strcpy(szDot, ".");
        strcpy(szComma, ",");
        strcpy(szColon, ":");
        strcpy(szNewline, "\x0D\x25");
        strcpy(szDoubleNewline, "\x0D\x25\x0D\x25");

        /////////////////////////////////////////////////////////////////////
        // Parse the HTTP request
        /////////////////////////////////////////////////////////////////////

        headers[0] = CSSTR_StrTok(szHTTPRequest, szNewline);

        i=1;
        while( (headers[i] = CSSTR_StrTok(0, szNewline)) != 0) {

           i++;
        }

        /////////////////////////////////////////////////////////////////////
        // process request line
        /////////////////////////////////////////////////////////////////////

        szMethod[0]      = 0;
        szService[0]     = 0;
        szHttpVersion[0] = 0;

        token = CSSTR_StrTok(headers[0], szSpace);
        iIndex = 0;

        while (token != 0) {

           switch(iIndex) {
              case 0:
                 strncpy(szMethod, token, 1024);
                 break;
              case 1:
                 strncpy(szService, token, 1024);
                 break;
              case 2:
                 strncpy(szHttpVersion, token, 1024);
                 break;
           }

           token = CSSTR_StrTok(0, szSpace);
           iIndex++;
        }

        /////////////////////////////////////////////////////////////////////
        // Get variable value pairs
        /////////////////////////////////////////////////////////////////////

        for (n=1; n<i; n++) {

           token = CSSTR_StrTok(headers[n], szColon);

           if (token == 0)
             break;

           CSSTR_Trim(szHTTPHeader, token);
           CSSTR_ToUpperCase(szHTTPHeader, 1024);

           if (!strcmp(szHTTPHeader, "SEC-WEBSOCKET-KEY")) {

              token = CSSTR_StrTok(0, szColon);

              if (token) {
                 CSSTR_Trim(szKeyHash, token);
              }
           }
           else if (!strcmp(token, "CONNECTION")) {

              token = CSSTR_StrTok(0, szColon);

              if (token)
                 CSSTR_Trim(szTempBuff, token);

              // This header can hold comma-separated values.
              // We want to find the upgrade header among the
              // possible values.

              token = CSSTR_StrTok(szTempBuff, szComma);

              while (token != 0) {

                 CSSTR_Trim(szTempBuff_2, token);

                 CSSTR_ToUpperCase(szTempBuff_2, 1024);

                 if (!strcmp(szTempBuff_2, "UPGRADE")) {
                    strncpy(szConnection, szTempBuff_2, 1024);

                    break;
                 }

                 token = CSSTR_StrTok(0, szComma);
              }
           }
           else if (!strcmp(token, "UPGRADE")) {

              token = CSSTR_StrTok(0, szColon);

              if (token)
                 CSSTR_Trim(szUpgrade, token);
           }
           else if (!strcmp(token, "SEC-WEBSOCKET-VERSION")) {

              token = CSSTR_StrTok(0, szColon);

              if (token)
                 CSSTR_Trim(szWebSocketVersion, token);
           }
        }

        /////////////////////////////////////////////////////////////////////
        // We send the response to the client. The client sent us
        // a challenge. We must compute SHA-1 hash to challenge
        // and then encoding it to Base 64.
        /////////////////////////////////////////////////////////////////////

        // WebSocket protocol requires to append this UUID to Key header
        strcat(szKeyHash, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

        // compute SHA-1 hash; for this we need to convert challenge to ASCII
        CSSTRCV_SetConversion(This->cvt, CCSID_JOBDEFAULT, CCSID_ASCII);
        CSSTRCV_StrCpy(This->cvt, szKeyHash, strlen(szKeyHash));
        CSSTRCV_Get(This->cvt, szKeyHash);

        HashAlgorithm.Algorithm = 2;  // SHA-1
        HashProvider = '0';
        error.iBytesProvided = 48;

        memset(InpuFmt, 0, 9);
        memset(AlgDesc, 0, 9);

        iSize = strlen(szKeyHash);
        Qc3CalculateHash(szKeyHash,
                         &iSize,
                         "DATA0100",
                         HashAlgorithm.padding,
                         "ALGD0500",
                         &HashProvider,
                         0,
                         szHash,
                         &error);

        szHash[20] = 0;  // null terminate resulting hash

        memset(szChallenge, 0, 129);

        /////////////////////////////////////////////////////////////////////
        // The hash needs to be encoded to BASE64
        /////////////////////////////////////////////////////////////////////

        CSSTRCV_SetConversion(This->cvt, CCSID_ASCII, CCSID_JOBDEFAULT);
        CSSTRCV_StrCpy(This->cvt, szHash, strlen(szHash));
        CSSTRCV_Get(This->cvt, szHash);

        CSSTR_ToBase64(szHash, 20, szChallenge, 128);

        // make HTTP response

        sprintf(szHTTPResponse,
                "HTTP/1.1 101 Switching Protocols\x0D\x25"
                "Upgrade: websocket\x0D\x25"
                "Sec-WebSocket-Accept: %s\x0D\x25"
                "Connection: Upgrade\x0D\x25\x0D\x25",
                szChallenge);

        // send handshake response
        size = (uint64_t)strlen(szHTTPResponse);

        // Convert response to ASCII
        CSSTRCV_SetConversion(This->cvt, CCSID_JOBDEFAULT, CCSID_ASCII);
        CSSTRCV_StrCpy(This->cvt, szHTTPResponse, size);
        CSSTRCV_Get(This->cvt, szHTTPResponse);

        hResult = CFS_Write(This->Connection,
                            szHTTPResponse,
                            &size,
                            -1);

        if (CS_FAIL(hResult)) {

           CSSTRCV_Destructor(&(This->cvt));

           CFS_Close(This->Connection);

           free(This);
           This = 0;
        }
     }
     else {

        CSSTRCV_Destructor(&(This->cvt));

        CFS_Close(This->Connection);

        free(This);
        This = 0;
     }
  }
  else {

     free(This);
     This = 0;
  }

  return This;

}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_Ping
//
// Sends a PING request over a non-secure connection.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CSWSCK. Use
//       the CSWSCK_OpenClient() or the CSWSCK_OpenServer()
//       function to initialise this instance.
//
// szData: The address of a data buffer that contains the data to send
//         over to the peer.
//
// iDataSize: The number of bytes in the buffer pointed to by szData.
//
// timeout: The number of seconds to wait for the PING operation to
//          succeed.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//    CS_FAILURE | CSWSCK_OPER_PING | CSWSCK_E_PARTIALDATA
//
//       Converting the data from EBCDIC to UTF8 yielded a number of
//       bytes larger than 125 bytes, which is the maximum number of
//       bytes to send alongside a PING oepration.
//
//    CS_FAILURE | CSWSCK_OPER_PING | diagnostics from CFSAPI
//
//       A networking error occured.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CSWSCK_Ping(CSWSCK*   This,
                     char*     szData,
                     uint64_t  iDataSize,
                     int       timeout)
{
  char szFrameHdr[2];
  char szOutData[125];
  int iSSLResult;

  uint64_t iHeaderSize;
  uint64_t iOutDataSize;

  CSRESULT hResult;

  // PING operation code and header size

  szFrameHdr[0] = 0x89;
  iHeaderSize = 2;

  if (szData != NULL) {

     CSSTRCV_StrCpy(This->cvt, szData, iDataSize);
     iOutDataSize = CSSTRCV_Size(This->cvt);

     if (iOutDataSize > 125) {

       //////////////////////////////////////////////////////////////////////
       // Must not send more than 125 bytes; an EBCDIC
       // characters may translate to more than one byte
       // therefore causing overflow...
       //////////////////////////////////////////////////////////////////////

       return CS_FAILURE | CSWSCK_OPER_PING | CSWSCK_E_PARTIALDATA;
     }
     else {

        CSSTRCV_Get(This->cvt, szOutData);

        // Just take first 125 bytes

        szFrameHdr[1] = 0x00 | iOutDataSize;

        // Send header
        hResult = CFS_Write(This->Connection,
                            szFrameHdr,
                            &iHeaderSize,
                            timeout);

        // Send data

        if (CS_SUCCEED(hResult)) {

           hResult = CFS_Write(This->Connection,
                               szOutData,
                               &iOutDataSize,
                               timeout);
        }
     }
  }
  else {

     szFrameHdr[1] = 0x00;

     // Send header only
     hResult = CFS_Write(This->Connection,
                         szFrameHdr,
                         &iHeaderSize,
                         timeout);
  }

  if (CS_FAIL(hResult)) {

     hResult = CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_Receive
//
// Waits for a websocket operation from a peer.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CSWSCK. Use
//       the CSWSCK_OpenClient() or the CSWSCK_OpenServer()
//       function to initialise this instance.
//
// iDataSize: The address of an unsigned 64 bit integer that will receive
//            the number of data bytes that were received. If the return
//            value is CS_FAILURE, this value is undefined and shouild
//            not be trusted.
//
// timeout: the number of seconds to wait for the peer.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//       This code can be combined with the following oepration codes:
//
//         CSWSCK_OPER_CONTINUATION
//         CSWSCK_OPER_TEXT
//         CSWSCK_OPER_BINARY
//         CSWSCK_OPER_CLOSE
//
//       This code indicates which websocket
//       operaation was erceived from the peer.
//
//       The success code may be supplemented with the following
//       diagnostic codes
//
//          CSWSCK_ALLDATA
//          CSWSCK_MOREDATA
//
//       When CSWSCK_MOREDATA is returned, this means the fin bit
//       was off and that a continuation packet can be expected.
//
//    CS_FAILURE
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CSWSCK_Receive(CSWSCK*   This,
                        uint64_t* iDataSize,
                        int       timeout) {

  char ws_mask[4];
  char ws_header[14];

  char* pData;

  int iSSLResult;
  int iBaseDataLength;

  ///////////////////////////////////////////////////////////////////////////
  // Since we can get continuation frames on a PING
  // operation, we must know what was the initial operation
  // on the first partial frame so that we can process PING
  // operations internally by this function.
  ///////////////////////////////////////////////////////////////////////////

  static int initialOperation = -1;

  long DataSize;
  long fragmentSize;
  long iCount;

  unsigned long MaskIsOn;

  int8_t   iDataSize_8;
  uint16_t iDataSize_16;
  uint64_t iDataSize_64;
  uint64_t iSize;
  uint64_t i;

  CSRESULT hResult;

  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////
  //
  // Branching label for PING/PONG reception
  CSWSCK_LABEL_RECEIVE:
  //
  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////

  // A websocket frame has at least 2 bytes

  iSize = 2;

  hResult = CFS_ReadRecord(This->Connection,
                           ws_header,
                           &iSize,
                           timeout);

  if (CS_SUCCEED(hResult)) {

     ///////////////////////////////////////////////////////////////////
     // Examine the basic length byte; this will determine
     // how many additional bytes we will be reading next.
     ///////////////////////////////////////////////////////////////////

     iBaseDataLength = CSWSCK_BaseLength(ws_header[1]);

     switch(iBaseDataLength)
     {
        case 126:

          iSize = 2;

          hResult = CFS_ReadRecord(This->Connection,
                                   (char*)&iDataSize_16,
                                   &iSize,
                                   timeout);

          This->dataSize = ntohs(iDataSize_16);

          break;

        case 127:

          iSize = 8;

          hResult = CFS_ReadRecord(This->Connection,
                                   (char*)&iDataSize_64,
                                   &iSize,
                                   timeout);

          This->dataSize = ntohll(iDataSize_64);
          break;

        default:

          This->dataSize = iBaseDataLength;

          break;
     }

     if (CS_FAIL(hResult)) {

        initialOperation = -1;
        return CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
     }

     ////////////////////////////////////////////////////////////////
     // We now read the data, if any
     ////////////////////////////////////////////////////////////////

     if (This->dataSize > 0) {

        /////////////////////////////////////////////////////////////
        // Determine if we have a mask (we should always get one) and
        // read it.
        /////////////////////////////////////////////////////////////

        MaskIsOn = 0;

        if (CSWSCK_MaskCode(ws_header[1]) == 1) {

           MaskIsOn = 1;
           iSize = 4;

           hResult = CFS_ReadRecord(This->Connection,
                                    ws_mask,
                                    &iSize,
                                    timeout);

        }

        if (CS_FAIL(hResult)) {

           initialOperation = -1;
           return CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
        }

        /////////////////////////////////////////////////////////////
        // Release previously allocated buffer if any
        // read it.
        /////////////////////////////////////////////////////////////

        if (This->dataBuffer != 0) {

           free(This->dataBuffer);
        }

        This->dataBuffer = (char*)malloc(This->dataSize);
        iSize = This->dataSize;

        /////////////////////////////////////////////////////////////
        // Read the data.
        /////////////////////////////////////////////////////////////

        hResult = CFS_ReadRecord(This->Connection,
                                 This->dataBuffer,
                                 &iSize,
                                 timeout);

        if (CS_FAIL(hResult)) {

           free(This->dataBuffer);
           This->dataBuffer = 0;
           initialOperation = -1;
           return CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
        }

        This->dataSize = iSize;

        if (MaskIsOn == 1)
        {
           /////////////////////////////////////////////////////////////
           // Unmask the data
           /////////////////////////////////////////////////////////////

           i=0;
           while (i<This->dataSize) {
              This->dataBuffer[i] = This->dataBuffer[i] ^ ws_mask[i % 4];
              i++;
           }
        }

        /////////////////////////////////////////////////////////////////////
        // Binary data must not be converted and a PING operation data
        // will not be returned to the called: PING data is already
        // in UTF8 and will not be converted since it must be sent back.
        /////////////////////////////////////////////////////////////////////

        if (CSWSCK_OpCode(ws_header[0]) != CSWSCK_OPER_BINARY &&
            CSWSCK_OpCode(ws_header[0]) != CSWSCK_OPER_PING) {

           //////////////////////////////////////////////////////////////////
           // This means the data is UTF8 format
           //////////////////////////////////////////////////////////////////

           CSSTRCV_SetConversion(This->cvt, CCSID_UTF8, CCSID_JOBDEFAULT);
           CSSTRCV_StrCpy(This->cvt, This->dataBuffer, This->dataSize);

           iSize = CSSTRCV_Size(This->cvt);

           if (iSize > This->dataSize) {

              ///////////////////////////////////////////////////////////////
              // This means the conversion yielded a larger buffer;
              // this is unlikely since we are converting to a
              // single byte character set but better safe than
              // sorry.
              ///////////////////////////////////////////////////////////////

              free(This->dataBuffer);
              This->dataBuffer = (char*)malloc(iSize);
           }

           CSSTRCV_Get(This->cvt, This->dataBuffer);
           This->dataSize = iSize;
        }
     }
     else {

        This->dataSize = 0;
     }

     *iDataSize = This->dataSize;

     switch(CSWSCK_OpCode(ws_header[0])) {

       case CSWSCK_OP_TEXT:

          initialOperation = CSWSCK_OP_TEXT;
          hResult = CS_SUCCESS | CSWSCK_OPER_TEXT;
          break;

       case CSWSCK_OP_CLOSE:

          initialOperation = CSWSCK_OP_CLOSE;
          hResult = CS_SUCCESS | CSWSCK_OPER_CLOSE;
          break;

       case CSWSCK_OP_BINARY:

          initialOperation = CSWSCK_OP_BINARY;
          hResult = CS_SUCCESS | CSWSCK_OPER_BINARY;
          break;

       case CSWSCK_OP_CONTINUATION:

          hResult = CS_SUCCESS | CSWSCK_OPER_CONTINUATION;
          break;

       case CSWSCK_OP_PING:

          // Send PONG response

          initialOperation = CSWSCK_OP_PING;
          CSLIST_Clear(This->internalData);

          if (This->dataSize > 0) {

             // The data must be sent back as a PONG response

             if (CSWSCK_Fin(ws_header[0])) {

                /////////////////////////////////////////////////////////////
                // We got all the data to send back
                /////////////////////////////////////////////////////////////

                hResult = CSWSCK_Send(This,
                                      CSWSCK_OPER_PONG,
                                      This->dataBuffer,
                                      This->dataSize,
                                      CSWSCK_FIN_ON,
                                      timeout);

                if (CS_FAIL(hResult)) {
                   return hResult;
                }
             }
             else {

                /////////////////////////////////////////////////////////////
                // We got a fraction of the data ...
                // the rest will come from a CONTINUATION frame
                // and we must receive again and send a PONG
                // once the FIN bit is set
                /////////////////////////////////////////////////////////////

                CSLIST_Insert(This->internalData,
                              This->dataBuffer,
                              This->dataSize,
                              CSLIST_BOTTOM);
             }
          }
          else {

             ////////////////////////////////////////////////////////////////
             // We got no data and hence just return an empty PONG.
             ////////////////////////////////////////////////////////////////

             hResult = CSWSCK_Send(This,
                                   CSWSCK_OPER_PONG,
                                   0,
                                   0,
                                   CSWSCK_FIN_ON,
                                   timeout);

             if (CS_FAIL(hResult)) {
                return hResult;
             }
          }

          // Resume reading ...
          goto CSWSCK_LABEL_RECEIVE;

       case CSWSCK_OP_PONG:

          initialOperation = CSWSCK_OP_PONG;

          // Resume reading ...
          goto CSWSCK_LABEL_RECEIVE;
    }

    if (CSWSCK_Fin(ws_header[0])) {

       //////////////////////////////////////////////////////////////////////
       // This means we got all the data for a given operation; the
       // initial operation flag must be reset.
       //////////////////////////////////////////////////////////////////////

       initialOperation = -1;
       hResult |= CSWSCK_ALLDATA;
    }
    else {

       hResult |= CSWSCK_MOREDATA;
    }
  }
  else {

     // Error getting first 2 bytes of protocol header
     hResult = CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_SecureClose
//
// Closes a secure session: this sends the CLOSE websocket operation
// to the peer. The connection is then closed.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CSWSCK. Use
//       the CSWSCK_SecureOpenClient() or the CSWSCK_SecureOpenServer()
//       function to initialise this instance.
//
// szData: The address of a data buffer that contains the data to send
//         over to the peer.
//
// iDataSize: The number of bytes in the buffer pointed to by szData.
//
// timeout: The number of seconds to wait for the PING operation to
//          succeed.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CSWSCK_SecureClose(CSWSCK*   This,
                            char*     szData,
                            uint64_t  iDataSize,
                            int       timeout)
{
   CSRESULT hResult;

   hResult = CS_FAILURE;

   if (This) {

      // Send Websocket close to client

      hResult = CSWSCK_SecureSend(This, CSWSCK_OPER_CLOSE,
                        szData, iDataSize, CSWSCK_FIN_ON, timeout);

      CFS_SecureClose(This->Connection);

      CSSTRCV_Destructor(&(This->cvt));

      free(This);
   }

   return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_SecureOpenServer
//
// Opens a secure websocket session.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// connfd: A socket descriptor to a TCP connection. This socket must be
//         already connected.
//
// szAppId: This parameter is ignored in this version.
//
// szOverflow: A pointer to a buffer that will receive any extra data
//             received from a web socket client. This should never
//             happen in practice and is just a precautionary measure.
//             If this parameter is NULL, then overflow data is ignored.
//
// iOverflowMax: The maximum number of overflow bytes to copy into the
//               szOverflow buffer, if not NULL.
//
// sessionInfo: The address of a data structure that will hold additional
//              data needed to establish a server session. At present, this
//              parameter is ignored.
//
// sessionInfoFmt: An integer identifying the format of the sessionInfo
//                 data structure. At present, this parameter is ignored.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    A pointer to a CSWSCK instance or NULL if the operation failed.
//
//
//////////////////////////////////////////////////////////////////////////////

CSWSCK* CSWSCK_SecureOpenServer(int   connfd,
                                char* szAppID,
                                void* sessionInfo,
                                int   sessionInfoFmt)
{
  char szHTTPRequest[65536];
  char szHTTPHeader[1025];
  char szHTTPResponse[1024];
  char szMethod[1025];
  char szService[1025];
  char szHttpVersion[1025];
  char szKeyHash[1025];
  char szWebSocketVersion[1025];
  char szConnection[1025];
  char szUpgrade[1025];
  char szTempBuff[1025];
  char szTempBuff_2[1025];
  char szChallenge[129];
  char InpuFmt[9];
  char AlgDesc[9];
  char szHash[21];

  // Constants

  char szSpace[10];
  char szDot[10];
  char szComma[10];
  char szColon[10];
  char szNewline[10];
  char szDoubleNewline[10];

  char HashProvider;

  char* headers[120];
  char* szHTTPOverflow;
  char* token;

  int iIndex;
  int i;
  int n;
  int iSSLResult;
  int iHeaderOffsets[33];
  int iSize;

  long position;
  long iOverflowSize;

  union {
    char bError[48];
    int iBytesProvided;
  } error;

  union {
    char padding[20];
    int Algorithm;
  }HashAlgorithm;

  uint64_t size;

  CSWSCK*       This;
  CSRESULT      hResult;
  CFS_INSTANCE* Connection;
  CSSTRCV       cvts;

  This = (CSWSCK*)malloc(sizeof(CSWSCK));

  ///////////////////////////////////////////////////////////////////////////
  // Create conversion objects
  ///////////////////////////////////////////////////////////////////////////

  This->Connection = CFS_SecureOpen(connfd,
                                    szAppID,
                                    CFS_SESSIONTYPE_SERVER,
                                    0,
                                    0,
                                    &iSSLResult);

  if (This->Connection != 0) {

     This->cvt = CSSTRCV_Constructor();

     size = 65535;
     hResult = CFS_SecureRead(This->Connection,
                              szHTTPRequest,
                              &size,
                              -1,
                              &iSSLResult);

     if (CS_SUCCEED(hResult)) {

        /////////////////////////////////////////////////////////////////////
        // Convert from ASCII to EBCDIC
        /////////////////////////////////////////////////////////////////////

        CSSTRCV_SetConversion(This->cvt, CCSID_ASCII, CCSID_JOBDEFAULT);
        CSSTRCV_StrCpy(This->cvt, szHTTPRequest, size);
        size = CSSTRCV_Size(This->cvt);
        CSSTRCV_Get(This->cvt, szHTTPRequest);

        // null terminate what was read from client.
        szHTTPRequest[size] = 0;

        strcpy(szSpace, " ");
        strcpy(szDot, ".");
        strcpy(szComma, ",");
        strcpy(szColon, ":");
        strcpy(szNewline, "\x0D\x25");
        strcpy(szDoubleNewline, "\x0D\x25\x0D\x25");

        /////////////////////////////////////////////////////////////////////
        // Parse the HTTP request
        /////////////////////////////////////////////////////////////////////

        headers[0] = CSSTR_StrTok(szHTTPRequest, szNewline);

        i=1;
        while( (headers[i] = CSSTR_StrTok(0, szNewline)) != 0) {

           i++;
        }

        /////////////////////////////////////////////////////////////////////
        // process request line
        /////////////////////////////////////////////////////////////////////

        szMethod[0]      = 0;
        szService[0]     = 0;
        szHttpVersion[0] = 0;

        token = CSSTR_StrTok(headers[0], szSpace);
        iIndex = 0;

        while (token != 0) {

           switch(iIndex) {
              case 0:
                 strncpy(szMethod, token, 1024);
                 break;
              case 1:
                 strncpy(szService, token, 1024);
                 break;
              case 2:
                 strncpy(szHttpVersion, token, 1024);
                 break;
           }

           token = CSSTR_StrTok(0, szSpace);
           iIndex++;
        }

        /////////////////////////////////////////////////////////////////////
        // Get variable value pairs
        /////////////////////////////////////////////////////////////////////

        for (n=1; n<i; n++) {

           token = CSSTR_StrTok(headers[n], szColon);

           if (token == 0)
             break;

           CSSTR_Trim(szHTTPHeader, token);
           CSSTR_ToUpperCase(szHTTPHeader, 1024);

           if (!strcmp(szHTTPHeader, "SEC-WEBSOCKET-KEY")) {

              token = CSSTR_StrTok(0, szColon);

              if (token) {
                 CSSTR_Trim(szKeyHash, token);
              }
           }
           else if (!strcmp(token, "CONNECTION")) {

              token = CSSTR_StrTok(0, szColon);

              if (token)
                 CSSTR_Trim(szTempBuff, token);

              // This header can hold comma-separated values.
              // We want to find the upgrade header among the
              // possible values.

              token = CSSTR_StrTok(szTempBuff, szComma);

              while (token != 0) {

                 CSSTR_Trim(szTempBuff_2, token);

                 CSSTR_ToUpperCase(szTempBuff_2, 1024);

                 if (!strcmp(szTempBuff_2, "UPGRADE")) {
                    strncpy(szConnection, szTempBuff_2, 1024);

                    break;
                 }

                 token = CSSTR_StrTok(0, szComma);
              }
           }
           else if (!strcmp(token, "UPGRADE")) {

              token = CSSTR_StrTok(0, szColon);

              if (token)
                 CSSTR_Trim(szUpgrade, token);
           }
           else if (!strcmp(token, "SEC-WEBSOCKET-VERSION")) {

              token = CSSTR_StrTok(0, szColon);

              if (token)
                 CSSTR_Trim(szWebSocketVersion, token);
           }
        }

        /////////////////////////////////////////////////////////////////////
        // We send the response to the client. The client sent us
        // a challenge. We must compute SHA-1 hash to challenge
        // and then encoding it to Base 64.
        /////////////////////////////////////////////////////////////////////

        // WebSocket protocol requires to append this UUID to Key header
        strcat(szKeyHash, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

        // compute SHA-1 hash; for this we need to convert challenge to ASCII
        CSSTRCV_SetConversion(This->cvt, CCSID_JOBDEFAULT, CCSID_ASCII);
        CSSTRCV_StrCpy(This->cvt, szKeyHash, strlen(szKeyHash));
        CSSTRCV_Get(This->cvt, szKeyHash);

        HashAlgorithm.Algorithm = 2;  // SHA-1
        HashProvider = '0';
        error.iBytesProvided = 48;

        memset(InpuFmt, 0, 9);
        memset(AlgDesc, 0, 9);

        iSize = strlen(szKeyHash);
        Qc3CalculateHash(szKeyHash,
                         &iSize,
                         "DATA0100",
                         HashAlgorithm.padding,
                         "ALGD0500",
                         &HashProvider,
                         0,
                         szHash,
                         &error);

        szHash[20] = 0;  // null terminate resulting hash

        memset(szChallenge, 0, 129);

        /////////////////////////////////////////////////////////////////////
        // The hash needs to be encoded to BASE64
        /////////////////////////////////////////////////////////////////////

        CSSTRCV_SetConversion(This->cvt, CCSID_ASCII, CCSID_JOBDEFAULT);
        CSSTRCV_StrCpy(This->cvt, szHash, strlen(szHash));
        CSSTRCV_Get(This->cvt, szHash);

        CSSTR_ToBase64(szHash, 20, szChallenge, 128);

        // make HTTP response

        sprintf(szHTTPResponse,
                "HTTP/1.1 101 Switching Protocols\x0D\x25"
                "Upgrade: websocket\x0D\x25"
                "Sec-WebSocket-Accept: %s\x0D\x25"
                "Connection: Upgrade\x0D\x25\x0D\x25",
                szChallenge);

        // send handshake response
        size = (uint64_t)strlen(szHTTPResponse);

        // Convert response to ASCII
        CSSTRCV_SetConversion(This->cvt, CCSID_JOBDEFAULT, CCSID_ASCII);
        CSSTRCV_StrCpy(This->cvt, szHTTPResponse, size);
        CSSTRCV_Get(This->cvt, szHTTPResponse);

        hResult = CFS_SecureWrite(This->Connection,
                                  szHTTPResponse,
                                  &size,
                                  -1,
                                  &iSSLResult);

        if (CS_FAIL(hResult)) {

           CSSTRCV_Destructor(&(This->cvt));

           CFS_SecureClose(This->Connection);

           free(This);
           This = 0;
        }
     }
     else {

        CSSTRCV_Destructor(&(This->cvt));

        CFS_SecureClose(This->Connection);

        free(This);
        This = 0;
     }
  }
  else {

     free(This);
     This = 0;
  }

  return This;

}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_SecurePing
//
// Sends a PING request over a secure connection.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CSWSCK. Use
//       the CSWSCK_SecureOpenClient() or the CSWSCK_SecureOpenServer()
//       function to initialise this instance.
//
// szData: The address of a data buffer that contains the data to send
//         over to the peer.
//
// iDataSize: The number of bytes in the buffer pointed to by szData.
//
// timeout: The number of seconds to wait for the PING operation to
//          succeed.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//    CS_FAILURE | CSWSCK_OPER_PING | CSWSCK_E_PARTIALDATA
//
//       Converting the data from EBCDIC to UTF8 yielded a number of
//       bytes larger than 125 bytes, which is the maximum number of
//       bytes to send alongside a PING oepration.
//
//    CS_FAILURE | CSWSCK_OPER_PING | diagnostics from CFSAPI
//
//       A networking error occured.
//
///////////////////////////////////////////////////////////////////////////////

CSRESULT CSWSCK_SecurePing(CSWSCK*   This,
                           char*     szData,
                           uint64_t  iDataSize,
                           int       timeout)
{
  char szFrameHdr[2];
  char szOutData[125];
  int iSSLResult;

  uint64_t iHeaderSize;
  uint64_t iOutDataSize;

  CSRESULT hResult;

  // PING operation code and header size

  szFrameHdr[0] = 0x89;
  iHeaderSize = 2;

  if (szData != NULL) {

     CSSTRCV_StrCpy(This->cvt, szData, iDataSize);
     iOutDataSize = CSSTRCV_Size(This->cvt);

     if (iOutDataSize > 125) {

       //////////////////////////////////////////////////////////////////////
       // Must not send more than 125 bytes; an EBCDIC
       // characters may translate to more than one byte
       // therefore causing overflow...
       //////////////////////////////////////////////////////////////////////

       return CS_FAILURE | CSWSCK_OPER_PING | CSWSCK_E_PARTIALDATA;
     }
     else {

        CSSTRCV_Get(This->cvt, szOutData);

        // Just take first 125 bytes

        szFrameHdr[1] = 0x00 | iOutDataSize;

        // Send header
        hResult = CFS_SecureWrite(This->Connection,
                                  szFrameHdr,
                                  &iHeaderSize,
                                  timeout,
                                  &iSSLResult);

        // Send data

        if (CS_SUCCEED(hResult)) {

           hResult = CFS_SecureWrite(This->Connection,
                                     szOutData,
                                     &iOutDataSize,
                                     timeout,
                                     &iSSLResult);
        }
     }
  }
  else {

     szFrameHdr[1] = 0x00;

     // Send header only
     hResult = CFS_SecureWrite(This->Connection,
                               szFrameHdr,
                               &iHeaderSize,
                               timeout,
                               &iSSLResult);
  }

  if (CS_FAIL(hResult)) {

     hResult = CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_SecureReceive
//
// Waits for a websocket operation from a peer.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CSWSCK. Use
//       the CSWSCK_SecureOpenClient() or the CSWSCK_SecureOpenServer()
//       function to initialise this instance.
//
// iDataSize: The address of an unsigned 64 bit integer that will receive
//            the number of data bytes that were received. If the return
//            value is CS_FAILURE, this value is undefined and shouild
//            not be trusted.
//
// timeout: the number of seconds to wait for the peer.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//       This code can be combined with the following oepration codes:
//
//         CSWSCK_OPER_CONTINUATION
//         CSWSCK_OPER_TEXT
//         CSWSCK_OPER_BINARY
//         CSWSCK_OPER_CLOSE
//
//       This code indicates which websocket
//       operaation was erceived from the peer.
//
//       The success code may be supplemented with the following
//       diagnostic codes
//
//          CSWSCK_ALLDATA
//          CSWSCK_MOREDATA
//
//       When CSWSCK_MOREDATA is returned, this means the fin bit
//       was off and that a continuation packet can be expected.
//
//    CS_FAILURE
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CSWSCK_SecureReceive(CSWSCK*   This,
                              uint64_t* iDataSize,
                              int       timeout) {

  char ws_mask[4];
  char ws_header[14];

  char* pData;

  int iSSLResult;
  int iBaseDataLength;

  ///////////////////////////////////////////////////////////////////////////
  // Since we can get continuation frames on a PING
  // operation, we must know what was the initial operation
  // on the first partial frame so that we can process PING
  // operations internally by this function.
  ///////////////////////////////////////////////////////////////////////////

  static int initialOperation = -1;

  long DataSize;
  long fragmentSize;
  long iCount;

  unsigned long MaskIsOn;

  int8_t   iDataSize_8;
  uint16_t iDataSize_16;
  uint64_t iDataSize_64;
  uint64_t iSize;
  uint64_t i;

  CSRESULT hResult;

  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////
  //
  // Branching label for PING/PONG reception
  CSWSCK_LABEL_RECEIVE:
  //
  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////

  // A websocket frame has at least 2 bytes

  iSize = 2;

  hResult = CFS_SecureReadRecord(This->Connection,
                                 ws_header,
                                 &iSize,
                                 timeout,
                                 &iSSLResult);

  if (CS_SUCCEED(hResult)) {

     ///////////////////////////////////////////////////////////////////
     // Examine the basic length byte; this will determine
     // how many additional bytes we will be reading next.
     ///////////////////////////////////////////////////////////////////

     iBaseDataLength = CSWSCK_BaseLength(ws_header[1]);

     switch(iBaseDataLength)
     {
        case 126:

          iSize = 2;

          hResult = CFS_SecureReadRecord(This->Connection,
                                         (char*)&iDataSize_16,
                                         &iSize,
                                         timeout,
                                         &iSSLResult);

          This->dataSize = ntohs(iDataSize_16);

          break;

        case 127:

          iSize = 8;

          hResult = CFS_SecureReadRecord(This->Connection,
                                         (char*)&iDataSize_64,
                                         &iSize,
                                         timeout,
                                         &iSSLResult);

          This->dataSize = ntohll(iDataSize_64);
          break;

        default:

          This->dataSize = iBaseDataLength;

          break;
     }

     if (CS_FAIL(hResult)) {

        initialOperation = -1;
        return CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
     }

     ////////////////////////////////////////////////////////////////
     // We now read the data, if any
     ////////////////////////////////////////////////////////////////

     if (This->dataSize > 0) {

        /////////////////////////////////////////////////////////////
        // Determine if we have a mask (we should always get one) and
        // read it.
        /////////////////////////////////////////////////////////////

        MaskIsOn = 0;

        if (CSWSCK_MaskCode(ws_header[1]) == 1) {

           MaskIsOn = 1;
           iSize = 4;

           hResult = CFS_SecureReadRecord(This->Connection,
                                          ws_mask,
                                          &iSize,
                                          timeout,
                                          &iSSLResult);

        }

        if (CS_FAIL(hResult)) {

           initialOperation = -1;
           return CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
        }

        /////////////////////////////////////////////////////////////
        // Release previously allocated buffer if any
        // read it.
        /////////////////////////////////////////////////////////////

        if (This->dataBuffer != 0) {

           free(This->dataBuffer);
        }

        This->dataBuffer = (char*)malloc(This->dataSize);
        iSize = This->dataSize;

        /////////////////////////////////////////////////////////////
        // Read the data.
        /////////////////////////////////////////////////////////////

        hResult = CFS_SecureReadRecord(This->Connection,
                                       This->dataBuffer,
                                       &iSize,
                                       timeout,
                                       &iSSLResult);

        if (CS_FAIL(hResult)) {

           free(This->dataBuffer);
           This->dataBuffer = 0;
           initialOperation = -1;
           return CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
        }

        This->dataSize = iSize;

        if (MaskIsOn == 1)
        {
           /////////////////////////////////////////////////////////////
           // Unmask the data
           /////////////////////////////////////////////////////////////

           i=0;
           while (i<This->dataSize) {
              This->dataBuffer[i] = This->dataBuffer[i] ^ ws_mask[i % 4];
              i++;
           }
        }

        /////////////////////////////////////////////////////////////////////
        // Binary data must not be converted and a PING operation data
        // will not be returned to the called: PING data is already
        // in UTF8 and will not be converted since it must be sent back.
        /////////////////////////////////////////////////////////////////////

        if (CSWSCK_OpCode(ws_header[0]) != CSWSCK_OPER_BINARY &&
            CSWSCK_OpCode(ws_header[0]) != CSWSCK_OPER_PING) {

           //////////////////////////////////////////////////////////////////
           // This means the data is UTF8 format
           //////////////////////////////////////////////////////////////////

           CSSTRCV_SetConversion(This->cvt, CCSID_UTF8, CCSID_JOBDEFAULT);
           CSSTRCV_StrCpy(This->cvt, This->dataBuffer, This->dataSize);

           iSize = CSSTRCV_Size(This->cvt);

           if (iSize > This->dataSize) {

              ///////////////////////////////////////////////////////////////
              // This means the conversion yielded a larger buffer;
              // this is unlikely since we are converting to a
              // single byte character set but better safe than
              // sorry.
              ///////////////////////////////////////////////////////////////

              free(This->dataBuffer);
              This->dataBuffer = (char*)malloc(iSize);
           }

           CSSTRCV_Get(This->cvt, This->dataBuffer);
           This->dataSize = iSize;
        }
     }
     else {

        if (This->dataBuffer != 0) {

           free(This->dataBuffer);
        }

        This->dataBuffer == 0
        This->dataSize = 0;
     }

     *iDataSize = This->dataSize;

     switch(CSWSCK_OpCode(ws_header[0])) {

       case CSWSCK_OP_TEXT:

          initialOperation = CSWSCK_OP_TEXT;
          hResult = CS_SUCCESS | CSWSCK_OPER_TEXT;
          break;

       case CSWSCK_OP_CLOSE:

          initialOperation = CSWSCK_OP_CLOSE;
          hResult = CS_SUCCESS | CSWSCK_OPER_CLOSE;
          break;

       case CSWSCK_OP_BINARY:

          initialOperation = CSWSCK_OP_BINARY;
          hResult = CS_SUCCESS | CSWSCK_OPER_BINARY;
          break;

       case CSWSCK_OP_CONTINUATION:

          hResult = CS_SUCCESS | CSWSCK_OPER_CONTINUATION;
          break;

       case CSWSCK_OP_PING:

          // Send PONG response

          initialOperation = CSWSCK_OP_PING;
          CSLIST_Clear(This->internalData);

          if (This->dataSize > 0) {

             // The data must be sent back as a PONG response

             if (CSWSCK_Fin(ws_header[0])) {

                /////////////////////////////////////////////////////////////
                // We got all the data to send back
                /////////////////////////////////////////////////////////////

                hResult = CSWSCK_SecureSend(This,
                                            CSWSCK_OPER_PONG,
                                            This->dataBuffer,
                                            This->dataSize,
                                            CSWSCK_FIN_ON,
                                            timeout);

                if (CS_FAIL(hResult)) {
                   return hResult;
                }
             }
             else {

                /////////////////////////////////////////////////////////////
                // We got a fraction of the data ...
                // the rest will come from a CONTINUATION frame
                // and we must receive again and send a PONG
                // once the FIN bit is set
                /////////////////////////////////////////////////////////////

                CSLIST_Insert(This->internalData,
                              This->dataBuffer,
                              This->dataSize,
                              CSLIST_BOTTOM);
             }
          }
          else {

             /////////////////////////////////////////////////////////////
             // We got no data and hence just return an empty PONG.
             /////////////////////////////////////////////////////////////

             hResult = CSWSCK_SecureSend(This,
                                         CSWSCK_OPER_PONG,
                                         0,
                                         0,
                                         CSWSCK_FIN_ON,
                                         timeout);

             if (CS_FAIL(hResult)) {
                return hResult;
             }
          }

          // Resume reading ...
          goto CSWSCK_LABEL_RECEIVE;

       case CSWSCK_OP_PONG:

          initialOperation = CSWSCK_OP_PONG;

          // Resume reading ...
          goto CSWSCK_LABEL_RECEIVE;
    }

    if (CSWSCK_Fin(ws_header[0])) {

       //////////////////////////////////////////////////////////////////////
       // This means we got all the data for a given operation; the
       // initial operation flag must be reset.
       //////////////////////////////////////////////////////////////////////

       initialOperation = -1;
       hResult |= CSWSCK_ALLDATA;
    }
    else {

       hResult |= CSWSCK_MOREDATA;
    }
  }
  else {

     // Error getting first 2 bytes of protocol header
     hResult = CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_SecureSend
//
// Sends a websocket operation to a peer. The CLOSE operation is not
// supported as it should be sent by using the CSWSCK_SecureClose()
// function.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CSWSCK. Use
//       the CSWSCK_SecureOpenClient() or the CSWSCK_SecureOpenServer()
//       function to initialise this instance.
//
// operation: The websocket operation to send over to the peer: must
//            be one of the following values:
//
//            CSWSCK_OPER_CONTINUATION
//            CSWSCK_OPER_TEXT
//            CSWSCK_OPER_BINARY
//            CSWSCK_OPER_CLOSE
//            CSWSCK_OPER_PING
//            CSWSCK_OPER_PONG
//
// data: a pointer to the buffer to send over to the peer.
//
// iDataSize: The address of an unsigned 64 bit integer that holds the
//            the number of data bytes that must be sent to the peer.
//
// fin: An integer indicating if this is the last data segment sent.
//      Possible values are:
//
//     CSWSCK_FIN_OFF: More data will be forthcoming in a CONTINUATION
//                     operation.
//
//     CSWSCK_FIN_ON: No more data will be sent for this
//                     operation.
//
// timeout: the number of seconds to wait until the data is sent.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//    CS_FAILURE
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CSWSCK_SecureSend(CSWSCK*  This,
                           long     operation,
                           char*    data,
                           uint64_t iDataSize,
                           char     fin,
                           int      timeout)
{
  uint16_t iSize16;
  uint64_t iSize64;
  uint64_t iSize;
  uint64_t iOutDataSize;

  char ws_header[14];

  char* szOutBuffer;

  int iSSLResult;

  long iCount;
  long i;
  long iNodeLength;

  uint64_t iNodeLength64;

  CSRESULT hResult;

  if (operation == CSWSCK_OPER_CLOSE) {
     return CS_FAILURE | CSWSCK_OPER_CLOSE | CSWSCK_E_NOTSUPPORTED;
  }

  memset(ws_header, 0, 14);
  szOutBuffer = 0;

  if (data != 0 && iDataSize > 0)
  {
     if (operation != CSWSCK_OPER_BINARY) {

        /////////////////////////////////////////////////////////////////////
        // Convert data to UTF8
        /////////////////////////////////////////////////////////////////////

        CSSTRCV_SetConversion(This->cvt, CCSID_JOBDEFAULT, CCSID_UTF8);
        CSSTRCV_StrCpy(This->cvt, data, iDataSize);
        iOutDataSize = CSSTRCV_Size(This->cvt);

        szOutBuffer = (char*)malloc(iOutDataSize * sizeof(char));

        CSSTRCV_Get(This->cvt, szOutBuffer);
     }
     else {
        iOutDataSize = iDataSize;
     }

     if (fin & CSWSCK_FIN_ON) {
        ws_header[0] = 0x80 | operation >> 16;
     }
     else {
        ws_header[0] = 0x00 | operation >> 16;
     }

     if (iOutDataSize < 126) {
        ws_header[1] = 0x00 | iOutDataSize;
        iSize = 2;
     }
     else if (iOutDataSize < 65536) {
        ws_header[1] = 0x00 | 126;
        iSize16 = htons(iOutDataSize);
        memcpy(&ws_header[2], &iSize16, sizeof(uint16_t));
        iSize = 4;
     }
     else {
        ws_header[1] = 0x00 | 127;

        // For Portabiliy; AS400 is already in NBO
        iSize64 = htonll(iOutDataSize);
        memcpy(&ws_header[2], &iSize64, sizeof(uint64_t));
        iSize = 10;
     }

     // Send frame header.
     hResult = CFS_SecureWriteRecord(This->Connection,
                                     ws_header,
                                     &iSize,
                                     timeout,
                                     &iSSLResult);

     if (CS_SUCCEED(hResult)) {

        if (operation != CSWSCK_OPER_BINARY) {

           // Send UTF8 data.
           hResult = CFS_SecureWriteRecord(This->Connection,
                                           szOutBuffer,
                                           &iOutDataSize,
                                           timeout,
                                           &iSSLResult);
           free(szOutBuffer);
        }
        else {

           // Send binary data.
           hResult = CFS_SecureWriteRecord(This->Connection,
                                           data,
                                           &iOutDataSize,
                                           timeout,
                                           &iSSLResult);
        }
     }
  }
  else {

     ////////////////////////////////////////////////////////////////////////
     // Send empty frame: the operation should only be
     // one of the following:
     //
     //   CLOSE
     //   PING
     //   PONG
     //
     ////////////////////////////////////////////////////////////////////////

     switch(operation) {

        case CSWSCK_OPER_PING:
        case CSWSCK_OPER_PONG:

           ws_header[0] = 0x80 | (operation >> 16);
           ws_header[1] = 0x00;

           iOutDataSize = 2;

           hResult = CFS_SecureWriteRecord(This->Connection,
                                           data,
                                           &iOutDataSize,
                                           timeout,
                                           &iSSLResult);

           break;

        default:

           hResult = CS_FAILURE | operation | CSWSCK_E_NOTSUPPORTED;
     }
  }

  if (CS_SUCCEED(hResult)) {
     hResult = CS_SUCCESS;
  }
  else {
     hResult = CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_Send
//
// Sends a websocket operation to a peer over a non secure connection.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CSWSCK. Use
//       the CSWSCK_OpenClient() or the CSWSCK_OpenServer()
//       function to initialise this instance.
//
// operation: The websocket operation to send over to the peer: must
//            be one of the following values:
//
//            CSWSCK_OPER_CONTINUATION
//            CSWSCK_OPER_TEXT
//            CSWSCK_OPER_BINARY
//            CSWSCK_OPER_CLOSE
//            CSWSCK_OPER_PING
//            CSWSCK_OPER_PONG
//
// data: a pointer to the buffer to send over to the peer.
//
// iDataSize: The address of an unsigned 64 bit integer that holds the
//            the number of data bytes that must be sent to the peer.
//
// fin: An integer indicating if this is the last data segment sent.
//      Possible values are:
//
//     CSWSCK_FIN_OFF: More data will be forthcoming in a CONTINUATION
//                     operation.
//
//     CSWSCK_FIN_ON: No more data will be sent for this
//                     operation.
//
// timeout: the number of seconds to wait until the data is sent.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//    CS_FAILURE
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CSWSCK_Send(CSWSCK*  This,
                     long     operation,
                     char*    data,
                     uint64_t iDataSize,
                     char     fin,
                     int      timeout)
{
  uint16_t iSize16;
  uint64_t iSize64;
  uint64_t iSize;
  uint64_t iOutDataSize;

  char ws_header[14];

  char* szOutBuffer;

  int iSSLResult;

  long iCount;
  long i;
  long iNodeLength;

  uint64_t iNodeLength64;

  CSRESULT hResult;

  if (operation == CSWSCK_OPER_CLOSE) {
     return CS_FAILURE | CSWSCK_OPER_CLOSE | CSWSCK_E_NOTSUPPORTED;
  }

  memset(ws_header, 0, 14);
  szOutBuffer = 0;

  if (data != 0 && iDataSize > 0)
  {
     if (operation != CSWSCK_OPER_BINARY) {

        /////////////////////////////////////////////////////////////////////
        // Convert data to UTF8
        /////////////////////////////////////////////////////////////////////

        CSSTRCV_SetConversion(This->cvt, CCSID_JOBDEFAULT, CCSID_UTF8);
        CSSTRCV_StrCpy(This->cvt, data, iDataSize);
        iOutDataSize = CSSTRCV_Size(This->cvt);

        szOutBuffer = (char*)malloc(iOutDataSize * sizeof(char));

        CSSTRCV_Get(This->cvt, szOutBuffer);
     }
     else {
        iOutDataSize = iDataSize;
     }

     if (fin & CSWSCK_FIN_ON) {
        ws_header[0] = 0x80 | operation >> 16;
     }
     else {
        ws_header[0] = 0x00 | operation >> 16;
     }

     if (iOutDataSize < 126) {
        ws_header[1] = 0x00 | iOutDataSize;
        iSize = 2;
     }
     else if (iOutDataSize < 65536) {
        ws_header[1] = 0x00 | 126;
        iSize16 = htons(iOutDataSize);
        memcpy(&ws_header[2], &iSize16, sizeof(uint16_t));
        iSize = 4;
     }
     else {
        ws_header[1] = 0x00 | 127;

        // For Portabiliy; AS400 is already in NBO
        iSize64 = htonll(iOutDataSize);
        memcpy(&ws_header[2], &iSize64, sizeof(uint64_t));
        iSize = 10;
     }

     // Send frame header.
     hResult = CFS_WriteRecord(This->Connection,
                               ws_header,
                               &iSize,
                               timeout);

     if (CS_SUCCEED(hResult)) {

        if (operation != CSWSCK_OPER_BINARY) {

           // Send UTF8 data.
           hResult = CFS_WriteRecord(This->Connection,
                                     szOutBuffer,
                                     &iOutDataSize,
                                     timeout);

           free(szOutBuffer);
        }
        else {

           // Send binary data.
           hResult = CFS_WriteRecord(This->Connection,
                                     data,
                                     &iOutDataSize,
                                     timeout);
        }
     }
  }
  else {

     ////////////////////////////////////////////////////////////////////////
     // Send empty frame: the operation should only be
     // one of the following:
     //
     //   CLOSE
     //   PING
     //   PONG
     //
     ////////////////////////////////////////////////////////////////////////

     switch(operation) {

        case CSWSCK_OPER_PING:
        case CSWSCK_OPER_PONG:

           ws_header[0] = 0x80 | (operation >> 16);
           ws_header[1] = 0x00;

           iOutDataSize = 2;

           hResult = CFS_WriteRecord(This->Connection,
                                     data,
                                     &iOutDataSize,
                                     timeout);

           break;

        default:

           hResult = CS_FAILURE | operation | CSWSCK_E_NOTSUPPORTED;
     }
  }

  if (CS_SUCCEED(hResult)) {
     hResult = CS_SUCCESS;
  }
  else {
     hResult = CS_FAILURE | CSWSCK_OPER_CFSAPI | CS_DIAG(hResult);
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// Private functions
//
//////////////////////////////////////////////////////////////////////////////

uint64_t ntohll(uint64_t value)
{
  union  {
     char bytes[2];
     uint16_t testVal;
  }  Integer16;

  union  {
     char bytes[8];
     uint64_t hll;
  }  Integer64;

  // Determine host endian-ness
  Integer16.testVal = 0x0001;

  if (Integer16.bytes[0] == 0x01) {

     // Little endian, we must convert

     Integer64.bytes[0] = value >> 56;
     Integer64.bytes[1] = value >> 48;
     Integer64.bytes[2] = value >> 40;
     Integer64.bytes[3] = value >> 32;
     Integer64.bytes[4] = value >> 24;
     Integer64.bytes[5] = value >> 16;
     Integer64.bytes[6] = value >> 8;
     Integer64.bytes[7] = value >> 0;

     return Integer64.hll;
  }

  return value;
}

uint64_t htonll(uint64_t value)
{
  return (ntohll(value));
}
