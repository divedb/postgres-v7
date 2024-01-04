//===----------------------------------------------------------------------===//
//
// lock.c
//  POSTGRES low-level lock mechanism
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/storage/lmgr/lock.c
//  v 1.88 2001/03/22 03:59:46 momjian Exp $
//
// NOTES
//  Outside modules can create a lock table and acquire/release
//  locks. A lock table is a shared memory hash table. When
//  a process tries to acquire a lock of a type that conflicts
//  with existing locks, it is put to sleep using the routines
//  in storage/lmgr/proc.c.
//
//  For the most part, this code should be invoked via lmgr.c
//  or another lock-management module, not directly.
//
//  Interface:
//
//  LockAcquire(), LockRelease(), LockMethodTableInit(),
//  LockMethodTableRename(), LockReleaseAll,
//  LockCheckConflicts(), GrantLock()
//
//===----------------------------------------------------------------------===//

#include "rdbms/storage/lock.h"

#include "rdbms/access/transam.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/memutils.h"  // TopMemoryContext

static int wait_on_lock(LockMethod lock_method, LockMode lock_mode, Lock* lock, Holder* holder);
static void lock_count_my_locks(ShmemOffset lock_offset, Proc* proc, int* my_holding);
static void lock_method_init(LockMethodTable* lock_method_table, LockMask* conflictp);

static char* LockModeNames[] = {"INVALID",   "AccessShareLock",       "RowShareLock",  "RowExclusiveLock",
                                "ShareLock", "ShareRowExclusiveLock", "ExclusiveLock", "AccessExclusiveLock"};

static char* DeadLockMessage = "Deadlock detected.\n\tSee the lock(l) manual page for a possible cause.";

// The following configuration options are available for lock debugging:
//
//  TRACE_LOCKS         -- give a bunch of output what's going on in this file
//  TRACE_USERLOCKS     -- same but for user locks
//  TRACE_LOCK_OIDMIN   -- do not trace locks for tables below this oid
//                         (use to avoid output on system tables)
//  TRACE_LOCK_TABLE    -- trace locks on this table (oid) unconditionally
//  DEBUG_DEADLOCKS     -- currently dumps locks at untimely occasions ;)
//
// Furthermore, but in storage/ipc/spin.c:
//  TRACE_SPINLOCKS     -- trace spinlocks (pretty useless)
//
// Define LOCK_DEBUG at compile time to get all these enabled.
int TraceLockOidMin = BOOTSTRAP_OBJECT_ID_DATA;
bool TraceLocks = false;
bool TraceUserLocks = false;
int TraceLockTable = 0;
bool DebugDeadLocks = false;

inline static bool lock_debug_enabled(const Lock* lock) {
  return (((LOCK_LOCK_METHOD(*lock) == DEFAULT_LOCK_METHOD && TraceLocks) ||
           (LOCK_LOCK_METHOD(*lock) == USER_LOCK_METHOD && TraceUserLocks)) &&
          (lock->tag.rel_id >= (Oid)TraceLockOidMin)) ||
         (TraceLockTable && (lock->tag.rel_id == TraceLockTable));
}

inline static void lock_print(const char* where, const Lock* lock, LockMode type) {
  if (lock_debug_enabled(lock)) {
    elog(DEBUG,
         "%s: lock(%lx) tbl(%d) rel(%u) db(%u) obj(%u) grantMask(%x) "
         "req(%d,%d,%d,%d,%d,%d,%d)=%d "
         "grant(%d,%d,%d,%d,%d,%d,%d)=%d wait(%d) type(%s)",
         where, MAKE_OFFSET(lock), lock->tag.lock_method, lock->tag.rel_id, lock->tag.db_id, lock->tag.obj_id.blk_no,
         lock->grant_mask, lock->requested[1], lock->requested[2], lock->requested[3], lock->requested[4],
         lock->requested[5], lock->requested[6], lock->requested[7], lock->nrequested, lock->granted[1],
         lock->granted[2], lock->granted[3], lock->granted[4], lock->granted[5], lock->granted[6], lock->granted[7],
         lock->ngranted, lock->wait_procs.size, LockModeNames[type]);
  }
}

inline static void holder_print(const char* where, const Holder* holderp) {
  if ((((HOLDER_LOCK_METHOD(*holderp) == DEFAULT_LOCK_METHOD && TraceLocks) ||
        (HOLDER_LOCK_METHOD(*holderp) == USER_LOCK_METHOD && TraceUserLocks)) &&
       (((Lock*)MAKE_PTR(holderp->tag.lock))->tag.rel_id >= (Oid)TraceLockOidMin)) ||
      (TraceLockTable && (((Lock*)MAKE_PTR(holderp->tag.lock))->tag.rel_id == TraceLockTable)))
    elog(DEBUG, "%s: holder(%lx) lock(%lx) tbl(%d) proc(%lx) xid(%u) hold(%d,%d,%d,%d,%d,%d,%d)=%d", where,
         MAKE_OFFSET(holderp), holderp->tag.lock, HOLDER_LOCK_METHOD(*(holderp)), holderp->tag.proc, holderp->tag.xid,
         holderp->holding[1], holderp->holding[2], holderp->holding[3], holderp->holding[4], holderp->holding[5],
         holderp->holding[6], holderp->holding[7], holderp->nholding);
}

// In Shmem or created in CreateSpinlocks().
SpinLock LockMgrLock;

// These are to simplify/speed up some bit arithmetic.
//
// XXX is a fetch from a static array really faster than a shift?
// Wouldn't bet on it...
static LockMask BitsOff[MAX_LOCK_MODES];
static LockMask BitsOn[MAX_LOCK_MODES];
static bool LockingIsDisabled;

// Map from lockmethod to the lock table structure.
static LockMethodTable* LockMethodTbl[MAX_LOCK_METHODS];
static int NumLockMethods;

// Init the lock module.  Create a private data structure for constructing conflict masks.
void init_locks() {
  int i;
  int bit;

  bit = 1;
  for (i = 0; i < MAX_LOCK_MODES; i++, bit <<= 1) {
    BitsOn[i] = bit;
    BitsOff[i] = ~bit;
  }
}

// Sets LockingIsDisabled flag to TRUE or FALSE.
void lock_disable(bool status) { LockingIsDisabled = status; }
bool locking_disabled() { return LockingIsDisabled; }

// Fetch the lock method table associated with a given lock.
LockMethodTable* get_locks_method_table(Lock* lock) {
  LockMethod lock_method = LOCK_LOCK_METHOD(*lock);
  Assert(lock_method > 0 && lock_method < NumLockMethods);
  return LockMethodTbl[lock_method];
}

// Initialize the lock table's lock type structures.
static void lock_method_init(LockMethodTable* lock_method_table, LockMask* conflictp, int* priop, int num_modes) {
  int i;

  lock_method_table->ctl->num_lock_modes = num_modes;

  // TODO(gc): why increase by 1.
  num_modes++;

  for (i = 0; i < num_modes; i++, priop++, conflictp++) {
    lock_method_table->ctl->conflict_tab[i] = *conflictp;
    lock_method_table->ctl->prio[i] = *priop;
  }
}

// Notes:
//  (a) a lock table has four separate entries in the shmem index
//  table. This is because every shared hash table and spinlock
//  has its name stored in the shmem index at its creation.  It
//  is wasteful, in this case, but not much space is involved.
//
// NOTE: data structures allocated here are allocated permanently, using
// TopMemoryContext and shared memory. We don't ever release them anyway,
// and in normal multi-backend operation the lock table structures set up
// by the postmaster are inherited by each backend, so they must be in
// TopMemoryContext.
// TODO(gc):
// 需要明白tuple跟锁之间是如何建立起关系的
LockMethod lock_method_table_init(char* tab_name, LockMask* conflicts, int* prio, int num_modes, int max_backends) {
  LockMethodTable* lock_method_table;
  char* shmem_name;
  HashCtrl info;
  int hash_flags;

  bool found;
  long init_table_size;
  long max_table_size;

  if (num_modes > MAX_LOCK_MODES) {
    elog(NOTICE, "%s: too many lock types %d greater than %d", __func__, num_modes, MAX_LOCK_MODES);

    return INVALID_LOCK_METHOD;
  }

  // Compute init/max size to request for lock hashtables.
  // 假如允许100个max_backends 那么max_table_size等于6400
  max_table_size = NLOCK_ENTS(max_backends);
  init_table_size = max_table_size / 10;

  // Allocate a string for the shmem index table lookups.
  // This is just temp space in this routine, so palloc is OK.
  shmem_name = (char*)palloc(strlen(tab_name) + 32);

  // Each lock table has a non-shared, permanent header.
  lock_method_table = (LockMethodTable*)memory_context_alloc(TopMemoryContext, sizeof(LockMethodTable));

  // Find/acquire the spinlock for the table.
  spin_acquire(LockMgrLock);

  // Allocate a control structure from shared memory or attach to it if
  // it already exists.
  sprintf(shmem_name, "%s (ctl)", tab_name);
  lock_method_table->ctl = (LockMethodCtrl*)shmem_init_struct(shmem_name, sizeof(LockMethodCtrl), &found);
}