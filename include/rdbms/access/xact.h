// =========================================================================
//
// xact.h
//  postgres transaction system header
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: xact.h,v 1.24 2000/01/26 05:57:51 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_ACCESS_XACT_H_
#define RDBMS_ACCESS_XACT_H_

#include "rdbms/access/transam.h"

// TODO(gc): fix this.

// Transaction state structure.
typedef struct TransactionStateData {
  TransactionId transaction_id_data;
  CommandId command_id;
  CommandId scan_command_id;

} TransactionStateData;

// Xact isolation levels.
#define XACT_DIRTY_READ      0  // Not implemented.
#define XACT_READ_COMMITTED  1
#define XACT_REPEATABLE_READ 2  // Not implemented.
#define XACT_SERIALIZABLE    3

extern int DefaultXactIsoLevel;
extern int XactIsoLevel;

// Transaction block states.
#define TBLOCK_DEFAULT    0
#define TBLOCK_BEGIN      1
#define TBLOCK_INPROGRESS 2
#define TBLOCK_END        3
#define TBLOCK_ABORT      4
#define TBLOCK_ENDABORT   5

typedef TransactionStateData* TransactionState;

#define TRANSACTION_ID_IS_VALID(xid)       (xid != NullTransactionId)
#define TRANSACTION_ID_STORE(xid, dest)    (*((TransactionId*)dest) = (TransactionId)xid)
#define STORE_INVALID_TRANSACTION_ID(dest) (*((TransactionId*)dest) = NullTransactionId)

#endif  // RDBMS_ACCESS_XACT_H_
