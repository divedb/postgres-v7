//===----------------------------------------------------------------------===//
//
// proc.c
//  routines to manage per-process shared memory data structure
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/storage/lmgr/proc.c,v 1.100 2001/03/22 06:16:17
//           momjian Exp $
//
//===----------------------------------------------------------------------===//

// Each postgres backend gets one of these.  We'll use it to
// clean up after the process should the process suddenly die.
//
//
// Interface (a):
//  ProcSleep(), ProcWakeup(),
//  ProcQueueAlloc() -- create a shm queue for sleeping processes
//  ProcQueueInit() -- create a queue without allocing memory
//
// Locking and waiting for buffers can cause the backend to be
// put to sleep.  Whoever releases the lock, etc. wakes the
// process up again (and gives it an error code so it knows
// whether it was awoken on an error condition).
//
// Interface (b):
//
// ProcReleaseLocks -- frees the locks associated with current transaction
//
// ProcKill         -- destroys the shared memory state (and locks)
//                     associated with the process.
//
// 5/15/91          -- removed the buffer pool based lock chain in favor
//                     of a shared memory lock chain. The write-protection is
//                     more expensive if the lock chain is in the buffer pool.
//                     The only reason I kept the lock chain in the buffer pool
//                     in the first place was to allow the lock table to grow larger
//                     than available shared memory and that isn't going to work
//                     without a lot of unimplemented support anyway.
//
// 4/7/95           -- instead of allocating a set of 1 semaphore per process, we
//                      allocate a semaphore from a set of PROC_NSEMS_PER_SET semaphores
//                      shared among backends (we keep a few sets of semaphores around).
//                      This is so that we can support more backends. (system-wide semaphore
//                      sets run out pretty fast.)   -ay 4/95
#include "rdbms/storage/proc.h"

#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "rdbms/access/xact.h"
#include "rdbms/miscadmin.h"
#include "rdbms/postgres.h"

int DeadlockTimeout = 1000;

// Spin lock for manipulating the shared process data structure:
// ProcGlobal.... Adding an extra spin lock seemed like the smallest
// hack to get around reading and updating this structure in shared
// memory. -mer 17 July 1991
SpinLock ProcStructLock;
Proc* MyProc = NULL;

static ProcHeader* ProcGlobal = NULL;
static bool WaitingForLock = false;

static void proc_kill();
static void proc_get_new_sem_id_and_num(IpcSemaphoreId* sem_id, int* sem_num);
static void proc_free_sem(IpcSemaphoreId sem_id, int sem_num);
static void zero_proc_semaphore(Proc* proc);
static void proc_free_all_semaphores();

// initializes the global process table. We put it here so that
// the postmaster can do this initialization. (ProcFreeAllSemaphores needs
// to read this table on exiting the postmaster. If we have the first
// backend do this, starting up and killing the postmaster without
// starting any backends will be a problem.)
//
// We also allocate all the per-process semaphores we will need to support
// the requested number of backends. We used to allocate semaphores
// only when backends were actually started up, but that is bad because
// it lets Postgres fail under load --- a lot of Unix systems are
// (mis)configured with small limits on the number of semaphores, and
// running out when trying to start another backend is a common failure.
// So, now we grab enough semaphores to support the desired max number
// of backends immediately at initialization --- if the sysadmin has set
// MaxBackends higher than his kernel will support, he'll find out sooner
// rather than later.
void init_proc_global(int max_backends) {
  bool found = false;

  // Attach to the free list.
  ProcGlobal = (ProcHeader*)shmem_init_struct("Proc Header", sizeof(ProcHeader), &found);

  // We're the first - initialize.
  // XXX if found should ever be true, it is a sign of impending doom ...
  // ought to complain if so?
  if (!found) {
    int i;

    ProcGlobal->free_procs = INVALID_OFFSET;

    for (i = 0; i < PROC_SEM_MAP_ENTRIES; i++) {
      ProcGlobal->proc_sem_ids[i] = -1;
      ProcGlobal->free_sem_map[i] = 0;
    }

    // Arrange to delete semas on exit --- set this up now so that we
    // will clean up if pre-allocation fails.  We use our own
    // freeproc, rather than IpcSemaphoreCreate's removeOnExit option,
    // because we don't want to fill up the on_shmem_exit list with a
    // separate entry for each semaphore set.
    on_shmem_exit(proc_free_all_semaphores, 0);

    // Pre-create the semaphores for the first maxBackends processes.
    ASSERT(max_backends > 9 && max_backends <= MAX_BACKENDS);

    for (i = 0; i < ((max_backends - 1) / PROC_NSEMS_PER_SET + 1); i++) {
      IpcSemaphoreId sem_id;
      sem_id = ipc_semaphore_create(PROC_NSEMS_PER_SET, IPC_PROTECTION, 1, false);
      ProcGlobal->proc_sem_ids[i] = sem_id;
    }
  }
}

// Create a per-process data structure for this process used by the lock manager on semaphore queues.
void init_process() {
  bool found = false;
  unsigned long location;
  unsigned long my_offset;

  spin_acquire(ProcStructLock);

  // Attach to the ProcGlobal structure.
  ProcGlobal = (ProcHeader*)shmem_init_struct("Proc Header", sizeof(ProcHeader), &found);

  if (!found) {
    // This should not happen. InitProcGlobal() is called before this.
    elog(STOP, "%s: Proc Header uninitialized", __func__);
  }

  if (MyProc != NULL) {
    spin_release(ProcStructLock);
    elog(ERROR, "%s: you already exist", __func__);
  }

  // Try to get a proc struct from the free list first.
  my_offset = ProcGlobal->free_procs;

  if (my_offset != INVALID_OFFSET) {
    MyProc = (Proc*)MAKE_PTR(my_offset);
    ProcGlobal->free_procs = MyProc->links.next;
  } else {
    // Have to allocate one.  We can't use the normal shmem index
    // table mechanism because the proc structure is stored by PID
    // instead of by a global name (need to look it up by PID when we
    // cleanup dead processes).
    MyProc = (Proc*)shmem_alloc(sizeof(Proc));

    if (!MyProc) {
      spin_release(ProcStructLock);
      elog(FATAL, "%s: cannot create new proc: out of memory", __func__);
    }
  }

  // Zero out the spin lock counts and set the sLocks field for
  // ProcStructLock to 1 as we have acquired this spinlock above but
  // didn't record it since we didn't have MyProc until now.
  MEMSET(MyProc->slocks, 0, sizeof(MyProc->slocks));
  MyProc->slocks[ProcStructLock] = 1;

  // Set up a wait-semaphore for the proc.
  if (IsUnderPostmaster) {
    proc_get_new_sem_id_and_num(&MyProc->sem.sem_id, &MyProc->sem.sem_num);

    // we might be reusing a semaphore that belongs to a dead backend.
    // So be careful and reinitialize its value here.
    zero_proc_semaphore(MyProc);
  } else {
    MyProc->sem.sem_id = -1;
    MyProc->sem.sem_num = -1;
  }

  shm_queue_elem_init(&MyProc->links);
  MyProc->err_type = STATUS_OK;
  MyProc->pid = MyProcPid;
  MyProc->database_id = MyDatabaseId;
  MyProc->xid = INVALID_TRANSACTION_ID;
  MyProc->xmin = INVALID_TRANSACTION_ID;
  MyProc->wait_lock = NULL;
  MyProc->wait_holder = NULL;
  shm_queue_init(&MyProc->proc_holders);

  // Release the lock.
  spin_release(ProcStructLock);

  // Install ourselves in the shmem index table. The name to use is
  // determined by the OS-assigned process id. That allows the cleanup
  // process to find us after any untimely exit.
  location = MAKE_OFFSET(MyProc);

  if ((!shmem_pid_lookup(MyProcPid, &location)) || (location != MAKE_OFFSET(MyProc))) {
    elog(STOP, "%s: ShmemPID table broken", __func__);
  }

  // Arrange to clean up at backend exit.
  on_shmem_exit(proc_kill, 0);

  // Now that we have a PROC, we could try to acquire locks, so
  // initialize the deadlock checker.
  init_deadlock_checking();
}