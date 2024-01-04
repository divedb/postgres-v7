//===----------------------------------------------------------------------===//
//
// fmgr.c
//  The Postgres function manager.
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header:
//  /home/projects/pgsql/cvsroot/pgsql/src/backend/utils/fmgr/fmgr.c,v 1.51
//  2001/03/22 03:59:59 momjian Exp $
//
//===----------------------------------------------------------------------===//
#include "rdbms/utils/fmgr.h"

#include "rdbms/postgres.h"
#include "rdbms/utils/elog.h"

// Declaration for old-style function pointer type.  This is now used only
// in fmgr_oldstyle() and is no longer exported.
//
// The m68k SVR4 ABI defines that pointers are returned in %a0 instead of
// %d0. So if a function pointer is declared to return a pointer, the
// compiler may look only into %a0, but if the called function was declared
// to return an integer type, it puts its value only into %d0. So the
// caller doesn't pink up the correct return value. The solution is to
// declare the function pointer to return int, so the compiler picks up the
// return value from %d0. (Functions returning pointers put their value
// *additionally* into %d0 for compatibility.) The price is that there are
// some warnings about int->pointer conversions...
#if defined(__mc68000__) && defined(__ELF__)
typedef int32((*func_ptr)());
#else
typedef char*((*func_ptr)());
#endif

// For an oldstyle function, fn_extra points to a record like this:
typedef struct {
  func_ptr func;                      // Address of the oldstyle function
  bool arg_toastable[FUNC_MAX_ARGS];  // Is n'th arg of a toastable datatype ?
} OldStyleFnExtra;

// These are for invocation of a specifically named function with a
// directly-computed parameter list.  Note that neither arguments nor result
// are allowed to be NULL. Also, the function cannot be one that needs to
// look at FmgrInfo, since there won't be any.
Datum direct_function_call1(PGFunction func, Datum arg1) {
  FunctionCallInfoData fcinfo;
  Datum result;
  MEMSET(&fcinfo, 0, sizeof(fcinfo));

  fcinfo.nargs = 1;
  fcinfo.arg[0] = arg1;
  result = (*func)(&fcinfo);

  // Check for null result, since caller is clearly not expecting one.
  if (fcinfo.is_null) {
    elog(ERROR, "%s: function %p returned NULL", __func__, (void*)func);
  }

  return result;
}

Datum direct_function_call2(PGFunction func, Datum arg1, Datum arg2) {
  FunctionCallInfoData fcinfo;
  Datum result;

  MEMSET(&fcinfo, 0, sizeof(fcinfo));
  fcinfo.nargs = 2;
  fcinfo.arg[0] = arg1;
  fcinfo.arg[1] = arg2;

  result = (*func)(&fcinfo);

  // Check for null result, since caller is clearly not expecting one.
  if (fcinfo.is_null) {
    elog(ERROR, "%s: function %p returned NULL", __func__, (void*)func);
  }

  return result;
}

Datum direct_function_call3(PGFunction func, Datum arg1, Datum arg2,
                            Datum arg3) {
  FunctionCallInfoData fcinfo;
  Datum result;

  MEMSET(&fcinfo, 0, sizeof(fcinfo));
  fcinfo.nargs = 3;
  fcinfo.arg[0] = arg1;
  fcinfo.arg[1] = arg2;
  fcinfo.arg[2] = arg3;

  result = (*func)(&fcinfo);

  // Check for null result, since caller is clearly not expecting one.
  if (fcinfo.is_null) {
    elog(ERROR, "%s: function %p returned NULL", __func__, (void*)func);
  }

  return result;
}

// static void fmgr_info_c_lang(FmgrInfo* finfo, HeapTuple procedure_tuple);
