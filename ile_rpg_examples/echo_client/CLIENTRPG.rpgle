
      /////////////////////////////////////////////////////////////////////////////////
      //
      //    Clarasoft Foundation Server for AS400
      //
      //    CLIENTRPG
      //    Echo client for connecting to CFS echo server
      //    Version 1.0.0
      //
      //    Compile module with:
      //
      //       CRTRPGMOD MODULE(CLIENTRPG) SRCFILE(QRPGLESRC)
      //                 SRCMBR(CLIENTRPG) DBGVIEW(*ALL)
      //
      //    Build with:
      //
      //       CRTPGM PGM(CLIENTRPG) BNDSRVPGM((CFSAPI))
      //
      //    To have a non secure session, call with:
      //
      //       call clientrpg parm('hostname' '41101' ' ')
      //
      //       Parameters are: hostname, port number (as string) and blank space.
      //
      //    To have a secure (SSL) session, the server must be started with
      //    SSL support and then call client with:
      //
      //       call clientrpg parm('hostname' '41101' 's')
      //
      //       Parameters are: hostname, port number (as string) 
      //                       and the letter s (lowercase). 
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

      /Copy cfsapih

     DEntryProc        Pr                  ExtPgm('CLIENTRPG')
     D@Host                          32A
     D@Port                          10A
     D@Mode                           1A

     DEntryProc        PI
     D@Host                          32A
     D@Port                          10A
     D@Mode                           1A

      *----------------------------------------------------------------------
      * ECHOHANDLER: non secure CFS echo server handler
      *----------------------------------------------------------------------

     DpCONN            S               *
     DappID            S             32A
     DFromCCSID        S              6A
     DszHost           S             33A
     DToCCSID          S              6A
     DhResult          S             10U 0
     DiSSLResult       S             10I 0
     DSize             S             20U 0
     DszMessage        S            256A
     Dconverter        S               *


     DSessionDS        DS                  LikeDS(CLIENTSESSIONINFO_100)

      /Free

         Clear SessionDS;

         szHost = %Trim(@Host) + x'00'; // we must null-terminate hostname
         SessionDS.szHostName = %Addr(szHost);
         SessionDS.port = %Int(@Port);
         SessionDS.connTimeout = 10;

         If @mode = 's';

           // We can use any certificate...
           // let's use the FTP server certificate

           appID = 'QIBM_QTMF_FTP_SERVER' + x'00';

           SessionDS.szApplicationID = %Addr(appID);

           pCONN = CFS_SecureConnect(%Addr(SessionDS):
                                     CFS_CLIENTSESSION_FMT_100:
                                     %Addr(iSSLResult));

           If (pCONN <> *Null);

             converter = CSSTRCV_Constructor();

             szMessage = 'Hello World!';

             // Convert to ASCII

             FromCCSID = '00000' + x'00';
             ToCCSID = '00819' + x'00';
             CSSTRCV_SetConversion(converter:
                                   %Addr(FromCCSID): %Addr(ToCCSID));

             CSSTRCV_StrCpy(converter:
                            %Addr(szMessage):
                            %Len(%Trim(szMessage)));

             // retrieve the converted string
             size = CSSTRCV_Size(converter);

             szMessage = *Blanks;
             CSSTRCV_Get(converter: %Addr(szMessage));

             // Send message to server

             hResult = CFS_SecureWrite(pCONN:
                                 %Addr(szMessage):
                                 %Addr(size):
                                 -1: %Addr(iSSLResult));

             // Read echo server response ...

             Size = 255;
             szMessage = *Blanks;
             hResult = CFS_SecureRead(pCONN:
                                %Addr(szMessage):
                                %Addr(Size):
                                -1: %Addr(iSSLResult));

             If  %bitand(hResult: CS_MASK_ERROR) = CS_SUCCESS;

               // Convert from ASCII to the job CCSID

               FromCCSID = '00819' + x'00';
               ToCCSID = '00000' + x'00';
               CSSTRCV_SetConversion(converter:
                                     %Addr(FromCCSID): %Addr(ToCCSID));

               CSSTRCV_StrCpy(converter: %Addr(szMessage): Size);

               size = CSSTRCV_Size(converter);

               szMessage = *Blanks;
               CSSTRCV_Get(converter: %Addr(szMessage));

               ////////////////////////////////////////////////////////////
               // We will tell the server we are done by
               // sending the letter "q"
               ////////////////////////////////////////////////////////////

               szMessage = 'q';

               FromCCSID = '00000' + x'00';
               ToCCSID = '00819' + x'00';
               CSSTRCV_SetConversion(converter:
                                     %Addr(FromCCSID): %Addr(ToCCSID));

               CSSTRCV_StrCpy(converter:
                              %Addr(szMessage):
                              %Len(%Trim(szMessage)));

               // retrieve the converted string
               size = CSSTRCV_Size(converter);

               szMessage = *Blanks;
               CSSTRCV_Get(converter: %Addr(szMessage));

               // Send respopnse to client
               hResult = CFS_SecureWrite(pCONN:
                                   %Addr(szMessage):
                                   %Addr(size):
                                   -1: %Addr(iSSLResult));

               // Server will respond to our quit message ...

               Size = 255;
               szMessage = *Blanks;
               hResult = CFS_SecureRead(pCONN:
                                  %Addr(szMessage):
                                  %Addr(Size):
                                  -1: %Addr(iSSLResult));

               If  %bitand(hResult: CS_MASK_ERROR) = CS_SUCCESS;

                 // Convert from ASCII to the job CCSID

                 FromCCSID = '00819' + x'00';
                 ToCCSID = '00000' + x'00';
                 CSSTRCV_SetConversion(converter:
                                       %Addr(FromCCSID): %Addr(ToCCSID));

                 CSSTRCV_StrCpy(converter: %Addr(szMessage): Size);

                 size = CSSTRCV_Size(converter);

                 szMessage = *Blanks;
                 CSSTRCV_Get(converter: %Addr(szMessage));

               EndIf;

             EndIf;

             CFS_SecureClose(pCONN);

           EndIf;

         Else;

           pCONN = CFS_Connect(%Addr(SessionDS):
                               CFS_CLIENTSESSION_FMT_100);

           If (pCONN <> *Null);

             converter = CSSTRCV_Constructor();

             szMessage = 'Hello World!';

             // Convert to ASCII

             FromCCSID = '00000' + x'00';
             ToCCSID = '00819' + x'00';
             CSSTRCV_SetConversion(converter:
                                   %Addr(FromCCSID): %Addr(ToCCSID));

             CSSTRCV_StrCpy(converter:
                            %Addr(szMessage):
                            %Len(%Trim(szMessage)));

             // retrieve the converted string
             size = CSSTRCV_Size(converter);

             szMessage = *Blanks;
             CSSTRCV_Get(converter: %Addr(szMessage));

             // Send message to server

             hResult = CFS_Write(pCONN:
                                 %Addr(szMessage):
                                 %Addr(size):
                                 -1);

             // Read echo server response ...

             Size = 255;
             szMessage = *Blanks;
             hResult = CFS_Read(pCONN:
                                %Addr(szMessage):
                                %Addr(Size):
                                -1);

             If  %bitand(hResult: CS_MASK_ERROR) = CS_SUCCESS;

               // Convert from ASCII to the job CCSID

               FromCCSID = '00819' + x'00';
               ToCCSID = '00000' + x'00';
               CSSTRCV_SetConversion(converter:
                                     %Addr(FromCCSID): %Addr(ToCCSID));

               CSSTRCV_StrCpy(converter: %Addr(szMessage): Size);

               size = CSSTRCV_Size(converter);

               szMessage = *Blanks;
               CSSTRCV_Get(converter: %Addr(szMessage));

               ////////////////////////////////////////////////////////////
               // We will tell the server we are done by
               // sending the letter "q"
               ////////////////////////////////////////////////////////////

               szMessage = 'q';

               FromCCSID = '00000' + x'00';
               ToCCSID = '00819' + x'00';
               CSSTRCV_SetConversion(converter:
                                     %Addr(FromCCSID): %Addr(ToCCSID));

               CSSTRCV_StrCpy(converter:
                              %Addr(szMessage):
                              %Len(%Trim(szMessage)));

               // retrieve the converted string
               size = CSSTRCV_Size(converter);

               szMessage = *Blanks;
               CSSTRCV_Get(converter: %Addr(szMessage));

               // Send respopnse to client
               hResult = CFS_Write(pCONN:
                                   %Addr(szMessage):
                                   %Addr(size):
                                   -1);

               // Server will respond to our quit message ...

               Size = 255;
               szMessage = *Blanks;
               hResult = CFS_Read(pCONN:
                                  %Addr(szMessage):
                                  %Addr(Size):
                                  -1);

               If  %bitand(hResult: CS_MASK_ERROR) = CS_SUCCESS;

                 // Convert from ASCII to the job CCSID

                 FromCCSID = '00819' + x'00';
                 ToCCSID = '00000' + x'00';
                 CSSTRCV_SetConversion(converter:
                                       %Addr(FromCCSID): %Addr(ToCCSID));

                 CSSTRCV_StrCpy(converter: %Addr(szMessage): Size);

                 size = CSSTRCV_Size(converter);

                 szMessage = *Blanks;
                 CSSTRCV_Get(converter: %Addr(szMessage));

               EndIf;

             EndIf;

             CFS_Close(pCONN);

           EndIf;

         EndIf;

         *InLr = *On;
         Return;

      /End-Free


