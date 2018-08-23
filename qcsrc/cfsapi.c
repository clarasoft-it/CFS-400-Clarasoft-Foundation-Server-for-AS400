/* ===========================================================================
  Clarasoft Foundation Server 400

  cfsapi.c
  Networking Primitives
  Version 1.0.0

  Compile module with:
     CRTCMOD MODULE(CFSAPI) SRCFILE(QCSRC) DBGVIEW(*ALL)

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

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <gskssl.h>
#include <limits.h>
#include <netdb.h>
#include <QSYSINC/MIH/CVTHC>
#include <QSYSINC/MIH/GENUUID>
#include <QUSRJOBI.h>
#include <QSYRUSRI.h>
#include <QUSEC.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/un.h>

#include "qcsrc/cscore.h"

#define CFS_NTOP_ADDR_MAX             (1025)
#define CFS_NTOP_PORT_MAX             (9)

#define CFS_SSL_MAXRECORDSIZE         (16383UL)

#define CFS_UUID_BUFFERSIZE           (37)
#define CFS_UUID_UPPERCASE            (0x00000000)
#define CFS_UUID_LOWERCASE            (0x00000001)
#define CFS_UUID_DASHES               (0x00000002)

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

#define CFS_SEC_SERVER_SESSION                          (1)
#define CFS_SEC_SERVER_SESSION_CLIENT_AUTH              (2)
#define CFS_SEC_SERVER_SESSION_CLIENT_AUTH_CRITICAL     (3)

#define CFS_SEC_CLIENT_SESSION_SERVER_AUTH_FULL         (1)
#define CFS_SEC_CLIENT_SESSION_SERVER_AUTH_PASSTHROUGH  (2)

typedef struct tagCFS_INSTANCE {

  int32_t size;
  int connfd;
  gsk_handle ssl_henv;
  gsk_handle ssl_hsession;
  char* szApplicationID;

} CFS_INSTANCE;

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

typedef struct tagCFS_CLIENTSESSION_100 {

  char* szHostName;
  char* szApplicationID;
  int port;

  int connTimeout;

  // Indicates what to do if server certtificate fails authentication

  // CFS_SEC_CLIENT_SESSION_SERVER_AUTH_FULL
  // CFS_SEC_CLIENT_SESSION_SERVER_AUTH_PASSTHROUGH

  GSK_ENUM_ID secSessionType;

} CFS_CLIENTSESSION_100;

typedef struct tagCFS_SERVERSESSION_100 {

  char* szApplicationID;

  // The following indicates the type of authetication for client:
  //
  // CFS_SEC_SERVER_SESSION (no client authentication)
  // CFS_SEC_SERVER_SESSION_CLIENT_AUTH
  // CFS_SEC_SERVER_SESSION_CLIENT_AUTH_CRITICAL

  int secSessionType;

} CFS_SERVERSESSION_100;

// ---------------------------------------------------------------------------
// Prototypes
// ---------------------------------------------------------------------------

CSRESULT
  CFS_Close
    (CFS_INSTANCE* This);

CFS_INSTANCE*
  CFS_Connect
    (void* sessionInfo,
     int sessionInfoFmt);

CSRESULT
  CFS_MakeUUID
    (char* szUUID,
     int mode);

CFS_INSTANCE*
  CFS_OpenChannel
    (int connfd,
     void* sessionInfo,
     int sessionInfoFmt);

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

CSRESULT
  CFS_ReceiveDescriptor
    (int fd,
     int* descriptor,
     int timeout);

CSRESULT
  CFS_SecureClose
    (CFS_INSTANCE* This);

CFS_INSTANCE*
  CFS_SecureConnect
    (void*  sessionInfo,
     int sessionInfoFmt,
     int* iSSLResult);

CFS_INSTANCE*
  CFS_SecureOpenChannel
    (int connfd,
     void* sessionInfo,
     int sessionInfoFmt,
     int* iSSLResult);

CSRESULT
  CFS_SecureRead
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout,
     int* iSSLResult);

CSRESULT
  CFS_SecureReadRecord
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout,
     int* iSSLResult);

CSRESULT
  CFS_SecureWrite
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout,
     int* iSSLResult);

CSRESULT
  CFS_SecureWriteRecord
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout,
     int* iSSLResult);

CSRESULT
  CFS_SendDescriptor
    (int fd,
     int descriptor,
     int timeout);

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

CSRESULT CFS_GetCurJobName
  (char szJobName[27]);

/* ---------------------------------------------------------------------------
   private functions
--------------------------------------------------------------------------- */

CSRESULT
  CFS_PRV_DoSecureConnect_100
    (CFS_INSTANCE* cfsi,
     struct addrinfo* addrInfo,
     CFS_CLIENTSESSION_100* sessionInfo,
     int*  iSSLResult);

CSRESULT
  CFS_PRV_DoSecureOpenChannel_100
    (CFS_INSTANCE* cfsi,
     CFS_SERVERSESSION_100* sessionInfo,
     int*  iSSLResult);

CSRESULT
  CFS_PRV_NetworkToPresentation
    (const struct sockaddr* sa,
     char* addrstr,
     char* portstr);

CSRESULT
  CFS_PRV_SetBlocking
    (int connfd,
     int blocking);

//////////////////////////////////////////////////////////////////////////////
//
// CFS_Close
//
// This function closes a non-secure session and environement.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_Close
    (CFS_INSTANCE* This) {

   close(This->connfd);

   free(This);

   return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_Connect
//
// This function initialises a non secure session. It also
// initialises an CFS_INSTANCE structure that must be used for
// communication with a peer.
//
//////////////////////////////////////////////////////////////////////////////

CFS_INSTANCE*
  CFS_Connect
    (void* sessionInfo,
     int sessionInfoFmt) {

   int rc;
   int e;

   char szPort[11];

   CSRESULT hResult;

   CFS_INSTANCE* cfsi;

   struct addrinfo* addrInfo;
   struct addrinfo* addrInfo_first;
   struct addrinfo  hints;
   struct timeval tv;

   fd_set readSet, writeSet;

   cfsi = (CFS_INSTANCE*)malloc(sizeof(CFS_INSTANCE));
   cfsi->size = sizeof(CFS_INSTANCE);
   cfsi->connfd = -1;

   hResult = CS_FAILURE;

   switch(sessionInfoFmt) {

      case CFS_CLIENTSESSION_FMT_100:

         memset(&hints, 0, sizeof(struct addrinfo));
         hints.ai_family = AF_UNSPEC;
         hints.ai_socktype = SOCK_STREAM;

         sprintf(szPort, "%d",
                 ((CFS_CLIENTSESSION_100*)sessionInfo)->port);

         rc = getaddrinfo(((CFS_CLIENTSESSION_100*)sessionInfo)
                               ->szHostName,
                          szPort,
                          &hints,
                          &addrInfo);

         if (rc == 0)
         {

            addrInfo_first = addrInfo;
            cfsi->connfd = -1;

            while (addrInfo != 0)
            {
               cfsi->connfd = socket(addrInfo->ai_family,
                                     addrInfo->ai_socktype,
                                     addrInfo->ai_protocol);

               if (cfsi->connfd >= 0) {

                  // Set socket to non-blocking
                  CFS_PRV_SetBlocking(cfsi->connfd, 0);

                  rc = connect(cfsi->connfd,
                               addrInfo->ai_addr,
                               addrInfo->ai_addrlen);

                  if (rc < 0) {

                    if (errno != EINPROGRESS) {

                      e = errno;
                      close(cfsi->connfd);
                      cfsi->connfd = -1;
                    }
                    else {

                      FD_ZERO(&readSet);
                      FD_SET(cfsi->connfd, &readSet);

                      writeSet = readSet;
                      tv.tv_sec = ((CFS_CLIENTSESSION_100*)sessionInfo)
                                  ->connTimeout;
                      tv.tv_usec = 0;

                      rc = select(cfsi->connfd+1,
                                  &readSet,
                                  &writeSet,
                                  NULL,
                                  &tv);

                      if (rc <= 0) {

                          e = errno;
                          close(cfsi->connfd);
                          cfsi->connfd = -1;
                      }
                      else {

                        hResult = CS_SUCCESS;
                      }
                    }
                  }
                  else {

                    hResult = CS_SUCCESS;
                  }

                  break;
               }

               addrInfo = addrInfo->ai_next;
            }

            freeaddrinfo(addrInfo_first);
         }

         break;
   }

   if (CS_FAIL(hResult)) {
      free(cfsi);
      cfsi = 0;
   }

   return cfsi;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_MakeUUID
//
// This function generates a UUID and returns its string representation.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_MakeUUID
    (char* szUUID,
     int mode) {

  int size;
  int i;

  char szBuffer[CFS_UUID_BUFFERSIZE];

  _UUID_Template_T Template;


  memset(&Template, 0, sizeof(_UUID_Template_T));
  Template.bytesProv = sizeof(_UUID_Template_T);
  _GENUUID(&Template);

  size = 32;
  cvthc(szBuffer, Template.uuid, size);

  if (mode & CFS_UUID_LOWERCASE) {

    for(i=0; i<32; i++)
       szBuffer[i] = (char)tolower(szBuffer[i]);
  }

  if (mode & CFS_UUID_DASHES) {

    // Format with dashes

     szUUID[8]  = '-';
     szUUID[13] = '-';
     szUUID[18] = '-';
     szUUID[23] = '-';
     memcpy(szUUID + 0,  szBuffer + 0,  8);
     memcpy(szUUID + 9,  szBuffer + 8,  4);
     memcpy(szUUID + 14, szBuffer + 12, 4);
     memcpy(szUUID + 19, szBuffer + 16, 4);
     memcpy(szUUID + 24, szBuffer + 20, 12);

     szUUID[36] = 0;
  }
  else {

     memcpy(szUUID, szBuffer, 32);
     szUUID[32] = 0;
  }

  return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_OpenChannel
//
// This function initialises a non secure session. It also
// initialises an CFS_INSTANCE structure that must be used for
// communication with a peer.
//
//////////////////////////////////////////////////////////////////////////////

CFS_INSTANCE*
  CFS_OpenChannel
    (int connfd,
     void* sessionInfo,
     int sessionInfoFmt) {

   int rc;

   CFS_INSTANCE* cfsi;

   struct sockaddr_in6 server_addr;
   struct hostent *server;

   cfsi = (CFS_INSTANCE*)malloc(sizeof(CFS_INSTANCE));

   cfsi->size = sizeof(CFS_INSTANCE);

   cfsi->connfd = connfd;

   // Set socket to non-blocking mode
   CFS_PRV_SetBlocking(cfsi->connfd, 0);

   return cfsi;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_Read
//
// This function reads a non-secure socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_Read
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* maxSize,
     int timeout)
{

   int rc;
   int readSize;

   struct pollfd fdset[1];

   uint64_t initialSize;
   uint64_t leftToRead;
   uint64_t offset;

   initialSize = *maxSize;

   *maxSize    = 0;
   offset      = 0;

   leftToRead = initialSize;

   //////////////////////////////////////////////////////////////////////////
   // The total record size exceeds the maximum int value; we will
   // read up to int size at a time until we fill the buffer.
   //////////////////////////////////////////////////////////////////////////

   readSize =
     leftToRead > INT_MAX ?
                  INT_MAX :
                  leftToRead;

   //////////////////////////////////////////////////////////////////////////
   // We first try to read the socket
   //////////////////////////////////////////////////////////////////////////

   /////////////////////////////////////////////////////////
   // This branching label for restarting an interrupted
   // recv() call. An interrupted system call may result
   // from a signal and will have errno set to EINTR.
   // We must call recv() again.

   CFS_WAIT_RECV_1:

   //
   /////////////////////////////////////////////////////////

   rc = recv(This->connfd, buffer + offset, readSize, 0);

   if (rc < 0) {

      if (errno == EINTR) {

         ///////////////////////////////////////////////////
         // recv() was interupted by a signal
         // or the kernel could not allocate an
         // internal data structure. We will call
         // recv() again.
         ///////////////////////////////////////////////////

         goto CFS_WAIT_RECV_1;
      }
      else {

         if (errno == EWOULDBLOCK) {

            if (timeout != 0) {

               ////////////////////////////////////////////////////////////
               // This means we must wait up to a given timeout; this
               // is the only time we will be waitng as this means
               // we did not read any data and a positive timeout
               // means we give the peer some time to send over the data.
               ////////////////////////////////////////////////////////////


               ////////////////////////////////////////////////////////////
               // This branching label for restarting an interrupted
               // poll call. An interrupted system call may result from
               // a caught signal and will have errno set to EINTR. We
               // must call poll again.

               CFS_WAIT_POLL:

               //
               ////////////////////////////////////////////////////////////


               fdset[0].fd = This->connfd;
               fdset[0].events = POLLIN;

               rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

               if (rc == 1) {

                  if (!fdset[0].revents & POLLIN) {

                     /////////////////////////////////////////////////////////
                     // If we get anything other than POLLIN
                     // this means an error occurred.
                     /////////////////////////////////////////////////////////

                     return   CS_FAILURE
                            | CFS_OPER_WAIT
                            | CFS_DIAG_SYSTEM;
                  }
               }
               else {

                  if (rc == 0) {

                     return   CS_SUCCESS
                            | CFS_OPER_WAIT
                            | CFS_DIAG_TIMEDOUT;
                  }
                  else {

                     if (errno == EINTR) {

                        ///////////////////////////////////////////////////
                        // poll() was interupted by a signal
                        // or the kernel could not allocate an
                        // internal data structure. We will call
                        // poll() again.
                        ///////////////////////////////////////////////////

                        goto CFS_WAIT_POLL;
                     }
                     else {

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_SYSTEM;
                     }
                  }
               }
            }
            else {

               return   CS_FAILURE
                      | CFS_OPER_READ
                      | CFS_DIAG_WOULDBLOCK;
            }
         }
         else {

            return   CS_FAILURE
                   | CFS_OPER_READ
                   | CFS_DIAG_SYSTEM;
         }
      }
   }
   else {

      if (rc == 0) {

         /////////////////////////////////////////////////////////////////
         // This indicates a connection close; we are done.
         /////////////////////////////////////////////////////////////////

         return CS_SUCCESS | CFS_OPER_READ | CFS_DIAG_CONNCLOSE;
      }
      else {

         /////////////////////////////////////////////////////////////////
         // We read some data (maybe as much as required) ...
         // if not, we will continue reading for as long
         // as we don't block.
         /////////////////////////////////////////////////////////////////

         offset += rc;
         *maxSize += rc;
         leftToRead -= rc;

         readSize =
            leftToRead > INT_MAX ?
                         INT_MAX :
                         leftToRead;

         if (leftToRead == 0) {

            // We have read as much as was required
            return CS_SUCCESS | CFS_OPER_READ | CFS_DIAG_ALLDATA;
         }
      }
   }

   ////////////////////////////////////////////////////////////////
   // If we get here, then we have read something but not up
   // to the maximum buffer size and more can possibly be read.
   ////////////////////////////////////////////////////////////////

   do {

      /////////////////////////////////////////////////////////
      // This branching label for restarting an interrupted
      // recv() call. An interrupted system call may result
      // from a signal and will have errno set to EINTR.
      // We must call recv() again.

      CFS_WAIT_READ_2:

      //
      /////////////////////////////////////////////////////////

      rc = recv(This->connfd, buffer + offset, readSize, 0);

      if (rc < 0) {

         if (errno == EINTR) {

            ///////////////////////////////////////////////////
            // recv() was interupted by a signal
            // or the kernel could not allocate an
            // internal data structure. We will call
            // recv() again.
            ///////////////////////////////////////////////////

            goto CFS_WAIT_READ_2;
         }
         else {

            if (errno == EWOULDBLOCK) {

               ///////////////////////////////////////////////////
               // Once in this loop, we read until we block;
               // the timeout no longer applies (it applied
               // only for the first read).
               ///////////////////////////////////////////////////

               return   CS_SUCCESS
                      | CFS_OPER_READ
                      | CFS_DIAG_WOULDBLOCK;
            }
            else {

               return   CS_FAILURE
                      | CFS_OPER_READ
                      | CFS_DIAG_SYSTEM;
            }
         }
      }
      else {

         if (rc == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done.
            /////////////////////////////////////////////////////////////////

            return CS_SUCCESS | CFS_OPER_READ | CFS_DIAG_CONNCLOSE;
         }
         else {

            offset += rc;
            *maxSize += rc;
            leftToRead -= rc;

            readSize =
               leftToRead > INT_MAX ?
                            INT_MAX :
                            leftToRead;
         }
      }
   }
   while (leftToRead > 0);

   return CS_SUCCESS | CFS_OPER_READ | CFS_DIAG_ALLDATA;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_ReadRecord
//
// This function reads a specific number of bytes from a non secure socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_ReadRecord
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout) {

   int rc;
   int readSize;

   struct pollfd fdset[1];

   uint64_t initialSize;
   uint64_t leftToRead;
   uint64_t offset;

   initialSize = *size;
   *size       = 0;
   offset      = 0;

   leftToRead = initialSize;

   //////////////////////////////////////////////////////////////////////////
   // The total record size exceeds the maximum int value; we will
   // read up to int size at a time until we fill the buffer.
   //////////////////////////////////////////////////////////////////////////

   readSize =
     leftToRead > INT_MAX ?
                  INT_MAX :
                  leftToRead;

   do {

      /////////////////////////////////////////////////////////
      // This branching label for restarting an interrupted
      // recv() call. An interrupted system call may result
      // from a signal and will have errno set to EINTR.
      // We must call recv() again.

      CFS_WAIT_READ:

      //
      /////////////////////////////////////////////////////////

      rc = recv(This->connfd, buffer + offset, readSize, 0);

      if (rc < 0) {

         if (errno == EINTR) {

            ///////////////////////////////////////////////////
            // recv() was interupted by a signal
            // or the kernel could not allocate an
            // internal data structure. We will call
            // recv() again.
            ///////////////////////////////////////////////////

            goto CFS_WAIT_READ;
         }
         else {

            if (errno == EWOULDBLOCK) {

               if (timeout != 0) {

                  ////////////////////////////////////////////////////////////
                  // This means we must wait up to a given timeout.
                  // This may occur several times in this loop so that
                  // the actual time it takes to read an entire buffer
                  // can exceed the timeout. The caller should not
                  // depend on a precise execution time for this function
                  // based on the timeout value.
                  ////////////////////////////////////////////////////////////


                  ////////////////////////////////////////////////////////////
                  // This branching label for restarting an interrupted
                  // poll call. An interrupted system call may result from
                  // a caught signal and will have errno set to EINTR. We
                  // must call poll again.

                  CFS_WAIT_POLL:

                  //
                  ////////////////////////////////////////////////////////////


                  fdset[0].fd = This->connfd;
                  fdset[0].events = POLLIN;

                  rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

                  if (rc == 1) {

                     if (!fdset[0].revents & POLLIN) {

                        //////////////////////////////////////////////////////
                        // If we get anything other than POLLIN,
                        // the revents could have other bits set
                        // such as POLLHUP, indicating the peer
                        // closed its end. We ignore this
                        // since connection closures are handled
                        // when the socket is read.
                        //////////////////////////////////////////////////////

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_SYSTEM;
                     }
                  }
                  else {

                     if (rc == 0) {

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_TIMEDOUT;
                     }
                     else {

                        if (errno == EINTR) {

                           ///////////////////////////////////////////////////
                           // poll() was interupted by a signal
                           // or the kernel could not allocate an
                           // internal data structure. We will call
                           // poll() again.
                           ///////////////////////////////////////////////////

                           goto CFS_WAIT_POLL;
                        }
                        else {

                           return   CS_FAILURE
                                  | CFS_OPER_WAIT
                                  | CFS_DIAG_SYSTEM;
                        }
                     }
                  }
               }
               else {

                  return   CS_FAILURE
                         | CFS_OPER_READ
                         | CFS_DIAG_WOULDBLOCK;
               }
            }
            else {

               return   CS_FAILURE
                      | CFS_OPER_READ
                      | CFS_DIAG_SYSTEM;
            }
         }
      }
      else {

         if (rc == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done.
            /////////////////////////////////////////////////////////////////

            return CS_FAILURE | CFS_OPER_READ | CFS_DIAG_CONNCLOSE;
         }
         else {

            /////////////////////////////////////////////////////////////////
            // We read some data (maybe as much as required) ...
            // if not, we will continue reading for as long
            // as we don't block.
            /////////////////////////////////////////////////////////////////

            offset += rc;
            *size += rc;
            leftToRead -= rc;

            readSize =
               leftToRead > INT_MAX ?
                            INT_MAX :
                            leftToRead;
         }
      }
   }
   while (leftToRead > 0);

   return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_ReceiveDescriptor
//
// This function receives a file (socket) descriptor from another process.
// It is assumed that the caller has already established a connection to
// the other process via a local domain (UNIX) socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_ReceiveDescriptor
    (int fd,
     int* descriptor,
     int timeout) {

   // The peer needs to send some data
   // even though we will ignore it; this is required
   // by the sendmsg function.

   char dummyByte = 0;

   struct iovec iov[1];

   struct pollfd fdset[1];

   struct msghdr msgInstance;

   struct timeval to;
   fd_set  readSet;

   int rc;
   int maxHandle;

   //////////////////////////////////////////////////////////////////////////
   // ancillary (control) data.
   // This is where the descriptor will be held.
   //////////////////////////////////////////////////////////////////////////

#ifdef MSGHDR_MSG_CONTROL

   union {
      struct cmsghdr cm;
      char control[CMSG_SPACE(sizeof(int))];
   } control_un;

   struct cmsghdr* cmptr;

   msgInstance.msg_control = control_un.control;
   msgInstance.msg_controllen = sizeof(control_un.control);

#else

   int receivedFD;
   msgInstance.msg_accrights = (caddr_t)&receivedFD;
   msgInstance.msg_accrightslen = sizeof(int);

#endif

   msgInstance.msg_name = NULL;
   msgInstance.msg_namelen = 0;

   //msgInstance.msg_iov = NULL;
   //msgInstance.msg_iovlen = 0;

   iov[0].iov_base = &dummyByte;
   iov[0].iov_len = 1;

   msgInstance.msg_iov = iov;
   msgInstance.msg_iovlen = 1;

   //////////////////////////////////////////////////////////////////////////
   // This branching label for restarting an interrupted poll call.
   // An interrupted system call may results from a caught signal
   // and will have errno set to EINTR. We must call poll again.

   CFS_WAIT_DESCRIPTOR:

   //
   //////////////////////////////////////////////////////////////////////////

   *descriptor = -1;

   fdset[0].fd = fd;
   fdset[0].events = POLLIN;

   rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

   switch(rc) {

      case 0:  // timed-out

         rc = CS_FAILURE | CFS_OPER_WAIT | CFS_DIAG_TIMEDOUT;
         break;

      case 1:  // descriptor is ready

         if(fdset[0].revents == POLLIN) {

            // get the descriptor

            rc = recvmsg(fd, &msgInstance, 0);

            if (rc >= 0) {

               // Assume the rest will fail
               rc = CS_FAILURE | CFS_OPER_READ | CFS_DIAG_SYSTEM;

#ifdef MSGHDR_MSG_CONTROL


               if ( (cmptr = CMSG_FIRSTHDR(&msgInstance)) != NULL) {

                  if (cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {

                     if (cmptr->cmsg_level == SOL_SOCKET &&
                         cmptr->cmsg_type  == SCM_RIGHTS) {

                        *descriptor = *((int*)CMSG_DATA(cmptr));
                        rc = CS_SUCCESS;

                     }
                  }
               }

#else

               if (msgInstance.msg_accrightslen == sizeof(receivedFD)) {

                  *descriptor = receivedFD;
                  rc = CS_SUCCESS;
               }

#endif

            }
            else {
               rc = CS_FAILURE | CFS_OPER_READ | CFS_DIAG_SYSTEM;
            }
         }

         break;

      default:

         if (errno == EINTR) {

            goto CFS_WAIT_DESCRIPTOR;
         }
         else {
            rc = CS_FAILURE | CFS_OPER_WAIT | CFS_DIAG_SYSTEM;
         }

         break;
   }

   return rc;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureClose
//
// This function closes the secure session and environement.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_SecureClose
    (CFS_INSTANCE* This) {

   int iSSLResult;

   iSSLResult = gsk_secure_soc_close(&(This->ssl_hsession));
   iSSLResult = gsk_environment_close(&(This->ssl_henv));

   close(This->connfd);

   free(This);

   return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureConnect
//
// This function initialises a secure connection to a secure server.
//
//////////////////////////////////////////////////////////////////////////////

CFS_INSTANCE*
  CFS_SecureConnect
    (void* sessionInfo,
     int sessionInfoFmt,
     int* iSSLResult) {

   int rc;
   int e;
   int iLocalSSLResult;

   char szPort[11];
   char szAddr[40];

   CFS_INSTANCE* cfsi;
   CSRESULT hResult;

   struct addrinfo* addrInfo;
   struct addrinfo* addrInfo_first;
   struct addrinfo  hints;

   cfsi = (CFS_INSTANCE*)malloc(sizeof(CFS_INSTANCE));
   cfsi->size = sizeof(CFS_INSTANCE);
   cfsi->connfd = -1;

   hResult = CS_FAILURE;

   switch(sessionInfoFmt) {

      case CFS_CLIENTSESSION_FMT_100:

         memset(&hints, 0, sizeof(struct addrinfo));
         hints.ai_family = AF_UNSPEC;
         hints.ai_socktype = SOCK_STREAM;
         hints.ai_protocol = IPPROTO_TCP;

         sprintf(szPort, "%d",
                 ((CFS_CLIENTSESSION_100*)sessionInfo)->port);

         rc = getaddrinfo(((CFS_CLIENTSESSION_100*)sessionInfo)
                               ->szHostName,
                          szPort,
                          &hints,
                          &addrInfo);

         if (rc == 0)
         {
            addrInfo_first = addrInfo;
            cfsi->connfd = -1;

            while (addrInfo != 0)
            {
               cfsi->connfd = socket(addrInfo->ai_family,
                                     addrInfo->ai_socktype,
                                     addrInfo->ai_protocol);

               if (cfsi->connfd >= 0) {

                  // Set socket to non-blocking mode and leave the loop
                  CFS_PRV_SetBlocking(cfsi->connfd, 0);

                  hResult = CFS_PRV_DoSecureConnect_100(cfsi,
                                                        addrInfo,
                                                        (CFS_CLIENTSESSION_100*)sessionInfo,
                                                        &iLocalSSLResult);

                  if (CS_SUCCEED(hResult)) {
                    break;
                  }
                  else {
                      close(cfsi->connfd);
                  }
               }

               addrInfo = addrInfo->ai_next;
            }

            freeaddrinfo(addrInfo_first);
         }

         break;
   }

   if (CS_FAIL(hResult)) {

      free(cfsi);
      cfsi = 0;
   }

   return cfsi;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureOpenChannel
//
// This function initialises a secure environement and session. It also
// initialises an CFS_INSTANCE structure that must be used for secure
// communication with a peer.
//
//////////////////////////////////////////////////////////////////////////////

CFS_INSTANCE*
  CFS_SecureOpenChannel
    (int connfd,
     void* sessionInfo,
     int sessionInfoFmt,
     int* iSSLResult) {

   int rc;
   int e;
   int i;

   CFS_INSTANCE* cfsi;

   cfsi = (CFS_INSTANCE*)malloc(sizeof(CFS_INSTANCE));

   cfsi->size = sizeof(CFS_INSTANCE);

   cfsi->connfd = connfd;

   // Set socket to non-blocking mode
   CFS_PRV_SetBlocking(cfsi->connfd, 0);

   switch(sessionInfoFmt) {

      case CFS_SERVERSESSION_FMT_100:

         if (CS_FAIL(CFS_PRV_DoSecureOpenChannel_100(cfsi,
                                                 sessionInfo,
                                                 iSSLResult))) {

            CFS_SecureClose(cfsi);
            cfsi = NULL;
         }

         break;
   }

   return cfsi;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureRead
//
// This function reads a secure socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_SecureRead
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* maxSize,
     int timeout,
     int* iSSLResult) {

   int rc;
   int readSize;

   struct pollfd fdset[1];

   uint64_t initialSize;
   uint64_t leftToRead;
   uint64_t offset;

   initialSize = *maxSize;
   *maxSize    = 0;
   offset      = 0;

   leftToRead = initialSize;

   //////////////////////////////////////////////////////////////////////////
   // The total record size exceeds the maximum int value; we will
   // read up to int size at a time until we fill the buffer.
   //////////////////////////////////////////////////////////////////////////

   readSize =
     leftToRead > INT_MAX ?
                  INT_MAX :
                  leftToRead;

   //////////////////////////////////////////////////////////////////////////
   // We first try to read on the socket.
   //////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////
   // This branching label for restarting an interrupted
   // gsk_secure_soc_read call.

   CFS_SECURE_READ_1:

   //
   ////////////////////////////////////////////////////////////

   *iSSLResult = gsk_secure_soc_read(This->ssl_hsession,
                                     buffer + offset,
                                     readSize,
                                     &readSize);

   if (errno == EINTR) {

      //////////////////////////////////////////////////////
      // gsk_secure_soc_read was interrupted by a signal.
      // we must restart gsk_secure_soc_read.
      //////////////////////////////////////////////////////

      goto CFS_SECURE_READ_1;
   }

   //////////////////////////////////////////////////////////////////////////
   // If we blocked, then we will wait up to the timeout.
   //////////////////////////////////////////////////////////////////////////

   switch(*iSSLResult) {

      case GSK_OK:

         if (readSize == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done.
            /////////////////////////////////////////////////////////////////

            return CS_SUCCESS | CFS_OPER_READ | CFS_DIAG_CONNCLOSE;
         }
         else {

            /////////////////////////////////////////////////////////////////
            // We got some data (maybe as much as required) ...
            // if not, we will continue reading for as long
            // as we don't block before filling in the supplied buffer.
            /////////////////////////////////////////////////////////////////

            offset += readSize;
            *maxSize += readSize;
            leftToRead -= readSize;

            readSize =
               leftToRead > INT_MAX ?
                            INT_MAX :
                            leftToRead;

            if (leftToRead == 0) {

               // We have read as much as was required
               return CS_SUCCESS | CFS_OPER_READ | CFS_DIAG_ALLDATA;
            }
         }

         break;

      case GSK_WOULD_BLOCK:

         if (timeout != 0) {

            ////////////////////////////////////////////////////////////
            // This means we must wait up to a given timeout; this
            // is the only time we will be waitng as this means
            // we did not read any data and a positive timeout
            // means we give the peer some time to send over data.
            ////////////////////////////////////////////////////////////


            ////////////////////////////////////////////////////////////
            // This branching label for restarting an interrupted
            // poll call. An interrupted system call may result from
            // a caught signal and will have errno set to EINTR. We
            // must call poll again.

            CFS_READ_WAIT:

            //
            ////////////////////////////////////////////////////////////


            fdset[0].fd = This->connfd;
            fdset[0].events = POLLIN;

            rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

            if (rc == 1) {

               if (!fdset[0].revents & POLLIN) {

                  /////////////////////////////////////////////////////////
                  // If we get anything other than POLLIN
                  // this means we got an error.
                  /////////////////////////////////////////////////////////

                  return   CS_FAILURE
                         | CFS_OPER_WAIT
                         | CFS_DIAG_SYSTEM;
               }
            }
            else {

               if (rc == 0) {

                  return   CS_SUCCESS
                         | CFS_OPER_WAIT
                         | CFS_DIAG_TIMEDOUT;
               }
               else {

                  if (errno == EINTR) {

                     ///////////////////////////////////////////////////
                     // poll() was interupted by a signal
                     // or the kernel could not allocate an
                     // internal data structure. We will call
                     // poll() again.
                     ///////////////////////////////////////////////////

                     goto CFS_READ_WAIT;
                  }
                  else {

                     return   CS_FAILURE
                            | CFS_OPER_WAIT
                            | CFS_DIAG_SYSTEM;
                  }
               }
            }
         }
         else {

            return   CS_SUCCESS
                   | CFS_OPER_READ
                   | CFS_DIAG_WOULDBLOCK;
         }

         break;


      default:

         return   CS_FAILURE
                | CFS_OPER_READ
                | CFS_DIAG_SYSTEM;
   }

   ////////////////////////////////////////////////////////////////
   // If we get here, then we have read something but not up
   // to the maximum buffer size and more may be available.
   ////////////////////////////////////////////////////////////////

   do {

      ////////////////////////////////////////////////////////////
      // This branching label for restarting an interrupted
      // gsk_secure_soc_read call.

      CFS_SECURE_READ_2:

      //
      ////////////////////////////////////////////////////////////

      *iSSLResult = gsk_secure_soc_read(This->ssl_hsession,
                                        buffer + offset,
                                        readSize,
                                        &readSize);

      if (errno == EINTR) {

         //////////////////////////////////////////////////////
         // gsk_secure_soc_read was interrupted by a signal.
         // we must restart gsk_secure_soc_read.
         //////////////////////////////////////////////////////

         goto CFS_SECURE_READ_2;
      }

      if (*iSSLResult == GSK_OK) {

         if (readSize == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done.
            /////////////////////////////////////////////////////////////////

            return CS_SUCCESS | CFS_OPER_READ | CFS_DIAG_CONNCLOSE;
         }
         else {

            offset += readSize;
            *maxSize += readSize;
            leftToRead -= readSize;

            readSize =
               leftToRead > INT_MAX ?
                            INT_MAX :
                            leftToRead;
         }
      }
      else {

         switch(*iSSLResult) {

            case GSK_WOULD_BLOCK:

               ///////////////////////////////////////////////////
               // Once in this loop, we read until we block;
               // the timeout no longer applies (it applied
               // only for the first read).
               ///////////////////////////////////////////////////

               return   CS_SUCCESS
                      | CFS_OPER_READ
                      | CFS_DIAG_WOULDBLOCK;

               break;

            default:

               return   CS_FAILURE
                      | CFS_OPER_READ
                      | CFS_DIAG_SYSTEM;
         }
      }
   }
   while (leftToRead > 0);

   return CS_SUCCESS | CFS_OPER_READ | CFS_DIAG_ALLDATA;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureReadRecord
//
// This function reads a specific number of bytes on a secure socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_SecureReadRecord
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout,
     int* iSSLResult) {

   int rc;
   int readSize;

   struct pollfd fdset[1];

   uint64_t initialSize;
   uint64_t leftToRead;
   uint64_t offset;

   initialSize = *size;
   *size       = 0;
   offset      = 0;

   leftToRead = initialSize;

   //////////////////////////////////////////////////////////////////////////
   // The total record size exceeds the maximum int value; we will
   // read up to int size at a time until we fill the buffer.
   //////////////////////////////////////////////////////////////////////////

   readSize =
     leftToRead > INT_MAX ?
                  INT_MAX :
                  leftToRead;

   do {

      ////////////////////////////////////////////////////////////
      // This branching label for restarting an interrupted
      // gsk_secure_soc_read call.

      CFS_SECURE_READ:

      //
      ////////////////////////////////////////////////////////////

      *iSSLResult = gsk_secure_soc_read(This->ssl_hsession,
                                        buffer + offset,
                                        readSize,
                                        &readSize);

      if (errno == EINTR) {

         //////////////////////////////////////////////////////
         // gsk_secure_soc_read was interrupted by a signal.
         // we must restart gsk_secure_soc_read.
         //////////////////////////////////////////////////////

         goto CFS_SECURE_READ;
      }

      if (*iSSLResult == GSK_OK) {

         if (readSize == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done and since
            // we did not get all the data, this is a failure.
            /////////////////////////////////////////////////////////////////

            return CS_FAILURE | CFS_OPER_READ | CFS_DIAG_CONNCLOSE;
         }
         else {

            offset += readSize;
            *size += readSize;
            leftToRead -= readSize;

            readSize =
               leftToRead > INT_MAX ?
                            INT_MAX :
                            leftToRead;
         }
      }
      else {

         switch(*iSSLResult) {

            case GSK_WOULD_BLOCK:

               /////////////////////////////////////////////////////////////
               //
               // We can block under the following circunstances:
               //
               // 1) We blocked before getting all the expected data.
               //    If we have a non-zero timout, we will wait up to
               //    that timeout for data.
               //
               // 2) We wanted to get up to a fragment; if we have
               //    a non-zero timeout value, and if we got no data,
               //    we will wait up to the timeout value or until
               //    some data arrives. If we have a zero timeout value,
               //    we simply return to the caller with the appropriate
               //    diagnostics (along with data if any).
               //
               /////////////////////////////////////////////////////////////

               if (timeout != 0) {

                  ////////////////////////////////////////////////////////////
                  // This means we must wait up to a given timeout;
                  // note that we could be doing this more than once
                  // for example suppose we want to read 10 bytes
                  // and block on the firat read; we then wait but
                  // data arrives within the timeout. We then read
                  // 3 bytes and block again. We will wait again up
                  // to the timeout. The point is that the total
                  // amount of time waiting for data could
                  // theoretically exceed the timeout value.
                  ////////////////////////////////////////////////////////////


                  ////////////////////////////////////////////////////////////
                  // This branching label for restarting an interrupted
                  // poll call. An interrupted system call may results from
                  // a caught signal and will have errno set to EINTR. We
                  // must call poll again.

                  CFS_READ_WAIT:

                  //
                  ////////////////////////////////////////////////////////////


                  fdset[0].fd = This->connfd;
                  fdset[0].events = POLLIN;

                  rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

                  if (rc == 1) {

                     if (!fdset[0].revents & POLLIN) {

                        /////////////////////////////////////////////////////
                        // If we get anything other than POLLIN
                        // this means we got an error.
                        /////////////////////////////////////////////////////

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_SYSTEM;
                     }
                  }
                  else {

                     if (rc == 0) {

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_TIMEDOUT;
                     }
                     else {

                        if (errno == EINTR) {

                           goto CFS_READ_WAIT;
                        }
                        else {

                           return   CS_FAILURE
                                  | CFS_OPER_WAIT
                                  | CFS_DIAG_SYSTEM;
                        }
                     }
                  }
               }
               else {

                  return   CS_FAILURE
                         | CFS_OPER_READ
                         | CFS_DIAG_WOULDBLOCK;
               }

               break;

            default:

               return   CS_FAILURE
                      | CFS_OPER_READ
                      | CFS_DIAG_SYSTEM;
         }
      }
   }
   while (leftToRead > 0);

   return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureWrite
//
// This function writes to a secure socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_SecureWrite
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* maxSize,
     int timeout,
     int* iSSLResult) {

   int rc;
   int writeSize;

   struct pollfd fdset[1];

   uint64_t initialSize;
   uint64_t leftToWrite;
   uint64_t offset;

   initialSize = *maxSize;
   *maxSize    = 0;
   offset      = 0;

   leftToWrite = initialSize;

   //////////////////////////////////////////////////////////////////////////
   // The total record size exceeds the maximum int value; we will
   // write up to int size at a time.
   //////////////////////////////////////////////////////////////////////////

   writeSize =
     leftToWrite > INT_MAX ?
                   INT_MAX :
                   leftToWrite;

   writeSize = leftToWrite;

   //////////////////////////////////////////////////////////////////////////
   // We first try to write on the socket.
   //////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////
   // This branching label for restarting an interrupted
   // gsk_secure_soc_write call.

   CFS_SECURE_WRITE_1:

   //
   ////////////////////////////////////////////////////////////

   *iSSLResult = gsk_secure_soc_write(This->ssl_hsession,
                                      buffer + offset,
                                      writeSize,
                                      &writeSize);

   if (errno == EINTR) {

      //////////////////////////////////////////////////////
      // gsk_secure_soc_write was interrupted by a signal.
      // we must restart gsk_secure_soc_write.
      //////////////////////////////////////////////////////

      goto CFS_SECURE_WRITE_1;
   }

   //////////////////////////////////////////////////////////////////////////
   // If we blocked, then we will wait up to the timeout.
   //////////////////////////////////////////////////////////////////////////

   switch(*iSSLResult) {

      case GSK_OK:

         if (writeSize == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done.
            /////////////////////////////////////////////////////////////////

            return CS_FAILURE | CFS_OPER_WRITE | CFS_DIAG_CONNCLOSE;
         }
         else {

            /////////////////////////////////////////////////////////////////
            // We wrote some data (maybe as much as required ...
            // if not, we will continue writing for as long
            // as we don't block before sending out the supplied buffer.
            /////////////////////////////////////////////////////////////////

            offset += writeSize;
            *maxSize += writeSize;
            leftToWrite -= writeSize;

            writeSize =
               leftToWrite > INT_MAX ?
                             INT_MAX :
                             leftToWrite;

            if (leftToWrite == 0) {

               // We have written as much as was required.
               return CS_SUCCESS | CFS_OPER_WRITE | CFS_DIAG_ALLDATA;
            }
         }

         break;

      case GSK_WOULD_BLOCK:

         if (timeout != 0) {

            ////////////////////////////////////////////////////////////
            // This means we must wait up to a given timeout; this
            // is the only time we will be waitng as this means
            // we did not write any data and a positive timeout
            // means we give the kernel some time to send over the data.
            ////////////////////////////////////////////////////////////


            ////////////////////////////////////////////////////////////
            // This branching label for restarting an interrupted
            // poll call. An interrupted system call may result from
            // a caught signal and will have errno set to EINTR. We
            // must call poll again.

            CFS_WRITE_WAIT:

            //
            ////////////////////////////////////////////////////////////


            fdset[0].fd = This->connfd;
            fdset[0].events = POLLOUT;

            rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

            if (rc == 1) {

               if (!fdset[0].revents & POLLOUT) {

                  ///////////////////////////////////////////////////////
                  // If we get anything other than POLLOUT
                  // this means we got an error.
                  ///////////////////////////////////////////////////////

                  return   CS_FAILURE
                         | CFS_OPER_WAIT
                         | CFS_DIAG_SYSTEM;
               }
            }
            else {

               if (rc == 0) {

                  return   CS_SUCCESS
                         | CFS_OPER_WAIT
                         | CFS_DIAG_TIMEDOUT;
               }
               else {

                  if (errno == EINTR) {

                     ///////////////////////////////////////////////////
                     // poll() was interupted by a signal
                     // or the kernel could not allocate an
                     // internal data structure. We will call
                     // poll() again.
                     ///////////////////////////////////////////////////

                     goto CFS_WRITE_WAIT;
                  }
                  else {

                     return   CS_FAILURE
                            | CFS_OPER_WAIT
                            | CFS_DIAG_SYSTEM;
                  }
               }
            }
         }
         else {

            return   CS_SUCCESS
                   | CFS_OPER_WRITE
                   | CFS_DIAG_WOULDBLOCK;
         }

         break;


      default:

         return   CS_FAILURE
                | CFS_OPER_WRITE
                | CFS_DIAG_SYSTEM;
   }

   ////////////////////////////////////////////////////////////////
   // If we get here, then we have written something but not up
   // to the maximum buffer size and more must be sent.
   ////////////////////////////////////////////////////////////////

   do {

      ////////////////////////////////////////////////////////////
      // This branching label for restarting an interrupted
      // gsk_secure_soc_write call.

      CFS_SECURE_WRITE_2:

      //
      ////////////////////////////////////////////////////////////

      *iSSLResult = gsk_secure_soc_write(This->ssl_hsession,
                                         buffer + offset,
                                         writeSize,
                                         &writeSize);

      if (errno == EINTR) {

         //////////////////////////////////////////////////////
         // gsk_secure_soc_write was interrupted by a signal.
         // we must restart gsk_secure_soc_write.
         //////////////////////////////////////////////////////

         goto CFS_SECURE_WRITE_2;
      }

      if (*iSSLResult == GSK_OK) {

         if (writeSize == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done.
            /////////////////////////////////////////////////////////////////

            return CS_FAILURE | CFS_OPER_WRITE | CFS_DIAG_CONNCLOSE;
         }
         else {

            offset += writeSize;
            *maxSize += writeSize;
            leftToWrite -= writeSize;

            writeSize =
               leftToWrite > INT_MAX ?
                             INT_MAX :
                             leftToWrite;
         }
      }
      else {

         switch(*iSSLResult) {

            case GSK_WOULD_BLOCK:

               ///////////////////////////////////////////////////
               // Once in this loop, we write until we block;
               // the timeout no longer applies (it applied
               // only for the first write).
               ///////////////////////////////////////////////////

               return   CS_SUCCESS
                      | CFS_OPER_WRITE
                      | CFS_DIAG_WOULDBLOCK;

               break;

            default:

               return   CS_FAILURE
                      | CFS_OPER_WRITE
                      | CFS_DIAG_SYSTEM;
         }
      }
   }
   while (leftToWrite > 0);

   return CS_SUCCESS | CFS_OPER_WRITE | CFS_DIAG_ALLDATA;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SecureWriteRecord
//
// This function writes a specific number of bytes to a secure socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_SecureWriteRecord
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout,
     int* iSSLResult) {

   int rc;
   int writeSize;

   struct pollfd fdset[1];

   uint64_t initialSize;
   uint64_t leftToWrite;
   uint64_t offset;

   initialSize = *size;
   *size       = 0;
   offset      = 0;

   leftToWrite = initialSize;

   //////////////////////////////////////////////////////////////////////////
   // The total record size exceeds the maximum int value; we will
   // write up to int size at a time until we send the entire buffer.
   //////////////////////////////////////////////////////////////////////////

   writeSize =
     leftToWrite > INT_MAX ?
                   INT_MAX :
                   leftToWrite;

   writeSize = leftToWrite;

   do {

      ////////////////////////////////////////////////////////////
      // This branching label for restarting an interrupted
      // gsk_secure_soc_write call.

      CFS_SECURE_WRITE:

      //
      ////////////////////////////////////////////////////////////

      *iSSLResult = gsk_secure_soc_write(This->ssl_hsession,
                                         buffer + offset,
                                         writeSize,
                                         &writeSize);

      if (errno == EINTR) {

         //////////////////////////////////////////////////////
         // gsk_secure_soc_write was interrupted by a signal.
         // we must restart gsk_secure_soc_write.
         //////////////////////////////////////////////////////

         goto CFS_SECURE_WRITE;
      }

      if (*iSSLResult == GSK_OK) {

         if (writeSize == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done and since
            // we did not send all the data, this is a failure.
            /////////////////////////////////////////////////////////////////

            return CS_FAILURE | CFS_OPER_WRITE | CFS_DIAG_CONNCLOSE;
         }
         else {

            offset += writeSize;
            *size += writeSize;
            leftToWrite -= writeSize;

            writeSize =
               leftToWrite > INT_MAX ?
                             INT_MAX :
                             leftToWrite;
         }
      }
      else {

         switch(*iSSLResult) {

            case GSK_WOULD_BLOCK:

               /////////////////////////////////////////////////////////////
               //
               // We can block under the following circunstances:
               //
               // 1) We blocked before writing all the required data.
               //    If we have a non-zero timout, we will wait up to
               //    that timeout to write data.
               //
               // 2) We wanted to write up to a fragment; if we have
               //    a non-zero timeout value, and if we wrote no data,
               //    we will wait up to the timeout value or until
               //    some data can be written. If we have a zero timeout
               //    value, we simply return to the caller with the
               //    appropriate diagnostics.
               //
               /////////////////////////////////////////////////////////////

               if (timeout != 0) {

                  ////////////////////////////////////////////////////////////
                  // This means we must wait up to a given timeout;
                  // note that we could be doing this more than once.
                  // The point is that the total
                  // amount of time waiting to write data could
                  // theoretically exceed the timeout value.
                  ////////////////////////////////////////////////////////////


                  ////////////////////////////////////////////////////////////
                  // This branching label for restarting an interrupted
                  // poll call. An interrupted system call may results from
                  // a caught signal and will have errno set to EINTR. We
                  // must call poll again.

                  CFS_WRITE_WAIT:

                  //
                  ////////////////////////////////////////////////////////////


                  fdset[0].fd = This->connfd;
                  fdset[0].events = POLLOUT;

                  rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

                  if (rc == 1) {

                     if (!fdset[0].revents & POLLOUT) {

                        /////////////////////////////////////////////////////
                        // If we get anything other than POLLOUT
                        // this means we cannot write
                        // after our timeout period.
                        /////////////////////////////////////////////////////

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_SYSTEM;
                     }
                  }
                  else {

                     if (rc == 0) {

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_TIMEDOUT;
                     }
                     else {

                        if (errno == EINTR) {

                           goto CFS_WRITE_WAIT;
                        }
                        else {

                           return   CS_FAILURE
                                  | CFS_OPER_WAIT
                                  | CFS_DIAG_SYSTEM;
                        }
                     }
                  }
               }
               else {

                  return   CS_FAILURE
                         | CFS_OPER_WRITE
                         | CFS_DIAG_WOULDBLOCK;
               }

               break;

            default:

               return   CS_FAILURE
                      | CFS_OPER_WRITE
                      | CFS_DIAG_SYSTEM;
         }
      }
   }
   while (leftToWrite > 0);

   return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_SendDescriptor
//
// This function sends a file (socket) descriptor to another process.
// The caller has already established a connection to the other process
// via a local domain (UNIX) socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_SendDescriptor
    (int fd,
     int descriptor,
     int timeout) {

   struct iovec iov[1];

   struct pollfd fdset[1];

   struct msghdr msgInstance;

   struct timeval to;
   fd_set  writeSet;

   int rc;
   int maxHandle;

   // We need to send some data, even though it will be ignored
   // by the peer

   char dummyByte = 0;

   //////////////////////////////////////////////////////////////////////////
   // ancillary (control) data.
   // This is where the descriptor will be held.
   //////////////////////////////////////////////////////////////////////////

#ifdef MSGHDR_MSG_CONTROL

   //////////////////////////////////////////////////////////////////////////
   // We are using a cmsghdr to pass along the ancillary data ...
   // This union is to properly align the cmsghdr structure with the data
   // buffer that will hold the descriptor.
   //////////////////////////////////////////////////////////////////////////

   union {
      struct cmsghdr cm;
      char control[CMSG_SPACE(sizeof(int))];
   } control_un;

   struct cmsghdr* cmptr;

   //////////////////////////////////////////////////////////////////////////
   //  Initialise the msghdr with the ancillary data and
   //  ancillary data length.
   //////////////////////////////////////////////////////////////////////////

   msgInstance.msg_control    = control_un.control;
   msgInstance.msg_controllen = sizeof(control_un.control);

   //////////////////////////////////////////////////////////////////////////
   //  Initialise the ancillary data itself with the
   //  descriptor to pass along sendmsg().
   //////////////////////////////////////////////////////////////////////////

   // point to first (and only) ancillary data entry.
   cmptr = CMSG_FIRSTHDR(&msgInstance);

   // initialise ancillary data header.
   cmptr->cmsg_len   = CMSG_LEN(sizeof(int));  // size of descriptor
   cmptr->cmsg_level = SOL_SOCKET;
   cmptr->cmsg_type  = SCM_RIGHTS;

   //////////////////////////////////////////////////////////////////////////
   // Assign the descriptor to ancillary data
   //
   // CMSG_DATA will return the address of the first data byte,
   // which is located somewhere in the control_un.control array.
   // To assign the descriptor, which is an integer, we must cast
   // this address to int* and dereference the address to set
   // it to the descriptor value.
   //////////////////////////////////////////////////////////////////////////

   *((int*)CMSG_DATA(cmptr)) = descriptor;

#else

   msgInstance.msg_accrights = (caddr_t)&descriptor;
   msgInstance.msg_accrightslen = sizeof(descriptor);

#endif

   msgInstance.msg_name    = NULL;
   msgInstance.msg_namelen = 0;

   //msgInstance.msg_iov     = NULL;
   //msgInstance.msg_iovlen  = 0;

   iov[0].iov_base = &dummyByte;
   iov[0].iov_len = 1;

   msgInstance.msg_iov = iov;
   msgInstance.msg_iovlen = 1;

   //////////////////////////////////////////////////////////////////////////
   // Send the descriptor via the stream pipe descriptor.
   //////////////////////////////////////////////////////////////////////////

   //////////////////////////////////////////////////////////////////////////
   // This branching label for restarting an interrupted poll call.
   // An interrupted system call may results from a caught signal
   // and will have errno set to EINTR. We must call poll again.

   CFS_WAIT_DESCRIPTOR:

   //
   //////////////////////////////////////////////////////////////////////////

   fdset[0].fd = fd;
   fdset[0].events = POLLOUT;

   rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

   switch(rc) {

      case 0:  // timed-out

         rc = CS_FAILURE | CFS_OPER_WAIT | CFS_DIAG_TIMEDOUT;
         break;

      case 1:  // descriptor is ready

         if(fdset[0].revents == POLLOUT) {

            rc = sendmsg(fd, &msgInstance, 0);

            if (rc < 0) {
               rc = CS_FAILURE | CFS_OPER_WRITE  | CFS_DIAG_SYSTEM;
            }
            else {
               rc = CS_SUCCESS;
            }
         }
         else {

            rc = CS_FAILURE | CFS_OPER_WAIT | CFS_DIAG_SYSTEM;
         }

         break;

      default:

         if (errno == EINTR) {

            goto CFS_WAIT_DESCRIPTOR;
         }
         else {
            rc = CS_FAILURE | CFS_OPER_WAIT | CFS_DIAG_SYSTEM;
         }

         break;
   }

   return rc;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_Write
//
// This function writes to a non-secure socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_Write
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* maxSize,
     int timeout) {

   int rc;
   int writeSize;

   struct pollfd fdset[1];

   uint64_t initialSize;
   uint64_t leftToWrite;
   uint64_t offset;

   initialSize = *maxSize;
   *maxSize    = 0;
   offset      = 0;

   leftToWrite = initialSize;

   //////////////////////////////////////////////////////////////////////////
   // The total record size exceeds the maximum int value; we will
   // write up to int size at a time.
   //////////////////////////////////////////////////////////////////////////

   writeSize =
     leftToWrite > INT_MAX ?
                   INT_MAX :
                   leftToWrite;

   //////////////////////////////////////////////////////////////////////////
   // We first try to write on the socket.
   //////////////////////////////////////////////////////////////////////////

   /////////////////////////////////////////////////////////
   // This branching label for restarting an interrupted
   // send() call. An interrupted system call may result
   // from a signal and will have errno set to EINTR.
   // We must call send() again.

   CFS_WAIT_SEND_1:

   //
   /////////////////////////////////////////////////////////

   rc = send(This->connfd, buffer + offset, writeSize, 0);

   if (rc < 0) {

      if (errno == EINTR) {

         ///////////////////////////////////////////////////
         // send() was interupted by a signal
         // or the kernel could not allocate an
         // internal data structure. We will call
         // send() again.
         ///////////////////////////////////////////////////

         goto CFS_WAIT_SEND_1;
      }
      else {

         if (errno == EWOULDBLOCK) {

            if (timeout != 0) {

               ////////////////////////////////////////////////////////////
               // This means we must wait up to a given timeout; this
               // is the only time we will be waitng as this means
               // we did not write any data and a positive timeout
               // means we give the kernel some time to send over the data.
               ////////////////////////////////////////////////////////////


               ////////////////////////////////////////////////////////////
               // This branching label for restarting an interrupted
               // poll call. An interrupted system call may result from
               // a caught signal and will have errno set to EINTR. We
               // must call poll again.

               CFS_WAIT_POLL:

               //
               ////////////////////////////////////////////////////////////


               fdset[0].fd = This->connfd;
               fdset[0].events = POLLOUT;

               rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

               if (rc == 1) {

                  /////////////////////////////////////////////////////////
                  // If we get anything other than POLLOUT
                  // this means thre was an error.
                  /////////////////////////////////////////////////////////

                  if (!fdset[0].revents & POLLOUT) {

                     return   CS_FAILURE
                            | CFS_OPER_WAIT
                            | CFS_DIAG_SYSTEM;
                  }
               }
               else {

                  if (rc == 0) {

                     return   CS_SUCCESS
                            | CFS_OPER_WAIT
                            | CFS_DIAG_TIMEDOUT;
                  }
                  else {

                     if (errno == EINTR) {

                        ///////////////////////////////////////////////////
                        // poll() was interupted by a signal
                        // or the kernel could not allocate an
                        // internal data structure. We will call
                        // poll() again.
                        ///////////////////////////////////////////////////

                        goto CFS_WAIT_POLL;
                     }
                     else {

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_SYSTEM;
                     }
                  }
               }
            }
            else {

               return   CS_SUCCESS
                      | CFS_OPER_WRITE
                      | CFS_DIAG_WOULDBLOCK;
            }
         }
         else {

            return   CS_FAILURE
                   | CFS_OPER_WAIT
                   | CFS_DIAG_SYSTEM;
         }
      }
   }
   else {

      if (rc == 0) {

         /////////////////////////////////////////////////////////////////
         // This indicates a connection close; we are done.
         /////////////////////////////////////////////////////////////////

         return CS_FAILURE | CFS_OPER_WRITE | CFS_DIAG_CONNCLOSE;
      }
      else {

         /////////////////////////////////////////////////////////////////
         // We wrote some data (maybe as much as required) ...
         // if not, we will continue writing for as long
         // as we don't block before sending out the supplied buffer.
         /////////////////////////////////////////////////////////////////

         offset += rc;
         *maxSize += rc;
         leftToWrite -= rc;

         writeSize =
            leftToWrite > INT_MAX ?
                          INT_MAX :
                          leftToWrite;

         if (leftToWrite == 0) {

            // We have written as much as was required.
            return CS_SUCCESS | CFS_OPER_WRITE | CFS_DIAG_ALLDATA;
         }
      }
   }

   ////////////////////////////////////////////////////////////////
   // If we get here, then we have written something but not up
   // to the maximum buffer size and more must be sent.
   ////////////////////////////////////////////////////////////////

   do {

      /////////////////////////////////////////////////////////
      // This branching label for restarting an interrupted
      // send() call. An interrupted system call may result
      // from a signal and will have errno set to EINTR.
      // We must call send() again.

      CFS_WAIT_SEND_2:

      //
      /////////////////////////////////////////////////////////

      rc = send(This->connfd, buffer + offset, writeSize, 0);

      if (rc < 0) {

         if (errno == EINTR) {

            ///////////////////////////////////////////////////
            // send() was interupted by a signal
            // or the kernel could not allocate an
            // internal data structure. We will call
            // send() again.
            ///////////////////////////////////////////////////

            goto CFS_WAIT_SEND_2;
         }
         else {

            if (errno == EWOULDBLOCK) {

               ///////////////////////////////////////////////////
               // Once in this loop, we write until we block;
               // the timeout no longer applies (it applied
               // only for the first write).
               ///////////////////////////////////////////////////

               return   CS_SUCCESS
                      | CFS_OPER_WRITE
                      | CFS_DIAG_WOULDBLOCK;
            }
            else {

               return   CS_FAILURE
                      | CFS_OPER_WRITE
                      | CFS_DIAG_SYSTEM;
            }
         }
      }
      else {

         if (writeSize == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done.
            /////////////////////////////////////////////////////////////////

            return CS_FAILURE | CFS_OPER_WRITE | CFS_DIAG_CONNCLOSE;
         }
         else {

            offset += rc;
            *maxSize += rc;
            leftToWrite -= rc;

            writeSize =
               leftToWrite > INT_MAX ?
                             INT_MAX :
                             leftToWrite;
         }
      }
   }
   while (leftToWrite > 0);

   return CS_SUCCESS | CFS_OPER_WRITE | CFS_DIAG_ALLDATA;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_WriteRecord
//
// This function writes a specific number of bytes to a non secure socket.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_WriteRecord
    (CFS_INSTANCE* This,
     char* buffer,
     uint64_t* size,
     int timeout) {

   int rc;
   int writeSize;

   struct pollfd fdset[1];

   uint64_t initialSize;
   uint64_t leftToWrite;
   uint64_t offset;

   initialSize = *size;
   *size       = 0;
   offset      = 0;

   leftToWrite = initialSize;

   //////////////////////////////////////////////////////////////////////////
   // The total record size exceeds the maximum int value; we will
   // write up to int size at a time until we send the entire buffer.
   //////////////////////////////////////////////////////////////////////////

   writeSize =
     leftToWrite > INT_MAX ?
                   INT_MAX :
                   leftToWrite;

   do {

      /////////////////////////////////////////////////////////
      // This branching label for restarting an interrupted
      // send() call. An interrupted system call may result
      // from a signal and will have errno set to EINTR.
      // We must call send() again.

      CFS_WAIT_SEND:

      //
      /////////////////////////////////////////////////////////

      rc = send(This->connfd, buffer + offset, writeSize, 0);

      if (rc < 0) {

         if (errno == EINTR) {

            ///////////////////////////////////////////////////
            // send() was interupted by a signal
            // or the kernel could not allocate an
            // internal data structure. We will call
            // send() again.
            ///////////////////////////////////////////////////

            goto CFS_WAIT_SEND;
         }
         else {

            if (errno == EWOULDBLOCK) {

               if (timeout != 0) {

                  ////////////////////////////////////////////////////////////
                  // This means we must wait up to a given timeout; this
                  // is the only time we will be waitng as this means
                  // we did not write any data and a positive timeout
                  // means we give the kernel some time to send over the data.
                  ////////////////////////////////////////////////////////////


                  ////////////////////////////////////////////////////////////
                  // This branching label for restarting an interrupted
                  // poll call. An interrupted system call may result from
                  // a caught signal and will have errno set to EINTR. We
                  // must call poll again.

                  CFS_WAIT_POLL:

                  //
                  ////////////////////////////////////////////////////////////


                  fdset[0].fd = This->connfd;
                  fdset[0].events = POLLOUT;

                  rc = poll(fdset, 1, timeout >= 0 ? timeout * 1000: -1);

                  if (rc == 1) {

                     /////////////////////////////////////////////////////////
                     // If we get anything other than POLLOUT
                     // this means we got an error.
                     /////////////////////////////////////////////////////////

                     if (!fdset[0].revents & POLLOUT) {

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_SYSTEM;
                     }
                  }
                  else {

                     if (rc == 0) {

                        return   CS_FAILURE
                               | CFS_OPER_WAIT
                               | CFS_DIAG_TIMEDOUT;
                     }
                     else {

                        if (errno == EINTR) {

                           ///////////////////////////////////////////////////
                           // poll() was interupted by a signal
                           // or the kernel could not allocate an
                           // internal data structure. We will call
                           // poll() again.
                           ///////////////////////////////////////////////////

                           goto CFS_WAIT_POLL;
                        }
                        else {

                           return   CS_FAILURE
                                  | CFS_OPER_WAIT
                                  | CFS_DIAG_SYSTEM;
                        }
                     }
                  }
               }
               else {

                  return   CS_FAILURE
                         | CFS_OPER_WAIT
                         | CFS_DIAG_WOULDBLOCK;
               }
            }
            else {

               return   CS_FAILURE
                      | CFS_OPER_WAIT
                      | CFS_DIAG_SYSTEM;
            }
         }
      }
      else {

         if (rc == 0) {

            /////////////////////////////////////////////////////////////////
            // This indicates a connection close; we are done.
            /////////////////////////////////////////////////////////////////

            return CS_SUCCESS | CFS_OPER_WRITE | CFS_DIAG_CONNCLOSE;
         }
         else {

            /////////////////////////////////////////////////////////////////
            // We wrote some data (maybe as much as required) ...
            // if not, we will continue writing for as long
            // as we don't block before sending out the supplied buffer.
            /////////////////////////////////////////////////////////////////

            offset += rc;
            *size += rc;
            leftToWrite -= rc;

            writeSize =
               leftToWrite > INT_MAX ?
                             INT_MAX :
                             leftToWrite;
         }
      }
   }
   while (leftToWrite > 0);

   return CS_SUCCESS;
}

/* ===========================================================================
   PRIVATE FUNCTIONS
=========================================================================== */

//////////////////////////////////////////////////////////////////////////////
//
// CFS_PRV_DoSecureConnect_100
//
// Sets up a secure session with a server
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_PRV_DoSecureConnect_100
    (CFS_INSTANCE* cfsi,
     struct addrinfo* addrInfo,
     CFS_CLIENTSESSION_100* sessionInfo,
     int* iSSLResult) {

   int rc;
   int e;

   CSRESULT hResult;

   struct timeval tv;

   fd_set readSet, writeSet;

   *iSSLResult = gsk_environment_open(&(cfsi->ssl_henv));

   if (*iSSLResult != GSK_OK) {
      return CS_FAILURE;
   }

   *iSSLResult = gsk_attribute_set_enum(cfsi->ssl_henv,
                                     GSK_SESSION_TYPE,
                                     GSK_CLIENT_SESSION);

   if (*iSSLResult != GSK_OK) {
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   if (sessionInfo->szApplicationID == 0) {

     *iSSLResult = gsk_attribute_set_buffer
                               (cfsi->ssl_henv,
                                GSK_KEYRING_FILE,
                                "*SYSTEM",
                                7);
   }
   else {

      *iSSLResult = gsk_attribute_set_buffer
                                 (cfsi->ssl_henv,
                                  GSK_OS400_APPLICATION_ID,
                                  sessionInfo->szApplicationID,
                                  strlen(sessionInfo->szApplicationID));
   }

   if (*iSSLResult != GSK_OK) {
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   switch(sessionInfo->secSessionType) {

      case CFS_SEC_CLIENT_SESSION_SERVER_AUTH_PASSTHROUGH:

         *iSSLResult = gsk_attribute_set_enum(cfsi->ssl_henv,
                                GSK_SERVER_AUTH_TYPE,
                                GSK_SERVER_AUTH_PASSTHRU);

        break;

      default:

         *iSSLResult = gsk_attribute_set_enum(cfsi->ssl_henv,
                                GSK_SERVER_AUTH_TYPE,
                                GSK_SERVER_AUTH_FULL);

        break;
   }

   if (*iSSLResult != GSK_OK) {
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   *iSSLResult = gsk_environment_init(cfsi->ssl_henv);

   if (*iSSLResult != GSK_OK) {
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   hResult = CS_SUCCESS;

   rc = connect(cfsi->connfd,
                addrInfo->ai_addr,
                addrInfo->ai_addrlen);


   if (rc < 0) {

     if (errno != EINPROGRESS) {

       e = errno;
       gsk_environment_close(&(cfsi->ssl_henv));
       hResult = CS_FAILURE;
     }
     else {

       FD_ZERO(&readSet);
       FD_SET(cfsi->connfd, &readSet);

       writeSet = readSet;
       tv.tv_sec = sessionInfo->connTimeout;
       tv.tv_usec = 0;

       rc = select(cfsi->connfd+1,
                   &readSet,
                   &writeSet,
                   NULL,
                   &tv);

       if (rc <= 0) {

         e = errno;
         gsk_environment_close(&(cfsi->ssl_henv));
         hResult = CS_FAILURE;
       }
     }
   }

   if (hResult == CS_SUCCESS) {

     *iSSLResult = gsk_secure_soc_open(cfsi->ssl_henv,
                                           &(cfsi->ssl_hsession));

     if (*iSSLResult != GSK_OK) {
       gsk_environment_close(&(cfsi->ssl_henv));
       return CS_FAILURE;
     }

     *iSSLResult = gsk_attribute_set_numeric_value(cfsi->ssl_hsession,
                                                   GSK_FD,
                                                   cfsi->connfd);

     if (*iSSLResult != GSK_OK) {
       gsk_secure_soc_close(&(cfsi->ssl_hsession));
       gsk_environment_close(&(cfsi->ssl_henv));
       return CS_FAILURE;
     }

     *iSSLResult = gsk_secure_soc_init(cfsi->ssl_hsession);

     if (*iSSLResult != GSK_OK) {
       gsk_secure_soc_close(&(cfsi->ssl_hsession));
       gsk_environment_close(&(cfsi->ssl_henv));
       return CS_FAILURE;
     }
   }

   return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_PRV_DoSecureOpenChannel_100
//
// Sets up a secure session with a connecting client.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_PRV_DoSecureOpenChannel_100
    (CFS_INSTANCE* cfsi,
     CFS_SERVERSESSION_100* sessionInfo,
     int*  iSSLResult) {


   int i;

   *iSSLResult = gsk_environment_open(&(cfsi->ssl_henv));

   if (*iSSLResult != GSK_OK) {
      return CS_FAILURE;
   }

   if (sessionInfo->szApplicationID == 0) {

      *iSSLResult = gsk_attribute_set_buffer
                                     (cfsi->ssl_henv,
                                      GSK_KEYRING_FILE,
                                      "*SYSTEM",
                                      7);
   }
   else {

      *iSSLResult = gsk_attribute_set_buffer
                                 (cfsi->ssl_henv,
                                  GSK_OS400_APPLICATION_ID,
                                  sessionInfo->szApplicationID,
                                  strlen(sessionInfo->szApplicationID));
   }

   if (*iSSLResult != GSK_OK) {
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   switch(sessionInfo->secSessionType) {

      case CFS_SEC_SERVER_SESSION_CLIENT_AUTH_CRITICAL:

        *iSSLResult = gsk_attribute_set_enum(cfsi->ssl_henv,
                                GSK_SESSION_TYPE,
                                GSK_SERVER_SESSION_WITH_CL_AUTH_CRITICAL);

              break;

      case CFS_SEC_SERVER_SESSION_CLIENT_AUTH:

        *iSSLResult = gsk_attribute_set_enum(cfsi->ssl_henv,
                                   GSK_SESSION_TYPE,
                                   GSK_SERVER_SESSION_WITH_CL_AUTH);

              break;

         default:

        *iSSLResult = gsk_attribute_set_enum(cfsi->ssl_henv,
                                   GSK_SESSION_TYPE,
                                   GSK_SERVER_SESSION);

              break;
   }

   if (*iSSLResult != GSK_OK) {
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   *iSSLResult = gsk_environment_init(cfsi->ssl_henv);

   if (*iSSLResult != GSK_OK) {
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   *iSSLResult = gsk_secure_soc_open(cfsi->ssl_henv,
                                     &(cfsi->ssl_hsession));

   if (*iSSLResult != GSK_OK) {
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   *iSSLResult = gsk_attribute_set_numeric_value(cfsi->ssl_hsession,
                                                 GSK_FD,
                                                 cfsi->connfd);

   if (*iSSLResult != GSK_OK) {
      gsk_secure_soc_close(&(cfsi->ssl_hsession));
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   *iSSLResult = gsk_secure_soc_init(cfsi->ssl_hsession);

   if (*iSSLResult != GSK_OK) {
      gsk_secure_soc_close(&(cfsi->ssl_hsession));
      gsk_environment_close(&(cfsi->ssl_henv));
      return CS_FAILURE;
   }

   return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_PRV_NetworkToPresentation
//
// This function returns a string representation of a network address.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_PRV_NetworkToPresentation
    (const struct sockaddr* sa,
     char* addrstr,
     char* portstr)
{
  CSRESULT rc;

  struct sockaddr_in*  sin;
  struct sockaddr_un*  unp;
  struct sockaddr_in6* sin6;
  struct sockaddr_dl*  sdl;

  strcpy(addrstr, "");
  strcpy(portstr, "");
  rc = CS_SUCCESS;

  switch(sa->sa_family)
  {
    case AF_INET:
    {
      sin = (struct sockaddr_in*)sa;

      if (inet_ntop(AF_INET, &sin->sin_addr, addrstr,
                    (socklen_t)CFS_NTOP_ADDR_MAX) != 0)
      {
         snprintf(portstr, CFS_NTOP_PORT_MAX, "%d", ntohs(sin->sin_port));
          rc = CS_SUCCESS;
      }

      break;
    }

    case AF_INET6:
    {
      sin6 = (struct sockaddr_in6 *)sa;
      addrstr[0] = '[';
      if (inet_ntop(AF_INET6, &sin6->sin6_addr,
              addrstr + 1,
              (socklen_t)(CFS_NTOP_ADDR_MAX - 1)) != (const char*)NULL)
      {
        snprintf(portstr, CFS_NTOP_PORT_MAX, "%d", ntohs(sin6->sin6_port));
        strcat(addrstr, "]");
        rc = CS_SUCCESS;
      }

      break;
    }

    case AF_UNIX:
    {
      unp = (struct sockaddr_un *)sa;

      /* OK to have no pathname bound to the socket: happens on
           every connect() unless client calls bind() first. */

      if (unp->sun_path[0] == 0)
      {
        strcpy(addrstr, "(no pathname bound)");
      }
      else
      {
        snprintf(addrstr, CFS_NTOP_ADDR_MAX, "%s", unp->sun_path);
        rc = CS_SUCCESS;
      }

      break;
    }

    default:
    {
       snprintf(addrstr, CFS_NTOP_ADDR_MAX,
                "sock_ntop: unknown AF_xxx: %d",
                sa->sa_family);

       rc = CS_FAILURE;
       break;
    }
  }

  return rc;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFS_PRV_SetBlocking
//
// This function sets a socket descriptor to either blocking
// or non-blocking mode.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CFS_PRV_SetBlocking
    (int connfd,
	 int blocking) {

    /* Save the current flags */

    int flags = fcntl(connfd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;

    if (fcntl(connfd, F_SETFL, flags) == -1)
      return CS_FAILURE;

    return CS_SUCCESS;
}

CSRESULT
  CFS_GetCurJobName
    (char szJobInfo[27]) {

  JOBINFOSTRUCT jobInfo;

  QUSRJOBI(&jobInfo,
           sizeof(JOBINFOSTRUCT),
           "JOBI0100",
           "*                         ",
           "                ");

  memcpy(szJobInfo,    jobInfo.JobName,   10);
  memcpy(szJobInfo+10, jobInfo.JobUser,   10);
  memcpy(szJobInfo+20, jobInfo.JobNumber, 6);

  szJobInfo[26] = 0;

  return CS_SUCCESS;
}
