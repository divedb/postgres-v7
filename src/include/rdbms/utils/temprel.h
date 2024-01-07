//===----------------------------------------------------------------------===//
//
// temprel.h
//  Temporary relation functions
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: temprel.h,v 1.15 2001/03/22 04:01:14 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_TEMP_REL_H_
#define RDBMS_UTILS_TEMP_REL_H_

#include "rdbms/access/htup.h"

void create_temp_relation(const char* rel_name, HeapTuple pg_class_tuple);

#endif  // RDBMS_UTILS_TEMP_REL_H_
