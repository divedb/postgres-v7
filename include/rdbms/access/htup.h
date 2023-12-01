// =========================================================================
//
// htup.h
//  POSTGRES heap tuple definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: htup.h,v 1.29 2000/04/12 17:16:26 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_ACCESS_HTUP_H_
#define RDBMS_ACCESS_HTUP_H_

#include "rdbms/postgres.h"
#include "rdbms/storage/itemptr.h"
#include "rdbms/utils/mctx.h"

#define MIN_HEAL_TUPLE_BIT_MAP_SIZE 32  // 8 * 4

// Check these, they are likely to be more severely limited by t_hoff.
#define MAX_HEAP_ATTRIBUTE_NUMBER 1600  // 8 * 200

// To avoid wasting space, the attributes should be layed out in such a
// way to reduce structure padding.
typedef struct HeapTupleHeaderData {
  Oid t_oid;               // OID of this tuple -- 4 bytes.
  CommandId t_cmin;        // Insert CID stamp -- 4 bytes each.
  CommandId t_cmax;        // Delete CommandId stamp.
  TransactionId t_xmin;    // Insert XID stamp -- 4 bytes.
  TransactionId t_xmax;    // Delete XID stamp.
  ItemPointerData t_ctid;  // Current TID of this or newer tuple.
  int16 t_natts;           // Number of attributes.
  uint16 t_info_mask;      // Various infos.
  uint8 t_hoff;            // Size of tuple header.
  bits8 t_bits[MIN_HEAL_TUPLE_BIT_MAP_SIZE / 8];

  // MORE DATA FOLLOWS AT END OF STRUCT.
} HeapTupleHeaderData;

typedef HeapTupleHeaderData* HeapTupleHeader;

// TODO(gc): why add extra sizeof(char).
#define MIN_TUPLE_SIZE \
  (MAXALIGN(sizeof(PageHeaderData)) + MAXALIGN(sizeof(HeapTupleHeaderData)) + MAXALIGN(sizeof(char)))

#define MAX_TUPLE_SIZE (BLCKSZ - MIN_TUPLE_SIZE)
#define MAX_ATTR_SIZE  (MAX_TUPLE_SIZE - MAXALIGN(sizeof(HeapTupleHeaderData)))

#define SELF_ITEM_POINTER_ATTRIBUTE_NUMBER      (-1)
#define OBJECT_ID_ATTRIBUTE_NUMBER              (-2)
#define MIN_TRANSACTION_ID_ATTRIBUTE_NUMBER     (-3)
#define MIN_COMMAND_ID_ATTRIBUTE_NUMBER         (-4)
#define MAX_TRANSACTION_ID_ATTRIBUTE_NUMBER     (-5)
#define MAX_COMMAND_ID_ATTRIBUTE_NUMBER         (-6)
#define FIRST_LOW_INVALID_HEAP_ATTRIBUTE_NUMBER (-7)

// If you make any changes above, the order off offsets in this must change.
extern long HeapSysOffset[];

// This new HeapTuple for version >= 6.5 and this is why it was changed:
//
// 1. t_len moved off on-disk tuple data - ItemIdData is used to get len;
// 2. t_ctid above is not self tuple TID now - it may point to
//	  updated version of tuple (required by MVCC);
// 3. someday someone let tuple to cross block boundaries -
//	  he have to add something below...
//
// Change for 7.0:
//  Up to now t_data could be NULL, the memory location directly following
//  HeapTupleData or pointing into a buffer. Now, it could also point to
//  a separate allocation that was done in the t_datamcxt memory context.
typedef struct HeapTupleData {
  uint32 t_len;               // Length of t_data.
  ItemPointerData t_self;     // SelfItemPointer.
  Oid t_table_oid;            // Table the tuple came from.
  MemoryContext t_data_mcxt;  // Memory context of allocation.
  HeapTupleHeader t_data;     // Tuple header and data.
} HeapTupleData;

typedef HeapTupleData* HeapTuple;

#define HEAP_TUPLE_SIZE MAXALIGN(sizeof(HeapTupleData))

#define GET_STRUCT(tuple) (((char*)((HeapTuple)(tuple))->t_data) + ((HeapTuple)(tuple))->t_data->t_hoff)

// Computes minimum size of bitmap given number of domains.
#define BIT_MAP_LEN(natts) \
  ((((((int)(natts)-1) >> 3) + 4 - (MIN_HEAL_TUPLE_BIT_MAP_SIZE >> 3)) & ~03) + (MIN_HEAL_TUPLE_BIT_MAP_SIZE >> 3))

#define HEAP_TUPLE_IS_VALID(tuple) POINTER_IS_VALID(tuple)

// Information stored in t_infomask:
#define HEAP_HASNULL           0x0001  // Has null attribute(s).
#define HEAP_HASVARLENA        0x0002  // Has variable length  attribute(s).
#define HEAP_HASEXTERNAL       0x0004  // Has external stored attribute(s).
#define HEAP_HASCOMPRESSED     0x0008  // Has compressed stored attribute(s).
#define HEAP_HASEXTENDED       0x000C  // The two above combined.
#define HEAP_XMIN_COMMITTED    0x0100  // t_xmin committed.
#define HEAP_XMIN_INVALID      0x0200  // t_xmin invalid/aborted.
#define HEAP_XMAX_COMMITTED    0x0400  // t_xmax committed.
#define HEAP_XMAX_INVALID      0x0800  // t_xmax invalid/aborted.
#define HEAP_MARKED_FOR_UPDATE 0x1000  // Marked for UPDATE.
#define HEAP_UPDATED           0x2000  // This is UPDATEd version of row.
#define HEAP_MOVED_OFF         0x4000  // Removed or moved to another place by vacuum.
#define HEAP_MOVED_IN          0x8000  // moved from another place by vacuum.
#define HEAP_XACT_MASK         0xFF00

#define HEAP_TUPLE_NO_NULLS(tuple)       (!(((HeapTuple)(tuple))->t_data->t_infomask & HEAP_HASNULL))
#define HEAP_TUPLE_ALL_FIXED(tuple)      (!(((HeapTuple)(tuple))->t_data->t_infomask & HEAP_HASVARLENA))
#define HEAP_TUPLE_HAS_EXTERNAL(tuple)   ((((HeapTuple)(tuple))->t_data->t_infomask & HEAP_HASEXTERNAL) != 0)
#define HEAP_TUPLE_HAS_COMPRESSED(tuple) ((((HeapTuple)(tuple))->t_data->t_infomask & HEAP_HASCOMPRESSED) != 0)
#define HEAP_TUPLE_HAS_EXTENDED(tuple)   ((((HeapTuple)(tuple))->t_data->t_infomask & HEAP_HASEXTENDED) != 0)

#endif  // RDBMS_ACCESS_HTUP_H_
