// =========================================================================
//
// localbuf.c
//  local buffer manager. Fast buffer manager for temporary tables
//  or special cases when the operation is not visible to other backends.
//
//  When a relation is being created, the descriptor will have rd_islocal
//  set to indicate that the local buffer manager should be used. During
//  the same transaction the relation is being created, any inserts or
//  selects from the newly created relation will use the local buffer
//  pool. rd_islocal is reset at the end of a transaction (commit/abort).
//  This is useful for queries like SELECT INTO TABLE and create index.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994=5, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/buffer/localbuf.c,v 1.30 2000/04/12 17:15:34 momjian Exp$
//
// =========================================================================

#include <math.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/types.h>

#include "rdbms/storage/buf_internals.h"

extern long int LocalBufferFlushCount;

int NLocBuffer = 64;
BufferDesc* LocalBufferDescriptors = NULL;
long* LocalRefCount = NULL;

static int NextFreeLocalBuf = 0;

// Allocate a local buffer. We do round robin allocation for now.
Buffer* local_buffer_alloc(Relation relation, BlockNumber block_num, bool* found_ptr) {
  int i;
  BufferDesc* buf_hdr = NULL;

  if (block_num == P_NEW) {
  }
}