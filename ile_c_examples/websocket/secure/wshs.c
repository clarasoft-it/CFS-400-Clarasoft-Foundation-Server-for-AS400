
/* ==========================================================================

  Clarasoft Foundation Server 400

  wsh.c
  Web socket secure echo server
  Version 1.0.0


  Command line arguments

     - protocol implementation name

     If argument is *NOLINK, then this handler implements
     the protocol implementation. Any other value identifies
     an implementation exported by a service program.

  To execute via the CLARAD server, call the CLARAD server
  with a comamnd like the following:

  call clarad
  parm('41101' '1' '1' '/QSYS.LIB/SOMELIB.LIB/WSHS.PGM' '*NOLINK')

  or if the echo server is implemented in an exported function:

  call clarad
  parm('41101' '1' '1' '/QSYS.LIB/SOMELIB.LIB/WSHS.PGM' 'SRVNAME')

  In the above, the SRVNAME parameter is the key in the CFSREG
  file that contains the dynamic binding information to activate
  the service program that implements the web socket echo service
  and the sub-procedure to call.

  You can test this server by using the following web client at:

      http://www.websocket.org/echo.html

  Note that this implemntation does not use SSL but the CSWSCK
  package does support SSL and a secure echo server can be
  built by replacing the non secure functions with their
  secure equivalents.

  Compile module with:

     CRTSQLCI OBJ(WSHS) SRCFILE(QCSRC)
              SRCMBR(WSHS) DBGVIEW(*SOURCE)

  Build program with:

    CRTPGM PGM(WSHS) MODULE(WSHS) BNDSRVPGM((CFSAPI))

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
#include "qcsrc/cswsck.h"
#include "qcsrc/cslist.h"


#define BUFF_MAX 256

#define CFS_DEF_SERVICENAME_MAX  (65)
#define CFS_DEF_LIBNAME_MAX      (11)
#define CFS_DEF_OBJNAME_MAX      (11)

void
  Cleanup
    (_CNL_Hndlr_Parms_T* args);

volatile unsigned return_code;

EXEC SQL INCLUDE SQLCA;

EXEC SQL BEGIN DECLARE SECTION;

char szServiceName[65];
char szLibraryName[11];
char szSrvPgmName[11];
char szInProcHandler[256];

EXEC SQL END DECLARE SECTION;

int conn_fd;
int stream_fd;

CSWSCK pCONN;

int main (int argc, char **argv)
{
  int i;
  int rc;
  int type;

  long bytes;
  long Count;

  char buffer = 0; // dummy byte character
                   // to send to main daemon

  char szMessage[BUFF_MAX+1];
  char szClientByte[BUFF_MAX+1];

  char* outBuffer;
  char* szResponse;
  char* pFragment;

  uint64_t size;
  uint64_t totalBytes;

  CSRESULT hResult;
  CSLIST Fragments;

  _SYSPTR pSrvPgm;  // OS400 System pointer

  CFS_PROTOCOLHANDLERPROC CFS_Handler;  // Pointer to handler function

  CFS_INSTANCE* pInstance;

  CSSTRCV cvtString;

  CFS_SERVERSESSION_100 sinfo;

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

  ///////////////////////////////////////////////////////////////////////////
  // The main daemon sends a single program argument which is a string
  // identifying the service to load if it is to be linked at runtime
  // by activating a service program and getting the address of an
  // exported sub-procedure that implements the service. This identifier
  // is the key from which to retrieve the activation information in the
  // CFSREG file.
  //
  // If the main daemon sends the value *NOLINK, then this means this
  // handler implemnts the service and no service program needs to be
  // activated.
  ///////////////////////////////////////////////////////////////////////////

  if (argc < 2) {
    exit(1);
  }

  if (!strcmp(argv[1], "*NOLINK")) {

     ////////////////////////////////////////////////////////////////////////
     // This means this handler implements the required service.
     ////////////////////////////////////////////////////////////////////////

     for (;;) {

        /////////////////////////////////////////////////////////////////////
        // The main server will eventually hand over the connection socket
        // needed to communicate with a client via the IPC descriptor
        /////////////////////////////////////////////////////////////////////

        hResult = CFS_ReceiveDescriptor(stream_fd, &conn_fd, -1);

        if (CS_SUCCEED(hResult)) {

           //////////////////////////////////////////////////////////////////
           // ECHO handler using CSWSCK secure functions:
           // Basic SSL requires an application ID which is set in the
           // session information structure. SSL defaults can be
           // overriden in the session info structure. In this example,
           // the application ID is WSD. We must specify the format of
           // our session info structure so that future releases may
           // add to this structure.
           //////////////////////////////////////////////////////////////////

           memset(&sinfo, 0, sizeof(sinfo));
           sinfo.pszBuffers[CFS_OFFSET_GSK_OS400_APPLICATION_ID] = "WSD";

           pCONN = CSWSCK_SecureOpenChannel(conn_fd,
                                            (void*)&sinfo,
                                            CFS_SERVERSESSION_FMT_100);

           if (pCONN != 0) {

              Fragments = CSLIST_Constructor();
              totalBytes = 0;

              do {

                 hResult = CSWSCK_SecureReceive(pCONN, &size, -1);

                 if (CS_SUCCEED(hResult)) {

                    switch(CSWSCK_OPERATION(hResult)) {

                       case CSWSCK_OPER_TEXT:
                       case CSWSCK_OPER_CONTINUATION:

                          szResponse = (char*)malloc(size * sizeof(char));
                          totalBytes += size;

                          CSWSCK_GetData(pCONN, szResponse, 0, size);

                          if (CS_DIAG(hResult) == CSWSCK_MOREDATA) {

                             sprintf(szMessage,
                                     "<br/>TEXT: Got %lld bytes,"
                                     " (waiting for more), DATA: <br/>",
                                     size);

                             CSLIST_Insert(Fragments, szMessage, strlen(szMessage), CSLIST_BOTTOM);
                             bytes = size;
                             CSLIST_Insert(Fragments, szResponse, bytes, CSLIST_BOTTOM);

                          }
                          else {

                             sprintf(szMessage,
                                     "<br/>TEXT-END: Got %lld bytes, DATA: <br/>",
                                     size);

                             CSLIST_Insert(Fragments, szMessage, strlen(szMessage), CSLIST_BOTTOM);
                             bytes = size;
                             CSLIST_Insert(Fragments, szResponse, bytes, CSLIST_BOTTOM);

                             // Send everything back

                             Count = CSLIST_Count(Fragments);

                             if (Count > 1) {

                                size = CSLIST_ItemSize(Fragments, 0);
                                bytes = sizeof(pFragment);
                                CSLIST_GetDataRef(Fragments, &pFragment, &bytes, 0);

                                CSWSCK_SecureSend(pCONN,
                                                  CSWSCK_OPER_TEXT,
                                                  pFragment,
                                                  size,
                                                  CSWSCK_FIN_OFF,
                                                  -1);

                                for (i=1; i<Count-1; i++) {

                                   size = CSLIST_ItemSize(Fragments, i);
                                   bytes = sizeof(pFragment);
                                   CSLIST_GetDataRef(Fragments, &pFragment, &bytes, i);

                                   CSWSCK_SecureSend(pCONN,
                                                     CSWSCK_OPER_CONTINUATION,
                                                     pFragment,
                                                     size,
                                                     CSWSCK_FIN_OFF,
                                                    -1);
                                }

                                size = CSLIST_ItemSize(Fragments, i);
                                bytes = sizeof(pFragment);
                                CSLIST_GetDataRef(Fragments, &pFragment, &bytes, i);

                                CSWSCK_SecureSend(pCONN,
                                                  CSWSCK_OPER_CONTINUATION,
                                                  pFragment,
                                                  size,
                                                  CSWSCK_FIN_OFF,
                                                  -1);
                             }
                             else {

                                size = CSLIST_ItemSize(Fragments, 0);
                                bytes = sizeof(pFragment);
                                CSLIST_GetDataRef(Fragments, &pFragment, &bytes, 0);

                                CSWSCK_SecureSend(pCONN,
                                                  CSWSCK_OPER_TEXT,
                                                  pFragment,
                                                  size,
                                                  CSWSCK_FIN_OFF,
                                                  -1);
                             }

                             sprintf(szMessage,
                                     "<br/>Total bytes received: %lld: <br/>",
                                     totalBytes);

                             CSWSCK_SecureSend(pCONN,
                                               CSWSCK_OPER_CONTINUATION,
                                               szMessage,
                                               strlen(szMessage),
                                               CSWSCK_FIN_ON,
                                               -1);

                             CSLIST_Clear(Fragments);
                          }

                          free(szResponse);

                          break;

                       case CSWSCK_OPER_CLOSE:

                          CSWSCK_SecureClose(pCONN, 0, 0, -1);
                          hResult = CS_FAILURE; // to leave the loop
                          break;
                    }
                 }
              }
              while (CS_SUCCEED(hResult));

              CSLIST_Destructor(&Fragments);
           }

           close(conn_fd);

           //////////////////////////////////////////////////////////////////
           // Tell main daemon we can handle another connection
           //////////////////////////////////////////////////////////////////

           send(stream_fd, &buffer, 1, 0);
        }
     }
  }
  else {

     ////////////////////////////////////////////////////////////////////////
     // This means this handler must activate a service program and
     // call an exported sub-procedure that implements the required service.
     ////////////////////////////////////////////////////////////////////////

     memcpy(szServiceName, argv[1], strlen(argv[1]) + 1); // include NULL

     EXEC SQL
       SELECT
           RGLIBNM,
           RGPRCHD,
           RGPRCNM
         INTO
           :szLibraryName,
           :szSrvPgmName,
           :szInProcHandler
       FROM
           CFSREG
       WHERE
           RGSRVNM = :szServiceName;

     if (SQLCODE != 0) {
       // protocol implementation not found
       exit(1);
     }

     ////////////////////////////////////////////////////////////////////////
     // Activate service program and get handler sub-procedure address.
     ////////////////////////////////////////////////////////////////////////

     // We must tediously null terminate the field values.

     i=0;
     while(szSrvPgmName[i] != ' ' && i<10)
                         i++;

     szSrvPgmName[i] = 0;

     i=0;
     while(szLibraryName[i] != ' ' && i<10)
                         i++;

     szLibraryName[i] = 0;

     i=0;
     while(szInProcHandler[i] != ' ' && i<255)
                         i++;

     szInProcHandler[i] = 0;

     pSrvPgm = rslvsp(WLI_SRVPGM, szSrvPgmName, szLibraryName, _AUTH_NONE);
     QleActBndPgm(&pSrvPgm, NULL, NULL, NULL, NULL);
     type = 0;
     CFS_Handler = 0;
     QleGetExp(NULL, NULL, NULL, szInProcHandler,
               (void **)&CFS_Handler, &type, NULL);

     if (CFS_Handler) {

        for (;;) {

           //////////////////////////////////////////////////////////////////
           // The main server will eventually hand over the connection socket
           // needed to communicate with a client via the IPC descriptor
           //////////////////////////////////////////////////////////////////

           hResult = CFS_ReceiveDescriptor(stream_fd, &conn_fd, -1);

           if (CS_SUCCEED(hResult)) {

              // Adjust parameter values as needed
              CFS_Handler(conn_fd, 0, 0, 0);
              close(conn_fd);
           }

           //////////////////////////////////////////////////////////////////
           // Tell main daemon we can handle another connection
           //////////////////////////////////////////////////////////////////

           send(stream_fd, &buffer, 1, 0);
        }
     }
  }

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


