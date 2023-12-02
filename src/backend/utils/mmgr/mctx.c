//===----------------------------------------------------------------------===//
//
// mcxt.c
//  POSTGRES memory context management code.
//
// This module handles context management operations that are independent
// of the particular kind of context being operated on.  It calls
// context-type-specific operations via the function pointers in a
// context's MemoryContextMethods struct.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/utils/mmgr/mcxt.c,v 1.28 2001/03/22 04:00:08 momjian
//  Exp $
//
//===----------------------------------------------------------------------===//

#include "rdbms/nodes/memnodes.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/excid.h"
#include "rdbms/utils/memutils.h"

MemoryContext CurrentMemoryContext = NULL;
MemoryContext TopMemoryContext = NULL;
MemoryContext ErrorContext = NULL;
MemoryContext PostmasterContext = NULL;
MemoryContext CacheMemoryContext = NULL;
MemoryContext QueryContext = NULL;
MemoryContext TopTransactionContext = NULL;
MemoryContext TransactionCommandContext = NULL;

// Start up the memory-context subsystem.
//
// This must be called before creating contexts or allocating memory in
// contexts.  TopMemoryContext and ErrorContext are initialized here;
// other contexts must be created afterwards.
//
// In normal multi-backend operation, this is called once during
// postmaster startup, and not at all by individual backend startup
// (since the backends inherit an already-initialized context subsystem
// by virtue of being forked off the postmaster).
//
// In a standalone backend this must be called during backend startup.
void memory_context_init() {
  assert(TopMemoryContext == NULL);

  // Initialize TopMemoryContext as an AllocSetContext with slow growth
  // rate --- we don't really expect much to be allocated in it.
  //
  // (There is special-case code in MemoryContextCreate() for this call.)
  TopMemoryContext = alloc_set_context_create(NULL, "TopMemoryContext", 8 * 1024, 8 * 1024, 8 * 1024);

  // Not having any other place to point CurrentMemoryContext, make it
  // point to TopMemoryContext. Caller should change this soon!
  CurrentMemoryContext = TopMemoryContext;

  // Initialize ErrorContext as an AllocSetContext with slow growth rate
  // --- we don't really expect much to be allocated in it. More to the
  // point, require it to contain at least 8K at all times. This is the
  // only case where retained memory in a context is *essential* --- we
  // want to be sure ErrorContext still has some memory even if we've
  // run out elsewhere!
  ErrorContext = alloc_set_context_create(TopMemoryContext, "ErrorContext", 8 * 1024, 8 * 1024, 8 * 1024);
}

// Release all space allocated within a context and its descendants,
// but don't delete the contexts themselves.
//
// The type-specific reset routine handles the context itself, but we
// have to do the recursion for the children.
void memory_context_reset(MemoryContext context) {
  assert(MEMORY_CONTEXT_IS_VALID(context));

  memory_context_reset_children(context);
  (*context->methods->reset)(context);
}

void memory_context_delete(MemoryContext context) {}

// Release all space allocated within a context's descendants,
// but don't delete the contexts themselves. The named context
// itself is not touched.
void memory_context_reset_children(MemoryContext context) {
  MemoryContext child;

  assert(MEMORY_CONTEXT_IS_VALID(context));

  for (child = context->firstchild; child != NULL; child = child->nextchild) {
    memory_context_reset(child);
  }
}

// Context-type-independent part of context creation.
//
// This is only intended to be called by context-type-specific
// context creation routines, not by the unwashed masses.
//
// The context creation procedure is a little bit tricky because
// we want to be sure that we don't leave the context tree invalid
// in case of failure (such as insufficient memory to allocate the
// context node itself).  The procedure goes like this:
//  1. Context-type-specific routine first calls MemoryContextCreate(),
//     passing the appropriate tag/size/methods values (the methods
//     pointer will ordinarily point to statically allocated data).
//     The parent and name parameters usually come from the caller.
//  2. MemoryContextCreate() attempts to allocate the context node,
//     plus space for the name.  If this fails we can elog() with no
//     damage done.
//  3. We fill in all of the type-independent MemoryContext fields.
//  4. We call the type-specific init routine (using the methods pointer).
//     The init routine is required to make the node minimally valid
//     with zero chance of failure --- it can't allocate more memory,
//     for example.
//  5. Now we have a minimally valid node that can behave correctly
//     when told to reset or delete itself.  We link the node to its
//     parent (if any), making the node part of the context tree.
//  6. We return to the context-type-specific routine, which finishes
//     up type-specific initialization.  This routine can now do things
//     that might fail (like allocate more memory), so long as it's
//     sure the node is left in a state that delete will handle.
//
// This protocol doesn't prevent us from leaking memory if step 6 fails
// during creation of a top-level context, since there's no parent link
// in that case.  However, if you run out of memory while you're building
// a top-level context, you might as well go home anyway...
//
// Normally, the context node and the name are allocated from
// TopMemoryContext (NOT from the parent context, since the node must
// survive resets of its parent context!). However, this routine is itself
// used to create TopMemoryContext!  If we see that TopMemoryContext is NULL,
// we assume we are creating TopMemoryContext and use malloc() to allocate
// the node.
//
// Note that the name field of a MemoryContext does not point to
// separately-allocated storage, so it should not be freed at context
// deletion.
MemoryContext memory_context_create(NodeTag tag, Size size, MemoryContextMethods* methods, MemoryContext parent,
                                    const char* name) {
  MemoryContext node;
  Size needed = size + strlen(name) + 1;

  // Get space for node and name.
  if (TopMemoryContext != NULL) {
    // Normal case: allocate the node in TopMemoryContext.
    node = (MemoryContext)memory_context_alloc(TopMemoryContext, needed);
  } else {
    // Special case for startup: use good ol' malloc.
    node = (MemoryContext)malloc(needed);

    assert(node != NULL);
  }

  MEMSET(node, 0, size);
  node->type = tag;
  node->methods = methods;
  node->parent = NULL;
  node->firstchild = NULL;
  node->nextchild = NULL;
  node->name = ((char*)node) + size;
  strcpy(node->name, name);

  // Type-specific routine finishes any other essential initialization.
  (*node->methods->init)(node);

  // OK to link node to parent (if any).
  if (parent) {
    node->parent = parent;
    node->nextchild = parent->firstchild;
    parent->firstchild = node;
  }

  return node;
}

// This is just a debugging utility, so it's not fancy.  The statistics
// are merely sent to stderr.
void memory_context_stats(MemoryContext context) {
  MemoryContext child;

  assert(MEMORY_CONTEXT_IS_VALID(context));

  (*context->methods->stats)(context);

  for (child = context->firstchild; child != NULL; child = child->nextchild) {
    memory_context_stats(child);
  }
}

#ifdef MEMORY_CONTEXT_CHECKING
void memory_context_check(MemoryContext context) {
  MemoryContext child;

  assert(MEMORY_CONTEXT_IS_VALID(context));

  (*context->methods->check)(context);
  for (child = context->firstchild; child != NULL; child = child->nextchild) {
    memory_context_check(child);
  }
}
#endif

bool memory_context_contains(MemoryContext context, void* pointer) {}

void* memory_context_alloc(MemoryContext context, Size size) {
  assert(MEMORY_CONTEXT_IS_VALID(context));

  if (!ALLOC_SIZE_IS_VALID(size)) {
    elog(ERROR, "%s: invalid request size %lu", __func__, (unsigned long)size);
  }

  return (*context->methods->alloc)(context, size);
}

MemoryContext memory_context_switch_to(MemoryContext context) {
  MemoryContext old;

  assert(MEMORY_CONTEXT_IS_VALID(context));

  old = CurrentMemoryContext;
  CurrentMemoryContext = context;

  return old;
}

char* memory_context_strdup(MemoryContext context, const char* string) {
  char* nstr;
  Size len = strlen(string) + 1;

  nstr = (char*)memory_context_alloc(context, len);
  memcpy(nstr, string, len);

  return nstr;
}

void* palloc(Size size) { return memory_context_alloc(CurrentMemoryContext, size); }

void pfree(void* pointer) {
  StandardChunkHeader* header;

  assert(pointer != NULL);
  assert(pointer == (void*)MAX_ALIGN(pointer));

  header = (StandardChunkHeader*)((char*)pointer - STANDARD_CHUNK_HEADER_SIZE);

  assert(MEMORY_CONTEXT_IS_VALID(header->context));

  (*header->context->methods->free_p)(header->context, pointer);
}