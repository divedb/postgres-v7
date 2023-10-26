#ifndef RDBMS_STORAGE_IPC_H_
#define RDBMS_STORAGE_IPC_H_

#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "rdbms/c.h"
#include "rdbms/config.h"
#include "rdbms/storage/s_lock.h"

#ifndef HAVE_UNION_SEMUN

union semun {
  int val;
  struct semid_ds* buf;
  unsigned short* array;
};

#endif

typedef uint16 SystemPortAddress;

// Semaphore definitions.
#define IPCProtection                 (0600)  // Access/modify by user only.
#define IPC_NMAXSEM                   25      // Maximum number of semaphores.
#define IpcSemaphoreDefaultStartValue 255
#define IpcSharedLock                 (-1)
#define IpcExclusiveLock              (-255)

#define IpcUnknownStatus   (-1)
#define IpcInvalidArgument (-2)
#define IpcSemIdExist      (-3)
#define IpcSemIdNotExist   (-4)

typedef uint32 IpcSemaphoreKey;  // Semaphore key.
typedef int IpcSemaphoreId;

// Shared memory definitions。
#define IpcMemCreationFailed (-1)
#define IpcMemIdGetFailed    (-2)
#define IpcMemAttachFailed   0

typedef uint32 IPCKey;

#define PrivateIPCKey IPC_PRIVATE
#define DefaultIPCKey 17317

typedef uint32 IpcMemoryKey;  // Shared memory key.
typedef int IpcMemoryId;

extern bool ProcExitInprogress;

void proc_exit(int code);
void shmem_exit(int code);
int on_proc_exit(void (*function)(), caddr_t arg);
int on_shmem_exit(void (*function)(), caddr_t arg);

// This function clears all proc_exit() registered functions.
void on_exit_reset(void);

IpcSemaphoreId ipc_semaphore_create(IpcSemaphoreKey sem_key, int sem_num, int permission, int sem_start_value,
                                    bool remove_on_exit);

// Remove a semaphore.
void ipc_semaphore_kill(IpcSemaphoreKey key);
void ipc_semaphore_lock(IpcSemaphoreId semid, int sem, int lock);
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
  BUF_MGR_LOCKID,
  LOCK_LOCKID,
  OID_GEN_LOCKID,
  XID_GEN_LOCKID,
  CNTL_FILE_LOCKID,
  SHMEM_LOCKID,
  SHMEM_INDEX_LOCKID,
  LOCK_MGR_LOCKID,
  SINVAL_LOCKID,

#ifdef STABLE_MEMORY_STORAGE
  MMCACHELOCKID,
#endif

  PROC_STRUCT_LOCKID,
  FIRST_FREE_LOCKID
} LockId;

#define MAX_SPINS FIRST_FREE_LOCKID

typedef struct SLock {
  slock_t locklock;
  unsigned char flag;
  short nshlocks;
  slock_t shlock;
  slock_t exlock;
  slock_t comlock;
  struct SLock* next;
} SLock;

// Note:
//  These must not hash to DefaultIPCKey or PrivateIPCKey.
#define SystemPortAddressGetIPCKey(address) (28597 * (address) + 17491)

// These keys are originally numbered from 1 to 12 consecutively but not
// all are used. The unused ones are removed.			- ay 4/95.
#define IPCKeyGetBufferMemoryKey(key)      ((key == PrivateIPCKey) ? key : 1 + (key))
#define IPCKeyGetSIBufferMemoryBlock(key)  ((key == PrivateIPCKey) ? key : 7 + (key))
#define IPCKeyGetSLockSharedMemoryKey(key) ((key == PrivateIPCKey) ? key : 10 + (key))
#define IPCKeyGetSpinLockSemaphoreKey(key) ((key == PrivateIPCKey) ? key : 11 + (key))
#define IPCKeyGetWaitIOSemaphoreKey(key)   ((key == PrivateIPCKey) ? key : 12 + (key))
#define IPCKeyGetWaitCLSemaphoreKey(key)   ((key == PrivateIPCKey) ? key : 13 + (key))

// NOTE: This macro must always give the highest numbered key as every backend
// process forked off by the postmaster will be trying to acquire a semaphore
// with a unique key value starting at key+14 and incrementing up.	Each
// backend uses the current key value then increments it by one.
#define IPCGetProcessSemaphoreInitKey(key) ((key == PrivateIPCKey) ? key : 14 + (key))

#endif  // RDBMS_STORAGE_IPC_H_
