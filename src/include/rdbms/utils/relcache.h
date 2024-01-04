//===----------------------------------------------------------------------===//
//
// relcache.h
//  Relation descriptor cache definitions.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: relcache.h,v 1.24 2001/01/24 19:43:29 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_RELCACHE_H_
#define RDBMS_UTILS_RELCACHE_H_

#include "rdbms/storage/relfilenode.h"
#include "rdbms/utils/rel.h"

// Relation lookup routines.
Relation relation_id_get_relation(Oid relation_id);
Relation relation_name_get_relation(const char* relation_name);
Relation relation_node_cache_get_relation(RelFileNode rnode);

#endif  // RDBMS_UTILS_RELCACHE_H_
