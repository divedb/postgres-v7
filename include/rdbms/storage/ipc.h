#ifndef RDBMS_STORAGE_IPC_H_
#define RDBMS_STORAGE_IPC_H_

#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "rdbms/c.h"
#include "rdbms/config.h"

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
void attach_slock__memory(IPCKey key);

#endif  // RDBMS_STORAGE_IPC_H_
