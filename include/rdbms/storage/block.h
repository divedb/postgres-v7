// -------------------------------------------------------------------------
//
// block.h
//  POSTGRES disk block definitions.
//
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: block.h,v 1.11 2000/03/17 02:36:41 tgl Exp $
//
// -------------------------------------------------------------------------

#ifndef RDBMS_STORAGE_BLOCK_H_
#define RDBMS_STORAGE_BLOCK_H_

#include "rdbms/postgres.h"

// BlockNumber:
//
// Each data file (heap or index) is divided into postgres disk blocks
// (which may be thought of as the unit of i/o -- a postgres buffer
// contains exactly one disk block). The blocks are numbered
// sequentially, 0 to 0xFFFFFFFE.
//
// InvalidBlockNumber is the same thing as P_NEW in buf.h.
//
// the access methods, the buffer manager and the storage manager are
// more or less the only pieces of code that should be accessing disk
// blocks directly.
typedef uint32 BlockNumber;

#define INVALID_BLOCK_NUMBER ((BlockNumber)0xFFFFFFFF)

// BlockId:
//
// This is a storage type for BlockNumber.	in other words, this type
// is used for on-disk structures (e.g., in HeapTupleData) whereas
// BlockNumber is the type on which calculations are performed (e.g.,
// in access method code).
//
// There doesn't appear to be any reason to have separate types except
// for the fact that BlockIds can be SHORTALIGN'd (and therefore any
// structures that contains them, such as ItemPointerData, can also be
// SHORTALIGN'd).  this is an important consideration for reducing the
// space requirements of the line pointer (ItemIdData) array on each
// page and the header of each heap or index tuple, so it doesn't seem
// wise to change this without good reason.
typedef struct BlockIdData {
  uint16 bi_hi;
  uint16 bi_lo;
} BlockIdData;

typedef BlockIdData* BlockId;

// BlockNumberIsValid
//  True iff blockNumber is valid.
#define BLOCK_NUMBER_IS_VALID(block_number) ((bool)((BlockNumber)(block_number) != INVALID_BLOCK_NUMBER))

// BlockIdIsValid
//  True iff the block identifier is valid.
#define BLOCK_ID_IS_VALID(block_id) ((bool)POINTER_IS_VALID(block_id))

// BlockIdSet
//  Sets a block identifier to the specified value.
#define BLOCK_ID_SET(block_id, block_number)                                     \
  (assert(POINTER_IS_VALID(block_id)), (block_id)->bi_hi = (block_number) >> 16, \
   (block_id)->bi_lo = (block_number)&0xffff)

// BlockIdCopy
//  Copy a block identifier.
#define BLOCK_ID_COPY(to_block_id, from_block_id)                                  \
  (assert(POINTER_IS_VALID(to_block_id)), assert(POINTER_IS_VALID(from_block_id)), \
   (to_block_id)->bi_hi = (from_block_id)->bi_hi, (to_block_id)->bi_lo = (from_block_id)->bi_lo)

// BlockIdEquals
//  Check for block number equality.
#define BLOCK_ID_EQUALS(block_id1, block_id2) \
  ((block_id1)->bi_hi == (block_id2)->bi_hi && (block_id1)->bi_lo == (block_id2)->bi_lo)

// BlockIdGetBlockNumber
//  Retrieve the block number from a block identifier.
#define BLOCK_ID_GET_BLOCK_NUMBER(block_id) \
  (assert(BLOCK_ID_IS_VALID(block_id)), (BlockNumber)(((block_id)->bi_hi << 16) | ((uint16)(block_id)->bi_lo)))

#endif  // RDBMS_STORAGE_BLOCK_H_
