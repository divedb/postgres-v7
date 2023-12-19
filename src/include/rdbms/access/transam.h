// =========================================================================
//
// transam.h
//  postgres transaction access method support code header
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: transam.h,v 1.24 2000/01/26 05:57:51 momjian Exp $
//
// NOTES
//  Transaction System Version 101 now support proper oid
//  generation and recording in the variable relation.
//
// =========================================================================

#ifndef RDBMS_ACCESS_TRANSAM_H_
#define RDBMS_ACCESS_TRANSAM_H_

#include "rdbms/postgres.h"
#include "rdbms/storage/block.h"
#include "rdbms/utils/rel.h"

// Transaction system version id
//
//  This is stored on the first page of the log, time and variable
// relations on the first 4 bytes.  This is so that if we improve
// the format of the transaction log after postgres version 2, then
// people won't have to rebuild their databases.
//
//  TRANS_SYSTEM_VERSION 100 means major version 1 minor version 0.
// Two databases with the same major version should be compatible,
// even if their minor versions differ.
#define TRANS_SYSTEM_VERSION 200

// Transaction id status values
//
//  Someday we will use "11" = 3 = XID_COMMIT_CHILD to mean the
// commiting of child xactions.
#define XID_COMMIT       2  // Transaction commited.
#define XID_ABORT        1  // Transaction aborted.
#define XID_INPROGRESS   0  // Transaction in progress.
#define XID_COMMIT_CHILD 3  // Child xact commited.

typedef unsigned char XidStatus;  // (2 bits).

// Note: we reserve the first 16384 object ids for internal use.
// oid's less than this appear in the .bki files.  the choice of
// 16384 is completely arbitrary.
#define BootstrapObjectIdData 16384

// BitIndexOf computes the index of the Nth xid on a given block
#define BIT_INDEX_OF(n) ((n)*2)

// Transaction page definitions.
#define TP_DATA_SIZE                BLCKSZ
#define TP_NUM_XID_STATUS_PER_BLOCK (TP_DATA_SIZE * 4)

// LogRelationContents structure
//
//  This structure describes the storage of the data in the
// first 128 bytes of the log relation.  This storage is never
// used for transaction status because transaction id's begin
// their numbering at 512.
//
//  The first 4 bytes of this relation store the version
// number of the transction system.
typedef struct LogRelationContentsData {
  int trans_system_version;
} LogRelationContentsData;

typedef LogRelationContentsData* LogRelationContents;

// VariableRelationContents structure
//
//  The variable relation is a special "relation" which
// is used to store various system "variables" persistantly.
// Unlike other relations in the system, this relation
// is updated in place whenever the variables change.
//
//  The first 4 bytes of this relation store the version
// number of the transction system.
//
//  Currently, the relation has only one page and the next
// available xid, the last committed xid and the next
// available oid are stored there.
typedef struct VariableRelationContentsData {
  int trans_system_version;
  TransactionId next_xid_data;
  TransactionId last_xid_data;  // unused.
  Oid next_oid;
} VariableRelationContentsData;

typedef VariableRelationContentsData* VariableRelationContents;

// VariableCache is placed in shmem and used by backends to
// get next available XID & OID without access to
// variable relation. Actually, I would like to have two
// different on-disk storages for next XID and OID...
// But hoping that someday we will use per database OID
// generator I leaved this as is.	- vadim 07/21/98
typedef struct VariableCacheData {
  uint32 xid_count;
  TransactionId next_xid;
  uint32 oid_count;  // Not implemented, yet.
  Oid next_oid;
} VariableCacheData;

typedef VariableCacheData* VariableCache;

// transam/transam.c
void initialize_transaction_log();
bool transaction_id_did_commit(TransactionId xid);
bool transaction_id_did_abort(TransactionId xid);
void transaction_id_commit(TransactionId xid);
void transaction_id_abort(TransactionId xid);

// transam/transsup.c
void ami_transaction_override(bool flag);
void trans_compute_block_number(Relation relation, TransactionId xid, BlockNumber* block_number_outp);
XidStatus trans_block_number_get_xid_status(Relation relation, BlockNumber block_number, TransactionId xid,
                                            bool* failp);
void trans_block_number_set_xid_status(Relation relation, BlockNumber block_number, TransactionId xid,
                                       XidStatus xstatus, bool* failp);

// transam/varsup.c
void variable_relation_put_next_xid(TransactionId xid);
void get_next_transaction_id(TransactionId* xid);
void read_new_transaction_id(Oid* oid_return);
void get_new_object_id(Oid* oid_return);
void check_max_object_id(Oid assigned_oid);

// Global variable extern declarations.

// transam.c
extern Relation LogRelation;
extern Relation VariableRelation;

extern TransactionId CachedTestXid;
extern XidStatus CachedTestXidStatus;

extern TransactionId NullTransactionId;
extern TransactionId AmiTransactionId;
extern TransactionId FirstTransactionId;

extern int RecoveryCheckingEnableState;

// transsup.c
extern bool AmiOverride;

// varsup.c
extern SpinLock OidGenLockId;
extern SpinLock XidGenLockId;
extern VariableCache ShmemVariableCache;

#endif  // RDBMS_ACCESS_TRANSAM_H_
