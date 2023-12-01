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

#include "rdbms/postgres.h"

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
} FmgrInfo;

// This struct is the data actually passed to an fmgr-called function.
typedef struct FunctionCallInfoData {
  FmgrInfo* fl_info;            // Ptr to lookup info used for this call.
  struct Node* context;         // Pass info about context of call.
  struct Node* result_info;     // Pass or return extra info about result.
  bool isnull;                  // Function must set true if result is NULL.
  short nargs;                  // # arguments actually passed.
  Datum arg[FUNC_MAX_ARGS];     // Arguments passed to function.
  bool argnull[FUNC_MAX_ARGS];  // T if arg[i] is actually NULL.
} FunctionCallInfoData;

// This routine fills a FmgrInfo struct, given the OID
// of the function to be called.
void fmgr_info(Oid functionId, FmgrInfo* finfo);

// This macro invokes a function given a filled-in FunctionCallInfoData
// struct. The macro result is the returned Datum --- but note that
// caller must still check fcinfo->isnull! Also, if function is strict,
// it is caller's responsibility to verify that no null arguments are present
// before calling.
#define FUNCTION_CALL_INVOKE(fcinfo) ((*(fcinfo)->flinfo->fn_addr)(fcinfo))

// Support macros to ease writing fmgr-compatible functions
//
// A C-coded fmgr-compatible function should be declared as
//
// Datum
// function_name(PG_FUNCTION_ARGS)
// {
//      ...
// }
//
// It should access its arguments using appropriate PG_GETARG_xxx macros
// and should return its result using PG_RETURN_xxx.

// Standard parameter list for fmgr-compatible functions.
#define PG_FUNCTION_ARGS FunctionCallInfo fcinfo

// If function is not marked "proisstrict" in pg_proc, it must check for
// null arguments using this macro.  Do not try to GETARG a null argument!
#define PG_ARG_IS_NULL(n) (fcinfo->argnull[n])

// Support for fetching detoasted copies of toastable datatypes (all of
// which are varlena types).  pg_detoast_datum() gives you either the input
// datum (if not toasted) or a detoasted copy allocated with palloc().
// pg_detoast_datum_copy() always gives you a palloc'd copy --- use it
// if you need a modifiable copy of the input. Caller is expected to have
// checked for null inputs first, if necessary.
//
// Note: it'd be nice if these could be macros, but I see no way to do that
// without evaluating the arguments multiple times, which is NOT acceptable.
#define PG_DETOAST_DATUM(datum)      pg_detoast_datum((struct VarLena*)DATUM_GET_POINTER(datum))
#define PG_DETOAST_DATUM_COPY(datum) pg_detoast_datum_copy((struct VarLena*)DATUM_GET_POINTER(datum))

// Support for cleaning up detoasted copies of inputs. This must only
// be used for pass-by-ref datatypes, and normally would only be used
// for toastable types.  If the given pointer is different from the
// original argument, assume it's a palloc'd detoasted copy, and pfree it.
// NOTE: most functions on toastable types do not have to worry about this,
// but we currently require that support functions for indexes not leak
// memory.
#define PG_FREE_IF_COPY(ptr, n)                             \
  do {                                                      \
    if ((Pointer)(ptr) != PG_GETARG_POINTER(n)) pfree(ptr); \
  } while (0)

// Macros for fetching arguments of standard types.
#define PG_GETARG_DATUM(n)   (fcinfo->arg[n])
#define PG_GETARG_INT32(n)   DATUM_GET_INT32(PG_GETARG_DATUM(n))
#define PG_GETARG_UINT32(n)  DATUM_GET_UINT32(PG_GETARG_DATUM(n))
#define PG_GETARG_INT16(n)   DATUM_GET_INT16(PG_GETARG_DATUM(n))
#define PG_GETARG_UINT16(n)  DATUM_GET_UINT16(PG_GETARG_DATUM(n))
#define PG_GETARG_CHAR(n)    DATUM_GET_CHAR(PG_GETARG_DATUM(n))
#define PG_GETARG_BOOL(n)    DATUM_GET_BOOL(PG_GETARG_DATUM(n))
#define PG_GETARG_OID(n)     DATUM_GET_OBJECT_ID(PG_GETARG_DATUM(n))
#define PG_GETARG_POINTER(n) DATUM_GET_POINTER(PG_GETARG_DATUM(n))
#define PG_GETARG_CSTRING(n) DATUM_GET_CSTRING(PG_GETARG_DATUM(n))
#define PG_GETARG_NAME(n)    DATUM_GET_NAME(PG_GETARG_DATUM(n))

// These macros hide the pass-by-reference-ness of the datatype.
#define PG_GETARG_FLOAT4(n) DATUM_GET_FLOAT4(PG_GETARG_DATUM(n))
#define PG_GETARG_FLOAT8(n) DATUM_GET_FLOAT8(PG_GETARG_DATUM(n))
#define PG_GETARG_INT64(n)  DATUM_GET_INT64(PG_GETARG_DATUM(n))

// Use this if you want the raw, possibly-toasted input datum.
#define PG_GETARG_RAW_VARLENA_P(n) ((struct VarLena)PG_GETARG_POINTER(n))

// Use this if you want the input datum de-toasted.
#define PG_GETARG_VARLENA_P(n) PG_DETOAST_DATUM(PG_GETARG_DATUM(n))

// DatumGetFoo macros for varlena types will typically look like this.
#define DATUM_GET_BYTEA_P(X)    ((Bytea*)PG_DETOAST_DATUM(X))
#define DATUM_GET_TEXT_P(X)     ((Text*)PG_DETOAST_DATUM(X))
#define DATUM_GET_BP_CHAR_P(X)  ((BpChar*)PG_DETOAST_DATUM(X))
#define DATUM_GET_VAR_CHAR_P(X) ((VarChar*)PG_DETOAST_DATUM(X))

// And we also offer variants that return an OK-to-write copy.
#define DATUM_GET_BYTEA_P_COPY(X)    ((Bytea*)PG_DETOAST_DATUM_COPY(X))
#define DATUM_GET_TEXT_P_COPY(X)     ((Text*)PG_DETOAST_DATUM_COPY(X))
#define DATUM_GET_BP_CHAR_P_COPY(X)  ((BpChar*)PG_DETOAST_DATUM_COPY(X))
#define DATUM_GET_VAR_CHAR_P_COPY(X) ((VarChar*)PG_DETOAST_DATUM_COPY(X))

// GETARG macros for varlena types will typically look like this.
#define PG_GETARG_BYTEA_P(n)   DATUM_GET_BYTEA_P(PG_GETARG_DATUM(n))
#define PG_GETARG_TEXT_P(n)    DATUM_GET_TEXT_P(PG_GETARG_DATUM(n))
#define PG_GETARG_BPCHAR_P(n)  DATUM_GET_BP_CHAR_P(PG_GETARG_DATUM(n))
#define PG_GETARG_VARCHAR_P(n) DATUM_GET_VAR_CHAR_P(PG_GETARG_DATUM(n))

// And we also offer variants that return an OK-to-write copy.
#define PG_GETARG_BYTEA_P_COPY(n)   DATUM_GET_BYTEA_P_COPY(PG_GETARG_DATUM(n))
#define PG_GETARG_TEXT_P_COPY(n)    DATUM_GET_TEXT_P_COPY(PG_GETARG_DATUM(n))
#define PG_GETARG_BPCHAR_P_COPY(n)  DATUM_GET_BP_CHAR_P_COPY(PG_GETARG_DATUM(n))
#define PG_GETARG_VARCHAR_P_COPY(n) DATUM_GET_VAR_CHAR_P_COPY(PG_GETARG_DATUM(n))

// To return a NULL do this.
#define PG_RETURN_NULL()   \
  do {                     \
    fcinfo->isnull = true; \
    return (Datum)0;       \
  } while (0)

// A few internal functions return void (which is not the same as NULL!).
#define PG_RETURN_VOID() return (Datum)0

// Macros for returning results of standard types.
#define PG_RETURN_DATUM(x)   return (x)
#define PG_RETURN_INT32(x)   return INT32_GET_DATUM(x)
#define PG_RETURN_UINT32(x)  return UINT32_GET_DATUM(x)
#define PG_RETURN_INT16(x)   return INT16_GET_DATUM(x)
#define PG_RETURN_CHAR(x)    return CHAR_GET_DATUM(x)
#define PG_RETURN_BOOL(x)    return BOOL_GET_DATUM(x)
#define PG_RETURN_OID(x)     return OBJECT_ID_GET_DATUM(x)
#define PG_RETURN_POINTER(x) return POINTER_GET_DATUM(x)
#define PG_RETURN_CSTRING(x) return CSTRING_GET_DATUM(x)
#define PG_RETURN_NAME(x)    return NAME_GET_DATUM(x)

// These macros hide the pass-by-reference-ness of the datatype.
#define PG_RETURN_FLOAT4(x) return float4_get_datum(x)
#define PG_RETURN_FLOAT8(x) return float8_get_datum(x)
#define PG_RETURN_INT64(x)  return int64_get_datum(x)

// RETURN macros for other pass-by-ref types will typically look like this.
#define PG_RETURN_BYTEA_P(x)   PG_RETURN_POINTER(x)
#define PG_RETURN_TEXT_P(x)    PG_RETURN_POINTER(x)
#define PG_RETURN_BPCHAR_P(x)  PG_RETURN_POINTER(x)
#define PG_RETURN_VARCHAR_P(x) PG_RETURN_POINTER(x)

// TODO(gc): add more definition here.

#endif  // RDBMS_UTILS_FMGR_H_
