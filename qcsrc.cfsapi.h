/* ===========================================================================
  Common Service Access Protocol
  cfsapi.h

  Networking Primitives definitions
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

#ifndef __CFS_CFSAPI_H__
#define __CFS_CFSAPI_H__

#include <gskssl.h>
#include <inttypes.h>
#include <netdb.h>
#include <QUSRJOBI.h>
#include <QSYRUSRI.h>
#include "qcsrc/cscore.h"

//////////////////////////////////////////////////////////////////////////////
// Clarasoft Foundation Server Definitions
//////////////////////////////////////////////////////////////////////////////

#define CFS_USERNOTFOUND              (0x00001001)
#define CFS_USERDISABLED              (0x00001002)

#define CFS_NTOP_ADDR_MAX             (1025)
#define CFS_NTOP_PORT_MAX             (9)

#define CFS_SSL_MAXRECORDSIZE         (16383UL)

#define CFS_UUID_BUFFERSIZE           (37)
#define CFS_UUID_UPPERCASE            (0x00000000)
#define CFS_UUID_LOWERCASE            (0x00000001)
#define CFS_UUID_DASHES               (0x00000002)

#define CFS_SESSIONTYPE_CLIENT        (0x00000001)
#define CFS_SESSIONTYPE_SERVER        (0x00000002)

#define CFS_CLIENTSESSION_FMT_100     (0x00001064)
#define CFS_SERVERSESSION_FMT_100     (0x00002064)

// Operation codes

#define CFS_OPER_WAIT                 (0x00010000)
#define CFS_OPER_READ                 (0x01010000)
#define CFS_OPER_WRITE                (0x01020000)

// Diagnostic codes

#define CFS_DIAG_CONNCLOSE            (0x0000F001)
#define CFS_DIAG_WOULDBLOCK           (0x0000F002)
#define CFS_DIAG_READNOBLOCK          (0x0000F003)
#define CFS_DIAG_WRITENOBLOCK         (0x0000F004)
#define CFS_DIAG_TIMEDOUT             (0x0000F005)
#define CFS_DIAG_ALLDATA              (0x0000F006)
#define CFS_DIAG_PARTIALDATA          (0x0000F007)
#define CFS_DIAG_NODATA               (0x0000F008)
#define CFS_DIAG_INVALIDSIZE          (0x0000F009)
#define CFS_DIAG_ENVOPEN              (0x0000F00A)
#define CFS_DIAG_APPID                (0x0000F00B)
#define CFS_DIAG_SESSIONTYPE          (0x0000F00C)
#define CFS_DIAG_ENVINIT              (0x0000F00D)
#define CFS_DIAG_SOCOPEN              (0x0000F00E)
#define CFS_DIAG_SETFD                (0x0000F00F)
#define CFS_DIAG_SOCINIT              (0x0000F010)
#define CFS_DIAG_SYSTEM               (0x0000FFFE)
#define CFS_DIAG_UNKNOWN              (0x0000FFFF)

#define CFS_OFFSET_BUFFERS_MAXITEMS                           (20)
#define CFS_OFFSET_NUMERIC_MAXITEMS                           (12)

#define CFS_OFFSET_GSK_KEYRING_FILE                           (0)
#define CFS_OFFSET_GSK_KEYRING_PW                             (1)
#define CFS_OFFSET_GSK_KEYRING_LABEL                          (2)
#define CFS_OFFSET_GSK_OS400_APPLICATION_ID                   (3)
#define CFS_OFFSET_GSK_V2_CIPHER_SPECS                        (4)
#define CFS_OFFSET_GSK_V3_CIPHER_SPECS                        (5)
#define CFS_OFFSET_GSK_V3_CIPHER_SPECS_EX                     (6)
#define CFS_OFFSET_GSK_TLSV12_CIPHER_SPECS                    (7)
#define CFS_OFFSET_GSK_TLSV12_CIPHER_SPECS_EX                 (8)
#define CFS_OFFSET_GSK_TLSV11_CIPHER_SPECS                    (9)
#define CFS_OFFSET_GSK_TLSV11_CIPHER_SPECS_EX                 (10)
#define CFS_OFFSET_GSK_TLSV10_CIPHER_SPECS                    (11)
#define CFS_OFFSET_GSK_TLSV10_CIPHER_SPECS_EX                 (12)
#define CFS_OFFSET_GSK_SSL_EXTN_SIGALG                        (13)
#define CFS_OFFSET_GSK_OCSP_URL                               (14)
#define CFS_OFFSET_GSK_OCSP_PROXY_SERVER_NAME                 (15)
#define CFS_OFFSET_GSK_SSL_EXTN_SERVERNAME_REQUEST            (16)
#define CFS_OFFSET_GSK_SSL_EXTN_SERVERNAME_CRITICAL_REQUEST   (17)
#define CFS_OFFSET_GSK_SSL_EXTN_SERVERNAME_LIST               (18)
#define CFS_OFFSET_GSK_SSL_EXTN_SERVERNAME_CRITICAL_LIST      (19)

#define CFS_OFFSET_GSK_GSK_FD                                 (0)
#define CFS_OFFSET_GSK_V2_SESSION_TIMEOUT                     (1)
#define CFS_OFFSET_GSK_V3_SESSION_TIMEOUT                     (2)
#define CFS_OFFSET_GSK_OS400_READ_TIMEOUT                     (3)
#define CFS_OFFSET_GSK_HANDSHAKE_TIMEOUT                      (4)
#define CFS_OFFSET_GSK_OCSP_MAX_RESPONSE_SIZE                 (5)
#define CFS_OFFSET_GSK_OCSP_TIMEOUT                           (6)
#define CFS_OFFSET_GSK_OCSP_NONCE_SIZE                        (7)
#define CFS_OFFSET_GSK_OCSP_CLIENT_CACHE_SIZE                 (8)
#define CFS_OFFSET_GSK_OCSP_PROXY_SERVER_PORT                 (9)
#define CFS_OFFSET_GSK_SSL_EXTN_MAXFRAGMENT_SIZE              (10)
#define CFS_OFFSET_GSK_TLS_CBCPROTECTION_METHOD               (11)

enum CFS_security {

   CFS_security_none,
   CFS_security_default,
   CFS_security_ssl
};

typedef struct tagJOBINFOSTRUCT {

  int  bytesReturned;
  int  bytesAvailable;
  char JobName[10];
  char JobUser[10];
  char JobNumber[6];
  char JobInternalID[16];
  char JobStatus[10];
  char JobType;
  char JobSubType;
  char reserved[2];
  int  Priority;
  int  Timeslice;
  int  DefaultWait;
  char Purge[10];

}JOBINFOSTRUCT;

// --------------------------------------------------------------
// Session instance
// --------------------------------------------------------------

//typedef struct tagCFS_INSTANCE {
//
//  int32_t size;
//  int connfd;
//  gsk_handle ssl_henv;
//  gsk_handle ssl_hsession;
//  char* szAppID;
//
//} CFS_INSTANCE;

typedef void* CFS_INSTANCE;

typedef struct tagCFS_CLIENTSESSION_100 {

  char* szHostName;
  char* szApplicationID;
  int port;

  int connTimeout;

  GSK_ENUM_ID serverAuthType;

  ///////////////////////////////////////////////////////////////////////////
  // Set the following flags to non-zero to specify non default
  // settings. For instance, if the gskProtocolOverrideDefaults field
  // is set to 1, then the implementation will check the values of
  // the gskProtocol_* fields and set or clear the corresponding
  // GSK settings. If the gskProtocolOverrideDefaults field is set to zero,
  // then those fields are ignored and the default GSK settings are
  // assumed.
  ///////////////////////////////////////////////////////////////////////////

  int gskProtocolOverrideDefaults;
  int gskSessionTypeOverrideDefaults;
  int gskAuthTypeOverrideDefaults;
  int gskSessionCloseOverrideDefaults;
  int gskOverrideNumericDefaults;

  int gskProtocol_GSK_PROTOCOL_TLSV12;
  int gskProtocol_GSK_PROTOCOL_TLSV11;
  int gskProtocol_GSK_PROTOCOL_TLSV10;
  int gskProtocol_GSK_PROTOCOL_TLSV1;
  int gskProtocol_GSK_PROTOCOL_SSLV3;
  int gskProtocol_GSK_PROTOCOL_SSLV2;

  int gskSession_GSK_ALLOW_UNAUTHENTICATED_RESUME;

  int gskAttribute_GSK_OCSP_ENABLE;
  int gskAttribute_GSK_OCSP_NONCE_GENERATION_ENABLE;
  int gskAttribute_GSK_OCSP_NONCE_CHECK_ENABLE;
  int gskAttribute_GSK_OCSP_RETRIEVE_VIA_GET;
  int gskAttribute_GSK_EXTENDED_RENEGOTIATION_CRITICAL_CLIENT;
  int gskAttribute_GSK_CERTREQ_DNLIST_ENABLE;

  int gskSessionClose_GSK_DELAYED_ENVIRONMENT_CLOSE;
  int gskSessionClose_GSK_NORMAL_ENVIRONMENT_CLOSE;

  int gskAuthType_GSK_SERVER_AUTH_FULL;
  int gskAuthType_GSK_SERVER_AUTH_PASSTHRU;

  int iNumericValues[CFS_OFFSET_NUMERIC_MAXITEMS];

  char* pszBuffers[CFS_OFFSET_BUFFERS_MAXITEMS];

} CFS_CLIENTSESSION_100;


typedef struct tagCFS_SERVERSESSION_100 {

  char* szApplicationID;

  ///////////////////////////////////////////////////////////////////////////
  // Set the following flags to non-zero to specify non default
  // settings. For instance, if the gskProtocolOverrideDefaults field
  // is set to 1, then the implementation will check the values of
  // the gskProtocol_* fields and set or clear the corresponding
  // GSK settings. If the gskProtocolOverrideDefaults field is set to zero,
  // then those fields are ignored and the default GSK settings are
  // assumed.
  ///////////////////////////////////////////////////////////////////////////

  int gskProtocolOverrideDefaults;
  int gskSessionTypeOverrideDefaults;
  int gskAuthTypeOverrideDefaults;
  int gskSessionCloseOverrideDefaults;
  int gskOverrideNumericDefaults;

  int gskProtocol_GSK_PROTOCOL_TLSV12;
  int gskProtocol_GSK_PROTOCOL_TLSV11;
  int gskProtocol_GSK_PROTOCOL_TLSV10;
  int gskProtocol_GSK_PROTOCOL_TLSV1;
  int gskProtocol_GSK_PROTOCOL_SSLV3;
  int gskProtocol_GSK_PROTOCOL_SSLV2;

  int gskSessionType_GSK_SERVER_SESSION;
  int gskSessionType_GSK_SERVER_SESSION_WITH_CL_AUTH;
  int gskSessionType_GSK_SERVER_SESSION_WITH_CL_AUTH_CRITICAL;

  int gskSession_GSK_ALLOW_UNAUTHENTICATED_RESUME;

  int gskAttribute_GSK_OCSP_ENABLE;
  int gskAttribute_GSK_OCSP_NONCE_GENERATION_ENABLE;
  int gskAttribute_GSK_OCSP_NONCE_CHECK_ENABLE;
  int gskAttribute_GSK_OCSP_RETRIEVE_VIA_GET;
  int gskAttribute_GSK_EXTENDED_RENEGOTIATION_CRITICAL_SERVER;
  int gskAttribute_GSK_CERTREQ_DNLIST_ENABLE;

  int gskSessionClose_GSK_DELAYED_ENVIRONMENT_CLOSE;
  int gskSessionClose_GSK_NORMAL_ENVIRONMENT_CLOSE;

  int gskAuthType_GSK_CLIENT_AUTH_FULL;
  int gskAuthType_GSK_CLIENT_AUTH_PASSTHRU;
  int gskAuthType_GSK_OS400_CLIENT_AUTH_REQUIRED;

  int iNumericValues[CFS_OFFSET_NUMERIC_MAXITEMS];

  char* pszBuffers[CFS_OFFSET_BUFFERS_MAXITEMS];

} CFS_SERVERSESSION_100;

// ---------------------------------------------------------------------------
// Handler function prototype
// ---------------------------------------------------------------------------
//
// Parameters are:
//
//   Connection socket
//   client IP address string (IPV6 format)
//   client port (as a null-terminated string)
//   additional data (if required; ths is implementation specific)
//
// ---------------------------------------------------------------------------

typedef CSRESULT (*CFS_PROTOCOLHANDLERPROC)(int, char*, char*, void*);

// ---------------------------------------------------------------------------
// Prototypes
// ---------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
//
// CFS_Close
//
// This function closes a non-secure session and environement.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CFS_INSTANCE. Use
//       the CFS_Open() function to initialise this instance.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//////////////////////////////////////////////////////////////////////////////


CSRESULT CFS_Close(CFS_INSTANCE* This);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_Connect
//
// This function establishes a client session with a remote host.
//
// communication with a peer.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// sessionInfo: A pointer to a data structure containing additional
//              information: for this version, a structure of type
//              CFS_CLIENTSESSION_100 must be filled. The following
//              mandatory fields must be provided:
//
//              szHostName: a null-terminated string identifying the
//                          target host to connect to.
//
//              port: the port number; dont convert this number in
//                    network byte order; the function will make the
//                    necessary conversion.
//
//              All other fields are ignored.
//
// sessionInfoFmt: An integer identifying the format of the sessionInfo
//                 parameter: for this version, the parameter is ignored.
//                 (set this parameter to zero).
//
// Return values:
// ---------------------------------------------------------------------------
//
//    A pointer to an CFS_INSTANCE instance; if the pointer is NULL,
//    this indicates failure.
//
//////////////////////////////////////////////////////////////////////////////


CFS_INSTANCE*
  CFS_Connect
    (void*  sessionInfo,
     int    sessionInfoFmt);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_MakeUUID
//
// This function generates a UUID and returns its string representation.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// szUUID: The address of a buffer that will hold the resulting UUID.
//         On output, the buffer will be null-terminated. The size
//         of the buffer can be set to CFS_UUID_BUFFERSIZE to
//         insure enough space for the maximum UUID representation.
//
//
// mode: Indicates how to represent the generated UUID. Hexadicimal digits
//       above 9 can be represented as either lowercase or uppercase
//       letters. Also, the string represntation may include (or not)
//       dashes seperating the UUID fields:
//
//       CFS_UUID_UPPERCASE: returns UUID with upercase letters and no dash
//
//          Example: D4BA3B92BD652AEC3B21107CD5F489D6
//
//       CFS_UUID_LOWERCASE: returns UUID with lowercase letters and no das
//
//          Example: d4ba3b92bd652aec3b21107cd5f489d6
//
//       CFS_UUID_UPPERCASE | CFS_UUID_DASHES : returns UUID with
//                                              upercase letters and dashes.
//
//          Example: D4BA3B92-BD65-2AEC-3B21-107CD5F489D6
//
//       CFS_UUID_LOWERCASE | CFS_UUID_DASHES : returns UUID with
//                                              lowercase letters and dashes.
//
//          Example: d4ba3b92-bd65-2aec-3b21-107cd5f489d6
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//////////////////////////////////////////////////////////////////////////////


CSRESULT
  CFS_MakeUUID
    (char* szUUID,
     int mode);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_NetworkToPresentation
//
// This function returns a string representation of a network address.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// sa: A pointer to a socket address.
//
// addrstr: A buffer that will receive the string representation of
//          the network address.
//
// portstr: A buffer that will receive the port number of the network
//          address.
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//    CS_FAILURE
//
//////////////////////////////////////////////////////////////////////////////


CSRESULT
  CFS_NetworkToPresentation
    (const struct sockaddr* sa,
     char* addrstr,
     char* portstr);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_OpenChannel
//
// This function initialises a non secure session. It also
// initialises an CFS_INSTANCE structure that must be used for
// communication with a peer.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an instance of type CFS_INSTANCE.
//
// szApplicationID: The name of the application ID: for this version,
//                  the parameter is ignored.
//
// sessionInfo: A pointer to a data structure containing additional
//              information: for this version, this parameter is
//              ignored and should be set to NULL.
//
// sessionInfoFmt: An integer identifying the format of the sessionInfo
//                 parameter: for this version, the parameter is ignored
//                 and should be set to zero.
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    A pointer to an CFS_INSTANCE instance; if the pointer is NULL,
//    this indicates failure.
//
//////////////////////////////////////////////////////////////////////////////


CFS_INSTANCE*
  CFS_OpenChannel
    (int    connfd,
     void*  sessionInfo,
     int    sessionInfoFmt);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_ReadRecord
//
// This function reads a specific number of bytes from a non secure socket.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CFS_INSTANCE. Use
//       the CFS_Open() function to initialise this instance.
//
// buffer: The address of a buffer that holds the data
//         read from the socket.
//
// size: The number of bytes to that must be read in the buffer.
//       Buffer size must be > 0.
//
// timeout: The maximum number of seconds to wait for data
//          to arrive.
//
//            <  0 : wait forever for data to arrive.
//
//            == 0 : perform a single read and return immediately.
//
//            >  0 : wait up to the specified timeout for data to arrive.
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    This function only succeeds when the entire buffer has been filled.
//    If we timed-out or blocked, this could mean the peer is sending data
//    too slowly and this could be managed by the caller by trying to
//    read again the remaining data.
//
//    CS_SUCCESS
//
//        All the requested bytes have been read.
//
//    CS_FAILURE | CFS_OPER_READ | CFS_DIAG_CONNCLOSE
//
//        The connection was closed while reading to the socket.
//
//    CS_FAILURE | CFS_OPER_READ | CFS_DIAG_SYSTEM
//
//        An error occured on the read operation.
//        Caller can check errno for more info.
//
//    CS_FAILURE | CFS_OPER_READ | CFS_DIAG_WOULDBLOCK
//
//        A zero timeout was specified and the read would
//        have blocked.
//
//    CS_FAILURE | CFS_OPER_WAIT | CFS_DIAG_TIMEDOUT
//
//        The timeout expired while waiting to read data. This
//        can only be returned if the timeout is non-zero.
//
//    CS_FAILURE | CFS_OPER_WAIT | CFS_DIAG_SYSTEM
//
//        An error occured while waiting to read data.
//        Caller can check errno for more info.
//
//
//    Note that when a failure code is returned, some data may have been
//    read. The "size" parameter will hold the number of bytes read.
//    A caller could manage partial reads if necessary.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_Read
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout);

CSRESULT
  CFS_ReadRecord
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_ReceiveDescriptor
//
// This function receives a file (socket) descriptor from another process.
// It is assumed that the caller has already established a connection to
// the other process via a local domain (UNIX) socket.
//
// Parameters
// ---------------------------------------------------------------------------
//
// fd: The local socket descriptor used to receive the descriptor.
//
// descriptor: The address of an integer that will receive the descriptor.
//
// timeout: The maximum number of seconds to wait for the descriptor:
//
//            <  0 : wait forever until data arrives.
//
//            == 0 : try to receive immediately and return.
//
//            >  0 : wait up to the specified timeout for data to arrive.
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//    CS_FAILURE | CFS_OPER_WAIT  | CFS_DIAG_TIMEDOUT
//    CS_FAILURE | CFS_OPER_WAIT  | CFS_DIAG_SYSTEM
//    CS_FAILURE | CFS_OPER_READ  | CFS_DIAG_SYSTEM
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_ReceiveDescriptor
    (int fd,
     int* descriptor,
     int timeout);

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureClose
//
// This function closes the secure session and environement.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CFS_INSTANCE. Use
//       the CFS_SecureOpen() function to initialise this instance.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//////////////////////////////////////////////////////////////////////////////


CSRESULT
  CFS_SecureClose
    (CFS_INSTANCE* This);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureConnect
//
// This function establishes a client session with a remote host.
//
// communication with a peer.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// sessionInfo: A pointer to a data structure containing additional
//              information: for this version, a structure of type
//              CFS_CLIENTSESSION_100 must be filled. The following
//              mandatory fields must be provided:
//
//              szHostName: a null-terminated string identifying the
//                          target host to connect to.
//
//              port: the port number; dont convert this number in
//                    network byte order; the function will make the
//                    necessary conversion.
//
//              All other fields are ignored.
//
// sessionInfoFmt: An integer identifying the format of the sessionInfo
//                 parameter: for this version, the parameter is ignored.
//                 (set this parameter to zero).
//
// Return values:
// ---------------------------------------------------------------------------
//
//    A pointer to an CFS_INSTANCE instance; if the pointer is NULL,
//    this indicates failure.
//
//////////////////////////////////////////////////////////////////////////////


CFS_INSTANCE*
  CFS_SecureConnect
    (void*  sessionInfo,
     int    sessionInfoFmt,
     int*   iSSLResult);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureOpenChannel
//
// This function initialises a secure communication environement.
// It also creates and
// initialises an CFS_INSTANCE structure that must be used for secure
// communication with a peer in all subsequent calls to the CFS_Secure*
// functions.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an instance of type CFS_INSTANCE.
//
// sessionInfo: A pointer to a data structure containing additional
//              information: The minimal information
//              to provide is defined in the CFS_CLIENTSESSION_100
//              structure. The following fields are required:
//                                                                  .
//              szApplicationID: Pointer to null-terminated string
//                               identifying the SSL certificate.
//
// sessionInfoFmt: An integer identifying the format of the sessionInfo
//                 parameter: For this version, only the value
//                 CFS_SERVERSESSION_FMT_100 is supported.
//
//
// iSSLResult: The address of a variable that will receive on return the
//             GSK return code from the last call to one of the
//             GSK Toolkit API functions used by this function.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    A pointer to an CFS_INSTANCE instance; if the pointer is NULL,
//    this indicates failure. The iSSLResult parameter will hold
//    the GSK API call error.
//
//////////////////////////////////////////////////////////////////////////////


CFS_INSTANCE*
  CFS_SecureOpenChannel
    (int    connfd,
     void*  sessionInfo,
     int    sessionInfoFmt,
     int*   iSSLResult);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureReadRecord
//
// This function reads a specific number of bytes on a secure socket.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CFS_INSTANCE. Use
//       the CFS_SecureOpen() function to initialise this instance.
//
// buffer: The address of a buffer that will receive the data
//         from the socket.
//
// size: The maximum number of bytes the buffer can receive.
//
// timeout: The maximum number of seconds to wait for data.
//          Timeout values can be:
//
//            <  0 : wait forever until data arrives.
//
//            == 0 : perform a single read and return immediately.
//
//            >  0 : wait up to the specified timeout for data to arrive.
//
// iSSLResult: The address of a variable that will receive on return the
//             GSK return code from the last call to the
//             gsk_secure_soc_read API function.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    This function only succeeds when the entire buffer has been filled.
//    If we timed-out or blocked, this could mean the peer sending data too
//    slowly and this could be managed by the caller by trying to
//    read again the remaining data.
//
//    CS_SUCCESS
//
//        All the requested bytes have been read.
//
//    CS_FAILURE | CFS_OPER_READ   | CFS_DIAG_CONNCLOSE
//
//        The connection was closed while reading the socket.
//
//    CS_FAILURE | CFS_OPER_READ   | CFS_DIAG_SYSTEM
//
//        An error occured on the read operation. Caller
//        can check iSSLResult and errno for more info.
//
//    CS_FAILURE | CFS_OPER_READ   | CFS_DIAG_WOULDBLOCK
//
//        The specified timeout is zero and the read would have blocked.
//
//    CS_FAILURE | CFS_OPER_WAIT   | CFS_DIAG_SYSTEM
//
//        An error occured on while waiting for data. Caller
//        can check iSSLResult and errno for more info.
//
//    CS_FAILURE | CFS_OPER_WAIT   | CFS_DIAG_TIMEDOUT
//
//        The timeout expired while waiting for data.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_SecureRead
    (CFS_INSTANCE* This,
     char*         buffer,
     uint64_t*     size,
     int           timeout,
     int*          iSSLResult);

CSRESULT
  CFS_SecureReadRecord
    (CFS_INSTANCE* This,
     char*         buffer,
     uint64_t*     size,
     int           timeout,
     int*          iSSLResult);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureWriteRecord
//
// This function writes a specific number of bytes to a secure socket.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CFS_INSTANCE. Use
//       the CFS_SecureOpen() function to initialise this instance.
//
// buffer: The address of a buffer that holds the data
//         to send on the socket.
//
// size: The number of bytes to send from the buffer.
//       Buffer size must be > 0.
//
// timeout: The maximum number of seconds to wait for data
//          to be sent (does not mean the peer has received it).
//
//            <  0 : wait forever until data can be sent.
//
//            == 0 : perform a single write and return immediately.
//
//            >  0 : wait up to the specified timeout until data can be sent.
//
// iSSLResult: The address of a variable that will receive on return the
//             GSK return code from the last call to the
//             gsk_secure_soc_write API function.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    This function only succeeds when the entire buffer has been written.
//    If we timed-out or blocked, this could mean we are sending too
//    quickly and this could be managed by the caller by trying to
//    write again the remaining data.
//
//    CS_SUCCESS
//
//        All the requested bytes have been written.
//
//    CS_FAILURE | CFS_OPER_WAIT   | CFS_DIAG_SYSTEM
//
//        An error occured on while waiting to write data.
//
//    CS_FAILURE | CFS_OPER_WAIT   | CFS_DIAG_TIMEDOUT
//
//        The timeout expired while waiting to write data.
//
//    CS_FAILURE | CFS_OPER_WRITE   | CFS_DIAG_CONNCLOSE
//
//        The connection was closed while writing to the socket.
//
//    CS_FAILURE | CFS_OPER_WRITE   | CFS_DIAG_SYSTEM
//
//        An error occured on the write operation. Caller
//        can check iSSLResult and errno for more info.
//
//    CS_FAILURE | CFS_OPER_WRITE   | CFS_DIAG_WOULDBLOCK
//
//        A zero timeout was specified and the write
//        operation would block. Some data may have been written.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_SecureWrite
    (CFS_INSTANCE* This,
     char*         buffer,
     uint64_t*     size,
     int           timeout,
     int*          iSSLResult);

CSRESULT
  CFS_SecureWriteRecord
    (CFS_INSTANCE* This,
     char*         buffer,
     uint64_t*     size,
     int           timeout,
     int*          iSSLResult);

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SendDescriptor
//
// This function sends a file (socket) descriptor to another process.
// The caller has already established a connection to the other process
// via a local domain (UNIX) socket.
//
// Parameters
// ---------------------------------------------------------------------------
//
// fd: The local socket descriptor used to send the descriptor.
//
// descriptor: The descriptor to send over to the other process.
//
// size: The number of bytes to send from the buffer.
//       Buffer size must be > 0.
//
// timeout: The maximum number of seconds to wait for the descriptor:
//       Buffer size must be > 0.
//
// timeout: The maximum number of seconds to wait for the descriptor:
//
//            <  0 : wait forever.
//
//            == 0 : try to send immediately and return.
//
//            >  0 : wait up to the specified timeout.
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//    CS_FAILURE | CFS_OPER_WAIT  | CFS_DIAG_TIMEDOUT
//    CS_FAILURE | CFS_OPER_WAIT  | CFS_DIAG_SYSTEM
//    CS_FAILURE | CFS_OPER_WRITE | CFS_DIAG_SYSTEM
//
//////////////////////////////////////////////////////////////////////////////


CSRESULT
  CFS_SendDescriptor
    (int fd,
     int descriptor,
     int timeout);

//////////////////////////////////////////////////////////////////////////////
//
// CFS_WriteRecord
//
// This function writes a specific number of bytes to a non secure socket.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: A pointer to an initialised instance of type CFS_INSTANCE. Use
//       the CFS_Open() function to initialise this instance.
//
// buffer: The address of a buffer that holds the data.
//         to send on the socket.
//
// size: The number of bytes to send from the buffer.
//       Buffer size must be > 0.
//
// timeout: The maximum number of seconds to wait for data
//          to be sent (does not mean the peer has received it).
//
//            <  0 : wait forever until data can be sent.
//
//            == 0 : perform a single write and return immediately.
//
//            >  0 : wait up to the specified timeout until data can be sent.
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    This function only succeeds when the entire buffer has been written.
//    If we timed-out or blocked, this could mean we are sending too
//    quickly and this could be managed by the caller by trying to
//    write again the remaining data.
//
//    CS_SUCCESS
//
//        All the requested bytes have been written.
//
//    CS_FAILURE | CFS_OPER_WAIT   | CFS_DIAG_SYSTEM
//
//        An error occured on while waiting to write data.
//
//    CS_FAILURE | CFS_OPER_WAIT   | CFS_DIAG_TIMEDOUT
//
//        The timeout expired while waiting to write data.
//
//    CS_FAILURE | CFS_OPER_WRITE   | CFS_DIAG_CONNCLOSE
//
//        The connection was closed while writing to the socket.
//
//    CS_FAILURE | CFS_OPER_WRITE   | CFS_DIAG_SYSTEM
//
//        An error occured on the write operation. Caller
//        can check errno for more info.
//
//    CS_FAILURE | CFS_OPER_WRITE   | CFS_DIAG_WOULDBLOCK
//
//        A zero timeout was specified and the write
//        operation would block. Some data may have been written.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_Write
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout);

CSRESULT
  CFS_WriteRecord
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_ValidateUserProfile
//
// This function determines if the user profile exits and if it is enabled.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// szUserProfileName: a null-terminated string that holds the user profile
//                    name to validate. The buffer must be right-padded
//                    with blanks up to 10 positions.
//
//                Example: user name is JOHN, then szUserProfileName must
//                         be set to "JOHN      "  (plus a NULL)
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//        User profile exists and is enbled
//
//    CS_FAILURE | CFS_NOTFOUND
//
//        User profile is not found
//
//    CS_FAILURE | CFS_DISABLED
//
//        User profile is disabled
//
//    CS_FAILURE | CFS_DIAG_UNKNOWN
//
//        Some system error occurred
//
//////////////////////////////////////////////////////////////////////////////


CSRESULT
  CFS_ValidateUserProfile
    (char* szUserProfileName);


//////////////////////////////////////////////////////////////////////////////
//
// CFS_GetUserProfile
//
// This function returns the user profile information in USERI0300 format.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// szUserProfileName: a null-terminated string that holds the user profile
//                    name to validate. The buffer must be right-padded
//                    with blanks up to 10 positions.
//
//                    Example: user name is JOHN, then szUserProfileName must
//                             be set to "JOHN      "  (plus a NULL). To get
//                             the current user profile, set this parameter
//                             to "*CURRENT  "
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//        User profile exists and information is returned; the actual
//        user profile name will be copied into the szUserProfileName
//        parameter.
//
//    CS_FAILURE | CFS_NOTFOUND
//
//        User profile is not found
//
//    CS_FAILURE | CFS_DIAG_UNKNOWN
//
//        Some system error occurred
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_GetUserProfile
    (char* szUserProfileName,
     Qsy_USRI0300_T* pUserProfile);

//////////////////////////////////////////////////////////////////////////////
//
// CFS_GetCurJobName
//
// This function returns the qualified current job name.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// szJobName: a buffer that will hold the job name on return. The buffer
//            is formatted as follow:
//
//            Positions  Value
//            0-9        Job name
//            10-19      User name
//            20-25      Job number
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CFS_GetCurJobName
  (char szJobName[27]);


#endif

