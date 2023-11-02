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

#include "rdbms/storage/buf_table.h"

#include "rdbms/utils/hashfn.h"
#include "rdbms/utils/hsearch.h"

static HTAB* SharedBufHash;

typedef struct lookup {
  BufferTag key;
  Buffer id;
} LookupEnt;

// Initialize shmem hash table for mapping buffers.
void init_buf_table(void) {
  HASHCTL info;
  int hash_flags;

  // Assume lock is held.
  info.keysize = sizeof(BufferTag);
  info.datasize = sizeof(Buffer);
  info.hash = tag_hash;

  hash_flags = (HASH_ELEM | HASH_FUNCTION);

  SharedBufHash = (HTAB*)shmem_ini
}