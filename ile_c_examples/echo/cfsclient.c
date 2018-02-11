/* ===========================================================================
  Clarasoft Foundation Server for AS400

  cfsclient.c
  Test client for CFS-400
  Version 1.0.0


  Compile module with:

  CRTCMOD MODULE(CFSCLIENT) OUTPUT(*print) DBGVIEW(*ALL)

  Create program with:

  CRTPGM PGM(CFSCLIENT) BNDSRVPGM((CFSAPI))

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

#include <inttypes.h>
#include "qcsrc/cscore.h"
#include "qcsrc/cfsapi.h"
#include "qcsrc/csstr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


uint64_t EbcdicToAscii(CSSTRCV* conv, char* from, char* to);
uint64_t AsciiToEbcdic(CSSTRCV* conv, char* from, char* to);

int main(int argc, char** argv) {

  char szHost[256];
  char szPort[12];
  char szData[256];

  // IBM i FTP server application ID; an application ID
  // associates resolves to an SSL certificate, so we
  // use any the SSL certificate; we use the certificate
  // that the FTP server also uses.

  char* appID = "QIBM_QTMF_FTP_SERVER";

  int iSSLResult;

  CSRESULT rc;

  CFS_CLIENTSESSION_100 sessionInfo;

  CFS_INSTANCE pInstance;

  CSSTRCV cvtString;

  uint64_t bodySize;
  uint64_t size;

  cvtString = CSSTRCV_Constructor();

  memset(&sessionInfo, 0, sizeof(CFS_CLIENTSESSION_100));

  // Prepare to connect; we use FTP SSL certificate (application id)
  sessionInfo.szApplicationID = appID;
  sessionInfo.port = 41101;
  strcpy(szHost, "dvuap001.uapinc.com");
  sessionInfo.szHostName = szHost;
  sessionInfo.connTimeout = 10;


  if (argv[1][0] == 's') {

    ////////////////////////////////////////////////////////
    // We want to have a secure session with the server
    ////////////////////////////////////////////////////////

    // connect to server
    pInstance = CFS_SecureConnect(
                  (void*)(&sessionInfo),
                  CFS_CLIENTSESSION_FMT_100,
                  &iSSLResult);

    if (pInstance != NULL) {

      strcpy(szData, "Hello World!");
      size = EbcdicToAscii(cvtString, szData, szData);

      rc = CFS_SecureWrite
             (pInstance,
              szData,
              &size,
              -1,
              &iSSLResult);

      if (CS_SUCCEED(rc)) {

        // Read echo server response

        size = 255;
        rc = CFS_SecureRead
               (pInstance,
                szData,
                &size,
                -1,
                &iSSLResult);

        szData[size] = 0;
        size = AsciiToEbcdic(cvtString, szData, szData);

        // Send quit signal to server

        // conversion from the job CCSID to ASCII
        strcpy(szData, "q");
        size = EbcdicToAscii(cvtString, szData, szData);

        rc = CFS_SecureWrite
               (pInstance,
                szData,
                &size,
                -1,
                &iSSLResult);

        size = 255;
        rc = CFS_SecureRead
               (pInstance,
                szData,
                &size,
                -1,
                &iSSLResult);

        szData[size] = 0;
        size = AsciiToEbcdic(cvtString, szData, szData);
      }

      CFS_SecureClose(pInstance);
    }
  }
  else {

    ////////////////////////////////////////////////////////
    // We want to have a non-secure session with the server
    ////////////////////////////////////////////////////////

    // connect to server
    pInstance = CFS_Connect(
                  (void*)(&sessionInfo),
                  CFS_CLIENTSESSION_FMT_100);

    if (pInstance != NULL) {

      strcpy(szData, "Hello World!");
      size = EbcdicToAscii(cvtString, szData, szData);

      rc = CFS_Write
             (pInstance,
              szData,
              &size,
              -1);

      if (CS_SUCCEED(rc)) {

        // Read echo server response

        size = 255;
        rc = CFS_Read
               (pInstance,
                szData,
                &size,
                -1);

        szData[size] = 0;
        size = AsciiToEbcdic(cvtString, szData, szData);

        // Send quit signal to server

        // conversion from the job CCSID to ASCII
        strcpy(szData, "q");
        size = EbcdicToAscii(cvtString, szData, szData);

        rc = CFS_Write
               (pInstance,
                szData,
                &size,
                -1);

        size = 255;
        rc = CFS_Read
               (pInstance,
                szData,
                &size,
                -1);

        szData[size] = 0;
        size = AsciiToEbcdic(cvtString, szData, szData);
      }

      CFS_Close(pInstance);
    }
  }

  return 0;
}

uint64_t EbcdicToAscii(CSSTRCV* conv, char* from, char* to) {

  uint64_t size;

  // conversion from the job CCSID to ASCII
  CSSTRCV_SetConversion(conv, "00000", "00819");
  CSSTRCV_StrCpy(conv, from,  strlen(from));
  // retrieve the converted string
  size = CSSTRCV_Size(conv);
  CSSTRCV_Get(conv, to);

  return size;
}

uint64_t AsciiToEbcdic(CSSTRCV* conv, char* from, char* to) {

  uint64_t size;

  // conversion from the job CCSID to ASCII
  CSSTRCV_SetConversion(conv, "00819", "00000");
  CSSTRCV_StrCpy(conv, from,  strlen(from));
  // retrieve the converted string
  size = CSSTRCV_Size(conv);
  CSSTRCV_Get(conv, to);

  return size;
}



