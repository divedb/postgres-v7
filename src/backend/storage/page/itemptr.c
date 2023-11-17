// =========================================================================
//
// itemptr.c
//  POSTGRES disk item pointer code.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/page/itemptr.c,v 1.8 2000/01/26 05:57:04 momjian Exp $
//
// =========================================================================

#include "rdbms/storage/itemptr.h"

bool item_pointer_equals(ItemPointer pointer1, ItemPointer pointer2) {
  return ITEM_POINTER_GET_BLOCK_NUMBER(pointer1) == ITEM_POINTER_GET_BLOCK_NUMBER(pointer2) &&
         ITEM_POINTER_GET_OFFSET_NUMBER(pointer1) == ITEM_POINTER_GET_OFFSET_NUMBER(pointer2);
}