
      /////////////////////////////////////////////////////////////////////////////////
      //
      //    Clarasoft Foundation Server for AS400
      //
      //    ECHO.RPGLE
      //    Demonstration echo service that uses the websocket protocol
      //    Version 1.0.0
      //
      //    Compile module with:
      //
      //       CRTRPGMOD MODULE(ECHOMOD) SRCFILE(QRPGLESRC)
      //                 SRCMBR(ECHO.RPGLE) DBGVIEW(*ALL)
      //
      //    Build program with:
      //
      //       CRTSRVPGM SRVPGM(ECHOSRV) MODULE(ECHOMOD)
      //       BNDSRVPGM(CFSAPI) EXPORT(*ALL)
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

      /Copy echo.rpgh

      *----------------------------------------------------------------------
      *  Connection object: this will be initialised by the CSAP broker
      *----------------------------------------------------------------------

     PECHOHANDLER      B                   EXPORT
     D                 PI            10I 0
     D@Conn                          10I 0 Value
     D@Address                         *   Value
     D@Port                            *   Value
     D@Data                            *   Value

     DpCONN            S               *
     DappID            S             32A
     DRc               S             10U 0
     DErrCode          S             10U 0
     DOpCode           S             10U 0
     DDiagCode         S             10U 0
     DOffset           S             20U 0
     DDataSize         S             20U 0
     DDataBuffer       S           4096A

      /Free

          appID = *Blanks; // Only used to identify SSL certtificate to use
                           // in secure connections. This example is a
                           // non SSL handler.

          pCONN = CSWSCK_OpenServer(@Conn: %Addr(appID): *Null: 0: *Null: 0);

          If (pCONN <> *Null);

            ErrCode = CSWSCK_SUCCESS;

            Dow ErrCode = CSWSCK_SUCCESS;

              // Wait for client to send data

              Rc = CSWSCK_Receive(pCONN: %Addr(DataSize): -1);

              // CFS-400 functions return complex results;
              // the success code has to be extracted from the result code
              // via bitwise operations

              ErrCode = %bitand(Rc: CSWSCK_MASK_ERROR);

              If ErrCode = CSWSCK_SUCCESS;

                OpCode    = %bitand(Rc: CSWSCK_MASK_OPERATION);

                If OpCode = CSWSCK_OPER_TEXT;

                  // This means we got data from the client.
                  // We extract the data into our buffer.

                  Offset = 0;  // This to mark where within our
                               // data buffer we must copy client
                               // data: this is the number of bytes
                               // AFTER the beginning position so
                               // zero means position 1 in the buffer.

                  CSWSCK_GetData(pCONN:
                                 %Addr(DataBuffer) + Offset:
                                 0:
                                 DataSize);

                  Offset += DataSize;

                  // Perhaps we just got a fragment of the data ...
                  // read data from client until no more data is forthcoming.
                  // The diagnostic code indicates if more data is waiting
                  // to be read from the client.

                  DiagCode  = %bitand(Rc: CSWSCK_MASK_DIAG);

                  Dow DiagCode = CSWSCK_MOREDATA;

                    Rc = CSWSCK_Receive(pCONN: %Addr(DataSize): -1);

                    ErrCode   = %bitand(Rc: CSWSCK_MASK_ERROR);

                    If ErrCode = CSWSCK_SUCCESS;

                      OpCode    = %bitand(Rc: CSWSCK_MASK_OPERATION);
                      DiagCode  = %bitand(Rc: CSWSCK_MASK_DIAG);

                      If OpCode = CSWSCK_OPER_CONTINUATION;

                        CSWSCK_GetData(pCONN:
                                       %Addr(DataBuffer) + Offset:
                                       0:
                                       DataSize);

                        Offset += DataSize;

                      Else;

                        ErrCode = CSWSCK_FAILURE;
                        Leave;

                      EndIf;

                    Else;

                      ErrCode = CSWSCK_FAILURE;
                      Leave;

                    EndIf;

                  EndDo;

                  // Echo back the data

                  Rc = CSWSCK_Send(pCONN:
                                   CSWSCK_OPER_TEXT:
                                   %Addr(DataBuffer):
                                   DataSize:
                                   CSWSCK_FIN_ON:
                                   -1);

                  ErrCode   = %bitand(Rc: CSWSCK_MASK_ERROR);

                Else;

                  If OpCode = CSWSCK_OPER_CLOSE;

                    ErrCode = CSWSCK_FAILURE; // To leave the receive loop

                  Else;

                    // This means we received some other request such as PING
                    // or binary data: this demonstration program does not
                    // handle those requests.

                  EndIf;

                EndIf;

              EndIf;

            EndDo;

          EndIf;

          // This sends a Websocket CLOSE operation
          CSWSCK_Close(pCONN: *Null: 0: -1);

          Return 0;

      /End-Free

     P                 E

