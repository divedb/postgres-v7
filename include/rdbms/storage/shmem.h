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

// The shared memory region can start at a different address
// in every process.  Shared memory "pointers" are actually
// offsets relative to the start of the shared memory region(s).
typedef unsigned long SHMEM_OFFSET;

#define INVALID_OFFSET (-1)
#define BAD_LOCATION   (-1)

// Start of the lowest shared memory region.  For now, assume that
// there is only one shared memory region.
extern SHMEM_OFFSET ShmemBase;

// Coerce an offset into a pointer in this process's address space.
#define MAKE_PTR(xx_offs) (ShmemBase + ((unsigned long)(xx_offs)))

// Coerce a pointer into a shmem offset.
#define MAKE_OFFSET(xx_ptr) (SHMEM_OFFSET)(((unsigned long)(xx_ptr)) - ShmemBase)

#define SHM_PTR_VALID(xx_ptr) (((unsigned long)xx_ptr) > ShmemBase)

// Cannot have an offset to ShmemFreeStart (offset 0)
#define SHM_OFFSET_VALID(xx_offs) ((xx_offs != 0) && (xx_offs != INVALID_OFFSET))

void ShmemIndexReset(void);
void ShmemCreate(unsigned int key, unsigned int size);
int InitShmem(unsigned int key, unsigned int size);
long* ShmemAlloc(unsigned long size);
int ShmemIsValid(unsigned long addr);
HTAB* ShmemInitHash(char* name, long init_size, long max_size, HASHCTL* infoP, int hash_flags);
bool ShmemPIDLookup(int pid, SHMEM_OFFSET* location_ptr);
SHMEM_OFFSET ShmemPIDDestroy(int pid);
long* ShmemInitStruct(char* name, unsigned long size, bool* found_ptr);

#endif  // RDBMS_STORAGE_SHMEM_H_
