// =========================================================================
//
// bufmgr.h
//  POSTGRES buffer manager definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: bufmgr.h,v 1.37 2000/04/12 17:16:51 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_BUFMGR_H_
#define RDBMS_STORAGE_BUFMGR_H_

#include "rdbms/storage/block.h"

// The maximum size of a disk block for any possible installation.
//
// in theory this could be anything, but in practice this is actually
// limited to 2^13 bytes because we have limited ItemIdData.lp_off and
// ItemIdData.lp_len to 13 bits (see itemid.h).
//
// limit is now 2^15.  Took four bits from ItemIdData.lp_flags and gave
// two apiece to ItemIdData.lp_len and lp_off. darrenk 01/06/98
#define MAXBLCKSZ 32768

typedef void* Block;

// Special pageno for bget.
#define P_NEW InvalidBlockNumber

typedef bits16 BufferLock;

// The rest is function defns in the bufmgr that are externally callable.

// These routines are beaten on quite heavily, hence the macroization.
// See buf_internals.h for a related comment.
#define BufferDescriptorGetBuffer(bdesc) ((bdesc)->buf_id + 1)

extern int ShowPinTrace;

// Buffer context lock modes
#define BUFFER_LOCK_UNLOCK    0
#define BUFFER_LOCK_SHARE     1
#define BUFFER_LOCK_EXCLUSIVE 2

// BufferIsValid
//  True iff the given buffer number is valid (either as a shared
//  or local buffer).
//
// Note:
//  BufferIsValid(InvalidBuffer) is False.
//  BufferIsValid(UnknownBuffer) is False.
//
// Note: For a long time this was defined the same as BufferIsPinned,
// that is it would say False if you didn't hold a pin on the buffer.
// I believe this was bogus and served only to mask logic errors.
// Code should always know whether it has a buffer reference,
// independently of the pin state.
#define BufferIsValid(bufnum) (BufferIsLocal(bufnum) ? ((bufnum) >= -NLocBuffer) : (!BAD_BUFFER_ID(bufnum)))

#endif  // RDBMS_STORAGE_BUFMGR_H_
