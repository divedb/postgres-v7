//===----------------------------------------------------------------------===//
//
// ipc.h
//  POSTGRES inter-process communication definitions.
//
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: ipc.h,v 1.38 2000/01/26 05:58:32 momjian Exp $
//
// NOTES
//  This file is very architecture-specific.
//  This stuff should actually be factored into the port/ directories.
//
// Some files that would normally need to include only sys/ipc.h must
// instead included this file because on Ultrix, sys/ipc.h is not designed
// to be included multiple times.  This file (by virtue of the ifndef IPC_H)
// is.
//===----------------------------------------------------------------------===//
#ifndef RDBMS_STORAGE_IPC_H_
#define RDBMS_STORAGE_IPC_H_

#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "rdbms/postgres.h"
#include "rdbms/storage/s_lock.h"

#ifdef __linux__

union semun {
  int val;
  struct semid_ds* buf;
  unsigned short* array;
};

#endif

typedef uint16 SystemPortAddress;

// Semaphore definitions.
#define IPC_PROTECTION (0600)    // Access/modify by user only
typedef uint32 IpcSemaphoreKey;  // Semaphore key.
typedef int IpcSemaphoreId;

#define IPC_NMAX_SEM  32   // Maximum number of semaphores
#define PG_SEMA_MAGIC 537  // Must be less than SEMVMX

// Shared memory definitions.
typedef uint32 IpcMemoryKey;  // Shared memory key
typedef int IpcMemoryId;

typedef struct {
  int32 magic;  // Magic # to identify Postgres segments

#define PGShmemMagic 679834892

  pid_t creator_pid;   // PID of creating process
  uint32 total_size;   // Total size of segment
  uint32 free_offset;  // Offset to first free space
} PGShmemHeader;

// SpinLock definitions.
typedef enum LockId {
  BUF_MGR_LOCK_ID,
  OID_GEN_LOCK_ID,
  XID_GEN_LOCK_ID,
  CNTL_FILE_LOCK_ID,
  SHMEM_LOCK_ID,
  SHMEM_INDEX_LOCK_ID,
  LOCK_MGR_LOCK_ID,
  SINVAL_LOCK_ID,
  PROC_STRUCT_LOCK_ID,

#ifdef STABLE_MEMORY_STORAGE
  MM_CACHE_LOCK_ID,
#endif

  MAX_SPINS
} LockId;

// ipc.c
extern bool ProcExitInprogress;

void proc_exit(int code);
void shmem_exit(int code);
int on_proc_exit(void (*function)(), caddr_t arg);
int on_shmem_exit(void (*function)(), caddr_t arg);
void on_exit_reset();

void ipc_init_key_assignment(int port);

IpcSemaphoreId ipc_semaphore_create(int sem_num, int permission, int sem_start_value, bool remove_on_exit);
void ipc_semaphore_kill(IpcSemaphoreId sem_id);
void ipc_semaphore_lock(IpcSemaphoreId sem_id, int sem, bool interrupt_ok);
void ipc_semaphore_unlock(IpcSemaphoreId sem_id, int sem);
bool ipc_semaphore_try_lock(IpcSemaphoreId sem_id, int sem);
int ipc_semaphore_get_value(IpcSemaphoreId sem_id, int sem);

PGShmemHeader* ipc_memory_create(uint32 size, bool make_private, int permission);
bool shared_memory_is_in_use(IpcMemoryKey shm_key, IpcMemoryId shm_id);

// ipci.c
void create_shared_memory_and_semaphores(bool make_private, int max_backends);

#endif  // RDBMS_STORAGE_IPC_H_
