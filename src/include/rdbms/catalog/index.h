//===----------------------------------------------------------------------===//
//
// index.h
//  prototypes for index.c.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: index.h,v 1.33 2001/03/22 04:00:35 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_CATALOG_INDEX_H_
#define RDBMS_CATALOG_INDEX_H_

#include "rdbms/access/itup.h"
#include "rdbms/nodes/execnodes.h"

extern Form_pg_am access_method_object_id_get_form(Oid access_method_object_id, MemoryContext result_cxt);
extern void update_index_predicate(Oid index_oid, Node* old_pred, Node* predicate);
extern void init_index_strategy(int num_atts, Relation index_relation, Oid access_method_object_id);
extern void index_create(char* heap_relation_name, char* index_relation_name, IndexInfo* index_info,
                         Oid access_method_object_id, Oid* class_object_id, bool is_lossy, bool primary,
                         bool allow_system_table_mods);
extern void index_drop(Oid index_id);

extern IndexInfo* build_index_info(HeapTuple index_tuple);
extern void form_index_datum(IndexInfo* index_info, HeapTuple heap_tuple, TupleDesc heap_descriptor,
                             MemoryContext result_cxt, Datum* datum, char* nullv);
extern void update_stats(Oid rel_id, long rel_tuples);
extern bool indexes_are_active(Oid rel_id, bool comfirm_committed);
extern void set_rel_has_index(Oid rel_id, bool has_index);

#ifndef OLD_FILE_NAMING

extern void set_new_rel_file_node(Relation relation);

#endif  // OLD_FILE_NAMING

extern bool set_reindex_processing(bool processing);
extern bool is_reindex_processing(void);

extern void index_build(Relation heap_relation, Relation index_relation, IndexInfo* index_info, Node* old_pred);
extern bool reindex_index(Oid index_id, bool force, bool inplace);
extern bool activate_indexes_of_a_table(Oid rel_id, bool activate);
extern bool reindex_relation(Oid rel_id, bool force);

#endif  // RDBMS_CATALOG_INDEX_H_
