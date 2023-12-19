//===----------------------------------------------------------------------===//
//
// shmem.c
//  Create shared memory and initialize shared memory data structures.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/ipc/shmem.c
//           v 1.50 2000/04/12 17:15:37 momjian Exp $
//
//===----------------------------------------------------------------------===//

// POSTGRES processes share one or more regions of shared memory.
// The shared memory is created by a postmaster and is inherited
// by each backends via fork(). The routines in this file are used for
// allocating and binding to shared memory data structures.
//
// NOTES:
//  (a) There are three kinds of shared memory data structures
//  available to POSTGRES: fixed-size structures, queues and hash
//  tables. Fixed-size structures contain things like global variables
//  for a module and should never be allocated after the process
//  initialization phase. Hash tables have a fixed maximum size, but
//  their actual size can vary dynamically. When entries are added
//  to the table, more space is allocated. Queues link data structures
//  that have been allocated either as fixed size structures or as hash
//  buckets. Each shared data structure has a string name to identify
//  it (assigned in the module that declares it).
//
//  (b) During initialization, each module looks for its
//  shared data structures in a hash table called the "Shmem Index".
//  If the data structure is not present, the caller can allocate
//  a new one and initialize it.  If the data structure is present,
//  the caller "attaches" to the structure by initializing a pointer
//  in the local address space.
//    The shmem index has two purposes: first, it gives us
//  a simple model of how the world looks when a backend process
//  initializes. If something is present in the shmem index,
//  it is initialized. If it is not, it is uninitialized. Second,
//  the shmem index allows us to allocate shared memory on demand
//  instead of trying to preallocate structures and hard-wire the
//  sizes and locations in header files. If you are using a lot
//  of shared memory in a lot of different places (and changing
//  things during development), this is important.
//
//  (c) memory allocation model: shared memory can never be
//  freed, once allocated. Each hash table has its own free list,
//  so hash buckets can be reused when an item is deleted.  However,
//  if one hash table grows very large and then shrinks, its space
//  cannot be redistributed to other tables. We could build a simple
//  hash bucket garbage collector if need be. Right now, it seems
//  unnecessary.
//
//    See InitSem() in sem.c for an example of how to use the
//  shmem index.

#include "rdbms/storage/shmem.h"

#include "rdbms/access/transam.h"
#include "rdbms/storage/ipc.h"
#include "rdbms/storage/spin.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/memutils.h"

// Shared memory global variables.
static PGShmemHeader* ShmemSegHdr;    // Shared mem segment header
static unsigned long ShmemEnd = 0;    // End address of shared memory
static HashTable* ShmemIndex = NULL;  // Primary index hashtable for shmem
static bool ShmemBootstrap = false;   // Bootstrapping shmem index?

unsigned long ShmemBase = 0;  // Start address of shared memory
SpinLock ShmemLock;           // Lock for shared memory allocation
SpinLock ShmemIndexLock;      // Lock for shmem index access

// set up shared-memory allocation and index table.
void init_shmem_allocation(PGShmemHeader* seg_hdr) {
  HashCtrl info;
  int hash_flags;
  ShmemIndexEnt* result;
  ShmemIndexEnt item;
  bool found;

  // Set up basic pointers to shared memory.
  ShmemSegHdr = seg_hdr;
  ShmemBase = (ShmemOffset)seg_hdr;
  ShmemEnd = ShmemBase + seg_hdr->total_size;

  // Since ShmemInitHash calls ShmemInitStruct, which expects the
  // ShmemIndex hashtable to exist already, we have a bit of a
  // circularity problem in initializing the ShmemIndex itself. We set
  // ShmemBootstrap to tell ShmemInitStruct to fake it.
  ShmemIndex = NULL;
  ShmemBootstrap = true;

  // Create the shared memory shmem index.
  info.keysize = SHMEM_INDEX_KEY_SIZE;
  info.datasize = SHMEM_INDEX_DATA_SIZE;
  hash_flags = HASH_ELEM;

  // This will acquire the shmem index lock, but not release it.
  ShmemIndex = shmem_init_hash("ShmemIndex", SHMEM_INDEX_SIZE, SHMEM_INDEX_SIZE, &info, hash_flags);

  if (!ShmemIndex) {
    elog(FATAL, "%s: couldn't initialize Shmem Index", __func__);
  }

  // Now, create an entry in the hashtable for the index itself.
  MEMSET(item.key, 0, SHMEM_INDEX_KEY_SIZE);
  strncpy(item.key, "ShmemIndex", SHMEM_INDEX_KEY_SIZE);

  result = (ShmemIndexEnt*)hash_search(ShmemIndex, (char*)&item, HASH_ENTER, &found);

  if (!result) {
    elog(FATAL, "%s: corrupted shmem index", __func__);
  }

  Assert(ShmemBootstrap && !found);

  result->location = MAKE_OFFSET(ShmemIndex->hctl);
  result->size = SHMEM_INDEX_SIZE;

  ShmemBootstrap = false;

  // Now release the lock acquired in ShmemInitStruct.
  spin_release(ShmemIndexLock);

  // Initialize ShmemVariableCache for transaction manager.
  ShmemVariableCache = (VariableCache)shmem_alloc(sizeof(*ShmemVariableCache));
  MEMSET(ShmemVariableCache, 0, sizeof(*ShmemVariableCache));
}

// Allocate max-aligned chunk from shared memory
//
// Assumes ShmemLock and ShmemSegHdr are initialized.
//
// Returns: real pointer to memory or NULL if we are out
// of space.  Has to return a real pointer in order
// to be compatible with malloc().
void* shmem_alloc(Size size) {
  uint32 new_free;
  void* new_space;

  // Ensure all space is adequately aligned.
  size = MAX_ALIGN(size);

  Assert(ShmemSegHdr);

  spin_acquire(ShmemLock);
  new_free = ShmemSegHdr->free_offset + size;

  if (new_free <= ShmemSegHdr->total_size) {
    new_space = (void*)MAKE_PTR(ShmemSegHdr->free_offset);
    ShmemSegHdr->free_offset = new_free;
  } else {
    new_space = NULL;
  }

  spin_release(ShmemLock);

  if (!new_space) {
    elog(NOTICE, "%s: out of memory", __func__);
  }

  return new_space;
}

// Test if an offset refers to valid shared memory
// Returns TRUE if the pointer is valid.
bool shmem_is_valid(unsigned long addr) { return (addr < ShmemEnd) && (addr >= ShmemBase); }

// Create/Attach to and initialize shared memory hash table.
//
// Notes:
//
// assume caller is doing some kind of synchronization
// so that two people dont try to create/initialize the
// table at once.  Use SpinAlloc() to create a spinlock
// for the structure before creating the structure itself.
// @param name:         table string name for shmem index
// @param init_size:    initial table size
// @param max_size:     max size of the table
// @param infop:        info about key and bucket size
// @param hash_flags:   info about infop
HashTable* shmem_init_hash(char* name, long init_size, long max_size, HashCtrl* infop, int hash_flags) {
  bool found;
  void* location;

  // Hash tables allocated in shared memory have a fixed directory; it
  // can't grow or other backends wouldn't be able to find it. So, make
  // sure we make it big enough to start with.
  //
  // The segbase is for calculating pointer values. The shared memory
  // allocator must be specified too.
  infop->dsize = infop->max_dsize = hash_select_dirsize(max_size);
  infop->segbase = (long*)ShmemBase;
  infop->alloc = shmem_alloc;
  hash_flags |= HASH_SHARED_MEM | HASH_DIRSIZE;

  // Look it up in the shmem index.
  location = shmem_init_struct(name, sizeof(HashHeader) + infop->dsize * sizeof(SegOffset), &found);

  // shmem index is corrupted. Let someone else give the error
  // message since they have more information.
  if (location == NULL) {
    return NULL;
  }

  // It already exists, attach to it rather than allocate and initialize
  // new space.
  if (found) {
    hash_flags |= HASH_ATTACH;
  }

  // Now provide the header and directory pointers.
  infop->hctl = (long*)location;
  infop->dir = (long*)(((char*)location) + sizeof(HashHeader));

  return hash_create(init_size, infop, hash_flags);
}

// Lookup process data structure using process id
//
// Returns: TRUE if no error. locationPtr is initialized if PID is
//          found in the shmem index.
//
// NOTES:
//  only information about success or failure is the value of
//  locationPtr.
bool shmem_pid_lookup(int pid, ShmemOffset* location_ptr) {
  ShmemIndexEnt* result;
  ShmemIndexEnt item;
  bool found;

  ASERT(ShmemIndex);

  MEMSET(item.key, 0, SHMEM_INDEX_KEY_SIZE);
  sprintf(item.key, "PID %d", pid);

  spin_acquire(ShmemIndexLock);
  result = (ShmemIndexEnt*)hash_search(ShmemIndex, (char*)&item, HASH_ENTER, &found);

  if (!result) {
    spin_release(ShmemIndexLock);
    elog(ERROR, "%s: ShmemIndex corrupted", __func__);

    return false;
  }

  if (found) {
    *location_ptr = result->location;
  } else {
    result->location = *location_ptr;
  }

  spin_release(ShmemIndexLock);

  return true;
}

// Destroy shmem index entry for process using process id
//
// Returns: offset of the process struct in shared memory or
// INVALID_OFFSET if not found.
//
// Side Effect: removes the entry from the shmem index
ShmemOffset shmem_pid_destroy(int pid) {
  ShmemIndexEnt* result;
  ShmemIndexEnt item;
  bool found;
  ShmemOffset location = 0;

  ASSERT(ShmemIndex);

  MEMSET(item.key, 0, SHMEM_INDEX_KEY_SIZE);
  sprintf(item.key, "PID %d", pid);

  spin_acquire(ShmemIndexLock);

  result = (ShmemIndexEnt*)hash_search(ShmemIndex, (char*)&item, HASH_REMOVE, &found);

  if (found) {
    location = result->location;
  }

  spin_release(ShmemIndexLock);

  if (!result) {
    elog(ERROR, "%s: PID table corrupted", __func__);
    return INVALID_OFFSET;
  }

  if (found) {
    return location;
  } else {
    return INVALID_OFFSET;
  }
}

// Create/attach to a structure in shared memory.
//
// This is called during initialization to find or allocate
// a data structure in shared memory. If no other processes
// have created the structure, this routine allocates space
// for it.  If it exists already, a pointer to the existing
// table is returned.
//
// Returns: real pointer to the object.  FoundPtr is TRUE if
// the object is already in the shmem index (hence, already
// initialized).
void* shmem_init_struct(char* name, Size size, bool* found_ptr) {
  ShmemIndexEnt* result;
  ShmemIndexEnt item;
  void* struct_ptr;

  strncpy(item.key, name, SHMEM_INDEX_KEY_SIZE);
  item.location = BAD_LOCATION;

  spin_acquire(ShmemIndexLock);

  if (!ShmemIndex) {
    // If the shmem index doesn't exist, we are bootstrapping: we must
    // be trying to init the shmem index itself.
    //
    // Notice that the ShmemIndexLock is held until the shmem index has
    // been completely initialized.
    ASSERT(strcmp(name, "ShmemIndex") == 0);
    ASSERT(ShmemBootstrap);
    *found_ptr = false;

    return shmem_alloc(size);
  }

  // Look it up in the shmem index.
  result = (ShmemIndexEnt*)hash_search(ShmemIndex, (char*)&item, HASH_ENTER, found_ptr);

  if (!result) {
    spin_release(ShmemIndexLock);
    elog(ERROR, "%s: Shmem Index corrupted", __func__);
    return NULL;
  }

  if (*found_ptr) {
    // Structure is in the shmem index so someone else has allocated
    // it already. The size better be the same as the size we are
    // trying to initialize to or there is a name conflict (or worse).
    if (result->size != size) {
      spin_release(ShmemIndexLock);
      elog(NOTICE, "%s: ShmemIndex entry size is wrong", __func__);
      // Let caller print its message too.
      return NULL;
    }

    struct_ptr = (void*)MAKE_PTR(result->location);
  } else {
    // It isn't in the table yet. allocate and initialize it.
    struct_ptr = shmem_alloc(size);

    if (!struct_ptr) {
      ASSERT(ShmemIndex);
      hash_search(ShmemIndex, (char*)&item, HASH_REMOVE, found_ptr);
      spin_release(ShmemIndexLock);
      *found_ptr = false;

      elog(NOTICE, "%s: cannot allocate '%s'", __func__, name);

      return NULL;
    }

    result->size = size;
    result->location = MAKE_OFFSET(struct_ptr);
  }

  ASSERT(shmem_is_valid((unsigned long)struct_ptr));

  spin_relase(ShmemIndexLock);

  return struct_ptr;
}
