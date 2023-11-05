// =========================================================================
//
// palloc.h
//  POSTGRES memory allocator definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: palloc.h,v 1.12 2000/01/26 05:58:38 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_UTILS_PALLOC_H_
#define RDBMS_UTILS_PALLOC_H_

// TODO(gc): fix this.
#define PALLOC_IS_MALLOC

#ifdef PALLOC_IS_MALLOC

#include <stdlib.h>

#define palloc(s)      malloc(s)
#define pfree(p)       free(p)
#define repalloc(p, s) realloc((p), (s))

#else  // ! PALLOC_IS_MALLOC

#include "rdbms/utils/mctx.h"

// In the case we use memory contexts, use macro's for palloc() etc.
#define palloc(s)      ((void*)memory_context_alloc(CurrentMemoryContext, (Size)(s)))
#define pfree(p)       memory_context_free(CurrentMemoryContext, (Pointer)(p))
#define repalloc(p, s) ((void*)memory_context_realloc(CurrentMemoryContext, (Pointer)(p), (Size)(s)))

#endif  // PALLOC_IS_MALLOC

// Like strdup except uses palloc.
char* pstrdup(const char* pointer);

#endif  // RDBMS_UTILS_PALLOC_H_
