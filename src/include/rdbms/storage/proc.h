//===----------------------------------------------------------------------===//
//
// proc.h
//  per-process shared memory data structures
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: proc.h,v 1.41 2001/03/22 04:01:08 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_STORAGE_PROC_H_
#define RDBMS_STORAGE_PROC_H_

#include "rdbms/access/xlog.h"
#include "rdbms/storage/lock.h"

// Configurable option.
extern int DeadlockTimeout;

typedef struct {
  IpcSemaphoreId sem_id;  // SysV semaphore set ID
  int sem_num;            // Semaphore number within set
} Sema;

// Each backend has a PROC struct in shared memory.  There is also a list of
// currently-unused PROC structs that will be reallocated to new backends.
//
// links: list link for any list the PROC is in.  When waiting for a lock,
// the PROC is linked into that lock's wait_procs queue. A recycled PROC
// is linked into ProcGlobal's freeProcs list.
struct Proc {
  // proc->links MUST BE FIRST IN STRUCT (see ProcSleep,ProcWakeup,etc)
  ShmemQueue links;    // List link if process is in a list
  Sema sem;            // ONE semaphore to sleep on
  int err_type;        // STATUS_OK or STATUS_ERROR after wakeup
  TransactionId xid;   // Transaction currently being executed by this proc
  TransactionId xmin;  // Minimal running XID as it was when we were starting our xact: vacuum must not remove tuples
                       // deleted by xid >= xmin !

  // XLOG location of first XLOG record written by this backend's
  // current transaction.  If backend is not in a transaction or hasn't
  // yet modified anything, logRec.xrecoff is zero.
  XLogRecPtr log_rec;

  // Info about lock the process is currently waiting for, if any.
  // waitLock and waitHolder are NULL if not currently waiting.
  Lock* wait_lock;          // Lock object we're sleeping on ...
  Holder* wait_holder;      // Per-holder info for awaited lock
  LockMode wait_lock_mode;  // Type of lock we're waiting for
  LockMask held_locks;      // Bitmask for lock types already held on this lock object by this backend
  int pid;                  // This backend's process id
  Oid database_id;          // OID of database this backend is using
  short slocks[MAX_SPINS];  // Spin lock stats
  ShmemQueue proc_holders;  // List of HOLDER objects for locks held or awaited by this backend
};

extern Proc* MyProc;
extern SpinLock ProcStructLock;

#define PROC_INCR_SLOCK(lock)               \
  do {                                      \
    if (MyProc) (MyProc->slocks[(lock)])++; \
  } while (0)

// There is one ProcGlobal struct for the whole installation.
//
// PROC_NSEMS_PER_SET is the number of semaphores in each sys-V semaphore set
// we allocate.  It must be no more than 32 (or however many bits in an int
// on your machine), or our free-semaphores bitmap won't work.  It also must
// be *less than* your kernel's SEMMSL (max semaphores per set) parameter,
// which is often around 25.  (Less than, because we allocate one extra sema
// in each set for identification purposes.)
//
// PROC_SEM_MAP_ENTRIES is the number of semaphore sets we need to allocate
// to keep track of up to MAXBACKENDS backends.
#define PROC_NSEMS_PER_SET   16
#define PROC_SEM_MAP_ENTRIES ((MAX_BACKENDS - 1) / PROC_NSEMS_PER_SET + 1)

typedef struct ProcGlobal {
  // Head of list of free PROC structures.
  ShmemOffset free_procs;

  // Info about semaphore sets used for per-process semaphores.
  IpcSemaphoreId proc_sem_ids[PROC_SEM_MAP_ENTRIES];
  int32 free_sem_map[PROC_SEM_MAP_ENTRIES];

  // In each free_sem_map entry, bit i is set if the i'th semaphore of the
  // set is allocated to a process. (i counts from 0 at the LSB)
} ProcHeader;

void init_proc_global(int max_backends);
void init_process();
void proc_release_locks(bool is_commit);
bool proc_remove(int pid);

void proc_queue_init(ProcQueue* queue);
int proc_sleep(LockMethodTable* lock_method_table, Lockmode lock_mode, Lock* lock, Holder* holder);
Proc* proc_wake_up(Proc* proc, int err_type);
void proc_lock_wake_up(LockMethodTable* lock_method_table, Lock* lock);
void proc_release_spins(Proc* proc);
bool lock_wait_cancel();
void handle_dead_lock(int postgres_signal_arg);

#endif  // RDBMS_STORAGE_PROC_H_
