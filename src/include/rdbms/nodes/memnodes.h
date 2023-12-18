//===----------------------------------------------------------------------===//
//
// memnodes.h
//  POSTGRES memory context node definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: memnodes.h,v 1.16 2000/01/26 05:58:16 momjian Exp $
//
// XXX the typedefs in this file are different from the other ???nodes.h;
//  they are pointers to structures instead of the structures themselves.
//  If you're wondering, this is plain laziness. I don't want to touch
//  the memory context code which should be revamped altogether some day.
//                                                        - ay 10/94
//===----------------------------------------------------------------------===//
#ifndef RDBMS_NODES_MEM_NODES_H_
#define RDBMS_NODES_MEM_NODES_H_

#include "rdbms/nodes/nodes.h"

typedef struct MemoryContextData* MemoryContext;

// MemoryContext
//  A logical context in which memory allocations occur.
//
// MemoryContext itself is an abstract type that can have multiple
// implementations, though for now we have only AllocSetContext.
// The function pointers in MemoryContextMethods define one specific
// implementation of MemoryContext --- they are a virtual function table
// in C++ terms.
//
// Node types that are actual implementations of memory contexts must
// begin with the same fields as MemoryContext.
//
// Note: for largely historical reasons, typedef MemoryContext is a pointer
// to the context struct rather than the struct type itself.
typedef struct MemoryContextMethods {
  void* (*alloc)(MemoryContext context, Size size);
  /* call this free_p in case someone #define's free() */
  void (*free_p)(MemoryContext context, void* pointer);
  void* (*realloc)(MemoryContext context, void* pointer, Size size);
  void (*init)(MemoryContext context);
  void (*reset)(MemoryContext context);
  void (*delete)(MemoryContext context);
#ifdef MEMORY_CONTEXT_CHECKING
  void (*check)(MemoryContext context);
#endif
  void (*stats)(MemoryContext context);
} MemoryContextMethods;

typedef struct MemoryContextData {
  NodeTag type;                   // Identifies exact kind of context.
  MemoryContextMethods* methods;  // Virtual function table.
  MemoryContext parent;           // NULL if no parent (toplevel context).
  MemoryContext firstchild;       // Head of linked list of children.
  MemoryContext nextchild;        // Next child of same parent.
  char* name;                     // Context name (just for debugging).
} MemoryContextData;

#define MEMORY_CONTEXT_IS_VALID(context) ((context) != NULL && (IS_A((context), AllocSetContext)))

#endif  // RDBMS_NODES_MEM_NODES_H_
