/* ==========================================================================
  Clarasoft Foundation Server 400
  wsckhs.c
  Web socket server using SSL
  Version 1.0.0
  
  Compile module with:
  
     CRTSQLCI OBJ(WSCKHS) SRCFILE(QCSRC)
              SRCMBR(WSCKHS.C) DBGVIEW(*SOURCE)
              
  Build program with:
  
    CRTPGM PGM(WSCKHS) MODULE(WSCKHS) BNDSRVPGM((CFSAPI))
    
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
#include <QLEAWI.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "qcsrc/cswsck.h"

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
char szAppID[129];

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

  char* outBuffer;
  char* szResponse;
  char* pFragment;

  uint64_t size;
  uint64_t totalBytes;

  CSRESULT hResult;

  CFS_SERVERSESSION_100 sinfo;

  _SYSPTR pSrvPgm;  // OS400 System pointer

  CFS_PROTOCOLHANDLERPROC InprocHandler; // Pointer to handler function

  /* ------------------------------------------------------------------------
   This code is to register a clean up handler
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
  // identifying the service to load.
  ///////////////////////////////////////////////////////////////////////////

  if (argc < 2) {
    exit(1);
  }

  ///////////////////////////////////////////////////////////////////////////
  // Retrieve service implementation (service program and exported
  // function to call).
  ///////////////////////////////////////////////////////////////////////////

  strncpy(szServiceName, argv[1], 64);

  EXEC SQL
    SELECT
      RGLIBNM,
      RGPRCHD,
      RGPRCNM,
      RGAPPID
      INTO
      :szLibraryName,
      :szSrvPgmName,
      :szInProcHandler,
      :szAppID
    FROM
      CFSREG
    WHERE
      RGSRVNM = :szServiceName;

  if (SQLCODE != 0) {
    exit(2);
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

  i=0;
  while(szAppID[i] != ' ' && i<255)
                      i++;

  szAppID[i] = 0;

  pSrvPgm = rslvsp(WLI_SRVPGM, szSrvPgmName,
                           szLibraryName, _AUTH_NONE);

  QleActBndPgm(&pSrvPgm, NULL, NULL, NULL, NULL);
  type = 0;
  InprocHandler = 0;

  QleGetExp(NULL, NULL, NULL, szInProcHandler,
            (void **)&InprocHandler, &type, NULL);

  if (InprocHandler) {

    ////////////////////////////////////////////////////////////////////////
    // Set SSL application ID; use "QIBM_QTMF_FTP_SERVER" if needed.
    ////////////////////////////////////////////////////////////////////////

    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.szApplicationID = szAppID;

    for (;;) {

      //////////////////////////////////////////////////////////////////
      // Tell main daemon we can handle a client connection
      //////////////////////////////////////////////////////////////////

      send(stream_fd, &buffer, 1, 0);

      /////////////////////////////////////////////////////////////////////
      // The main server will eventually hand over the connection socket
      // needed to communicate with a client via the IPC descriptor
      /////////////////////////////////////////////////////////////////////

      hResult = CFS_ReceiveDescriptor(stream_fd, &conn_fd, -1);

      if (CS_SUCCEED(hResult)) {

        if (conn_fd >= 0) {

          pCONN = CSWSCK_SecureOpenChannel(conn_fd, (void*)&sinfo,
                                           CFS_SERVERSESSION_FMT_100);

          if (pCONN != 0) {

            //////////////////////////////////////////////////////////////////
            // Call handler function exported by service program
            //////////////////////////////////////////////////////////////////

            InprocHandler(pCONN);

            //////////////////////////////////////////////////////////////////
            // Client (or service) is done...
            //////////////////////////////////////////////////////////////////

            CSWSCK_SecureClose(pCONN, 0, 0, -1);
            pCONN = 0;
          }

          close(conn_fd);
        }
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

   if (pCONN != 0) {
     CSWSCK_SecureClose(pCONN, 0, 0, -1);
   }

   close(stream_fd);
   close(conn_fd);
}
