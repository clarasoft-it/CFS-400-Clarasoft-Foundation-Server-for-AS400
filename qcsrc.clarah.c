/* ==========================================================================
  Clarasoft Foundation Server 400
  clarah.c
  Clara daemon handler job
  Version 1.0.0

  Compile module with:
     CRTSQLCI OBJ(CLARAH) SRCFILE(QCSRC)
              SRCMBR(CLARAH) DBGVIEW(*SOURCE)

  Build program with:
     CRTPGM PGM(CLARAH) MODULE(CLARAH) BNDSRVPGM(CFSAPI)

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
#include <miptrnam.h>
#include <stdio.h>
#include <stdlib.h>

#include "qcsrc/cfsapi.h"
#include "qcsrc/csstr.h"
#include <QLEAWI.h>

#define BUFF_MAX 1025

#define CFS_DEF_SERVICENAME_MAX  (65)
#define CFS_DEF_LIBNAME_MAX      (11)
#define CFS_DEF_OBJNAME_MAX      (11)

void
  Cleanup
    (_CNL_Hndlr_Parms_T* args);

volatile unsigned return_code;

int conn_fd;
int stream_fd;

int main (int argc, char **argv)
{
  int i;
  int rc;
  int type;
  int quit;

  char buffer = 0; // dummy byte character
                   // to send to main daemon

  char szMessage[BUFF_MAX+256];
  char szResponse[BUFF_MAX+1];

  uint64_t size;

  CSRESULT hResult;

  CFS_PROTOCOLHANDLERPROC CFS_Handler;  // Pointer to handler function

  CFS_INSTANCE pInstance;

  CSSTRCV cvtString;

  /* ------------------------------------------------------------------------
   This code is to register a cleanup handler
   for when the main server job is cancelled. This is not
   necessary but is proper i5 OS practice for servers.
   The #pragma directive must be coupled with another at some later point
   in the main() function; see code just before return statement in main().
  ------------------------------------------------------------------------ */

  return_code = 0;
  #pragma cancel_handler( Cleanup, return_code )

  ///////////////////////////////////////////////////////////////////////////
  // This is the file handle passed from the main daemon
  // that represents the handler's end of the stream
  // pipe created by the main server through socketpair().
  // We set it to zero because it has been set as the first
  // (zero-based) file descriptor in the set of file
  // descriptors in the call to the spawn API.
  ///////////////////////////////////////////////////////////////////////////

  stream_fd = 0;

  send(stream_fd, &buffer, 1, 0);

  ////////////////////////////////////////////////////////////////////////
  // This means this handler implements the required service.
  ////////////////////////////////////////////////////////////////////////

  // String Conversion object
  cvtString = CSSTRCV_Constructor();

  for (;;) {

     /////////////////////////////////////////////////////////////////////
     // The main server will eventually hand over the connection socket
     // needed to communicate with a client via the IPC descriptor
     /////////////////////////////////////////////////////////////////////

     hResult = CFS_ReceiveDescriptor(stream_fd, &conn_fd, -1);

     if (CS_SUCCEED(hResult)) {

        //////////////////////////////////////////////////////////////////
        // ECHO handler using CFSAPI non-secure functions
        //////////////////////////////////////////////////////////////////

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

              // Start a conversion from ASCII to the job CCSID
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

        //////////////////////////////////////////////////////////////////
        // Tell main daemon we can handle another connection
        //////////////////////////////////////////////////////////////////

        send(stream_fd, &buffer, 1, 0);
     }
  }

  CSSTRCV_Destructor(&cvtString);

  close(stream_fd);

  /* ------------------------------------------------------------------------
    If you have registered a cancel handler
    (see above at the beginning of the main() function).
  ------------------------------------------------------------------------ */

  #pragma disable_handler

  return 0;

}

/* --------------------------------------------------------------------------
 * Cleanup
 * ----------------------------------------------------------------------- */

void Cleanup(_CNL_Hndlr_Parms_T* data) {

   close(stream_fd);
   close(conn_fd);
}

