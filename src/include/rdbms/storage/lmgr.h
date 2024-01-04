//===----------------------------------------------------------------------===//
//
// lmgr.h
//  POSTGRES lock manager definitions.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: lmgr.h,v 1.30 2001/03/22 04:01:07 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_STORAGE_LMGR_H_
#define RDBMS_STORAGE_LMGR_H_

#include "rdbms/storage/lock.h"
#include "rdbms/utils/rel.h"

// AccessShareLock
//  A read-lock mode acquired automatically on tables being queried.
//  Conflicts with AccessExclusiveLock only.
// RowShareLock
//  Acquired by SELECT FOR UPDATE and LOCK TABLE for IN ROW SHARE MODE statements.
//  Conflicts with ExclusiveLock and AccessExclusiveLock modes.

// NoLock is not a lock mode, but a flag value meaning "don't get a lock".
#define NO_LOCK                  0
#define ACCESS_SHARE_LOCK        1  // SELECT
#define ROW_SHARE_LOCK           2  // SELECT FOR UPDATE
#define ROW_EXCLUSIVE_LOCK       3  // INSERT, UPDATE, DELETE
#define SHARE_LOCK               4  // CREATE INDEX
#define SHARE_ROW_EXCLUSIVE_LOCK 5  // Like EXCLUSIVE MODE, allows SHARE ROW
#define EXCLUSIVE_LOCK           6  // Blocks ROW SHARE/SELECT...FOR UPDATE
#define ACCESS_EXCLUSIVE_LOCK    7  // ALTER TABLE, DROP TABLE, VACUUM, and unqualified LOCK TABLE

extern LockMethod LockTableId;

LockMethod init_lock_table(int max_backends);

#endif  // RDBMS_STORAGE_LMGR_H_
