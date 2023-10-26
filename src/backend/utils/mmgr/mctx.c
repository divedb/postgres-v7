#include "rdbms/utils/mctx.h"

#include "rdbms/utils/enbl.h"
#include "rdbms/utils/oset.h"

#define MemoryContextEnabled  (MemoryContextEnableCount > 0)
#define ActiveGlobalMemorySet (&ActiveGlobalMemorySetData)

// Global state.
static int MemoryContextEnableCount = 0;
static OrderedSetData ActiveGlobalMemorySetData;

// Description of allocated memory representation goes here.
#define PSIZE(PTR)      (*((int32*)(PTR)-1))
#define PSIZEALL(PTR)   (*((int32*)(PTR)-1) + sizeof(int32))
#define PSIZESKIP(PTR)  ((char*)((int32*)(PTR) + 1))
#define PSIZEFIND(PTR)  ((char*)((int32*)(PTR)-1))
#define PSIZESPACE(LEN) ((LEN) + sizeof(int32))

// True iff 0 < size and size <= MaxAllocSize.
#define AllocSizeIsValid(size) (0 < (size) && (size) <= MaxAllocSize)

MemoryContext CurrentMemoryContext = NULL;

static Pointer global_memory_alloc(GlobalMemory mem, Size size);
static void global_memory_free(GlobalMemory mem, Pointer pointer);
static Pointer global_memory_realloc(GlobalMemory mem, Pointer pointer, Size size);
static char* global_memory_get_name(GlobalMemory mem);
static void global_memory_dump(GlobalMemory mem);

static struct MemoryContextMethodsData GlobalContextMethodsData = {
    global_memory_alloc,     // palloc
    global_memory_free,      // pfree
    global_memory_realloc,   // repalloc
    global_memory_get_name,  // getname
    global_memory_dump       // dump
};

static struct GlobalMemoryData TopGlobalMemoryData = {
    T_GlobalMemory,                                            // NodeTag
    &GlobalContextMethodsData,                                 // ContextMethods
    {NULL, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}},  // Free AllocSet
    "TopGlobal",                                               // Name
    {0}                                                        // Uninitialized OrderedElemData elemD
};

// TopMemoryContext
//  Memory context for general global allocations.
//
// Note:
//  Don't use this memory context for random allocations.  If you
//  allocate something here, you are expected to clean it up when
//  appropriate.
MemoryContext TopMemoryContext = (MemoryContext)&TopGlobalMemoryData;

// EnableMemoryContext
//  Enables/disables memory management and global contexts.
//
// Note:
//  This must be called before creating contexts or allocating memory.
//  This must be called before other contexts are created.
//
// Exceptions:
//  BadArg if on is invalid.
//  BadState if on is false when disabled.
void enable_memory_context(bool on) {
  static bool processing = false;

  assert(!processing);

  if (bypass_enable(&MemoryContextEnableCount, on)) {
    return;
  }

  processing = true;

  if (on) {
    // Initialize TopGlobalMemoryData.setData
    alloc_set_init(&TopGlobalMemoryData.set_data, DynamicAllocMode, 0);

    // Make TopGlobalMemoryData member of ActiveGlobalMemorySet
    ordered_set_init(ActiveGlobalMemorySet, offsetof(struct GlobalMemoryData, elem_data));
    ordered_elem_push_into(&TopGlobalMemoryData.elem_data, ActiveGlobalMemorySet);

    CurrentMemoryContext = TopMemoryContext;
  } else {
    GlobalMemory context;

    while (PointerIsValid(context = (GlobalMemory)ordered_set_get_head(ActiveGlobalMemorySet))) {
      if (context == &TopGlobalMemoryData) {
        ordered_elem_pop(&TopGlobalMemoryData.elem_data);
      } else {
        global_memory_destroy(context);
      }
    }

    // Freeing memory here should be safe as this is called only after
    // all modules which allocate in TopMemoryContext have been
    // disabled.
    alloc_set_reset(&TopGlobalMemoryData.set_data);
  }

  processing = false;
}

// MemoryContextAlloc
//  Returns pointer to aligned allocated memory in the given context.
//
// Note:
//  none
//
// Exceptions:
//  BadState if called before InitMemoryManager.
//  BadArg if context is invalid or if size is 0.
//  BadAllocSize if size is larger than MaxAllocSize.
Pointer memory_context_alloc(MemoryContext context, Size size) {
  assert(MemoryContextEnabled);
  assert(MemoryContextIsValid(context));
  assert(AllocSizeIsValid(size));

  return context->method->alloc(context, size);
}

// MemoryContextRelloc
//  Returns pointer to aligned allocated memory in the given context.
//
// Note:
//  none
//
// Exceptions:
//  ???
//  BadArgumentsErr if firstTime is true for subsequent calls.
Pointer memory_context_realloc(MemoryContext context, Pointer pointer, Size size) {
  assert(MemoryContextEnabled);
  assert(MemoryContextIsValid(context));
  assert(PointerIsValid(pointer));
  assert(AllocSizeIsValid(size));

  return context->method->realloc(context, pointer, size);
}

// MemoryContextFree
//  Frees allocated memory referenced by pointer in the given context.
//
// Note:
//  none
//
// Exceptions:
//  ???
//  BadArgumentsErr if firstTime is true for subsequent calls.
void memory_context_free(MemoryContext context, Pointer pointer) {
  assert(MemoryContextEnabled);
  assert(MemoryContextIsValid(context));
  assert(PointerIsValid(pointer));

  context->method->free_p(context, pointer);
}

// MemoryContextSwitchTo
//  Returns the current context; installs the given context.
//
// Note:
//  none
//
// Exceptions:
//  BadState if called when disabled.
//  BadArg if context is invalid.
MemoryContext memory_context_switch_to(MemoryContext context) {
  MemoryContext old;

  assert(MemoryContextEnabled);
  assert(MemoryContextIsValid(context));

  old = CurrentMemoryContext;
  CurrentMemoryContext = context;

  return old;
}

// CreateGlobalMemory
//  Returns new global memory context.
//
// Note:
//  Assumes name is static.
//
// Exceptions:
//  BadState if called when disabled.
//  BadState if called outside TopMemoryContext (TopGlobalMemory).
//  BadArg if name is invalid.
GlobalMemory create_global_memory(char* name) {
  GlobalMemory context;
  MemoryContext savecxt;

  assert(MemoryContextEnabled);

  savecxt = memory_context_switch_to(TopMemoryContext);
  context = (GlobalMemory)new_node(sizeof(struct GlobalMemoryData), T_GlobalMemory);
  context->method = &GlobalContextMethodsData;
  context->name = name;

  alloc_set_init(&context->set_data, DynamicAllocMode, 0);
  ordered_elem_push_into(&context->elem_data, ActiveGlobalMemorySet);
  memory_context_switch_to(savecxt);

  return context;
}

// GlobalMemoryDestroy
//  Destroys given global memory context.
//
// Exceptions:
//  BadState if called when disabled.
//  BadState if called outside TopMemoryContext (TopGlobalMemory).
//  BadArg if context is invalid GlobalMemory.
//  BadArg if context is TopMemoryContext (TopGlobalMemory).
void global_memory_destroy(GlobalMemory context) {
  assert(MemoryContextEnabled);
  assert(IsA(context, GlobalMemory));
  assert(context != &TopGlobalMemoryData);

  alloc_set_reset(&context->set_data);

  ordered_elem_pop(&context->elem_data);
  memory_context_free(TopMemoryContext, (Pointer)context);
}

// GlobalMemoryAlloc
//  Returns pointer to aligned space in the global context.
//
// Exceptions:
//  ExhaustedMemory if allocation fails.
static Pointer global_memory_alloc(GlobalMemory mem, Size size) { return alloc_set_alloc(&mem->set_data, size); }

// GlobalMemoryFree
//  Frees allocated memory in the global context.
//
// Exceptions:
//  BadContextErr if current context is not the global context.
//  BadArgumentsErr if pointer is invalid.
static void global_memory_free(GlobalMemory mem, Pointer pointer) { alloc_set_free(&mem->set_data, pointer); }

// GlobalMemoryRealloc
//  Returns pointer to aligned space in the global context.
//
// Note:
//  Memory associated with the pointer is freed before return.
//
// Exceptions:
//  BadContextErr if current context is not the global context.
//  BadArgumentsErr if pointer is invalid.
//  NoMoreMemoryErr if allocation fails.
static Pointer global_memory_realloc(GlobalMemory mem, Pointer pointer, Size size) {
  return alloc_set_realloc(&mem->set_data, pointer, size);
}

// GlobalMemoryGetName
//  Returns name string for context.
//
// Exceptions:
//  ???
static char* global_memory_get_name(GlobalMemory mem) { return mem->name; }

// GlobalMemoryDump
//  Dumps global memory context for debugging.
//
// Exceptions:
//  ???
static void global_memory_dump(GlobalMemory mem) {
  GlobalMemory context;

  printf("--\n%s:\n", global_memory_get_name(mem));

  context = (GlobalMemory)ordered_elem_get_predecessor(&mem->elem_data);

  if (PointerIsValid(context)) {
    printf("\tpredecessor=%s\n", global_memory_get_name(context));
  }

  context = (GlobalMemory)ordered_elem_get_successor(&mem->elem_data);

  if (PointerIsValid(context)) {
    printf("\tsucessor=%s\n", global_memory_get_name(context));
  }

  alloc_set_dump(&mem->set_data);
}