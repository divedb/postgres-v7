// =========================================================================
//
// freelist.c
//  routines for manipulating the buffer pool's replacement strategy
//  freelist.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/buffer/freelist.c,v 1.21 2000/04/09 04:43:19 tgl Exp $
//
// =========================================================================

// OLD COMMENTS
//
// Data Structures:
//  SharedFreeList is a circular queue.  Notice that this
//  is a shared memory queue so the next/prev "ptrs" are
//  buffer ids, not addresses.
//
// Sync: all routines in this file assume that the buffer
//  semaphore has been acquired by the caller.

#include "rdbms/storage/freelist.h"

#include "rdbms/postgres.h"

static BufferDesc* SharedFreeList;

// Only actually used in debugging.  The lock
// should be acquired before calling the freelist manager.
extern SPINLOCK BufMgrLock;

#define IsInQueue(bf)                                                                            \
  (assert((bf->free_next != INVALID_DESCRIPTOR)), assert((bf->free_prev != INVALID_DESCRIPTOR)), \
   assert((bf->flags & BM_FREE)))

#define NotInQueue(bf)                                                                           \
  (assert((bf->free_next == INVALID_DESCRIPTOR)), assert((bf->free_prev == INVALID_DESCRIPTOR)), \
   assert(!(bf->flags & BM_FREE)))

// In theory, this is the only routine that needs to be changed
// if the buffer replacement strategy changes. Just change
// the manner in which buffers are added to the freelist queue.
// Currently, they are added on an LRU basis.
void add_buffer_to_freelist(BufferDesc* buf_desc) {
#ifdef BMTRACE
  _bm_trace(bf->tag.relId.dbId, bf->tag.relId.relId, bf->tag.blockNum, BufferDescriptorGetBuffer(bf), BMT_DEALLOC);
#endif  // BMTRACE

  NotInQueue(buf_desc);

  // Change bf so it points to inFrontOfNew and its successor.
  buf_desc->free_prev = SharedFreeList->free_prev;
  buf_desc->free_next = Free_List_Descriptor;

  // Insert new into chain.
  BufferDescriptors[buf_desc->free_next].free_prev = buf_desc->buf_id;
  BufferDescriptors[buf_desc->free_prev].free_next = buf_desc->buf_id;
}

// Make buffer unavailable for replacement.
void pin_buffer(BufferDesc* buf_desc) {
  long b;

  if (buf_desc->refcount == 0) {
    IsInQueue(buf_desc);

    // Remove from freelist queue.
    BufferDescriptors[buf_desc->free_next].free_prev = buf_desc->free_prev;
    BufferDescriptors[buf_desc->free_prev].free_next = buf_desc->free_next;
    buf_desc->free_next = buf_desc->free_prev = INVALID_DESCRIPTOR;

    // Mark buffer as no longer free.
    buf_desc->flags &= ~BM_FREE;
  } else {
    NotInQueue(buf_desc);
  }

  b = BufferDescriptorGetBuffer(buf_desc) - 1;

  assert(PrivateRefCount[b] >= 0);

  // TODO(gc): 为什么等于的0时候 需要+1
  if (PrivateRefCount[b] == 0) {
    buf_desc->refcount++;
  }

  PrivateRefCount[b]++;
}

// Make buffer available for replacement.
void unpin_buffer(BufferDesc* buf_desc) {
  long b = BufferDescriptorGetBuffer(buf_desc) - 1;

  assert(buf_desc->refcount > 0);
  assert(PrivateRefCount[b] > 0);
  PrivateRefCount[b]--;

  if (PrivateRefCount[b] == 0) {
    buf_desc->refcount--;
  }

  if (buf_desc->refcount == 0) {
    add_buffer_to_freelist(buf_desc);
    buf_desc->flags |= BM_FREE;
  }
}

// Get the 'next' buffer from the freelist.
BufferDesc* get_free_buffer(void) {
  BufferDesc* buf_desc;

  if (Free_List_Descriptor == SharedFreeList->free_next) {
    // Queue is empty. All buffers in the buffer pool are pinned.
    printf("[ERROR]: out of free buffers. Time to abort!\n");

    return NULL;
  }

  buf_desc = &(BufferDescriptors[SharedFreeList->free_next]);

  // Remove from freelist queue.
  BufferDescriptors[buf_desc->free_next].free_prev = buf_desc->free_prev;
  BufferDescriptors[buf_desc->free_prev].free_next = buf_desc->free_next;
  buf_desc->free_next = buf_desc->free_prev = INVALID_DESCRIPTOR;

  buf_desc->flags &= ~(BM_FREE);

  return buf_desc;
}

// Initialize the dummy buffer descriptor used as a freelist head.
//
// Assume:
//  All of the buffers are already linked in a circular
//  queue. Only called by postmaster and only during
//  initialization.
void init_freelist(bool init) {
  SharedFreeList = &(BufferDescriptors[Free_List_Descriptor]);

  if (init) {
    // We only do this once, normally the postmaster.
    SharedFreeList->data = INVALID_OFFSET;
    SharedFreeList->flags = 0;
    SharedFreeList->flags &= ~(BM_VALID | BM_DELETED | BM_FREE);
    SharedFreeList->buf_id = Free_List_Descriptor;

    // Insert it into a random spot in the circular queue.
    SharedFreeList->free_next = BufferDescriptors[0].free_next;
    SharedFreeList->free_prev = 0;
    BufferDescriptors[SharedFreeList->free_next].free_prev = Free_List_Descriptor;
    BufferDescriptors[SharedFreeList->free_prev].free_next = Free_List_Descriptor;
  }
}