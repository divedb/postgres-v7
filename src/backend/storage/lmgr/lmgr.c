//===----------------------------------------------------------------------===//
//
// lmgr.c
//  POSTGRES lock manager code
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/storage/lmgr/lmgr.c
//           v 1.45 2001/03/22 03:59:46 momjian Exp $
//
//===----------------------------------------------------------------------===//

#include "rdbms/storage/lmgr.h"

#include "rdbms/access/xact.h"
#include "rdbms/catalog/catalog.h"
#include "rdbms/postgres.h"

static LockMask LockConflicts = {
    (int)NULL,
    // AccessShareLock
    (1 << ACCESS_EXCLUSIVE_LOCK),
    // RowShareLock
    (1 << ACCESS_EXCLUSIVE_LOCK) | (1 << EXCLUSIVE_LOCK),
    // RowExclusiveLock
    (1 << ACCESS_EXCLUSIVE_LOCK) | (1 << EXCLUSIVE_LOCK) | (1 << SHARE_ROW_EXCLUSIVE_LOCK) | (1 << SHARE_LOCK),
    // ShareLock
    (1 << ACCESS_EXCLUSIVE_LOCK) | (1 << EXCLUSIVE_LOCK) | (1 << SHARE_ROW_EXCLUSIVE_LOCK) | (1 << ROW_EXCLUSIVE_LOCK),
    // ShareRowExclusiveLock
    (1 << ACCESS_EXCLUSIVE_LOCK) | (1 << EXCLUSIVE_LOCK) | (1 << SHARE_ROW_EXCLUSIVE_LOCK) | (1 << SHARE_LOCK) |
        (1 << ROW_EXCLUSIVE_LOCK),
    // ExclusiveLock
    (1 << ACCESS_EXCLUSIVE_LOCK) | (1 << EXCLUSIVE_LOCK) | (1 << SHARE_ROW_EXCLUSIVE_LOCK) | (1 << SHARE_LOCK) |
        (1 << ROW_EXCLUSIVE_LOCK) | (1 << ROW_SHARE_LOCK),
    // AccessExclusiveLock
    (1 << ACCESS_EXCLUSIVE_LOCK) | (1 << EXCLUSIVE_LOCK) | (1 << SHARE_ROW_EXCLUSIVE_LOCK) | (1 << SHARE_LOCK) |
        (1 << ROW_EXCLUSIVE_LOCK) | (1 << ROW_SHARE_LOCK) | (1 << ACCESS_SHARE_LOCK)};

static int LockPrios[] = {(int)NULL, 1, 2, 3, 4, 5, 6, 7};

LockMethod LockTableId = (LockMethod)NULL;
LockMethod LongTermTableId = (LockMethod)NULL;

// Create the lock table described by LockConflicts and LockPrios.
LockMethod init_lock_table(int max_backends) {
  int lock_method;

  lock_method = lock_method_table_init("LockTable", LockConflicts, LockPrios, MAX_LOCK_MODES, max_backends);
}