/* ==========================================================================
  Clarasoft JSON Object
  csjson.c

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qcsrcx/cslib.h"

#define JSON_PARSE_STANDARD   0

#define JSON_TOK_BOOL_FALSE   0
#define JSON_TOK_BOOL_TRUE    1
#define JSON_TOK_NULL         3
#define JSON_TOK_LBRACE      10
#define JSON_TOK_RBRACE      11
#define JSON_TOK_LBRACKET    20
#define JSON_TOK_RBRACKET    21
#define JSON_TOK_COLON       30
#define JSON_TOK_COMMA       31
#define JSON_TOK_NUMERIC     51
#define JSON_TOK_STRING      52

#define JSON_TYPE_BOOL_FALSE  0
#define JSON_TYPE_BOOL_TRUE   1
#define JSON_TYPE_NULL        3
#define JSON_TYPE_OBJECT     10
#define JSON_TYPE_ARRAY      20
#define JSON_TYPE_NUMERIC    51
#define JSON_TYPE_STRING     52
#define JSON_TYPE_UNKNOWN    99

typedef struct tagCSJSON
{
  CSLIST Tokens;
  CSMAP Object;

  long slabSize;
  long nextSlabSize;

  char* szSlab;

} CSJSON;

typedef struct tagCSJSON_TOKENINFO
{
  char *szToken;
  int  type;
  long size;

} CSJSON_TOKENINFO;

typedef struct tagCSJSON_DIRENTRY
{
  int    type;
  long   numItems;
  CSLIST Listing;

} CSJSON_DIRENTRY;

typedef struct tagCSJSON_LSENTRY
{
  int   type;
  char* szKey;
  long  keySize;
  char* szValue;
  long  valueSize;

} CSJSON_LSENTRY;

/* ---------------------------------------------------------------------------
 * private methods
 * --------------------------------------------------------------------------- */

CSRESULT
  CSJSON_PRIVATE_Serialize
    (CSJSON* This,
     char*  szPath,
     int    type,
     CSLIST OutStr);

CSRESULT
  CSJSON_PRIVATE_O
    (CSJSON* This,
     long*  index,
     char*  szPath,
     long   len);

CSRESULT
  CSJSON_PRIVATE_A
    (CSJSON* This,
     long*  index,
     char*  szPath,
     long   len);

CSRESULT
  CSJSON_PRIVATE_VV
    (CSJSON* This,
     long*  index,
     char*  szPath,
     long len,
     CSLIST  listing);

CSRESULT
  CSJSON_PRIVATE_SerializeSubDir
    (CSJSON* This,
     char*  szPath,
     char** szOutStream,
     long*  curPos);

CSRESULT
  CSJSON_PRIVATE_IsNumeric
    (char* szNumber);

/* ---------------------------------------------------------------------------
 * implementation
 * --------------------------------------------------------------------------- */

CSJSON*
  CSJSON_Constructor
    (void) {

  CSJSON *Instance;

  Instance = (CSJSON *)malloc(sizeof(CSJSON));

  Instance->Tokens = CSLIST_Constructor();
  Instance->Object = CSMAP_Constructor();

  Instance->szSlab = 0;

  Instance->slabSize = 0;
  Instance->nextSlabSize = 0;

  return Instance;
}

void
  CSJSON_Destructor
    (CSJSON **This) {

  char *pszKey;

  long valueSize;
  long count;
  long size;
  long i;

  CSJSON_DIRENTRY* pdire;
  CSJSON_LSENTRY* plse;

  // Cleanup the JSON object previously built

  CSMAP_IterStart((*This)->Object);

  while (CS_SUCCEED(CSMAP_IterNext((*This)->Object, &pszKey,
                                    (void **)(&pdire), &valueSize)))
  {
    if (pdire->Listing != 0)
    {
      if (pdire->type == JSON_TYPE_ARRAY) {

        count = CSLIST_Count(pdire->Listing);

        for (i=0; i<count; i++) {
          CSLIST_GetDataRef(pdire->Listing, (void**)(&plse), i);
          if (plse->szKey != 0) {
            free(plse->szKey);
          }
          if (plse->szValue != 0) {
            free(plse->szValue);
          }
        }

        CSLIST_Destructor(&(pdire->Listing));
      }
      else {

        CSMAP_IterStart(pdire->Listing);

        while(CS_SUCCEED(CSMAP_IterNext(pdire->Listing, &pszKey, (void**)(&plse), &size))) {
          if (plse->szKey != 0) {
            free(plse->szKey);
          }
          if (plse->szValue != 0) {
            free(plse->szValue);
          }
        }

        CSMAP_Destructor(&(pdire->Listing));
      }
    }
  }

  CSMAP_Destructor(&((*This)->Object));

  CSLIST_Destructor(&((*This)->Tokens));

  // Cleanup other allocatons

  if ((*This)->szSlab) {
    free((*This)->szSlab);
  }

  free(*This);

  *This = 0;

  return;
}

CSRESULT
  CSJSON_PRIVATE_Tokenize
    (CSJSON *This,
     char *szJsonString)
{
  int n;
  int Len;
  int Stop;
  int haveDot;
  int haveExp;

  long tempIndex;
  long startToken;

  char szBoolBuffer[6];

  char CurChar;

  CSRESULT Rc;
  CSJSON_TOKENINFO ti;

  Rc = CS_SUCCESS;

  Len = strlen(szJsonString);

  n = 0;
  while (n < Len) {

    switch (szJsonString[n])
    {

    case ',':

      ti.szToken = 0;
      ti.type = JSON_TOK_COMMA;

      CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);
      break;

    case '{':

      ti.szToken = 0;
      ti.type = JSON_TOK_LBRACE;

      CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);

      break;

    case '}':

      ti.szToken = 0;
      ti.type = JSON_TOK_RBRACE;

      CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);

      break;

    case '[':

      ti.szToken = 0;
      ti.type = JSON_TOK_LBRACKET;

      CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);

      break;

    case ']':

      ti.szToken = 0;
      ti.type = JSON_TOK_RBRACKET;

      CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);

      break;

    case ':':

      ti.szToken = 0;
      ti.type = JSON_TOK_COLON;

      CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);

      break;

    case '"':

      CurChar = 0;
      Stop = 0;
      tempIndex = 0;

      n++;
      startToken = n;
      while (n < Len && !Stop)
      {

        ////////////////////////////////////////////////////////
        //    We need to detect escaped characters;
        ////////////////////////////////////////////////////////

        switch (szJsonString[n])
        {

        case '\\':

          if (CurChar == '\\')
          {
            tempIndex++;
            CurChar = 0;
          }
          else
          {
            CurChar = szJsonString[n];
          }

          n++; // examine next character

          break;

        case 't':

          if (CurChar == '\\')
          {
            tempIndex++;
            CurChar = 0;
          }
          else
          {
            tempIndex++;
          }

          n++; // examine next character

          break;

        case 'r':

          if (CurChar == '\\')
          {
            tempIndex++;
            CurChar = 0;
          }
          else
          {
            tempIndex++;
          }

          n++; // examine next character

          break;

        case 'n':

          if (CurChar == '\\')
          {
            tempIndex++;
            CurChar = 0;
          }
          else
          {
            tempIndex++;
          }

          n++; // examine next character

          break;

        case 'b':

          if (CurChar == '\\')
          {
            tempIndex++;
            CurChar = 0;
          }
          else
          {
            tempIndex++;
          }

          n++; // examine next character

          break;

        case 'f':

          if (CurChar == '\\')
          {
            tempIndex++;
            CurChar = 0;
          }
          else
          {
            tempIndex++;
          }

          n++; // examine next character

          break;

        case '"':

          if (CurChar == '\\')
          {
            tempIndex++;
            CurChar = 0;

            n++; // examine next character
          }
          else
          {
            Stop = 1;
          }

          break;

        default:

          /*
            Check for reserved characters:

            \b, \f, \t, \r, \n, \
          */

          switch (szJsonString[n])
          {

          case '\\':
            Rc = n; //Rc = CS_FAILURE;
            n = Len; // This will make the program leave the loop
            break;

          case '\b':
            Rc = n; //Rc = CS_FAILURE;
            n = Len; // This will make the program leave the loop
            break;

          case '\f':
            Rc = n; //Rc = CS_FAILURE;
            n = Len; // This will make the program leave the loop
            break;

          case '\r':
            Rc = n; //Rc = CS_FAILURE;
            n = Len; // This will make the program leave the loop
            break;

          case '\n':
            Rc = n; //Rc = CS_FAILURE;
            n = Len; // This will make the program leave the loop
            break;

          case '\t':
            Rc = n; //Rc = CS_FAILURE;
            n = Len; // This will make the program leave the loop
            break;

          default:

            tempIndex++;
            CurChar = 0;

            n++; // examine next character
          }

          break;
        }
      }

      if (szJsonString[n] == '"')
      {

        // We copy the buffer as a NULL-terminated string; to transfer binary
        // data in a JSON string, one should encode the data in BASE-64.

        ti.type = JSON_TOK_STRING;
        ti.size = tempIndex+1;
        ti.szToken = (char*)malloc(ti.size * sizeof(char));
        memcpy(ti.szToken, &(szJsonString[startToken]), tempIndex);
        ti.szToken[tempIndex] = 0;

        CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);
      }
      else
      {
        Rc = n; //Rc = CS_FAILURE;
        n = Len; // This will make the program leave the loop
      }

      break;

    default:

      if ((szJsonString[n] >= '0' && szJsonString[n] <= '9') ||
          (szJsonString[n] == '-'))
      {

        // Token may be numeric

        tempIndex = 1;  // there is at least one digit
        startToken = n;

        Stop = 0;
        haveDot = 0;
        haveExp = 0;

        n++;
        while (n < Len && !Stop)
        {
          switch (szJsonString[n])
          {

            case ' ':

              Stop = 1;
              break;

            case ',':

              n--;
              Stop = 1;
              break;

            case ':':

              n--;
              Stop = 1;
              break;

            case '[':

              n--;
              Stop = 1;
              break;

            case ']':

              n--;
              Stop = 1;
              break;

            case '{':

              n--;
              Stop = 1;
              break;

            case '}':

              n--;
              Stop = 1;
              break;

            default:

              switch(szJsonString[n]) {

                case '.':

                  // Only one dot and it must precede the exponent character

                  if (!haveDot && !haveExp) {

                    // check that if first digit is zero, then there are no
                    // other digits between the zero and the dot

                    if (szJsonString[startToken] == '-') {
                      if (szJsonString[startToken+1] == '0') {
                        if (tempIndex == 2) {
                          // Ok, number starts with -0 and next character is dot
                          tempIndex++;
                          n++;
                        }
                        else {
                          Rc = n; //Rc = CS_FAILURE;
                          n = Len; // This will make the program leave the loop
                        }
                      }
                      else {
                        // Ok, number starts with - and some non-zero digit
                        tempIndex++;
                        n++;
                      }
                    }
                    else {
                      if (szJsonString[startToken] == '0') {
                        if (tempIndex == 1) {
                          // Ok, number starts with 0 and next character is dot
                          tempIndex++;
                          n++;
                        }
                        else {
                          Rc = n; //Rc = CS_FAILURE;
                          n = Len; // This will make the program leave the loop
                        }

                      }
                      else {
                        // Ok, number starts with - and some non-zero digit
                        tempIndex++;
                        n++;
                      }
                    }

                    haveDot = 1;
                  }
                  else {
                    Rc = n; //Rc = CS_FAILURE;
                    n = Len; // This will make the program leave the loop
                  }

                  break;

                case '+':
                case '-':

                  // This must immediately follow the exponent character
                  if (szJsonString[startToken+tempIndex-1] == 'e' || szJsonString[startToken+tempIndex-1] == 'E') {
                    tempIndex++;
                    n++;
                  }
                  else {
                    Rc = n; //Rc = CS_FAILURE;
                    n = Len; // This will make the program leave the loop
                  }

                  break;

                case 'e':
                case 'E':

                  if (!haveExp) {
                    // This character must be preceded by a digit
                    if (szJsonString[startToken+tempIndex-1] >= '0' && szJsonString[startToken+tempIndex-1] <= '9') {
                      tempIndex++;
                      haveExp = 1;
                      n++;
                    }
                    else {
                      Rc = n; //Rc = CS_FAILURE;
                      n = Len; // This will make the program leave the loop
                    }
                  }
                  else {
                    Rc = n; //Rc = CS_FAILURE;
                    n = Len; // This will make the program leave the loop
                  }

                  break;

                default:

                  if ((szJsonString[n] >= '0' && szJsonString[n] <= '9'))
                  {
                    if (szJsonString[startToken+tempIndex-1] == 'E' || szJsonString[startToken+tempIndex-1] == 'e') {
                      // If the previous character is the expponent and we have a zero,
                      // we can ignore it because exponenets dont have leading zeroes

                      if (szJsonString[n] != '0') {
                        tempIndex++;
                      }
                    }
                    else {
                      tempIndex++;
                    }
                    n++;
                  }
                  else {
                    Rc = n; //Rc = CS_FAILURE;
                    n = Len; // This will make the program leave the loop
                  }

                  break;
              }

              break;
          }
        }

        if (Rc == CS_SUCCESS) {
          ti.type = JSON_TOK_NUMERIC;
          ti.size = tempIndex + 1;
          ti.szToken = (char*)malloc((ti.size) * sizeof(char));
          memcpy(ti.szToken, &(szJsonString[startToken]), ti.size-1);
          ti.szToken[ti.size-1] = 0;

          CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);
        }

      }
      else {

        if ((szJsonString[n] == 'f') ||
            (szJsonString[n] == 't') ||
            (szJsonString[n] == 'n'))
        {

          // Could be null or a boolean token

          tempIndex = 0;
          szBoolBuffer[tempIndex] = szJsonString[n];
          startToken = n;
          tempIndex++;

          Stop = 0;

          n++;
          while (n < Len && !Stop && tempIndex < 6)
          {

            switch (szJsonString[n])
            {

            case ' ':

              Stop = 1;
              break;

            case ',':

              n--;
              Stop = 1;
              break;

            case ':':

              n--;
              Stop = 1;
              break;

            case '[':

              n--;
              Stop = 1;
              break;

            case ']':

              n--;
              Stop = 1;
              break;

            case '{':

              n--;
              Stop = 1;
              break;

            case '}':

              n--;
              Stop = 1;
              break;

            default:

              szBoolBuffer[tempIndex] = szJsonString[n];
              tempIndex++;
              n++;
              break;
            }
          }

          szBoolBuffer[tempIndex] = 0;

          if (!strcmp(szBoolBuffer, "false")) {

            ti.type = JSON_TOK_BOOL_FALSE;
            ti.szToken = 0;
            CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);
          }
          else if (!strcmp(szBoolBuffer, "true")) {

            ti.type = JSON_TOK_BOOL_TRUE;
            ti.szToken = 0;
            CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);
          }
          else if (!strcmp(szBoolBuffer, "null")) {

            ti.type = JSON_TOK_NULL;
            ti.szToken = 0;
            CSLIST_Insert(This->Tokens, &ti, sizeof(CSJSON_TOKENINFO), CSLIST_BOTTOM);
          }
          else {
            Rc = n; //Rc = CS_FAILURE;
            n = Len; // This will make the program leave the loop
          }

        }
        else {

          // We can accept white spaces between tokens but ignore them,
          // otherwise, the JSON is invalid.

          if (!((szJsonString[n] == '\x05')  ||
                (szJsonString[n] == '\x25')  ||
                (szJsonString[n] == '\x0D')  ||
                (szJsonString[n] == ' ')  ||
                (szJsonString[n] == '\t') ||
                (szJsonString[n] == '\r') ||
                (szJsonString[n] == '\n')))
          {
            Rc = n; //Rc = CS_FAILURE;
            n = Len; // This will make the program leave the loop
          }
        }
      }

      break;
    }

    n++;
  }

  return Rc;
}

CSRESULT
  CSJSON_Parse
    (CSJSON *This,
     char *pJsonString,
     int parseMode)
{
  CSRESULT Rc;
  CSJSON_DIRENTRY*  pdire;
  CSJSON_LSENTRY* plse;
  CSJSON_TOKENINFO* pti;

  char* pszKey;

  long valueSize;
  long index;
  long count;
  long i;
  long size;

  // Cleanup the previous object

  CSMAP_IterStart(This->Object);

  while (CS_SUCCEED(CSMAP_IterNext(This->Object, &pszKey,
                                    (void **)(&pdire), &valueSize)))
  {
    if (pdire->Listing != 0)
    {
      if (pdire->type == JSON_TYPE_ARRAY) {

        count = CSLIST_Count(pdire->Listing);

        for (i=0; i<count; i++) {
          CSLIST_GetDataRef(pdire->Listing, (void**)(&plse), i);
          if (plse->szKey != 0) {
            free(plse->szKey);
          }
          if (plse->szValue != 0) {
            free(plse->szValue);
          }
        }

        CSLIST_Destructor(&(pdire->Listing));
      }
      else {

        CSMAP_IterStart(pdire->Listing);

        while(CS_SUCCEED(CSMAP_IterNext(pdire->Listing, &pszKey, (void**)(&plse), &size))) {
          if (plse->szKey != 0) {
            free(plse->szKey);
          }
          if (plse->szValue != 0) {
            free(plse->szValue);
          }
        }

        CSMAP_Destructor(&(pdire->Listing));
      }
    }
  }

  CSLIST_Clear(This->Tokens);
  CSMAP_Clear(This->Object);

  // Let's see if this is a valid JSON string

  Rc = CSJSON_PRIVATE_Tokenize(This, pJsonString);

  if (Rc == CS_SUCCESS)
  {
    This->nextSlabSize = 3; // at least an empty JSON object ({} or []})
    index = 0;

    // Get first token to determine JSON type
    CSLIST_GetDataRef(This->Tokens, (void**)(&pti), 0);

    switch(pti->type) {
      case JSON_TOK_LBRACE:
        Rc = CSJSON_PRIVATE_O(This, &index, "/", 1);
        break;
      case JSON_TOK_LBRACKET:
        Rc = CSJSON_PRIVATE_A(This, &index, "/", 1);
        break;
      default:
        // the string is not a valid JSON
        goto CSJSON_PARSE_FAILURE_CLEANUP_TOKENS;
    }

    if (CS_SUCCEED(Rc)) {

      ////////////////////////////////////////////////////
      //
      // Consider this: {}}}}} or []]]]]]
      //
      // The CSJSON_PRIVATE_O/A functions will return success
      // because it will find a matching } to the first {
      // and return immediately, not examining what follows.
      //
      // We need to check if there are remaining tokens and
      // if there are some, then the JSON is ill-formed.
      ////////////////////////////////////////////////////

      if (index < CSLIST_Count(This->Tokens)) {
        goto CSJSON_PARSE_FAILURE_CLEANUP_OBJECT;
      }
      else
      {
        goto CSJSON_PARSE_END;
      }

    }
    else {
      goto CSJSON_PARSE_FAILURE_CLEANUP_OBJECT;
    }
  }
  else {

    ///////////////////////////////////////////////////////////////
    // Branching label
    CSJSON_PARSE_FAILURE_CLEANUP_TOKENS:

    // Tokenization has failed ... some tokens are invalid and
    // we need to cleanup token values; the object is not
    // built at this point so there is no need to clean it.

    count = CSLIST_Count(This->Tokens);

    for (i=0; i<count; i++) {
      CSLIST_GetDataRef(This->Tokens, (void**)(&pti), i);

      if (pti->szToken != 0) {
        free(pti->szToken);
      }
    }

    CSLIST_Clear(This->Tokens);

    return CS_FAILURE;
  }

  ///////////////////////////////////////////////////////////////
  // Branching label
  CSJSON_PARSE_FAILURE_CLEANUP_OBJECT:

  /////////////////////////////////////////////////////////////////
  // We need to cleanup the object; at this point, listing entries
  // all point to token fields because we are parsing a string.
  //
  // The object is partially built such that deep cleaning the
  // Object listing entries will leave tokens unaffected with
  // token buffers left un-freed. So in this particualr case, we
  // cleanup the tokens and simply clear the object of its
  // listing entries.
  //
  // In a situation where the string is parsed successfully,
  // then doing a deep cleaning of the Object is appropriate.
  /////////////////////////////////////////////////////////////////

  CSMAP_IterStart(This->Object);

  while (CS_SUCCEED(CSMAP_IterNext(This->Object, &pszKey,
                                    (void **)(&pdire), &valueSize)))
  {
    if (pdire->Listing != 0)
    {
      if (pdire->type == JSON_TYPE_ARRAY) {
        CSLIST_Destructor(&(pdire->Listing));
      }
      else {
        CSMAP_Destructor(&(pdire->Listing));
      }
    }
  }

  count = CSLIST_Count(This->Tokens);

  for (i=0; i<count; i++) {
    CSLIST_GetDataRef(This->Tokens, (void**)(&pti), i);

    if (pti->szToken != 0) {
      free(pti->szToken);
    }
  }

  CSLIST_Clear(This->Tokens);
  CSMAP_Clear(This->Object);

  return CS_FAILURE;

  ///////////////////////////////////////////////////////////////
  // Branching label
  CSJSON_PARSE_END:

  return CS_SUCCESS;
}

CSRESULT
  CSJSON_PRIVATE_O
    (CSJSON* This,
     long* index,
     char* szPath,
     long  len) {

  CSRESULT Rc;
  CSLIST Listing;
  CSJSON_TOKENINFO* pti;
  CSJSON_DIRENTRY dire;

  long vv;

  char* szNewPath;

  vv = 0;
  Rc = CSLIST_GetDataRef(This->Tokens, (void**)(&pti), *index);

  if (Rc == CS_SUCCESS) {

    if (pti->type == JSON_TOK_LBRACE) {

      (*index)++;

      // Since this is an object, the directory listing will be a map
      Listing = CSMAP_Constructor();

      szNewPath = (char*)malloc((len * sizeof(char)) + 1);
      memcpy(szNewPath, szPath, len+1);

      while (CS_SUCCEED(CSJSON_PRIVATE_VV(This, index,
                                          szNewPath, len, Listing))) {
        vv++;

        Rc = CSLIST_GetDataRef(This->Tokens, (void**)(&pti), *index);

        if (Rc == CS_SUCCESS) {

          if (pti->type != JSON_TOK_COMMA) {
            break;
          }
          else {
            (This->nextSlabSize)++;  // for the comma
            (*index)++;
          }
        }
        else {
          Rc = CS_FAILURE;
          break;
        }
      }

      if (Rc == CS_SUCCESS) {

        if (vv > 0) {

          if (pti->type == JSON_TOK_RBRACE) {

            Rc = CSLIST_GetDataRef(This->Tokens,
                             (void**)(&pti), (*index)-1);

            if (Rc == CS_SUCCESS) {

              if (pti->type == JSON_TOK_COMMA) {
                free(szNewPath);
                CSMAP_Destructor(&Listing);
                Rc = CS_FAILURE;
              }
              else {

                dire.type = JSON_TYPE_OBJECT;
                dire.Listing = Listing;
                dire.numItems  = vv;

                CSMAP_InsertKeyRef(This->Object, szNewPath,
                                  (void*)(&dire), sizeof(CSJSON_DIRENTRY));

                This->nextSlabSize += 2; // because we need 2 braces
                (*index)++;
              }
            }
            else {
              // CS_FAILURE
              CSMAP_Destructor(&Listing);
              free(szNewPath);
            }
          }
          else {
            CSMAP_Destructor(&Listing);
            free(szNewPath);
            Rc = CS_FAILURE;
          }
        }
        else {

          Rc = CSLIST_GetDataRef(This->Tokens,
                             (void**)(&pti), *index);

          if (Rc == CS_SUCCESS) {

            if (pti->type == JSON_TOK_RBRACE) {

              Rc = CSLIST_GetDataRef(This->Tokens,
                             (void**)(&pti), (*index)-1);

              if (Rc == CS_SUCCESS) {

                if (pti->type == JSON_TOK_COMMA) {
                  CSMAP_Destructor(&Listing);
                  free(szNewPath);
                  Rc = CS_FAILURE;
                }
                else {

                  dire.type = JSON_TYPE_OBJECT;
                  dire.Listing = Listing;
                  dire.numItems  = vv;

                  CSMAP_InsertKeyRef(This->Object, szNewPath,
                                     (void*)(&dire), sizeof(CSJSON_DIRENTRY));

                  This->nextSlabSize += 2; // because we need 2 braces
                  (*index)++;
                }
              }
              else {
                // FAILURE
                CSMAP_Destructor(&Listing);
                free(szNewPath);
              }
            }
            else {
              CSMAP_Destructor(&Listing);
              free(szNewPath);
              Rc = CS_FAILURE;
            }
          }
          else {
            // FAILURE
            CSMAP_Destructor(&Listing);
            free(szNewPath);
          }
        }
      }
      else {
        Rc = CS_FAILURE;
        CSMAP_Destructor(&Listing);
        free(szNewPath);
      }
    }
    else {
      Rc = CS_FAILURE;
    }
  }
  else {
    // CS_FAILURE
  }

  return Rc;
}

CSRESULT
  CSJSON_PRIVATE_VV
    (CSJSON* This,
     long*  index,
     char*  szPath,
     long   len,
     CSMAP  Listing) {

  char* pNewPath;

  long newPathLen;

  CSRESULT Rc;
  CSJSON_TOKENINFO* pti;
  CSJSON_TOKENINFO* ls_pti;
  CSJSON_LSENTRY lse;

  Rc = CS_SUCCESS;

  Rc = CSLIST_GetDataRef(This->Tokens,
                           (void**)(&pti), *index);

  if (Rc == CS_SUCCESS) {

    if (pti->type == JSON_TOK_STRING) {

      ls_pti = pti; // This is a pointer to the token and will be inserted
                    // into the directory listing; this avoids copying
                    // the pointer to the token value as well as its size.

      (*index)++;

      Rc = CSLIST_GetDataRef(This->Tokens,
                           (void**)(&pti), *index);

      if (pti->type == JSON_TOK_COLON) {

        (*index)++;

        Rc = CSLIST_GetDataRef(This->Tokens,
                             (void**)(&pti), *index);

        switch(pti->type) {

          case JSON_TOK_LBRACE:

            if (len > 1) {
              newPathLen = len + 1 + ls_pti->size;
              pNewPath = (char*)malloc(newPathLen * sizeof(char));
              memcpy(pNewPath, szPath, len);
              pNewPath[len] = '/';
              memcpy(&pNewPath[len+1], ls_pti->szToken, ls_pti->size);
            }
            else {
              newPathLen = len + ls_pti->size;
              pNewPath = (char*)malloc(newPathLen * sizeof(char));
              memcpy(pNewPath, szPath, len);
              memcpy(&pNewPath[len], ls_pti->szToken, ls_pti->size);
            }

            Rc = CSJSON_PRIVATE_O(This, index, pNewPath, newPathLen-1);
            free(pNewPath);

            lse.szKey = ls_pti->szToken;
            lse.keySize = ls_pti->size;
            lse.szValue = 0;
            lse.type = JSON_TYPE_OBJECT;
            CSMAP_Insert(Listing, ls_pti->szToken, (void*)(&lse), sizeof(CSJSON_LSENTRY));
            // because we need 2 double quotes, a colon and the key value
            This->nextSlabSize = This->nextSlabSize + ls_pti->size + 3;

            break;

          case JSON_TOK_LBRACKET:

            if (len > 1) {
              newPathLen = len + 1 + ls_pti->size;
              pNewPath = (char*)malloc(newPathLen * sizeof(char));
              memcpy(pNewPath, szPath, len);
              pNewPath[len] = '/';
              memcpy(&pNewPath[len+1], ls_pti->szToken, ls_pti->size);
            }
            else {
              newPathLen = len + ls_pti->size;
              pNewPath = (char*)malloc((len + 1 + ls_pti->size) * sizeof(char));
              memcpy(pNewPath, szPath, len);
              memcpy(&pNewPath[len], ls_pti->szToken, ls_pti->size);
            }

            Rc = CSJSON_PRIVATE_A(This, index, pNewPath, newPathLen-1);
            free(pNewPath);

            lse.szKey = ls_pti->szToken;
            lse.keySize = ls_pti->size;
            lse.szValue = 0;
            lse.type = JSON_TYPE_ARRAY;
            CSMAP_Insert(Listing, ls_pti->szToken, (void*)(&lse), sizeof(CSJSON_LSENTRY));
            // because we need 2 double quotes, a colon and the key value
            This->nextSlabSize = This->nextSlabSize + ls_pti->size + 3;

            break;

          case JSON_TOK_STRING:

            lse.szKey = ls_pti->szToken;
            lse.keySize = ls_pti->size;
            lse.type = JSON_TYPE_STRING;
            lse.szValue = pti->szToken;
            lse.valueSize = pti->size;
            CSMAP_Insert(Listing, ls_pti->szToken, (void*)(&lse), sizeof(CSJSON_LSENTRY));
            // because we need 4 double quotes, a colon and the key/value pair
            This->nextSlabSize = This->nextSlabSize + ls_pti->size + pti->size + 5;

            (*index)++;

            Rc = CS_SUCCESS;

            break;

          case JSON_TOK_NUMERIC:

            lse.szKey = ls_pti->szToken;
            lse.keySize = ls_pti->size;
            lse.type = JSON_TYPE_NUMERIC;
            lse.szValue = pti->szToken;
            lse.valueSize = pti->size;
            CSMAP_Insert(Listing, ls_pti->szToken, (void*)(&lse), sizeof(CSJSON_LSENTRY));
            // because we need 2 double quotes, a colon and the key/value pair
            This->nextSlabSize = This->nextSlabSize + ls_pti->size + pti->size + 3;

            (*index)++;

            Rc = CS_SUCCESS;

            break;

          case JSON_TOK_BOOL_FALSE:

            lse.szKey = ls_pti->szToken;
            lse.keySize = ls_pti->size;
            lse.type = JSON_TYPE_BOOL_FALSE;
            lse.szValue = 0;
            CSMAP_Insert(Listing, ls_pti->szToken, (void*)(&lse), sizeof(CSJSON_LSENTRY));
            // because we need 2 double quotes, a colon and the key/value pair
            This->nextSlabSize = This->nextSlabSize + ls_pti->size + 8;

            (*index)++;

            Rc = CS_SUCCESS;

            break;

          case JSON_TOK_BOOL_TRUE:

            lse.szKey = ls_pti->szToken;
            lse.keySize = ls_pti->size;
            lse.type = JSON_TOK_BOOL_TRUE;
            lse.szValue = 0;
            CSMAP_Insert(Listing, ls_pti->szToken, (void*)(&lse), sizeof(CSJSON_LSENTRY));
            // because we need 2 double quotes, a colon and the key/value pair
            This->nextSlabSize = This->nextSlabSize + ls_pti->size + 7;

            (*index)++;

            Rc = CS_SUCCESS;

            break;

          case JSON_TOK_NULL:

            lse.szKey = ls_pti->szToken;
            lse.keySize = ls_pti->size;
            lse.type = JSON_TYPE_NULL;
            lse.szValue = 0;
            CSMAP_Insert(Listing, ls_pti->szToken, (void*)(&lse), sizeof(CSJSON_LSENTRY));
            // because we need 2 double quotes, a colon and the key/value pair
            This->nextSlabSize = This->nextSlabSize + ls_pti->size + 7;

            (*index)++;

            Rc = CS_SUCCESS;

            break;

          default:

            Rc = CS_FAILURE;

            break;
        }
      }
      else {
        Rc = CS_FAILURE;
      }
    }
    else {
      Rc = CS_FAILURE;
    }
  }
  else {
    //FAILURE
  }

  return Rc;
}

CSRESULT
  CSJSON_PRIVATE_A
    (CSJSON* This,
     long*  index,
     char*  szPath,
     long   len) {

  CSRESULT Rc;
  CSJSON_TOKENINFO* pti;
  CSJSON_DIRENTRY dire;
  CSJSON_LSENTRY lse;
  CSJSON_LSENTRY* plse;
  CSLIST Listing;

  long indexLen;
  long start;
  long curIndex;
  long commaFlag;
  long newPathLen;
  long count;
  long i;

  char* szNewPath;

  char szIndex[11];

  Rc = CSLIST_GetDataRef(This->Tokens, (void**)(&pti), *index);

  if (Rc == CS_SUCCESS) {

    if (pti->type == JSON_TOK_LBRACKET) {

      Listing = CSLIST_Constructor();

      (*index)++;

      start = *index;
      curIndex = 0;
      commaFlag = 1;

      while (1) {

        if (CS_SUCCEED(CSLIST_GetDataRef(This->Tokens,
                            (void**)(&pti), start))) {

          switch (pti->type) {

            case JSON_TOK_LBRACE:

              if (commaFlag == 1) {

                if (len > 1) {
                  // allocate enough space for path, a slash,
                  // 10 digits and the NULL
                  newPathLen = len + 12;
                  szNewPath = (char*)malloc(newPathLen * sizeof(char));
                  memcpy(szNewPath, szPath, len);
                  szNewPath[len] = '/';
                  sprintf(szIndex, "%ld", curIndex);
                  indexLen = strlen(szIndex) + 1;  // take / into account
                  memcpy(&szNewPath[len+1], szIndex, indexLen);
                  newPathLen = len + indexLen;
                }
                else {
                  // we are root; allocate enough space for
                  // path, 10 digits and the NULL
                  newPathLen = len + 11;
                  szNewPath = (char*)malloc(newPathLen * sizeof(char));
                  memcpy(szNewPath, szPath, len);
                  sprintf(szIndex, "%ld", curIndex);
                  indexLen = strlen(szIndex) + 1;  // take / into account
                  memcpy(&szNewPath[len], szIndex, indexLen);
                  newPathLen = len + indexLen - 1;
                }

                Rc = CSJSON_PRIVATE_O(This, &start, szNewPath, newPathLen);
                free(szNewPath);

                curIndex++;
                commaFlag = 0;
                lse.szKey = 0;
                lse.szValue = 0;
                lse.type = JSON_TYPE_OBJECT;
                CSLIST_Insert(Listing, (void*)(&lse), sizeof(CSJSON_LSENTRY), CSLIST_BOTTOM);
              }
              else {
                goto CSJSON_PRIVATE_A_CLEANUP;
              }

              break;

            case JSON_TOK_LBRACKET:

              if (commaFlag == 1) {

                if (len > 1) {
                  // allocate enough space for path, a slash,
                  // 10 digits and the NULL
                  newPathLen = len + 12;
                  szNewPath = (char*)malloc(newPathLen * sizeof(char));
                  memcpy(szNewPath, szPath, len);
                  szNewPath[len] = '/';
                  sprintf(szIndex, "%ld", curIndex);
                  indexLen = strlen(szIndex) + 1;  // take / into account
                  memcpy(&szNewPath[len+1], szIndex, indexLen);
                  newPathLen = len + indexLen;
                }
                else {
                  // we are root; allocate enough space for
                  // path, 10 digits and the NULL
                  newPathLen = len + 11;
                  szNewPath = (char*)malloc(newPathLen * sizeof(char));
                  memcpy(szNewPath, szPath, len);
                  sprintf(szIndex, "%ld", curIndex);
                  indexLen = strlen(szIndex) + 1;  // take / into account
                  memcpy(&szNewPath[len], szIndex, indexLen);
                  newPathLen = len + indexLen -1;
                }

                Rc = CSJSON_PRIVATE_A(This, &start, szNewPath, newPathLen);
                free(szNewPath);

                curIndex++;
                commaFlag = 0;

                lse.szKey = 0;
                lse.szValue = 0;
                lse.type = JSON_TYPE_ARRAY;
                CSLIST_Insert(Listing, (void*)(&lse), sizeof(CSJSON_LSENTRY), CSLIST_BOTTOM);
              }
              else {
                goto CSJSON_PRIVATE_A_CLEANUP;
              }

              break;

            case JSON_TOK_COMMA:

              commaFlag = 1;
              start++;
              // because we need room for the comma
              (This->nextSlabSize)++;
              break;

            case JSON_TOK_RBRACKET:

              if ((commaFlag == 0 && curIndex > 0) ||
                  (commaFlag == 1 && curIndex == 0)) {

                // allocate enough space for path

                szNewPath = (char*)malloc((len + 1) * sizeof(char));
                memcpy(szNewPath, szPath, len+1);

                dire.type = JSON_TYPE_ARRAY;
                dire.Listing = Listing;
                dire.numItems  = curIndex;

                CSMAP_InsertKeyRef(This->Object, szNewPath,
                                    (void*)(&dire), sizeof(CSJSON_DIRENTRY));

                // because we need 2 brackets
                This->nextSlabSize += 2;

                *index = start + 1;
                goto CSJSON_PRIVATE_A_END;
              }
              else {
                goto CSJSON_PRIVATE_A_CLEANUP;
              }

              break;

            default:

              if (commaFlag == 1) {

                if (pti->type == JSON_TOK_STRING ||
                    pti->type == JSON_TOK_NUMERIC) {

                  lse.szKey = 0;
                  lse.szValue = pti->szToken;
                  lse.valueSize = pti->size;
                  lse.type = pti->type;
                  CSLIST_Insert(Listing, (void*)(&lse), sizeof(CSJSON_LSENTRY), CSLIST_BOTTOM);
                  // because we need (maybe) 2 double quotes and the value
                  This->nextSlabSize = This->nextSlabSize + lse.valueSize + 2;

                  start++;
                  curIndex++;
                }
                else {
                  if (pti->type == JSON_TOK_BOOL_FALSE ||
                      pti->type == JSON_TOK_BOOL_TRUE ||
                      pti->type == JSON_TOK_NULL) {

                    lse.szKey = 0;
                    lse.szValue = 0;
                    lse.type = pti->type;
                    CSLIST_Insert(Listing, (void*)(&lse), sizeof(CSJSON_LSENTRY), CSLIST_BOTTOM);
                    // because we need up to the max value size
                    This->nextSlabSize = This->nextSlabSize + 5;

                    start++;
                    curIndex++;
                  }
                  else {
                    //CS_FAILURE... leave the loop
                    goto CSJSON_PRIVATE_A_CLEANUP;
                  }
                }

                commaFlag = 0;
                break;
            }
            else {
              //CS_FAILURE... leave the loop
              goto CSJSON_PRIVATE_A_CLEANUP;
            }
          }
        }
        else {
          //CS_FAILURE... leave the loop
          goto CSJSON_PRIVATE_A_CLEANUP;
        }
      }

      ///////////////////////////////////////////////////////////
      // Branching Label
      CSJSON_PRIVATE_A_CLEANUP:
      //
      // We need to cleanup the listing and destroy it
      ///////////////////////////////////////////////////////////

      count = CSLIST_Count(Listing);

      for (i=0; i<count; i++) {
        CSLIST_GetDataRef(Listing, (void**)(&plse), i);
        if (plse->szKey != 0) {
          free(plse->szKey);
        }
        if (plse->szValue != 0) {
          free(plse->szValue);
        }
      }

      CSLIST_Destructor(&Listing);

      Rc = CS_FAILURE;
    }
    else {

      Rc = CS_FAILURE;

    }
  }

  ///////////////////////////////////////////////////////////
  // Branching Label
  CSJSON_PRIVATE_A_END:

  return Rc;
}

CSRESULT
  CSJSON_LookupDir
    (CSJSON* This,
     char* szPath,
     CSJSON_DIRENTRY* pdire) {

  long size;
  CSJSON_DIRENTRY* lpdire;

  if (CS_SUCCEED(CSMAP_Lookup(This->Object,
                             szPath,
                             (void**)(&lpdire),
                             &size))) {

    if (lpdire->type == JSON_TYPE_ARRAY ||
        lpdire->type == JSON_TYPE_OBJECT) {

      pdire->numItems = lpdire->numItems;
      pdire->type = lpdire->type;
      return CS_SUCCESS;
    }
  }
  else {
    pdire->numItems = 0;
    pdire->type = JSON_TYPE_UNKNOWN;
  }

  return CS_FAILURE;
}

CSRESULT CSJSON_LookupKey
  (CSJSON* This,
   char* szPath,
   char* szKey,
   CSJSON_LSENTRY* plse) {

  long size;
  CSJSON_LSENTRY* lplse;
  CSJSON_DIRENTRY* lpdire;

  if (CS_SUCCEED(CSMAP_Lookup(This->Object,
                             szPath,
                             (void**)(&lpdire),
                             &size))) {

    if (lpdire->type == JSON_TYPE_OBJECT) {

      if (CS_SUCCEED(CSMAP_Lookup(lpdire->Listing,
                                  szKey,
                                  (void**)(&lplse),
                                  &size))) {

        if (lplse->type == JSON_TYPE_STRING ||
            lplse->type == JSON_TYPE_NUMERIC) {
          plse->type = lplse->type;
          plse->szKey = 0;
          plse->keySize = 0;
          plse->szValue = lplse->szValue;
          plse->valueSize = lplse->valueSize;
        }
        else {
          plse->type = lplse->type;
          plse->szKey = 0;
          plse->keySize = 0;
          plse->szValue = 0;
          plse->valueSize = 0;
        }

        return CS_SUCCESS;
      }
    }
  }

  plse->keySize = 0;
  plse->szKey = 0;
  plse->szValue = 0;
  plse->valueSize = 0;

  return CS_FAILURE;
}

CSRESULT
  CSJSON_LookupIndex
    (CSJSON* This,
     char* szPath,
     long index,
     CSJSON_LSENTRY* plse) {

  long size;
  CSJSON_LSENTRY* lplse;
  CSJSON_DIRENTRY* lpdire;

  if (CS_SUCCEED(CSMAP_Lookup(This->Object,
                             szPath,
                             (void**)(&lpdire),
                             &size))) {

    if (lpdire->type == JSON_TYPE_ARRAY) {

      if (CS_SUCCEED(CSLIST_GetDataRef(lpdire->Listing,
                                  (void**)(&lplse),
                                  index))) {

        if (lplse->type == JSON_TYPE_STRING ||
            lplse->type == JSON_TYPE_NUMERIC) {
          plse->type = lplse->type;
          plse->szKey = 0;
          plse->keySize = 0;
          plse->szValue = lplse->szValue;
          plse->valueSize = lplse->valueSize;
        }
        else {
          plse->type = lplse->type;
          plse->szKey = 0;
          plse->keySize = 0;
          plse->szValue = 0;
          plse->valueSize = 0;
        }

        return CS_SUCCESS;      }
    }
  }

  plse->keySize = 0;
  plse->szKey = 0;
  plse->szValue = 0;
  plse->valueSize = 0;

  return CS_FAILURE;
}

CSRESULT
  CSJSON_Dump
    (CSJSON* This,
     CSLIST values) {

  long count;
  long i;
  long size;
  char* szKey;

  CSJSON_DIRENTRY* pdire;
  CSJSON_LSENTRY* pls;

  CSMAP_IterStart(This->Object);

  while(CS_SUCCEED(CSMAP_IterNext(This->Object, &szKey, (void**)(&pdire), &size)))
  {

    if (szKey[0] != 0) {

      switch (pdire->type) {
        case JSON_TYPE_STRING:
          printf("\n%s : STRING", szKey);
          break;
        case JSON_TYPE_NUMERIC:
          printf("\n%s : NUMERIC", szKey);
          break;
        case JSON_TYPE_BOOL_FALSE:
          printf("\n%s : BOOL", szKey);
          break;
        case JSON_TYPE_BOOL_TRUE:
          printf("\n%s : BOOL", szKey);
          break;
        case JSON_TYPE_NULL:
          printf("\n%s : NULL", szKey);
          break;
        case JSON_TYPE_ARRAY:

          printf("\n%s : <ARRAY>", szKey);

          count = CSLIST_Count(pdire->Listing);

          for (i=0; i<count; i++) {

            CSLIST_GetDataRef(pdire->Listing, (void**)(&pls), i);

            switch (pls->type) {
              case JSON_TYPE_STRING:
                printf("\n\tSTRING");
                break;
              case JSON_TYPE_NUMERIC:
                printf("\n\tNUMERIC");
                break;
              case JSON_TYPE_BOOL_FALSE:
                printf("\n\tBOOL");
                break;
              case JSON_TYPE_BOOL_TRUE:
                printf("\n\tBOOL");
                break;
              case JSON_TYPE_NULL:
                printf("\n\tNULL");
                break;
              case JSON_TYPE_ARRAY:
                printf("\n\t<ARRAY>");
                break;
              case JSON_TYPE_OBJECT:
                printf("\n\t<OBJECT>");
                break;
              default:
                printf("\n\t<UNKNOWN>");
                break;
            }
          }

          break;

        case JSON_TYPE_OBJECT:

          printf("\n%s : <OBJECT>", szKey);

          CSMAP_IterStart(pdire->Listing);

          while (CS_SUCCEED(CSMAP_IterNext(pdire->Listing, &szKey, (void**)(&pls), &size))) {

            switch (pls->type) {

              case JSON_TYPE_STRING:
                printf("\n\t%s : STRING", pls->szKey);
                break;
              case JSON_TYPE_NUMERIC:
                printf("\n\t%s : NUMERIC", pls->szKey);
                break;
              case JSON_TYPE_BOOL_FALSE:
                printf("\n\t%s : BOOL", pls->szKey);
                break;
              case JSON_TYPE_BOOL_TRUE:
                printf("\n\t%s : BOOL", pls->szKey);
                break;
              case JSON_TYPE_NULL:
                printf("\n\t%s : NULL", pls->szKey);
                break;
              case JSON_TYPE_ARRAY:
                printf("\n\t%s : <ARRAY>", pls->szKey);
                break;
              case JSON_TYPE_OBJECT:
                printf("\n\t%s : <OBJECT>", pls->szKey);
                break;
              default:
                printf("\n\t<UNKNOWN>");
                break;
            }
          }

          break;
        default:
          printf("<UNKNOWN>");
          break;
      }
    }
  }

  return CS_SUCCESS;
}

CSRESULT
  CSJSON_Init
    (CSJSON* This,
     long type) {

  CSRESULT Rc;

  CSJSON_DIRENTRY dire;
  CSJSON_DIRENTRY* pdire;
  CSJSON_LSENTRY* plse;

  Rc = CS_SUCCESS;

  char* pszKey;
  char* szRoot;

  long valueSize;
  long count;
  long i;
  long size;

  This->nextSlabSize = 2;

  // Cleanup the previous object

  CSMAP_IterStart(This->Object);

  while (CS_SUCCEED(CSMAP_IterNext(This->Object, &pszKey,
                                    (void **)(&pdire), &valueSize)))
  {
    if (pdire->Listing != 0)
    {
      if (pdire->type == JSON_TYPE_ARRAY) {

        count = CSLIST_Count(pdire->Listing);

        for (i=0; i<count; i++) {
          CSLIST_GetDataRef(pdire->Listing, (void**)(&plse), i);
          if (plse->szKey != 0) {
            free(plse->szKey);
          }
          if (plse->szValue != 0) {
            free(plse->szValue);
          }
        }

        CSLIST_Destructor(&(pdire->Listing));
      }
      else {

        CSMAP_IterStart(pdire->Listing);

        while(CS_SUCCEED(CSMAP_IterNext(pdire->Listing, &pszKey, (void**)(&plse), &size))) {
          if (plse->szKey != 0) {
            free(plse->szKey);
          }
          if (plse->szValue != 0) {
            free(plse->szValue);
          }
        }

        CSMAP_Destructor(&(pdire->Listing));
      }
    }
  }

  CSMAP_Clear(This->Object);

  szRoot = (char*)malloc(2*sizeof(char));
  szRoot[0] = '/';
  szRoot[1] = 0;

  dire.type = type;
  dire.numItems  = 0;

  switch(type) {

    case JSON_TYPE_ARRAY:

      dire.Listing = CSLIST_Constructor();

      CSMAP_InsertKeyRef(This->Object, szRoot,
                        (void*)(&dire), sizeof(CSJSON_DIRENTRY));

      break;

    case JSON_TYPE_OBJECT:

      dire.Listing = CSMAP_Constructor();

      CSMAP_InsertKeyRef(This->Object, szRoot,
                        (void*)(&dire), sizeof(CSJSON_DIRENTRY));

      break;

    default:

      free(szRoot);
      Rc = CS_FAILURE;
      break;
  }

  return Rc;
}

long
  CSJSON_Serialize
    (CSJSON* This,
     char* szPath,
     char** szOutStream,
     int mode) {

  long count;
  long i;
  long size;
  long curPos;
  long pathLen;
  long indexLen;

  char* szKey;
  char* szSubPath;

  char szIndex[11];

  CSJSON_DIRENTRY* dire;
  CSJSON_LSENTRY* pls;

  // Allocate slab, if the present one is too small: note that the slab size may be actually
  // larger than the actuall output stream.This is ok because we will return the actual size
  // of the output stream and as long as the slab is large enough, it will be fine.

  if (This->nextSlabSize > This->slabSize) {
    This->slabSize = This->nextSlabSize;
    free(This->szSlab);
    This->szSlab = (char*)malloc((This->slabSize +1) *sizeof(char));
  }

  // We hand over the slab to the caller; it is under agreement that the caller will
  // not use the slab other than to read it and/or copy it. Caller should not write
  // into the slab nor deallocate it

  *szOutStream = This->szSlab;

  CSMAP_IterStart(This->Object);

  curPos = 0;

  if(CS_SUCCEED(CSMAP_Lookup(This->Object, szPath, (void**)(&dire), &size)))  {

    switch (dire->type) {

      case JSON_TYPE_ARRAY:

        (*szOutStream)[curPos] = '[';
        curPos++;

        if (dire->Listing != 0) {

          count = CSLIST_Count(dire->Listing);

          for (i=0; i<count; i++) {

            CSLIST_GetDataRef(dire->Listing, (void**)(&pls), i);

            switch (pls->type) {
              case JSON_TYPE_STRING:
                (*szOutStream)[curPos] = '"';
                curPos++;
                memcpy(&(*szOutStream)[curPos], pls->szValue, pls->valueSize-1);
                curPos += pls->valueSize-1;
                (*szOutStream)[curPos] = '"';
                curPos++;
                break;
              case JSON_TYPE_NUMERIC:
                memcpy(&(*szOutStream)[curPos], pls->szValue, pls->valueSize-1);
                curPos += pls->valueSize-1;
                break;
              case JSON_TYPE_BOOL_FALSE:
                memcpy(&(*szOutStream)[curPos], "false", 5);
                curPos += 5;
                break;
              case JSON_TYPE_BOOL_TRUE:
                memcpy(&(*szOutStream)[curPos], "true", 4);
                curPos += 4;
                break;
              case JSON_TYPE_NULL:
                memcpy(&(*szOutStream)[curPos], "null", 4);
                curPos += 4;
                break;

              case JSON_TYPE_ARRAY:

                (*szOutStream)[curPos] = '[';
                curPos++;

                sprintf(szIndex, "%ld", i);
                pathLen = strlen(szPath);
                indexLen = strlen(szIndex);
                szSubPath = (char*)malloc((pathLen + indexLen + 2) * sizeof(char));
                memcpy(szSubPath, szPath, pathLen);

                if (pathLen > 1) { // we are not root path
                  szSubPath[pathLen] = '/';
                  memcpy(&szSubPath[pathLen+1], szIndex, indexLen+1);
                }
                else {
                  memcpy(&szSubPath[1], szIndex, indexLen+1);
                }

                CSJSON_PRIVATE_SerializeSubDir(This, szSubPath, szOutStream, &curPos);

                (*szOutStream)[curPos] = ']';
                curPos++;
                free(szSubPath);

                break;

              case JSON_TYPE_OBJECT:

                (*szOutStream)[curPos] = '{';
                curPos++;

                sprintf(szIndex, "%ld", i);
                pathLen = strlen(szPath);
                indexLen = strlen(szIndex);
                szSubPath = (char*)malloc((pathLen + indexLen + 2) * sizeof(char));
                memcpy(szSubPath, szPath, pathLen);

                if (pathLen > 1) { // we are not root path
                  szSubPath[pathLen] = '/';
                  memcpy(&szSubPath[pathLen+1], szIndex, indexLen+1);
                }
                else {
                  memcpy(&szSubPath[1], szIndex, indexLen+1);
                }

                CSJSON_PRIVATE_SerializeSubDir(This, szSubPath, szOutStream, &curPos);

                (*szOutStream)[curPos] = '}';
                curPos++;
                free(szSubPath);

                break;

              default:

                break;
            }

            (*szOutStream)[curPos] = ',';
            curPos++;
          }

          // to erase dangling comma after last element, if there are some
          if (count > 0) {
            curPos--;
          }
        }

        (*szOutStream)[curPos] = ']';
        curPos++;

        break;

      case JSON_TYPE_OBJECT:

        (*szOutStream)[curPos] = '{';
        curPos++;

        CSMAP_IterStart(dire->Listing);
        count = 0;

        while(CS_SUCCEED(CSMAP_IterNext(dire->Listing, &szKey, (void**)(&pls), &size))) {

          count++;

          switch (pls->type) {
            case JSON_TYPE_STRING:
              (*szOutStream)[curPos] = '"';
              curPos++;
              memcpy(&(*szOutStream)[curPos], pls->szKey, pls->keySize-1);
              curPos += pls->keySize-1;
              (*szOutStream)[curPos] = '"';
              curPos++;
              (*szOutStream)[curPos] = ':';
              curPos++;
              (*szOutStream)[curPos] = '"';
              curPos++;
              memcpy(&(*szOutStream)[curPos], pls->szValue, pls->valueSize-1);
              curPos += pls->valueSize-1;
              (*szOutStream)[curPos] = '"';
              curPos++;
              break;
            case JSON_TYPE_NUMERIC:
              (*szOutStream)[curPos] = '"';
              curPos++;
              memcpy(&(*szOutStream)[curPos], pls->szKey, pls->keySize-1);
              curPos += pls->keySize-1;
              (*szOutStream)[curPos] = '"';
              curPos++;
              (*szOutStream)[curPos] = ':';
              curPos++;
              memcpy(&(*szOutStream)[curPos], pls->szValue, pls->valueSize-1);
              curPos += pls->valueSize-1;
              break;
            case JSON_TYPE_BOOL_FALSE:
              (*szOutStream)[curPos] = '"';
              curPos++;
              memcpy(&(*szOutStream)[curPos], pls->szKey, pls->keySize-1);
              curPos += pls->keySize-1;
              (*szOutStream)[curPos] = '"';
              curPos++;
              (*szOutStream)[curPos] = ':';
              curPos++;
              memcpy(&(*szOutStream)[curPos], "false", 5);
              curPos += 5;
              break;
            case JSON_TYPE_BOOL_TRUE:
              (*szOutStream)[curPos] = '"';
              curPos++;
              memcpy(&(*szOutStream)[curPos], pls->szKey, pls->keySize-1);
              curPos += pls->keySize-1;
              (*szOutStream)[curPos] = '"';
              curPos++;
              (*szOutStream)[curPos] = ':';
              curPos++;
              memcpy(&(*szOutStream)[curPos], "true", 4);
              curPos += 4;
              break;
            case JSON_TYPE_NULL:
              (*szOutStream)[curPos] = '"';
              curPos++;
              memcpy(&(*szOutStream)[curPos], pls->szKey, pls->keySize-1);
              curPos += pls->keySize-1;
              (*szOutStream)[curPos] = '"';
              curPos++;
              (*szOutStream)[curPos] = ':';
              curPos++;
              memcpy(&(*szOutStream)[curPos], "null", 4);
              curPos += 4;
              break;

            case JSON_TYPE_ARRAY:

              // add key name to stream
              (*szOutStream)[curPos] = '"';
              curPos++;
              memcpy(&(*szOutStream)[curPos], pls->szKey, pls->keySize-1);
              curPos += pls->keySize-1;
              (*szOutStream)[curPos] = '"';
              curPos++;
              (*szOutStream)[curPos] = ':';
              curPos++;
              (*szOutStream)[curPos] = '[';
              curPos++;

              // create next item key
              pathLen = strlen(szPath);
              szSubPath = (char*)malloc((pathLen + pls->keySize + 1) * sizeof(char));
              memcpy(szSubPath, szPath, pathLen);

              if (pathLen > 1) { // we are not root path
                szSubPath[pathLen] = '/';
                memcpy(&szSubPath[pathLen+1], pls->szKey, pls->keySize);
              }
              else {
                memcpy(&szSubPath[1], pls->szKey, pls->keySize);
              }

              // serialize subtree
              CSJSON_PRIVATE_SerializeSubDir(This, szSubPath, szOutStream, &curPos);

              // finish the object subtree
              (*szOutStream)[curPos] = ']';
              curPos++;
              free(szSubPath);

              break;

            case JSON_TYPE_OBJECT:

              // add key name to stream
              (*szOutStream)[curPos] = '"';
              curPos++;
              memcpy(&(*szOutStream)[curPos], pls->szKey, pls->keySize-1);
              curPos += pls->keySize-1;
              (*szOutStream)[curPos] = '"';
              curPos++;
              (*szOutStream)[curPos] = ':';
              curPos++;
              (*szOutStream)[curPos] = '{';
              curPos++;

              // create next item key
              pathLen = strlen(szPath);
              szSubPath = (char*)malloc((pathLen + pls->keySize + 1) * sizeof(char));
              memcpy(szSubPath, szPath, pathLen);

              if (pathLen > 1) { // we are not root path
                szSubPath[pathLen] = '/';
                memcpy(&szSubPath[pathLen+1], pls->szKey, pls->keySize);
              }
              else {
                memcpy(&szSubPath[1], pls->szKey, pls->keySize);
              }

              // serialize subtree
              CSJSON_PRIVATE_SerializeSubDir(This, szSubPath, szOutStream, &curPos);

              // finish the object subtree
              (*szOutStream)[curPos] = '}';
              curPos++;
              free(szSubPath);

              break;

            default:
              break;
          }

          (*szOutStream)[curPos] = ',';
          curPos++;
        }

        // to erase dangling comma aftert last element, if there are some
        if (count > 0) {
          curPos--;
        }

        (*szOutStream)[curPos] = '}';
        curPos++;

        break;

      default:

        break;
    }
  }

  (*szOutStream)[curPos] = 0;

  return curPos;
}

CSRESULT
  CSJSON_PRIVATE_SerializeSubDir
    (CSJSON* This,
     char*  szPath,
     char** szOutStream,
     long*  curPos) {

  long count;
  long i;
  long size;
  long pathLen;
  long indexLen;

  char* szSubPath;
  char* szKey;

  char szIndex[11];

  CSJSON_DIRENTRY* dire;
  CSJSON_LSENTRY* pls;

  if(CS_SUCCEED(CSMAP_Lookup(This->Object, szPath, (void**)(&dire), &size)))
  {
      switch (dire->type) {

        case JSON_TYPE_ARRAY:

          if (dire->Listing != 0) {

            count = CSLIST_Count(dire->Listing);

            for (i=0; i<count; i++) {

              CSLIST_GetDataRef(dire->Listing, (void**)(&pls), i);

              switch (pls->type) {
                case JSON_TYPE_STRING:
                  (*szOutStream)[*curPos] = '"';
                  (*curPos)++;
                  memcpy(&(*szOutStream)[(*curPos)], pls->szValue, pls->valueSize-1);
                  (*curPos) += pls->valueSize-1;
                  (*szOutStream)[*curPos] = '"';
                  (*curPos)++;
                  break;
                case JSON_TYPE_NUMERIC:
                  memcpy(&(*szOutStream)[(*curPos)], pls->szValue, pls->valueSize-1);
                  (*curPos) += pls->valueSize-1;
                  break;
                case JSON_TYPE_BOOL_FALSE:
                  memcpy(&(*szOutStream)[(*curPos)], "false", 5);
                  (*curPos) += 5;
                  break;
                case JSON_TYPE_BOOL_TRUE:
                  memcpy(&(*szOutStream)[(*curPos)], "true", 4);
                  (*curPos) += 4;
                  break;
                case JSON_TYPE_NULL:
                  memcpy(&(*szOutStream)[(*curPos)], "null", 4);
                  (*curPos) += 4;
                  break;

                case JSON_TYPE_ARRAY:

                  (*szOutStream)[*curPos] = '[';
                  (*curPos)++;

                  sprintf(szIndex, "%ld", i);
                  pathLen = strlen(szPath);
                  indexLen = strlen(szIndex);
                  szSubPath = (char*)malloc((pathLen + indexLen + 2) * sizeof(char));
                  memcpy(szSubPath, szPath, pathLen);

                  if (pathLen > 1) { // we are not root path
                    szSubPath[pathLen] = '/';
                    memcpy(&szSubPath[pathLen+1], szIndex, indexLen+1);
                  }
                  else {
                    memcpy(&szSubPath[1], szIndex, indexLen+1);
                  }

                  CSJSON_PRIVATE_SerializeSubDir(This, szSubPath, szOutStream, curPos);

                  (*szOutStream)[*curPos] = ']';
                  (*curPos)++;
                  free(szSubPath);

                  break;

                case JSON_TYPE_OBJECT:

                  (*szOutStream)[*curPos] = '{';
                  (*curPos)++;

                  sprintf(szIndex, "%ld", i);
                  pathLen = strlen(szPath);
                  indexLen = strlen(szIndex);
                  szSubPath = (char*)malloc((pathLen + indexLen + 2) * sizeof(char));
                  memcpy(szSubPath, szPath, pathLen);

                  if (pathLen > 1) { // we are not root path
                    szSubPath[pathLen] = '/';
                    memcpy(&szSubPath[pathLen+1], szIndex, indexLen+1);
                  }
                  else {
                    memcpy(&szSubPath[1], szIndex, indexLen+1);
                  }

                  CSJSON_PRIVATE_SerializeSubDir(This, szSubPath, szOutStream, curPos);

                  (*szOutStream)[*curPos] = '}';
                  (*curPos)++;
                  free(szSubPath);

                  break;

                default:

                  break;
              }

              (*szOutStream)[(*curPos)] = ',';
              (*curPos)++;
            }

            // to erase dangling comma aftert last element, if there are some
            if (count > 0) {
              (*curPos)--;
            }
          }

          break;

        case JSON_TYPE_OBJECT:

          count = 0;

          CSMAP_IterStart(dire->Listing);

          while (CS_SUCCEED(CSMAP_IterNext(dire->Listing, &szKey, (void**)(&pls), &size))) {

            count++;

            switch (pls->type) {

              case JSON_TYPE_STRING:
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], pls->szKey, pls->keySize-1);
                (*curPos) += pls->keySize-1;
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                (*szOutStream)[*curPos] = ':';
                (*curPos)++;
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], pls->szValue, pls->valueSize-1);
                (*curPos) += pls->valueSize-1;
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                break;
              case JSON_TYPE_NUMERIC:
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], pls->szKey, pls->keySize-1);
                (*curPos) += pls->keySize-1;
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                (*szOutStream)[*curPos] = ':';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], pls->szValue, pls->valueSize-1);
                (*curPos) += pls->valueSize-1;
                break;
              case JSON_TYPE_BOOL_FALSE:
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], pls->szKey, pls->keySize-1);
                (*curPos) += pls->keySize-1;
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                (*szOutStream)[*curPos] = ':';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], "false", 5);
                (*curPos) += 5;
                break;
              case JSON_TYPE_BOOL_TRUE:
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], pls->szKey, pls->keySize-1);
                (*curPos) += pls->keySize-1;
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                (*szOutStream)[*curPos] = ':';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], "true", 4);
                (*curPos) += 4;
                break;
              case JSON_TYPE_NULL:
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], pls->szKey, pls->keySize-1);
                (*curPos) += pls->keySize-1;
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                (*szOutStream)[*curPos] = ':';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], "null", 4);
                (*curPos) += 4;
                break;

              case JSON_TYPE_ARRAY:

                // add key name to stream
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], pls->szKey, pls->keySize-1);
                *curPos += pls->keySize-1;
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                (*szOutStream)[*curPos] = ':';
                (*curPos)++;
                (*szOutStream)[*curPos] = '[';
                (*curPos)++;

                // create next item key
                pathLen = strlen(szPath);
                szSubPath = (char*)malloc((pathLen + pls->keySize + 1) * sizeof(char));
                memcpy(szSubPath, szPath, pathLen);

                if (pathLen > 1) { // we are not root path
                  szSubPath[pathLen] = '/';
                  memcpy(&szSubPath[pathLen+1], pls->szKey, pls->keySize);
                }
                else {
                  memcpy(&szSubPath[1], pls->szKey, pls->keySize);
                }

                // serialize subtree
                CSJSON_PRIVATE_SerializeSubDir(This, szSubPath, szOutStream, curPos);

                // finish the object subtree
                (*szOutStream)[*curPos] = ']';
                (*curPos)++;
                free(szSubPath);

                break;

              case JSON_TYPE_OBJECT:

                // add key name to stream
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                memcpy(&(*szOutStream)[*curPos], pls->szKey, pls->keySize-1);
                (*curPos) += pls->keySize-1;
                (*szOutStream)[*curPos] = '"';
                (*curPos)++;
                (*szOutStream)[*curPos] = ':';
                (*curPos)++;
                (*szOutStream)[*curPos] = '{';
                (*curPos)++;

                // create next item key
                pathLen = strlen(szPath);
                szSubPath = (char*)malloc((pathLen + pls->keySize + 1) * sizeof(char));
                memcpy(szSubPath, szPath, pathLen);

                if (pathLen > 1) { // we are not root path
                  szSubPath[pathLen] = '/';
                  memcpy(&szSubPath[pathLen+1], pls->szKey, pls->keySize);
                }
                else {
                  memcpy(&szSubPath[1], pls->szKey, pls->keySize);
                }

                // serialize subtree
                CSJSON_PRIVATE_SerializeSubDir(This, szSubPath, szOutStream, curPos);

                // finish the object subtree
                (*szOutStream)[*curPos] = '}';
                (*curPos)++;
                free(szSubPath);

                break;

              default:
                break;
            }

            (*szOutStream)[(*curPos)] = ',';
            (*curPos)++;
          }

          // to erase dangling comma aftert last element, if there are some
          if (count > 0) {
            (*curPos)--;
          }

          break;

        default:

          break;
      }
  }

  return CS_SUCCESS;
}

CSRESULT
  CSJSON_InsertBool
    (CSJSON* This,
     char*   szPath,
     char*   szKey,
     int     boolValue) {

  long keySize;
  long size;

  CSJSON_DIRENTRY* ppve;
  CSJSON_LSENTRY lse;
  CSJSON_LSENTRY* plse;

  if (boolValue != JSON_TYPE_BOOL_FALSE) {
    boolValue = JSON_TYPE_BOOL_TRUE;
  }

  if (CS_SUCCEED(CSMAP_Lookup(This->Object,
                              szPath,
                              (void**)(&ppve),
                              &size)))
  {

    switch(ppve->type) {

      case JSON_TYPE_ARRAY:

        lse.szKey = 0;
        lse.keySize = 0;
        lse.szValue = 0;
        lse.type = boolValue;
        lse.valueSize = 0;
        CSLIST_Insert(ppve->Listing, (void*)(&lse), sizeof(CSJSON_LSENTRY), CSLIST_BOTTOM);

        ppve->numItems++;
        This->nextSlabSize = This->nextSlabSize + 6;

        break;

      case JSON_TYPE_OBJECT:

        keySize = strlen(szKey);

        // Check if key exists

        if (CS_SUCCEED(CSMAP_Lookup(ppve->Listing,
                                    szKey,
                                    (void**)(&plse),
                                    &size))) {

          // replace existing key

          switch(plse->type) {
            case JSON_TYPE_STRING:
              free(plse->szValue);
              This->nextSlabSize = This->nextSlabSize - plse->valueSize -2;
              break;
            case JSON_TYPE_NUMERIC:
              free(plse->szValue);
              This->nextSlabSize = This->nextSlabSize - plse->valueSize;
              break;
            case JSON_TYPE_BOOL_FALSE:
              This->nextSlabSize = This->nextSlabSize - 5;
              break;
            case JSON_TYPE_BOOL_TRUE:
              This->nextSlabSize = This->nextSlabSize - 5;
              break;
            case JSON_TYPE_NULL:
              This->nextSlabSize = This->nextSlabSize - 4;
              break;
          }

          plse->valueSize = 0;
          plse->szValue = 0;
          plse->type = boolValue;
          This->nextSlabSize = This->nextSlabSize + 5;

        }
        else {

          lse.keySize = keySize+1;
          lse.szKey = (char*)malloc(lse.keySize * sizeof(char));
          memcpy(lse.szKey, szKey, lse.keySize);
          lse.valueSize = 0;
          lse.szValue = 0;
          lse.type = boolValue;
          CSMAP_Insert(ppve->Listing, szKey, (void*)(&lse), sizeof(CSJSON_LSENTRY));

          ppve->numItems++;
          This->nextSlabSize = This->nextSlabSize + keySize +  + 9;
        }

        break;

      default:

        return CS_FAILURE;
    }

    return CS_SUCCESS;
  }

  return CS_FAILURE;
}

CSRESULT
  CSJSON_InsertNull
    (CSJSON* This,
     char*   szPath,
     char*   szKey) {

  CSJSON_DIRENTRY* ppve;
  CSJSON_LSENTRY lse;
  CSJSON_LSENTRY* plse;

  long keySize;
  long size;

  if (CS_SUCCEED(CSMAP_Lookup(This->Object,
                              szPath,
                              (void**)(&ppve),
                              &size)))
  {

    switch(ppve->type) {

      case JSON_TYPE_ARRAY:

        lse.szKey = 0;
        lse.keySize = 0;
        lse.szValue = 0;
        lse.type = JSON_TYPE_NULL;
        lse.valueSize = 0;
        CSLIST_Insert(ppve->Listing, (void*)(&lse), sizeof(CSJSON_LSENTRY), CSLIST_BOTTOM);

        ppve->numItems++;
        This->nextSlabSize = This->nextSlabSize + 5;

        break;

      case JSON_TYPE_OBJECT:

        keySize = strlen(szKey);

        // Check if key exists

        if (CS_SUCCEED(CSMAP_Lookup(ppve->Listing,
                                    szKey,
                                    (void**)(&plse),
                                    &size))) {

          // replace existing key

          switch(plse->type) {
            case JSON_TYPE_STRING:
              free(plse->szValue);
              This->nextSlabSize = This->nextSlabSize - plse->valueSize -2;
              break;
            case JSON_TYPE_NUMERIC:
              free(plse->szValue);
              This->nextSlabSize = This->nextSlabSize - plse->valueSize;
              break;
            case JSON_TYPE_BOOL_FALSE:
              This->nextSlabSize = This->nextSlabSize - 5;
              break;
            case JSON_TYPE_BOOL_TRUE:
              This->nextSlabSize = This->nextSlabSize - 5;
              break;
            case JSON_TYPE_NULL:
              This->nextSlabSize = This->nextSlabSize - 4;
              break;
          }

          plse->valueSize = 0;
          plse->szValue = 0;
          plse->type = JSON_TYPE_NULL;
          This->nextSlabSize = This->nextSlabSize + 5;

        }
        else {

          lse.keySize = keySize+1;
          lse.szKey = (char*)malloc(lse.keySize * sizeof(char));
          memcpy(lse.szKey, szKey, lse.keySize);
          lse.valueSize = 0;
          lse.szValue = 0;
          lse.type = JSON_TYPE_NULL;
          CSMAP_Insert(ppve->Listing, szKey, (void*)(&lse), sizeof(CSJSON_LSENTRY));

          ppve->numItems++;
          This->nextSlabSize = This->nextSlabSize + keySize + 8;
        }

        break;

      default:

        return CS_FAILURE;
    }

    return CS_SUCCESS;
  }

  return CS_FAILURE;
}

CSRESULT
  CSJSON_InsertNumeric
    (CSJSON* This,
     char*   szPath,
     char*   szKey,
     char*   szValue) {

  CSJSON_DIRENTRY* ppve;
  CSJSON_LSENTRY lse;
  CSJSON_LSENTRY* plse;

  long valueSize;
  long keySize;
  long size;

  if (CS_SUCCEED(CSMAP_Lookup(This->Object,
                              szPath,
                              (void**)(&ppve),
                              &size)))
  {
    valueSize = strlen(szValue);

    switch(ppve->type) {

      case JSON_TYPE_ARRAY:

        lse.szKey = 0;
        lse.keySize = 0;
        lse.szValue = (char*)malloc((valueSize+1) * sizeof(char));
        memcpy(lse.szValue, szValue, valueSize+1);
        lse.type = JSON_TYPE_NUMERIC;
        lse.valueSize = valueSize+1;
        CSLIST_Insert(ppve->Listing, (void*)(&lse), sizeof(CSJSON_LSENTRY), CSLIST_BOTTOM);

        ppve->numItems++;
        This->nextSlabSize = This->nextSlabSize + valueSize + 1;

        break;

      case JSON_TYPE_OBJECT:

        keySize = strlen(szKey);

        // Check if key exists

        if (CS_SUCCEED(CSMAP_Lookup(ppve->Listing,
                                    szKey,
                                    (void**)(&plse),
                                    &size))) {

          // replace existing key

          switch(plse->type) {
            case JSON_TYPE_STRING:
              free(plse->szValue);
              This->nextSlabSize = This->nextSlabSize - plse->valueSize -2;
              break;
            case JSON_TYPE_NUMERIC:
              free(plse->szValue);
              This->nextSlabSize = This->nextSlabSize - plse->valueSize;
              break;
            case JSON_TYPE_BOOL_FALSE:
              This->nextSlabSize = This->nextSlabSize - 5;
              break;
            case JSON_TYPE_BOOL_TRUE:
              This->nextSlabSize = This->nextSlabSize - 4;
              break;
            case JSON_TYPE_NULL:
              This->nextSlabSize = This->nextSlabSize - 4;
              break;
          }

          plse->valueSize = valueSize+1;
          plse->szValue = (char*)malloc(plse->valueSize * sizeof(char));
          memcpy(plse->szValue, szValue, plse->valueSize);
          plse->type = JSON_TYPE_NUMERIC;
          This->nextSlabSize = This->nextSlabSize + valueSize + 4;

        }
        else {

          lse.keySize = keySize+1;
          lse.szKey = (char*)malloc(lse.keySize * sizeof(char));
          memcpy(lse.szKey, szKey, lse.keySize);
          lse.valueSize = valueSize+1;
          lse.szValue = (char*)malloc((lse.valueSize) * sizeof(char));
          memcpy(lse.szValue, szValue, lse.valueSize);
          lse.type = JSON_TYPE_NUMERIC;
          CSMAP_Insert(ppve->Listing, szKey, (void*)(&lse), sizeof(CSJSON_LSENTRY));

          ppve->numItems++;
          This->nextSlabSize = This->nextSlabSize + keySize + valueSize + 4;
        }

        break;

      default:

        return CS_FAILURE;
    }

    return CS_SUCCESS;
  }

  return CS_FAILURE;
}

CSRESULT
  CSJSON_InsertString
    (CSJSON* This,
     char*  szPath,
     char*  szKey,
     char*  szValue) {

  CSJSON_DIRENTRY* ppve;
  CSJSON_LSENTRY lse;
  CSJSON_LSENTRY* plse;

  long valueSize;
  long keySize;
  long size;

  if (CS_SUCCEED(CSMAP_Lookup(This->Object,
                              szPath,
                              (void**)(&ppve),
                              &size)))
  {
    valueSize = strlen(szValue);

    switch(ppve->type) {

      case JSON_TYPE_ARRAY:

        lse.szKey = 0;
        lse.keySize = 0;
        lse.szValue = (char*)malloc((valueSize+1) * sizeof(char));
        memcpy(lse.szValue, szValue, valueSize+1);
        lse.type = JSON_TYPE_STRING;
        lse.valueSize = valueSize+1;
        CSLIST_Insert(ppve->Listing, (void*)(&lse), sizeof(CSJSON_LSENTRY), CSLIST_BOTTOM);

        ppve->numItems++;
        This->nextSlabSize = This->nextSlabSize + valueSize + 3;

        break;

      case JSON_TYPE_OBJECT:

        keySize = strlen(szKey);

        // Check if key exists

        if (CS_SUCCEED(CSMAP_Lookup(ppve->Listing,
                                    szKey,
                                    (void**)(&plse),
                                    &size))) {

          // replace existing key

          switch(plse->type) {
            case JSON_TYPE_STRING:
              free(plse->szValue);
              This->nextSlabSize = This->nextSlabSize - plse->valueSize -2;
              break;
            case JSON_TYPE_NUMERIC:
              free(plse->szValue);
              This->nextSlabSize = This->nextSlabSize - plse->valueSize;
              break;
            case JSON_TYPE_BOOL_FALSE:
              This->nextSlabSize = This->nextSlabSize - 5;
              break;
            case JSON_TYPE_BOOL_TRUE:
              This->nextSlabSize = This->nextSlabSize - 4;
              break;
            case JSON_TYPE_NULL:
              This->nextSlabSize = This->nextSlabSize - 4;
              break;
          }

          plse->valueSize = valueSize+1;
          plse->szValue = (char*)malloc(plse->valueSize * sizeof(char));
          memcpy(plse->szValue, szValue, plse->valueSize);
          plse->type = JSON_TYPE_STRING;
          This->nextSlabSize = This->nextSlabSize + valueSize + 6;

        }
        else {

          lse.keySize = keySize+1;
          lse.szKey = (char*)malloc(lse.keySize * sizeof(char));
          memcpy(lse.szKey, szKey, lse.keySize);
          lse.valueSize = valueSize+1;
          lse.szValue = (char*)malloc((lse.valueSize) * sizeof(char));
          memcpy(lse.szValue, szValue, lse.valueSize);
          lse.type = JSON_TYPE_STRING;
          CSMAP_Insert(ppve->Listing, szKey, (void*)(&lse), sizeof(CSJSON_LSENTRY));

          ppve->numItems++;
          This->nextSlabSize = This->nextSlabSize + keySize + valueSize + 6;
        }

        break;

      default:

        return CS_FAILURE;
    }

    return CS_SUCCESS;
  }

  return CS_FAILURE;
}

CSRESULT
  CSJSON_MkDir
    (CSJSON* This,
     char*  szPath,
     char*  szKey,
     int    type) {

  CSRESULT Rc;
  CSJSON_DIRENTRY* ppdire;
  CSJSON_DIRENTRY* pp_testDire;
  CSJSON_DIRENTRY dire;
  CSJSON_LSENTRY lse;

  char* szNewPath;

  char szIndex[11];

  long size;
  long len;
  long keyLen;
  long indexLen;
  long totalSize;

  if (type != JSON_TYPE_ARRAY && type != JSON_TYPE_OBJECT) {
    return CS_FAILURE;
  }

  if (CS_SUCCEED(CSMAP_Lookup(This->Object,
                              szPath,
                              (void**)(&ppdire),
                              &size)))
  {
    switch (ppdire->type) {

      case JSON_TYPE_ARRAY:

        len = strlen(szPath);
        sprintf(szIndex, "%ld", ppdire->numItems);
        indexLen = strlen(szIndex);

        if (szPath[1] == 0) {
          totalSize = len + indexLen + 1;
          // allocate for base path, and the next index
          szNewPath = (char*)malloc(totalSize * sizeof(char));
          memcpy(szNewPath, szPath, len);
          memcpy(&szNewPath[len], szIndex, indexLen);
          szNewPath[len+indexLen] = 0;
        }
        else {
          totalSize = len + indexLen + 2;
          // allocate for base path, a slash and the next index
          szNewPath = (char*)malloc(totalSize * sizeof(char));
          memcpy(szNewPath, szPath, len);
          szNewPath[len] = '/';
          memcpy(&szNewPath[len+1], szIndex, indexLen);
          szNewPath[len+indexLen+1] = 0;
        }

        lse.szKey = 0;
        lse.szValue = 0;
        lse.type = type;
        lse.keySize = 0;
        lse.valueSize = 0;
        CSLIST_Insert(ppdire->Listing, (void*)(&lse), sizeof(CSJSON_LSENTRY), CSLIST_BOTTOM);

        ppdire->numItems++;

        dire.type = type;
        dire.numItems = 0;

        if (type == JSON_TYPE_ARRAY) {
          dire.Listing = CSLIST_Constructor();
        }
        else {
          dire.Listing = CSMAP_Constructor();
        }

        CSMAP_InsertKeyRef(This->Object, szNewPath, (void*)(&dire), sizeof(CSJSON_DIRENTRY));

        This->nextSlabSize += 3; // to braces/brackets and possibly a comma

        Rc = CS_SUCCESS;
        break;

      case JSON_TYPE_OBJECT:

        if (szKey == 0) {
          return CS_FAILURE;
        }

        len = strlen(szPath);
        keyLen = strlen(szKey);
        totalSize = len + keyLen + 2;

        if (szPath[1] == 0) {
          totalSize = len + keyLen + 1;
          // allocate for base path, and the key
          szNewPath = (char*)malloc(totalSize * sizeof(char));
          memcpy(szNewPath, szPath, len);
          memcpy(&szNewPath[len], szKey, keyLen);
          szNewPath[len+keyLen] = 0;
        }
        else {
          totalSize = len + keyLen + 2;
          // allocate for base path, a slash and the key
          szNewPath = (char*)malloc(totalSize * sizeof(char));
          memcpy(szNewPath, szPath, len);
          szNewPath[len] = '/';
          memcpy(&szNewPath[len+1], szKey, keyLen);
          szNewPath[len+keyLen+1] = 0;
        }

        // Check if key exists
        if (CS_SUCCEED(CSMAP_Lookup(This->Object,
                                    szNewPath,
                                    (void**)(&pp_testDire),
                                    &size))) {

          free(szNewPath);
          Rc = CS_FAILURE;
        }
        else {

          lse.szKey = (char*)malloc((keyLen+1) * sizeof(char));
          memcpy(lse.szKey, szKey, keyLen+1);
          lse.keySize = keyLen+1;
          lse.type = type;
          lse.valueSize = 0;
          lse.szValue = 0;
          CSMAP_Insert(ppdire->Listing, szKey, (void*)(&lse), sizeof(CSJSON_LSENTRY));

          ppdire->numItems++;

          dire.type = type;
          dire.numItems = 0;

          if (type == JSON_TYPE_ARRAY) {
            dire.Listing = CSLIST_Constructor();
          }
          else {
            dire.Listing = CSMAP_Constructor();
          }

          CSMAP_InsertKeyRef(This->Object, szNewPath, (void*)(&dire), sizeof(CSJSON_DIRENTRY));

          This->nextSlabSize = This->nextSlabSize + lse.keySize + 6; // to braces/brackets and possibly a comma and two

          Rc = CS_SUCCESS;
        }

        break;

      default:
        Rc = CS_FAILURE;
    }
  }
  else {
    Rc = CS_FAILURE;
  }

  return Rc;
}

CSRESULT
  CSJSON_Ls
    (CSJSON* This,
     char* szPath,
     CSLIST listing) {

  CSJSON_DIRENTRY* pve;
  CSJSON_LSENTRY* plse;

  char* szKey;

  long size;
  long count;
  long i;

  CSLIST_Clear(listing);

  if(CS_SUCCEED(CSMAP_Lookup(This->Object, szPath, (void**)(&pve), &size)))  {

    if (pve->type == JSON_TYPE_OBJECT) {

      CSMAP_IterStart(pve->Listing);

      while(CS_SUCCEED(CSMAP_IterNext(pve->Listing, &szKey, (void**)(&plse), &size))) {

        // insert address of listing node
        CSLIST_Insert(listing, (void*)(&plse), sizeof(plse), CSLIST_BOTTOM);
      }
    }
    else {

      if (pve->type == JSON_TYPE_ARRAY) {

        count = CSLIST_Count(pve->Listing);

        for (i=0; i<count; i++) {
          CSLIST_GetDataRef(pve->Listing, (void**)(&plse), i);
          CSLIST_Insert(listing, (void*)(&plse), sizeof(plse), CSLIST_BOTTOM);
        }
      }
      else {
        return JSON_TYPE_UNKNOWN;
      }
    }
  }
  else {
    return JSON_TYPE_UNKNOWN;
  }

  return pve->type;
}


CSRESULT CSJSON_PRIVATE_IsNumeric(char* szNumber) {

  CSRESULT Rc;

  long n;

  int Stop;
  int haveDot;
  int haveExp;

  haveDot = 0;
  haveExp = 0;
  Stop = 0;

  Rc = CS_SUCCESS;
  n=0;

  if ((szNumber[n] >= '0' && szNumber[n] <= '9') ||
        (szNumber[n] == '-'))
  {

    Stop = 0;
    haveDot = 0;
    haveExp = 0;

    n++;
    while (szNumber[n] != 0 && !Stop) {

      switch (szNumber[n])
      {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          n++;
          break;

        case '.':

          // Only one dot and it must precede the exponent character

          if (!haveDot && !haveExp) {

            // check that if first digit is zero, then there are no
            // other digits between the zero and the dot

            if (szNumber[0] == '-') {
              if (szNumber[1] == '0') {
                if (n != 2) {
                  //number does not starts with -0 and next character is not dot
                  Stop = 1;
                  Rc = CS_FAILURE;
                }
                else {
                  n++;
                }
              }
              else {
                n++;
              }
            }
            else {
              if (szNumber[0] == '0') {
                if (n != 1) {
                  //number starts with 0 but next character is not dot
                  Stop = 1;
                  Rc = CS_FAILURE;
                }
                else {
                  n++;
                }
              }
              else {
                n++;
              }
            }

            haveDot = 1;
          }
          else {
            Stop = 1;
            Rc = CS_FAILURE;
          }

          break;

        case '+':
        case '-':

          // This must immediately follow the exponent character
          if (szNumber[n-1] == 'e' || szNumber[n-1] == 'E') {
            n++;
          }
          else {
            Stop = 1;
            Rc = CS_FAILURE;
          }

          break;

        case 'e':
        case 'E':

          if (!haveExp) {
            // This character must be preceded by a digit
            if (szNumber[n-1] >= '0' && szNumber[n-1] <= '9') {
              haveExp = 1;
              n++;
            }
            else {
              Stop = 1;
              Rc = CS_FAILURE;
            }
          }
          else {
            Stop = 1;
            Rc = CS_FAILURE;
          }

          break;

        default:

          Stop = 1;
          break;

      }
    }
  }

  return Rc;
} 
