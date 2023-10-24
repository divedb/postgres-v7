// =========================================================================
//
// aset.c
//  Allocation set definitions.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/utils/mmgr/aset.c,v 1.26 2000/04/12 17:16:09 momjian Exp $
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
//                Jan Wieck
//
//  Performance improvement from Tom Lane, 8/99: for extremely large request
//  sizes, we do want to be able to give the memory back to free() as soon
//  as it is pfree()'d.  Otherwise we risk tying up a lot of memory in
//  freelist entries that might never be usable.  This is specially needed
//  when the caller is repeatedly repalloc()'ing a block bigger and bigger;
//  the previous instances of the block were guaranteed to be wasted until
//  AllocSetReset() under the old way.
// =========================================================================

#include "rdbms/utils/aset.h"

#include <assert.h>
#include <stdbool.h>

#include "rdbms/utils/memutils.h"

// ====================
// Chunk freelist k holds chunks of size 1 << (k + ALLOC_MINBITS),
// for k = 0 .. ALLOCSET_NUM_FREELISTS=2.
// The last freelist holds all larger free chunks.    Those chunks come in
// varying sizes depending on the request size, whereas smaller chunks are
// coerced to powers of 2 to improve their "recyclability".
//
// CAUTION: ALLOC_MINBITS must be large enough so that
// 1<<ALLOC_MINBITS is at least MAXALIGN,
// or we may fail to align the smallest chunks adequately.
// 16=byte alignment is enough on all currently known machines.
// ====================

#define ALLOC_MINBITS 4  // Smallest chunk size is 16 bytes
#define ALLOC_SMALLCHUNK_LIMIT \
  (1 << (ALLOCSET_NUM_FREELISTS - 2 + ALLOC_MINBITS))  // Size of largest chunk that we use a fixed size for

// ====================
// The first block allocated for an allocset has size ALLOC_MIN_BLOCK_SIZE.
// Each time we have to allocate another block, we double the block size
// (if possible, and without exceeding ALLOC_MAX_BLOCK_SIZE), so as to reduce
// the bookkeeping load on malloc().
//
// Blocks allocated to hold oversize chunks do not follow this rule, however;
// they are just however big they need to be to hold that single chunk.
// AllocSetAlloc has some freedom about whether to consider a chunk larger
// than ALLOC_SMALLCHUNK_LIMIT to be "oversize".  We require all chunks
// >= ALLOC_BIGCHUNK_LIMIT to be allocated as single=chunk blocks; those
// chunks are treated specially by AllocSetFree and AllocSetRealloc.  For
// request sizes between ALLOC_SMALLCHUNK_LIMIT and ALLOC_BIGCHUNK_LIMIT,
// AllocSetAlloc has discretion whether to put the request into an existing
// block or make a single=chunk block.
//
// We must have ALLOC_MIN_BLOCK_SIZE > ALLOC_SMALLCHUNK_LIMIT and
// ALLOC_BIGCHUNK_LIMIT > ALLOC_SMALLCHUNK_LIMIT.
// ====================

#define ALLOC_MIN_BLOCK_SIZE (8 * 1024)
#define ALLOC_MAX_BLOCK_SIZE (8 * 1024 * 1024)

#define ALLOC_BIGCHUNK_LIMIT (64 * 1024)  // Chunks >= ALLOC_BIGCHUNK_LIMIT are immediately free()d by pfree()

#define ALLOC_BLOCKHDRSZ MAXALIGN(sizeof(AllocBlockData))
#define ALLOC_CHUNKHDRSZ MAXALIGN(sizeof(AllocChunkData))

#define AllocPointerGetChunk(ptr) ((AllocChunk)(((char*)(ptr)) - ALLOC_CHUNKHDRSZ))
#define AllocChunkGetPointer(chk) ((AllocPointer)(((char*)(chk)) + ALLOC_CHUNKHDRSZ))
#define AllocPointerGetAset(ptr)  ((AllocSet)(AllocPointerGetChunk(ptr)->aset))
#define AllocPointerGetSize(ptr)  (AllocPointerGetChunk(ptr)->size)

void alloc_set_init(AllocSet set, AllocMode mode, Size limit) {
  assert(PointerIsValid(set));
  assert((int)DynamicAllocMode <= (int)mode);
  assert((int)mode <= (int)BoundedAllocMode);

  memset(set, 0, sizeof(AllocSetData));
}

void alloc_set_reset(AllocSet set) {
  assert(AllocSetIsValid(set));

  AllocBlock block = set->blocks;
  AllocBlock next;

  while (block != NULL) {
    next = block->next;
    free(block);
    block = next;
  }

  memset(set, 0, sizeof(AllocSetData));
}

bool alloc_set_contains(AllocSet set, AllocPointer pointer) {
  assert(AllocSetIsValid(set));
  assert(AllocPointerIsValid(pointer));

  return AllocPointerGetAset(pointer) == set;
}

// Depending on the size of an allocation compute which freechunk
// list of the alloc set it belongs to.
// 0th chunk: <= 16   bytes.
// 1th chunk: <= 32   bytes.
// 2th chunk: <= 64   bytes.
// 3th chunk: <= 128  bytes.
// 4th chunk: <= 256  bytes.
// 5th chunk: <= 512  bytes.
// 6th chunk: <= 1024 bytes.
// 7th chunk: <= 2048 bytes.
static inline int alloc_set_free_index(Size size) {
  int idx = 0;

  if (size > 0) {
    size = (size - 1) >> ALLOC_MINBITS;

    while (size != 0 && idx < ALLOCSET_NUM_FREELISTS - 1) {
      idx++;
      size >>= 1;
    }
  }

  return idx;
}

AllocPointer alloc_set_alloc(AllocSet set, Size size) {
  assert(AllocSetIsValid(set));

  AllocBlock block;
  AllocChunk chunk;
  AllocChunk prior_free = NULL;
  int fidx;
  Size chunk_size;
  Size blksize;

  // Lookup in the corresponding free list if there is a free chunk we could reuse.
  fidx = alloc_set_free_index(size);

  for (chunk = set->freelist[fidx]; chunk != NULL; chunk = (AllocChunk)chunk->aset) {
    if (chunk->size >= size) {
      break;
    }

    prior_free = chunk;
  }

  // If one is found, remove it from the free list, make it again a
  // member of the alloc set and return its data address.
  // 需要注意Chunk的aset字段 既可以指向next 也可以指向set
  if (chunk != NULL) {
    if (prior_free == NULL) {
      set->freelist[fidx] = (AllocChunk)chunk->aset;
    } else {
      prior_free->aset = (void*)set;
    }

    chunk->aset = (void*)set;

    return AllocChunkGetPointer(chunk);
  }

  // Choose the actual chunk size to allocate.
  if (size > ALLOC_SMALLCHUNK_LIMIT) {
    chunk_size = MAXALIGN(size);
  } else {
    chunk_size = 1 << (fidx + ALLOC_MINBITS);
  }

  assert(chunk_size >= size);

  // If there is enough room in the active allocation block, and the
  // chunk is less than ALLOC_BIGCHUNK_LIMIT, put the chunk into the
  // active allocation block.
  if ((block = set->blocks) != NULL) {
    Size have_free = block->endptr - block->freeptr;

    if (have_free < (chunk_size + ALLOC_CHUNKHDRSZ) || chunk_size >= ALLOC_BIGCHUNK_LIMIT) {
      block = NULL;
    }
  }

  // Otherwise, if requested size exceeds smallchunk limit, allocate an
  // entire separate block for this allocation.  In particular, we will
  // always take this path if the requested size exceeds bigchunk limit.
  if (block == NULL && size > ALLOC_SMALLCHUNK_LIMIT) {
    blksize = chunk_size + ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ;
    block = (AllocBlock)malloc(blksize);

    if (block == NULL) {
      // TODO(gc): change to elog later.
      fprintf(stderr, "Memory exhausted in AllocSetAlloc()");
      exit(EXIT_FAILURE);
    }

    block->aset = set;
    block->freeptr = block->endptr = ((char*)block) + blksize;

    chunk = (AllocChunk)(((char*)block) + ALLOC_BLOCKHDRSZ);
    chunk->aset = set;
    chunk->size = chunk_size;

    // Try to stick the block underneath the active allocation block,
    // so that we don't lose the use of the space remaining therein.
    if (set->blocks != NULL) {
      block->next = set->blocks->next;
      set->blocks->next = block;
    } else {
      block->next = NULL;
      set->blocks = block;
    }

    return AllocChunkGetPointer(chunk);
  }

  // Time to create a new regular block.
  if (block == NULL) {
    if (set->blocks == NULL) {
      blksize = ALLOC_MIN_BLOCK_SIZE;
      block = (AllocBlock)malloc(blksize);
    } else {
      blksize = set->blocks->endptr - ((char*)set->blocks);

      // Special case: if very first allocation was for a large
      // chunk, could have a funny-sized top block.  Do something
      // reasonable.
      if (blksize < ALLOC_MIN_BLOCK_SIZE) {
        blksize = ALLOC_MIN_BLOCK_SIZE;
      }

      blksize <<= 1;
      if (blksize > ALLOC_MAX_BLOCK_SIZE) {
        blksize = ALLOC_MAX_BLOCK_SIZE;
      }

      block = (AllocBlock)malloc(blksize);

      // We could be asking for pretty big blocks here, so cope if
      // malloc fails.  But give up if there's less than a meg or so
      // available...
      while (block == NULL && blksize > 1024 * 1024) {
        blksize >>= 1;
        block = (AllocBlock)malloc(blksize);
      }
    }

    if (block == NULL) {
      // TODO(gc): change to elog later.
      fprintf(stderr, "Memory exhausted in AllocSetAlloc()");
      exit(EXIT_FAILURE);
    }

    block->aset = set;
    block->freeptr = ((char*)block) + ALLOC_BLOCKHDRSZ;
    block->endptr = ((char*)block) + blksize;
    block->next = set->blocks;

    set->blocks = block;
  }

  // OK, do the allocation.
  chunk = (AllocChunk)(block->freeptr);
  chunk->aset = (void*)set;
  chunk->size = chunk_size;
  block->freeptr += (chunk_size + ALLOC_CHUNKHDRSZ);
  assert(block->freeptr <= block->endptr);

  return AllocChunkGetPointer(chunk);
}

// Frees allocated memory; memory is removed from the set.
void alloc_set_free(AllocSet set, AllocPointer pointer) {
  assert(alloc_set_contains(set, pointer));

  AllocChunk chunk;
  chunk = AllocPointerGetChunk(pointer);

  if (chunk->size >= ALLOC_BIGCHUNK_LIMIT) {
    // Big chunks are certain to have been allocated as single-chunk
    // blocks.	Find the containing block and return it to malloc().
    AllocBlock block = set->blocks;
    AllocBlock prevblock = NULL;

    while (block != NULL) {
      if (chunk == (AllocChunk)(((char*)block) + ALLOC_BLOCKHDRSZ)) {
        break;
      }

      prevblock = block;
      block = block->next;
    }

    if (block == NULL) {
      fprintf(stderr, "AllocSetFree: cannot find block containing chunk");
      exit(EXIT_FAILURE);
    }

    assert(block->freeptr == ((char*)block) + (chunk->size + ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ));

    if (prevblock == NULL) {
      set->blocks = block->next;
    } else {
      prevblock->next = block->next;
    }

    free(block);
  } else {
    int fidx = alloc_set_free_index(chunk->size);

    chunk->aset = (void*)set->freelist[fidx];
    set->freelist[fidx] = chunk;
  }
}

// Returns new pointer to allocated memory of given size; this memory
// is added to the set.  Memory associated with given pointer is copied
// into the new memory, and the old memory is freed.
//
// Exceptions:
//  BadArg if set is invalid.
//  BadArg if pointer is invalid.
//  BadArg if pointer is not member of set.
//  MemoryExhausted if allocation fails.
AllocPointer alloc_set_realloc(AllocSet set, AllocPointer pointer, Size size) {
  Size old_size;

  assert(alloc_set_contains(set, pointer));

  // Chunk sizes are aligned to power of 2 on AllocSetAlloc(). Maybe the
  // allocated area already is >= the new size.  (In particular, we
  // always fall out here if the requested size is a decrease.)
  old_size = AllocPointerGetSize(pointer);

  if (old_size >= size) {
    return pointer;
  }

  if (old_size >= ALLOC_BIGCHUNK_LIMIT) {
    // If the chunk is already >= bigchunk limit, then it must have
    // been allocated as a single-chunk block.	Find the containing
    // block and use realloc() to make it bigger with minimum space
    // wastage.
    AllocChunk chunk = AllocPointerGetChunk(pointer);
    AllocBlock block = set->blocks;
    AllocBlock prevblock = NULL;
    Size blksize;

    while (block != NULL) {
      if (chunk == (AllocChunk)(((char*)block) + ALLOC_BLOCKHDRSZ)) {
        break;
      }

      prevblock = block;
      block = block->next;
    }

    if (block == NULL) {
      fprintf(stderr, "AllocSetRealloc: cannot find block containing chunk");
      exit(EXIT_FAILURE);
    }

    assert(block->freeptr == ((char*)block) + (chunk->size + ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ));

    size = MAXALIGN(size);
    blksize = size + ALLOC_BLOCKHDRSZ + ALLOC_CHUNKHDRSZ;
    block = (AllocBlock)realloc(block, blksize);
    if (block == NULL) {
      fprintf(stderr, "Memory exhausted in AllocSetReAlloc()");
      exit(EXIT_FAILURE);
    }

    block->freeptr = block->endptr = ((char*)block) + blksize;
    chunk = (AllocChunk)(((char*)block) + ALLOC_BLOCKHDRSZ);

    if (prevblock == NULL) {
      set->blocks = block;
    } else {
      prevblock->next = block;
    }

    chunk->size = size;

    return AllocChunkGetPointer(chunk);
  } else {
    // Normal small-chunk case: just do it by brute force.
    AllocPointer new_pointer = alloc_set_alloc(set, size);
    memcpy(new_pointer, pointer, old_size);
    alloc_set_free(set, pointer);

    return new_pointer;
  }
}