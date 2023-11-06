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

// Construct path to a relation's file
//
// Note that this only works with relations that are visible to the current
// backend, ie, either in the current database or shared system relations.
//
// Result is a palloc'd string.
char* relpath(const char* relname);

// True iff name is the name of a system catalog relation.
//
// We now make a new requirement where system catalog relns must begin
// with pg_ while user relns are forbidden to do so.  Make the test
// trivial and instantaneous.
//
// XXX this is way bogus. -- pma
bool is_system_relation_name(const char* rel_name) {
  if (rel_name[0] && rel_name[1] && rel_name[2]) {
    return rel_name[0] == 'p' && rel_name[1] == 'g' && rel_name[2] == '_';
  }

  return false;
}

// True iff name is the name of a shared system catalog relation.
bool is_shared_system_relation_name(const char* rel_name);