/* ==========================================================================
  Clarasoft Foundation Server 400

  echohdlr.c
  Clara daemon dynamic handler job (non-secure/secure handler)
  Version 1.0.0

  Compile and bind the module into a service program. Export both
  functions. To enable dynamic binding of these functions, insert two
  records in the CFSREG file:


  First record:

  RGSRVNM: ECHO       // Last argument to the clarad daemon
  RGLIBNM: MYLIB      // The library where the service program resides
  RGPRCHD: ECHOHDLR   // The service program name
  RGPRCNM: echo       // The case sensitive name of exported function
                      // for non secure session


  Second record:

  RGSRVNM: ECHOSEC       // Last argument to the clarad daemon
  RGLIBNM: MYLIB         // The library where the service program resides
  RGPRCHD: ECHOHDLR      // The service program name
  RGPRCNM: echo_secure   // The case sensitive name of exported function
                         // for the secure session

  To use the non secure exported function, call the
  clarad daemon as follow:

    call clarad parm('41101' '5' '10' '/QSYS.LIB/MYLIB.LIB/CLARAHS.PGM'
                     'ECHO')

  To use the secure exported function, call the
  clarad daemon as follow:

    call clarad parm('41101' '5' '10' '/QSYS.LIB/MYLIB.LIB/CLARAHS.PGM'
                     'ECHOSEC')



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

#include <except.h>
#include <stdio.h>
#include <stdlib.h>

#include "qcsrc/cfsapi.h"
#include "qcsrc/csstr.h"

#define BUFF_MAX 1025

int stream_fd;

CSRESULT
  echo
    (int conn_fd,
     char* szClientInfo,
     char* szReserved,
     void* data)
{
  int i;
  int rc;
  int type;
  int quit;

  char szMessage[BUFF_MAX+256];
  char szResponse[BUFF_MAX+1];

  uint64_t size;

  CSRESULT hResult;

  CFS_INSTANCE pInstance;

  CSSTRCV cvtString;


  // String Conversion object
  cvtString = CSSTRCV_Constructor();

  pInstance = CFS_OpenChannel(conn_fd, 0, 0);

  do {

    size = 255;
    hResult = CFS_Read(pInstance,
                       szMessage,
                       &size,
                       -1);

    if (CS_SUCCEED(hResult) &&
        CS_DIAG(hResult) != CFS_DIAG_CONNCLOSE) {

      ////////////////////////////////////////////////////////////
      // echo back:
      // this is just for demonstration purposes.
      // In this example, we assume the client
      // sends ASCII ... we will convert to EBCDIC
      // and then back to ASCII to send over to client.
      ////////////////////////////////////////////////////////////

      szMessage[size] = 0;

      // Start a conversion from UTF8 to the job CCSID
      CSSTRCV_SetConversion(cvtString, "00819", "00000");

      CSSTRCV_StrCpy(cvtString, szMessage, strlen(szMessage));

      size = CSSTRCV_Size(cvtString);

      // We know that szClientByte is large enough ...
      CSSTRCV_Get(cvtString, szResponse);

      // NOTICE: The conversion does not place
      // a NULL at the end of the string!!!

      szResponse[size] = 0;

      ////////////////////////////////////////////////////////////
      // Just to give a way for the client to
      // disconnect from this handler, if the first
      // character is the letter q, then we leave
      // the loop.
      ////////////////////////////////////////////////////////////

      quit = 0;

      if (szResponse[0] == 'q') {
        quit = 1;
        strcpy(szMessage, "ECHO HANDLER: GOODBYE :-)");
      }
      else {
        strcpy(szMessage, "ECHO HANDLER: ");
        strcat(szMessage, szResponse);
      }

      ////////////////////////////////////////////////////////////
      // Convert the whole response to ASCII
      ////////////////////////////////////////////////////////////

      CSSTRCV_SetConversion(cvtString, "00000", "00819");

      CSSTRCV_StrCpy(cvtString, szMessage, strlen(szMessage));

      // retrieve the converted string
      size = CSSTRCV_Size(cvtString);

      CSSTRCV_Get(cvtString, szResponse);

      // Send respopnse to client
      hResult = CFS_Write(pInstance,
                          szResponse,
                          &size,
                          -1);

      if (quit) {
        break;
      }
    }
    else {

      // Either an error or a connection close;
      // leave the reading loop and wait for
      // next client connection.
      break;
    }
  }
  while (CS_SUCCEED(hResult));

  CFS_Close(pInstance);
  close(conn_fd);

  CSSTRCV_Destructor(&cvtString);

  return CS_SUCCESS;

}

CSRESULT
  echo_secure
    (int conn_fd,
     char* szClientInfo,
     char* szReserved,
     void* data)
{
  int i;
  int rc;
  int type;
  int quit;

  char szMessage[BUFF_MAX+256];
  char szResponse[BUFF_MAX+1];

  // IBM i FTP server application ID; an application ID
  // associates resolves to an SSL certificate, so we
  // use any the SSL certificate; we use the certificate
  // that the FTP server also uses.

  char* appID = "QIBM_QTMF_FTP_SERVER";

  int iSSLResult;

  uint64_t size;

  CSRESULT hResult;

  CFS_INSTANCE pInstance;

  CSSTRCV cvtString;

  CFS_SERVERSESSION_100 sessionInfo;

  // String Conversion object
  cvtString = CSSTRCV_Constructor();

  memset(&sessionInfo, 0, sizeof(CFS_SERVERSESSION_100));

  // Prepare to connect; we use FTP SSL certificate (application id)
  sessionInfo.szApplicationID = appID;

  pInstance = CFS_SecureOpenChannel
                (conn_fd,
                 (void*)(&sessionInfo),
                 CFS_SERVERSESSION_FMT_100, &iSSLResult);

  do {

    size = 255;
    hResult = CFS_SecureRead(pInstance,
                             szMessage,
                             &size,
                             -1, &iSSLResult);

    if (CS_SUCCEED(hResult) &&
        CS_DIAG(hResult) != CFS_DIAG_CONNCLOSE) {

      ////////////////////////////////////////////////////////////
      // echo back:
      // this is just for demonstration purposes.
      // In this example, we assume the client
      // sends ASCII ... we will convert to EBCDIC
      // and then back to ASCII to send over to client.
      ////////////////////////////////////////////////////////////

      szMessage[size] = 0;

      // Start a conversion from UTF8 to the job CCSID
      CSSTRCV_SetConversion(cvtString, "00819", "00000");

      CSSTRCV_StrCpy(cvtString, szMessage, strlen(szMessage));

      size = CSSTRCV_Size(cvtString);

      // We know that szClientByte is large enough ...
      CSSTRCV_Get(cvtString, szResponse);

      // NOTICE: The conversion does not place
      // a NULL at the end of the string!!!

      szResponse[size] = 0;

      ////////////////////////////////////////////////////////////
      // Just to give a way for the client to
      // disconnect from this handler, if the first
      // character is the letter q, then we leave
      // the loop.
      ////////////////////////////////////////////////////////////

      quit = 0;

      if (szResponse[0] == 'q') {
        quit = 1;
        strcpy(szMessage, "ECHO HANDLER: GOODBYE :-)");
      }
      else {
        strcpy(szMessage, "ECHO HANDLER: ");
        strcat(szMessage, szResponse);
      }

      ////////////////////////////////////////////////////////////
      // Convert the whole response to ASCII
      ////////////////////////////////////////////////////////////

      CSSTRCV_SetConversion(cvtString, "00000", "00819");

      CSSTRCV_StrCpy(cvtString, szMessage, strlen(szMessage));

      // retrieve the converted string
      size = CSSTRCV_Size(cvtString);

      CSSTRCV_Get(cvtString, szResponse);

      // Send respopnse to client
      hResult = CFS_SecureWrite(pInstance,
                                szResponse,
                                &size,
                                -1, &iSSLResult);

      if (quit) {
        break;
      }
    }
    else {

      // Either an error or a connection close;
      // leave the reading loop and wait for
      // next client connection.
      break;
    }
  }
  while (CS_SUCCEED(hResult));

  CFS_SecureClose(pInstance);
  close(conn_fd);

  CSSTRCV_Destructor(&cvtString);

  return CS_SUCCESS;

}

