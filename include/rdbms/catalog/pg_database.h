// =========================================================================
//
// pg_database.h
//  definition of the system "database" relation (pg_database)
//  along with the relation's initial contents.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: pg_database.h,v 1.9 2000/01/26 05:57:57 momjian Exp $
//
// NOTES
//  the genbki.sh script reads this file and generates .bki
//  information from the DATA() statements.
//
// =========================================================================

#ifndef RDBMS_CATALOG_PG_DATABASE_H_
#define RDBMS_CATALOG_PG_DATABASE_H_

CATALOG(pg_database) BOOTSTRAP {
  NameData datname;
  int4 datdba;
  int4 encoding;
  text datpath;  // VARIABLE LENGTH FIELD.
}
FormData_pg_database;

typedef FormData_pg_database* Form_pg_database;

#define Natts_pg_database         4
#define Anum_pg_database_datname  1
#define Anum_pg_database_datdba   2
#define Anum_pg_database_encoding 3
#define Anum_pg_database_datpath  4

#endif  // RDBMS_CATALOG_PG_DATABASE_H_
