// =========================================================================
//
// database.c
//  miscellaneous initialization support stuff
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/utils/misc/database.c,v 1.37 2000/04/12 17:16:07 momjian Exp $
//
// =========================================================================

#include "rdbms/miscadmin.h"

// Find the OID and path of the database.
//
// The database's oid forms half of the unique key for the system
// caches and lock tables.  We therefore want it initialized before
// we open any relations, since opening relations puts things in the
// cache. To get around this problem, this code opens and scans the
// pg_database relation by hand.
//
// This code knows way more than it should about the layout of
// tuples on disk, but there seems to be no help for that.
// We're pulling ourselves up by the bootstraps here...
void get_raw_database_info(const char* name, Oid* db_id, char* path) {
  int db_fd;
  int nbytes;
  int max;
  int i;
}