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

#ifndef __CLARASOFT_CFS_CSWSCK_H__
#define __CLARASOFT_CFS_CSWSCK_H__

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

CSRESULT
  CSWSCK_Close
    (CSWSCK    This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_Connect
//
// Establishes a non-secure connection to a websocket server.
// NOT IMPLEMENTED YET.
//
//////////////////////////////////////////////////////////////////////////////

CSWSCK
  CSWSCK_Connect
    (void* sessionInfo,
     int   sessionInfoFmt);

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
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSWSCK_GetData
    (CSWSCK    This,
     char*     szBuffer,
     uint64_t  offset,
     uint64_t  iMaxDataSize);

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_OpenChannel
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

CSWSCK
  CSWSCK_OpenChannel
    (int   connfd,
     void* sessionInfo,
     int   sessionInfoFmt);

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

CSRESULT
  CSWSCK_Ping
    (CSWSCK    This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

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

CSRESULT
  CSWSCK_Receive
    (CSWSCK    This,
     uint64_t* iDataSize,
     int       timeout);

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

CSRESULT
  CSWSCK_SecureClose
    (CSWSCK    This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_SecureConnect
//
// Establishes a secure connection to a websocket server.
// NOT IMPLEMENTED YET.
//
//////////////////////////////////////////////////////////////////////////////

CSWSCK
  CSWSCK_SecureConnect
    (void* sessionInfo,
     int   sessionInfoFmt);

//////////////////////////////////////////////////////////////////////////////
//
// CSWSCK_SecureOpenChannel
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

CSWSCK*
  CSWSCK_SecureOpenChannel
    (int   connfd,
     void* sessionInfo,
     int   sessionInfoFmt);

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

CSRESULT
  CSWSCK_SecurePing
    (CSWSCK    This,
     char*     szData,
     uint64_t  iDataSize,
     int       timeout);

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

CSRESULT
  CSWSCK_SecureReceive
    (CSWSCK    This,
     uint64_t* iDataSize,
     int       timeout);

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

CSRESULT
  CSWSCK_SecureSend
    (CSWSCK    This,
     long      operation,
     char*     szData,
     uint64_t  iDataSize,
     char      fin,
     int       timeout);

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

CSRESULT
  CSWSCK_Send
    (CSWSCK    This,
     long      operation,
     char*     szData,
     uint64_t  iDataSize,
     char      fin,
     int       timeout);

#endif
