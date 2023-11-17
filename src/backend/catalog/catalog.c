// =========================================================================
//
// catalog.c
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/catalog/catalog.c,v 1.32 2000/04/12 17:14:55 momjian Exp $
//
// =========================================================================

#include "rdbms/catalog/catalog.h"

#include <stdio.h>

#include "rdbms/catalog/catname.h"
#include "rdbms/miscadmin.h"
#include "rdbms/utils/palloc.h"

// relpath - Construct path to a relation's file
//
// Note that this only works with relations that are visible to the current
// backend, ie, either in the current database or shared system relations.
//
// Result is a palloc'd string.
char* relpath(const char* relation_name) {
  char* path;

  if (is_shared_system_relation_name(relation_name)) {
    // Shared system relations live in DataDir.
    size_t buf_size = strlen(DataDir) + sizeof(NameData) + 2;

    path = (char*)palloc(buf_size);
    snprintf(path, buf_size, "%s%c%s", DataDir, SEP_CHAR, relation_name);

    return path;
  }

  // If it is in the current database, assume it is in current working
  // directory. NB: this does not work during bootstrap!
  return pstrdup(relation_name);
}

// rel_path_blind - construct path to a relation's file
//
// Construct the path using only the info available to smgrblindwrt,
// namely the names and OIDs of the database and relation.	(Shared system
// relations are identified with dbid = 0.)  Note that we may have to
// access a relation belonging to a different database!
//
// Result is a palloc'd string.
char* rel_path_blind(const char* db_name, const char* rel_name, Oid db_id, Oid rel_id) {
  char* path;

  if (db_id == 0) {
    // Shared system relations live in DataDir.
    path = (char*)palloc(strlen(DataDir) + sizeof(NameData) + 2);
    sprintf(path, "%s%c%s", DataDir, SEP_CHAR, rel_name);
  } else if (db_id == MyDatabaseId) {
    // XXX why is this inconsistent with relpath() ?
    path = (char*)palloc(strlen(DatabasePath) + sizeof(NameData) + 2);
    sprintf(path, "%s%c%s", DatabasePath, SEP_CHAR, rel_name);
  } else {
  }
}

// True iff name is the name of a system catalog relation.
//
// We now make a new requirement where system catalog relns must begin
// with pg_ while user relns are forbidden to do so.  Make the test
// trivial and instantaneous.
//
// XXX this is way bogus. -- pma
bool is_system_relation_name(const char* relation_name) {
  if (relation_name[0] && relation_name[1] && relation_name[2]) {
    return relation_name[0] == 'p' && relation_name[1] == 'g' && relation_name[2] == '_';
  }

  return false;
}

// True iff name is the name of a shared system catalog relation.
bool is_shared_system_relation_name(const char* relation_name) {
  int i = 0;

  // Quick out: if it's not a system relation, it can't be a shared
  // system relation.
  if (!is_system_relation_name(relation_name)) {
    return false;
  }

  while (SharedSystemRelationNames[i] != NULL) {
    if (strcmp(SharedSystemRelationNames[i], relation_name) == 0) {
      return true;
    }

    i++;
  }

  return false;
}