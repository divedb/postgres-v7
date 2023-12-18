// -------------------------------------------------------------------------
//
// itemid.h
//  Standard POSTGRES buffer page item identifier definitions.
//
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: itemid.h,v 1.10 2000/01/26 05:58:33 momjian Exp $
//
// -------------------------------------------------------------------------

#ifndef RDBMS_STORAGE_ITEM_ID_H_
#define RDBMS_STORAGE_ITEM_ID_H_

typedef uint16 ItemOffset;
typedef uint16 ItemLength;
typedef bits16 ItemIdFlags;

typedef struct ItemIdData {
  unsigned lp_off : 15;
  unsigned lp_flags : 2;
  unsigned lp_len : 15;
} ItemIdData;

typedef struct ItemIdData* ItemId;

#ifndef LP_USED
#define LP_USED 0x01  // This line pointer is being used.
#endif

#define ITEM_ID_GET_LENGTH(itemid) ((itemid)->lp_len)
#define ITEM_ID_GET_OFFSET(itemid) ((itemid)->lp_off)
#define ITEM_ID_GET_FLAGS(itemid)  ((itemid)->lp_flags)
#define ITEM_ID_IS_VALID(itemid)   POINTER_IS_VALID(itemid)
#define ITEM_ID_IS_USED(itemid)    (assert(ITEM_ID_IS_VALID(itemid)), (bool)(((itemid)->lp_flags & LP_USED) != 0))

#endif  // RDBMS_STORAGE_ITEM_ID_H_
