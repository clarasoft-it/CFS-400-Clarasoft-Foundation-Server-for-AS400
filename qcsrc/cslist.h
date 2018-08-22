/* ==========================================================================
  Clarasoft Core Tools
  
  cslist.h
  Linked list definitions
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

#ifndef __CLARASOFT_CT_CSLIST_H__
#define __CLARASOFT_CT_CSLIST_H__

#include "qcsrc/cscore.h"

/* --------------------------------------------------------------------------
  Linked List
-------------------------------------------------------------------------- */

#define CSLIST_TOP      (0x00000000)   // at the beginning of the list
#define CSLIST_BOTTOM   (0xFFFFFFFF)   // at the end of the list

typedef void* CSLIST;

CSLIST
  CSLIST_Constructor
    (void);

void
  CSLIST_Destructor
    (CSLIST*);

CSRESULT
  CSLIST_Insert
    (CSLIST This,
     void*   value,
     long    valueSize,
     long    index);

CSRESULT
  CSLIST_Remove
    (CSLIST This,
     long    index);

CSRESULT
  CSLIST_Get
    (CSLIST This,
     void*   value,
     long*   valueSize,
     long    index);

CSRESULT
  CSLIST_Set
    (CSLIST This,
     void*   value,
     long    valueSize,
     long    index);

long
  CSLIST_Count
    (CSLIST This);

void
  CSLIST_Clear
    (CSLIST This);

long
  CSLIST_ItemSize
    (CSLIST This,
     long    index);

CSRESULT
  CSLIST_GetDataRef
    (CSLIST This,
     void**  value,
     long*   valueSize,
     long    index);

#endif

