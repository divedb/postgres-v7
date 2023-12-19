//===----------------------------------------------------------------------===//
//
// catalog.h
//  prototypes for functions in lib/catalog/catalog.c
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: catalog.h,v 1.16 2001/03/22 04:00:34 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_CATALOG_CATALOG_H_
#define RDBMS_CATALOG_CATALOG_H_

#include "rdbms/access/tupdesc.h"

#ifdef OLD_FILE_NAMING

char* rel_path(const char* rel_name);
char* rel_path_blind(const char* db_name, const char* rel_name, Oid db_id, Oid rel_id);

#else
#include "rdbms/storage/relfilenode.h"

char* rel_path(RelFileNode rnode);
char* get_database_path(Oid tbl_node);

#endif

bool is_system_relation_name(const char* rel_name);
bool is_shared_system_relation_name(const char* rel_name);

extern Oid newoid(void);

#endif  // RDBMS_CATALOG_CATALOG_H_
