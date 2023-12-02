//===----------------------------------------------------------------------===//
//
// aset.c
//  Allocation set definitions.
//
// AllocSet is our standard implementation of the abstract MemoryContext
// type.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// IDENTIFICATION
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/utils/mmgr/aset.c,v 1.41 2001/03/22 04:00:07 momjian
// Exp $
//
// NOTE:
//  This is a new (Feb. 05, 1999) implementation of the allocation set
//  routines. AllocSet...() does not use OrderedSet...() any more.
//  Instead it manages allocations in a block pool by itself, combining
//  many small allocations in a few bigger blocks. AllocSetFree() normally
//  doesn't free() memory really. It just add's the free'd area to some
//  list for later reuse by AllocSetAlloc(). All memory blocks are free()'d
//  at once on AllocSetReset(), which happens when the memory context gets
//  destroyed.
//      Jan Wieck
//
//  Performance improvement from Tom Lane, 8/99: for extremely large request
//  sizes, we do want to be able to give the memory back to free() as soon
//  as it is pfree()'d.  Otherwise we risk tying up a lot of memory in
//  freelist entries that might never be usable.  This is specially needed
//  when the caller is repeatedly repalloc()'ing a block bigger and bigger;
//  the previous instances of the block were guaranteed to be wasted until
//  AllocSetReset() under the old way.
//
//  Further improvement 12/00: as the code stood, request sizes in the
//  midrange between "small" and "large" were handled very inefficiently,
//  because any sufficiently large free chunk would be used to satisfy a
//  request, even if it was much larger than necessary.  This led to more
//  and more wasted space in allocated chunks over time.  To fix, get rid
//  of the midrange behavior: we now handle only "small" power-of-2-size
//  chunks as chunks.  Anything "large" is passed off to malloc(). Change
//  the number of freelists to change the small/large boundary.
//
//
// About CLOBBER_FREED_MEMORY:
//
// If this symbol is defined, all freed memory is overwritten with 0x7F's.
// This is useful for catching places that reference already-freed memory.
//
// About MEMORY_CONTEXT_CHECKING:
//
// Since we usually round request sizes up to the next power of 2, there
// is often some unused space immediately after a requested data area.
// Thus, if someone makes the common error of writing past what they've
// requested, the problem is likely to go unnoticed ... until the day when
// there//isn't* any wasted space, perhaps because of different memory
// alignment on a new platform, or some other effect. To catch this sort
// of problem, the MEMORY_CONTEXT_CHECKING option stores 0x7E just beyond
// the requested space whenever the request is less than the actual chunk
// size, and verifies that the byte is undamaged when the chunk is freed.
//
//===----------------------------------------------------------------------===//

#include "rdbms/utils/elog.h"
#include "rdbms/utils/memutils.h"

// Chunk freelist k holds chunks of size 1 << (k + ALLOC_MINBITS),
// for k = 0 .. ALLOCSET_NUM_FREELISTS-1.
//
// Note that all chunks in the freelists have power-of-2 sizes.  This
// improves recyclability: we may waste some space, but the wasted space
// should stay pretty constant as requests are made and released.
//
// A request too large for the last freelist is handled by allocating a
// dedicated block from malloc().  The block still has a block header and
// chunk header, but when the chunk is freed we'll return the whole block
// to malloc(), not put it on our freelists.
//
// CAUTION: ALLOC_MINBITS must be large enough so that
// 1<<ALLOC_MINBITS is at least MAXALIGN,
// or we may fail to align the smallest chunks adequately.
// 16-byte alignment is enough on all currently known machines.
//
// With the current parameters, request sizes up to 8K are treated as chunks,
// larger requests go into dedicated blocks.  Change ALLOCSET_NUM_FREELISTS
// to adjust the boundary point.

#define ALLOC_MIN_BITS          4
#define ALLOC_SET_NUM_FREELISTS 10
#define ALLOC_CHUNK_LIMIT       (1 << (ALLOC_SET_NUM_FREELISTS - 1 + ALLOC_MIN_BITS))

// The first block allocated for an allocset has size initBlockSize.
// Each time we have to allocate another block, we double the block size
// (if possible, and without exceeding maxBlockSize), so as to reduce
// the bookkeeping load on malloc().
//
// Blocks allocated to hold oversize chunks do not follow this rule, however;
// they are just however big they need to be to hold that single chunk.
#define ALLOC_BLOCK_HDR_SZ MAX_ALIGN(sizeof(AllocBlockData))
#define ALLOC_CHUNK_HDR_SZ MAX_ALIGN(sizeof(AllocChunkData))

typedef struct AllocBlockData* AllocBlock;
typedef struct AllocChunkData* AllocChunk;

typedef void* AllocPointer;

// AllocSetContext is our standard implementation of MemoryContext.
typedef struct AllocSetContext {
  MemoryContextData header;                      // Standard memory-context fields.
  AllocBlock blocks;                             // Head of list of blocks in this set.
  AllocChunk freelist[ALLOC_SET_NUM_FREELISTS];  // Free chunk lists.
  Size init_block_size;                          // Initial block size.
  Size max_block_size;                           // Maximum block size.
  AllocBlock keeper;                             // If not NULL, keep this block over reset.
} AllocSetContext;

typedef AllocSetContext* AllocSet;

// AllocBlock
//  An AllocBlock is the unit of memory that is obtained by aset.c
//  from malloc(). It contains one or more AllocChunks, which are
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

// AllocChunk
//  The prefix of each piece of memory in an AllocBlock
//
// NB: this MUST match StandardChunkHeader as defined by utils/memutils.h
typedef struct AllocChunkData {
  void* aset;  // aset is the owning aset if allocated, or the freelist link if free.
  Size size;   //  size is always the size of the usable space in the chunk.

#ifdef MEMORY_CONTEXT_CHECKING
  // When debugging memory usage, also store actual requested size
  // this is zero in a free chunk.
  Size requested_size;
#endif

} AllocChunkData;

#define ALLOC_POINTER_IS_VALID(pointer)  POINTER_IS_VALID(pointer)
#define ALLOC_SET_IS_VALID(set)          POINTER_IS_VALID(set)
#define ALLOC_POINTER_GET_CHUNK(pointer) ((AllocChunk)(((char*)(pointer)) - ALLOC_CHUNK_HDR_SZ))
#define ALLOC_CHUNK_GET_POINTER(chunk)   ((AllocPointer)(((char*)(chunk)) + ALLOC_CHUNK_HDR_SZ))

// These functions implement the MemoryContext API for AllocSet contexts.
static void* alloc_set_alloc(MemoryContext context, Size size);
static void* alloc_set_free(MemoryContext context, void* pointer);
static void* alloc_set_realloc(MemoryContext context, void* pointer, Size size);
static void alloc_set_init(MemoryContext context);
static void alloc_set_reset(MemoryContext context);
static void alloc_set_delete(MemoryContext context);

#ifdef MEMORY_CONTEXT_CHECKING
static void alloc_set_check(MemoryContext context);
#endif

static void alloc_set_stats(MemoryContext context);

// This is the virtual function table for AllocSet contexts.
static MemoryContextMethods AllocSetMethods = {alloc_set_alloc, alloc_set_free,  alloc_set_realloc,
                                               alloc_set_init,  alloc_set_reset, alloc_set_delete,
#ifdef MEMORY_CONTEXT_CHECKING
                                               alloc_set_check,
#endif
                                               alloc_set_stats};

#ifdef HAVE_ALLOCINFO
#define ALLOC_FREE_INFO(cxt, chunk) \
  fprintf(stderr, "%s: %s: %p, %d\n", __func__, (cxt)->header.name, (chunk), (chunk)->size)
#define ALLOC_ALLOC_INFO(cxt, chunk) \
  fprintf(stderr, "%s: %s: %p, %d\n", __func__, (cxt)->header.name, (chunk), (chunk)->size)
#else
#define ALLOC_FREE_INFO(cxt, chunk)
#define ALLOC_ALLOC_INFO(cxt, chunk)
#endif

// Depending on the size of an allocation compute which freechunk
// list of the alloc set it belongs to. Caller must have verified
// that size <= ALLOC_CHUNK_LIMIT.
static inline int alloc_set_free_index(Size size) {
  int idx = 0;

  if (size > 0) {
    size = (size - 1) >> ALLOC_MIN_BITS;

    while (size != 0) {
      idx++;
      size >>= 1;
    }

    assert(idx < ALLOC_SET_NUM_FREELISTS);
  }

  return idx;
}

// Create a new AllocSet context.
//
// parent: parent context, or NULL if top-level context
// name: name of context (for debugging --- string will be copied)
// minContextSize: minimum context size
// initBlockSize: initial allocation block size
// maxBlockSize: maximum allocation block size
MemoryContext alloc_set_context_create(MemoryContext parent, const char* name, Size min_context_size,
                                       Size init_block_size, Size max_block_size) {
  AllocSet context;

  // Do the type-independent part of context creation.
  context = (AllocSet)memory_context_create(T_AllocSetContext, sizeof(AllocSetContext), &AllocSetMethods, parent, name);

  // Make sure alloc parameters are reasonable, and save them.
  //
  // We somewhat arbitrarily enforce a minimum 1K block size.
  init_block_size = MAX_ALIGN(init_block_size);

  if (init_block_size < 1024) {
    init_block_size = 1024;
  }

  max_block_size = MAX_ALIGN(max_block_size);

  if (max_block_size < init_block_size) {
    max_block_size = init_block_size;
  }

  context->init_block_size = init_block_size;
  context->max_block_size = max_block_size;

  // Grab always-allocated space, if requested.
  if (min_context_size > ALLOC_BLOCK_HDR_SZ + ALLOC_CHUNK_HDR_SZ) {
    Size blk_size = MAX_ALIGN(min_context_size);
    AllocBlock block;

    block = (AllocBlock)malloc(blk_size);

    if (block == NULL) {
    }
  }
}

// Returns pointer to allocated memory of given size; memory is added to the set.
static void* alloc_set_alloc(MemoryContext context, Size size) {
  AllocSet set = (AllocSet)context;
  AllocBlock block;
  AllocChunk chunk;
  AllocChunk prior_free;
  int fidx;
  Size chunk_size;
  Size blk_size;

  assert(ALLOC_SET_IS_VALID(set));

  // If requested size exceeds maximum for chunks, allocate an entire
  // block for this request.
  if (size > ALLOC_CHUNK_LIMIT) {
    chunk_size = MAX_ALIGN(size);
    blk_size = chunk_size + ALLOC_BLOCK_HDR_SZ + ALLOC_CHUNK_HDR_SZ;
    block = (AllocBlock)malloc(blk_size);

    if (block == NULL) {
      memory_context_stats(TopMemoryContext);
      elog(ERROR, "Memory exhausted in %s(%lu)", __func__, (unsigned long)size);
    }

    block->aset = set;
    block->freeptr = block->endptr = ((char*)block) + blk_size;
    chunk = (AllocChunk)(((char*)block) + ALLOC_BLOCK_HDR_SZ);
    chunk->aset = set;
    chunk->size = chunk_size;

#ifdef MEMORY_CONTEXT_CHECKING
    chunk->requested_size = size;
    // Set mark to catch clobber of "unused" space.
    if (size < chunk_size) {
      ((char*)ALLOC_CHUNK_GET_POINTER(chunk))[size] = 0x7E;
    }
#endif

    // Stick the new block underneath the active allocation block, so
    // that we don't lose the use of the space remaining therein.
    if (set->blocks != NULL) {
      block->next = set->blocks->next;
      set->blocks->next = block;
    } else {
      block->next = NULL;
      set->blocks = block;
    }

    ALLOC_ALLOC_INFO(set, chunk);

    return ALLOC_CHUNK_GET_POINTER(chunk);
  }

  // Request is small enough to be treated as a chunk.  Look in the
  // corresponding free list to see if there is a free chunk we could
  // reuse.
  fidx = alloc_set_free_index(size);
  prior_free = NULL;

  for (chunk = set->freelist[fidx]; chunk; chunk = (AllocChunk)chunk->aset) {
    if (chunk->size >= size) {
      break;
    }

    prior_free = chunk;
  }

  // If one is found, remove it from the free list, make it again a
  // member of the alloc set and return its data address.
  if (chunk != NULL) {
    if (prior_free == NULL) {
      set->freelist[fidx] = (AllocChunk)chunk->aset;
    } else {
      prior_free->aset = chunk->aset;
    }

    chunk->aset = (void*)set;

#ifdef MEMORY_CONTEXT_CHECKING
    chunk->requested_size = size;
    // Set mark to catch clobber of "unused" space.
    if (size < chunk->size) {
      ((char*)ALLOC_CHUNK_GET_POINTER(chunk))[size] = 0x7E;
    }
#endif

    ALLOC_ALLOC_INFO(set, chunk);

    return ALLOC_CHUNK_GET_POINTER(chunk);
  }

  // Choose the actual chunk size to allocate.
  chunk_size = 1 << (fidx + ALLOC_MIN_BITS);

  assert(chunk_size >= size);

  // If there is enough room in the active allocation block, we will put
  // the chunk into that block.  Else must start a new one.
  if ((block = set->blocks) != NULL) {
    Size avail_space = block->endptr - block->freeptr;

    if (avail_space < (chunk_size + ALLOC_CHUNK_HDR_SZ)) {
      // The existing active (top) block does not have enough room
      // for the requested allocation, but it might still have a
      // useful amount of space in it.  Once we push it down in the
      // block list, we'll never try to allocate more space from it.
      // So, before we do that, carve up its free space into chunks
      // that we can put on the set's freelists.
      //
      // Because we can only get here when there's less than
      // ALLOC_CHUNK_LIMIT left in the block, this loop cannot
      // iterate more than ALLOCSET_NUM_FREELISTS-1 times.
      while (avail_space >= ((1 << ALLOC_MIN_BITS) + ALLOC_CHUNK_HDR_SZ)) {
        Size avail_chunk = avail_space - ALLOC_CHUNK_HDR_SZ;
        int a_fidx = alloc_set_free_index(avail_chunk);

        // In most cases, we'll get back the index of the next
        // larger freelist than the one we need to put this chunk
        // on. The exception is when availchunk is exactly a
        // power of 2.
        if (avail_chunk != (1 << (a_fidx + ALLOC_MIN_BITS))) {
          a_fidx--;

          assert(a_fidx >= 0);

          avail_chunk = (1 << (a_fidx + ALLOC_MIN_BITS));
        }

        chunk = (AllocChunk)(block->freeptr);
        block->freeptr += (avail_chunk + ALLOC_CHUNK_HDR_SZ);
        avail_space -= (avail_chunk + ALLOC_CHUNK_HDR_SZ);
        chunk->size = avail_chunk;

#ifdef MEMORY_CONTEXT_CHECKING
        chunk->requested_size = 0;  // Mark it free.
#endif

        chunk->aset = (void*)set->freelist[a_fidx];
        set->freelist[a_fidx] = chunk;
      }

      // Mark that we need to create a new block.
      block = NULL;
    }
  }

  // Time to create a new regular (multi-chunk) block?
  if (block == NULL) {
    Size required_size;

    if (set->blocks == NULL) {
      // First block of the alloc set, use initBlockSize.
      blk_size = set->init_block_size;
    } else {
      // Get size of prior block.
      blk_size = set->blocks->endptr - ((char*)set->blocks);

      // Special case: if very first allocation was for a large
      // chunk (or we have a small "keeper" block), could have an
      // undersized top block. Do something reasonable.
      if (blk_size < set->init_block_size) {
        blk_size = set->init_block_size;
      } else {
        // Crank it up, but not past max.
        blk_size <<= 1;

        if (blk_size > set->max_block_size) {
          blk_size = set->max_block_size;
        }
      }
    }

    // If initBlockSize is less than ALLOC_CHUNK_LIMIT, we could need
    // more space...
    required_size = chunk_size + ALLOC_BLOCK_HDR_SZ + ALLOC_CHUNK_HDR_SZ;

    if (blk_size < required_size) {
      blk_size = required_size;
    }

    block = (AllocBlock)malloc(blk_size);

    // We could be asking for pretty big blocks here, so cope if
    // malloc fails.  But give up if there's less than a meg or so
    // available...
    while (block == NULL && blk_size > 1024 * 1024) {
      blk_size >>= 1;
      if (blk_size < required_size) {
        break;
      }
      block = (AllocBlock)malloc(blk_size);
    }

    if (block == NULL) {
      memory_context_stats(TopMemoryContext);
      elog(ERROR, "Memory exhausted in %s(%lu)", __func__, (unsigned long)size);
    }

    block->aset = set;
    block->freeptr = ((char*)block) + ALLOC_BLOCK_HDR_SZ;
    block->endptr = ((char*)block) + blk_size;
    block->next = set->blocks;
    set->blocks = block;
  }

  chunk = (AllocChunk)(block->freeptr);
  block->freeptr += (chunk_size + ALLOC_CHUNK_HDR_SZ);

  assert(block->freeptr <= block->endptr);

  chunk->aset = (void*)set;
  chunk->size = chunk_size;

#ifdef MEMORY_CONTEXT_CHECKING
  chunk->requested_size = size;
  /* set mark to catch clobber of "unused" space */
  if (size < chunk->size) {
    ((char*)ALLOC_CHUNK_GET_POINTER(chunk))[size] = 0x7E;
  }
#endif

  ALLOC_ALLOC_INFO(set, chunk);

  return ALLOC_CHUNK_GET_POINTER(chunk);
}

static void* alloc_set_free(MemoryContext context, void* pointer) {
  AllocSet set = (AllocSet)context;
  AllocChunk chunk = ALLOC_POINTER_GET_CHUNK(pointer);

  ALLOC_FREE_INFO(set, chunk);

#ifdef MEMORY_CONTEXT_CHECKING
  // Test for someone scribbling on unused space in chunk.
  if (chunk->requested_size < chunk->size) {
    if (((char*)pointer)[chunk->requested_size] != 0x7E) {
      elog(NOTICE, "%s: detected write past chunk end in %s %p", __func__, set->header.name, chunk);
    }
  }
#endif

  if (chunk->size > ALLOC_CHUNK_LIMIT) {
    // Big chunks are certain to have been allocated as single-chunk
    // blocks. Find the containing block and return it to malloc().
    AllocBlock block = set->blocks;
    AllocBlock prev_block = NULL;

    while (block != NULL) {
      if (chunk == (AllocChunk)(((char*)block) + ALLOC_BLOCK_HDR_SZ)) {
        break;
      }

      prev_block = block;
      block = block->next;
    }

    if (block == NULL) {
      elog(ERROR, "%s: cannot find block containing chunk %p", __func__, chunk);
    }

    // let's just make sure chunk is the only one in the block.
    assert(block->freeptr == ((char*)block) + (chunk->size + ALLOC_BLOCK_HDR_SZ + ALLOC_CHUNK_HDR_SZ));

    // OK, remove block from aset's list and free it.
    if (prev_block == NULL) {
      set->blocks = block->next;
    } else {
      prev_block->next = block->next;
    }

#ifdef CLOBBER_FREED_MEMORY
    // Wipe freed memory for debugging purposes.
    memset(block, 0x7F, block->freeptr - ((char*)block));
#endif

    // TODO(gc): why free here?
    free(block);
  } else {
    // Normal case, put the chunk into appropriate freelist.
    int fidx = alloc_set_free_index(chunk->size);
    chunk->aset = (void*)set->freelist[fidx];

#ifdef CLOBBER_FREED_MEMORY
    // Wipe freed memory for debugging purposes.
    memset(pointer, 0x7F, chunk->size);
#endif

#ifdef MEMORY_CONTEXT_CHECKING
    // Reset requested_size to 0 in chunks that are on freelist.
    chunk->requested_size = 0;
#endif

    set->freelist[fidx] = chunk;
  }
}

// Returns new pointer to allocated memory of given size; this memory
// is added to the set. Memory associated with given pointer is copied
// into the new memory, and the old memory is freed.
static void* alloc_set_realloc(MemoryContext context, void* pointer, Size size) {
  AllocSet set = (AllocSet)context;
  AllocChunk chunk = ALLOC_POINTER_GET_CHUNK(pointer);
  Size old_size = chunk->size;

#ifdef MEMORY_CONTEXT_CHECKING
  /* Test for someone scribbling on unused space in chunk */
  if (chunk->requested_size < oldsize) {
    if (((char*)pointer)[chunk->requested_size] != 0x7E) {
      elog(NOTICE, "%s: detected write past chunk end in %s %p", __func__, set->header.name, chunk);
    }
  }
#endif

  // Chunk sizes are aligned to power of 2 in AllocSetAlloc(). Maybe the
  // allocated area already is >= the new size.  (In particular, we
  // always fall out here if the requested size is a decrease.)
  if (old_size >= size) {
#ifdef MEMORY_CONTEXT_CHECKING
    chunk->requested_size = size;

    if (size < oldsize) {
      ((char*)pointer)[size] = 0x7E;
    }
#endif

    return pointer;
  }

  if (old_size > ALLOC_CHUNK_LIMIT) {
    // The chunk must been allocated as a single-chunk block.  Find
    // the containing block and use realloc() to make it bigger with
    // minimum space wastage.
    AllocBlock block = set->blocks;
    AllocBlock prev_block = NULL;
    Size chunk_size;
    Size blk_size;

    while (block != NULL) {
      if (chunk == (AllocChunk)(((char*)block) + ALLOC_BLOCK_HDR_SZ)) {
        break;
      }

      prev_block = block;
      block = block->next;
    }

    if (block == NULL) {
      elog(ERROR, "%s: cannot find block containing chunk %p", __func__, chunk);
    }

    // Let's just make sure chunk is the only one in the block.
    assert(block->freeptr == ((char*)block) + (chunk->size + ALLOC_BLOCK_HDR_SZ + ALLOC_CHUNK_HDR_SZ));

    chunk_size = MAX_ALIGN(size);
    blk_size = chunk_size + ALLOC_BLOCK_HDR_SZ + ALLOC_CHUNK_HDR_SZ;
    block = (AllocBlock)realloc(block, blk_size);

    if (block == NULL) {
      memory_context_stats(TopMemoryContext);
      elog(ERROR, "Memory exhausted in %s(%lu)", __func__, (unsigned long)size);
    }

    block->freeptr = block->endptr = ((char*)block) + blk_size;

    // Update pointers since block has likely been moved.
    chunk = (AllocChunk)(((char*)block) + ALLOC_BLOCK_HDR_SZ);

    if (prev_block == NULL) {
      set->blocks = block;
    } else {
      prev_block->next = block;
    }

    chunk->size = chunk_size;

#ifdef MEMORY_CONTEXT_CHECKING
    chunk->requested_size = size;

    if (size < chunk->size) {
      ((char*)ALLOC_CHUNK_GET_POINTER(chunk))[size] = 0x7E;
    }
#endif

    return ALLOC_CHUNK_GET_POINTER(chunk);
  } else {
    // Small-chunk case. If the chunk is the last one in its block,
    // there might be enough free space after it that we can just
    // enlarge the chunk in-place. It's relatively painful to find
    // the containing block in the general case, but we can detect
    // last-ness quite cheaply for the typical case where the chunk is
    // in the active (topmost) allocation block.  (At least with the
    // regression tests and code as of 1/2001, realloc'ing the last
    // chunk of a non-topmost block hardly ever happens, so it's not
    // worth scanning the block list to catch that case.)
    //
    // NOTE: must be careful not to create a chunk of a size that
    // AllocSetAlloc would not create, else we'll get confused later.
    AllocPointer new_pointer;

    if (size <= ALLOC_CHUNK_LIMIT) {
      AllocBlock block = set->blocks;
      char* chunk_end;

      chunk_end = (char*)chunk + (old_size + ALLOC_CHUNK_HDR_SZ);

      if (chunk_end == block->freeptr) {
        Size free_space = block->endptr - block->freeptr;
        int fidx;
        Size new_size;
        Size delta;

        fidx = alloc_set_free_index(size);
        new_size = 1 << (fidx + ALLOC_MIN_BITS);

        assert(new_size >= old_size);

        delta = new_size - old_size;

        if (free_space >= delta) {
          block->freeptr += delta;
          chunk->size += delta;

#ifdef MEMORY_CONTEXT_CHECKING
          chunk->requested_size = size;

          if (size < chunk->size) {
            ((char*)pointer)[size] = 0x7E;
          }
#endif

          return pointer;
        }
      }
    }

    new_pointer = alloc_set_alloc((MemoryContext)set, size);
    memcpy(new_pointer, pointer, old_size);
    alloc_set_free((MemoryContext)set, pointer);

    return new_pointer;
  }
}

// This is called by MemoryContextCreate() after setting up the
// generic MemoryContext fields and before linking the new context
// into the context tree.  We must do whatever is needed to make the
// new context minimally valid for deletion.  We must *not* risk
// failure --- thus, for example, allocating more memory is not cool.
// (AllocSetContextCreate can allocate memory when it gets control
// back, however.)
static void alloc_set_init(MemoryContext context) {}

// Frees all memory which is allocated in the given set.
//
// Actually, this routine has some discretion about what to do.
// It should mark all allocated chunks freed, but it need not
// necessarily give back all the resources the set owns.  Our
// actual implementation is that we hang on to any "keeper"
// block specified for the set.
static void alloc_set_reset(MemoryContext context) {
  AllocSet set = (AllocSet)context;
  AllocBlock block = set->blocks;

  assert(ALLOC_SET_IS_VALID(set));

#ifdef MEMORY_CONTEXT_CHECKING
  // Check for corruption and leaks before freeing.
  alloc_set_check(context);
#endif

  // Clear chunk freelists.
  MEMSET(set->freelist, 0, sizeof(set->freelist));

  // New blocks list is either empty or just the keeper block.
  set->blocks = set->keeper;

  while (block != NULL) {
    AllocBlock next = block->next;

    if (block == set->keeper) {
      // Reset the block, but don't return it to malloc.
      char* data_start = ((char*)block) + ALLOC_BLOCK_HDR_SZ;

#ifdef CLOBBER_FREED_MEMORY
      // Wipe freed memory for debugging purposes.
      memset(data_start, 0x7F, block->freeptr - data_start);
#endif
      block->freeptr = data_start;
      block->next = NULL;
    } else {
      // Normal case, release the block.
#ifdef CLOBBER_FREED_MEMORY
      memset(block, 0x7F, block->freeptr - ((char*)block));
#endif
      free(block);
    }

    block = next;
  }
}

// Frees all memory which is allocated in the given set,
// in preparation for deletion of the set.
//
// Unlike AllocSetReset, this *must* free all resources of the set.
// But note we are not responsible for deleting the context node itself.
static void alloc_set_delete(MemoryContext context) {
  AllocSet set = (AllocSet)context;
  AllocBlock block = set->blocks;

  assert(ALLOC_SET_IS_VALID(set));

#ifdef MEMORY_CONTEXT_CHECKING
  alloc_set_check(context);
#endif

  MEMSET(set->freelist, 0, sizeof(set->freelist));
  set->blocks = NULL;
  set->keeper = NULL;

  while (block != NULL) {
    AllocBlock next = block->next;

#ifdef CLOBBER_FREED_MEMORY
    memset(block, 0x7F, block->freeptr - ((char*)block));
#endif

    free(block);
    block = next;
  }
}