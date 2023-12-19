//===----------------------------------------------------------------------===//
//
// shmqueue.c
//  shared memory linked lists
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/storage/ipc/shmqueue.c,v 1.16 2001/03/22 03:59:45
//           momjian Exp $
//
// NOTES
//
// Package for managing doubly-linked lists in shared memory.
// The only tricky thing is that SHM_QUEUE will usually be a field
// in a larger record. SHMQueueNext has to return a pointer
// to the record itself instead of a pointer to the SHMQueue field
// of the record.  It takes an extra parameter and does some extra
// pointer arithmetic to do this correctly.
//
// NOTE: These are set up so they can be turned into macros some day.
//
//-------------------------------------------------------------------------

#include "rdbms/postgres.h"
#include "rdbms/storage/shmem.h"

#ifdef SHMQUEUE_DEBUG

#define SHMQUEUE_DEBUG_ELOG NOTICE

static void dumpq(ShmemQueue* q, char* s);

#endif  // SHMQUEUE_DEBUG

// Make the head of a new queue point to itself.
void shm_queue_init(ShmemQueue* queue) {
  ASSERT(SHM_PTR_VALID(queue));

  queue->prev = queue->next = MAKE_OFFSET(queue);
}

// Clear an element's links.
void shm_queue_elem_init(ShmemQueue* queue) {
  ASSERT(SHM_PTR_VALID(queue));

  queue->prev = queue->next = INVALID_OFFSET;
}

// Remove an element from the queue and close the links.
void shm_queue_delete(ShmemQueue* queue) {
  ShmemQueue* next_elem = (ShmemQueue*)MAKE_PTR(queue->next);
  ShmemQueue* prev_elem = (ShmemQueue*)MAKE_PTR(queue->prev);

  ASSERT(SHM_PTR_VALID(queue));
  ASSERT(SHM_PTR_VALID(next_elem));
  ASSERT(SHM_PTR_VALID(prev_elem));

#ifdef SHMQUEUE_DEBUG

  dumpq(queue, "in SHMQueueDelete: begin");

#endif

  prev_elem->next = queue->next;
  next_elem->prev = queue->prev;
  queue->prev = queue->next = INVALID_OFFSET;
}