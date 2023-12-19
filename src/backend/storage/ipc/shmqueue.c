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

// Put elem in queue before the given queue
// element. Inserting "before" the queue head puts the elem
// at the tail of the queue.
void shm_queue_insert_before(ShmemQueue* queue, ShmemQueue* elem) {
  ShmemQueue* prev_ptr = (ShmemQueue*)MAKE_PTR(queue->prev);
  ShmemOffset elem_offset = MAKE_OFFSET(elem);

  ASSERT(SHM_PTR_VALID(queue));
  ASSERT(SHM_PTR_VALID(elem));

#ifdef SHMQUEUE_DEBUG

  dumpq(queue, "in SHMQueueInsertBefore: begin");

#endif

  elem->next = prev_ptr->next;
  elem->prev = queue->prev;
  queue->prev = elem_offset;
  prev_ptr->next = elem_offset;

#ifdef SHMQUEUE_DEBUG

  dumpq(queue, "in SHMQueueInsertBefore: end");

#endif
}

// Get the next element from a queue
//
// To start the iteration, pass the queue head as both queue and curElem.
// Returns NULL if no more elements.
//
// Next element is at curElem->next.  If SHMQueue is part of
// a larger structure, we want to return a pointer to the
// whole structure rather than a pointer to its SHMQueue field.
// I.E. struct {
//      int         stuff;
//      SHMQueue    elem;
// } ELEMType;
// When this element is in a queue, (prevElem->next) is struct.elem.
// We subtract linkOffset to get the correct start address of the structure.
//
// calls to SHMQueueNext should take these parameters:
//
//  &(queueHead), &(queueHead), offsetof(ELEMType, elem)
// or
//  &(queueHead), &(curElem->elem), offsetof(ELEMType, elem)
Pointer shm_queue_next(ShmemQueue* queue, ShmemQueue* cur_elem, Size link_offset) {
  ShmemQueue* elem_ptr = (ShmemQueue*)MAKE_PTR(cur_elem->next);

  ASSERT(SHM_PTR_VALID(cur_elem));

  // back to the queue head?
  if (elem_ptr == queue) {
    return NULL;
  }

  return (Pointer)(((char*)elem_ptr) - link_offset);
}

// TRUE if queue head is only element, FALSE otherwise.
bool shm_queue_empty(ShmemQueue* queue) {
  ASSERT(SHM_PTR_VALID(queue));

  if (queue->prev == MAKE_OFFSET(queue)) {
    ASSERT(queue->next == MAKE_OFFSET(queue));

    return true;
  }

  return false;
}

#ifdef SHMQUEUE_DEBUG

static void dumpq(ShmemQueue* q, char* s) {
  char elem[NAME_DATA_LEN];
  char buf[1024];
  ShmemQueue* start = q;
  int count = 0;

  sprintf(buf, "q prevs: %lx", MAKE_OFFSET(q));
  q = (ShmemQueue*)MAKE_PTR(q->prev);

  while (q != start) {
    sprintf(elem, "--->%lx", MAKE_OFFSET(q));
    strcat(buf, elem);
    q = (ShmemQueue*)MAKE_PTR(q->prev);

    if (q->prev == MAKE_OFFSET(q)) {
      break;
    }

    if (count++ > 40) {
      strcat(buf, "BAD PREV QUEUE!!");
      break;
    }
  }

  sprintf(elem, "--->%lx", MAKE_OFFSET(q));
  strcat(buf, elem);
  elog(SHMQUEUE_DEBUG_ELOG, "%s: %s", s, buf);

  sprintf(buf, "q nexts: %lx", MAKE_OFFSET(q));
  count = 0;
  q = (ShmemQueue*)MAKE_PTR(q->next);

  while (q != start) {
    sprintf(elem, "--->%lx", MAKE_OFFSET(q));
    strcat(buf, elem);
    q = (ShmemQueue*)MAKE_PTR(q->next);

    if (q->next == MAKE_OFFSET(q)) {
      break;
    }

    if (count++ > 10) {
      strcat(buf, "BAD NEXT QUEUE!!");
      break;
    }
  }

  sprintf(elem, "--->%lx", MAKE_OFFSET(q));
  strcat(buf, elem);
  elog(SHMQUEUE_DEBUG_ELOG, "%s: %s", s, buf);
}

#endif  // SHMQUEUE_DEBUG