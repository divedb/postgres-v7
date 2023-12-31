// =========================================================================
//
// spin.c
//  routines for managing spin locks
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/ipc/spin.c,v 1.24 2000/04/12 17:15:37 momjian Exp $
//
// =========================================================================

// POSTGRES has two kinds of locks: semaphores (which put the
// process to sleep) and spinlocks (which are supposed to be
// short term locks).  Currently both are implemented as SysV
// semaphores, but presumably this can change if we move to
// a machine with a test-and-set (TAS) instruction.  Its probably
// a good idea to think about (and allocate) short term and long
// term semaphores separately anyway.
//
// NOTE:
//  These routines are not supposed to be widely used in Postgres.
//  They are preserved solely for the purpose of porting Mark Sullivan's
//  buffer manager to Postgres.

#include "rdbms/storage/spin.h"

#include "rdbms/utils/elog.h"

// Globals used in this file.
IpcSemaphoreId SpinLockId;

static bool spin_is_locked(SpinLock lock) {
  int semval;

  semval = ipc_semaphore_get_value(SpinLockId, lock);

  return semval < IPC_SEMAPHORE_DEFAULT_START_VALUE;
}

// Create a sysV semaphore array for the spinlocks.
void create_spin_locks(IPCKey key) {
  SpinLockId = ipc_semaphore_create(key, MAX_SPINS, IPCProtection, IpcSemaphoreDefaultStartValue, true);

  if (SpinLockId <= 0) {
    elog(STOP, "%s: cannot create spin locks", __func__);
  }
}

// We need several spinlocks for bootstrapping:
// ShmemIndexLock (for the shmem index table) and
// ShmemLock (for the shmem allocator), BufMgrLock (for buffer
// pool exclusive access), LockMgrLock (for the lock table), and
// ProcStructLock (a spin lock for the shared process structure).
// If there's a Sony WORM drive attached, we also have a spinlock
// (SJCacheLock) for it.  Same story for the main memory storage mgr.
void init_spin_locks() {
  extern SpinLock ShmemLock;
  extern SpinLock ShmemIndexLock;
  extern SpinLock BufMgrLock;
  extern SpinLock LockMgrLock;
  extern SpinLock ProcStructLock;
  extern SpinLock SInvalLock;
  extern SpinLock OidGenLockId;
  extern SpinLock XidGenLockId;
  extern SpinLock ControlFileLockId;

#ifdef STABLE_MEMORY_STORAGE
  extern SpinLock MMCacheLock;

#endif

  // These five (or six) spinlocks have fixed location is shmem.
  ShmemLock = (SpinLock)SHMEM_LOCKID;
  ShmemIndexLock = (SpinLock)SHMEM_INDEX_LOCKID;
  BufMgrLock = (SpinLock)BUF_MGR_LOCKID;
  LockMgrLock = (SpinLock)LOCK_MGR_LOCKID;
  ProcStructLock = (SpinLock)PROC_STRUCT_LOCKID;
  SInvalLock = (SpinLock)SINVAL_LOCKID;
  OidGenLockId = (SpinLock)OID_GEN_LOCKID;
  XidGenLockId = (SpinLock)XID_GEN_LOCKID;
  ControlFileLockId = (SpinLock)CNTL_FILE_LOCKID;

#ifdef STABLE_MEMORY_STORAGE
  MMCacheLock = (SpinLock)MMCACHELOCKID;
#endif
}

// Try to grab a spinlock.
// FAILS if the semaphore is corrupted.
void spin_acquire(SpinLock lock) {
  ipc_semaphore_lock(SpinLockId, lock, IPC_EXCLUSIVE_LOCK);

  // TODO(gc): fix this.
  // PROC_INCR_SLOCK(lock);
}

// Release a spin lock.
void spin_release(SpinLock lock) {
  assert(spin_is_locked(lock));

  // TODO(gc): fix this.
  // PROC_DECR_SLOCK(lock);
  ipc_semaphore_unlock(SpinLockId, lock, IPC_EXCLUSIVE_LOCK);
}