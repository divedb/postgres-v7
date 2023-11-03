// =========================================================================
//
// fmgr.h
//  Definitions for the Postgres function manager and function=call
//  interface.
//
// This file must be included by all Postgres modules that either define
// or call fmgr=callable functions.
//
//
// Portions Copyright (c) 1996=2009, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $PostgreSQL: pgsql/src/include/fmgr.h,v 1.62 2009/01/01 17:23:55 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_UTILS_FMGR_H_
#define RDBMS_UTILS_FMGR_H_

// We don't want to include primnodes.h here, so make a stub reference.
typedef struct Node* FmNodePtr;

// Likewise, avoid including stringinfo.h here.
typedef struct StringInfoData* FmStringInfo;

// All functions that can be called directly by fmgr must have this signature.
// (Other functions can be called by using a handler that does have this
// signature.)
typedef struct FunctionCallInfoData* FunctionCallInfo;

typedef Datum (*PGFunction)(FunctionCallInfo fcinfo);

// This struct holds the system-catalog information that must be looked up
// before a function can be called through fmgr.  If the same function is
// to be called multiple times, the lookup need be done only once and the
// info struct saved for re-use.
typedef struct FmgrInfo {
  PGFunction fn_addr;      // Pointer to function or handler to be called.
  Oid fn_oid;              // OID of function (NOT of handler, if any).
  short fn_nargs;          // 0..FUNC_MAX_ARGS, or -1 if variable arg count.
  bool fn_strict;          // Function is "strict" (NULL in => NULL out).
  bool fn_retset;          // Function returns a set.
  unsigned char fn_stats;  // Collect stats if track_functions > this.
  void* fn_extra;          // Extra space for use by handler.
  MemoryContext fn_mcxt;   // Memory context to store fn_extra in.
  FmNodePtr fn_expr;       // Expression parse tree for call, or NULL.
} FmgrInfo;

// This struct is the data actually passed to an fmgr-called function.
typedef struct FunctionCallInfoData {
  FmgrInfo* flinfo;             // Ptr to lookup info used for this call.
  FmNodePtr context;            // Pass info about context of call.
  FmNodePtr resultinfo;         // Pass or return extra info about result.
  bool isnull;                  // Function must set true if result is NULL.
  short nargs;                  // # arguments actually passed.
  Datum arg[FUNC_MAX_ARGS];     // Arguments passed to function.
  bool argnull[FUNC_MAX_ARGS];  // T if arg[i] is actually NULL.
} FunctionCallInfoData;

char* fmgr_c(FmgrInfo* finfo, FmgrValues* values, bool* is_null);

// This routine fills a FmgrInfo struct, given the OID
// of the function to be called.
void fmgr_info(Oid functionId, FmgrInfo* finfo);

#endif  // RDBMS_UTILS_FMGR_H_
