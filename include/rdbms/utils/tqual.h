//===----------------------------------------------------------------------===//
//
// tqual.h
//  POSTGRES "time" qualification definitions.
//
//  Should be moved/renamed... - vadim 07/28/98
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: tqual.h,v 1.30 2001/01/24 19:43:29 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_RQUAL_H_
#define RDBMS_UTILS_RQUAL_H_

#include "rdbms/access/htup.h"
#include "rdbms/access/xact.h"

// TODO(gc):

typedef struct SnapshotData {
  TransactionId xmin;   // XID < xmin are visible to me.
  TransactionId xmax;   // XID >= xmax are invisible to me.
  uint32 xcnt;          // # of xact below.
  TransactionId* xip;   // Array of xacts in progress.
  ItemPointerData tid;  // Required for Dirty snapshot -:(.
} SnapshotData;

typedef SnapshotData* Snapshot;

#define SNAPSHOT_NOW  ((Snapshot)0x0)
#define SNAPSHOT_SELF ((Snapshot)0x1)
#define SNAPSHOT_ANY  ((Snapshot)0x2)
#endif  // RDBMS_UTILS_RQUAL_H_
