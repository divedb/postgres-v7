//===----------------------------------------------------------------------===//
//
// heapam.h
//  POSTGRES heap access method definitions.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: heapam.h,v 1.63.2.1 2001/08/09 19:22:24 inoue Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_ACCESS_HEAP_AM_H_
#define RDBMS_ACCESS_HEAP_AM_H_

#include <time.h>

#include "rdbms/access/relscan.h"
#include "rdbms/storage/lock.h"
#include "rdbms/utils/rel.h"

typedef struct HeapAccessStatisticsData {
  time_t init_global_timestamp;   // Time global statistics started.
  time_t local_reset_timestamp;   // Last time local reset was done.
  time_t last_request_timestamp;  // Last time stats were requested.

  int global_open;
  int global_openr;
  int global_close;
  int global_begin_scan;
  int global_rescan;
  int global_end_scan;
  int global_get_next;
  int global_fetch;
  int global_insert;
  int global_delete;
  int global_replace;
  int global_mark4_update;
  int global_mark_pos;
  int global_restrpos;
  int global_buffer_get_relation;
  int global_relation_id_get_relation;
  int global_relation_id_get_relation_buf;
  int global_relation_name_get_relation;
  int global_get_rel_desc;
  int global_heap_get_tup;
  int global_relation_put_heap_tuple;
  int global_relation_put_long_heap_tuple;

  int local_open;
  int local_openr;
  int local_close;
  int local_begin_scan;
  int local_rescan;
  int local_end_scan;
  int local_get_next;
  int local_fetch;
  int local_insert;
  int local_delete;
  int local_replace;
  int local_mark4_update;
  int local_mark_pos;
  int local_restrpos;
  int local_buffer_get_relation;
  int local_relation_id_get_relation;
  int local_relation_id_get_relation_buf;
  int local_relation_name_get_relation;
  int local_get_rel_desc;
  int local_heap_get_tup;
  int local_relation_put_heap_tuple;
  int local_relation_put_long_heap_tuple;
} HeapAccessStatisticsData;

typedef HeapAccessStatisticsData* HeapAccessStatistics;

// In heapam.c
extern Relation heap_open(Oid relationId, LockMode lockmode);
extern Relation heap_openr(const char* relationName, LockMode lockmode);
extern Relation heap_open_nofail(Oid relationId);
extern Relation heap_openr_nofail(const char* relationName);
extern void heap_close(Relation relation, LockMode lockmode);
extern HeapScanDesc heap_beginscan(Relation relation, int atend, Snapshot snapshot, unsigned nkeys, ScanKey key);
extern void heap_rescan(HeapScanDesc scan, bool scanFromEnd, ScanKey key);
extern void heap_endscan(HeapScanDesc scan);
extern HeapTuple heap_getnext(HeapScanDesc scandesc, int backw);
extern void heap_fetch(Relation relation, Snapshot snapshot, HeapTuple tup, Buffer* userbuf);
extern ItemPointer heap_get_latest_tid(Relation relation, Snapshot snapshot, ItemPointer tid);
extern void setLastTid(const ItemPointer tid);
extern Oid heap_insert(Relation relation, HeapTuple tup);
extern int heap_delete(Relation relation, ItemPointer tid, ItemPointer ctid);
extern int heap_update(Relation relation, ItemPointer otid, HeapTuple tup, ItemPointer ctid);
extern int heap_mark4update(Relation relation, HeapTuple tup, Buffer* userbuf);
extern void simple_heap_delete(Relation relation, ItemPointer tid);
extern void simple_heap_update(Relation relation, ItemPointer otid, HeapTuple tup);
extern void heap_markpos(HeapScanDesc scan);
extern void heap_restrpos(HeapScanDesc scan);

extern void heap_redo(XLogRecPtr lsn, XLogRecord* rptr);
extern void heap_undo(XLogRecPtr lsn, XLogRecord* rptr);
extern void heap_desc(char* buf, uint8 xl_info, char* rec);

// In common/heaptuple.c
extern Size ComputeDataSize(TupleDesc tupleDesc, Datum* value, char* nulls);
extern void DataFill(char* data, TupleDesc tupleDesc, Datum* value, char* nulls, uint16* infomask, bits8* bit);
extern int heap_attisnull(HeapTuple tup, int attnum);
extern Datum nocachegetattr(HeapTuple tup, int attnum, TupleDesc att, bool* isnull);
extern HeapTuple heap_copytuple(HeapTuple tuple);
extern void heap_copytuple_with_tuple(HeapTuple src, HeapTuple dest);
extern HeapTuple heap_form_tuple(TupleDesc tup_desc, Datum* value, char* nulls);
extern HeapTuple heap_modifytuple(HeapTuple tuple, Relation relation, Datum* replValue, char* replNull, char* repl);
extern void heap_freetuple(HeapTuple tuple);
HeapTuple heap_addheader(uint32 natts, int structlen, char* structure);

// In common/heap/stats.c
extern void PrintHeapAccessStatistics(HeapAccessStatistics stats);
extern void initam(void);

#endif  // RDBMS_ACCESS_HEAP_AM_H_
