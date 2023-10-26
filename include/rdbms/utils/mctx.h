#ifndef RDBMS_UTILS_MCTX_H_
#define RDBMS_UTILS_MCTX_H_

// MemoryContext
//  A logical context in which memory allocations occur.
//
// The types of memory contexts can be thought of as members of the
// following inheritance hierarchy with properties summarized below.
//
//                        Node
//                        |
//                MemoryContext___
//                /                \
//        GlobalMemory    PortalMemoryContext
//                        /                \
//        PortalVariableMemory    PortalHeapMemory
//
//                        Flushed at        Flushed at        Checkpoints
//                        Transaction       Portal
//                        Commit            Close
//
// GlobalMemory                    n                n                n
// PortalVariableMemory            n                y                n
// PortalHeapMemory                y                y                y

#include "rdbms/nodes/nodes.h"
#include "rdbms/utils/aset.h"
#include "rdbms/utils/fstack.h"
#include "rdbms/utils/oset.h"

typedef struct MemoryContextMethodsData {
  Pointer (*alloc)();
  void (*free_p)();
  Pointer (*realloc)();
  char* (*getname)();
  void (*dump)();
} * MemoryContextMethods;

typedef struct MemoryContextData {
  NodeTag type;
  MemoryContextMethods method;
} MemoryContextData;

// Think about doing this right some time but we'll have explicit fields
// for now -ay 10/94.

typedef struct GlobalMemoryData {
  NodeTag type;
  MemoryContextMethods method;
  AllocSetData set_data;
  char* name;
  OrderedElemData elem_data;
} GlobalMemoryData;

typedef struct PortalVariableMemoryData {
  NodeTag type;
  MemoryContextMethods method;
  AllocSetData set_data;
} * PortalVariableMemory;

typedef struct PortalHeapMemoryData {
  NodeTag type;
  MemoryContextMethods method;
  Pointer block;
  FixedStackData stack_data;
} * PortalHeapMemory;

typedef struct MemoryContextData* MemoryContext;
typedef struct MemoryContextData* PortalMemoryContext;
typedef struct GlobalMemoryData* GlobalMemory;

#define MemoryContextIsValid(context)                                                                 \
  (IsA(context, MemoryContext) || IsA(context, GlobalMemory) || IsA(context, PortalVariableMemory) || \
   IsA(context, PortalHeapMemory))

extern MemoryContext CurrentMemoryContext;
extern MemoryContext TopMemoryContext;

// MaxAllocSize
//  Arbitrary limit on size of allocations.
//
// Note:
//  There is no guarantee that allocations smaller than MaxAllocSize
//  will succeed.  Allocation requests larger than MaxAllocSize will
//  be summarily denied.
//
//  This value should not be referenced except in one place in the code.
//
// XXX This should be defined in a file of tunable constants.
#define MaxAllocSize (0xFFFFFFF)  // 16G - 1

void enable_memory_context(bool on);
Pointer memory_context_alloc(MemoryContext context, Size size);
Pointer memory_context_realloc(MemoryContext context, Pointer pointer, Size size);
void memory_context_free(MemoryContext context, Pointer pointer);
MemoryContext memory_context_switch_to(MemoryContext context);
GlobalMemory create_global_memory(char* name);
void global_memory_destroy(GlobalMemory context);

#endif  // RDBMS_UTILS_MCTX_H_
