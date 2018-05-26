/* ==========================================================================
  Clarasoft Foundation Server 400
  wsclient.c
  Websocket client demo program
  Version 1.0.0

  Compile module with:

  CRTCMOD MODULE(WSCLIENT) SRCFILE(QCSRC) SRCMBR(WSCLIENT.C)
      DBGVIEW(*ALL)

  Build program with:
     CRTPGM PGM(WSCLIENT) MODULE(WSCLEINT) BNDSRVPGM(CFSAPI)

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

   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>
   #include "qcsrc/cscore.h"
   #include "qcsrc/csstr.h"
   #include "qcsrc/cslist.h"
   #include "qcsrc/cfsapi.h"
   #include "qcsrc/cswsck.h"


   int main(int argc, char** argv) {

      char szHost[256];
      char szPort[12];
      char szData[256];
      char szResponse[4097];

      CSRESULT hResult;

      uint64_t Size;
      uint64_t Offset;

      CFS_CLIENTSESSION_100 sessionInfo;

      CSWSCK pInstance;

      memset(&sessionInfo, 0, sizeof(CFS_CLIENTSESSION_100));
      sessionInfo.szHostName = argv[1];
      sessionInfo.port = atoi(argv[2]);
      sessionInfo.connTimeout = 10;

      pInstance = CSWSCK_Connect(
                         (void*)(&sessionInfo),
                          CFS_CLIENTSESSION_FMT_100);

      if (pInstance != NULL) {

        strcpy(szData, "Hello World!");
        Size = strlen(szData);
        hResult = CSWSCK_Send(pInstance,
                              CSWSCK_OPER_TEXT,
                              szData,
                              Size,
                              CSWSCK_FIN_ON,
                              20);

        if (CS_SUCCEED(hResult)) {

          Offset = 0;

          do {

            hResult = CSWSCK_Receive(pInstance,
                                     &Size,
                                     20);

            if (CS_FAIL(hResult)) {
              break;
            }

            CSWSCK_GetData(pInstance,
                            szResponse+Offset, 0, Size);
            Offset += Size;

          }
          while (CS_DIAG(hResult) == CSWSCK_MOREDATA);

        }

        CSWSCK_Close(pInstance, 0, 0, -1);
      }

      return 0;
   }



