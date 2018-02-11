/* ==========================================================================
  Clarasoft Core Tools
  
  csstr.h
  string utilities
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
========================================================================== */

#ifndef __CLARASOFT_CT_CSSTR_H__
#define __CLARASOFT_CT_CSSTR_H__

#include "qcsrc/cscore.h"

#define CCSID_JOBDEFAULT               (0)
#define CCSID_UTF8                     (1208)
#define CCSID_ASCII                    (819)
#define CCSID_WINDOWS_1251             (1251)
#define CCSID_ISO_8859_1               (819)

#define CSSTR_B64_LINEBREAK_OFFSET     (0x0000004D)
#define CSSTR_B64_MASK_LINEBREAK       (0x0000000F)
#define CSSTR_B64_LINEBREAK_NONE       (0x00000000)
#define CSSTR_B64_LINEBREAK_LF         (0x00000001)
#define CSSTR_B64_LINEBREAK_CRLF       (0x00000002)
#define CSSTR_B64_IGNOREINVALIDCHAR    (0x00000100)

#define CSSTR_URLENCODE_SPACETOPLUS    (0x00000100)
#define CSSTR_URLENCODE_XLATERESERVED  (0x00000200)

#define CS_OPER_CSSTRCV                (0x00020000)

typedef void* CSSTRCV;


//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_Constructor
//
// This function creates a CSSTRCV instance.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    A pointer to a CSSTRCV instance or NULL if the construction failed.
//
//////////////////////////////////////////////////////////////////////////////

CSSTRCV
  CSSTRCV_Constructor
    (void);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_Destructor
//
// This function releases the resources of a CSSTRCV instance.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: The address of a pointer to the CSSTR instance to release.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_Destructor
    (CSSTRCV* This);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_Get
//
// This function copies the converted string in a supplied buffer.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: The address of a pointer to the CSSTRCV instance.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_Get
    (CSSTRCV This,
     char*  outBuffer);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_SetConversion
//
// This function initialises a conversion. This function basically
// sets up the instance to convert from one CCSID to another.
//
// If the instance contains a previously converted buffer, this buffer
// will be discarded such that it is not possible to mix conversion target
// CCSIDs over a set of inputs
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: The address of a pointer to the CSSTRCV instance to release.
//
// iFromCCSID:  A long integer identiying the CCSID
//              of the input string to convert.
//
// iToCCSID: a long integer identiying the CCSID
//           of the out string; this is the CCSID to which the input
//           string will be converted.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS
//    CS_FAILURE
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_SetConversion
    (CSSTRCV This,
     char* szFromCCSID,
     char* szToCCSID);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_Size
//
// This function returns the number of bytes in the converted string.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: The address of a pointer to the CSSTRCV instance.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    The size in bytes in the converted string.
//
//////////////////////////////////////////////////////////////////////////////

long
  CSSTRCV_Size
    (CSSTRCV);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_StrCat
//
// This function continues a set of conversion inputs.
// the source/target CCSIDs specified from a previous call to the
// CSSTR_SetConversion() function are used for the conversion. If the
// previous input string held an incomplete character sequence, the
// conversion assumes the input buffer holds the remaining bytes and will
// try to convert this incomplete character and add it to the converted buffer
// and will carry on converting the rest of the input buffer and append it to
// the resulting converted buffer.
//
// The conversion will be carried out either until all characters
// are translated or until an invalid character is found. An invalid
// character is assumed to be a partial character that will be completed
// by a call to CSSTR_StrCat. This assumption has a side
// effect: it assumes that no data follows the partial character. This would
// cause data to be dropped. But a partial character in this case would
// equate to an invalid character and conversion should stop anyway.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: The address of a pointer to the CSSTRCV instance to release.
//
// inBuffer a pointer to the buffer to be converted. This buffer
//          needs not be null terminated since its size is provided.
//          The function does not assume the buffer is null-terminated.
//
// size: The number of bytes to convert from the input buffer. This is NOT
//       nessarily the number of characters to be converted.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS | CS_OPER_NOVALUE | CS_DIAG_NOVALUE;
//
//      All characters have been converted
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EINVAL;
//
//      A character that could not be converted was encountered in
//      the input string; all prior characters have been
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EILSEQ;
//
//      All characters have been converted
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_E2BIG;
//
//      Conversion stopped because the output buffer was not large
//      enough.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EBADF;
//
//      The conversion descriptor was not valid; possible causes
//      may be that the CSSTR_SetConversion() function was
//      not called or that the previous call returned a failure code.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EBADDATA;
//
//      Shift state not valid in input data.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_ECONVERT;
//
//      The mixed input data contained DBCS characters.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EFAULT;
//
//      Invalid parameter.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_ENOBUFS;
//
//      Number of bytes for the input or output buffer not valid,
//      or the input length cannot be determined.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_ENOMEM;
//
//      Insufficient storage space was available to perform the conversion.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EUNKNOWN;
//
//      An undetected error occurred.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_UNKNOWN;
//
//      None of the above errors occured but the conversion failed.
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_StrCat
    (CSSTRCV This,
     char*  inBuff,
     long   size);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_StrCpy
//
// This function starts a new set of conversion inputs. It uses the
// the source/target CCSIDs specified from a previous call to the
// CSSTR_SetConversion() function.
//
// If the instance contains a previously converted buffer, this buffer
// will be discarded.
//
// Note that the CSSTR_SetConversion() needs not be called before
// each call to this function so long as the source/target CCSIDs are
// the same.
//
// The conversion will be carried out either until all characters
// are translated or until an invalid character is found. An invalid
// character is assumed to be a partial character that will be completed
// by a call to CSSTR_StrCat. This assumption has a side
// effect: it assumes that no data follows the partial character. This would
// cause data to be dropped. But a partial character in this case would
// equate to an invalid character and conversion should stop anyway.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: The address of a pointer to the CSSTRCV instance to release.
//
// inBuffer a pointer to the buffer to be converted. This buffer
//          needs not be null terminated since its size is provided.
//          The function does not assume the buffer is null-terminated.
//
// size: The number of bytes to convert from the input buffer. This is NOT
//       nessarily the number of characters to be converted.
//
//
// Possible return values:
// ---------------------------------------------------------------------------
//
//    CS_SUCCESS | CS_OPER_NOVALUE | CS_DIAG_NOVALUE;
//
//      All characters have been converted
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EINVAL;
//
//      A character that could not be converted was encountered in
//      the input string; all prior characters have been
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EILSEQ;
//
//      All characters have been converted
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_E2BIG;
//
//      Conversion stopped because the output buffer was not large
//      enough.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EBADF;
//
//      The conversion descriptor was not valid; possible causes
//      may be that the CSSTR_SetConversion() function was
//      not called or that the previous call returned a failure code.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EBADDATA;
//
//      Shift state not valid in input data.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_ECONVERT;
//
//      The mixed input data contained DBCS characters.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EFAULT;
//
//      Invalid parameter.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_ENOBUFS;
//
//      Number of bytes for the input or output buffer not valid,
//      or the input length cannot be determined.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_ENOMEM;
//
//      Insufficient storage space was available to perform the conversion.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EUNKNOWN;
//
//      An undetected error occurred.
//
//    CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_UNKNOWN;
//
//      None of the above errors occured but the conversion failed.
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_StrCpy
    (CSSTRCV This,
     char*  inBuff,
     long   size);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_FromBase64
//
// Converts an Base64 string to its ASCII equivalent.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
//  inBuffer: the address of buffer to convert. This buffer is NOT a
//            null-terminated buffer because the data may be binary
//            data that contains nulls. This is why the buffer size
//            is to be provided.
//
//  inSize: the length of the input buffer.
//
//
//  outBuffer: the address of the bufffer that will hold the converted
//             string; the buffer will be null-terminated. The caller
//             must insure that the resulting buffer be large enough to
//             hold the converted string along with the null terminator.
//
// Return values:
// ---------------------------------------------------------------------------
//
//    Length of the converted string
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_FromBase64
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     uint64_t   outSize);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_FromBase64Ex
//
// Converts an Base64 string to its ASCII equivalent by possibly ignoring
// invalid characters (or not) in the Base64 string. Some Base64 strings
// contain line breaks and those are invalid characters. Calling this
// function by ignoring invalid characters will skip over those line breaks.
//
// Parameters
// ---------------------------------------------------------------------------
//
//  inBuffer: the address of buffer to convert. This buffer is NOT a
//            null-terminated buffer because the data may be binary
//            data that contains nulls. This is why the buffer size
//            is to be provided.
//
//  inSize: the length of the input buffer.
//
//
//  outBuffer: the address of the bufffer that will hold the converted
//             string; the buffer will be null-terminated. The caller
//             must insure that the resulting buffer be large enough to
//             hold the converted string along with the null terminator.
//
//  flags: indicates if invalid characters are to be ignored in the conversion.
//
//         0 (zero) : don't ignore invalid characters.
//
//         CSSTR_B64_IGNOREINVALIDCHAR: ignore invalid characters.
//
// Return values:
// ---------------------------------------------------------------------------
//
//    Length of the converted string
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_FromBase64Ex
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     uint64_t   outSize,
     int flags);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_StrTok
//
// This function is like the strtok function but can handle
// delimiters more than one charater in length.
// Like the strtok function, the input buffer will be
// modified by the function.
// Examples:
//   "", with delimter equal to :
//     function returns zero tokens;
//     return value is null.
//   "abc", with delimter equal to :
//     fucntion returns zero tokens;
//     return value is null;
//   ":", with delimter equal to :
//     fucntion returns one token;
//     return value is pointer to an empty string;
//   "abc:", with delimter equal to :
//     function returns 1 token, namely "abc";
//     return value is pointer to first token.
//   "abc::"
//     function return 2 tokens:
//       "abc" and ""
//   "abc::def"
//     function will return 3 tokens, the middle one will
//     point to an empty string
//
// Parameters
// ---------------------------------------------------------------------------
//
//
// Return values:
// ---------------------------------------------------------------------------
//
//    Address of beginning of token
//
//////////////////////////////////////////////////////////////////////////////

char*
  CSSTR_StrTok
    (char* szBuffer,
     char*szDelimiter);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_ToBase64
//
// Converts an ASCII string to its Base64 equivalent.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
//  inBuffer: the address of buffer to convert. This buffer is NOT a
//            null-terminated buffer because the data may be binary
//            data that contains nulls. This is why the buffer size
//            is to be provided.
//
//  inSize: the length of the input buffer.
//
//
//  outBuffer: the address of the bufffer that will hold the converted
//             string; the buffer will be null-terminated. The caller
//             must insure that the resulting buffer be large enough to
//             hold the converted string along with the null terminator.
//
//
// Return values:
// ---------------------------------------------------------------------------
//
//    Length of the converted string
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_ToBase64
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     uint64_t   outSize);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_ToBase64Ex
//
// Converts an ASCII string to its Base64 equivalent by possibly taking
// into account the line break offset standard.
//
// Parameters
// ---------------------------------------------------------------------------
//
//  inBuffer: the address of buffer to convert. This buffer is NOT a
//            null-terminated buffer because the data may be binary
//            data that contains nulls. This is why the buffer size
//            is to be provided.
//
//  inSize: the length of the input buffer.
//
//
//  outBuffer: the address of the bufffer that will hold the converted
//             string; the buffer will be null-terminated. The caller
//             must insure that the resulting buffer be large enough to
//             hold the converted string along with the null terminator.
//
//  flags: Indicates which line break to apply:
//
//          0 (zero): Don't insert line break
//
//          CSSTR_B64_LINEBREAK_LF : inserts a line feed only
//
//          CSSTR_B64_LINEBREAK_CRLF: inserts a carriage return
//                                    and a line feed
//
// Return values:
// ---------------------------------------------------------------------------
//
//    Length of the converted string
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_ToBase64Ex
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     uint64_t   outSize,
     int flags);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_toLowerCase
//
// Convers a string from uppercase to lowercase.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
//  buffer: the address of the null-terminated string to convert
//
//  size: the length of the string to convert. The fact that a size is
//        specified enables the caller to convert only part of an
//        actual string. The buffer address could point to somewhere
//        within the string.
//
// Return values:
// ---------------------------------------------------------------------------
//
//  The length of the converted string.
//
//////////////////////////////////////////////////////////////////////////////

int
  CSSTR_ToLowerCase
    (char* buffer,
     int size);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_toUpperCase
//
// Convers a string from lowercase to uppercase.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
//  buffer: the address of the null-terminated string to convert
//
//  size: the length of the string to convert. The fact that a size is
//        specified enables the caller to convert only part of an
//        actual string. The buffer address could point to somewhere
//        within the string.
//
// Return values:
// ---------------------------------------------------------------------------
//
//   The length of the converted string.
//
//////////////////////////////////////////////////////////////////////////////

int
  CSSTR_ToUpperCase
    (char* buffer,
     int size);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_Trim
//
// Removes blanks from a string; the original string remains
// unaffected.
//
// Parameters
// ---------------------------------------------------------------------------
//
//  source: the address of the null-terminated string to trim.
//
//  target: the address of the resulting buffer (will be null-terminated).
//
// Return values:
// ---------------------------------------------------------------------------
//
//   Length of resulting string
//
//////////////////////////////////////////////////////////////////////////////

long
  CSSTR_Trim
    (char* szSource,
     char* szTarget);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_UrlDecode
//
// URL-decodes an URL-encoded ASCII string. The resulting string is in ASCII.
// The caller must insure that the resulting buffer is large enough.
//
// Parameters
// ---------------------------------------------------------------------------
//
//  in: the address of the null-terminated URL-encoded string.
//
//  out: the address of the receiving buffer. The caller must insure that
//       the resulting  buffer is largeg enough to recieve the encoded string
//       or else an overflow may occur.
//
// Return values:
// ---------------------------------------------------------------------------
//
//   Length of the rsulting string
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_UrlDecode
    (unsigned char* in,
     unsigned char* out);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_UrlEncode
//
// URL-encodes an ASCII string. The resulting string is in ASCII.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
//  in: the address of the null-terminated ASCII string to encode
//
//  out: the address of the receiving buffer. This buffer will contain
//       the URL-encoded string. The caller must insure that the resulting
//       buffer is largeg enough to recieve the encoded string or else an
//       overflow may occur.
//
// Return values:
// ---------------------------------------------------------------------------
//
//   The length of the resulting encoded string
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_UrlEncode
    (unsigned char* in,
     unsigned char* out,
     int flags);

#endif
