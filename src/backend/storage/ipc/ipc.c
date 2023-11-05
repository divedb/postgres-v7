// =========================================================================
//
// ipc.c
//   POSTGRES inter=process communication definitions.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//   $Header: /usr/local/cvsroot/pgsql/src/backend/storage/ipc/ipc.c,v 1.46 2000/04/12 17:15:36 momjian Exp $
//
// NOTES
//
//   Currently, semaphores are used (my understanding anyway) in two
//   different ways:
//      1. as mutexes on machines that don't have test=and=set (eg.
//      mips R3000).
//      2. for putting processes to sleep when waiting on a lock
//      and waking them up when the lock is free.
//      The number of semaphores in (1) is fixed and those are shared
//      among all backends. In (2), there is 1 semaphore per process and those
//      are not shared with anyone else.
//                                                                   -ay4/95
//
// =========================================================================
#include "rdbms/storage/ipc.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "rdbms/c.h"

#ifdef linux

union semun {
  int val;
  struct semid_ds* buf;
  unsigned short* array;
};

#endif

// This flag is set during proc_exit() to change elog()'s behavior,
// so that an elog() from an on_proc_exit routine cannot get us out
// of the exit procedure.  We do NOT want to go back to the idle loop...
bool ProcExitInprogress = false;

static int UsePrivateMemory = 0;

static void ipc_memory_detach(int status, char* shmaddr);
static void ipc_config_tip();

#define MAX_ON_EXITS 20

static struct OnExit {
  void (*function)();
  caddr_t arg;
} OnProcExitList[MAX_ON_EXITS], OnShmemExitList[MAX_ON_EXITS];

static int OnProcExitIndex;
static int OnShmemExitIndex;

typedef struct PrivateMemStruct {
  int id;
  char* memptr;
} PrivateMem;

static PrivateMem IpcPrivateMem[16];

static int private_memory_create(IpcMemoryKey mem_key, uint32 size) {
  static int memid = 0;

  UsePrivateMemory = 1;

  IpcPrivateMem[memid].id = memid;
  IpcPrivateMem[memid].memptr = malloc(size);

  if (IpcPrivateMem[memid].memptr == NULL) {
    fprintf(stderr, "private_memory_create: not enough memory to malloc");
    exit(EXIT_FAILURE);
  }

  MEMSET(IpcPrivateMem[memid].memptr, 0, size);

  return memid++;
}

static char* private_memory_attach(IpcMemoryId memid) { return IpcPrivateMem[memid].memptr; }

// This function calls all the callbacks registered
// for it (to free resources) and then calls exit.
// This should be the only function to call exit().
//                                      -cim 2/6/90
void proc_exit(int code) {
  ProcExitInprogress = true;

  printf("proc_exit(%d)\n", code);

  shmem_exit(code);

  // Call all the callbacks registered before calling exit().
  //
  // Note that since we decrement on_proc_exit_index each time,
  // if a callback calls elog(ERROR) or elog(FATAL) then it won't
  // be invoked again when control comes back here (nor will the
  // previously-completed callbacks).  So, an infinite loop
  // should not be possible.
  while (--OnProcExitIndex >= 0) {
    (*OnProcExitList[OnProcExitIndex].function)(code, OnProcExitList[OnProcExitIndex].arg);
  }

  printf("exit(%d)\n", code);

  exit(code);
}

// Run all of the on_shmem_exit routines but don't exit in the end.
// This is used by the postmaster to re-initialize shared memory and
// semaphores after a backend dies horribly.
void shmem_exit(int code) {
  // TPRINTF(TRACE_VERBOSE, "shmem_exit(%d)", code);
  // printf("shmem_exit(%d)\n", code);

  while (--OnShmemExitIndex >= 0) {
    (*OnShmemExitList[OnShmemExitIndex].function)(code, OnShmemExitList[OnShmemExitIndex].arg);
  }

  OnShmemExitIndex = 0;
}

// this function adds a callback function to the list of
// functions invoked by proc_exit().
//                                           -cim 2/6/90
int on_proc_exit(void (*function)(), caddr_t arg) {
  if (OnProcExitIndex >= MAX_ON_EXITS) {
    return -1;
  }

  OnProcExitList[OnProcExitIndex].function = function;
  OnProcExitList[OnProcExitIndex].arg = arg;
  ++OnProcExitIndex;

  return 0;
}

int on_shmem_exit(void (*function)(), caddr_t arg) {
  if (OnShmemExitIndex >= MAX_ON_EXITS) {
    return -1;
  }

  OnShmemExitList[OnShmemExitIndex].function = function;
  OnShmemExitList[OnShmemExitIndex].arg = arg;
  ++OnShmemExitIndex;

  return 0;
}

void on_exit_reset() {
  OnShmemExitIndex = 0;
  OnProcExitIndex = 0;
}

static void ipc_private_semaphore_kill(int status, int semid) {
  union semun semun;

  semctl(semid, 0, IPC_RMID, semun);
}

static void ipc_private_memory_kill(int status, int shmid) {
  if (!UsePrivateMemory) {
    if (shmctl(shmid, IPC_RMID, (struct shmid_ds*)NULL) < 0) {
      fprintf(stderr, "IPCPrivateMemoryKill: shmctl(%d, %d, 0) failed: %d\n", shmid, IPC_RMID);
      exit(EXIT_FAILURE);
    }
  }
}

// Note:
// XXX This should be split into two different calls. One should
// XXX be used to create a semaphore set. The other to "attach" a
// XXX existing set.  It should be an error for the semaphore set
// XXX to to already exist or for it not to, respectively.
//
//  Currently, the semaphore sets are "attached" and an error
//  is detected only when a later shared memory attach fails.
IpcSemaphoreId ipc_semaphore_create(IpcSemaphoreKey sem_key, int sem_num, int permission, int sem_start_value,
                                    bool remove_on_exit) {
  int i;
  int err_status;
  int semid;
  u_short array[IPC_NMAXSEM];
  union semun semun;

  if (sem_num > IPC_NMAXSEM || sem_num <= 0) {
    return -1;
  }

  semid = semget(sem_key, 0, 0);

  if (semid == -1) {
    semid = semget(sem_key, sem_num, IPC_CREAT | permission);

    if (semid < 0) {
      fprintf(stderr,
              "`%s`: semget failed (%s) "
              "key=%d, num=%d, permission=%o\n",
              __func__, strerror(errno), sem_key, sem_num, permission);

      ipc_config_tip();

      return -1;
    }

    for (i = 0; i < sem_num; i++) {
      array[i] = sem_start_value;
    }

    semun.array = array;
    err_status = semctl(semid, 0, SETALL, semun);

    if (err_status == -1) {
      fprintf(stderr, "`%s`: semctl failed (%s) id=%d\n", __func__, strerror(errno), semid);
      semctl(semid, 0, IPC_RMID, semun);
      ipc_config_tip();

      return -1;
    }

    if (remove_on_exit) {
      on_shmem_exit(ipc_private_semaphore_kill, (caddr_t)semid);
    }
  }

  return semid;
}

void ipc_semaphore_kill(IpcSemaphoreKey key) {
  int semid;
  union semun semun;

  semid = semget(key, 0, 0);

  if (semid != -1) {
    semctl(semid, 0, IPC_RMID, semun);
  }
}

// Note: the xxx_return variables are only used for debugging.
static int IpcSemaphoreLockReturn;

void ipc_semaphore_lock(IpcSemaphoreId semid, int sem, int lock) {
  int err_status;
  struct sembuf sops;

  sops.sem_op = lock;
  sops.sem_flg = 0;
  sops.sem_num = sem;

  // Note: if errStatus is -1 and errno == EINTR then it means we
  //    returned from the operation prematurely because we were
  //    sent a signal.  So we try and lock the semaphore again.
  //    I am not certain this is correct, but the semantics aren't
  //    clear it fixes problems with parallel abort synchronization,
  //    namely that after processing an abort signal, the semaphore
  //    call returns with -1 (and errno == EINTR) before it should.
  //    -cim 3/28/90
  do {
    err_status = semop(semid, &sops, 1);
  } while (err_status == -1 && errno == EINTR);

  IpcSemaphoreLockReturn = err_status;

  if (err_status == -1) {
    fprintf(stderr, "IpcSemaphoreLock: semop failed (%s) id=%d\n", strerror(errno), semid);
    proc_exit(255);
  }
}

static int IpcSemaphoreUnlockReturn;

void ipc_semaphore_unlock(IpcSemaphoreId semid, int sem, int lock) {
  int err_status;
  struct sembuf sops;

  sops.sem_op = -lock;
  sops.sem_flg = 0;
  sops.sem_num = sem;

  // Note:
  //    If errStatus is -1 and errno == EINTR then it means we
  //    returned from the operation prematurely because we were
  //    sent a signal.  So we try and lock the semaphore again.
  //    I am not certain this is correct, but the semantics aren't
  //    clear it fixes problems with parallel abort synchronization,
  //    namely that after processing an abort signal, the semaphore
  //    call returns with -1 (and errno == EINTR) before it should.
  //    -cim 3/28/90
  do {
    err_status = semop(semid, &sops, 1);
  } while (err_status == -1 && errno == EINTR);

  IpcSemaphoreUnlockReturn = err_status;

  if (err_status == -1) {
    fprintf(stderr, "%s: semop failed (%s) id=%d", __func__, strerror(errno), semid);
    proc_exit(255);
  }
}

int ipc_semaphore_get_count(IpcSemaphoreId semid, int sem) {
  int semncnt;
  union semun dummy;

  semncnt = semctl(semid, sem, GETNCNT, dummy);

  return semncnt;
}

int ipc_semaphore_get_value(IpcSemaphoreId semid, int sem) {
  int semval;
  union semun dummy;

  semval = semctl(semid, sem, GETVAL, dummy);

  return semval;
}

IpcMemoryId ipc_memory_create(IpcMemoryKey mem_key, uint32 size, int permission) {
  IpcMemoryId shmid;

  if (mem_key == PRIVATE_IPC_KEY) {
    shmid = private_memory_create(mem_key, size);
  } else {
    shmid = shmget(mem_key, size, IPC_CREAT | permission);
  }

  if (shmid < 0) {
    fprintf(stderr,
            "IpcMemoryCreate: shmget failed (%s) "
            "key=%d, size=%d, permission=%o\n",
            strerror(errno), mem_key, size, permission);
    ipc_config_tip();

    return IPC_MEM_CREATION_FAILED;
  }

  on_shmem_exit(ipc_private_memory_kill, (caddr_t)shmid);

  return shmid;
}

IpcMemoryId ipc_memory_id_get(IpcMemoryKey mem_key, uint32 size) {
  IpcMemoryId shmid;

  shmid = shmget(mem_key, size, 0);

  if (shmid < 0) {
    fprintf(stderr,
            "IpcMemoryIdGet: shmget failed (%s) "
            "key=%d, size=%d, permission=%o",
            strerror(errno), mem_key, size, 0);

    return IPC_MEM_ID_GET_FAILED;
  }

  return shmid;
}

static void ipc_memory_detach(int status, char* shm_addr) {
  if (shmdt(shm_addr) < 0) {
    fprintf(stderr, "IpcMemoryDetach: shmdt(0x%p): %p\n", shm_addr);
  }
}

char* ipc_memory_attach(IpcMemoryId memid) {
  char* mem_address;

  if (UsePrivateMemory) {
    mem_address = (char*)private_memory_attach(memid);
  } else {
    mem_address = (char*)shmat(memid, 0, 0);
  }

  if (mem_address == (char*)-1) {
    fprintf(stderr, "IpcMemoryAttach: shmat failed (%s) id=%d\n", strerror(errno), memid);

    return IPC_MEM_ATTACH_FAILED;
  }

  if (!UsePrivateMemory) {
    on_shmem_exit(ipc_memory_detach, (caddr_t)mem_address);
  }

  return mem_address;
}

void ipc_memory_kill(IpcMemoryKey mem_key) {
  IpcMemoryId shmid;

  if (!UsePrivateMemory && (shmid = shmget(mem_key, 0, 0)) >= 0) {
    if (shmctl(shmid, IPC_RMID, (struct shmid_ds*)NULL) < 0) {
      fprintf(stderr, "IpcMemoryKill: shmctl(%d, %d, 0) failed: %d", shmid, IPC_RMID);
      exit(EXIT_FAILURE);
    }
  }
}

void create_and_init_slock_memory(IPCKey key);
void attach_slock__memory(IPCKey key);

static void ipc_config_tip() {
  fprintf(stderr, "This type of error is usually caused by an improper\n");
  fprintf(stderr, "shared memory or System V IPC semaphore configuration.\n");
  fprintf(stderr, "For more information, see the FAQ and platform-specific\n");
  fprintf(stderr, "FAQ's in the source directory pgsql/doc or on our\n");
  fprintf(stderr, "web site at http://www.postgresql.org.\n");
}
