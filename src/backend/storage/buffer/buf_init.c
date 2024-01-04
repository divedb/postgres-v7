//===----------------------------------------------------------------------===//
//
// buf_init.c
//  buffer manager initialization routines
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/storage/buffer/buf_init.c,v 1.42 2001/03/22 03:59:44
//           momjian Exp $
//
//===----------------------------------------------------------------------===//

#include "rdbms/postgres.h"
#include "rdbms/storage/buf_internals.h"

static void shutdown_buffer_pool_access();

// If BMTRACE is defined, we trace the last 200 buffer allocations and
// deallocations in a circular buffer in shared memory.
#ifdef BMTRACE

BufMgrTrace* TraceBuf;
long* CurTraceBuf;

#define BMT_LIMIT 200

#endif  // BMTRACE

int ShowPinTrace = 0;
int DataDescriptors;
int FreeListDescriptor;
int LookupListDescriptor;
int NumDescriptors;

BufferDesc* BufferDescriptors;
Block* BufferBlockPointers;

long* PrivateRefCount;                  // Also used in freelist.c.
bits8* BufferLocks;                     // Flag bits showing locks I have set.
BufferTag* BufferTagLastDirtied;        // Tag buffer had when last dirtied by me.
BufferBlindId* BufferBlindLastDirtied;  // And its BlindId too.
bool* BufferDirtiedByMe;                // T if buf has been dirtied in cur xact.

long int ReadBufferCount;
long int ReadLocalBufferCount;
long int BufferHitCount;
long int LocalBufferHitCount;
long int BufferFlushCount;
long int LocalBufferFlushCount;

// Initialize module:
//
// should calculate size of pool dynamically based on the
// amount of available memory.
void init_buffer_pool(IPCKey key) {
  bool found_bufs;
  bool found_descs;
  int i;

  DataDescriptors = NBuffers;
  FreeListDescriptor = DataDescriptors;
  LookupListDescriptor = DataDescriptors + 1;
  NumDescriptors = DataDescriptors + 1;

  spin_acquire(BufMgrLock);

#ifdef BMTRACE
  CurTraceBuf = (long*)ShmemInitStruct("Buffer trace", (BMT_LIMIT * sizeof(bmtrace)) + sizeof(long), &foundDescs);
  if (!foundDescs) MemSet(CurTraceBuf, 0, (BMT_LIMIT * sizeof(bmtrace)) + sizeof(long));

  TraceBuf = (bmtrace*)&(CurTraceBuf[1]);
#endif

  BufferDescriptors =
      (BufferDesc*)shmem_init_struct("Buffer Descriptors", NumDescriptors * sizeof(BufferDesc), &found_descs);
  BufferBlocks = (BufferBlock)shmem_init_struct("Buffer Blocks", NBuffers * BLCKSZ, &found_bufs);

  if (found_descs || found_bufs) {
    // Both should be present or neither.
    assert(found_descs && found_bufs);
  } else {
    BufferDesc* buf;
    unsigned long block;

    buf = BufferDescriptors;
    block = (unsigned long)BufferBlocks;

    // Link the buffers into a circular, doubly-linked list to
    // initialize free list.  Still don't know anything about
    // replacement strategy in this file.
    for (i = 0; i < DataDescriptors; block += BLCKSZ, buf++, i++) {
      assert(shmem_is_valid((unsigned long)block));

      buf->free_next = i + 1;
      buf->free_prev = i - 1;

      CLEAR_BUFFERTAG(&(buf->tag));
      buf->data = MAKE_OFFSET(block);
      buf->flags = (BM_DELETED | BM_FREE | BM_VALID);
      buf->refcount = 0;
      buf->buf_id = i;
    }

    // Close the circular queue.
    BufferDescriptors[0].free_prev = DataDescriptors - 1;
    BufferDescriptors[DataDescriptors - 1].free_next = 0;
  }

  // Init the rest of the module.
  init_buf_table();
  init_freelist(!found_descs);
  spin_release(BufMgrLock);

  PrivateRefCount = (long*)calloc(NBuffers, sizeof(long));
  BufferLocks = (bits8*)calloc(NBuffers, sizeof(bits8));
  BufferTagLastDirtied = (BufferTag*)calloc(NBuffers, sizeof(BufferTag));
  BufferBlindLastDirtied = (BufferBlindId*)calloc(NBuffers, sizeof(BufferBlindId));
  BufferDirtiedByMe = (bool*)calloc(NBuffers, sizeof(bool));
}