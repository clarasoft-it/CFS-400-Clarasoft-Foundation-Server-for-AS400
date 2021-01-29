/* ==========================================================================
  Clarasoft Core Tools
  cslib.c
  string implementation
  Version 1.0.0

  Compile module with:
     CRTCMOD MODULE(CSLIB) SRCFILE(QCSRCX) DBGVIEW(*ALL)

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
#include <QSYSINC/MIH/CVTHC>
#include <QSYSINC/MIH/GENUUID>
#include <QUSRJOBI.h>
#include <QSYRUSRI.h>
#include <QUSEC.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CS_SUCCESS                     (0x00000000)
#define CS_FAILURE                     (0x80000000)

#define CS_MASK_OPER                   (0x0FFF0000)
#define CS_MASK_DIAG                   (0x0000FFFF)

#define CS_SUCCEED(x)                  (((x) & (0xF0000000)) == (0))
#define CS_FAIL(x)                     ((x) & (CS_FAILURE))
#define CS_OPER(x)                     ((x) & (CS_MASK_OPER))
#define CS_DIAG(x)                     ((x) & (CS_MASK_DIAG))

#define CS_OPER_NOVALUE                (0x00000000)
#define CS_DIAG_NOVALUE                (0x00000000)
#define CS_DIAG_UNKNOWN                (0x0000FFFF)

typedef long CSRESULT;

#define CSLIST_TOP                     (0x00000000)   // at the beginning of the list
#define CSLIST_BOTTOM                  (0x7FFFFFFF)   // at the end of the list

#define B64_MASK_11                    (0xFC)
#define B64_MASK_12                    (0x03)
#define B64_MASK_21                    (0xF0)
#define B64_MASK_22                    (0x0F)
#define B64_MASK_31                    (0xC0)
#define B64_MASK_32                    (0x3F)

//#define CCSID_JOBDEFAULT               (0)
//#define CCSID_UTF8                     (1208)
//#define CCSID_ASCII                    (819)
//#define CCSID_WINDOWS_1251             (1251)
//#define CCSID_ISO_8859_1               (819)

#define CSSTR_B64_LINEBREAK_NONE       (0x00000000)
#define CSSTR_B64_LINEBREAK_LF         (0x00000001)
#define CSSTR_B64_LINEBREAK_CRLF       (0x00000002)
#define CSSTR_B64_LINEBREAK_OFFSET     (0x0000004D)

#define CSSTR_B64_MASK_LINEBREAK       (0x0000000F)
#define CSSTR_B64_IGNOREINVALIDCHAR    (0x00000100)

#define CSSTR_INPUT_ASCII              (0x00000001)
#define CSSTR_INPUT_EBCDIC             (0x00000002)
#define CSSTR_OUTPUT_ASCII             (0x00000004)
#define CSSTR_OUTPUT_EBCDIC            (0x00000008)

#define CSSTR_URLENCODE_XLATERESERVED  (0x00000010)
#define CSSTR_URLENCODE_SPACETOPLUS    (0x00000020)

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

#define CSSYS_UUID_BUFFERSIZE          (37)
#define CSSYS_UUID_UPPERCASE           (0x00000000)
#define CSSYS_UUID_LOWERCASE           (0x00000001)
#define CSSYS_UUID_DASHES              (0x00000002)

typedef struct tagJOBINFOSTRUCT {

  int  bytesReturned;
  int  bytesAvailable;
  char JobName[10];
  char JobUser[10];
  char JobNumber[6];
  char JobInternalID[16];
  char JobStatus[10];
  char JobType;
  char JobSubType;
  char reserved[2];
  int  Priority;
  int  Timeslice;
  int  DefaultWait;
  char Purge[10];

}JOBINFOSTRUCT;

typedef struct tagCSLISTNODE {

  struct tagCSLISTNODE * previous;
  struct tagCSLISTNODE * next;

  void* data;
  long dataSize;

} CSLISTNODE;

typedef CSLISTNODE* PCSLISTNODE;

typedef struct tagCSLIST {

  PCSLISTNODE first;
  PCSLISTNODE last;
  PCSLISTNODE current;

  long numItems;
  long curIndex;

} CSLIST;

typedef struct tagCSMAPNODE {

  /* data item */

  char* key;
  char* value;
  int   keySize;
  int   valueSize;

  /* Links */

  struct tagCSMAPNODE* parent;
  struct tagCSMAPNODE* left;
  struct tagCSMAPNODE* right;

  /* To manage balancing */

  int   height;

} CSMAPNODE;

typedef CSMAPNODE* pAVLTREE;

typedef struct tagCSMAP {

   pAVLTREE tree;

   /* For iterator */

   CSLIST* keys;
   long NextKey;

}CSMAP;

typedef struct tagCSSTRCV {

   CSLIST* Tokens;
   char szFromCCSID[65];
   char szToCCSID[65];
   long bufferSize;
   long numOfChars;
   char leftOver[4];
   int leftOverSize;
   int missingSize;
   iconv_t cd;

} CSSTRCV;

/* ------------------------------------------------------------------------------
  private prototypes
------------------------------------------------------------------------------ */

CSRESULT
  CSLIST_PRIVATE_Goto
    (CSLIST* This,
     long    index);

pAVLTREE
  CSMAP_PRIVATE_Insert
     (pAVLTREE Tree,
       pAVLTREE parent,
       char*    key,
       void*    value,
       long     valueSize);

pAVLTREE
  CSMAP_PRIVATE_InsertKeyRef
     (pAVLTREE Tree,
       pAVLTREE parent,
       char*    key,
       void*    value,
       long     valueSize);

pAVLTREE
  CSMAP_PRIVATE_Remove
    (pAVLTREE Tree,
       char* key);

CSRESULT
  CSMAP_PRIVATE_Lookup
     (pAVLTREE Tree,
       char*    key,
       void**   value,
       long*    valueSize);

pAVLTREE
  CSMAP_PRIVATE_SingleLeftRotation
     (pAVLTREE Tree);

pAVLTREE
  CSMAP_PRIVATE_SingleRightRotation
     (pAVLTREE Tree);

pAVLTREE
  CSMAP_PRIVATE_DoubleLeftRotation
     (pAVLTREE Tree);

pAVLTREE
  CSMAP_PRIVATE_DoubleRightRotation
     (pAVLTREE Tree);

pAVLTREE
  CSMAP_PRIVATE_Successor
     (pAVLTREE Tree);

pAVLTREE
  CSMAP_PRIVATE_Rebalance
     (pAVLTREE Tree);

void
  CSMAP_PRIVATE_ResetTreeHeight
     (pAVLTREE Tree);

void
  CSMAP_PRIVATE_Clear
     (pAVLTREE Tree);

long
  CSMAP_PRIVATE_Height
     (pAVLTREE Tree);

long
  CSMAP_PRIVATE_ComputeLeftHeight
     (pAVLTREE Tree);

long
  CSMAP_PRIVATE_ComputeRightHeight
     (pAVLTREE Tree);

pAVLTREE
  CSMAP_PRIVATE_Traverse
     (pAVLTREE Tree, CSLIST* keys);

CSRESULT
  CSSTRCV_PRV_Convert
    (CSSTRCV*  This,
     char*     strFrom,
     size_t    strFromMaxSize,
     size_t*   strFromConvertedSize,
     char*     strTo,
     size_t    strToMaxSize,
     size_t*   strToConvertedSize);

/* ------------------------------------------------------------------------
 * Implementation
 * --------------------------------------------------------------------- */


/* --------------------------------------------------------------------------
   CSLIST_Constructor
   Creates an instance of type CSLIST.
-------------------------------------------------------------------------- */

CSLIST*
  CSLIST_Constructor
    (void) {

  CSLIST* pList;

  pList = (CSLIST*)malloc(sizeof(CSLIST));

  pList->first   = 0;
  pList->last    = 0;
  pList->current = 0;

  pList->numItems = 0;
  pList->curIndex = 0;

  return pList;
}

/* --------------------------------------------------------------------------
   CSLIST_Destructor
   Releases the resources of an instance of type CSLIST.
-------------------------------------------------------------------------- */

void
  CSLIST_Destructor
    (CSLIST** This) {

  long i;
  CSLISTNODE* pCurNode;
  CSLISTNODE* pNextNode;

  pCurNode = (*This)->first;
  for (i=0; i<(*This)->numItems; i++) {

    pNextNode = pCurNode->next;

    if (pCurNode->dataSize > 0)
    {
      free(pCurNode->data);
    }

    free(pCurNode);
    pCurNode = pNextNode;
  }

  free(*This);
  *This = 0;
}

/* --------------------------------------------------------------------------
   CSLIST_Clear
   Removes all items from a CSLIST.
-------------------------------------------------------------------------- */

void
  CSLIST_Clear
    (CSLIST* This)
{
  long i;
  CSLISTNODE* pCurNode;
  CSLISTNODE* pNextNode;

  if (This->first != 0) {

    pCurNode = This->first;
    for (i=0; i<This->numItems; i++) {

      pNextNode = pCurNode->next;

      if (pCurNode->dataSize > 0)
      {
        free(pCurNode->data);
      }

      free(pCurNode);
      pCurNode = pNextNode;
    }
  }

  This->numItems = 0;
  This->curIndex = 0;
  This->current  = 0;
  This->first    = 0;
  This->last     = 0;

  return;
}

/* --------------------------------------------------------------------------
   CSLIST_Insert
   Inserts a new item in a CSLIST at a specified index.
-------------------------------------------------------------------------- */

CSRESULT
  CSLIST_Insert
    (CSLIST* This,
     void*   value,
     long valueSize,
     long index) {

  CSLISTNODE* NewNode;

  if (This == 0) {
    return CS_FAILURE;
  }

  if (This->numItems == CSLIST_BOTTOM)
  {
    // Very unlikely but who knows!
    return CS_FAILURE;
  }

  // Make sure index makes sense

  if (index >= This->numItems)
  {
    // index is zero-based; if it is
    // equal to at least the number
    // of items, we assume caller
    // wants to insert at the end of
    // the list

    index = CSLIST_BOTTOM;
  }
  else {

    if (index < 0)
    {
      index = CSLIST_BOTTOM;
    }
  }

  // Allocate new node

  NewNode = (CSLISTNODE*)malloc(sizeof(CSLISTNODE));

  NewNode->next     = 0;
  NewNode->previous = 0;
  NewNode->dataSize = valueSize;

  if (NewNode->dataSize > 0)
  {
    NewNode->data = (void*)malloc(NewNode->dataSize);
    memcpy(NewNode->data, value, NewNode->dataSize);
  }
  else
  {
    NewNode->data = 0;
  }

  // insert new node

  if (This->numItems > 0)
  {
    switch(index)
    {
      case CSLIST_TOP:  // We want a new first item

        This->first->previous = NewNode;
        NewNode->next         = This->first;
        This->first           = NewNode;
        This->curIndex        = 0;

        break;

      case CSLIST_BOTTOM: // insert at the end of the list

        CSLIST_PRIVATE_Goto(This, This->numItems-1);

        This->last->next = NewNode;
        NewNode->previous = This->last;

        This->last = NewNode;

        This->curIndex = This->numItems; // because it's zero-based
                                         // we will increment the
                                         // item count later
        break;

      default:

        CSLIST_PRIVATE_Goto(This, index);

        This->current->previous->next = NewNode;
        NewNode->previous = This->current->previous;

        NewNode->next = This->current;
        This->current->previous = NewNode;

        // Note that we need not update the current index
        // because we are merely replacing the current index node
        //This->curIndex = index;

        break;
    }
  }
  else
  {
    // This is the first item in the list
    This->first    = NewNode;
    This->last     = NewNode;
    This->curIndex = 0;
  }

  This->current = NewNode;
  This->numItems++;

  return CS_SUCCESS;
}

/* --------------------------------------------------------------------------
   CSLIST_Remove
   Removes an item in a CSLIST at a specified index.
-------------------------------------------------------------------------- */

CSRESULT
  CSLIST_Remove
    (CSLIST* This,
     long    index) {

  PCSLISTNODE Temp;

  if (index >= This->numItems) {
    return CS_FAILURE;
  }

  if (This->numItems > 0)
  {
    CSLIST_PRIVATE_Goto(This, index);

    Temp = This->current;

    if (This->current == This->first)
    {
      This->first = This->current->next;

      if (This->first != 0) {
        //This is the case where there is only one node
        This->first->previous = 0;
      }

      This->current = This->first;
    }
    else
    {
      if (This->current == This->last)
      {
        This->last = This->last->previous;

        // NOTE: we need not check if there is only one node
        //       because it would have been the first node also
        //       and it would have been taken care of above

        This->last->next = 0;

        This->current = This->last;
        This->curIndex--;
      }
      else
      {
        This->current->previous->next
                  = This->current->next;
        This->current->next->previous
                  = This->current->previous;
        This->current = This->current->next;
      }
    }

    if (Temp->dataSize > 0)
    {
      free(Temp->data);
    }

    free(Temp);

    This->numItems--;

    if (This->numItems == 0)
    {
      This->first    = 0;
      This->last     = 0;
      This->current  = 0;
      This->curIndex = 0;
    }
  }
  else
  {
    return CS_FAILURE;
  }

  return CS_SUCCESS;
}

/* --------------------------------------------------------------------------
   CSLIST_Count
   Removes the number of items in a CSLIST.
-------------------------------------------------------------------------- */

long
  CSLIST_Count
    (CSLIST* This)
{
    return This->numItems;
}

/* --------------------------------------------------------------------------
   CSLIST_Get
   Retrieves a copy a an item from the CSLIST.
-------------------------------------------------------------------------- */

CSRESULT
  CSLIST_Get
    (CSLIST* This,
     void*   value,
     long index)
{
  if (index >= This->numItems) {
    if (index == CSLIST_BOTTOM) {
      index = This->numItems-1;
    }
    else {
      return CS_FAILURE;
    }
  }

  if (This->numItems > 0)
  {
    CSLIST_PRIVATE_Goto(This, index);
    memcpy(value, This->current->data,
                This->current->dataSize);
    return CS_SUCCESS;
  }
  else
  {
    return CS_FAILURE;
  }

  return CS_SUCCESS;
}

/* --------------------------------------------------------------------------
   CSLIST_GetDataRef
   Returns the address of a specified item; the caller
   can directly access the CSLIST item. Use with care!!!
-------------------------------------------------------------------------- */

CSRESULT
  CSLIST_GetDataRef
    (CSLIST* This,
     void**  value,
     long    index)
{
  if (index >= This->numItems) {
    if (index == CSLIST_BOTTOM) {
      index = This->numItems-1;
    }
    else {
      return CS_FAILURE;
    }
  }

  if (This->numItems > 0)
  {
    CSLIST_PRIVATE_Goto(This, index);

    if (This->current->dataSize > 0)
    {
      *value = This->current->data;
    }
    else {
      *value = 0;
    }
  }
  else
  {
    *value = 0;
  }

  return CS_SUCCESS;
}

/* --------------------------------------------------------------------------
   CSLIST_Set
   Replaces an existing item at a specified index location.
-------------------------------------------------------------------------- */

CSRESULT
  CSLIST_Set
    (CSLIST* This,
     void*   value,
     long    valueSize,
     long    index)
{
  if (This->numItems > 0)
  {
    CSLIST_PRIVATE_Goto(This, index);

    if (This->current->dataSize > 0)
    {
      free(This->current->data);
    }

    if (valueSize > 0)
    {
      This->current->data = (void*)malloc(valueSize);
      memcpy(This->current->data, value, valueSize);
    }
    else
    {
      This->current->data = 0;
    }

    This->current->dataSize = valueSize;
  }
  else
  {
    return CS_FAILURE;
  }

  return CS_SUCCESS;
}

/* --------------------------------------------------------------------------
   CSLIST_ItemSize
   returns the size in bytes of an existing item at a
   specified index location.
-------------------------------------------------------------------------- */

long
  CSLIST_ItemSize
    (CSLIST* This,
     long    index) {

  if (This->numItems > 0)
  {
    CSLIST_PRIVATE_Goto(This, index);

    return This->current->dataSize;
  }
  else
  {
    return -1;
  }
}

/* --------------------------------------------------------------------------
   CSLIST_PRIVATE_Goto
   For internal use by a CSLIST; positions the current node
   pointer to a specified item based on its index.
-------------------------------------------------------------------------- */

CSRESULT
  CSLIST_PRIVATE_Goto
    (CSLIST* This,
     long index)
{
  // This method is to speed up positioning on average

  if (index < 0)
  {
    // assume we want to go at the end of the list
    index = This->numItems - 1;
  }

  if (index != This->curIndex)
  {
    if (index < This->curIndex)
    {
      while (index < This->curIndex)
      {
        This->current = This->current->previous;
        This->curIndex--;
      }
    }
    else
    {
      if (index > This->curIndex)
      {
        if (index > (This->numItems - 1))
        {
          index = This->numItems-1;
        }

        while (index > This->curIndex)
        {
          This->current = This->current->next;
          This->curIndex++;
        }
      }
    }
  }

  return CS_SUCCESS;
}

CSMAP*
  CSMAP_Constructor
     (void) {

   CSMAP* pInstance;

   pInstance = (CSMAP*)malloc(sizeof(CSMAP));

   pInstance->tree = 0;

   pInstance->keys = CSLIST_Constructor();

   return pInstance;
}

CSRESULT
  CSMAP_Clear
     (CSMAP* This)
{
  CSLIST_Clear(This->keys);
  CSMAP_PRIVATE_Clear(This->tree);

  This->tree = 0;

  return CS_SUCCESS;
}

void
  CSMAP_Destructor
     (CSMAP** This) {

   if ((*This)->tree != 0)
   {
      CSMAP_Clear(*This);
   }

   CSLIST_Destructor(&(((*This)->keys)));
   free(*This);

   *This = 0;
}

CSRESULT
  CSMAP_Insert
     (CSMAP* This,
       char*  key,
       void*  value,
       long   valueSize) {

   This->tree = CSMAP_PRIVATE_Insert(This->tree,
                             0, key,  value, valueSize);
   return CS_SUCCESS;
}

CSRESULT
  CSMAP_InsertKeyRef
     (CSMAP* This,
       char*  key,
       void*  value,
       long   valueSize) {

   This->tree = CSMAP_PRIVATE_InsertKeyRef(This->tree,
                               0, key,  value, valueSize);
   return CS_SUCCESS;
}

CSRESULT
  CSMAP_Remove
     (CSMAP* This,
       char*  key) {

   This->tree = CSMAP_PRIVATE_Remove(This->tree, key);
   return CS_SUCCESS;
}

CSRESULT
  CSMAP_Lookup
     (CSMAP* This,
       char*  key,
       void** value,
       long*  valueSize) {

  return CSMAP_PRIVATE_Lookup(This->tree,
                               key, value, valueSize);
}

CSRESULT
  CSMAP_IterStart
     (CSMAP* This)
{
   CSLIST_Clear(This->keys);

   CSMAP_PRIVATE_Traverse(This->tree, This->keys);

   This->NextKey = 0;
   return CS_SUCCESS;
}

CSRESULT
  CSMAP_IterNext
     (CSMAP* This,
       char** key,
       void** value,
       long*  valueSize)
{
   long count;
   pAVLTREE pNode;
   count = CSLIST_Count((void*)This->keys);

   if (This->NextKey < count)
   {
      CSLIST_Get(This->keys, (void*)&pNode, This->NextKey);

      *key = (char*)(pNode->key);
      *value = (void*)(pNode->value);
      *valueSize = (long)(pNode->valueSize);

      This->NextKey++;
   }
   else
   {
      *key = 0;
      *value = 0;
      return CS_FAILURE;
   }

   return CS_SUCCESS;
}

CSLIST*
  CSMAP_FRD_GetIter
     (CSMAP* This)
{
   CSLIST_Clear(This->keys);

   CSMAP_PRIVATE_Traverse(This->tree, This->keys);

   return This->keys;
}

void
  CSMAP_PRIVATE_Clear
     (pAVLTREE Tree) {

   if (Tree == 0)
      return;

   if (Tree->left != 0)
   {
      CSMAP_PRIVATE_Clear(Tree->left);
   }

   if (Tree->right != 0)
   {
      CSMAP_PRIVATE_Clear(Tree->right);
   }

   if (Tree->valueSize > 0)
   {
      free(Tree->value);
   }

   // Since we can refernce keys outside the MAP,
   // we must not frre outer data; an outside key has
   // a size of zero

   if (Tree->keySize > 0) {
     free(Tree->key);
   }

   free(Tree);

   return;
}

CSRESULT
  CSMAP_PRIVATE_Lookup
     (pAVLTREE This,
       char*    key,
       void**   value,
       long*    valueSize) {

   long compare;

   if (This == 0)
   {
      *valueSize = 0;
      *value = 0;
      return CS_FAILURE;
   }
   else
   {
      compare = strcmp(This->key, key);

      if (compare == 0)  // Keys match
      {
         *valueSize = This->valueSize;
         *value = (void*)This->value;
         return CS_SUCCESS;
      }
      else
      {
         if (compare < 0)  // specified key is greater than current node key
         {
            return CSMAP_PRIVATE_Lookup(This->right, key, value, valueSize);
         }
         else
         {
            return CSMAP_PRIVATE_Lookup(This->left, key, value, valueSize);
         }
      }
   }
}

pAVLTREE
  CSMAP_PRIVATE_Insert
     (pAVLTREE This,
       pAVLTREE parent,
       char*    key,
       void*    value,
       long     valueSize) {

   CSMAPNODE* root;

   long compare;
   long iKeySize;

   if (This == 0)
   {
      if (key == 0)
      {
         return 0;
      }

      /* ---------------------------------------------------------------
       This is the insertion point
      --------------------------------------------------------------- */

      root = (pAVLTREE)malloc(sizeof(CSMAPNODE));

      /* ---------------------------------------------------------------
       * Set the node's key and value
      --------------------------------------------------------------- */

    iKeySize = strlen(key) + 1;
      root->key = (char*)malloc(sizeof(char) * iKeySize);
      strcpy(root->key, key);

      root->keySize = iKeySize;
      root->valueSize = valueSize;

      if (root->valueSize > 0)
      {
         root->value = (char*)malloc(root->valueSize);
         memcpy(root->value, value, root->valueSize);
      }
      else
      {
         root->value = 0;  /* NULL value for this node */
      }

      /* ---------------------------------------------------------------
       * Set the node's relations
      --------------------------------------------------------------- */

      root->height = 1;
      root->parent = parent;
      root->left   = 0;
      root->right  = 0;
   }
   else
   {
      compare = strcmp(This->key, key);
      if (compare == 0)  // Keys match
      {

         /* ---------------------------------------------------------------
          * The tree already has this key ... we will update it's value
         --------------------------------------------------------------- */

         free(This->value);

         This->valueSize = valueSize;

         if (This->valueSize > 0)
         {
            This->value = (char*)malloc(This->valueSize);
            memcpy(This->value, value, This->valueSize);
         }
         else
         {
            This->value = 0;  /* NULL value for this node */
         }

         root = This;
      }
      else
      {
         if (compare < 0)  // specified key is higher than current node key
         {
            /* ------------------------------------------------------------
             * The node will have to be inserted in the
             *  right tree from this node
            ------------------------------------------------------------ */

            This->right  = CSMAP_PRIVATE_Insert(This->right,
                                           This, key, value, valueSize);
         }
         else {  // specified key is lower than current node key

            /* ------------------------------------------------------------
             * The node will have to be inserted in
             * the left tree from this node.
            ------------------------------------------------------------ */

            This->left  = CSMAP_PRIVATE_Insert(This->left,
                                         This, key, value, valueSize);
         }

         // Update root node height and rebalance

         CSMAP_PRIVATE_ResetTreeHeight(This);
         This = CSMAP_PRIVATE_Rebalance(This);

         root = This;
      }
   }

   return root;
}

pAVLTREE
  CSMAP_PRIVATE_InsertKeyRef
     (pAVLTREE This,
       pAVLTREE parent,
       char*    key,
       void*    value,
       long     valueSize) {

   CSMAPNODE* root;

   long compare;

   if (This == 0)
   {
      if (key == 0)
      {
         return 0;
      }

      /* ---------------------------------------------------------------
       This is the insertion point
      --------------------------------------------------------------- */

      root = (pAVLTREE)malloc(sizeof(CSMAPNODE));

      /* ---------------------------------------------------------------
       * Set the node's key and value
      --------------------------------------------------------------- */

      root->key = key;

      root->keySize = 0;
      root->valueSize = valueSize;

      if (root->valueSize > 0)
      {
         root->value = (char*)malloc(root->valueSize);
         memcpy(root->value, value, root->valueSize);
      }
      else
      {
         root->value = 0;  /* NULL value for this node */
      }

      /* ---------------------------------------------------------------
       * Set the node's relations
      --------------------------------------------------------------- */

      root->height = 1;
      root->parent = parent;
      root->left   = 0;
      root->right  = 0;
   }
   else
   {
      compare = strcmp(This->key, key);
      if (compare == 0)  // Keys match
      {

         /* ---------------------------------------------------------------
          * The tree already has this key ... we will update it's value
         --------------------------------------------------------------- */

         free(This->value);

         This->valueSize = valueSize;

         if (This->valueSize > 0)
         {
            This->value = (char*)malloc(This->valueSize);
            memcpy(This->value, value, This->valueSize);
         }
         else
         {
            This->value = 0;  /* NULL value for this node */
         }

         root = This;
      }
      else
      {
         if (compare < 0)  // specified key is higher than current node key
         {
            /* -------------------------------------------------------------
             * The node will have to be inserted in
             * the right tree from this node
            ------------------------------------------------------------- */

            This->right  = CSMAP_PRIVATE_InsertKeyRef(This->right,
                                                This, key, value, valueSize);
         }
         else {  // specified key is lower than current node key

            /* -------------------------------------------------------------
             * The node will have to be inserted
             * in the left tree from this node.
            ------------------------------------------------------------- */

            This->left  = CSMAP_PRIVATE_InsertKeyRef(This->left,
                                         This, key, value, valueSize);
         }

         // Update root node height and rebalance

         CSMAP_PRIVATE_ResetTreeHeight(This);
         This = CSMAP_PRIVATE_Rebalance(This);

         root = This;
      }
   }

   return root;
}

pAVLTREE
  CSMAP_PRIVATE_Remove
     (pAVLTREE This,
       char*    key) {

   long compare;

   pAVLTREE pSuccessor;
   pAVLTREE pNewRoot;

   if (This == 0)
   {
      return 0;
   }
   else
   {
      compare = strcmp(This->key, key);

      if (compare == 0)  // Keys match
      {
         /* -----------------------------------------------------------
          * We remove the node like any BST.
          * Three case:
          *
          * 1) Node has no children (it is a leaf)
          * 2) Node has a single child
          * 3) Node has two children
          *
          * Case 1)
          *             Just delete the node
          *
          * -------------------------------------------------------- */

         if (This->left == 0 && This->right == 0)
         {
            // Case 1): We are removing a leaf

            if (This->valueSize > 0)
            {
               free(This->value);
            }

            free(This->key);
            free(This);
            return 0;
         }
         else
         {
            if (This->left != 0 && This->right != 0)
            {
               /* -----------------------------------------------------------------
                case 3): Node has a left and right tree. In that case,
                we must find the successor node (the next node in keyed sequence).

                A few facts about the successor:

                  The successor's left tree is always a null tree
                  The successor has a right tree (could be a null tree or not)

                  The successor's parent must adopt the successor's right tree as
                  its left tree The successor's parent's height could change as a
                  result so we may have to re-balance parent

                  The successor node could be the root's immediate right tree;
                   we must consider this case
               ----------------------------------------------------------------- */

               pSuccessor = CSMAP_PRIVATE_Successor(This);

               // We will replace the root node's key and
               // value with those of the successor node

               if (This->valueSize > 0)
               {
                  free(This->value);
               }

               free(This->key);

               This->key       = pSuccessor->key;
               This->value     = pSuccessor->value;
               This->keySize   = pSuccessor->keySize;
               This->valueSize = pSuccessor->valueSize;

               if (pSuccessor == This->right)
               {
                  // This is the case of the successor being the root's right tree
                  // The root must now adopt the successor's right tree

                  This->right = pSuccessor->right;

                  if (pSuccessor->right != 0)
                  {
                     pSuccessor->right->parent = This;
                  }

                  // No need to re-balance successor's right tree because by
                  // definition, the successor's right tree is an AVL tree
               }
               else
               {
                  // The successor's parent node must adopt the
                  // successor's right tree as it's left tree

                  pSuccessor->parent->left = pSuccessor->right;

                  if (pSuccessor->right != 0)
                  {
                     pSuccessor->right->parent = pSuccessor->parent;
                  }

                  // successor parent height could change ... recompute

                  CSMAP_PRIVATE_ResetTreeHeight(pSuccessor->parent);

                  // Re-balance successor's parent if necessary

                  pSuccessor->parent =
                               CSMAP_PRIVATE_Rebalance(pSuccessor->parent);
               }

               // We are now done with the successor node itself ... we delete it

               free(pSuccessor);

               // The root node may now have a new height ... recompute

               CSMAP_PRIVATE_ResetTreeHeight(This);
               This = CSMAP_PRIVATE_Rebalance(This);

               return This;
            }
            else
            {
               // case 2): only one child
               // connect child to parent parent

               if (This->left != 0)
               {
                  This->left->parent = This->parent;
                  pNewRoot = This->left;
               }
               else
               {
                  This->right->parent = This->parent;
                  pNewRoot = This->right;
               }

               /* let's delete the former root */

               if (This->valueSize > 0)
               {
                  free(This->value);
               }

               free(This->key);
               free(This);

               // return new root to parent; no need to re-balance

               return pNewRoot;
            }
         }
      }
      else
      {
         if (compare < 0)  // specified key is greater than current node key
         {
            This->right = CSMAP_PRIVATE_Remove(This->right, key);
         }
         else
         {
            This->left = CSMAP_PRIVATE_Remove(This->left, key);
         }

         /* Compute new root's height */

         CSMAP_PRIVATE_ResetTreeHeight(This);
         This = CSMAP_PRIVATE_Rebalance(This);

         return This;
      }
   }

   // if we get here, then something is VERY wrong
   // since we considered all cases!
   return 0;
}

void
  CSMAP_PRIVATE_ResetTreeHeight
     (pAVLTREE Tree)
{
   long LeftHeight;
   long RightHeight;

   LeftHeight = CSMAP_PRIVATE_Height(Tree->left);
   RightHeight = CSMAP_PRIVATE_Height(Tree->right);

   if (LeftHeight > RightHeight)
   {
      Tree->height = LeftHeight + 1;
   }
   else
   {
      Tree->height = RightHeight + 1;
   }
}

pAVLTREE
  CSMAP_PRIVATE_Rebalance
     (pAVLTREE root)
{
   long BalanceFactor;
   long LeftHeight;
   long RightHeight;

   LeftHeight = CSMAP_PRIVATE_Height(root->left);
   RightHeight = CSMAP_PRIVATE_Height(root->right);

   BalanceFactor = LeftHeight - RightHeight;

   if (BalanceFactor > 1)  // left heavy
   {
      LeftHeight  = CSMAP_PRIVATE_Height(root->left->left);
      RightHeight = CSMAP_PRIVATE_Height(root->left->right);

      if ( (RightHeight - LeftHeight) > 0 )
      {
         // left subtree is right-heavy
         root = CSMAP_PRIVATE_DoubleRightRotation(root);
      }
      else
      {
         root = CSMAP_PRIVATE_SingleRightRotation(root);
      }
   }
   else
   {
      if (BalanceFactor < -1)  // right heavy
      {
         LeftHeight  = CSMAP_PRIVATE_Height(root->right->left);
         RightHeight = CSMAP_PRIVATE_Height(root->right->right);

         if ( (LeftHeight - RightHeight) > 0 )
         {
            // left subtree is right-heavy
            root = CSMAP_PRIVATE_DoubleLeftRotation(root);
         }
         else
         {
            root = CSMAP_PRIVATE_SingleLeftRotation(root);
         }
      }
   }

   return root;
}

long
  CSMAP_PRIVATE_Height
     (pAVLTREE Tree) {

   if (Tree == 0) {
      return 0;
   }

   return Tree->height;
}

long
  CSMAP_PRIVATE_ComputeRightHeight
     (pAVLTREE Tree) {

   if (Tree == 0)
   {
      return 0;
   }
   else
   {
      if (Tree->right == 0) {
         return 0;
      }
      else {
         return CSMAP_PRIVATE_ComputeRightHeight(Tree->right) + 1;
      }
   }
}

long
  CSMAP_PRIVATE_ComputeLeftHeight
     (pAVLTREE Tree) {


   if (Tree == 0)
   {
      return 0;
   }
   else
   {
      if (Tree->left)
      {
         return CSMAP_PRIVATE_ComputeLeftHeight(Tree->left) + 1;
      }
      else
      {
         return 0;
      }
   }
}

pAVLTREE
  CSMAP_PRIVATE_SingleLeftRotation
     (pAVLTREE This) {

   /* ------------------------------------------------------------------------
    * 1) In this case, the root will become the left child of it's
    *    right child.
    *
    * 2) The root's right child will become the root
    *
    * 3) The left child of the root's right child will become
    *    the right child of the root
    * --------------------------------------------------------------------- */

   pAVLTREE Temp;

   long LeftHeight;
   long RightHeight;

   /* ------------------------------------------------------------------------
    * We now perform the rotation
    * --------------------------------------------------------------------- */

   Temp                = This->right;
   This->right         = This->right->left;
   Temp->left          = This;
   Temp->parent        = This->parent;
   This->parent        = Temp;

   if (This->right)
   {
      This->right->parent = This;
   }

   /* ------------------------------------------------------------------------
    * The rotation might affect the node's height
    *
    * Compute height of former root first.
    * --------------------------------------------------------------------- */

   LeftHeight  = CSMAP_PRIVATE_Height(This->left);
   RightHeight = CSMAP_PRIVATE_Height(This->right);

   if (LeftHeight > RightHeight)
   {
      This->height = LeftHeight + 1;
   }
   else
   {
      This->height = RightHeight + 1;
   }

   /* ------------------------------------------------------------------------
    * Compute height of new root.
    * --------------------------------------------------------------------- */

   LeftHeight  = This->height;
   RightHeight = CSMAP_PRIVATE_Height(Temp->right);

   if (LeftHeight > RightHeight)
   {
      Temp->height = LeftHeight + 1;
   }
   else
   {
      Temp->height = RightHeight + 1;
   }

   /* ------------------------------------------------------------------------
    * The new root is the former root's right child
    * --------------------------------------------------------------------- */

   return Temp;
}

pAVLTREE
  CSMAP_PRIVATE_SingleRightRotation
     (pAVLTREE This) {

   /* ------------------------------------------------------------------------
    * 1) In this case, the root will become the right child of it's
    *    left child.
    *
    * 2) The root's left child will become the root
    *
    * 3) The right child of the root's left child will become
    *    the left child of the root
    * --------------------------------------------------------------------- */

   pAVLTREE Temp;

   long LeftHeight;
   long RightHeight;

   /* ------------------------------------------------------------------------
    * We now perform the rotation
    * --------------------------------------------------------------------- */

   Temp               = This->left;
   This->left         = This->left->right;
   Temp->right        = This;
   Temp->parent       = This->parent;
   This->parent       = Temp;

   if (This->left) {
      This->left->parent = This;
   }

   /* ------------------------------------------------------------------------
    * The rotation might affect the node's height
    *
    * Compute height of former root first.
    * --------------------------------------------------------------------- */

   LeftHeight  = CSMAP_PRIVATE_Height(This->left);
   RightHeight = CSMAP_PRIVATE_Height(This->right);

   if (LeftHeight > RightHeight)
   {
      This->height = LeftHeight + 1;
   }
   else
   {
      This->height = RightHeight + 1;
   }

   /* ------------------------------------------------------------------------
    * We now compute the new root's height
    * --------------------------------------------------------------------- */

   LeftHeight = CSMAP_PRIVATE_Height(Temp->left);
   RightHeight  = This->height;

   if (LeftHeight > RightHeight)
   {
      Temp->height = LeftHeight + 1;
   }
   else
   {
      Temp->height = RightHeight + 1;
   }

   /* ------------------------------------------------------------------------
    * The new root is the former root's right child
    * --------------------------------------------------------------------- */

   return Temp;
}

pAVLTREE
  CSMAP_PRIVATE_DoubleLeftRotation
     (pAVLTREE This) {

  This->right = CSMAP_PRIVATE_SingleRightRotation(This->right);
  return CSMAP_PRIVATE_SingleLeftRotation(This);

}

pAVLTREE
  CSMAP_PRIVATE_DoubleRightRotation
     (pAVLTREE This) {

  This->left = CSMAP_PRIVATE_SingleLeftRotation(This->left);
  return CSMAP_PRIVATE_SingleRightRotation(This);

}

pAVLTREE
  CSMAP_PRIVATE_Successor
     (pAVLTREE Tree) {

   pAVLTREE cur;

   if (Tree->right == 0)
   {
      return 0;
   }

   cur = Tree->right;

   while (cur->left != 0)
   {
      cur = cur->left;
   }

   return cur;
}

pAVLTREE
  CSMAP_PRIVATE_Traverse
     (pAVLTREE Tree,
       CSLIST*   keys) {

      pAVLTREE pNode;

      if (Tree == 0)
         return 0;

      if (Tree->left != 0)
      {
         CSMAP_PRIVATE_Traverse(Tree->left, keys);
      }

      //insert the node in the list rather than copy the key
      pNode = Tree;

      CSLIST_Insert(keys, &pNode, sizeof(pAVLTREE), CSLIST_BOTTOM);

      if (Tree->right != 0)
      {
         CSMAP_PRIVATE_Traverse(Tree->right, keys);
      }

      return 0;
}


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
       CSLIST_Get(This->Tokens, outBuffer+offset, i);
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

         return CS_FAILURE | CS_OPER_CSSTRCV | EINVAL;
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

            return CS_FAILURE | CS_OPER_CSSTRCV | EILSEQ;
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

      if (CS_DIAG(hResult) == EINVAL) {

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

      if (CS_DIAG(hResult) == EINVAL) {

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
           rc = CS_FAILURE | CS_OPER_ICONV | EILSEQ;
           break;
        case E2BIG:
           rc = CS_FAILURE | CS_OPER_ICONV | E2BIG;
           break;
        case EINVAL:
           rc = CS_FAILURE | CS_OPER_ICONV | EINVAL;
           break;
        case EBADF:
           rc = CS_FAILURE | CS_OPER_ICONV | EBADF;
           break;
        case EBADDATA:
           rc = CS_FAILURE | CS_OPER_ICONV | EBADDATA;
           break;
        case ECONVERT:
           rc = CS_FAILURE | CS_OPER_ICONV | ECONVERT;
           break;
        case EFAULT:
           rc = CS_FAILURE | CS_OPER_ICONV | EFAULT;
           break;
        case ENOBUFS:
           rc = CS_FAILURE | CS_OPER_ICONV | ENOBUFS;
           break;
        case ENOMEM:
           rc = CS_FAILURE | CS_OPER_ICONV | ENOMEM;
           break;
        case EUNKNOWN:
           rc = CS_FAILURE | CS_OPER_ICONV | EUNKNOWN;
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
// delimiters more than one charater in length.
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
// Convers a string from uppercase to lowercase.
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
// Convers a string from lowercase to uppercase.
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
// URL-decodes an URL-encoded string.
//
//////////////////////////////////////////////////////////////////////////////

long
  CSSTR_UrlDecode
    (unsigned char* in,
     unsigned char* out,
     long InSize,
     int flags) {

  long i;
  long j;
  long Size;
  long OutSize;

  char code[3];
  unsigned int n;

  CSSTRCV* cvt;

  cvt = CSSTRCV_Constructor();

  if (flags & CSSTR_INPUT_EBCDIC) {

    // Input is EBCDIC; convert to ASCII

    CSSTRCV_SetConversion(cvt, "00000", "00819");
    CSSTRCV_StrCpy(cvt, in, InSize);
    Size = CSSTRCV_Size(cvt);
    CSSTRCV_Get(cvt, in);
    in[Size] = 0;
  }

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

  if (flags & CSSTR_OUTPUT_EBCDIC) {
    // We want EBCDIC output; convert to to EBCDIC

    CSSTRCV_SetConversion(cvt, "00819", "00000");
    CSSTRCV_StrCpy(cvt, out, j);
    j = CSSTRCV_Size(cvt);
    CSSTRCV_Get(cvt, out);
    out[j] = 0;
  }

  CSSTRCV_Destructor(&cvt);

  return j;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSTR_UrlEncode
//
// URL-encodes a string. The caller must insure that
// the resulting buffer is large enough (up to 3 times the size).
//
//////////////////////////////////////////////////////////////////////////////

long
  CSSTR_UrlEncode
    (unsigned char* in,
     unsigned char* out,
     long InSize,
     int flags) {

  long i;
  long j;
  long Size;
  long OutSize;

  char hex[16] = { 48, 49, 50, 51, 52, 53, 54, 55 , 56, 57,
                   65, 66, 67, 68, 69, 70 };

  CSSTRCV* cvt;

  cvt = CSSTRCV_Constructor();

  if (flags & CSSTR_INPUT_EBCDIC) {

    // Input is EBCDIC; convert to ASCII

    CSSTRCV_SetConversion(cvt, "00000", "00819");
    CSSTRCV_StrCpy(cvt, in, InSize);
    Size = CSSTRCV_Size(cvt);
    CSSTRCV_Get(cvt, in);
    in[Size] = 0;
  }

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

  if (flags & CSSTR_OUTPUT_EBCDIC) {
    // We want EBCDIC output; convert to to EBCDIC

    CSSTRCV_SetConversion(cvt, "00819", "00000");
    CSSTRCV_StrCpy(cvt, out, j);
    j = CSSTRCV_Size(cvt);
    CSSTRCV_Get(cvt, out);
    out[j] = 0;
  }

  CSSTRCV_Destructor(&cvt);

  return j;
}


CSRESULT
  CSSYS_GetCurJobInfo
    (char szJobName[11],
     char szJobNumber[7],
     char szJobUser[10]) {

  JOBINFOSTRUCT jobInfo;

  QUSRJOBI(&jobInfo,
           sizeof(JOBINFOSTRUCT),
           "JOBI0100",
           "*                         ",
           "                ");

  memcpy(szJobName, jobInfo.JobName,   10);
  szJobName[10] = 0;
  memcpy(szJobNumber, jobInfo.JobNumber,   6);
  szJobNumber[6] = 0;
  memcpy(szJobUser, jobInfo.JobUser, 10);
  szJobUser[10] = 0;

  return CS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSSYS_MakeUUID
//
// This function generates a UUID and returns its string representation.
//
//////////////////////////////////////////////////////////////////////////////

CSRESULT
  CSSYS_MakeUUID
    (char* szUUID,
     int mode) {

  int size;
  int i;

  char szBuffer[CSSYS_UUID_BUFFERSIZE];

  _UUID_Template_T Template;


  memset(&Template, 0, sizeof(_UUID_Template_T));
  Template.bytesProv = sizeof(_UUID_Template_T);
  _GENUUID(&Template);

  size = 32;
  cvthc(szBuffer, Template.uuid, size);

  if (mode & CSSYS_UUID_LOWERCASE) {

    for(i=0; i<32; i++)
       szBuffer[i] = (char)tolower(szBuffer[i]);
  }

  if (mode & CSSYS_UUID_DASHES) {

    // Format with dashes

     szUUID[8]  = '-';
     szUUID[13] = '-';
     szUUID[18] = '-';
     szUUID[23] = '-';
     memcpy(szUUID + 0,  szBuffer + 0,  8);
     memcpy(szUUID + 9,  szBuffer + 8,  4);
     memcpy(szUUID + 14, szBuffer + 12, 4);
     memcpy(szUUID + 19, szBuffer + 16, 4);
     memcpy(szUUID + 24, szBuffer + 20, 12);

     szUUID[36] = 0;
  }
  else {

     memcpy(szUUID, szBuffer, 32);
     szUUID[32] = 0;
  }

  return CS_SUCCESS;
}


 
