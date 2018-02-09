/* ==========================================================================
  Clarasoft Foundation Server 400

  clarahs.c
  Clara daemon secure handler job
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

     CRTPGM PGM(CLARAH) MODULE(CLARAH) BNDSRVPGM(CFSAPI)



  To use this handler's static implementation, call the
  clarad daemon as follow:

    call clarad parm('41101' '5' '10' '/QSYS.LIB/MYLIB.LIB/CLARAHS.PGM'
                     '*NOLINK')

  To use this handler's dynamically bound implementation, call the
  clarad daemon as follow:

    call clarad parm('41101' '5' '10' '/QSYS.LIB/MYLIB.LIB/CLARAHS.PGM'
                     'ECHO')

  In the above call, the dynamically bound implementation information
  is stored in the CFSREG file under the ECHO key value field.

  This corresponding record in the CFSREG file might look like this:

  RGSRVNM: ECHOSEC      // Last argument to the clarad daemon
  RGLIBNM: MYLIB        // The library where the service program resides
  RGPRCHD: ECHOHDLR     // The service program name
  RGPRCNM: echo_secure  // The case sensitive name of exported function
                        // for non secure session

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
  int quit;
  int iSSLResult;

  char buffer = 0; // dummy byte character
                   // to send to main daemon

  char szMessage[BUFF_MAX+256];
  char szResponse[BUFF_MAX+1];

  // Application ID can be retrieved externally;
  // this is for demonstartion purposes
  // and we will use the FTP server certifiacte

  char szAppID[] = "QIBM_QTMF_FTP_SERVER";

  uint64_t size;

  CSRESULT hResult;

  _SYSPTR pSrvPgm;  // OS400 System pointer

  CFS_PROTOCOLHANDLERPROC CFS_Handler;  // Pointer to handler function

  CFS_INSTANCE pInstance;

  CFS_SERVERSESSION_100 sessionInfo;

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

           // Application ID can be retrieved externally;
           // this is for demonstartion purposes

           memset(&sessionInfo, 0, sizeof(CFS_SERVERSESSION_100));
           sessionInfo.szApplicationID = szAppID;

           pInstance = CFS_SecureOpenChannel
                          (conn_fd,
                           (void*)(&sessionInfo),
                           CFS_SERVERSESSION_FMT_100,
                           &iSSLResult);

           if (pInstance != NULL) {

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

             //////////////////////////////////////////////////////////////////
             // Tell main daemon we can handle another connection
             //////////////////////////////////////////////////////////////////

             send(stream_fd, &buffer, 1, 0);
           }
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

     pSrvPgm = rslvsp(WLI_SRVPGM, szSrvPgmName, szLibraryName, _AUTH_NONE);
     QleActBndPgm(&pSrvPgm, NULL, NULL, NULL, NULL);
     type = 0;
     CFS_Handler = 0;
     QleGetExp(NULL, NULL, NULL, szInProcHandler,
               (void **)&CFS_Handler, &type, NULL);

     if (CFS_Handler) {

        send(stream_fd, &buffer, 1, 0);

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

  CLARAHS_HANDLER_END:

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


