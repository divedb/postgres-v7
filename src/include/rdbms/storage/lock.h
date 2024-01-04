//===----------------------------------------------------------------------===//
//
// lock.h
//  POSTGRES low-level lock mechanism
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: lock.h,v 1.37 2000/04/12 17:16:51 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_STORAGE_LOCK_H_
#define RDBMS_STORAGE_LOCK_H_

#include "rdbms/storage/ipc.h"
#include "rdbms/storage/itemptr.h"
#include "rdbms/storage/shmem.h"

typedef struct ProcQueue {
  ShmemQueue links;  // Head of list of PROC objects
  int size;          // Number of entries in list
} ProcQueue;

// struct proc is declared in storage/proc.h, but must forward-reference it.
typedef struct Proc Proc;

extern SpinLock LockMgrLock;

#ifdef LOCK_DEBUG

extern int TraceLockOidMin;
extern bool TraceLocks;
extern bool TraceUserLocks;
extern int TraceLockTable;
extern bool DebugDeadLocks;

#endif  // LOCK_DEBUG

// The following defines are used to estimate how much shared
// memory the lock manager is going to require.
// See LockShmemSize() in lock.c.
//
// NLOCKS_PER_XACT  - The number of unique objects locked in a transaction
//                    (this should be configurable!)
// NLOCKENTS        - The maximum number of lock entries in the lock table.
#define NLOCKS_PER_XACT          64
#define NLOCK_ENTS(max_backends) (NLOCKS_PER_XACT * (max_backends))

typedef int LockMask;
typedef int LockMode;
typedef int LockMethod;

// MAX_LOCKMODES cannot be larger than the # of bits in LOCKMASK.
#define MAX_LOCK_MODES 8

// MAX_LOCK_METHODS corresponds to the number of spin locks allocated in
// CreateSpinLocks() or the number of shared memory locations allocated
// for lock table spin locks in the case of machines with TAS instructions.
#define MAX_LOCK_METHODS    3
#define INVALID_TABLE_ID    0
#define INVALID_LOCK_METHOD INVALID_TABLE_ID
#define DEFAULT_LOCK_METHOD 1
#define USER_LOCK_METHOD    2
#define MIN_LOCK_METHOD     DEFAULT_LOCK_METHOD

// There is normally only one lock method, the default one.
// If user locks are enabled, an additional lock method is present
//
// LOCKMETHODCTL and LOCKMETHODTABLE are split because the first lives
// in shared memory.  This is because it contains a spinlock.
// LOCKMETHODTABLE exists in private memory.  Both are created by the
// postmaster and should be the same in all backends

// This is the control structure for a lock table. It
// lives in shared memory. This information is the same
// for all backends.
//
// lockmethod   -- the handle used by the lock table's clients to
//                 refer to the type of lock table being used.
//
// numLockModes -- number of lock types (READ,WRITE,etc) that
//                 are defined on this lock table
//
// conflictTab  -- this is an array of bitmasks showing lock
//                 type conflicts. conflictTab[i] is a mask with the j-th bit
//                 turned on if lock types i and j conflict.
//
// prio         -- each lockmode has a priority, so, for example, waiting
//                 writers can be given priority over readers (to avoid
//                 starvation).
//
// masterlock   -- synchronizes access to the table
typedef struct LockMethodCtrl {
  LockMethod lock_method;
  int num_lock_modes;
  int conflict_tab[MAX_LOCK_MODES];
  int prio[MAX_LOCK_MODES];
  SpinLock master_lock;
} LockMethodCtrl;

// Eack backend has a non-shared lock table header.
//
// lock_hash        -- hash table holding per-locked-object lock information
// holder_hash      -- hash table holding per-lock-holder lock information
// ctl              -- shared control structure described above.
typedef struct LockMethodTable {
  HashTable* lock_hash;
  HashTable* holder_hash;
  LockMethodCtrl* ctl;
} LockMethodTable;

// LockTag is the key information needed to look up a LOCK item in the* lock
// hashtable. A LockTag value uniquely identifies a lockable object.
typedef struct LockTag {
  Oid rel_id;
  Oid db_id;

  union {
    BlockNumber blk_no;
    TransactionId xid;
  } obj_id;

  // offnum should be part of objId.tupleId above, but would increase
  // sizeof(LOCKTAG) and so moved here; currently used by userlocks
  // only.
  OffsetNumber off_num;
  uint16 lock_method;  // Needed by userlocks
} LockTag;

// Per-locked-object lock information:
//
// tag          -- uniquely identifies the object being locked
// grant_mask   -- bitmask for all lock types currently granted on this object.
// wait_mask    -- bitmask for all lock types currently awaited on this object.
// lock_holders -- list of HOLDER objects for this lock.
// wait_procs   -- queue of processes waiting for this lock.
// requested    -- count of each lock type currently requested on the lock
//                 (includes requests already granted!!).
// nrequested   -- total requested locks of all types.
// granted      -- count of each lock type currently granted on the lock.
// ngranted     -- total granted locks of all types.
typedef struct Lock {
  LockTag tag;                    // Unique identifier of lockable object
  int grant_mask;                 // Bitmask for lock types already granted
  int wait_mask;                  // Bitmask for lock types awaited
  ShmemQueue lock_holders;        // List of HOLDER objects assoc. with lock
  ProcQueue wait_procs;           // List of PROC objects waiting on lock
  int requested[MAX_LOCK_MODES];  // Counts of requested locks
  int nrequested;                 // Total of requested[] array
  int granted[MAX_LOCK_MODES];    // Counts of granted locks
  int ngranted;                   // Total of granted[] array
} Lock;

#define SHMEM_LOCK_TAB_KEY_SIZE  sizeof(LockTag)
#define SHMEM_LOCK_TAB_DATA_SIZE (sizeof(Lock) - SHMEM_LOCKTAB_KEY_SIZE)
#define LOCK_LOCK_METHOD(lock)   ((lock).tag.lock_method)

// We may have several different transactions holding or awaiting locks
// on the same lockable object.  We need to store some per-holder information
// for each such holder (or would-be holder).
//
// HOLDERTAG is the key information needed to look up a HOLDER item in the
// holder hashtable.  A HOLDERTAG value uniquely identifies a lock holder.
//
// There are two possible kinds of holder tags: a transaction (identified
// both by the PROC of the backend running it, and the xact's own ID) and
// a session (identified by backend PROC, with xid = InvalidTransactionId).
//
// Currently, session holders are used for user locks and for cross-xact
// locks obtained for VACUUM.  We assume that a session lock never conflicts
// with per-transaction locks obtained by the same backend.
//
// The holding[] array counts the granted locks (of each type) represented
// by this holder. Note that there will be a holder object, possibly with
// zero holding[], for any lock that the process is currently waiting on.
// Otherwise, holder objects whose counts have gone to zero are recycled
// as soon as convenient.
//
// Each HOLDER object is linked into lists for both the associated LOCK object
// and the owning PROC object. Note that the HOLDER is entered into these
// lists as soon as it is created, even if no lock has yet been granted.
// A PROC that is waiting for a lock to be granted will also be linked into
// the lock's waitProcs queue.
typedef struct HolderTag {
  ShmemOffset lock;   // Link to per-lockable-object information
  ShmemOffset proc;   // Link to PROC of owning backend
  TransactionId xid;  // xact ID, or InvalidTransactionId
} HolderTag;

typedef struct Holder {
  HolderTag tag;  // Unique identifier of holder object

  int holding[MAX_LOCK_MODES];  // Count of locks currently held
  int nholding;                 // Total of holding[] array
  ShmemQueue lock_link;         // List link for lock's list of holders
  ShmemQueue proc_link;         // List link for process's list of holders
} Holder;

#define SHMEM_HOLDER_TAB_KEY_SIZE  sizeof(HolderTag)
#define SHMEM_HOLDER_TAB_DATA_SIZE (sizeof(Holder) - SHMEM_HOLDER_TAB_KEY_SIZE)

#define HOLDER_LOCK_METHOD(holder) (((Lock*)MAKE_PTR((holder).tag.lock))->tag.lock_method)

#define LOCK_LOCK_TABLE()   spin_acquire(LockMgrLock)
#define UNLOCK_LOCK_TABLE() spin_release(LockMgrLock)

void init_locks();
void lock_disable(bool status);
bool locking_disabled();
LockMethodTable* get_locks_method_table(Lock* lock);
LockMethod lock_method_table_init(char* tab_name, LockMask* conflictsp, int* priop, int num_modes, int max_backends);
LockMethod lock_method_table_rename(LockMethod lock_method);
bool lock_acquire(LockMethod lock_method, LockTag* lock_tag, TransactionId xid, LockMode lock_mode);
bool lock_release(LockMethod lock_method, LockTag* lock_tag, TransactionId xid, LockMode lock_mode);
bool lock_release_all(LockMethod lock_method, Proc* proc, bool all_xids, TransactionId xid);
int lock_check_conflicts(LockMethodTable* lock_method_table, LockMode lock_mode, Lock* lock, Holder* holder, Proc* proc,
                         int* my_holding);
void grant_lock(Lock* lock, Holder* holder, LockMode lock_mode);
void remove_from_wait_queue(Proc* proc);
int lock_shmem_size(int max_backends);
bool deadlock_check(Proc* proc);
void init_deadlock_checking();

#ifdef LOCK_DEBUG

void dump_locks();
void dump_all_locks();

#endif  // LOCK_DEBUG

#endif  // RDBMS_STORAGE_LOCK_H_
