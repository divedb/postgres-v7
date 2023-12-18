// =========================================================================
//
// funcindex.h
//
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: funcindex.h,v 1.9 2000/01/26 05:57:50 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_ACCESS_FUNC_INDEX_H_
#define RDBMS_ACCESS_FUNC_INDEX_H_

#include "rdbms/postgres.h"

typedef struct func_index {
  int nargs;
  Oid arg_list[FUNC_MAX_ARGS];
  Oid proc_oid;
  NameData func_name;
} FuncIndexInfo;

typedef FuncIndexInfo* FuncIndexInfoPtr;

// Some marginally useful macro definitions.
#define FI_GET_NAME(finfo)                   (finfo)->func_name.data
#define FI_GET_NARGS(finfo)                  (finfo)->nargs
#define FI_GET_PROC_OID(finfo)               (finfo)->proc_oid
#define FI_GET_ARG(finfo, arg_num)           (finfo)->arg_list[arg_num]
#define FI_GET_ARG_LIST(finfo)               (finfo)->arg_list
#define FI_SET_NARGS(finco, num_args)        ((finfo)->nargs = num_args)
#define FI_SET_PROC_OID(finfo, id)           ((finfo)->proc_oid = id)
#define FI_SET_ARG(finfo, arg_num, arg_type) ((finfo)->arg_list[arg_num] = arg_type)

#define FI_IS_FUNCTIONAL_INDEX(finfo) (finfo->proc_oid != INVALID_OID)

#endif  // RDBMS_ACCESS_FUNC_INDEX_H_
