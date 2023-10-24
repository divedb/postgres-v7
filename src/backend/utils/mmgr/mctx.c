#include "rdbms/utils/mctx.h"

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
