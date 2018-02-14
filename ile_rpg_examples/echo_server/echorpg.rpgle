
      /////////////////////////////////////////////////////////////////////////////////
      //
      //    Clarasoft Foundation Server for AS400
      //
      //    ECHORPG
      //    Demonstration echo service called dynamically by clara daemon handler
      //    Version 1.0.0
      //
      //    Compile module with:
      //
      //       CRTRPGMOD MODULE(ECHORPG) SRCFILE(QRPGLESRC)
      //                 SRCMBR(ECHORPG) DBGVIEW(*ALL)
      //
      //    Build this as a service program program with:
      //
      //       CRTSRVPGM SRVPGM(ECHORPG) MODULE(ECHORPG)
      //       BNDSRVPGM(CFSAPI) EXPORT(*ALL)
      //
      //    This service program will be loaded by the CLARAH handler (or CLARAHS)...
      //    To use with the CLARAH handler, do the following:
      //
      //      1) Insert the following in the CFSREG file:
      //
      //         First record:
      //
      //         RGSRVNM: ECHORPG
      //         RGLIBNM: <the library name where the ECHO service pgm resides>
      //         RGPRCHD: ECHORPG or <the name of the service program
      //                          compiled from this source>
      //         RGPRCNM: ECHOHANDLER
      //
      //         Second record:
      //
      //         RGSRVNM: ECHORPGSEC
      //         RGLIBNM: <the library name where the ECHO service pgm resides>
      //         RGPRCHD: ECHORPG or <the name of the service program
      //                          compiled from this source>
      //         RGPRCNM: ECHOHANDLERSEC
      //
      //      2) Execute the CLARAD daemon with the following command
      //         if you want to connect to the non secure handler:
      //
      //           call clarad parm('41101' '3' '4'
      //                   '/QSYS.LIB/LIBANME.LIB/CLARAH.PGM' 'ECHORPG')
      //
      //           Or the following if you want the secure (SSL) handler
      //
      //           call clarad parm('41101' '3' '4'
      //                   '/QSYS.LIB/LIBANME.LIB/CLARAH.PGM' 'ECHORPGSEC')
      //
      //
      //     Distributed under the MIT license
      //
      //     Copyright (c) 2013 Clarasoft I.T. Solutions Inc.
      //
      //     Permission is hereby granted, free of charge, to any person obtaining
      //     a copy of this software and associated documentation files
      //     (the "Software"), to deal in the Software without restriction,
      //     including without limitation the rights to use, copy, modify,
      //     merge, publish, distribute, sublicense, and/or sell
      //     copies of the Software, and to permit persons to whom the Software is
      //     furnished to do so, subject to the following conditions:
      //     The above copyright notice and this permission notice shall be
      //     included in all copies or substantial portions of the Software.
      //     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
      //     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
      //     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
      //     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
      //     ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
      //     TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
      //     THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
      //
      /////////////////////////////////////////////////////////////////////////////////


     H NOMAIN

      /Copy cfsapih

      *----------------------------------------------------------------------
      * ECHOHANDLER: non secure CFS echo server handler
      *----------------------------------------------------------------------

     PECHOHANDLER...
     P                 B                   EXPORT
     D                 PI            10I 0
     D@Conn                          10I 0 Value
     D@Address                         *   Value
     D@Port                            *   Value
     D@Data                            *   Value

     DpCONN            S               *
     DFromCCSID        S              6A
     DToCCSID          S              6A
     DhResult          S             10U 0
     Dquit             S             10I 0
     DSize             S             20U 0
     DszMessage        S            256A
     DszResponse       S            256A
     Dconverter        S               *


     DSessionDS        DS                  LikeDS(SERVERSESSIONINFO_100)

      /Free

          Clear SessionDS;

          pCONN = CFS_OpenChannel(@Conn:
                                  %Addr(SessionDS):
                                  CFS_SERVERSESSION_FMT_100);

          If (pCONN <> *Null);

            converter = CSSTRCV_Constructor();

            hResult = CS_SUCCESS;
            Dou %bitand(hResult: CS_MASK_ERROR) <> CS_SUCCESS;

              Size = 255;
              szMessage = *Blanks;
              hResult = CFS_Read(pCONN:
                                 %Addr(szMessage):
                                 %Addr(Size):
                                 -1);

              If  %bitand(hResult: CS_MASK_ERROR) = CS_SUCCESS And
                  %bitand(hResult: CS_MASK_DIAG) <> CFS_DIAG_CONNCLOSE;

                ////////////////////////////////////////////////////////////
                // echo back:
                // this is just for demonstration purposes.
                // In this example, we assume the client
                // sends ASCII ... we will convert to EBCDIC
                // and then back to ASCII to send over to client.
                ////////////////////////////////////////////////////////////

                // null-terminate

                // Start a conversion from ASCII to the job CCSID
                FromCCSID = '00819' + x'00';
                ToCCSID = '00000' + x'00';
                CSSTRCV_SetConversion(converter:
                                      %Addr(FromCCSID): %Addr(ToCCSID));


                CSSTRCV_StrCpy(converter: %Addr(szMessage): Size);

                size = CSSTRCV_Size(converter);

                // We know that szClientByte is large enough ...
                szResponse = *Blanks;
                CSSTRCV_Get(converter: %Addr(szResponse));

                ////////////////////////////////////////////////////////////
                // Just to give a way for the client to
                // disconnect from this handler, if the first
                // character is the letter q, then we leave
                // the loop.
                ////////////////////////////////////////////////////////////

                quit = 0;

                szMessage = *Blanks;

                if %Trim(szResponse) = 'q';

                  quit = 1;
                  szMessage = 'ECHO HANDLER: GOODBYE :-)';

                Else;

                  szMessage = 'ECHO HANDLER: ' + %Trim(szResponse);

                EndIf;

                FromCCSID = '00000' + x'00';
                ToCCSID = '00819' + x'00';
                CSSTRCV_SetConversion(converter:
                                      %Addr(FromCCSID): %Addr(ToCCSID));

                CSSTRCV_StrCpy(converter:
                               %Addr(szMessage):
                               %Len(%Trim(szMessage)));

                // retrieve the converted string
                size = CSSTRCV_Size(converter);

                szResponse = *Blanks;
                CSSTRCV_Get(converter: %Addr(szResponse));

                // Send respopnse to client
                hResult = CFS_Write(pCONN:
                                    %Addr(szResponse):
                                    %Addr(size):
                                    -1);

                if quit = 1;
                  Leave;
                EndIf;
              EndIf;

            EndDo;

            CFS_Close(pCONN);

          EndIf;

          Return 0;

      /End-Free

     P                 E

      *----------------------------------------------------------------------
      * ECHOHANDLERSEC: secure (SSL) CFS echo server handler
      *----------------------------------------------------------------------

     PECHOHANDLERSEC...
     P                 B                   EXPORT
     D                 PI            10I 0
     D@Conn                          10I 0 Value
     D@Address                         *   Value
     D@Port                            *   Value
     D@Data                            *   Value

     DpCONN            S               *
     DappID            S             32A
     DFromCCSID        S              6A
     DToCCSID          S              6A
     DhResult          S             10U 0
     DiSSLResult       S             10I 0
     Dquit             S             10I 0
     DSize             S             20U 0
     DszMessage        S            256A
     DszResponse       S            256A
     Dconverter        S               *


     DSessionDS        DS                  LikeDS(SERVERSESSIONINFO_100)

      /Free

          appID = *Blanks; // Only used to identify SSL certtificate to use
                           // in secure connections. This example is a
                           // non SSL handler.

          Clear SessionDS;

          appID = 'QIBM_QTMF_FTP_SERVER' + x'00';

          SessionDS.szApplicationID = %Addr(appID);

          pCONN = CFS_SecureOpenChannel(@Conn:
                                        %Addr(SessionDS):
                                        CFS_SERVERSESSION_FMT_100:
                                        %Addr(iSSLResult));

          If (pCONN <> *Null);

            converter = CSSTRCV_Constructor();

            hResult = CS_SUCCESS;
            Dou %bitand(hResult: CS_MASK_ERROR) <> CS_SUCCESS;

              Size = 255;
              szMessage = *Blanks;
              hResult = CFS_SecureRead(pCONN:
                                       %Addr(szMessage):
                                       %Addr(Size):
                                       -1: %Addr(iSSLResult));

              If  %bitand(hResult: CS_MASK_ERROR) = CS_SUCCESS And
                  %bitand(hResult: CS_MASK_DIAG) <> CFS_DIAG_CONNCLOSE;

                ////////////////////////////////////////////////////////////
                // echo back:
                // this is just for demonstration purposes.
                // In this example, we assume the client
                // sends ASCII ... we will convert to EBCDIC
                // and then back to ASCII to send over to client.
                ////////////////////////////////////////////////////////////

                // null-terminate

                // Start a conversion from ASCII to the job CCSID
                FromCCSID = '00819' + x'00';
                ToCCSID = '00000' + x'00';
                CSSTRCV_SetConversion(converter:
                                      %Addr(FromCCSID): %Addr(ToCCSID));


                CSSTRCV_StrCpy(converter: %Addr(szMessage): Size);

                size = CSSTRCV_Size(converter);

                // We know that szClientByte is large enough ...
                szResponse = *Blanks;
                CSSTRCV_Get(converter: %Addr(szResponse));

                ////////////////////////////////////////////////////////////
                // Just to give a way for the client to
                // disconnect from this handler, if the first
                // character is the letter q, then we leave
                // the loop.
                ////////////////////////////////////////////////////////////

                quit = 0;

                szMessage = *Blanks;

                if %Trim(szResponse) = 'q';

                  quit = 1;
                  szMessage = 'ECHO HANDLER: GOODBYE :-)';

                Else;

                  szMessage = 'ECHO HANDLER: ' + %Trim(szResponse);

                EndIf;

                FromCCSID = '00000' + x'00';
                ToCCSID = '00819' + x'00';
                CSSTRCV_SetConversion(converter:
                                      %Addr(FromCCSID): %Addr(ToCCSID));

                CSSTRCV_StrCpy(converter:
                               %Addr(szMessage):
                               %Len(%Trim(szMessage)));

                // retrieve the converted string
                size = CSSTRCV_Size(converter);

                szResponse = *Blanks;
                CSSTRCV_Get(converter: %Addr(szResponse));

                // Send respopnse to client
                hResult = CFS_SecureWrite(pCONN:
                                          %Addr(szResponse):
                                          %Addr(size):
                                          -1: %Addr(iSSLResult));

                if quit = 1;
                  Leave;
                EndIf;
              EndIf;

            EndDo;

            CFS_SecureClose(pCONN);

          EndIf;

          Return 0;

      /End-Free

     P                 E


