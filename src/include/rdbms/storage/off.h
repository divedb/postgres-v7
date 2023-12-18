// =========================================================================
//
// off.h
//  POSTGRES disk "offset" definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: off.h,v 1.9 2000/01/26 05:58:33 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_OFF_H_
#define RDBMS_STORAGE_OFF_H_

typedef uint16 OffsetNumber;

#define INVALID_OFFSET_NUMBER ((OffsetNumber)0)
#define FIRST_OFFSET_NUMBER   ((OffsetNumber)1)
#define MAX_OFFSET_NUMBER     ((OffsetNumber)(BLCKSZ / sizeof(ItemIdData)))
#define OFFSET_NUMBER_MASK    (0XFFFF)

#define OFFSET_NUMBER_IS_VALID(offset_num) \
  ((bool)((offset_num != INVALID_OFFSET_NUMBER) && (offset_num <= MAX_OFFSET_NUMBER)))

// OffsetNumberNext
// OffsetNumberPrev
//  Increments/decrements the argument.  These macros look pointless
//  but they help us disambiguate the different manipulations on
//  OffsetNumbers (e.g., sometimes we substract one from an
//  OffsetNumber to move back, and sometimes we do so to form a
//  real C array index).
#define OFFSET_NUMBER_NEXT(offset_num) ((OffsetNumber)(1 + (offset_num)))
#define OFFSET_NUMBER_PREV(offset_num) ((OffsetNumber)(-1 + (offset_num)))

#endif  // RDBMS_STORAGE_OFF_H_
