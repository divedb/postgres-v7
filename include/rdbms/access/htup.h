// =========================================================================
//
// htup.h
//  POSTGRES heap tuple definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: htup.h,v 1.29 2000/04/12 17:16:26 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_ACCESS_HTUP_H_
#define RDBMS_ACCESS_HTUP_H_

#include "rdbms/postgres.h"

#define MinHeapTupleBitmapSize 32  // 8 * 4

// Check these, they are likely to be more severely limited by t_hoff.
#define MaxHeapAttributeNumber 1600  // 8 * 200

// To avoid wasting space, the attributes should be layed out in such a
// way to reduce structure padding.
typedef struct HeapTupleHeaderData {
  Oid t_oid;               // OID of this tuple -- 4 bytes.
  CommandId t_cmin;        // Insert CID stamp -- 4 bytes each.
  CommandId t_cmax;        // Delete CommandId stamp.
  TransactionId t_xmin;    // Insert XID stamp -- 4 bytes.
  TransactionId t_xmax;    // Delete XID stamp.
  ItemPointerData t_ctid;  //
} HeapTupleHeaderData;

#endif  // RDBMS_ACCESS_HTUP_H_
