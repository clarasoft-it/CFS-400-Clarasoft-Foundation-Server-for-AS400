/* ===========================================================================
  Clarasoft Core definitions
  
  cscore.h
  Networking Primitives definitions
  Version 1.0.0
  
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
=========================================================================== */

#ifndef __CLARASOFT_CSCORE_H__
#define __CLARASOFT_CSCORE_H__

#define CS_SUCCESS      (0x00000000)
#define CS_FAILURE      (0x80000000)

#define CS_MASK_OPER    (0x0FFF0000)
#define CS_MASK_DIAG    (0x0000FFFF)

#define CS_SUCCEED(x)   (((x) & (0xF0000000)) == (0))
#define CS_FAIL(x)      ((x) & (CS_FAILURE))
#define CS_OPER(x)      ((x) & (CS_MASK_OPER))
#define CS_DIAG(x)      ((x) & (CS_MASK_DIAG))

#define CS_OPER_NOVALUE           0x00000000
#define CS_DIAG_NOVALUE           0x00000000

#define CS_DIAG_EDOM              0x0000F001
#define CS_DIAG_ERANGE            0x0000F002
#define CS_DIAG_ETRUNC            0x0000F003
#define CS_DIAG_ENOTOPEN          0x0000F004
#define CS_DIAG_ENOTREAD          0x0000F005
#define CS_DIAG_ERECIO            0x0000F006
#define CS_DIAG_ENOTWRITE         0x0000F007
#define CS_DIAG_ESTDIN            0x0000F008
#define CS_DIAG_ESTDOUT           0x0000F009
#define CS_DIAG_ESTDERR           0x0000F00A
#define CS_DIAG_EBADSEEK          0x0000F00B
#define CS_DIAG_EBADNAME          0x0000F00C

#define CS_DIAG_EBADMODE          0x0000F00D
#define CS_DIAG_EBADPOS           0x0000F00E
#define CS_DIAG_ENOPOS            0x0000F00F
#define CS_DIAG_ENUMMBRS          0x0000F0A0
#define CS_DIAG_ENUMRECS          0x0000F0A1
#define CS_DIAG_EBADFUNC          0x0000F0A2
#define CS_DIAG_ENOREC            0x0000F0A3
#define CS_DIAG_EBADDATA          0x0000F0A4
#define CS_DIAG_EBADOPT           0x0000F0A5
#define CS_DIAG_ENOTUPD           0x0000F0A6
#define CS_DIAG_ENOTDLT           0x0000F0A7
#define CS_DIAG_EPAD              0x0000F0A8
#define CS_DIAG_EBADKEYLN         0x0000F0A9
#define CS_DIAG_EPUTANDGET        0x0000F0AA
#define CS_DIAG_EGETANDPUT        0x0000F0AB
#define CS_DIAG_EIOERROR          0x0000F0AC
#define CS_DIAG_EIORECERR         0x0000F0AD
#define CS_DIAG_EINVAL            0x0000F0AE
#define CS_DIAG_EIO               0x0000F0AF
#define CS_DIAG_ENODEV            0x0000F0B0
#define CS_DIAG_EBUSY             0x0000F0B1
#define CS_DIAG_ENOENT            0x0000F0B2
#define CS_DIAG_EPERM             0x0000F0B3
#define CS_DIAG_EACCES            0x0000F0B4
#define CS_DIAG_ENOTDIR           0x0000F0B5
#define CS_DIAG_ENOSPC            0x0000F0B6
#define CS_DIAG_EXDEV             0x0000F0B7
#define CS_DIAG_EWOULDBLOCK       0x0000F0B8
#define CS_DIAG_EAGAIN            0x0000F0B9
#define CS_DIAG_EINTR             0x0000F0BA
#define CS_DIAG_EFAULT            0x0000F0BB
#define CS_DIAG_ETIME             0x0000F0BC
#define CS_DIAG_ENXIO             0x0000F0BD
#define CS_DIAG_ECLOSED           0x0000F0BE
#define CS_DIAG_EADDRINUSE        0x0000F0BF
#define CS_DIAG_EADDRNOTAVAIL     0x0000F0C0
#define CS_DIAG_EAFNOSUPPORT      0x0000F0C1
#define CS_DIAG_EALREADY          0x0000F0C2
#define CS_DIAG_ECONNABORTED      0x0000F0C3
#define CS_DIAG_ECONNREFUSED      0x0000F0C4
#define CS_DIAG_ECONNRESET        0x0000F0C5
#define CS_DIAG_EDESTADDRREQ      0x0000F0C6
#define CS_DIAG_EHOSTDOWN         0x0000F0C7
#define CS_DIAG_EHOSTUNREACH      0x0000F0C8
#define CS_DIAG_EINPROGRESS       0x0000F0C9
#define CS_DIAG_EISCONN           0x0000F0CA
#define CS_DIAG_EMSGSIZE          0x0000F0CB
#define CS_DIAG_ENETDOWN          0x0000F0CC
#define CS_DIAG_ENETRESET         0x0000F0CD
#define CS_DIAG_ENETUNREACH       0x0000F0CE
#define CS_DIAG_ENOBUFS           0x0000F0CF
#define CS_DIAG_ENOPROTOOPT       0x0000F0D0
#define CS_DIAG_ENOTCONN          0x0000F0D1
#define CS_DIAG_ENOTSOCK          0x0000F0D2
#define CS_DIAG_ENOTSUP           0x0000F0D3
#define CS_DIAG_EOPNOTSUPP        0x0000F0D4
#define CS_DIAG_EPFNOSUPPORT      0x0000F0D5
#define CS_DIAG_EPROTONOSUPPORT   0x0000F0D6
#define CS_DIAG_EPROTOTYPE        0x0000F0D7
#define CS_DIAG_ERCVDERR          0x0000F0D8
#define CS_DIAG_ESHUTDOWN         0x0000F0D9
#define CS_DIAG_ESOCKTNOSUPPORT   0x0000F0DA
#define CS_DIAG_ETIMEDOUT         0x0000F0DB
#define CS_DIAG_EUNATCH           0x0000F0DC
#define CS_DIAG_EBADF             0x0000F0DD
#define CS_DIAG_EMFILE            0x0000F0DE
#define CS_DIAG_ENFILE            0x0000F0DF
#define CS_DIAG_EPIPE             0x0000F0E0
#define CS_DIAG_ECANCELED         0x0000F0E1
#define CS_DIAG_ECANCEL           0x0000F0E2
#define CS_DIAG_EEXIST            0x0000F0E3
#define CS_DIAG_EDEADLK           0x0000F0E4
#define CS_DIAG_ENOMEM            0x0000F0E5
#define CS_DIAG_EOWNERTERM        0x0000F0E6
#define CS_DIAG_EDESTROYED        0x0000F0E7
#define CS_DIAG_ETERM             0x0000F0E8
#define CS_DIAG_EMLINK            0x0000F0E9
#define CS_DIAG_ESPIPE            0x0000F0EA
#define CS_DIAG_ENOSYS            0x0000F0EB
#define CS_DIAG_EISDIR            0x0000F0EC
#define CS_DIAG_EROFS             0x0000F0ED
#define CS_DIAG_EUNKNOWN          0x0000F0EE
#define CS_DIAG_EITERBAD          0x0000F0EF
#define CS_DIAG_EDAMAGE           0x0000F0F0
#define CS_DIAG_ELOOP             0x0000F0F1
#define CS_DIAG_ENAMETOOLONG      0x0000F0F2
#define CS_DIAG_ENOLCK            0x0000F0F3
#define CS_DIAG_ENOTEMPTY         0x0000F0F4
#define CS_DIAG_ENOSYSRSC         0x0000F0F5
#define CS_DIAG_ECONVERT          0x0000F0F6
#define CS_DIAG_E2BIG             0x0000F0F7
#define CS_DIAG_EILSEQ            0x0000F0F8
#define CS_DIAG_ESOFTDAMAGE       0x0000F0F9
#define CS_DIAG_ENOTENROLL        0x0000F0FA
#define CS_DIAG_EOFFLINE          0x0000F0FB
#define CS_DIAG_EROOBJ            0x0000F0FC
#define CS_DIAG_ELOCKED           0x0000F0FD
#define CS_DIAG_EFBIG             0x0000F0FE
#define CS_DIAG_EIDRM             0x0000F0FF
#define CS_DIAG_ENOMSG            0x0000F100
#define CS_DIAG_EFILECVT          0x0000F101
#define CS_DIAG_EBADFID           0x0000F102
#define CS_DIAG_ESTALE            0x0000F103
#define CS_DIAG_ESRCH             0x0000F104
#define CS_DIAG_ENOTSIGINIT       0x0000F105
#define CS_DIAG_ECHILD            0x0000F106
#define CS_DIAG_ETOOMANYREFS      0x0000F107
#define CS_DIAG_ENOTSAFE          0x0000F108
#define CS_DIAG_EOVERFLOW         0x0000F109
#define CS_DIAG_EJRNDAMAGE        0x0000F10A
#define CS_DIAG_EJRNINACTIVE      0x0000F10B
#define CS_DIAG_EJRNRCVSPC        0x0000F10C
#define CS_DIAG_EJRNRMT           0x0000F10D
#define CS_DIAG_ENEWJRNRCV        0x0000F10E
#define CS_DIAG_ENEWJRN           0x0000F10F
#define CS_DIAG_EJOURNALED        0x0000F110
#define CS_DIAG_EJRNENTTOOLONG    0x0000F111
#define CS_DIAG_EDATALINK         0x0000F112
#define CS_DIAG_ENOTAVAIL         0x0000F113
#define CS_DIAG_ENOTTY            0x0000F114
#define CS_DIAG_EFBIG2            0x0000F115
#define CS_DIAG_ETXTBSY           0x0000F116
#define CS_DIAG_EASPGRPNOTSET     0x0000F117
#define CS_DIAG_ERESTART          0x0000F118
#define CS_DIAG_ESCANFAILURE      0x0000F119

#define CS_DIAG_UNKNOWN           0x0000FFFF

typedef long CSRESULT;

#endif
