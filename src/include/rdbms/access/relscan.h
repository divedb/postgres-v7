//===----------------------------------------------------------------------===//
//
// relscan.h
//  POSTGRES internal relation scan descriptor definitions.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: relscan.h,v 1.20 2001/01/24 19:43:19 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_ACCESS_REL_SCAN_H_
#define RDBMS_ACCESS_REL_SCAN_H_

#include "rdbms/storage/buf.h"
#include "rdbms/utils/tqual.h"

typedef ItemPointerData MarkData;

typedef struct HeapScanDescData {
  Relation rs_rd;            // Pointer to relation descriptor.
  HeapTupleData rs_ptup;     // Previous tuple in scan.
  HeapTupleData rs_ctup;     // Current tuple in scan.
  HeapTupleData rs_ntup;     // Next tuple in scan.
  Buffer rs_pbuf;            // Previous buffer in scan.
  Buffer rs_cbuf;            // Current buffer in scan.
  Buffer rs_nbuf;            // Next buffer in scan.
  ItemPointerData rs_mptid;  // Marked previous tid.
  ItemPointerData rs_mctid;  // Marked current tid.
  ItemPointerData rs_mntid;  // Marked next tid.
  ItemPointerData rs_mcd;    // Marked current delta XXX ???.
  Snapshot rs_snapshot;      // Snapshot to see.
  bool rs_atend;             // Restart scan at end?.
  uint16 rs_cdelta;          // Current delta in chain.
  uint16 rs_nkeys;           // Number of attributes in keys.
  ScanKey rs_key;            // Key descriptors.
} HeapScanDescData;

typedef HeapScanDescData* HeapScanDesc;

#endif  // RDBMS_ACCESS_REL_SCAN_H_
