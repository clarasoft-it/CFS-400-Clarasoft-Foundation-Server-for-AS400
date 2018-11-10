/* ==========================================================================
  Clarasoft Core Tools
  csstr.c
  string implementation
  Version 1.0.0
  
  Compile module with:
     CRTCMOD MODULE(CSSTR) SRCFILE(QCSRC) DBGVIEW(*ALL)
     
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
  CRTCMOD MODULE(CSSTR) SRCFILE(QCSRC) DBGVIEW(*ALL)
========================================================================== */

#include <ctype.h>
#include <errno.h>
#include <iconv.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qcsrc/cscore.h"
#include "qcsrc/cslist.h"

#define B64_MASK_11                    (0xFC)
#define B64_MASK_12                    (0x03)
#define B64_MASK_21                    (0xF0)
#define B64_MASK_22                    (0x0F)
#define B64_MASK_31                    (0xC0)
#define B64_MASK_32                    (0x3F)

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

#define CS_OPER_ICONV                  (0x0F010000)
#define CS_OPER_CSSTRCV                (0x00020000)

#define CSUTF8_BYTE1                   (0x00)
#define CSUTF8_BYTE2                   (0xC0)
#define CSUTF8_BYTE3                   (0xE0)
#define CSUTF8_BYTE4                   (0xF0)

#define CSUTF8_ANDMASK_2               (0xE0)
#define CSUTF8_ANDMASK_3               (0xF0)
#define CSUTF8_ANDMASK_4               (0xF8)

#define CSUTF8_ISCODE_2(x) (((x) & CSUTF8_ANDMASK_2) == CSUTF8_BYTE2)
#define CSUTF8_ISCODE_3(x) (((x) & CSUTF8_ANDMASK_3) == CSUTF8_BYTE3)
#define CSUTF8_ISCODE_4(x) (((x) & CSUTF8_ANDMASK_4) == CSUTF8_BYTE4)

#define CSUTF8_CHARWIDTH(x) CSUTF8_ISCODE_2(x) ? 2 : \
                            CSUTF8_ISCODE_3(x) ? 3 : \
                            CSUTF8_ISCODE_4(x) ? 4 : 1

typedef struct tagCSSTRCV {

   CSLIST Tokens;
   char szFromCCSID[6];
   char szToCCSID[6];
   long bufferSize;
   long numOfChars;
   char leftOver[4];
   int leftOverSize;
   int missingSize;
   iconv_t cd;

}CSSTRCV;

CSSTRCV*
  CSSTRCV_Constructor
    (void);

CSRESULT
  CSSTRCV_Destructor
    (CSSTRCV** This);

long
  CSSTRCV_Get
    (CSSTRCV* This,
     char*  outBuffer);

CSRESULT
  CSSTRCV_SetConversion
    (CSSTRCV* This,
     char* szFromCCSID,
     char* szToCCSID);

long
  CSSTRCV_Size
    (CSSTRCV*);

CSRESULT
  CSSTRCV_StrCat
    (CSSTRCV* This,
     char*  inBuff,
     long   size);

CSRESULT
  CSSTRCV_StrCpy
    (CSSTRCV* This,
     char*  inBuff,
     long   size);

uint64_t
  CSSTR_FromBase64
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer);

uint64_t
  CSSTR_FromBase64Ex
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     int flags);

char*
  CSSTR_StrTok
    (char* szBuffer,
     char*szDelimiter);

uint64_t
  CSSTR_ToBase64
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer);

uint64_t
  CSSTR_ToBase64Ex
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     int flags);

int
  CSSTR_ToLowerCase
    (char* buffer,
     int size);

int
  CSSTR_ToUpperCase
    (char* buffer,
     int size);

long
  CSSTR_Trim
    (char* szSource,
     char* szTarget);

uint64_t
  CSSTR_UrlDecode
    (unsigned char* in,
     unsigned char* out);

uint64_t
  CSSTR_UrlEncode
    (unsigned char* in,
     unsigned char* out,
     int flags);

CSRESULT
  CSSTRCV_PRV_Convert
    (CSSTRCV*  This,
     char*     strFrom,
     size_t    strFromMaxSize,
     size_t*   strFromConvertedSize,
     char*     strTo,
     size_t    strToMaxSize,
     size_t*   strToConvertedSize);

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_Constructor
//
// This function creates a CSSTRCV instance.
//
//////////////////////////////////////////////////////////////////////////////

CSSTRCV*
  CSSTRCV_Constructor
    (void) {

   CSSTRCV* This;
   CSRESULT hResult;

   This = (CSSTRCV*)malloc(sizeof(CSSTRCV));
   memset(This, 0, sizeof(CSSTRCV));

   This->Tokens = CSLIST_Constructor();

   return This;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_Destructor
//
// This function releases the resources of a CSSTRCV instance.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_Destructor
    (CSSTRCV** This) {

   CSLIST_Destructor(&((*This)->Tokens));
   free(*This);
   *This = 0;

   return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_Get
//
// This function copies the converted string in a supplied buffer.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_Get
    (CSSTRCV* This,
     char* outBuffer) {

   long i;
   long iCount;
   long offset;
   long getSize;

   iCount = CSLIST_Count(This->Tokens);

   for (i=0, offset=0; i< iCount; i++, offset += getSize) {

       getSize = CSLIST_ItemSize(This->Tokens, i);
       CSLIST_Get(This->Tokens, outBuffer+offset, &getSize, i);
   }

   return CS_SUCCESS;
}

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
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_SetConversion
    (CSSTRCV* This,
     char* szFromCCSID,
     char* szToCCSID) {

  char szFromCodeStruct[32];
  char szToCodeStruct[32];

  strcpy(This->szFromCCSID, szFromCCSID);
  strcpy(This->szToCCSID, szToCCSID);

  memset(szFromCodeStruct, '0', 32);
  memset(szToCodeStruct,   '0', 32);

  memcpy(&szFromCodeStruct[0], "IBMCCSID", 8);

  memcpy(&szFromCodeStruct[8], szFromCCSID, 5);

  memcpy(&szToCodeStruct[0], "IBMCCSID", 8);

  memcpy(&szToCodeStruct[8], szToCCSID, 5);

  // Close former conversion descriptor
  iconv_close(This->cd);

  This->cd = iconv_open(szToCodeStruct, szFromCodeStruct);

  if (This->cd.return_value == -1) {

     This->szFromCCSID[0] = 0;
     This->szToCCSID[0] = 0;
     return CS_FAILURE;
  }

  // Release the previous string buffer

  CSLIST_Clear(This->Tokens);

  This->bufferSize   = 0;
  This->numOfChars   = 0;
  This->missingSize  = 0;
  This->leftOver[0]  = 0;
  This->leftOverSize = 0;

  return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_Size
//
// This function returns the number of bytes in the converted string.
//
//////////////////////////////////////////////////////////////////////////////

long
  CSSTRCV_Size
    (CSSTRCV* This) {

   return This->bufferSize;
}

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
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_StrCat
    (CSSTRCV* This,
     char* inBuffer,
     long size)
{

   char* outBuffer;

   size_t inSize;
   size_t outSize;
   size_t InConvSize;
   size_t OutConvSize;
   size_t remainingSize;
   size_t offsetOut;
   size_t offsetIn;

   int i;
   int w;

   CSRESULT hResult;

   inSize = size;
   offsetIn = 0;

   // This insures enough space and accounts for the leftover partial bytes
   // from a previous copy or concatenation
   outSize = size * 4 + This->leftOverSize;

   outBuffer = (char*)malloc(outSize * sizeof(char));

   // output buffer starting position
   offsetOut = 0;

   // translate partial character if any

   if (This->leftOverSize > 0) {

      if (This->missingSize > inSize) {

         // We do not have all the remaining character bytes:
         // we will store what we got...

         for (i=0; i<inSize; i++) {
            This->leftOver[This->leftOverSize+i] = inBuffer[i];
         }

         This->leftOverSize += i;
         This->missingSize -= i;

         return CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EINVAL;
      }
      else {

         switch(This->missingSize) {
            case 1:
              This->leftOver[This->leftOverSize] = inBuffer[0];
              offsetIn = 1;
              break;
            case 2:
              This->leftOver[This->leftOverSize] = inBuffer[0];
              This->leftOver[This->leftOverSize+1] = inBuffer[1];
              offsetIn = 2;
              break;
            case 3:
              This->leftOver[This->leftOverSize] = inBuffer[0];
              This->leftOver[This->leftOverSize+1] = inBuffer[1];
              This->leftOver[This->leftOverSize+2] = inBuffer[2];
              offsetIn = 3;
              break;
         }

         hResult = CSSTRCV_PRV_Convert(This,
                                       This->leftOver,
                                       This->leftOverSize +
                                           This->missingSize,
                                       &InConvSize,
                                       outBuffer,
                                       outSize,
                                       &OutConvSize);

         if (CS_FAIL(hResult)) {

            // This means the supplied buffer does not hold the
            // rest of the partially supplied character from the
            // previous copy or concatenation and the leftover
            // character is invalid

            return CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG_EILSEQ;
         }
         else {

            // Reset partial character buffer

            This->missingSize  = 0;
            This->leftOver[0]  = 0;
            This->leftOverSize = 0;

             // Add the completed character

             CSLIST_Insert(This->Tokens,
                           outBuffer, OutConvSize, CSLIST_BOTTOM);

            // Adjust the output buffer size
            This->bufferSize += OutConvSize;
         }

         if ((inSize - offsetIn) <= 0) {

            // We only got the remaining partial character as the input;
            // no more conversion is required.

            return CS_SUCCESS;
         }
      }
   }

   hResult = CSSTRCV_PRV_Convert(This,
                                 inBuffer + offsetIn,
                                 inSize - offsetIn,
                                 &InConvSize,
                                 outBuffer,
                                 outSize,
                                 &OutConvSize);

   if (CS_FAIL(hResult)) {

      if (CS_DIAG(hResult) == CS_DIAG_EINVAL) {

         /////////////////////////////////////////////////////////////////////
         // This means the conversion stopped when it encountered
         // a partial character; we will store the partial character
         // and assume the remaining bytes will be provided
         // in the next call to CSUTF8_StrCat.
         /////////////////////////////////////////////////////////////////////

         This->leftOver[0] = inBuffer[InConvSize];

         // Add converted string to tokens, if any

         if (OutConvSize > 0) {

           This->bufferSize += OutConvSize;

            CSLIST_Insert(This->Tokens,
                          outBuffer, OutConvSize, CSLIST_BOTTOM);
         }

         //////////////////////////////////////////////////////////////
         // Note that we should never get a character width of 1.
         // Also, we don't care if the character width is 2 since it
         // is partially present, we only have the first byte which
         // we have copied already. What happens next depends on the
         // target character set. At present, this class supports
         // ASCII, UTF8 and EBCDIC. We must perform specific
         // processing according to the current target CCSID.
         //////////////////////////////////////////////////////////////

         This->leftOverSize = inSize - InConvSize;

         w = CSUTF8_CHARWIDTH(This->leftOver[0]);

         This->missingSize =
            w - This->leftOverSize;

         if (This->leftOverSize > 1) {

            // Copy partial byte following first character byte

            This->leftOver[1] = inBuffer[InConvSize+1];

            if (CSUTF8_CHARWIDTH(inBuffer[InConvSize]) == 4) {

               if (This->leftOverSize > 2) {

                  This->leftOver[2] = inBuffer[InConvSize+2];
               }
            }
         }
      }
      else {

         // This is a fatal error

      }

      hResult = CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG(hResult);
   }
   else {

      // Everything got converted

      This->bufferSize += OutConvSize;

      // Add converted string to tokens

      CSLIST_Insert(This->Tokens,
                    outBuffer, OutConvSize, CSLIST_BOTTOM);
   }

   free(outBuffer);

   return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_StrCpy
//
// This function starts a new set of conversion inputs. It uses the
// the source/target CCSIDs specified from a previous call to the
// CSSTR_SetConversion() function.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_StrCpy
    (CSSTRCV* This,
     char* inBuffer,
     long size)
{

   char* outBuffer;

   size_t inSize;
   size_t outSize;
   size_t InConvSize;
   size_t OutConvSize;

   int w;

   CSRESULT hResult;

   // Reset instance

   CSLIST_Clear(This->Tokens);

   This->bufferSize   = 0;
   This->numOfChars   = 0;
   This->missingSize  = 0;
   This->leftOver[0]  = 0;
   This->leftOverSize = 0;

   inSize = size;

   outSize = size * 4;  // This insures enough space
   outBuffer = (char*)malloc(outSize * sizeof(char));

   hResult =  CSSTRCV_PRV_Convert(This,
                                  inBuffer,
                                  inSize,
                                  &InConvSize,
                                  outBuffer,
                                  outSize,
                                  &OutConvSize);

   if (CS_FAIL(hResult)) {

      if (CS_DIAG(hResult) == CS_DIAG_EINVAL) {

        /////////////////////////////////////////////////////////////////////
        // This means the conversion stopped when it encountered
        // a partial character; we will store the partial character
        // and assume the remaining bytes will be provided
        // in the next call to CSUTF8_StrCat.
        /////////////////////////////////////////////////////////////////////

        This->leftOver[0] = inBuffer[InConvSize];

        // Add converted string to tokens, if any

        if (OutConvSize > 0) {

           This->bufferSize = OutConvSize;

           CSLIST_Insert(This->Tokens,
                         outBuffer, OutConvSize, CSLIST_BOTTOM);
        }

        This->leftOverSize = inSize - InConvSize;

        w = CSUTF8_CHARWIDTH(This->leftOver[0]);

        This->missingSize =
        w - This->leftOverSize;

        if (This->leftOverSize > 1) {

          // Copy partial byte following first character byte

          This->leftOver[1] = inBuffer[InConvSize+1];

          if (CSUTF8_CHARWIDTH(inBuffer[InConvSize]) == 4) {

            if (This->leftOverSize > 2) {

              This->leftOver[2] = inBuffer[InConvSize+2];
            }
          }
        }
      }
      else {

         // This is a fatal error

      }

      hResult = CS_FAILURE | CS_OPER_CSSTRCV | CS_DIAG(hResult);
   }
   else {

      // Everything got converted

      This->bufferSize = OutConvSize;

      // Add converted string to tokens

      CSLIST_Insert(This->Tokens,
                    outBuffer, OutConvSize, CSLIST_BOTTOM);
   }

   free(outBuffer);

   return hResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTRCV_PRV_Convert
//
// This function performs the conversion from one CCSID to another.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSTRCV_PRV_Convert
    (CSSTRCV* This,
     char*    strFrom,
     size_t   strFromMaxSize,
     size_t*  strFromConvertedSize,
     char*    strTo,
     size_t   strToMaxSize,
     size_t*  strToConvertedSize)
{
  int retcode;

  CSRESULT rc;

  *strFromConvertedSize = strFromMaxSize;
  *strToConvertedSize = strToMaxSize;

  retcode = iconv(This->cd,
                  &strFrom,
                  strFromConvertedSize,
                  &strTo,
                  strToConvertedSize);

  *strFromConvertedSize = strFromMaxSize - *strFromConvertedSize;

  *strToConvertedSize = *strToConvertedSize < strToMaxSize ?
                        strToMaxSize - *strToConvertedSize :
                        0;

  if (retcode >= 0) {

     rc = CS_SUCCESS;

  }
  else {

     switch(errno) {

        case EILSEQ:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EILSEQ;
           break;
        case E2BIG:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_E2BIG;
           break;
        case EINVAL:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EINVAL;
           break;
        case EBADF:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EBADF;
           break;
        case EBADDATA:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EBADDATA;
           break;
        case ECONVERT:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_ECONVERT;
           break;
        case EFAULT:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EFAULT;
           break;
        case ENOBUFS:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_ENOBUFS;
           break;
        case ENOMEM:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_ENOMEM;
           break;
        case EUNKNOWN:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EUNKNOWN;
           break;
        default:
           rc = CS_FAILURE | CS_OPER_ICONV | CS_DIAG_UNKNOWN;
           break;
     }
  }

  return rc;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_FromBase64
//
// Converts an Base64 string to its ASCII equivalent.
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_FromBase64
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer) {

    uint64_t i;
    uint64_t j;
    uint64_t trailing;

    ///////////////////////////////////////////////////////////
    //
    // The following table holds the indices of the
    // B64EncodeTable values that are valid B64 characters.
    // All other indices are set to -1, which is an
    // invalid array index. If a character in the B64
    // string resolves to an index value of -1, this means
    // that the B64 string is actually not a B64 string
    // because it has a character that falls outside the
    // values in the B64EncodeTable table.
    //
    ///////////////////////////////////////////////////////////

    char B64DecodeTable[256] = {
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  62,  -1,  -1,  -1,  63,
        52,  53,  54,  55,  56,  57,  58,  59,
        60,  61,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,   0,   1,   2,   3,   4,   5,   6,
         7,   8,   9,  10,  11,  12,  13,  14,
        15,  16,  17,  18,  19,  20,  21,  22,
        23,  24,  25,  -1,  -1,  -1,  -1,  -1,
        -1,  26,  27,  28,  29,  30,  31,  32,
        33,  34,  35,  36,  37,  38,  39,  40,
        41,  42,  43,  44,  45,  46,  47,  48,
        49,  50,  51,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1   -1
    };

   // Determine actual size by ignoring padding characters

   //////////////////////////////////////////////////////////////////
   // We could have an overflow... the loop
   // increments the input buffer index within
   // the loop, which holds the potential to
   // overflow the actual input buffer size.
   // We know for a fact that we at most process
   // 3 byte blocks; so to insure that we don<t
   // overflow beyond, we will set the loop
   // condition on a 4 byte boundary. We can then
   // have leftover bytes that need to be processed
   // so we will compute how many extra bytes (no
   // more than 2) after the main loop.
   //////////////////////////////////////////////////////////////////

   if (inBuffer[inSize-1] == '=') {
       inSize--;
   }

   if (inBuffer[inSize-1] == '=') {
       inSize--;
   }

   trailing = inSize % 4;
   inSize = inSize - trailing;

   for (i=0, j=0; i<inSize; i++, j++) {

       //byte 0
       outBuffer[j] = B64DecodeTable[inBuffer[i]] << 2;
       i++;
       outBuffer[j] |= (B64DecodeTable[inBuffer[i]]  >> 4);

       //byte 1
       j++;
       outBuffer[j] = (B64DecodeTable[inBuffer[i]]  << 4);
       i++;
       outBuffer[j] |= (B64DecodeTable[inBuffer[i]]) >> 2;

       // byte 2
       j++;
       outBuffer[j] = (B64DecodeTable[inBuffer[i]]  << 6);
       i++;
       outBuffer[j] |= (B64DecodeTable[inBuffer[i]]);
   }

   switch(trailing) {

   case 1:

       outBuffer[j] = B64DecodeTable[inBuffer[i]] << 2;

       break;

   case 2:

       outBuffer[j] = B64DecodeTable[inBuffer[i]] << 2;
       i++;
       outBuffer[j] |= (B64DecodeTable[inBuffer[i]]  >> 4);
       j++;
       outBuffer[j] = (B64DecodeTable[inBuffer[i]]  << 4);

       break;

   case 3:

       outBuffer[j] = B64DecodeTable[inBuffer[i]] << 2;
       i++;
       outBuffer[j] |= (B64DecodeTable[inBuffer[i]]  >> 4);
       j++;
       outBuffer[j] = (B64DecodeTable[inBuffer[i]]  << 4);
       i++;
       outBuffer[j] |= (B64DecodeTable[inBuffer[i]]) >> 2;
       j++;
       outBuffer[j] = (B64DecodeTable[inBuffer[i]]  << 6);

       break;
   }

   outBuffer[j] = 0;
   return j;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_FromBase64Ex
//
// Converts an Base64 string to its ASCII equivalent by possibly ignoring
// invalid characters (or not) in the Base64 string. Some Base64 strings
// contain line breaks and those are invalid characters. Calling this
// function by ignoring invalid characters will skip over those line breaks.
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_FromBase64Ex
    (unsigned char* inBuffer,
     uint64_t   inSize,
     unsigned char* outBuffer,
     int flags) {

    uint64_t i;
    uint64_t j;

    ///////////////////////////////////////////////////////////
    //
    // The following table holds the indices of the
    // B64EncodeTable values that are valid B64 characters.
    // All other indices are set to -1, which is an
    // invalid array index. If a character in the B64
    // string resolves to an index value of -1, this means
    // that the B64 string is actually not a B64 string
    // because it has a character that falls outside the
    // values in the B64EncodeTable table.
    //
    ///////////////////////////////////////////////////////////

    char B64DecodeTable[256] = {
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  62,  -1,  -1,  -1,  63,
        52,  53,  54,  55,  56,  57,  58,  59,
        60,  61,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,   0,   1,   2,   3,   4,   5,   6,
         7,   8,   9,  10,  11,  12,  13,  14,
        15,  16,  17,  18,  19,  20,  21,  22,
        23,  24,  25,  -1,  -1,  -1,  -1,  -1,
        -1,  26,  27,  28,  29,  30,  31,  32,
        33,  34,  35,  36,  37,  38,  39,  40,
        41,  42,  43,  44,  45,  46,  47,  48,
        49,  50,  51,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1   -1
    };

    if (inBuffer[inSize-1] == '=') {
       inSize--;
    }

    if (inBuffer[inSize-1] == '=') {
       inSize--;
    }

    if (CSSTR_B64_IGNOREINVALIDCHAR & flags) {

        for (i=0, j=0; i<inSize; i++, j++) {

            while ((inBuffer[i] == '\r') || (inBuffer[i] == '\n')) {
                   i++;
            }
            if (i == inSize) {
               break;
            }
            while (B64DecodeTable[inBuffer[i]] == -1) {
                   i++;
            }
            if (i == inSize) {
               break;
            }

            //byte 0
            outBuffer[j] = B64DecodeTable[inBuffer[i]] << 2;
            i++;

            while ((inBuffer[i] == '\r') || (inBuffer[i] == '\n')) {
                   i++;
            }
            if (i == inSize) {
               break;
            }
            while (B64DecodeTable[inBuffer[i]] == -1) {
               i++;
            }
            if (i == inSize) {
               break;
            }

            outBuffer[j] |= (B64DecodeTable[inBuffer[i]]  >> 4);

            //byte 1
            j++;
            outBuffer[j] = (B64DecodeTable[inBuffer[i]]  << 4);
            i++;

            while ((inBuffer[i] == '\r') || (inBuffer[i] == '\n')) {
                   i++;
            }
            if (i == inSize) {
               break;
            }
            while (B64DecodeTable[inBuffer[i]] == -1) {
               i++;
            }
            if (i == inSize) {
               break;
            }

            outBuffer[j] |= (B64DecodeTable[inBuffer[i]]) >> 2;

            // byte 2
            j++;
            outBuffer[j] = (B64DecodeTable[inBuffer[i]]  << 6);
            i++;

            while ((inBuffer[i] == '\r') || (inBuffer[i] == '\n')) {
                   i++;
            }
            if (i == inSize) {
               break;
            }
            while (B64DecodeTable[inBuffer[i]] == -1) {
               i++;
            }
            if (i == inSize) {
               break;
            }

            outBuffer[j] |= (B64DecodeTable[inBuffer[i]]);
        }
    }
    else {

        for (i=0, j=0; i<inSize; i++, j++) {

            while ((inBuffer[i] == '\r') || (inBuffer[i] == '\n')) {
                   i++;
            }
            if (i == inSize) {
               break;
            }

            if (B64DecodeTable[inBuffer[i]] == -1) {
                outBuffer[0] = 0;
                return -1;
            }

            //byte 0
            outBuffer[j] = B64DecodeTable[inBuffer[i]] << 2;
            i++;

            while ((inBuffer[i] == '\r') || (inBuffer[i] == '\n')) {
               i++;
            }
            if (i == inSize) {
               break;
            }

            if (B64DecodeTable[inBuffer[i]] == -1) {
                outBuffer[0] = 0;
                return -1;
            }

            outBuffer[j] |= (B64DecodeTable[inBuffer[i]]  >> 4);

            //byte 1
            j++;
            outBuffer[j] = (B64DecodeTable[inBuffer[i]]  << 4);
            i++;

            while ((inBuffer[i] == '\r') || (inBuffer[i] == '\n')) {
               i++;
            }
            if (i == inSize) {
               break;
            }

            if (B64DecodeTable[inBuffer[i]] == -1) {
                outBuffer[0] = 0;
                return -1;
            }

            outBuffer[j] |= (B64DecodeTable[inBuffer[i]]) >> 2;

            // byte 2
            j++;
            outBuffer[j] = (B64DecodeTable[inBuffer[i]]  << 6);
            i++;

            while ((inBuffer[i] == '\r') || (inBuffer[i] == '\n')) {
               i++;
            }
            if (i == inSize) {
               break;
            }

            if (B64DecodeTable[inBuffer[i]] == -1) {
                outBuffer[0] = 0;
                return -1;
            }

            outBuffer[j] |= (B64DecodeTable[inBuffer[i]]);
        }
    }

    outBuffer[j] = 0;
    return j;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_StrTok
//
// This function is like the strtok function but can handle
// delimiters more than one character in length.
// Like the strtok function, the input buffer will be
// modified by the function.
//
//////////////////////////////////////////////////////////////////////////////

char*
  CSSTR_StrTok
    (char* szBuffer,
     char* szDelimiter)
{
  long i, k, found;
  long startDelim;
  long delimLength;

  static char* nextToken;

  char* token;

  if (szBuffer != 0)
  {
    nextToken = szBuffer;
    token = 0;
  }
  else
  {
    token = nextToken;
  }

  if (nextToken != 0)
  {
    delimLength = strlen(szDelimiter);
    found = 0;

    i=0;
    while (*(nextToken + i) != 0)
    {
      if (*(nextToken + i) == szDelimiter[0])
      {
        startDelim = i;

        k=0;
        while(*(nextToken + i) != 0 &&
              *(nextToken + i) == szDelimiter[k] &&
              k<delimLength)
        {
          k++;
          i++;
        }

        if (k == delimLength)
        {
          // this means we have found a delimiter
          found = 1;

          // null the delimiter's first char
          *(nextToken + startDelim) = 0;

          break;
        }
      }
      else
      {
        i++;
      }
    }

    if (found == 0)
    {
      // this means there are no more tokens
      nextToken = 0;
    }
    else
    {
      token = nextToken;

      if (*(nextToken + i) == 0)
      {
        // empty string following the delimiter
        nextToken = 0;
      }
      else
      {
        nextToken = nextToken + i;
      }
    }
  }
  else
  {
    token = 0;
  }

  return token;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_ToBase64
//
// Converts an ASCII string to its Base64 equivalent.
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_ToBase64
    (unsigned char* inBuffer,
     uint64_t inSize,
     unsigned char* outBuffer) {

    uint64_t i;
    uint64_t j;
    uint64_t trailing;

   char tempByte;

   /////////////////////////////////////////////////////////////////////////
   // These are the ASCII codes for Base 64 encoding
   /////////////////////////////////////////////////////////////////////////

   char B64EncodeTable[64] = {

      65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
      75,  76,  77,  78,  79,  80,  81,  82,  83,  84,
      85,  86,  87,  88,  89,  90,  97,  98,  99, 100,
     101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
     111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
     121, 122,  48,  49,  50,  51,  52,  53,  54,  55,
     56,  57,  43,  47

   };

   // Determine how many trailing bytes

   trailing = inSize % 3;

   inSize = inSize - trailing;

   for (i=0, j=0; i<inSize; i++, j++) {

      outBuffer[j] = B64EncodeTable[(inBuffer[i] & B64_MASK_11) >> 2];
      tempByte = (inBuffer[i] & B64_MASK_12) << 4;
      j++; i++;

      outBuffer[j] = B64EncodeTable[((inBuffer[i] & B64_MASK_21) >> 4)
       | tempByte];
      tempByte = (inBuffer[i] & B64_MASK_22) << 2;
      j++; i++;

      outBuffer[j] = B64EncodeTable[((inBuffer[i] & B64_MASK_31) >> 6)
       | tempByte];
      j++;
      outBuffer[j] = B64EncodeTable[inBuffer[i] & B64_MASK_32];
   }

   switch(trailing) {

      case 1:

        outBuffer[j] = B64EncodeTable[(inBuffer[i] & B64_MASK_11) >> 2];
        tempByte = (inBuffer[i] & B64_MASK_12) << 4;
        j++; i++;

        outBuffer[j] = B64EncodeTable[tempByte | 0];
        j++;

        outBuffer[j] = 61; //'=';
        j++;
        outBuffer[j] = 61; //'=';
        j++;

        break;

      case 2:

        outBuffer[j] = B64EncodeTable[(inBuffer[i] & B64_MASK_11) >> 2];
        tempByte = (inBuffer[i] & B64_MASK_12) << 4;
        j++; i++;

        outBuffer[j] = B64EncodeTable[((inBuffer[i] & B64_MASK_21) >> 4)
         | tempByte];
        tempByte = (inBuffer[i] & B64_MASK_22) << 2;
        j++; i++;

        outBuffer[j] = B64EncodeTable[tempByte | 0];
        j++;

        outBuffer[j] = 61; //'=';
        j++;

        break;
   }

   outBuffer[j] = 0;

   return j;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_ToBase64Ex
//
// Converts an ASCII string to its Base64 equivalent by possibly taking
// into account the line break offset standard.
//
//////////////////////////////////////////////////////////////////////////////

 uint64_t
   CSSTR_ToBase64Ex
     (unsigned char* inBuffer,
      uint64_t inSize,
      unsigned char* outBuffer,
      int flags) {

    uint64_t i;
    uint64_t j;
    uint64_t trailing;

   char tempByte;

   //////////////////////////////////////////////////////////////////////////
   // These are the ASCII codes for Base 64 encoding
   //////////////////////////////////////////////////////////////////////////

   char B64EncodeTable[64] = {

      65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
      75,  76,  77,  78,  79,  80,  81,  82,  83,  84,
      85,  86,  87,  88,  89,  90,  97,  98,  99, 100,
     101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
     111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
     121, 122,  48,  49,  50,  51,  52,  53,  54,  55,
     56,  57,  43,  47

   };

   // Determine how many trailing bytes

   trailing = inSize % 3;

   inSize = inSize - trailing;

   for (i=0, j=0; i<inSize; i++, j++) {

      if ((j%CSSTR_B64_LINEBREAK_OFFSET)
         == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

      outBuffer[j] = B64EncodeTable[(inBuffer[i] & B64_MASK_11) >> 2];
      tempByte = (inBuffer[i] & B64_MASK_12) << 4;
      j++; i++;

      if ((j%CSSTR_B64_LINEBREAK_OFFSET)
         == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

      outBuffer[j] = B64EncodeTable[((inBuffer[i] & B64_MASK_21) >> 4)
                     | tempByte];
      tempByte = (inBuffer[i] & B64_MASK_22) << 2;
      j++; i++;

      if ((j%CSSTR_B64_LINEBREAK_OFFSET) == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

      outBuffer[j] = B64EncodeTable[((inBuffer[i] & B64_MASK_31) >> 6)
                     | tempByte];
      j++;

      if ((j%CSSTR_B64_LINEBREAK_OFFSET) == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

      outBuffer[j] = B64EncodeTable[inBuffer[i] & B64_MASK_32];
   }

   switch(trailing) {

      case 1:

        outBuffer[j] = B64EncodeTable[(inBuffer[i] & B64_MASK_11) >> 2];
        tempByte = (inBuffer[i] & B64_MASK_12) << 4;
        j++; i++;

      if ((j%CSSTR_B64_LINEBREAK_OFFSET) == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

        outBuffer[j] = B64EncodeTable[tempByte | 0];
        j++;

      if ((j%CSSTR_B64_LINEBREAK_OFFSET) == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

        outBuffer[j] = 61; //'=';
        j++;

      if ((j%CSSTR_B64_LINEBREAK_OFFSET) == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

        outBuffer[j] = 61; //'=';
        j++;

        break;

      case 2:

        outBuffer[j] = B64EncodeTable[(inBuffer[i] & B64_MASK_11) >> 2];
        tempByte = (inBuffer[i] & B64_MASK_12) << 4;
        j++; i++;

      if ((j%CSSTR_B64_LINEBREAK_OFFSET) == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

        outBuffer[j] = B64EncodeTable[((inBuffer[i] & B64_MASK_21) >> 4)
                       | tempByte];

        tempByte = (inBuffer[i] & B64_MASK_22) << 2;
        j++; i++;

      if ((j%CSSTR_B64_LINEBREAK_OFFSET) == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

        outBuffer[j] = B64EncodeTable[tempByte | 0];
        j++;

      if ((j%CSSTR_B64_LINEBREAK_OFFSET) == (CSSTR_B64_LINEBREAK_OFFSET-1)) {

          switch(flags & CSSTR_B64_MASK_LINEBREAK) {

              case CSSTR_B64_LINEBREAK_LF:

                outBuffer[j] = '\n';
                j++;
                break;

              case CSSTR_B64_LINEBREAK_CRLF:

                outBuffer[j] = '\r';
                j++;
                outBuffer[j] = '\n';
                j++;
                break;
          }
      }

        outBuffer[j] = 61; //'=';
        j++;

        break;
   }

   outBuffer[j] = 0;

   return j;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_toLowerCase
//
// Converts a string from upper case to lower case.
//
//////////////////////////////////////////////////////////////////////////////

int
  CSSTR_ToLowerCase
    (char* buffer,
    int size) {

   int i = 0;

   while(buffer[i] && i < size)
   {
      buffer[i] = tolower(buffer[i]);
      i++;
   }

   return i;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_toUpperCase
//
// Converts a string from lower case to upper case.
//
//////////////////////////////////////////////////////////////////////////////

int
  CSSTR_ToUpperCase
    (char* buffer,
     int size) {

   int i = 0;

   while(buffer[i] && i < size)
   {
      buffer[i] = toupper(buffer[i]);
      i++;
   }

   return i;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_Trim
//
// Removes blanks from a string; the original string remains
// unaffected.
//
//////////////////////////////////////////////////////////////////////////////

long
  CSSTR_Trim
    (char* source,
     char* target) {

  long i, j, k;

  if (target == 0 || source == 0)
  {
      return 0;
  }

  i=0;
  while(*(source + i) != 0) {

    if (*(source + i) == ' ')
    {
      i++;
    }
    else {
      break;
    }
  }

  j=i;
  while(*(source + j) != 0)
    j++;

  if (*(source + j) == 0)
    j--;

  while(*(source + j) == ' ')
    j--;

  k=0;
  while(i <= j) {
    *(target + k) = *(source + i);
    i++; k++;
  }

  *(target + k) = 0;
  return k;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_UrlDecode
//
// URL-decodes an URL-encoded ASCII string. The resulting string is in ASCII.
// The caller must insure that the resulting buffer is large enough.
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_UrlDecode
    (unsigned char* in,
     unsigned char* out) {

  uint64_t i;
  uint64_t j;

  char code[3];
  unsigned int n;

  i=0;
  j=0;
  code[2] = 0;

  while( in[i] != 0 ){

    if (in[i] == '+') {
        out[j] = ' ';
        i++;
        j++;
    }
    else {
      if (in[i] == '%')
      {
        // skip over the percent
        i++;

        // next two characters are hex value;
        // could be digits or characters a-f or A-F

        if ((in[i] >= '0' && in[i] <= '9') ||
            (in[i] >= 'A' && in[i] <= 'Z') ||
            (in[i] >= 'a' && in[i] <= 'z')) {

            code[0] = in[i];
        }
        else {
          // error
          out[0] = 0;
          return -1;
        }

        i++;

        if ((in[i] >= '0' && in[i] <= '9') ||
            (in[i] >= 'A' && in[i] <= 'Z') ||
            (in[i] >= 'a' && in[i] <= 'z')) {

            code[1] = in[i];
        }
        else {
          // error
          out[0] = 0;
          return -1;
        }

        sscanf(code, "%x", (unsigned int*)&n);
        out[j] = (unsigned char)n;
        i++;
        j++;
      }
      else {

        out[j] = in[i];
        i++;
        j++;
      }
    }
  }

  out[j] = 0;
  return j;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_UrlEncode
//
// URL-encodes an ASCII string. The resulting string is in ASCII.
//
//////////////////////////////////////////////////////////////////////////////

uint64_t
  CSSTR_UrlEncode
    (unsigned char* in,
     unsigned char* out,
     int flags) {

  uint64_t i;
  uint64_t j;

  char hex[16] = { 48, 49, 50, 51, 52, 53, 54, 55 , 56, 57,
                   65, 66, 67, 68, 69, 70 };

  i=0;
  j=0;
  while( in[i] != 0) {

    if ((97 <= in[i] && in[i] <= 122) ||   //ASCII  A to Z
        (65 <= in[i] && in[i] <= 90)  ||   //ASCII  a to z
        (48 <= in[i] && in[i] <= 57) ){    //ASCII  0 to 9

      out[j] = in[i];
    }
    else {

      if ((in[i] == 0x20) && (flags & CSSTR_URLENCODE_SPACETOPLUS)) {
        out[j] = 43;  // ASCII plus sign
      }
      else {

        switch(in[i]) {

          case 0x21:
          case 0x23:
          case 0x24:
          case 0x26:
          case 0x27:
          case 0x28:
          case 0x29:
          case 0x2A:
          case 0x2B:
          case 0x2C:
          case 0x2F:
          case 0x3A:
          case 0x3B:
          case 0x3D:
          case 0x3F:
          case 0x40:
          case 0x5B:
          case 0x5D:

            // This is one of the reserved characters ....
            // perhaps we need to keep them as is...

            if (flags & CSSTR_URLENCODE_XLATERESERVED) {

              out[j] = 37; // ASCII percent sign (%);
              j++;
              out[j] = hex[in[i] >> 4];
              j++;
              out[j] = hex[in[i] & 15];
            }
            else {

              out[j] = in[i];
            }

            break;

          default:

            out[j] = 37; // ASCII percent sign (%);
            j++;
            out[j] = hex[in[i] >> 4];
            j++;
            out[j] = hex[in[i] & 15];

            break;
        }
      }
    }

    i++;
    j++;
  }

  out[j] = 0;

  return j;
}
