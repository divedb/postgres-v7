// =========================================================================
//
// memutils.h
//  this file contains general memory alignment, allocation
//  and manipulation stuff that used to be spread out
//  between the following files:
//
//      align.h   alignment macros
//      aset.h    memory allocation set stuff
//      oset.h    (used by aset.h)
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: memutils.h,v 1.34 2000/04/12 17:16:55 momjian Exp $
//
// =========================================================================
#ifndef RDBMS_UTILS_MEMUTILS_H_
#define RDBMS_UTILS_MEMUTILS_H_

#include <stddef.h>

// Alignment macros: align a length or address appropriately for a given type.
//
// There used to be some incredibly crufty platform-dependent hackery here,
// but now we rely on the configure script to get the info for us. Much nicer.
//
// NOTE: TYPEALIGN will not work if ALIGNVAL is not a power of 2.
// That case seems extremely unlikely to occur in practice, however.

#define TYPEALIGN(ALIGNVAL, LEN) (((long)(LEN) + (ALIGNVAL - 1)) & ~(ALIGNVAL - 1))

#define SHORTALIGN(LEN)  TYPEALIGN(alignof(short), LEN)
#define INTALIGN(LEN)    TYPEALIGN(alignof(int), LEN)
#define LONGALIGN(LEN)   TYPEALIGN(alignof(long), LEN)
#define DOUBLEALIGN(LEN) TYPEALIGN(alignof(double), LEN)
#define MAXALIGN(LEN)    TYPEALIGN(alignof(maxalign_t), LEN)

#endif  // RDBMS_UTILS_MEMUTILS_H_
