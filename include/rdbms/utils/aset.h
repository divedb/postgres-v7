#ifndef RDBMS_UTILS_ASET_H_
#define RDBMS_UTILS_ASET_H_

#include <stdbool.h>

#include "rdbms/c.h"

// Description:
//  An allocation set is a set containing allocated elements.  When
//  an allocation is requested for a set, memory is allocated and a
//  pointer is returned.  Subsequently, this memory may be freed or
//  reallocated.  In addition, an allocation set may be reset which
//  will cause all memory allocated within it to be freed.
//
//  XXX The following material about allocation modes is all OUT OF DATE.
//  aset.c currently implements only one allocation strategy,
//  DynamicAllocMode, and that's the only one anyone ever requests anyway.
//  If we ever did have more strategies, the new ones might or might
//  not look like what is described here...
//
//  Allocations may occur in four different modes.  The mode of
//  allocation does not affect the behavior of allocations except in
//  terms of performance.  The allocation mode is set at the time of
//  set initialization.  Once the mode is chosen, it cannot be changed
//  unless the set is reinitialized.
//
//  "Dynamic" mode forces all allocations to occur in a heap.  This
//  is a good mode to use when small memory segments are allocated
//  and freed very frequently.  This is a good choice when allocation
//  characteristics are unknown.  This is the default mode.
//
//  "Static" mode attempts to allocate space as efficiently as possible
//  without regard to freeing memory.  This mode should be chosen only
//  when it is known that many allocations will occur but that very
//  little of the allocated memory will be explicitly freed.
//
//  "Tunable" mode is a hybrid of dynamic and static modes.  The
//  tunable mode will use static mode allocation except when the
//  allocation request exceeds a size limit supplied at the time of set
//  initialization.  "Big" objects are allocated using dynamic mode.
//
//  "Bounded" mode attempts to allocate space efficiently given a limit
//  on space consumed by the allocation set.  This restriction can be
//  considered a "soft" restriction, because memory segments will
//  continue to be returned after the limit is exceeded.  The limit is
//  specified at the time of set initialization like for tunable mode.
//
// Note:
//  Allocation sets are not automatically reset on a system reset.
//  Higher level code is responsible for cleaning up.
//
//  There may be other modes in the future.

// Aligned pointer which may be a member of an allocation set.
typedef Pointer AllocPointer;

//     Mode of allocation for an allocation set.
typedef enum AllocMode {
  DynamicAllocMode,  // Always dynamically allocate
  StaticAllocMode,   // Always "statically" allocate
  TunableAllocMode,  // Allocations are "tuned"
  BoundedAllocMode   // Allocations bounded to fixed usage
} AllocMode;

#define DefaultAllocMode DynamicAllocMode

typedef struct AllocSetData* AllocSet;
typedef struct AllocBlockData* AllocBlock;
typedef struct AllocChunkData* AllocChunk;

typedef struct AllocSetData {
  AllocBlock blocks;  // Head of list of blocks in this set.
#define ALLOCSET_NUM_FREELISTS 8
  AllocChunk freelist[ALLOCSET_NUM_FREELISTS];  // Free chunk lists.
} AllocSetData;

// AllocBlock
//  An AllocBlock is the unit of memory that is obtained by aset.c
//  from malloc().    It contains one or more AllocChunks, which are
//  the units requested by palloc() and freed by pfree().  AllocChunks
//  cannot be returned to malloc() individually, instead they are put
//  on freelists by pfree() and re-used by the next palloc() that has
//  a matching request size.
//
//  AllocBlockData is the header data for a block --- the usable space
//  within the block begins at the next alignment boundary.
typedef struct AllocBlockData {
  AllocSet aset;    // aset that owns this block
  AllocBlock next;  // Next block in aset's blocks list
  char* freeptr;    // Start of free space in this block
  char* endptr;     // End of space in this block
} AllocBlockData;

// The prefix of each piece of memory in an AllocBlock
typedef struct AllocChunkData {
  void* aset;  // aset is the owning aset if allocated, or the freelist link if free.
  Size size;   // size is always the size of the usable space in the chunk.
} AllocChunkData;

#define AllocPointerIsValid(pointer) PointerIsValid(pointer)
#define AllocSetIsValid(set)         PointerIsValid(set)

void alloc_set_init(AllocSet set, AllocMode mode, Size limit);
void alloc_set_reset(AllocSet set);
bool alloc_set_contains(AllocSet set, AllocPointer pointer);
AllocPointer alloc_set_alloc(AllocSet set, Size size);
void alloc_set_free(AllocSet set, AllocPointer pointer);
AllocPointer alloc_set_realloc(AllocSet set, AllocPointer pointer, Size size);
void alloc_set_dump(AllocSet set);

#endif  // RDBMS_UTILS_ASET_H_
