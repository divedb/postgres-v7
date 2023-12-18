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
#define IPC_PROTECTION                    (0600)  // Access/modify by user only.
#define IPC_NMAX_SEM                      25      // Maximum number of semaphores.
#define IPC_SEMAPHORE_DEFAULT_START_VALUE 255
#define IPC_SHARED_LOCK                   (-1)
#define IPC_EXCLUSIVE_LOCK                (-255)

#define IPC_UNKOWN_STATUS    (-1)
#define IPC_INVALID_ARGUMENT (-2)
#define IPC_SEM_ID_EXIST     (-3)
#define IPC_SEM_ID_NOT_EXIST (-4)

typedef uint32 IpcSemaphoreKey;  // Semaphore key.
typedef int IpcSemaphoreId;

// Shared memory definitions.
#define IPC_MEM_CREATION_FAILED (-1)
#define IPC_MEM_ID_GET_FAILED   (-2)
#define IPC_MEM_ATTACH_FAILED   0

typedef uint32 IPCKey;

#define PRIVATE_IPC_KEY IPC_PRIVATE
#define DEFAULT_IPC_KEY 17317

typedef uint32 IpcMemoryKey;  // Shared memory key.
typedef int IpcMemoryId;

// Definition in ipc.c
extern bool ProcExitInprogress;

void proc_exit(int code);
void shmem_exit(int code);
int on_proc_exit(void (*function)(), caddr_t arg);
int on_shmem_exit(void (*function)(), caddr_t arg);

// This function clears all proc_exit() registered functions.
void on_exit_reset();

IpcSemaphoreId ipc_semaphore_create(IpcSemaphoreKey sem_key, int sem_num, int permission, int sem_start_value,
                                    bool remove_on_exit);
void ipc_semaphore_kill(IpcSemaphoreKey key);
void ipc_semaphore_lock(IpcSemaphoreId semid, int sem, int lock);
void ipc_semaphore_unlock(IpcSemaphoreId semid, int sem, int lock);
int ipc_semaphore_get_count(IpcSemaphoreId semid, int sem);
int ipc_semaphore_get_value(IpcSemaphoreId semid, int sem);

IpcMemoryId ipc_memory_create(IpcMemoryKey mem_key, uint32 size, int permission);
IpcMemoryId ipc_memory_id_get(IpcMemoryKey mem_key, uint32 size);
char* ipc_memory_attach(IpcMemoryId memid);
void ipc_memory_kill(IpcMemoryKey mem_key);
void create_and_init_slock_memory(IPCKey key);
void attach_slock_memory(IPCKey key);

#define NO_LOCK        0
#define SHARED_LOCK    1
#define EXCLUSIVE_LOCK 2

typedef enum LockId {
  BUF_MGR_LOCK_ID,
  LOCK_LOCK_ID,
  OID_GEN_LOCK_ID,
  XID_GEN_LOCK_ID,
  CNTL_FILE_LOCK_ID,
  SHMEM_LOCK_ID,
  SHMEM_INDEX_LOCK_ID,
  LOCK_MGR_LOCK_ID,
  SINVAL_LOCK_ID,

#ifdef STABLE_MEMORY_STORAGE
  MM_CACHE_LOCK_ID,
#endif

  PROC_STRUCT_LOCK_ID,
  FIRST_FREE_LOCK_ID
} LockId;

#define MAX_SPINS FIRST_FREE_LOCK_ID

typedef struct SLock {
  TasLock lock_lock;
  unsigned char flag;
  short nshlocks;
  TasLock shlock;
  TasLock exlock;
  TasLock comlock;
  struct SLock* next;
} SLock;

// Note:
//  These must not hash to DEFAULT_IPC_KEY or PRIVATE_IPC_KEY.
#define SYSTEM_PORT_ADDRESS_GET_IPC_KEY(address) (28597 * (address) + 17491)

// These keys are originally numbered from 1 to 12 consecutively but not
// all are used. The unused ones are removed. - ay 4/95.
#define IPC_KEY_GET_BUFFER_MEMORY_KEY(key)       ((key == PRIVATE_IPC_KEY) ? key : 1 + (key))
#define IPC_KEY_GET_SI_BUFFER_MEMORY_BLOCK(key)  ((key == PRIVATE_IPC_KEY) ? key : 7 + (key))
#define IPC_KEY_GET_SLOCK_SHARED_MEMORY_KEY(key) ((key == PRIVATE_IPC_KEY) ? key : 10 + (key))
#define IPC_KEY_GET_SPIN_LOCK_SEMAPHORE_KEY(key) ((key == PRIVATE_IPC_KEY) ? key : 11 + (key))
#define IPC_KEY_GET_WAIT_IO_SEMAPHORE_KEY(key)   ((key == PRIVATE_IPC_KEY) ? key : 12 + (key))
#define IPC_KEY_GET_WAIT_CL_SEMAPHORE_KEY(key)   ((key == PRIVATE_IPC_KEY) ? key : 13 + (key))

// NOTE: This macro must always give the highest numbered key as every backend
// process forked off by the postmaster will be trying to acquire a semaphore
// with a unique key value starting at key+14 and incrementing up. Each
// backend uses the current key value then increments it by one.
#define IPC_GET_PROCESS_SEMAPHORE_INIT_KEY(key) ((key == PRIVATE_IPC_KEY) ? key : 14 + (key))

// ipci.c
IPCKey system_port_address_create_ipc_key(SystemPortAddress address);
void create_shared_memory_and_semaphores(IPCKey key, int max_backends);
void attach_shared_memory_and_semaphores(IPCKey key);

#endif  // RDBMS_STORAGE_IPC_H_
