// =========================================================================
//
// fcache.h
//
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: fcache.h,v 1.10 2000/01/26 05:58:38 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_UTILS_FCACHE_H_
#define RDBMS_UTILS_FCACHE_H_

#include "rdbms/utils/fmgr.h"

typedef struct {
  int type_len;     // Length of the return type.
  int type_by_val;  // True if return type is pass by value.
  FmgrInfo func;    // Address of function to call (for c funcs).
  Oid foid;         // Oid of the function in pg_proc.
  Oid language;     // Oid of the language in pg_language.
  int nargs;        // Number of arguments.

  // Might want to make these two arrays of size MAXFUNCARGS.

  Oid* arg_oid_vect;  // Oids of all the arguments.
  bool* null_vect;    // Keep track of null arguments.
  char* src;          // Source code of the function.
  char* bin;          // Binary object code ??.
  char* func_state;   // Fuction_state struct for execution.
  bool one_result;    // True we only want 1 result from the function.
  bool has_set_arg;   // True if func is part of a nested dot expr whose argument is func returning a set ugh!.
  Pointer func_slot;  // If one result we need to copy it before we end execution of the function and free stuff.
  char* set_arg;  // Current argument for nested dot execution. Nested dot expressions mean we have funcs whose argument
                  // is a set of tuples.
  bool is_trusted;  // trusted fn?.
} FunctionCache, *FunctionCachePtr;

#endif  // RDBMS_UTILS_FCACHE_H_
