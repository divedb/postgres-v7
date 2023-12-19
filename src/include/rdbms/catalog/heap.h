//===----------------------------------------------------------------------===//
//
// heap.h
//  prototypes for functions in lib/catalog/heap.c
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: heap.h,v 1.34 2001/03/22 04:00:35 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_CATALOG_HEAP_H_
#define RDBMS_CATALOG_HEAP_H_

#include "rdbms/utils/rel.h"

typedef struct RawColumnDefault {
  AttrNumber att_num;  // Attribute to attach default to
  Node* raw_default;   // Default value (untransformed parse tree)
} RawColumnDefault;

Oid rel_name_find_rel_id(const char* rel_name);
Relation heap_create(char* rel_name, TupleDesc tup_desc, bool is_temp, bool storage_create,
                     bool allow_system_table_mods);
void heap_storage_create(Relation rel);
Oid heap_create_with_catalog(char* rel_name, TupleDesc tup_desc, char rel_kind, bool is_temp,
                             bool allow_system_table_mods);
void heap_drop_with_catalog(const char* rel_name, bool allow_system_table_mods);
void heap_truncate(char* rel_name);
void add_relation_raw_constraints(Relation rel, List* raw_col_defaults, List* raw_constraints);

#endif  // RDBMS_CATALOG_HEAP_H_
