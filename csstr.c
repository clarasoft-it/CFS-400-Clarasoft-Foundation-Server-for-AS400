
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

#define B64_MASK_11   0xFC
#define B64_MASK_12   0x03
#define B64_MASK_21   0xF0
#define B64_MASK_22   0x0F
#define B64_MASK_31   0xC0
#define B64_MASK_32   0x3F


/////////////////////////////////////////////////////////////////////////////
// These are the ASCII codes for Base 64 encoding
/////////////////////////////////////////////////////////////////////////////

char B64EncodeTable[64] = {

   65,  66,  67,  68,  69,  70,  71,  72,
   73,  74,  75,  76,  77,  78,  79,  80,
   81,  82,  83,  84,  85,  86,  87,  88,
   89,  90,  97,  98,  99, 100, 101, 102,
  103, 104, 105, 106, 107, 108, 109, 110,
  111, 112, 113, 114, 115, 116, 117, 118,
  119, 120, 121, 122,  48,  49,  50,  51,
   52,  53,  54,  55,  56,  57,  43,  47

};


#define CCSID_JOBDEFAULT   0
#define CCSID_UTF8         1208
#define CCSID_ASCII        819

#define CS_OPER_ICONV      0x0F010000
#define CS_OPER_CSSTRCV    0x00020000

#define CSUTF8_BYTE1       (0x00)
#define CSUTF8_BYTE2       (0xC0)
#define CSUTF8_BYTE3       (0xE0)
#define CSUTF8_BYTE4       (0xF0)

#define CSUTF8_ANDMASK_2   (0xE0)
#define CSUTF8_ANDMASK_3   (0xF0)
#define CSUTF8_ANDMASK_4   (0xF8)

#define CSUTF8_ISCODE_2(x) (((x) & CSUTF8_ANDMASK_2) == CSUTF8_BYTE2)
#define CSUTF8_ISCODE_3(x) (((x) & CSUTF8_ANDMASK_3) == CSUTF8_BYTE3)
#define CSUTF8_ISCODE_4(x) (((x) & CSUTF8_ANDMASK_4) == CSUTF8_BYTE4)

#define CSUTF8_CHARWIDTH(x) CSUTF8_ISCODE_2(x) ? 2 : \
                            CSUTF8_ISCODE_3(x) ? 3 : \
                            CSUTF8_ISCODE_4(x) ? 4 : 1

typedef struct tagCSSTRCV {

   CSLIST Tokens;
   long iFromCCSID;
   long iToCCSID;
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

CSRESULT
  CSSTRCV_SetConversion
    (CSSTRCV* This,
     long iFromCCSID,
     long iToCCSID);

CSRESULT
  CSSTRCV_StrCpy
    (CSSTRCV* This,
     char*  inBuff,
     long   size);

CSRESULT
  CSSTRCV_StrCat
    (CSSTRCV* This,
     char*  inBuff,
     long   size);

long
  CSSTRCV_Size
    (CSSTRCV*);

long
  CSSTRCV_Get
    (CSSTRCV* This,
     char*  outBuffer);

CSRESULT
  CSSTRCV_PRV_Convert
    (CSSTRCV*  This,
     char*     strFrom,
     size_t    strFromMaxSize,
     size_t*   strFromConvertedSize,
     char*     strTo,
     size_t    strToMaxSize,
     size_t*   strToConvertedSize);

char*
  CSSTR_StrTok
    (char* szBuffer,
     char*szDelimiter);

long
  CSSTR_Trim
    (char* szTarget,
     char* szSource);

int
  CSSTR_ToUpperCase
    (char* buffer,
     int size);

int
  CSSTR_ToLowerCase
    (char* buffer,
     int size);

int
  CSSTR_FromBase64
    (char* inBuffer,
     int   inSize,
     char* outBuffer,
     int   outSize);

long
  CSSTR_ToBase64
    (char* inBuffer,
     int   inSize,
     char* outBuffer,
     int   outSize);

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

CSSTRCV* CSSTRCV_Constructor(void) {

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

CSRESULT CSSTRCV_Destructor(CSSTRCV** This) {

   CSLIST_Destructor(&((*This)->Tokens));
   free(*This);
   *This = 0;

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

CSRESULT CSSTRCV_SetConversion(CSSTRCV* This,
                                   long     iFromCCSID,
                                   long     iToCCSID) {

  char szFromCodeStruct[32];
  char szToCodeStruct[32];
  char szFromCCSID[6];
  char szToCCSID[6];


  This->iFromCCSID = iFromCCSID;
  This->iToCCSID = iToCCSID;

  memset(szFromCodeStruct, '0', 32);
  memset(szToCodeStruct,   '0', 32);

  memcpy(&szFromCodeStruct[0], "IBMCCSID", 8);

  switch(This->iFromCCSID) {

     case CCSID_JOBDEFAULT:
        strcpy(szFromCCSID, "00000");
        break;

     case CCSID_UTF8:
        strcpy(szFromCCSID, "01208");
        break;

     case CCSID_ASCII:
        strcpy(szFromCCSID, "00819");
        break;
  }

  memcpy(&szFromCodeStruct[8], szFromCCSID, 5);

  memcpy(&szToCodeStruct[0], "IBMCCSID", 8);

  switch(This->iToCCSID) {

     case CCSID_JOBDEFAULT:
        strcpy(szToCCSID, "00000");
        break;

     case CCSID_UTF8:
        strcpy(szToCCSID, "01208");
        break;

     case CCSID_ASCII:
        strcpy(szToCCSID, "00819");
        break;
  }

  memcpy(&szToCodeStruct[8], szToCCSID, 5);

  // Close former conversion descriptor
  iconv_close(This->cd);

  This->cd = iconv_open(szToCodeStruct, szFromCodeStruct);

  if (This->cd.return_value == -1) {

     This->iFromCCSID = -1;
     This->iToCCSID = -1;
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

CSRESULT CSSTRCV_StrCpy (CSSTRCV* This,
                         char*    inBuffer,
                         long     size)
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

         switch(This->iFromCCSID) {

            case CCSID_JOBDEFAULT:

               break;

            case CCSID_UTF8:

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

               break;

            case CCSID_ASCII:

               break;
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

CSRESULT CSSTRCV_StrCat (CSSTRCV* This,
                         char*    inBuffer,
                         long     size)
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

   switch(This->iFromCCSID) {

      case CCSID_JOBDEFAULT:

         break;

      case CCSID_UTF8:

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

         break;

      case CCSID_ASCII:

         break;
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

         switch(This->iFromCCSID) {

            case CCSID_JOBDEFAULT:

               break;

            case CCSID_UTF8:

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

               break;

            case CCSID_ASCII:

               break;
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

long CSSTRCV_Size(CSSTRCV* This) {

   return This->bufferSize;
}

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

long CSSTRCV_Get(CSSTRCV* This,
                 char*    outBuffer) {

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
// CSSTRCV_PRV_Convert
//
// This function performs the conversion from one CCSID to another.
//
//
// Parameters
// ---------------------------------------------------------------------------
//
// This: The address of a pointer to the CSSTRCV instance.
//
// strFrom: The address of the source buffer (input string) to
//          be converted.
//
// strFromMaxSize: The maximum number of bytes to convert from the
//                 input string.
//
// strFromConvertedSize: The number of bytes converted from the
//                       input string.
//
// strTo: The address of the output buffer that will hold the
//        converted string.
//
// strToMaxSize: The maximum number of bytes the output buffer
//               can hold.
//
// strToConvertedSize: The number of bytes in the output buffer
//                     that resulted from the conversion.
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
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EILSEQ;
//
//      All characters have been converted
//
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_E2BIG;
//
//      Conversion stopped because the output buffer was not large
//      enough.
//
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EBADF;
//
//      The conversion descriptor was not valid; possible causes
//      may be that the CSSTR_SetConversion() function was
//      not called or that the previous call returned a failure code.
//
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EBADDATA;
//
//      Shift state not valid in input data.
//
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_ECONVERT;
//
//      The mixed input data contained DBCS characters.
//
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EFAULT;
//
//      Invalid parameter.
//
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_ENOBUFS;
//
//      Number of bytes for the input or output buffer not valid,
//      or the input length cannot be determined.
//
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_ENOMEM;
//
//      Insufficient storage space was available to perform the conversion.
//
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_EUNKNOWN;
//
//      An undetected error occurred.
//
//    CS_FAILURE | CS_OPER_ICONV | CS_DIAG_UNKNOWN;
//
//      None of the above errors occured but the conversion failed.
//
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT CSSTRCV_PRV_Convert(CSSTRCV* This,
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

/* ----------------------------------------------------------------

 This function is like the strtok function but can handle
 delimiters more than one charater in length.

 Like the strtok function, the input buffer will be
 modified by the function.

 Examples:

   "", with delimter equal to :

     function returns zero tokens;
     return value is null.

   "abc", with delimter equal to :

     fucntion returns zero tokens;
     return value is null;

   ":", with delimter equal to :

     fucntion returns one token;
     return value is pointer to an empty string;

   "abc:", with delimter equal to :

     function returns 1 token, namely "abc";
     return value is pointer to first token.

   "abc::"

     function return 2 tokens:

       "abc" and ""

   "abc::def"

     function will return 3 tokens, the middle one will
     point to an empty string

------------------------------------------------------------------ */

char* CSSTR_StrTok(char* szBuffer,
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

          // null the delimter's first char
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

/* --------------------------------------------------------------
   CSSTR_Trim
   Removes blanks from a string; the original string remains
   unaffected.
   ----------------------------------------------------------- */

long CSSTR_Trim(char* target, char* source) {

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

/* --------------------------------------------------------------
   CSSTR_toUpperCase
   Convers a string from lowercase to uppercase.
   ----------------------------------------------------------- */

int CSSTR_ToUpperCase(char* buffer, int size) {

   int i = 0;

   while(buffer[i] && i < size)
   {
      buffer[i] = toupper(buffer[i]);
      i++;
   }

   return i;
}

/* --------------------------------------------------------------
   CSSTR_toLowerCase
   Convers a string from uppercase to lowercase.
   ----------------------------------------------------------- */

int CSSTR_ToLowerCase(char* buffer, int size) {

   int i = 0;

   while(buffer[i] && i < size)
   {
      buffer[i] = tolower(buffer[i]);
      i++;
   }

   return i;
}


long CSSTR_ToBase64(char* inBuffer,
                    int   inSize,
                    char* outBuffer,
                    int   outSize) {

   long i;
   long j;
   long trailing;
   long len;

   char tempByte;

   CSSTRCV* cvt;

   cvt = CSSTRCV_Constructor();

   CSSTRCV_SetConversion(cvt, CCSID_JOBDEFAULT, CCSID_ASCII);
   CSSTRCV_StrCpy(cvt, inBuffer, inSize);

   // note that the converted string should be the same size
   // as the input string.

   CSSTRCV_Get(cvt, inBuffer);

   // Determine how many trailing bytes

   trailing = inSize % 3;

   inSize = inSize - trailing;

   for (i=0, j=0; i<inSize; i++, j++) {

      outBuffer[j] = B64EncodeTable[(inBuffer[i] & B64_MASK_11) >> 2];
      tempByte = (inBuffer[i] & B64_MASK_12) << 4;
      j++; i++;

      outBuffer[j] = B64EncodeTable[((inBuffer[i] & B64_MASK_21) >> 4) | tempByte];
      tempByte = (inBuffer[i] & B64_MASK_22) << 2;
      j++; i++;

      outBuffer[j] = B64EncodeTable[((inBuffer[i] & B64_MASK_31) >> 6) | tempByte];
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

        outBuffer[j] = B64EncodeTable[((inBuffer[i] & B64_MASK_21) >> 4) | tempByte];
        tempByte = (inBuffer[i] & B64_MASK_22) << 2;
        j++; i++;

        outBuffer[j] = B64EncodeTable[tempByte | 0];
        j++;

        outBuffer[j] = 61; //'=';
        j++;

        break;
   }

   outBuffer[j] = 0;

   CSSTRCV_SetConversion(cvt, CCSID_ASCII, CCSID_JOBDEFAULT);
   CSSTRCV_StrCpy(cvt, outBuffer, j);

   // note that the converted string should be the same size
   // as the input string.

   CSSTRCV_Get(cvt, outBuffer);
   CSSTRCV_Destructor(&cvt);

   return j;
}

