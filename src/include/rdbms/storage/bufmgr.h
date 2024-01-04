//===----------------------------------------------------------------------===//
//
// bufmgr.h
//  POSTGRES buffer manager definitions.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: bufmgr.h,v 1.37 2000/04/12 17:16:51 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_STORAGE_BUFMGR_H_
#define RDBMS_STORAGE_BUFMGR_H_

#include "rdbms/access/xlogdefs.h"
#include "rdbms/storage/buf.h"
#include "rdbms/storage/lock.h"
#include "rdbms/utils/rel.h"

typedef void* Block;

// globals.c
extern int NBuffers;

// buf_init.c
extern Block* BufferBlockPointers;
extern long* PrivateRefCount;

// localbuf.c
extern int NLocBuffer;
extern Block* LocalBufferBlockPointers;
extern long* LocalRefCount;

// Special pageno for bget.
#define P_NEW INVALID_BLOCK_NUMBER

// Buffer context lock modes
#define BUFFER_LOCK_UNLOCK    0
#define BUFFER_LOCK_SHARE     1
#define BUFFER_LOCK_EXCLUSIVE 2

#define BAD_BUFFER_ID(bid) ((bid) < 1 || (bid) > NBuffers)
#define INVALID_DESCRIPTOR (-3)

#define UNLOCK_AND_RELEASE_BUFFER(buffer) (lock_buffer(buffer, BUFFER_LOCK_UNLOCK), release_buffer(buffer))
#define UNLOCK_AND_WRITE_BUFFER(buffer)   (lock_buffer(buffer, BUFFER_LOCK_UNLOCK), write_buffer(buffer))
#define BUFFER_IS_VALID(bufnum)           (BUFFER_IS_LOCAL(bufnum) ? ((bufnum) >= -NLocBuffer) : (!BAD_BUFFER_ID(bufnum)))

#define BUFFER_IS_PINNED(bufnum)                                                         \
  (BUFFER_IS_LOCAL(bufnum) ? ((bufnum) >= -NLocBuffer && LocalRefCount[-(bufnum)-1] > 0) \
                           : (BAD_BUFFER_ID(bufnum) ? false : (PrivateRefCount[(bufnum)-1] > 0)))

// Increment the pin count on a buffer that we have *already* pinned
// at least once.
//
// This macro cannot be used on a buffer we do not have pinned,
// because it doesn't change the shared buffer state. Therefore the
// Assert checks are for refcount > 0.  Someone got this wrong once...
#define INCR_BUFFER_REF_COUNT(buffer)                                                                      \
  (BUFFER_IS_LOCAL(buffer)                                                                                 \
       ? ((void)ASSERT_MACRO((buffer) >= -NLocBuffer), (void)ASSERT_MACRO(LocalRefCount[-(buffer)-1] > 0), \
          (void)LocalRefCount[-(buffer)-1]++)                                                              \
       : ((void)ASSERT_MACRO(!BAD_BUFFER_ID(buffer)), (void)ASSERT_MACRO(PrivateRefCount[(buffer)-1] > 0), \
          (void)PrivateRefCount[(buffer)-1]++))

#define BUFFER_GET_BLOCK(buffer)          \
  (ASSERT_MACRO(BUFFER_IS_VALID(buffer)), \
   BUFFER_IS_LOCAL(buffer) ? LocalBufferBlockPointers[-(buffer)-1] : BufferBlockPointers[(buffer)-1])

// bufmgr.c
Buffer relation_get_buffer_write_buffer(Relation relation, BlockNumber block_number, Buffer buffer);

void init_buffer_pool();

#endif  // RDBMS_STORAGE_BUFMGR_H_
