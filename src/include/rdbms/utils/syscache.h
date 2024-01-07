//===----------------------------------------------------------------------===//
//
// syscache.h
//  System catalog cache definitions.
//
// See also lsyscache.h, which provides convenience routines for
// common cache-lookup operations.
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: syscache.h,v 1.29 2001/03/22 04:01:14 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_SYS_CACHE_H_
#define RDBMS_UTILS_SYS_CACHE_H_

#include "rdbms/access/htup.h"

// Declarations for util/syscache.c.
//
// SysCache identifiers.
//
// The order of these must match the order
// they are entered into the structure cacheinfo[] in syscache.c
// Keep them in alphabeticall order.
#define AGGNAME      0
#define AMNAME       1
#define AMOPOPID     2
#define AMOPSTRATEGY 3
#define ATTNAME      4
#define ATTNUM       5
#define CLADEFTYPE   6
#define CLANAME      7
#define GRONAME      8
#define GROSYSID     9
#define INDEXRELID   10
#define INHRELID     11
#define LANGNAME     12
#define LANGOID      13
#define LISTENREL    14
#define OPERNAME     15
#define OPEROID      16
#define PROCNAME     17
#define PROCOID      18
#define RELNAME      19
#define RELOID       20
#define RULENAME     21
#define RULEOID      22
#define SHADOWNAME   23
#define SHADOWSYSID  24
#define STATRELID    25
#define TYPENAME     26
#define TYPEOID      27

void init_catalog_cache();
HeapTuple search_sys_cache(int cache_id, Datum key1, Datum key2, Datum key3,
                           Datum key4);
void release_sys_cache(HeapTuple tuple);

// Convenience routines.
HeapTuple search_sys_cache_copy(int cache_id, Datum key1, Datum key2,
                                Datum key3, Datum key4);

#endif  // RDBMS_UTILS_SYS_CACHE_H_
