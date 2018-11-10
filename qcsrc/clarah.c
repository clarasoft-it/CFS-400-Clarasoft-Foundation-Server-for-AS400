/* ==========================================================================
  Clarasoft Foundation Server 400
  clarah.c
  Clara daemon handler
  Version 1.0.0
  
  Command line arguments
     - protocol (service) implementation name
     
  Compile module with:
    CRTSQLCI OBJ(CLARAH) SRCFILE(QCSRC)
         SRCMBR(CLARAH) DBGVIEW(*SOURCE)
         
  Build program with:
    CRTPGM PGM(CLARAH) MODULE(CLARAH) BNDSRVPGM(CFSAPI)
    
  To use this handler, call the clarad daemon as follow:
    call clarad parm('MY_SERVICE_INSTANCE')
    
  The parameter is the key into the CFSCONF file, which
  will hold the name of this handler (CLARAH).
  
  When clarad spawns clarah, the parameter string is passed
  to clarah, which will in turn read the CFSREG file under the
  parameter key; there, it will find the service program,
  and procedure name to bind and call as the service's implementation
  when a client connects. 
  
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
#include <sys/socket.h>
#include <unistd.h>

#define BUFF_MAX 1025

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
char szApplicationID[129];

EXEC SQL END DECLARE SECTION;

int stream_fd;
CFS_SERVICEINFO_100 serviceInfoStruct;

int main (int argc, char **argv)
{
  int i;
  int rc;
  int type;
  int quit;

  char buffer = 0; // dummy byte character
                   // to send to main daemon
				
  uint64_t size;

  CSRESULT hResult;

  _SYSPTR pSrvPgm;  // OS400 System pointer

  CFS_PROTOCOLHANDLERPROC InprocHandler; // Pointer to handler function

  CFS_INSTANCE pInstance;

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
  // identifying the service to load if it is to be linked at runtime
  // by activating a service program and getting the address of an
  // exported sub-procedure that implements the service. This identifier
  // is the key from which to retrieve the activation information in the
  // CFSREG file.
  //
  // If the main daemon sends the value *NOLINK, then this means this
  // handler implements the service and no service program needs to be
  // activated.
  ///////////////////////////////////////////////////////////////////////////

  if (argc < 2) {
    exit(1);
  }

  ////////////////////////////////////////////////////////////////////////
  // This means this handler must activate a service program and
  // call an exported sub-procedure that implements the required service.
  ////////////////////////////////////////////////////////////////////////

  memcpy(szServiceName, argv[1], strlen(argv[1]) + 1); // include NULL

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
        :szApplicationID
    FROM
        CFSREG
    WHERE
        RGSRVNM = :szServiceName;

  if (SQLCODE != 0) {
    fprintf(stderr, "SQL Error"); fflush(stderr);
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

  i=0;
  while(szApplicationID[i] != ' ' && i<128)
                      i++;

  szApplicationID[i] = 0;

  pSrvPgm = rslvsp(WLI_SRVPGM, szSrvPgmName, szLibraryName, _AUTH_NONE);
  QleActBndPgm(&pSrvPgm, NULL, NULL, NULL, NULL);
  type = 0;
  InprocHandler = 0;
  QleGetExp(NULL, NULL, NULL, szInProcHandler,
            (void **)&InprocHandler, &type, NULL);

  if (InprocHandler) {

     serviceInfoStruct.serviceInfoFmt = CFS_SERVICEINFO_FMT_100;
     serviceInfoStruct.szServiceName = szServiceName;
     serviceInfoStruct.szApplicationID = szApplicationID;


     for (;;) {

        //////////////////////////////////////////////////////////////////
        // Signal the main daemon we are ready to service a client
        //////////////////////////////////////////////////////////////////

        send(stream_fd, &buffer, 1, 0);

        //////////////////////////////////////////////////////////////////
        // The main server will eventually hand over the connection socket
        // needed to communicate with a client via the IPC descriptor
        //////////////////////////////////////////////////////////////////

        hResult = CFS_ReceiveDescriptor(stream_fd,
                                        &serviceInfoStruct.conn_fd, -1);

        if (CS_SUCCEED(hResult)) {

           // Adjust parameter values as needed
           InprocHandler((void*)&serviceInfoStruct);
           close(serviceInfoStruct.conn_fd);
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
   close(serviceInfoStruct.conn_fd);
}
