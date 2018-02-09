/* ===========================================================================
  Clarasoft core tools

  CSLIST.h

  Linked list
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

#include <stdlib.h>
#include <string.h>

#include "qcsrc/cscore.h"

#define CSLIST_TOP      (0x00000000)   // at the beginning of the list
#define CSLIST_BOTTOM   (0xFFFFFFFF)   // at the end of the list

typedef struct tagCSLISTNODE {

  struct tagCSLISTNODE * previous;
  struct tagCSLISTNODE * next;

  void* data;
  long dataSize;

}CSLISTNODE;

typedef CSLISTNODE* PCSLISTNODE;

typedef struct tagCSLIST {

  PCSLISTNODE first;
  PCSLISTNODE last;
  PCSLISTNODE current;

  long numItems;
  long curIndex;

}CSLIST;

CSLIST*
  CSLIST_Constructor
    (void);

void
  CSLIST_Destructor
    (CSLIST**);

CSRESULT
  CSLIST_Insert
    (CSLIST* This,
     void*   value,
     long    valueSize,
     long    index);

CSRESULT
  CSLIST_Remove
    (CSLIST* This,
     long    index);

CSRESULT
  CSLIST_Get
    (CSLIST* This,
     void*   value,
     long*   valueSize,
     long    index);

CSRESULT
  CSLIST_Set
    (CSLIST* This,
     void*   value,
     long    valueSize,
     long    index);

long
  CSLIST_Count
    (CSLIST* This);

void
  CSLIST_Clear
    (CSLIST* This);

long
  CSLIST_ItemSize
    (CSLIST* This,
     long    index);

CSRESULT
  CSLIST_GetDataRef
    (CSLIST* This,
     void**  value,
     long*   valueSize,
     long    index);

CSRESULT
  CSLIST_PRIVATE_Goto
    (CSLIST* This,
     long    index);

/* --------------------------------------------------------------------------
   CSLIST_Constructor
   Creates an instance of type CSLIST.
-------------------------------------------------------------------------- */

CSLIST* CSLIST_Constructor(void) {

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

void CSLIST_Destructor(CSLIST** This) {

  CSLIST_Clear(*This);
  free(*This);
  *This = 0;
}

/* --------------------------------------------------------------------------
   CSLIST_Clear
   Removes all items from a CSLIST.
-------------------------------------------------------------------------- */

void CSLIST_Clear(CSLIST* This)
{
  long i;
  long numItems;

  if (This == 0) {
     return;
  }

  // Don't use This->numItems because it will change as we
  // remove nodes.

  numItems = This->numItems;

  for (i=0; i<numItems; i++)
  {
    CSLIST_Remove((This), 0);
  }
}

/* --------------------------------------------------------------------------
   CSLIST_Insert
   Inserts a new item in a CSLIST at a specified index.
-------------------------------------------------------------------------- */

CSRESULT CSLIST_Insert(CSLIST* This, void* value,
                       long valueSize, long index) {

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
    // equal to at leat the number
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

CSRESULT CSLIST_Remove(CSLIST* This, long index) {

  PCSLISTNODE Temp;

  if (This == 0) {
     return CS_FAILURE;
  }

  if (This->numItems > 0)
  {
    if (This->numItems == 0)
    {
      Temp = This->first;

      This->first    = 0;
      This->last     = 0;
      This->current  = 0;
      This->curIndex = 0;
    }
    else
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
    }

    if (Temp->dataSize > 0)
    {
      free(Temp->data);
    }

    free(Temp);

    This->numItems--;
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

long CSLIST_Count(CSLIST* This)
{
    return This->numItems;
}

/* --------------------------------------------------------------------------
   CSLIST_Get
   Retrieves a copy a an item from the CSLIST.
-------------------------------------------------------------------------- */

CSRESULT CSLIST_Get(CSLIST* This, void* value,
                     long *valueSize, long index)
{
  if (This == 0)
  {
    return CS_FAILURE;
  }

  if (This->numItems > 0)
  {
    CSLIST_PRIVATE_Goto(This, index);

    if (This->current->dataSize > 0)
    {
      if (*valueSize < This->current->dataSize)
      {
        memcpy(value, This->current->data, *valueSize);
      }
      else
      {
        memcpy(value, This->current->data,
                    This->current->dataSize);
      }
    }

    *valueSize = This->current->dataSize;
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

CSRESULT CSLIST_GetDataRef(CSLIST* This, void** value,
                           long *valueSize, long index)
{
  CSRESULT hResult;

  hResult = CS_FAILURE;

  if (This == 0)
  {
     return hResult;
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

    *valueSize = This->current->dataSize;
    hResult = CS_SUCCESS;
  }
  else
  {
    *value = 0;
    *valueSize = 0;
  }

  return hResult;
}

/* --------------------------------------------------------------------------
   CSLIST_Set
   Replaces an existing item at a specified index location.
-------------------------------------------------------------------------- */

CSRESULT CSLIST_Set(CSLIST* This, void* value,
                     long valueSize, long index)
{
  if (This == 0)
  {
    return CS_FAILURE;
  }

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

long CSLIST_ItemSize(CSLIST* This, long index) {

  if (This == 0)
  {
    return -1;
  }

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

CSRESULT CSLIST_PRIVATE_Goto(CSLIST* This, long index)
{
  // This method is to speed up positioning on average

  if (This->numItems <= 0)
  {
    return CS_FAILURE;
  }

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
