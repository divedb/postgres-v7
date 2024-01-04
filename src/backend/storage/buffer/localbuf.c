//===----------------------------------------------------------------------===//
//
// localbuf.c
//  Local buffer manager. Fast buffer manager for temporary tables
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
//  $Header:
//  /usr/local/cvsroot/pgsql/src/backend/storage/buffer/localbuf.c,v 1.30
//  2000/04/12 17:15:34 momjian Exp$
//
//===----------------------------------------------------------------------===//

#include <math.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/types.h>

#include "rdbms/storage/buf_internals.h"
#include "rdbms/storage/bufmgr.h"
#include "rdbms/utils/elog.h"

extern long int LocalBufferFlushCount;

int NLocBuffer = 64;
BufferDesc* LocalBufferDescriptors = NULL;
Block* LocalBufferBlockPointers = NULL;
long* LocalRefCount = NULL;

static int NextFreeLocalBuf = 0;

// Allocate a local buffer. We do round robin allocation for now.
Buffer* local_buffer_alloc(Relation relation, BlockNumber block_num,
                           bool* found_ptr) {
  int i;
  BufferDesc* buf_hdr = NULL;

  // 如果创建1个新的
  if (block_num == P_NEW) {
    block_num = relation->rd_nblocks;
    relation->rd_nblocks++;
  }

  // A low tech search for now -- not optimized for scans.
  for (i = 0; i < NLocBuffer; i++) {
    if (LocalBufferDescriptors[i].tag.rnode.rel_node ==
            relation->rd_node.rel_node &&
        LocalBufferDescriptors[i].tag.block_num == block_num) {
#ifdef LBDEBUG
      fprintf(stderr, "LB ALLOC (%u,%d) %d\n", RELATION_GET_REL_ID(relation),
              block_num, -i - 1);
#endif

      LocalRefCount[i]++;
      *found_ptr = true;

      return &LocalBufferDescriptors[i];
    }
  }

#ifdef LBDEBUG
  fprintf(stderr, "LB ALLOC (%u,%d) %d\n", RELATION_GET_REL_ID(relation),
          block_num, -NextFreeLocalBuf - 1);
#endif

  // Need to get a new buffer (round robin for now).
  for (i = 0; i < NLocBuffer; i++) {
    int b = (NextFreeLocalBuf + i) % NLocBuffer;

    if (LocalRefCount[b] == 0) {
      buf_hdr = &LocalBufferDescriptors[b];
      LocalRefCount[b]++;
      NextFreeLocalBuf = (b + 1) % NLocBuffer;
      break;
    }
  }

  if (buf_hdr == NULL) {
    elog(ERROR, "no empty local buffer.");
  }

  // This buffer is not referenced but it might still be dirty (the last
  // transaction to touch it doesn't need its contents but has not
  // flushed it).  if that's the case, write it out before reusing it!
  // TODO(gc): cntx_dirty的作用
  if (buf_hdr->flags & BM_DIRTY || buf_hdr->cntx_dirty) {
  }
}