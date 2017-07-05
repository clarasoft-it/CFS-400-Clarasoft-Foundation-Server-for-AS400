
/* ==========================================================================

  Clarasoft Foundation Server 400

  echo.c
  Echo service handler function exported by service program and
  linked at runtime by a clara-handler.
  Version 1.0.0



  Compile module with:

     CRTCMOD MODULE(ECHO) SRCFILE(QCSRC) DBGVIEW(*ALL)

  Build service program with:

     CRTSRVPGM SRVPGM(ECHO) MODULE(ECHO)
        EXPORT(*ALL) BNDSRVPGM((CFSAPI))

  To execute from CLARAD,

    1) Insert the following record in CFSREG:

        RGSRVNM: ECHO
        RGLIBNM: <the library name where the ECHO service pgm resides>
        RGPRCHD: ECHO or <the name of the service program
                          compiled from this source>
        RGPRCNM: echoProc

    2) Execute the CLARAD daemon with the following command:

       call clarad parm('41101' '3' '4'
                        '/QSYS.LIB/LIBANME.LIB/CLARAH.PGM' 'ECHO')


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
#include <stdlib.h>

#include "qcsrc/cscore.h"
#include "qcsrc/cfsapi.h"
#include "qcsrc/cslist.h"
#include "qcsrc/csstr.h"

#define BUFF_MAX 256

CSRESULT echoProc(int fd, char* szIP, char* szPORT, void* data);

CSRESULT echoProc(int fd, char* szIP, char* szPORT, void* data) {

   uint64_t size;
   CSRESULT hResult;

   char szMessage[BUFF_MAX+1];
   char szClientByte[BUFF_MAX+1];

   char* outBuffer;

   CFS_INSTANCE* pInstance;

   CSSTRCV cvtString;

   // String Conversion object
   cvtString = CSSTRCV_Constructor();

   pInstance = CFS_OpenChannel(fd, 0, 0);

   do {

      size = BUFF_MAX;
      hResult = CFS_Read(pInstance,
                         szMessage,
                         &size,
                         -1);

      if (CS_SUCCEED(hResult)) {

          ////////////////////////////////////////////////////////////
          // echo back:
          // this is just for demonstration purposes.
          // In this example, we assume the client
          // sends UTF 8 ...
          // will receive an ASCII byte which we will
          // convert to EBCDIC. Then, we reconvert it back
          // to UTF8 ...
          ////////////////////////////////////////////////////////////

          szClientByte[0] = szMessage[0];
          szClientByte[1] = 0;

          // Start a conversion from UTF8 to the job CCSID
          CSSTRCV_SetConversion(cvtString, CCSID_UTF8, CCSID_JOBDEFAULT);

          CSSTRCV_StrCpy(cvtString, szClientByte, strlen(szClientByte));

          size = CSSTRCV_Size(cvtString);

          // We know that szClientByte is large enough ...
          CSSTRCV_Get(cvtString, szClientByte);

          // null terminate
          szClientByte[size] = 0;






          ////////////////////////////////////////////////////////////
          // Just to give a way for the client to
          // disconnect from this handler, if the
          // character is the letter q, then we leave
          // the loop.
          ////////////////////////////////////////////////////////////

          if (szClientByte[0] == 'q')
               break;





          ////////////////////////////////////////////////////////////
          // Convert the whole response to UTF8
          ////////////////////////////////////////////////////////////

          strcpy(szMessage, "ECHO HANDLER: ");
          strcat(szMessage, szClientByte);

          // Start a conversion from the job CCSID to UTF8
          CSSTRCV_SetConversion(cvtString, CCSID_JOBDEFAULT, CCSID_UTF8);

          CSSTRCV_StrCpy(cvtString, szMessage, strlen(szMessage));

          // retrieve the converted string
          size = CSSTRCV_Size(cvtString);

          outBuffer = (char*)malloc(size * sizeof(char));

          CSSTRCV_Get(cvtString, outBuffer);

          // Send respopnse to client
          hResult = CFS_Write(pInstance,
                              outBuffer,
                              &size,
                              -1);

          free(outBuffer);
      }
   }
   while (CS_SUCCEED(hResult));

   CFS_Close(pInstance);

   return CS_SUCCESS;
}

