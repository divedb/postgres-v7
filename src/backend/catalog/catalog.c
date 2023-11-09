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

// Construct path to a relation's file
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