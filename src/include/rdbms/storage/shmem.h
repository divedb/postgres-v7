//===----------------------------------------------------------------------===//
//
// shmem.h
//  Shared memory management structures
//
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: shmem.h,v 1.22 2000/01/26 05:58:33 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_STORAGE_SHMEM_H_
#define RDBMS_STORAGE_SHMEM_H_

#include "rdbms/storage/spin.h"
#include "rdbms/utils/hsearch.h"

// The shared memory region can start at a different address
// in every process.  Shared memory "pointers" are actually
// offsets relative to the start of the shared memory region(s).
typedef unsigned long ShmemOffset;

#define INVALID_OFFSET (-1)
#define BAD_LOCATION   (-1)

// Start of the lowest shared memory region.  For now, assume that
// there is only one shared memory region.
extern ShmemOffset ShmemBase;

// Coerce an offset into a pointer in this process's address space.
#define MAKE_PTR(xx_offs) (ShmemBase + ((unsigned long)(xx_offs)))

// Coerce a pointer into a shmem offset.
#define MAKE_OFFSET(xx_ptr) (SHMEM_OFFSET)(((unsigned long)(xx_ptr)) - ShmemBase)

#define SHM_PTR_VALID(xx_ptr) (((unsigned long)xx_ptr) > ShmemBase)

// Cannot have an offset to ShmemFreeStart (offset 0)
#define SHM_OFFSET_VALID(xx_offs) ((xx_offs != 0) && (xx_offs != INVALID_OFFSET))

extern SpinLock ShmemLock;
extern SpinLock ShmemIndexLock;

typedef struct ShmemQueue {
  ShmemOffset prev;
  ShmemOffset next;
} ShmemQueue;

// Definition in shmem.c
void shmem_index_reset();
void shmem_create(unsigned int key, unsigned int size);
bool init_shmem(unsigned int key, unsigned int size);
long* shmem_alloc(unsigned long size);
int shmem_is_valid(unsigned long addr);
HTAB* shmem_init_hash(char* name, long init_size, long max_size, HASHCTL* infop, int hash_flags);
bool shmem_pid_lookup(int pid, ShmemOffset* location_ptr);
ShmemOffset shmem_pid_destroy(int pid);
long* shmem_init_struct(char* name, unsigned long size, bool* found_ptr);

typedef int TableID;

// Size constants for the shmem index table.
// Max size of data structure string name.
#define SHMEM_INDEX_KEY_SIZE (50)
// Data in shmem index table hash bucket.
#define SHMEM_INDEX_DATA_SIZE (sizeof(ShmemIndexEnt) - SHMEM_INDEX_KEYSIZE)
// Maximum size of the shmem index table.
#define SHMEM_INDEX_SIZE (100)

// This is a hash bucket in the shmem index table.
typedef struct {
  char key[SHMEM_INDEX_KEY_SIZE];  // String name.
  unsigned long location;          // Location in shared mem.
  unsigned long size;              // Number of bytes allocated for the structure.
} ShmemIndexEnt;

#endif  // RDBMS_STORAGE_SHMEM_H_
