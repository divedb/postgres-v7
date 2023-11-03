// =========================================================================
//
// shmqueue.c
//  shared memory linked lists
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//	  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/ipc/shmqueue.c,v 1.13 2000/01/26 05:56:58 momjian Exp $
//
// NOTES
//
// Package for managing doubly=linked lists in shared memory.
// The only tricky thing is that SHM_QUEUE will usually be a field
// in a larger record.	SHMQueueGetFirst has to return a pointer
// to the record itself instead of a pointer to the SHMQueue field
// of the record.  It takes an extra pointer and does some extra
// pointer arithmetic to do this correctly.
//
// NOTE: These are set up so they can be turned into macros some day.
//
// =========================================================================

#include "rdbms/storage/shmem_queue.h"

#ifdef SHMQUEUE_DEBUG

#define SHMQUEUE_DEBUG_DEL  // deletions.
#define SHMQUEUE_DEBUG_HD   // head inserts.
#define SHMQUEUE_DEBUG_TL   // tail inserts.
#define SHMQUEUE_DEBUG_ELOG NOTICE

#endif

// Make the head of a new queue point to itself.
void shm_queue_init(SHM_QUEUE* queue) {
  assert(SHM_PTR_VALID(queue));

  queue->prev = MAKE_OFFSET(queue);
  queue->next = MAKE_OFFSET(queue);
}

// Clear an element's links.
void shm_queue_elem_init(SHM_QUEUE* queue) {
  assert(SHM_PTR_VALID(queue));

  queue->prev = INVALID_OFFSET;
  queue->next = INVALID_OFFSET;
}

// Remove an element from the queue and close the links.
void shm_queue_delete(SHM_QUEUE* queue) { SHM_QUEUE* next_elem = }
