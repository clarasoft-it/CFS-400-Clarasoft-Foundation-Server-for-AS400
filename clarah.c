
/* ==========================================================================
  Clarasoft Foundation Server 400
  
  clarah.c
  Clara daemon handler job
  Version 1.0.0
  
  
  Command line arguments
  
     - protocol implementation name
	 
     If argument is *NOLINK, then this handler implements
     the protocol implementation. Any other value identifies
     an implementation exported by a service program.
	
	
  Compile module with:
  
     CRTSQLCI OBJ(CLARAH) SRCFILE(QCSRC)
              SRCMBR(CLARAH) DBGVIEW(*SOURCE)
			  
  Build program with:
  
     CRTPGM PGM(CLARAH) MODULE(CLARAH CFSAPI CSLIST CSSTR)
	 
	 
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

int main (int argc, char **argv)
{
  int i;
  int rc;
  int type;

  char buffer = 0; // dummy byte character
                   // to send to main daemon

  char szMessage[BUFF_MAX+1];
  char szClientByte[BUFF_MAX+1];

  char* outBuffer;

  uint64_t size;

  CSRESULT hResult;

  _SYSPTR pSrvPgm;  // OS400 System pointer

  CFS_PROTOCOLHANDLERPROC CFS_Handler;  // Pointer to handler function

  CFS_INSTANCE* pInstance;

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

           pInstance = CFS_Open(conn_fd,
                                0,
                                CFS_SESSIONTYPE_SERVER,
                                0,
                                0);

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

           //////////////////////////////////////////////////////////////////
           // ECHO handler using basic socket functions
           // (with no conversion of incoming and outgoing data: see above
           // example for converting data from UTF8 to EBCDIC and
           // vice-versa).
           //////////////////////////////////////////////////////////////////

           /*
           do
           {
              // Get client character
              rc = recv(conn_fd,
                        szMessage,
                        BUFF_MAX,
                        0);
              if (rc > 0) {
                 // Just to give a way for the client to
                 // disconnect from this handler, if the
                 // character is the letter q, then we leave
                 // the loop... recall that AS400 is EBCDIC
                 // so we will check against the ASCII code for
                 // the letter q because for this example,
                 // we will assume the client is ASCII based.
                 if (szMessage[0] == 113) // ASCII code for letter q
                    break;
                 // echo it back
                 rc = send(conn_fd,
                           szMessage,
                           BUFF_MAX,
                           0);
              }
           }
           while (rc > 0);
           close(conn_fd);
           */

           //////////////////////////////////////////////////////////////////
           // Tell main daemon we can handle another connection
           //////////////////////////////////////////////////////////////////

           send(stream_fd, &buffer, 1, 0);
        }
     }

     CSSTRCV_Destructor(&cvtString);

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
