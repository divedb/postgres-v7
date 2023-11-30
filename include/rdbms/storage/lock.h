//===----------------------------------------------------------------------===//
//
// lock.h
//
//
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: lock.h,v 1.37 2000/04/12 17:16:51 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_STORAGE_LOCK_H_
#define RDBMS_STORAGE_LOCK_H_

#include "rdbms/postgres.h"
#include "rdbms/storage/block.h"
#include "rdbms/storage/off.h"
#include "rdbms/storage/spin.h"
#include "rdbms/utils/hsearch.h"

extern SpinLock LockMgrLock;

typedef int LockMask;
typedef int LockMode;
typedef int LockMethod;

#define INIT_TABLE_SIZE 100
#define MAX_TABLE_SIZE  1000

// The following defines are used to estimate how much shared
// memory the lock manager is going to require.
// See LockShmemSize() in lock.c.
//
// NLOCKS_PER_XACT - The number of unique locks acquired in a transaction
//                   (should be configurable!)
// NLOCKENTS - The maximum number of lock entries in the lock table.
#define NLOCKS_PER_XACT          64
#define NLOCK_ENTS(max_backends) (NLOCKS_PER_XACT * (max_backends))

// MAX_LOCKMODES cannot be larger than the bits in LOCKMASK.
#define MAX_LOCK_MODES 8

// MAX_LOCK_METHODS corresponds to the number of spin locks allocated in
// CreateSpinLocks() or the number of shared memory locations allocated
// for lock table spin locks in the case of machines with TAS instructions.
#define MAX_LOCK_METHODS 3

#define INVALID_TABLE_ID    0
#define INVALID_LOCK_METHOD INVALID_TABLE_ID
#define DEFAULT_LOCK_METHOD 1
#define USER_LOCK_METHOD    2
#define MIN_LOCK_METHOD     DEFAULT_LOCK_METHOD

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
  uint16 lock_method;  // Needed by userlocks.
} LockTag;

#define TAG_SIZE                       (sizeof(LockTag))
#define LOCK_TAG_LOCK_METHOD(lock_tag) ((lock_tag).lock_method)

// This is the control structure for a lock table.	It
// lives in shared memory:
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
  int num_lock_methods;
  int conflict_tab[MAX_LOCK_MODES];
  int prio[MAX_LOCK_MODES];
  SpinLock master_lock;
} LockMethodCtrl;

// lockHash -- hash table on lock Ids,
// xidHash  -- hash on xid and lockId in case
//             multiple processes are holding the lock
// ctl      -- control structure described above.
typedef struct LockMethodTable {
  HTAB* lock_hash;
  HTAB* xid_hash;
  LockMethodCtrl* ctl;
} LockMethodTable;

// A transaction never conflicts with its own locks.  Hence, if
// multiple transactions hold non-conflicting locks on the same
// data, private per-transaction information must be stored in the
// XID table.  The tag is XID + shared memory lock address so that
// all locks can use the same XID table.  The private information
// we store is the number of locks of each type (holders) and the
// total number of locks (nHolding) held by the transaction.
//
// NOTE:
// There were some problems with the fact that currently TransactionIdData
// is a 5 byte entity and compilers long word aligning of structure fields.
// If the 3 byte padding is put in front of the actual xid data then the
// hash function (which uses XID_TAGSIZE when deciding how many bytes of a
// struct to look at for the key) might only see the last two bytes of the xid.
//
// Clearly this is not good since its likely that these bytes will be the
// same for many transactions and hence they will share the same entry in
// hash table causing the entry to be corrupted.  For this long-winded
// reason I have put the tag in a struct of its own to ensure that the
// XID_TAGSIZE is computed correctly.  It used to be sizeof (SHMEM_OFFSET) +
// sizeof(TransactionIdData) which != sizeof(XIDTAG).
//
// Finally since the hash function will now look at all 12 bytes of the tag
// the padding bytes MUST be zero'd before use in hash_search() as they
// will have random values otherwise.  Jeff 22 July 1991.

#endif  // RDBMS_STORAGE_LOCK_H_
