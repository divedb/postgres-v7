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
#include "rdbms/utils/memutils.h"

// Shared memory global variables.
static PgShmemHeader* ShmemSegHdr;   // Shared mem segment header
unsigned long ShmemBase = 0;         // Start address of shared memory
static unsigned long ShmemEnd = 0;   // End address of shared memory.
static unsigned long ShmemSize = 0;  // Current size (and default).

extern VariableCache ShmemVariableCache;  // varsup.c

SpinLock ShmemLock;       // Lock for shared memory allocation.
SpinLock ShmemIndexLock;  // Lock for shmem index access.

static unsigned long* ShmemFreeStart = NULL;    // Pointer to the OFFSET of first free shared memory.
static unsigned long* ShmemIndexOffset = NULL;  // Start of the shmem index table (for bootstrap).
static bool ShmemBootstrap = false;             // Flag becomes true when shared mem is created by POSTMASTER.
static HTAB* ShmemIndex = NULL;

// Resets the shmem index to NULL....
// useful when the postmaster destroys existing shared memory
// and creates all new segments after a backend crash.
void shmem_index_reset() { ShmemIndex = NULL; }

// This routine is called once by the postmaster to
// initialize the shared buffer pool.	Assume there is
// only one postmaster so no synchronization is necessary
// until after this routine completes successfully.
//
// key is a unique identifier for the shmem region.
// size is the size of the region.
static IpcMemoryId ShmemId;

void shmem_create(unsigned int key, unsigned int size) {
  if (size) {
    ShmemSize = size;
  }

  // Create shared mem region.
  if ((ShmemId = IpcMemoryCreate(key, ShmemSize, IPCProtection)) == IpcMemCreationFailed) {
    printf("`%s`: cannot create region.\n", __func__);
    exit(1);
  }

  // ShmemBootstrap is true if shared memory has been created, but not
  // yet initialized.  Only the postmaster/creator-of-all-things should
  // have this flag set.
  ShmemBootstrap = true;
}

// Map region into process address space and initialize shared data structures.
bool init_shmem(unsigned int key, unsigned int size) {
  Pointer shared_region;
  unsigned long curr_free_space;

  HASHCTL info;
  int hash_flags;
  ShmemIndexEnt* result;
  ShmemIndexEnt item;
  bool found;
  IpcMemoryId shmid;

  // If zero key, use default memory size.
  if (size) {
    ShmemSize = size;
  }

  // Default key is 0.
  // Attach to shared memory region (SysV or BSD OS specific).
  if (ShmemBootstrap && key == PrivateIPCKey) {
    // If we are running backend alone.
    shmid = ShmemId;
  } else {
    shmid = ipc_memory_id_get(IPCKeyGetBufferMemoryKey(key), ShmemSize);
  }

  shared_region = ipc_memory_attach(shmid);

  if (shared_region == NULL) {
    printf("[FATAL]: `%s` couldn't attach to shmem.\n");
    return false;
  }

  // Get pointers to the dimensions of shared memory.
  ShmemBase = (unsigned long)shared_region;
  ShmemEnd = (unsigned long)shared_region + ShmemSize;
  curr_free_space = 0;

  // First long in shared memory is the count of available space.
  ShmemFreeStart = (unsigned long*)ShmemBase;
  // Next is a shmem pointer to the shmem index.
  ShmemIndexOffset = ShmemFreeStart + 1;
  // Next is ShmemVariableCache.
  ShmemVariableCache = (VariableCache)(ShmemIndexOffset + 1);

  curr_free_space += sizeof(ShmemFreeStart) + sizeof(ShmemIndexOffset) + LONGALIGN(sizeof(VariableCacheData));

  // Bootstrap initialize spin locks so we can start to use the
  // allocator and shmem index.
  init_spin_locks();

  // We have just allocated additional space for two spinlocks. Now
  // setup the global free space count.
  if (ShmemBootstrap) {
    *ShmemFreeStart = curr_free_space;
    memset(ShmemVariableCache, 0, sizeof(*ShmemVariableCache));
  }

  // If ShmemFreeStart is NULL, then the allocator won't work.
  assert(*ShmemFreeStart);

  // Create OR attach to the shared memory shmem index.
  info.keysize = SHMEM_INDEX_KEYSIZE;
  info.datasize = SHMEM_INDEX_DATASIZE;
  hash_flags = HASH_ELEM;

  // This will acquire the shmem index lock, but not release it.
  ShmemIndex = shmem_init_hash("ShmemIndex", SHMEM_INDEX_SIZE, SHMEM_INDEX_SIZE, &info, hash_flags);

  if (!ShmemIndex) {
    printf("[FATAL]: `%s` couldn't initialize shmem index.\n", __func__);
    return false;
  }

  // Now, check the shmem index for an entry to the shmem index.	If
  // there is an entry there, someone else created the table. Otherwise,
  // we did and we have to initialize it.
  MemSet(item.key, 0, SHMEM_INDEX_KEYSIZE);
  strncpy(item.key, "ShmemIndex", SHMEM_INDEX_KEYSIZE);

  result = (ShmemIndexEnt*)hash_search(ShmemIndex, (char*)&item, HASH_ENTER, &found);

  if (!result) {
    printf("[FATAL]: `%s` corrupted shmem index.\n");
    return false;
  }

  if (!found) {
    // Bootstrapping shmem: we have to initialize the shmem index now.
    assert(ShmemBootstrap);

    result->location = MAKE_OFFSET(ShmemIndex->hctl);
    *ShmemIndexOffset = result->location;
    result->size = SHMEM_INDEX_SIZE;

    ShmemBootstrap = false;
  } else {
    assert(!ShmemBootstrap);
  }

  // Now release the lock acquired in ShmemHashInit.
  spin_release(ShmemIndexLock);

  assert(result->location == MAKE_OFFSET(ShmemIndex->hctl));

  return true;
}

// Allocate word-aligned byte string from shared memory.
//
// Assumes ShmemLock and ShmemFreeStart are initialized.
// Returns: Real pointer to memory or NULL if we are out
//  of space. Has to return a real pointer in order
//  to be compatable with malloc().
long* shmem_alloc(unsigned long size) {
  unsigned long tmp_free;
  long* new_space;

  // Ensure space is word aligned.
  //
  // Word-alignment is not good enough. We have to be more conservative:
  // doubles need 8-byte alignment. (We probably only need this on RISC
  // platforms but this is not a big waste of space.) - ay 12/94
  if (size % sizeof(double)) {
    size += sizeof(double) - (size % sizeof(double));
  }

  assert(*ShmemFreeStart);

  spin_acquire(ShmemLock);

  tmp_free = *ShmemFreeStart + size;

  if (tmp_free <= ShmemSize) {
    new_space = (long*)MAKE_PTR(*ShmemFreeStart);
    *ShmemFreeStart += size;
  } else {
    new_space = NULL;
  }

  spin_release(ShmemLock);

  if (!new_space) {
    printf("[NOTICE]: `%s` out of memory.\n", __func__);
  }

  return new_space;
}

// Test if an offset refers to valid shared memory
//
// Returns TRUE if the pointer is valid.
int shmem_is_valid(unsigned long addr) { return (addr < ShmemEnd) && (addr >= ShmemBase); }

// Create/Attach to and initialize shared memory hash table.
//
// Notes:
//
// Assume caller is doing some kind of synchronization
// so that two people dont try to create/initialize the
// table at once.  Use SpinAlloc() to create a spinlock
// for the structure before creating the structure itself.
//
// `param` name: table string name for shmem index
// `param` init_size: initial table size
// `param` max_size: max size of the table
// `param` infop: info about key and bucket size
// `param` hash_flags: info about infop
HTAB* shmem_init_hash(char* name, long init_size, long max_size, HASHCTL* infop, int hash_flags) {
  bool found;
  long* location;

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
  location = shmem_init_struct(name, sizeof(HHDR) + infop->dsize * sizeof(SEG_OFFSET), &found);

  // shmem index is corrupted.	Let someone else give the error
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
  infop->dir = (long*)(((char*)location) + sizeof(HHDR));

  return hash_create(init_size, infop, hash_flags);
}

// Lookup process data structure using process id
//
// Returns: TRUE if no error. locationPtr is initialized if PID is
//  found in the shmem index.
//
// NOTES:
//  Only information about success or failure is the value of
//  locationPtr.
bool shmem_pid_lookup(int pid, SHMEM_OFFSET* location_ptr) {
  ShmemIndexEnt* result;
  ShmemIndexEnt item;
  bool found;

  assert(ShmemIndex);
  MemSet(item.key, 0, SHMEM_INDEX_KEYSIZE);
  sprintf(item.key, "PID %d", pid);

  spin_acquire(ShmemIndexLock);
  result = (ShmemIndexEnt*)hash_search(ShmemIndex, (char*)&item, HASH_ENTER, &found);

  if (!result) {
    spin_release(ShmemIndexLock);
    printf("[ERROR]: `%s` corrupted.\n", __func__);

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
SHMEM_OFFSET shmem_pid_destroy(int pid) {
  ShmemIndexEnt* result;
  ShmemIndexEnt item;
  bool found;
  SHMEM_OFFSET location = 0;

  assert(ShmemIndex);

  MemSet(item.key, 0, SHMEM_INDEX_KEYSIZE);
  sprintf(item.key, "PID %d", pid);

  spin_acquire(ShmemIndexLock);
  result = (ShmemIndexEnt*)hash_search(ShmemIndex, (char*)&item, HASH_REMOVE, &found);

  if (found) {
    location = result->location;
  }

  spin_release(ShmemIndexLock);

  if (!result) {
    printf("[ERROR]: `%s` PID table corrupted.\n", __func__);
    return INVALID_OFFSET;
  }

  if (found) {
    return location;
  }

  return INVALID_OFFSET;
}

long* shmem_init_struct(char* name, unsigned long size, bool* found_ptr) {
  ShmemIndexEnt* result;
  ShmemIndexEnt item;
  long* struct_ptr;

  strncpy(item.key, name, SHMEM_INDEX_KEYSIZE);
  item.location = BAD_LOCATION;

  spin_acquire(ShmemIndexLock);

  if (!ShmemIndex) {
#ifdef USE_ASSERT_CHECKING

    char* strname = "ShmemIndex";

#endif

    // If the shmem index doesnt exist, we fake it.
    //
    // If we are creating the first shmem index, then let shmemalloc()
    // allocate the space for a new HTAB.  Otherwise, find the old one
    // and return that.  Notice that the ShmemIndexLock is held until
    // the shmem index has been completely initialized.
    assert(!strcmp(name, strname));

    if (ShmemBootstrap) {
      // In POSTMASTER/Single process.
      *found_ptr = false;

      return (long*)shmem_alloc(size);
    } else {
      assert(ShmemIndexOffset);

      *found_ptr = true;

      return (long*)MAKE_PTR(*ShmemIndexOffset);
    }
  } else {
    // Look it up in the shmem index.
    result = (ShmemIndexEnt*)hash_search(ShmemIndex, (char*)&item, HASH_ENTER, found_ptr);
  }

  if (!result) {
    spin_release(ShmemIndexLock);

    printf("[ERROR]: `%s` shmem index corrupted.\n", __func__);

    return NULL;
  } else if (*found_ptr) {
    // Structure is in the shmem index so someone else has allocated
    // it already.	The size better be the same as the size we are
    // trying to initialize to or there is a name conflict (or worse).
    if (result->size != size) {
      spin_release(ShmemIndexLock);

      printf("[NOTICE]: ShmemIndex entry size is wrong.\n");

      // Let caller print its message too.
      return NULL;
    }

    struct_ptr = (long*)MAKE_PTR(result->location);
  } else {
    // It isn't in the table yet. allocate and initialize it.
    struct_ptr = shmem_alloc((long)size);

    if (!struct_ptr) {
      // Out of memory.
      assert(ShmemIndex);

      hash_search(ShmemIndex, (char*)&item, HASH_REMOVE, found_ptr);
      spin_release(ShmemIndexLock);
      *found_ptr = FALSE;

      printf("[NOTICE]: `%s` cannot allocate '%s'.\n", name);

      return NULL;
    }

    result->size = size;
    result->location = MAKE_OFFSET(struct_ptr);
  }

  assert(ShmemIsValid((unsigned long)struct_ptr));

  spin_release(ShmemIndexLock);

  return struct_ptr;
}