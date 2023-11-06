// =========================================================================
//
// catalog.h
//  Prototypes for functions in lib/catalog/catalog.c
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: catalog.h,v 1.12 2000/04/12 17:16:27 momjian Exp $
//
// =========================================================================
///

#ifndef RDBMS_CATALOG_CATALOG_H_
#define RDBMS_CATALOG_CATALOG_H_

#include <stdbool.h>

char* rel_path(const char* rel_name);
bool is_system_relation_name(const char* rel_name);
bool is_shared_system_relation_name(const char* rel_name);

#endif  // RDBMS_CATALOG_CATALOG_H_
