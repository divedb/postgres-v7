// =========================================================================
//
// buf_table.c
//  routines for finding buffers in the buffer pool.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/buffer/buf_table.c,v 1.16 2000/01/26 05:56:50 momjian
// Exp $
//
// =========================================================================

// OLD COMMENTS
//
// Data Structures:
//
//  Buffers are identified by their BufferTag (buf.h).	This
// file contains routines for allocating a shmem hash table to
// map buffer tags to buffer descriptors.
//
// Synchronization:
//
//  All routines in this file assume buffer manager spinlock is
//  held by their caller.

#include "rdbms/storage/buf_internals.h"
#include "rdbms/storage/bufmgr.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/hashfn.h"
#include "rdbms/utils/hsearch.h"

static HTAB* SharedBufHash;

typedef struct lookup {
  BufferTag key;
  Buffer id;
} LookupEnt;

// Initialize shmem hash table for mapping buffers.
void init_buf_table(void) {
  extern int NBuffers;

  HASHCTL info;
  int hash_flags;

  // Assume lock is held.
  info.keysize = sizeof(BufferTag);
  info.datasize = sizeof(Buffer);
  info.hash = tag_hash;

  hash_flags = (HASH_ELEM | HASH_FUNCTION);

  SharedBufHash = (HTAB*)shmem_init_hash("Shared Buffer Lookup Table", NBuffers, NBuffers, &info, hash_flags);

  if (!SharedBufHash) {
    elog(FATAL, "%s couldn't initialize shared buffer pool hash table", __func__);
    exit(1);
  }
}

BufferDesc* buf_table_lookup(BufferTag* tag_ptr) {
  extern BufferDesc* BufferDescriptors;

  LookupEnt* result;
  bool found;

  if (tag_ptr->block_num == P_NEW) {
    return NULL;
  }

  result = (LookupEnt*)hash_search(SharedBufHash, (char*)tag_ptr, HASH_FIND, &found);

  if (!result) {
    elog(ERROR, "%s: BufferLookup table corrupted", __func__);
    return NULL;
  }

  if (!found) {
    return NULL;
  }

  return &(BufferDescriptors[result->id]);
}

bool buf_table_delete(BufferDesc* buf_desc) {
  LookupEnt* result;
  bool found;

  // Buffer not initialized or has been removed from table already.
  // BM_DELETED keeps us from removing buffer twice.
  if (buf_desc->flags & BM_DELETED) {
    return true;
  }

  buf_desc->flags |= BM_DELETED;
  result = (LookupEnt*)hash_search(SharedBufHash, (char*)&(buf_desc->tag), HASH_REMOVE, &found);

  if (!(result && found)) {
    elog(ERROR, "%s: BufferLookup table corrupted", __func__);
    return false;
  }

  return true;
}

bool buf_table_insert(BufferDesc* buf_desc) {
  LookupEnt* result;
  bool found;

  // Cannot insert it twice.
  assert(buf_desc->flags & BM_DELETED);

  buf_desc->flags &= ~(BM_DELETED);
  result = (LookupEnt*)hash_search(SharedBufHash, (char*)&(buf_desc->tag), HASH_ENTER, &found);

  if (!result) {
    elog(ERROR, "%s: BufferLookup table corrupted", __func__);
    return false;
  }

  // Found something else in the table.
  if (found) {
    elog(ERROR, "%s: BufferLookup table corrupted", __func__);
    return false;
  }

  result->id = buf_desc->buf_id;

  return true;
}