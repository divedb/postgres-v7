//===----------------------------------------------------------------------===//
//
// temprel.c
//  POSTGRES temporary relation handling
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
// $Header:
//  home/projects/pgsql/cvsroot/pgsql/src/backend/utils/cache/temprel.c
//  v 1.35 2001/03/22 03:59:58 momjian Exp $
//
//===----------------------------------------------------------------------===//
#include "rdbms/utils/temprel.h"

#include <sys/types.h>

#include "rdbms/catalog/pg_class.h"
#include "rdbms/nodes/pg_list.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/memutils.h"

// This implements temp tables by modifying the relname cache lookups
// of pg_class.
//
// When a temp table is created, normal entries are made for it in pg_class,
// pg_type, etc using a unique "physical" relation name. We also make an
// entry in the temp table list maintained by this module. Subsequently,
// relname lookups are filtered through the temp table list, and attempts
// to look up a temp table name are changed to look up the physical name.
// This allows temp table names to mask a regular table of the same name
// for the duration of the session.  The temp table list is also used
// to drop the underlying physical relations at session shutdown.
static List* TempRels = NIL;

typedef struct TempTable {
  NameData user_rel_name;  // Logical name of temp table
  NameData rel_name;       // Underlying unique name
  Oid rel_id;              // Needed properties of rel
  char rel_kind;

  // If this entry was created during this xact, it should be deleted at
  // xact abort. Conversely, if this entry was deleted during this
  // xact, it should be removed at xact commit.  We leave deleted
  // entries in the list until commit so that we can roll back if needed
  // --- but we ignore them for purposes of lookup!
  bool created_in_cur_xact;
  bool deleted_in_cur_xact;
} TempTable;

// Create a temp-relation list entry given the logical temp table name
// and the already-created pg_class tuple for the underlying relation.
//
// NB: we assume a check has already been made for a duplicate logical name.
void create_temp_relation(const char* rel_name, HeapTuple pg_class_tuple) {
  Form_pg_class pg_class_form = (Form_pg_class)GET_STRUCT(pg_class_tuple);
  MemoryContext old_cxt;
}