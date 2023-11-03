#ifndef RDBMS_STORAGE_SHMEM_DEF_H_
#define RDBMS_STORAGE_SHMEM_DEF_H_

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

#endif  // RDBMS_STORAGE_SHMEM_DEF_H_
