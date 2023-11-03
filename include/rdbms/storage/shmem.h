// =========================================================================
//
// shmem.h
//  shared memory management structures
//
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: shmem.h,v 1.22 2000/01/26 05:58:33 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_SHMEM_H_
#define RDBMS_STORAGE_SHMEM_H_

#include "rdbms/storage/shmem_def.h"
#include "rdbms/storage/spin.h"
#include "rdbms/utils/hsearch.h"

void shmem_index_reset();
void shmem_create(unsigned int key, unsigned int size);
bool init_shmem(unsigned int key, unsigned int size);
long* shmem_alloc(unsigned long size);
int shmem_is_valid(unsigned long addr);
HTAB* shmem_init_hash(char* name, long init_size, long max_size, HASHCTL* infop, int hash_flags);
bool shmem_pid_lookup(int pid, SHMEM_OFFSET* location_ptr);
SHMEM_OFFSET shmem_pid_destroy(int pid);
long* shmem_init_struct(char* name, unsigned long size, bool* found_ptr);

typedef int TableID;

// Size constants for the shmem index table.
// Max size of data structure string name.
#define SHMEM_INDEX_KEYSIZE (50)
// Data in shmem index table hash bucket.
#define SHMEM_INDEX_DATASIZE (sizeof(ShmemIndexEnt) - SHMEM_INDEX_KEYSIZE)
// Maximum size of the shmem index table.
#define SHMEM_INDEX_SIZE (100)

// This is a hash bucket in the shmem index table.
typedef struct {
  char key[SHMEM_INDEX_KEYSIZE];  // String name.
  unsigned long location;         // Location in shared mem.
  unsigned long size;             // Number of bytes allocated for the structure.
} ShmemIndexEnt;

#endif  // RDBMS_STORAGE_SHMEM_H_
