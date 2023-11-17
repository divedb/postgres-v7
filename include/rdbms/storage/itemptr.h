// =========================================================================
//
// itemptr.h
//  POSTGRES disk item pointer definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: itemptr.h,v 1.14 2000/01/26 05:58:33 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_ITEM_PTR_H_
#define RDBMS_STORAGE_ITEM_PTR_H_

#include "rdbms/storage/block.h"
#include "rdbms/storage/off.h"

// This is a pointer to an item on another disk page in the same file.
// blkid tells us which block, posid tells us which entry in the linp
// (ItemIdData) array we want.
typedef struct ItemPointerData {
  BlockIdData ip_blk_id;
  OffsetNumber ip_pos_id;
} ItemPointerData;

typedef ItemPointerData* ItemPointer;

#define ITEM_POINTER_IS_VALID(pointer) (POINTER_IS_VALID(pointer) && ((pointer)->ip_pos_id != 0))
#define ITEM_POINTER_GET_BLOCK_NUMBER(pointer) \
  (assert(ITEM_POINTER_IS_VALID(pointer)), BLOCK_ID_GET_BLOCK_NUMBER(&(pointer)->ip_blk_id))
#define ITEM_POINTER_GET_OFFSET_NUMBER(pointer) (assert(ITEM_POINTER_IS_VALID(pointer)), (pointer)->ip_pos_id)
#define ITEM_POINTER_SET(pointer, block_num, off_num) \
  (assert(POINTER_IS_VALID(pointer)), BLOCK_ID_SET(&((pointer)->ip_blk_id), block_num), (pointer)->ip_pos_id = off_num)
#define ITEM_POINTER_SET_BLOCK_NUMBER(pointer, block_num) \
  (assert(POINTER_IS_VALID(pointer)), BLOCK_ID_SET(&((pointer)->ip_blk_id), block_num))
#define ITEM_POINTER_SET_OFFSET_NUMBER(pointer, offset_num) \
  (assert(POINTER_IS_VALID(pointer)), (pointer)->ip_pos_id = (offset_num))
#define ITEM_POINTER_COPY(from_pointer, to_pointer) \
  (assert(POINTER_IS_VALID(from_pointer)), assert(POINTER_IS_VALID(to_pointer)), *(to_pointer) = *(from_pointer))
#define ITEM_POINTER_SET_INVALID(pointer)                                                           \
  (assert(POINTER_IS_VALID(pointer)), BLOCK_ID_SET(&((pointer)->ip_blk_id), INVALID_OFFSET_NUMBER), \
   (pointer)->ip_pos_id = INVALID_OFFSET_NUMBER)

bool item_pointer_equals(ItemPointer pointer1, ItemPointer pointer2);

#endif  // RDBMS_STORAGE_ITEM_PTR_H_
