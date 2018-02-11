
      /////////////////////////////////////////////////////////////////////////////////
      //
      //    Clarasoft Foundation Server for AS400
      //
      //    CFSAPIH
      //
      //    CFS-400 Definitions for ILE RPG
      //    Version 1.0.0
      //
      //
      //    Distributed under the MIT license
      //
      //    Copyright (c) 2017 Clarasoft I.T. Solutions Inc.
      //
      //    Permission is hereby granted, free of charge, to any person obtaining
      //    a copy of this software and associated documentation files
      //    (the "Software"), to deal in the Software without restriction,
      //    including without limitation the rights to use, copy, modify,
      //    merge, publish, distribute, sublicense, and/or sell
      //    copies of the Software, and to permit persons to whom the Software is
      //    furnished to do so, subject to the following conditions:
      //    The above copyright notice and this permission notice shall be
      //    included in all copies or substantial portions of the Software.
      //    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
      //    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
      //    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
      //    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
      //    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
      //    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
      //    THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
      //
      /////////////////////////////////////////////////////////////////////////////////


      /IF NOT DEFINED(CFSAPIH)
      /DEFINE CFSAPIH

      *----------------------------------------------------------------------
      *  CFS Constants
      *----------------------------------------------------------------------

     DCS_SUCCESS...
     D                 C                     Const(x'00000000')

     DCS_FAILURE...
     D                 C                     Const(x'80000000')

     DCS_MASK_ERROR...
     D                 C                     Const(x'80000000')
     DCS_MASK_OPERATION...
     D                 C                     Const(x'0FFF0000')
     DCS_MASK_DIAG...
     D                 C                     Const(x'0000FFFF')

     DCFS_CLIENTSESSION_FMT_100...
     D                 C                     Const(x'00003064')
     DCFS_SERVERSESSION_FMT_100...
     D                 C                     Const(x'00004064')

     DCFS_OPER_WAIT...
     D                 C                     Const(x'00010000')
     DCFS_OPER_READ...
     D                 C                     Const(x'01010000')
     DCFS_OPER_WRITE...
     D                 C                     Const(x'01020000')

     DCFS_DIAG_CONNCLOSE...
     D                 C                     Const(x'0000F001')
     DCFS_DIAG_WOULDBLOCK...
     D                 C                     Const(x'0000F002')
     DCFS_DIAG_READNOBLOCK...
     D                 C                     Const(x'0000F003')
     DCFS_DIAG_WRITENOBLOCK...
     D                 C                     Const(x'0000F004')
     DCFS_DIAG_TIMEDOUT...
     D                 C                     Const(x'0000F005')
     DCFS_DIAG_ALLDATA...
     D                 C                     Const(x'0000F006')
     DCFS_DIAG_PARTIALDATA...
     D                 C                     Const(x'0000F007')
     DCFS_DIAG_NODATA...
     D                 C                     Const(x'0000F008')
     DCFS_DIAG_INVALIDSIZE...
     D                 C                     Const(x'0000F009')
     DCFS_DIAG_ENVOPEN...
     D                 C                     Const(x'0000F00A')
     DCFS_DIAG_APPID...
     D                 C                     Const(x'0000F00B')
     DCFS_DIAG_SESSIONTYPE...
     D                 C                     Const(x'0000F00C')
     DCFS_DIAG_ENVINIT...
     D                 C                     Const(x'0000F00D')
     DCFS_DIAG_SOCOPEN...
     D                 C                     Const(x'0000F00E')
     DCFS_DIAG_SETFD...
     D                 C                     Const(x'0000F00F')
     DCFS_DIAG_SOCINIT...
     D                 C                     Const(x'0000F010')
     DCFS_DIAG_SYSTEM...
     D                 C                     Const(x'0000FFFE')
     DCFS_DIAG_UNKNOWN...
     D                 C                     Const(x'0000FFFF')

     DCFS_OFFSET_BUFFERS_MAXITEMS...
     D                 C                     Const(19)
     DCFS_OFFSET_NUMERIC_MAXITEMS...
     D                 C                     Const(11)

     DCFS_OFFSET_GSK_KEYRING_FILE...
     D                 C                     Const(1)
     DCFS_OFFSET_GSK_KEYRING_PW...
     D                 C                     Const(2)
     DCFS_OFFSET_GSK_KEYRING_LABEL...
     D                 C                     Const(3)
     DCFS_OFFSET_GSK_V2_CIPHER_SPECS...
     D                 C                     Const(4)
     DCFS_OFFSET_GSK_V3_CIPHER_SPECS...
     D                 C                     Const(5)
     DCFS_OFFSET_GSK_V3_CIPHER_SPECS_EX...
     D                 C                     Const(6)
     DCFS_OFFSET_GSK_TLSV12_CIPHER_SPECS...
     D                 C                     Const(7)
     DCFS_OFFSET_GSK_TLSV12_CIPHER_SPECS_EX...
     D                 C                     Const(8)
     DCFS_OFFSET_GSK_TLSV11_CIPHER_SPECS...
     D                 C                     Const(9)
     DCFS_OFFSET_GSK_TLSV11_CIPHER_SPECS_EX...
     D                 C                     Const(10)
     DCFS_OFFSET_GSK_TLSV10_CIPHER_SPECS...
     D                 C                     Const(11)
     DCFS_OFFSET_GSK_TLSV10_CIPHER_SPECS_EX...
     D                 C                     Const(12)
     DCFS_OFFSET_GSK_SSL_EXTN_SIGALG...
     D                 C                     Const(13)
     DCFS_OFFSET_GSK_OCSP_URL...
     D                 C                     Const(14)
     DCFS_OFFSET_GSK_OCSP_PROXY_SERVER_NAME...
     D                 C                     Const(15)
     DCFS_OFFSET_GSK_SSL_EXTN_SERVERNAME_REQUEST...
     D                 C                     Const(16)
     DCFS_OFFSET_GSK_SSL_EXTN_SERVERNAME_CRITICAL_REQUEST...
     D                 C                     Const(17)
     DCFS_OFFSET_GSK_SSL_EXTN_SERVERNAME_LIST...
     D                 C                     Const(18)
     DCFS_OFFSET_GSK_SSL_EXTN_SERVERNAME_CRITICAL_LIST...
     D                 C                     Const(19)

     DCFS_OFFSET_GSK_V2_SESSION_TIMEOUT...
     D                 C                     Const(1)
     DCFS_OFFSET_GSK_V3_SESSION_TIMEOUT...
     D                 C                     Const(2)
     DCFS_OFFSET_GSK_OS400_READ_TIMEOUT...
     D                 C                     Const(3)
     DCFS_OFFSET_GSK_HANDSHAKE_TIMEOUT...
     D                 C                     Const(4)
     DCFS_OFFSET_GSK_OCSP_MAX_RESPONSE_SIZE...
     D                 C                     Const(5)
     DCFS_OFFSET_GSK_OCSP_TIMEOUT...
     D                 C                     Const(6)
     DCFS_OFFSET_GSK_OCSP_NONCE_SIZE...
     D                 C                     Const(7)
     DCFS_OFFSET_GSK_OCSP_CLIENT_CACHE_SIZE...
     D                 C                     Const(8)
     DCFS_OFFSET_GSK_OCSP_PROXY_SERVER_PORT...
     D                 C                     Const(9)
     DCFS_OFFSET_GSK_SSL_EXTN_MAXFRAGMENT_SIZE...
     D                 C                     Const(10)
     DCFS_OFFSET_GSK_TLS_CBCPROTECTION_METHOD...
     D                 C                     Const(11)

      *----------------------------------------------------------------------
      * SESSION INFORMATION STRUCTURES (one for client session,
      * the other for server session)
      *----------------------------------------------------------------------

     DCLIENTSESSIONINFO_100...
     D                 DS                    Align
     D                                       Qualified
     DszApplicationID                  *
     DszHostName                       *
     Dport...
     D                               10I 0
     DconnTimeout...
     D                               10I 0
     DTimeout...
     D                               10I 0
     DgskProtocolOverrideDefaults...
     D                               10I 0
     DgskAuthTypeOverrideDefaults...
     D                               10I 0
     DgskSessionCloseOverrideDefaults...
     D                               10I 0
     DgskOverrideNumericDefaults...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_TLSV12...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_TLSV11...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_TLSV10...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_TLSV1...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_SSLV3...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_SSLV2...
     D                               10I 0
     DgskAttribute_GSK_OCSP_ENABLE...
     D                               10I 0
     DgskAttribute_GSK_OCSP_NONCE_GENERATION_ENABLE...
     D                               10I 0
     DgskAttribute_GSK_OCSP_NONCE_CHECK_ENABLE...
     D                               10I 0
     DgskAttribute_GSK_OCSP_RETRIEVE_VIA_GET...
     D                               10I 0
     DgskAttribute_GSK_EXTENDED_RENEGOTIATION_CRITICAL_CLIENT...
     D                               10I 0
     DgskSessionClose_GSK_DELAYED_ENVIRONMENT_CLOSE...
     D                               10I 0
     DgskSessionClose_GSK_NORMAL_ENVIRONMENT_CLOSE...
     D                               10I 0
     DgskAuthType_GSK_SERVER_AUTH_FULL...
     D                               10I 0
     DgskAuthType_GSK_SERVER_AUTH_PASSTHRU...
     D                               10I 0
     DpszBuffers...
     D                                 *    Dim(19)
     DiNumericValues...
     D                               10I 0  Dim(11)

     DSERVERSESSIONINFO_100...
     D                 DS                    Align
     D                                       Qualified
     DszApplicationID                  *
     DgskProtocolOverrideDefaults...
     D                               10I 0
     DgskAuthTypeOverrideDefaults...
     D                               10I 0
     DgskSessionCloseOverrideDefaults...
     D                               10I 0
     DgskOverrideNumericDefaults...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_TLSV12...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_TLSV11...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_TLSV10...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_TLSV1...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_SSLV3...
     D                               10I 0
     DgskProtocol_GSK_PROTOCOL_SSLV2...
     D                               10I 0
     DgskAttribute_GSK_OCSP_ENABLE...
     D                               10I 0
     DgskAttribute_GSK_OCSP_NONCE_GENERATION_ENABLE...
     D                               10I 0
     DgskAttribute_GSK_OCSP_NONCE_CHECK_ENABLE...
     D                               10I 0
     DgskAttribute_GSK_OCSP_RETRIEVE_VIA_GET...
     D                               10I 0
     DgskAttribute_GSK_EXTENDED_RENEGOTIATION_CRITICAL_SERVER...
     D                               10I 0
     DgskAttribute_GSK_CERTREQ_DNLIST_ENABLE...
     D                               10I 0
     DgskSession_GSK_ALLOW_UNAUTHENTICATED_RESUME...
     D                               10I 0
     DgskSessionClose_GSK_DELAYED_ENVIRONMENT_CLOSE...
     D                               10I 0
     DgskSessionClose_GSK_NORMAL_ENVIRONMENT_CLOSE...
     D                               10I 0
     DgskAuthType_GSK_CLIENT_AUTH_FULL...
     D                               10I 0
     DgskAuthType_GSK_CLIENT_AUTH_PASSTHRU...
     D                               10I 0
     DgskAuthType_GSK_OS400_CLIENT_AUTH_REQUIRED...
     D                               10I 0
     DpszBuffers...
     D                                 *    Dim(19)
     DiNumericValues...
     D                               10I 0  Dim(11)

      *----------------------------------------------------------------------
      *  CFS-400 Functions
      *----------------------------------------------------------------------

     DCFS_Close...
     D                 Pr            10U 0   ExtProc('CFS_Close')
     D@This                            *     Value

     DCFS_Connect...
     D                 Pr              *     ExtProc('CFS_Connect')
     D@SessionInfo                     *     Value
     D@Format                        10I 0   Value

     DCFS_MakeUUID...
     D                 Pr            10I 0   ExtProc('CFS_MakeUUID')
     D@Buffer                          *     Value
     D@Format                        10I 0   Value

     DCFS_NetworkToPresentation...
     D                 Pr            10U 0   ExtProc
     D                                       ('CFS_NetworkToPresentation')
     D@Addr_in                         *     Value
     D@outAddr                         *     Value
     D@outPort                         *     Value

     DCFS_OpenChannel...
     D                 Pr              *     ExtProc('CFS_OpenChannel')
     D@ConnFd                        10I 0   Value
     D@sessionInfo                     *     Value
     D@sessionInfoFmt                10I 0   Value

     DCFS_Read...
     D                 Pr            10U 0   ExtProc('CFS_Read')
     D@This                            *     Value
     D@Data                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value

     DCFS_ReadRecord...
     D                 Pr            10U 0   ExtProc('CFS_ReadRecord')
     D@This                            *     Value
     D@Data                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value

     DCFS_ReceiveDescriptor...
     D                 Pr            10U 0   ExtProc('CFS_ReceiveDescriptor')
     D@Fd                            10I 0   Value
     D@Descriptor                      *     Value
     D@TimeOut                       10I 0   Value

     DCFS_SecureClose...
     D                 Pr            10U 0   ExtProc('CFS_SecureClose')
     D@This                            *     Value

     DCFS_SecureConnect...
     D                 Pr              *     ExtProc('CFS_SecureConnect')
     D@SessionInfo                     *     Value
     D@Format                        10I 0   Value
     D@SSLResult                       *     Value

     DCFS_SecureOpenChannel...
     D                 Pr              *     ExtProc('CFS_SecureOpenChannel')
     D@ConnFd                        10I 0   Value
     D@sessionInfo                     *     Value
     D@sessionInfoFmt                10I 0   Value
     D@SSLResult                       *     Value

     DCFS_SecureRead...
     D                 Pr            10U 0   ExtProc('CFS_SecureRead')
     D@This                            *     Value
     D@Data                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value
     D@SSLResult                       *     Value

     DCFS_SecureReadRecord...
     D                 Pr            10U 0   ExtProc('CFS_SecureReadRecord')
     D@This                            *     Value
     D@Data                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value
     D@SSLResult                       *     Value

     DCFS_SecureWrite...
     D                 Pr            10U 0   ExtProc('CFS_SecureWrite')
     D@This                            *     Value
     D@Data                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value
     D@SSLResult                       *     Value

     DCFS_SecureWriteRecord...
     D                 Pr            10U 0   ExtProc('CFS_SecureReadWrite')
     D@This                            *     Value
     D@Data                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value
     D@SSLResult                       *     Value

     DCFS_SendDescriptor...
     D                 Pr            10U 0   ExtProc('CFS_SendDescriptor')
     D@Fd                            10I 0   Value
     D@Descriptor                    10I 0   Value
     D@TimeOut                       10I 0   Value

     DCFS_Write...
     D                 Pr            10U 0   ExtProc('CFS_Write')
     D@This                            *     Value
     D@Data                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value

     DCFS_WriteRecord...
     D                 Pr            10U 0   ExtProc('CFS_WriteRecord')
     D@This                            *     Value
     D@Data                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value

      *----------------------------------------------------------------------
      *  CFS-400 Websocket CONSTANTS
      *----------------------------------------------------------------------

     DCSWSCK_OPER_CONTINUATION...
     D                 C                     Const(x'00000000')

     DCSWSCK_OPER_TEXT...
     D                 C                     Const(x'00010000')

     DCSWSCK_OPER_BINARY...
     D                 C                     Const(x'00020000')

     DCSWSCK_OPER_CLOSE...
     D                 C                     Const(x'00080000')

     DCSWSCK_OPER_PING...
     D                 C                     Const(x'00090000')

     DCSWSCK_OPER_PONG...
     D                 C                     Const(x'000A0000')

     DCSWSCK_FIN_OFF...
     D                 C                     Const(x'00')

     DCSWSCK_FIN_ON...
     D                 C                     Const(x'01')

     DCSWSCK_E_NODATA...
     D                 C                     Const(x'00000001')

     DCSWSCK_E_PARTIALDATA...
     D                 C                     Const(x'00000002')

     DCSWSCK_E_ALLDATA...
     D                 C                     Const(x'00000003')

     DCSWSCK_MOREDATA...
     D                 C                     Const(x'00000002')

     DCSWSCK_ALLDATA...
     D                 C                     Const(x'00000003')

     DCSWSCK_RCV_ALL...
     D                 C                     Const(x'00000001')

     DCSWSCK_RCV_PARTIAL...
     D                 C                     Const(x'00000002')

     DCSWSCK_MASK_ERROR...
     D                 C                     Const(x'80000000')

     DCSWSCK_MASK_OPERATION...
     D                 C                     Const(x'0FFF0000')

     DCSWSCK_MASK_DIAG...
     D                 C                     Const(x'0000FFFF')

     DCSWSCK_SUCCESS...
     D                 C                     Const(x'00000000')

     DCSWSCK_FAILURE...
     D                 C                     Const(x'80000000')

      *----------------------------------------------------------------------
      *  CFS-400 : Websocket Functions
      *----------------------------------------------------------------------

     DCSWSCK_Connect...
     D                 Pr            10U 0   ExtProc('CSWSCK_Connect')
     D@sessionInfo                     *     Value
     D@sessionInfoFmt                10I 0   Value

     DCSWSCK_OpenChannel...
     D                 Pr              *     ExtProc('CSWSCK_OpenChannel')
     D@ConnFd                        10I 0   Value
     D@sessionInfo                     *     Value
     D@sessionInfoFmt                10I 0   Value

     DCSWSCK_SecureConnect...
     D                 Pr              *     ExtProc('CSWSCK_SecureConnect')
     D@sessionInfo                     *     Value
     D@sessionInfoFmt                10I 0   Value

     DCSWSCK_SecureOpenChannel...
     D                 Pr              *     ExtProc('CSWSCK_SecureOpenChannel')
     D@ConnFd                        10I 0   Value
     D@sessionInfo                     *     Value
     D@sessionInfoFmt                10I 0   Value

     DCSWSCK_Close...
     D                 Pr            10U 0   ExtProc('CSWSCK_Close')
     D@This                            *     Value
     D@Data                            *     Value
     D@dataSize                      20U 0   Value
     D@timeout                       10I 0   Value

     DCSWSCK_SecureClose...
     D                 Pr            10U 0   ExtProc('CSWSCK_SecureClose')
     D@This                            *     Value
     D@Data                            *     Value
     D@dataSize                      20U 0   Value
     D@timeout                       10I 0   Value

     DCSWSCK_Ping...
     D                 Pr            10U 0   ExtProc('CSWSCK_Ping')
     D@This                            *     Value
     D@Data                            *     Value
     D@dataSize                      20U 0   Value
     D@timeout                       10I 0   Value

     DCSWSCK_SecurePing...
     D                 Pr            10U 0   ExtProc('CSWSCK_SecurePing')
     D@This                            *     Value
     D@Data                            *     Value
     D@dataSize                      20U 0   Value
     D@timeout                       10I 0   Value

     DCSWSCK_Receive...
     D                 Pr            10U 0   ExtProc('CSWSCK_Receive')
     D@This                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value

     DCSWSCK_SecureReceive...
     D                 Pr            10U 0   ExtProc('CSWSCK_SecureReceive')
     D@This                            *     Value
     D@DataSize                        *     Value
     D@TimeOut                       10I 0   Value

     DCSWSCK_GetData...
     D                 Pr            10U 0   ExtProc('CSWSCK_GetData')
     D@This                            *     Value
     D@Data                            *     Value
     D@Offset                        10I 0   Value
     D@Size                          10I 0   Value

     DCSWSCK_Send...
     D                 Pr            10U 0   ExtProc('CSWSCK_Send')
     D@This                            *     Value
     D@Oeration                      10I 0   Value
     D@Data                            *     Value
     D@DataSize                      10I 0   Value
     D@FinState                      10I 0   Value
     D@TimeOut                       10I 0   Value

     DCSWSCK_SecureSend...
     D                 Pr            10U 0   ExtProc('CSWSCK_SecureSend')
     D@This                            *     Value
     D@Oeration                      10I 0   Value
     D@Data                            *     Value
     D@DataSize                      10I 0   Value
     D@FinState                      10I 0   Value
     D@TimeOut                       10I 0   Value

      *----------------------------------------------------------------------
      *  CSLIST
      *----------------------------------------------------------------------

     DCSLIST_TOP...
     D                 C                   Const(x'00000000')
     DCSLIST_BOTTOM...
     D                 C                   Const(x'FFFFFFFF')

     DCSLIST_Clear...
     D                 PR            10U 0 ExtProc('CSLIST_Clear')
     D@This                            *   Value

     DCSLIST_Constructor...
     D                 PR              *   ExtProc('CSLIST_Constructor')

     DCSLIST_Count...
     D                 PR            10U 0 ExtProc('CSLIST_Count')
     D@This                            *   Value

     DCSLIST_Destructor...
     D                 PR            10U 0 ExtProc('CSLIST_Destructor')
     D@This                            *

     DCSLIST_Get...
     D                 PR            10U 0 ExtProc('CSLIST_Get')
     D@This                            *   Value
     D@Value                           *   Value
     D@Bytes                         10I 0
     D@Index                         10U 0 Value

     DCSLIST_GetDataRef...
     D                 PR            10U 0 ExtProc('CSLIST_GetDataRef')
     D@This                            *   Value
     D@ItemRef                         *
     D@Bytes                         10I 0
     D@Index                         10U 0 Value

     DCSLIST_Insert...
     D                 PR            10U 0 ExtProc('CSLIST_Insert')
     D@This                            *   Value
     D@Value                           *   Value
     D@Bytes                         10I 0 Value
     D@Index                         10U 0 Value

     DCSLIST_ItemSize...
     D                 PR            10U 0 ExtProc('CSLIST_ItemSize')
     D@This                            *   Value
     D@Index                         10U 0 Value

     DCSLIST_Remove...
     D                 PR            10U 0 ExtProc('CSLIST_Remove')
     D@This                            *   Value
     D@Index                         10U 0 Value

     DCSLIST_Set...
     D                 PR            10U 0 ExtProc('CSLIST_Set')
     D@This                            *   Value
     D@Value                           *   Value
     D@Bytes                         10I 0 Value
     D@Index                         10U 0 Value

      *----------------------------------------------------------------------
      *  CSSTR
      *----------------------------------------------------------------------

     DCSSTR_B64_LINEBREAK_OFFSET...
     D                 C                   Const(x'0000004D')
     DCSSTR_B64_MASK_LINEBREAK...
     D                 C                   Const(x'0000000F')
     DCSSTR_B64_LINEBREAK_NONE...
     D                 C                   Const(x'00000000')
     DCSSTR_B64_LINEBREAK_LF...
     D                 C                   Const(x'00000001')
     DCSSTR_B64_LINEBREAK_CRLF...
     D                 C                   Const(x'00000002')
     DCSSTR_B64_IGNOREINVALIDCHAR...
     D                 C                   Const(x'00000100')
     DCSSTR_URLENCODE_SPACETOPLUS...
     D                 C                   Const(x'00000100')
     DCSSTR_URLENCODE_XLATERESERVED...
     D                 C                   Const(x'00000200')

     DCS_OPER_CSSTRCV...
     D                 C                   Const(x'00020000')


     DCSSTR_FromBase64...
     D                 PR            20U 0 ExtProc('CSSTR_FromBase64')
     D  InBuffer                       *   Value
     D  InBufferSize                 20U 0 Value
     D  OutBuffer                      *   Value

     DCSSTR_FromBase64Ex...
     D                 PR            20U 0 ExtProc('CSSTR_FromBase64Ex')
     D  InBuffer                       *   Value
     D  InBufferSize                 20U 0 Value
     D  OutBuffer                      *   Value
     D  Flags                        10I 0 Value

     DCSSTR_StrTok...
     D                 PR              *   ExtProc('CSSTR_StrTok')
     D  Buffer                         *   Value
     D  Delimiter                      *   Value

     DCSSTR_ToBase64...
     D                 PR            20U 0 ExtProc('CSSTR_ToBase64')
     D  InBuffer                       *   Value
     D  InBufferSize                 20U 0 Value
     D  OutBuffer                      *   Value

     DCSSTR_ToBase64Ex...
     D                 PR            20U 0 ExtProc('CSSTR_ToBase64Ex')
     D  InBuffer                       *   Value
     D  InBufferSize                 20U 0 Value
     D  OutBuffer                      *   Value
     D  Flags                        10I 0 Value

     DCSSTR_ToLowerCase...
     D                 PR            10I 0 ExtProc('CSSTR_ToLowerCase')
     D  Buffer                         *   Value
     D  Size                         10I 0 Value

     DCSSTR_ToUpperCase...
     D                 PR            10I 0 ExtProc('CSSTR_ToUpperCase')
     D  Buffer                         *   Value
     D  Size                         10I 0 Value

     DCSSTR_Trim...
     D                 PR            10I 0 ExtProc('CSSTR_Trim')
     D  InBuffer                       *   Value
     D  OutBuffer                      *   Value

     DCSSTR_UrlDecode...
     D                 PR            20U 0 ExtProc('CSSTR_UrlDecode')
     D  InBuffer                       *   Value
     D  OutBuffer                      *   Value

     DCSSTR_UrlEncode...
     D                 PR            20U 0 ExtProc('CSSTR_UrlEncode')
     D  InBuffer                       *   Value
     D  OutBuffer                      *   Value

     DCSSTRCV_Constructor...
     D                 PR              *   ExtProc('CSSTRCV_Constructor')

     DCSSTRCV_Destructor...
     D                 PR            10U 0 ExtProc('CSSTRCV_Destructor')
     D  Conv                           *   value

     DCSSTRCV_Get...
     D                 PR            10U 0 ExtProc('CSSTRCV_Get')
     D  Conv                           *   Value
     D  String                         *   Value

     DCSSTRCV_SetConversion...
     D                 PR            10U 0 ExtProc('CSSTRCV_SetConversion')
     D  Conv                           *   Value
     D  FromCCSID                      *   Value
     D  ToCCSID                        *   Value

     DCSSTRCV_Size...
     D                 PR            20U 0 ExtProc('CSSTRCV_Size')
     D  Conv                           *   Value

     DCSSTRCV_StrCat...
     D                 PR            10U 0 ExtProc('CSSTRCV_StrCat')
     D  Conv                           *   Value
     D  String                         *   Value
     D  Size                         20U 0 Value

     DCSSTRCV_StrCpy...
     D                 PR            10U 0 ExtProc('CSSTRCV_StrCpy')
     D  Conv                           *   Value
     D  String                         *   Value
     D  Size                         20U 0 Value

      /ENDIF





