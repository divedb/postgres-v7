//===----------------------------------------------------------------------===//
//
// memutils.h
//  This file contains declarations for memory allocation utility
//  functions.  These are functions that are not quite widely used
//  enough to justify going in utils/palloc.h, but are still part
//  of the API of the memory management subsystem.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: memutils.h,v 1.34 2000/04/12 17:16:55 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_MEM_UTILS_H_
#define RDBMS_UTILS_MEM_UTILS_H_

#include "rdbms/nodes/memnodes.h"
#include "rdbms/postgres.h"

// Quasi-arbitrary limit on size of allocations.
//
// Note:
//  There is no guarantee that allocations smaller than MaxAllocSize
//  will succeed.  Allocation requests larger than MaxAllocSize will
//  be summarily denied.
//
//  XXX This is deliberately chosen to correspond to the limiting size
//  of varlena objects under TOAST. See VARATT_MASK_SIZE in postgres.h.
#define MAX_ALLOC_SIZE ((Size)0x3fffffff)  // 1 gigabyte - 1.

// All chunks allocated by any memory context manager are required to be
// preceded by a StandardChunkHeader at a spacing of STANDARDCHUNKHEADERSIZE.
// A currently-allocated chunk must contain a backpointer to its owning
// context as well as the allocated size of the chunk. The backpointer is
// used by pfree() and repalloc() to find the context to call. The allocated
// size is not absolutely essential, but it's expected to be needed by any
// reasonable implementation.
typedef struct StandardChunkHeader {
  MemoryContext context;  // Owning context.
  Size size;              // Size of data space allocated in chunk.

#ifdef MEMORY_CONTEXT_CHECKING
  // When debugging memory usage, also store actual requested size.
  Size requested_size;
#endif

} StandardChunkHeader;

#define STANDARD_CHUNK_HEADER_SIZE MAX_ALIGN(sizeof(StandardChunkHeader))

// Standard top-level memory contexts.
//
// Only TopMemoryContext and ErrorContext are initialized by
// MemoryContextInit() itself.
extern MemoryContext TopMemoryContext;
extern MemoryContext ErrorContext;
extern MemoryContext PostmasterContext;
extern MemoryContext CacheMemoryContext;
extern MemoryContext QueryContext;
extern MemoryContext TopTransactionContext;
extern MemoryContext TransactionCommandContext;

// In mcxt.c
void memory_context_init();
void memory_context_reset(MemoryContext context);
void memory_context_delete(MemoryContext context);
void memory_context_reset_children(MemoryContext context);
void memory_context_delete_children(MemoryContext context);
void memory_context_reset_and_delete_children(MemoryContext context);
void memory_context_stats(MemoryContext context);
void memory_context_check(MemoryContext context);
bool memory_context_contains(MemoryContext context, void* pointer);

// This routine handles the context-type-independent part of memory
// context creation. It's intended to be called from context-type-
// specific creation routines, and noplace else.
MemoryContext memory_context_create(NodeTag tag, Size size, MemoryContextMethods* methods, MemoryContext parent,
                                    const char* name);

// In aset.c
MemoryContext alloc_set_context_create(MemoryContext parent, const char* name, Size min_context_size,
                                       Size init_block_size, Size max_block_size);

// Recommended default alloc parameters, suitable for "ordinary" contexts
// that might hold quite a lot of data.
#define ALLOCSET_DEFAULT_MIN_SIZE  (8 * 1024)
#define ALLOCSET_DEFAULT_INIT_SIZE (8 * 1024)
#define ALLOCSET_DEFAULT_MAX_SIZE  (8 * 1024 * 1024)

#endif  // RDBMS_UTILS_MEM_UTILS_H_
