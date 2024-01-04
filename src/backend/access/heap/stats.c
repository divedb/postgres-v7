//===----------------------------------------------------------------------===//
//
// stats.c
//  heap access method debugging statistic collection routines
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/access/heap/stats.c
//           v 1.24 2001/03/22 06:16:07 momjian Exp $
//
// NOTES
//  initam should be moved someplace else.
//
//===----------------------------------------------------------------------===//
#include <time.h>

#include "rdbms/access/heapam.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/memutils.h"

static void init_heap_access_statistics();

HeapAccessStatistics HeapAccessStats = NULL;

static void init_heap_access_statistics() {
  MemoryContext old_context;
  HeapAccessStatistics stats;

  if (HeapAccessStats != NULL) {
    return;
  }

  old_context = memory_context_switch_to(TopMemoryContext);
  stats = (HeapAccessStatistics)palloc(sizeof(HeapAccessStatisticsData));

  stats->global_open = 0;
  stats->global_openr = 0;
  stats->global_close = 0;
  stats->global_begin_scan = 0;
  stats->global_rescan = 0;
  stats->global_end_scan = 0;
  stats->global_get_next = 0;
  stats->global_fetch = 0;
  stats->global_insert = 0;
  stats->global_delete = 0;
  stats->global_replace = 0;
  stats->global_mark4_update = 0;
  stats->global_mark_pos = 0;
  stats->global_restrpos = 0;
  stats->global_buffer_get_relation = 0;
  stats->global_relation_id_get_relation = 0;
  stats->global_relation_id_get_relation_buf = 0;
  stats->global_get_rel_desc = 0;
  stats->global_heap_get_tup = 0;
  stats->global_relation_put_heap_tuple = 0;
  stats->global_relation_put_long_heap_tuple = 0;

  stats->local_open = 0;
  stats->local_openr = 0;
  stats->local_close = 0;
  stats->local_begin_scan = 0;
  stats->local_rescan = 0;
  stats->local_end_scan = 0;
  stats->local_get_next = 0;
  stats->local_fetch = 0;
  stats->local_insert = 0;
  stats->local_delete = 0;
  stats->local_replace = 0;
  stats->local_mark4_update = 0;
  stats->local_mark_pos = 0;
  stats->local_restrpos = 0;
  stats->local_buffer_get_relation = 0;
  stats->local_relation_id_get_relation = 0;
  stats->local_relation_id_get_relation_buf = 0;
  stats->local_get_rel_desc = 0;
  stats->local_heap_get_tup = 0;
  stats->local_relation_put_heap_tuple = 0;
  stats->local_relation_put_long_heap_tuple = 0;
  stats->local_relation_name_get_relation = 0;
  stats->global_relation_name_get_relation = 0;

  time(&stats->init_global_timestamp);
  time(&stats->local_reset_timestamp);
  time(&stats->last_request_timestamp);

  memory_context_switch_to(old_context);

  HeapAccessStats = stats;
}

void init_am() { init_heap_access_statistics(); }

void print_heap_access_statistics(HeapAccessStatistics stats) {
  /*
   * return nothing if stats aren't valid
   */
  if (stats == NULL) return;

  printf("======== heap am statistics ========\n");
  printf("init_global_timestamp:      %s",
         ctime(&(stats->init_global_timestamp)));
  printf("local_reset_timestamp:      %s",
         ctime(&(stats->local_reset_timestamp)));
  printf("last_request_timestamp:     %s",
         ctime(&(stats->last_request_timestamp)));
  printf("local/global_open:                        %6d/%6d\n",
         stats->local_open, stats->global_open);
  printf("local/global_openr:                       %6d/%6d\n",
         stats->local_openr, stats->global_openr);
  printf("local/global_close:                       %6d/%6d\n",
         stats->local_close, stats->global_close);
  printf("local/global_beginscan:                   %6d/%6d\n",
         stats->local_begin_scan, stats->global_begin_scan);
  printf("local/global_rescan:                      %6d/%6d\n",
         stats->local_rescan, stats->global_rescan);
  printf("local/global_endscan:                     %6d/%6d\n",
         stats->local_end_scan, stats->global_end_scan);
  printf("local/global_getnext:                     %6d/%6d\n",
         stats->local_get_next, stats->global_get_next);
  printf("local/global_fetch:                       %6d/%6d\n",
         stats->local_fetch, stats->global_fetch);
  printf("local/global_insert:                      %6d/%6d\n",
         stats->local_insert, stats->global_insert);
  printf("local/global_delete:                      %6d/%6d\n",
         stats->local_delete, stats->global_delete);
  printf("local/global_replace:                     %6d/%6d\n",
         stats->local_replace, stats->global_replace);
  printf("local/global_mark4update:                     %6d/%6d\n",
         stats->local_mark4_update, stats->global_mark4_update);
  printf("local/global_markpos:                     %6d/%6d\n",
         stats->local_mark_pos, stats->global_mark_pos);
  printf("local/global_restrpos:                    %6d/%6d\n",
         stats->local_restrpos, stats->global_restrpos);
  printf("================\n");
  printf("local/global_BufferGetRelation:             %6d/%6d\n",
         stats->local_buffer_get_relation, stats->global_buffer_get_relation);
  printf("local/global_RelationIdGetRelation:         %6d/%6d\n",
         stats->local_relation_id_get_relation,
         stats->global_relation_id_get_relation);
  printf("local/global_RelationIdGetRelation_Buf:     %6d/%6d\n",
         stats->local_relation_id_get_relation_buf,
         stats->global_relation_id_get_relation_buf);
  printf("local/global_getreldesc:                    %6d/%6d\n",
         stats->local_get_rel_desc, stats->global_get_rel_desc);
  printf("local/global_heapgettup:                    %6d/%6d\n",
         stats->local_heap_get_tup, stats->global_heap_get_tup);
  printf("local/global_RelationPutHeapTuple:          %6d/%6d\n",
         stats->local_relation_put_heap_tuple,
         stats->global_relation_put_heap_tuple);
  printf("local/global_RelationPutLongHeapTuple:      %6d/%6d\n",
         stats->local_relation_put_long_heap_tuple,
         stats->global_relation_put_long_heap_tuple);
  printf("===================================\n");
  printf("\n");
}