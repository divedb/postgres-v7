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
#include "rdbms/access/tupmacs.h"
#include "rdbms/access/xlogdefs.h"
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

#define INCR_HEAP_ACCESS_STAT(x) \
  (HeapAccessStats == NULL ? 0 : (HeapAccessStats->x)++)

// Fetch a user attribute's value as a Datum (might be either a
// value, or a pointer into the data area of the tuple).
//
// This must not be used when a system attribute might be
// requested. Furthermore, the passed attnum MUST be valid.  Use heap_getattr()
// instead, if in doubt.
//
// This gets called many times, so we macro the cacheable and NULL
// lookups, and call nocachegetattr() for the rest.

extern Datum no_cache_get_attr(HeapTuple tup, int attnum, TupleDesc att,
                               bool* isnull);

#if !defined(DISABLE_COMPLEX_MACRO)

#define FAST_GET_ATTR(tup, attnum, tuple_desc, isnull)                        \
  (ASSERT_MACRO((attnum) > 0),                                                \
   ((isnull) ? (*(isnull) = false) : (DUMMY_RET)NULL),                        \
   HEAP_TUPLE_NO_NULLS(tup)                                                   \
       ? ((tuple_desc)->attrs[(attnum)-1]->attcacheoff >= 0                   \
              ? (FETCH_ATT((tuple_desc)->attrs[(attnum)-1],                   \
                           (char*)(tup)->t_data + (tup)->t_data->t_hoff +     \
                               (tuple_desc)->attrs[(attnum)-1]->attcacheoff)) \
              : no_cache_get_attr((tup), (attnum), (tuple_desc), (isnull)))   \
       : (ATT_IS_NULL((attnum)-1, (tup)->t_data->t_bits)                      \
              ? (((isnull) ? (*(isnull) = true) : (DUMMY_RET)NULL),           \
                 (Datum)NULL)                                                 \
              : (no_cache_get_attr((tup), (attnum), (tuple_desc), (isnull)))))
#endif

// Extract an attribute of a heap tuple and return it as a Datum.
// This works for either system or user attributes.  The given
// attnum is properly range-checked.
//
// If the field in question has a NULL value, we return a zero
// Datum and set *isnull == true.  Otherwise, we set *isnull == false.
//
// <tup> is the pointer to the heap tuple.  <attnum> is the
// attribute number of the column (field) caller wants. <tupleDesc> is a pointer
// to the structure describing the row and all its fields.

#define HEAP_GET_ATTR(tup, attnum, tupleDesc, isnull)                   \
  (ASSERT_MACRO((tup) != NULL),                                         \
   (((attnum) > 0)                                                      \
        ? (((attnum) > (int)(tup)->t_data->t_natts)                     \
               ? (((isnull) ? (*(isnull) = true) : (dummyret)NULL),     \
                  (Datum)NULL)                                          \
               : FAST_GET_ATTR((tup), (attnum), (tupleDesc), (isnull))) \
        : heap_get_sys_attr((tup), (attnum), (isnull))))

Datum heap_get_sys_attr(HeapTuple tup, int attnum, bool* isnull);

extern HeapAccessStatistics HeapAccessStats;

// function prototypes for heap access method
//
// heap_create, heap_create_with_catalog, and heap_drop_with_catalog
// are declared in catalog/heap.h

// heapam.c
Relation heap_open(Oid relationId, LockMode lockmode);
Relation heap_openr(const char* relationName, LockMode lockmode);
Relation heap_open_no_fail(Oid relationId);
Relation heap_openr_no_fail(const char* relationName);
void heap_close(Relation relation, LockMode lockmode);
HeapScanDesc heap_begin_scan(Relation relation, int atend, Snapshot snapshot,
                             unsigned nkeys, ScanKey key);
void heap_rescan(HeapScanDesc scan, bool scan_from_end, ScanKey key);
void heap_end_scan(HeapScanDesc scan);
HeapTuple heap_get_next(HeapScanDesc scandesc, int backw);
void heap_fetch(Relation relation, Snapshot snapshot, HeapTuple tup,
                Buffer* user_buf);
ItemPointer heap_get_latest_tid(Relation relation, Snapshot snapshot,
                                ItemPointer tid);
void set_last_tid(const ItemPointer tid);
Oid heap_insert(Relation relation, HeapTuple tup);
int heap_delete(Relation relation, ItemPointer tid, ItemPointer ctid);
int heap_update(Relation relation, ItemPointer otid, HeapTuple tup,
                ItemPointer ctid);
int heap_mark4_update(Relation relation, HeapTuple tup, Buffer* userbuf);
void simple_heap_delete(Relation relation, ItemPointer tid);
void simple_heap_update(Relation relation, ItemPointer otid, HeapTuple tup);
void heap_mark_pos(HeapScanDesc scan);
void heap_restrpos(HeapScanDesc scan);
void heap_redo(XLogRecPtr lsn, XLogRecord* rptr);
void heap_undo(XLogRecPtr lsn, XLogRecord* rptr);
void heap_desc(char* buf, uint8 xl_info, char* rec);

// common/heaptuple.c
Size compute_data_size(TupleDesc tuple_desc, Datum* value, char* nulls);
void data_fill(char* data, TupleDesc tuple_desc, Datum* value, char* nulls,
               uint16* infomask, bits8* bit);
int heap_att_is_null(HeapTuple tup, int attnum);
Datum no_cache_get_attr(HeapTuple tup, int attnum, TupleDesc att, bool* isnull);
HeapTuple heap_copy_tuple(HeapTuple tuple);
void heap_copy_tuple_with_tuple(HeapTuple src, HeapTuple dest);
HeapTuple heap_form_tuple(TupleDesc tup_desc, Datum* value, char* nulls);
HeapTuple heap_modify_tuple(HeapTuple tuple, Relation relation,
                            Datum* repl_value, char* repl_null, char* repl);
void heap_free_tuple(HeapTuple tuple);
HeapTuple heap_add_header(uint32 natts, int struct_len, char* structure);

// common/heap/stats.c
void print_heap_access_statistics(HeapAccessStatistics stats);
void init_am(void);

#endif  // RDBMS_ACCESS_HEAP_AM_H_
