//===----------------------------------------------------------------------===//
//
// ipc.c
//  POSTGRES inter-process communication definitions.
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/storage/ipc/ipc.c,v 1.66 2001/03/23 04:49:54 momjian
// Exp $
//
// NOTES
//
//  Currently, semaphores are used (my understanding anyway) in two
//  different ways:
//  1. as mutexes on machines that don't have test-and-set (eg.
//     mips R3000).
//  2. for putting processes to sleep when waiting on a lock
//     and waking them up when the lock is free.
//  The number of semaphores in (1) is fixed and those are shared
//  among all backends. In (2), there is 1 semaphore per process and those
//  are not shared with anyone else.
//                                                                      -ay 4/95
//===----------------------------------------------------------------------===//

#include "rdbms/storage/ipc.h"

#include <errno.h>
#include <signal.h>  // For kill()
#include <stdlib.h>
#include <sys/file.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#include "rdbms/miscadmin.h"
#include "rdbms/utils/elog.h"

// This flag is set during proc_exit() to change elog()'s behavior,
// so that an elog() from an on_proc_exit routine cannot get us out
// of the exit procedure. We do NOT want to go back to the idle loop...
bool ProcExitInprogress = false;

static IpcSemaphoreId internal_ipc_semaphore_create(IpcSemaphoreKey sem_key, int num_sems, int permission,
                                                    int sem_start_value, bool remove_on_exit);
static void callback_semaphore_kill(int status, Datum sem_id);
static pid_t ipc_semaphore_get_last_pid(IpcSemaphoreId sem_id, int sem);
static void* internal_ipc_memory_create(IpcMemoryKey mem_key, uint32 size, int permission);
static void ipc_memory_detach(int status, Datum shm_addr);
static void ipc_memory_delete(int status, Datum shm_id);
static void* private_memory_create(uint32 size);
static void private_memory_delete(int status, Datum mem_addr);

// exit() handling stuff
//
// These functions are in generally the same spirit as atexit(2),
// but provide some additional features we need --- in particular,
// we want to register callbacks to invoke when we are disconnecting
// from a broken shared-memory context but not exiting the postmaster.
//
// Callback functions can take zero, one, or two args: the first passed
// arg is the integer exitcode, the second is the Datum supplied when
// the callback was registered.
//
// XXX these functions probably ought to live in some other module.
#define MAX_ON_EXITS 20

static struct OnExit {
  void (*function)();
  caddr_t arg;
} OnProcExitList[MAX_ON_EXITS], OnShmemExitList[MAX_ON_EXITS];

static int OnProcExitIndex;
static int OnShmemExitIndex;

// The idea here is to detect and re-use keys that may have been assigned
// by a crashed postmaster or backend.
static IpcMemoryKey NextShmemSegID = 0;
static IpcSemaphoreKey NextSemaID = 0;

// This function calls all the callbacks registered
// for it (to free resources) and then calls exit.
// This should be the only function to call exit().
//                                      -cim 2/6/90
void proc_exit(int code) {
  // Once we set this flag, we are committed to exit. Any elog() will
  // NOT send control back to the main loop, but right back here.
  ProcExitInprogress = true;

  // Forget any pending cancel or die requests; we're doing our best to
  // close up shop already.  Note that the signal handlers will not set
  // these flags again, now that proc_exit_inprogress is set.
  InterruptPending = false;
  ProcDiePending = false;
  QueryCancelPending = false;
  // And let's just make *sure* we're not interrupted ...
  ImmediateInterruptOK = false;
  InterruptHoldoffCount = 1;
  CritSectionCount = 0;

  if (DebugLvl > 1) {
    elog(DEBUG, "proc_exit(%d)", code);
  }

  // Do our shared memory exits first.
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

  if (DebugLvl > 1) {
    elog(DEBUG, "exit(%d)", code);
  }

  exit(code);
}

// Run all of the on_shmem_exit routines but don't exit in the end.
// This is used by the postmaster to re-initialize shared memory and
// semaphores after a backend dies horribly.
void shmem_exit(int code) {
  if (DebugLvl > 1) {
    elog(DEBUG, "shmem_exit(%d)", code);
  }

  // Call all the registered callbacks.
  //
  // As with proc_exit(), we remove each callback from the list
  // before calling it, to avoid infinite loop in case of error.
  while (--OnShmemExitIndex >= 0) {
    (*OnShmemExitList[OnShmemExitIndex].function)(code, OnShmemExitList[OnShmemExitIndex].arg);
  }

  OnShmemExitIndex = 0;
}

// This function adds a callback function to the list of
// functions invoked by proc_exit().
// -cim 2/6/90
void on_proc_exit(void (*function)(), Datum arg) {
  if (OnProcExitIndex >= MAX_ON_EXITS) {
    elog(FATAL, "Out of on_proc_exit slots");
  }

  OnProcExitList[OnProcExitIndex].function = function;
  OnProcExitList[OnProcExitIndex].arg = arg;
  ++OnProcExitIndex;
}

// This function adds a callback function to the list of
// functions invoked by shmem_exit().
// -cim 2/6/90
void on_shmem_exit(void (*function)(), Datum arg) {
  if (OnShmemExitIndex >= MAX_ON_EXITS) {
    elog(FATAL, "Out of on_shmem_exit slots");
  }

  OnShmemExitList[OnShmemExitIndex].function = function;
  OnShmemExitList[OnShmemExitIndex].arg = arg;
  ++OnShmemExitIndex;
}

// This function clears all on_proc_exit() and on_shmem_exit()
// registered functions.  This is used just after forking a backend,
// so that the backend doesn't believe it should call the postmaster's
// on-exit routines when it exits...
void on_exit_reset() {
  OnShmemExitIndex = 0;
  OnProcExitIndex = 0;
}

void ipc_init_key_assignment(int port) {
  NextShmemSegID = port * 1000;
  NextSemaID = port * 1000;
}

// Create a semaphore set with the given number of useful semaphores
// (an additional sema is actually allocated to serve as identifier).
// Dead Postgres sema sets are recycled if found, but we do not fail
// upon collision with non-Postgres sema sets.
IpcSemaphoreId ipc_semaphore_create(int num_sems, int permission, int sem_start_value, bool remove_on_exit) {
  IpcSemaphoreId sem_id;
  union semun semun;

  // Loop till we find a free IPC key.
  for (NextSemaID++;; NextSemaID++) {
    pid_t creator_pid;

    sem_id = internal_ipc_semaphore_create(NextSemaID, num_sems + 1, permission, sem_start_value, remove_on_exit);

    if (sem_id >= 0) {
      break;
    }

    // See if it looks to be leftover from a dead Postgres process.
    sem_id = semget(NextSemaID, num_sems + 1, 0);

    if (sem_id < 0) {
      continue;
    }

    // Sema belongs to a non-Postgres app.
    if (ipc_semaphore_get_value(sem_id, num_sems) != PG_SEMA_MAGIC) {
      continue;
    }

    // If the creator PID is my own PID or does not belong to any
    // extant process, it's safe to zap it.
    creator_pid = ipc_semaphore_get_last_pid(sem_id, num_sems);

    if (creator_pid != getpid()) {
      if (kill(creator_pid, 0) == 0 || errno != ESRCH) {
        continue;
      }
    }

    // The sema set appears to be from a dead Postgres process, or
    // from a previous cycle of life in this same process.	Zap it, if
    // possible.  This probably shouldn't fail, but if it does, assume
    // the sema set belongs to someone else after all, and continue
    // quietly.
    semun.val = 0;

    if (semctl(sem_id, 0, IPC_RMID, semun) < 0) {
      continue;
    }

    // Now try again to create the sema set.
    sem_id = internal_ipc_semaphore_create(NextSemaID, num_sems + 1, permission, sem_start_value, remove_on_exit);

    if (sem_id >= 0) {
      break;
    }

    // Can only get here if some other process managed to create the
    // same sema key before we did.  Let him have that one, loop
    // around to try next key.
  }

  // OK, we created a new sema set. Mark it as created by this process.
  // We do this by setting the spare semaphore to PGSemaMagic-1 and then
  // incrementing it with semop(). That leaves it with value
  // PGSemaMagic and sempid referencing this process.
  semun.val = PG_SEMA_MAGIC - 1;
  if (semctl(sem_id, num_sems, SETVAL, semun) < 0) {
    fprintf(stderr, "%s: semctl(id=%d, %d, SETVAL, %d) failed: %s\n", __func__, sem_id, num_sems, PG_SEMA_MAGIC - 1,
            strerror(errno));

    if (errno == ERANGE) {
      fprintf(stderr,
              "You possibly need to raise your kernel's SEMVMX value to be at least\n"
              "%d.  Look into the PostgreSQL documentation for details.\n",
              PG_SEMA_MAGIC);
    }

    proc_exit(1);
  }

  ipc_semaphore_unlock(sem_id, num_sems);

  return sem_id;
}

void ipc_semaphore_kill(IpcSemaphoreId sem_id) {
  union semun semun;

  semun.val = 0;

  if (semctl(sem_id, 0, IPC_RMID, semun) < 0) {
    fprintf(stderr, "%s: semctl(%d, 0, IPC_RMID, ...) failed: %s\n", __func__, sem_id, strerror(errno));
  }

  // We used to report a failure via elog(NOTICE), but that's pretty
  // pointless considering any client has long since disconnected...
}

void ipc_semaphore_lock(IpcSemaphoreId sem_id, int sem, bool interrupt_ok) {
  int err_status;
  struct sembuf sops;

  sops.sem_op = -1;
  sops.sem_flg = 0;
  sops.sem_num = sem;

  // Note: if errStatus is -1 and errno == EINTR then it means we
  // returned from the operation prematurely because we were sent a
  // signal.	So we try and lock the semaphore again.
  //
  // Each time around the loop, we check for a cancel/die interrupt. We
  // assume that if such an interrupt comes in while we are waiting, it
  // will cause the semop() call to exit with errno == EINTR, so that we
  // will be able to service the interrupt (if not in a critical section
  // already).
  //
  // Once we acquire the lock, we do NOT check for an interrupt before
  // returning.  The caller needs to be able to record ownership of the
  // lock before any interrupt can be accepted.
  //
  // There is a window of a few instructions between CHECK_FOR_INTERRUPTS
  // and entering the semop() call.  If a cancel/die interrupt occurs in
  // that window, we would fail to notice it until after we acquire the
  // lock (or get another interrupt to escape the semop()).  We can
  // avoid this problem by temporarily setting ImmediateInterruptOK to
  // true before we do CHECK_FOR_INTERRUPTS; then, a die() interrupt in
  // this interval will execute directly.  However, there is a huge
  // pitfall: there is another window of a few instructions after the
  // semop() before we are able to reset ImmediateInterruptOK.  If an
  // interrupt occurs then, we'll lose control, which means that the
  // lock has been acquired but our caller did not get a chance to
  // record the fact. Therefore, we only set ImmediateInterruptOK if the
  // caller tells us it's OK to do so, ie, the caller does not need to
  // record acquiring the lock.  (This is currently true for lockmanager
  // locks, since the process that granted us the lock did all the
  // necessary state updates. It's not true for SysV semaphores used to
  // emulate spinlocks --- but our performance on such platforms is so
  // horrible anyway that I'm not going to worry too much about it.)
  do {
    ImmediateInterruptOK = interrupt_ok;
    CHECK_FOR_INTERRUPTS();
    err_status = semop(sem_id, &sops, 1);
    ImmediateInterruptOK = false;
  } while (err_status == -1 && errno == EINTR);

  if (err_status == -1) {
    fprintf(stderr, "%s: semop(id=%d) failed: %s\n", __func__, sem_id, strerror(errno));
    proc_exit(255);
  }
}

void ipc_semaphore_unlock(IpcSemaphoreId sem_id, int sem) {
  int err_status;
  struct sembuf sops;

  sops.sem_op = 1;
  sops.sem_flg = 0;
  sops.sem_num = sem;

  // Note: if errStatus is -1 and errno == EINTR then it means we
  // returned from the operation prematurely because we were sent a
  // signal. So we try and unlock the semaphore again. Not clear this
  // can really happen, but might as well cope.
  do {
    err_status = semop(sem_id, &sops, 1);
  } while (err_status == -1 && errno == EINTR);

  if (err_status == -1) {
    fprintf(stderr, "%s: semop(id=%d) failed: %s\n", __func__, sem_id, strerror(errno));
    proc_exit(255);
  }
}

bool ipc_semaphore_try_lock(IpcSemaphoreId sem_id, int sem) {
  int err_status;
  struct sembuf sops;

  sops.sem_op = -1;
  sops.sem_flg = IPC_NOWAIT;
  sops.sem_num = sem;

  // Note: if errStatus is -1 and errno == EINTR then it means we
  // returned from the operation prematurely because we were sent a
  // signal. So we try and lock the semaphore again.
  do {
    err_status = semop(sem_id, &sops, 1);
  } while (err_status == -1 && errno == EINTR);

  if (err_status == -1) {
    if (errno == EAGAIN) {
      return false;
    }

    if (errno == EWOULDBLOCK) {
      return false;
    }

    fprintf(stderr, "%s: semop(id=%d) failed: %s\n", __func__, sem_id, strerror(errno));
    proc_exit(255);
  }

  return true;
}

int ipc_semaphore_get_value(IpcSemaphoreId sem_id, int sem) {
  union semun dummy;

  dummy.val = 0;

  return semctl(sem_id, sem, GETVAL, dummy);
}

// These routines represent a fairly thin layer on top of SysV shared
// memory functionality.

PGShmemHeader* ipc_memory_create(uint32 size, bool make_private, int permission) {
  void* mem_addr;
  PGShmemHeader* hdr;

  ASSERT(size > MAX_ALIGN(sizeof(PGShmemHeader)));

  for (NextShmemSegID++;; NextShmemSegID++) {
    IpcMemoryId shm_id;

    // Special case if creating a private segment --- just malloc() it
    if (make_private) {
      mem_addr = private_memory_create(size);
      break;
    }

    // Try to create new segment.
    mem_addr = internal_ipc_memory_create(NextShmemSegID, size, permission);

    if (mem_addr) {
      break;
    }

    // See if it looks to be leftover from a dead Postgres process.
    shm_id = shmget(NextShmemSegID, sizeof(PGShmemHeader), 0);

    if (shm_id < 0) {
      continue;
    }

    mem_addr = shmat(shm_id, 0, 0);

    if (mem_addr == (void*)-1) {
      continue;
    }

    hdr = (PGShmemHeader*)mem_addr;

    if (hdr->magic != PG_SHMEM_MAGIC) {
      shmdt(mem_addr);
      continue;
    }

    // If the creator PID is my own PID or does not belong to any
    // extant process, it's safe to zap it.
    if (hdr->creator_pid != getpid()) {
      if (kill(hdr->creator_pid, 0) == 0 || errno != ESRCH) {
        shmdt(mem_addr);
        continue;
      }
    }

    // The segment appears to be from a dead Postgres process, or from
    // a previous cycle of life in this same process.  Zap it, if
    // possible.  This probably shouldn't fail, but if it does, assume
    // the segment belongs to someone else after all, and continue
    // quietly.
    shmdt(mem_addr);

    if (shmctl(shm_id, IPC_RMID, (struct shmid_ds*)NULL) < 0) {
      continue;
    }

    // Now try again to create the segment.
    mem_addr = internal_ipc_memory_create(NextShmemSegID, size, permission);

    if (mem_addr) {
      break;
    }

    // Can only get here if some other process managed to create the
    // same shmem key before we did.  Let him have that one, loop
    // around to try next key.
  }

  // OK, we created a new segment.  Mark it as created by this process.
  // The order of assignments here is critical so that another Postgres
  // process can't see the header as valid but belonging to an invalid
  // PID!
  hdr = (PGShmemHeader*)mem_addr;
  hdr->creator_pid = getpid();
  hdr->magic = PG_SHMEM_MAGIC;

  // Initialize space allocation status for segment.
  hdr->total_size = size;
  hdr->free_offset = MAX_ALIGN(sizeof(PGShmemHeader));

  return hdr;
}

bool shared_memory_is_in_use(IpcMemoryKey shm_key, IpcMemoryId shm_id) {
  struct shmid_ds shm_stat;

  // We detect whether a shared memory segment is in use by seeing
  // whether it (a) exists and (b) has any processes are attached to it.
  //
  // If we are unable to perform the stat operation for a reason other than
  // nonexistence of the segment (most likely, because it doesn't belong
  // to our userid), assume it is in use.
  if (shmctl(shm_id, IPC_STAT, &shm_stat) < 0) {
    // EINVAL actually has multiple possible causes documented in the
    // shmctl man page, but we assume it must mean the segment no
    // longer exists.
    if (errno == EINVAL) {
      return false;
    }

    return true;
  }

  // If it has attached processes, it's in use.
  if (shm_stat.shm_nattch != 0) {
    return true;
  }

  return false;
}

// Attempt to create a new semaphore set with the specified key.
// Will fail (return -1) if such a set already exists.
// On success, a callback is optionally registered with on_shmem_exit
// to delete the semaphore set when on_shmem_exit is called.
//
// If we fail with a failure code other than collision-with-existing-set,
// print out an error and abort.  Other types of errors are not recoverable.
static IpcSemaphoreId internal_ipc_semaphore_create(IpcSemaphoreKey sem_key, int num_sems, int permission,
                                                    int sem_start_value, bool remove_on_exit) {
  int sem_id;
  int i;
  u_short array[IPC_NMAX_SEM];
  union semun semun;

  ASSERT(num_sems > 0 && num_sems <= IPC_NMAX_SEM);

  sem_id = semget(sem_key, num_sems, IPC_CREAT | IPC_EXCL | permission);

  if (sem_id < 0) {
    // Fail quietly if error indicates a collision with existing set.
    // One would expect EEXIST, given that we said IPC_EXCL, but
    // perhaps we could get a permission violation instead?  Also,
    // EIDRM might occur if an old set is slated for destruction but
    // not gone yet.
    if (errno == EEXIST || errno == EACCES
#ifdef EIDRM
        || errno == EIDRM
#endif
    ) {
      return -1;
    }

    // Else complain and abort.
    fprintf(stderr, "%s: semget(key=%d, num=%d, 0%o) failed: %s\n", __func__, (int)sem_key, num_sems,
            (IPC_CREAT | IPC_EXCL | permission), strerror(errno));

    if (errno == ENOSPC) {
      fprintf(stderr,
              "\nThis error does *not* mean that you have run out of disk space.\n\n"
              "It occurs either because system limit for the maximum number of\n"
              "semaphore sets (SEMMNI), or the system wide maximum number of\n"
              "semaphores (SEMMNS), would be exceeded.  You need to raise the\n"
              "respective kernel parameter.  Look into the PostgreSQL documentation\n"
              "for details.\n\n");
    }

    proc_exit(1);
  }

  // Initialize new semas to specified start value.
  for (i = 0; i < num_sems; i++) {
    array[i] = sem_start_value;
  }

  semun.array = array;

  if (semctl(sem_id, 0, SETALL, semun) < 0) {
    fprintf(stderr, "%s: semctl(id=%d, 0, SETALL, ...) failed: %s\n", __func__, sem_id, strerror(errno));

    if (errno == ERANGE) {
      fprintf(stderr,
              "You possibly need to raise your kernel's SEMVMX value to be at least\n"
              "%d.  Look into the PostgreSQL documentation for details.\n",
              sem_start_value);
    }

    ipc_semaphore_kill(sem_id);
    proc_exit(1);
  }

  // Register on-exit routine to delete the new set.
  if (remove_on_exit) {
    on_shmem_exit(callback_semaphore_kill, INT32_GET_DATUM(sem_id));
  }

  return sem_id;
}

static void callback_semaphore_kill(int status, Datum sem_id) { ipc_semaphore_kill(DATUM_GET_INT32(sem_id)); }

static pid_t ipc_semaphore_get_last_pid(IpcSemaphoreId sem_id, int sem) {
  union semun dummy;

  dummy.val = 0;

  return semctl(sem_id, sem, GETPID, dummy);
}

// Attempt to create a new shared memory segment with the specified key.
// Will fail (return NULL) if such a segment already exists.  If successful,
// attach the segment to the current process and return its attached address.
// On success, callbacks are registered with on_shmem_exit to detach and
// delete the segment when on_shmem_exit is called.
//
// If we fail with a failure code other than collision-with-existing-segment,
// print out an error and abort.  Other types of errors are not recoverable.
static void* internal_ipc_memory_create(IpcMemoryKey mem_key, uint32 size, int permission) {
  IpcMemoryId shm_id;
  void* mem_addr;

  shm_id = shmget(mem_key, size, IPC_CREAT | IPC_EXCL | permission);

  if (shm_id < 0) {
    // Fail quietly if error indicates a collision with existing
    // segment. One would expect EEXIST, given that we said IPC_EXCL,
    // but perhaps we could get a permission violation instead?  Also,
    // EIDRM might occur if an old seg is slated for destruction but
    // not gone yet.
    if (errno == EEXIST || errno == EACCES
#ifdef EIDRM
        || errno == EIDRM
#endif
    ) {
      return NULL;
    }

    // Else complain and abort.
    if (errno == EINVAL) {
      fprintf(stderr,
              "\nThis error can be caused by one of three things:\n\n"
              "1. The maximum size for shared memory segments on your system was\n"
              "   exceeded.  You need to raise the SHMMAX parameter in your kernel\n"
              "   to be at least %u bytes.\n\n"
              "2. The requested shared memory segment was too small for your system.\n"
              "   You need to lower the SHMMIN parameter in your kernel.\n\n"
              "3. The requested shared memory segment already exists but is of the\n"
              "   wrong size.  This is most likely the case if an old version of\n"
              "   PostgreSQL crashed and didn't clean up.  The `ipcclean' utility\n"
              "   can be used to remedy this.\n\n"
              "The PostgreSQL Administrator's Guide contains more information about\n"
              "shared memory configuration.\n\n",
              size);
    } else if (errno == ENOSPC) {
      fprintf(stderr,
              "\nThis error does *not* mean that you have run out of disk space.\n\n"
              "It occurs either if all available shared memory ids have been taken,\n"
              "in which case you need to raise the SHMMNI parameter in your kernel,\n"
              "or because the system's overall limit for shared memory has been\n"
              "reached.  The PostgreSQL Administrator's Guide contains more\n"
              "information about shared memory configuration.\n\n");
    }

    proc_exit(1);
  }

  // Register on-exit routine to delete the new segment.
  on_shmem_exit(ipc_memory_delete, INT32_GET_DATUM(shm_id));
  mem_addr = shmat(shm_id, 0, 0);

  if (mem_addr == (void*)-1) {
    fprintf(stderr, "%s: shmat(id=%d) failed: %s\n", __func__, shm_id, strerror(errno));
    proc_exit(1);
  }

  // Register on-exit routine to detach new segment before deleting.
  on_shmem_exit(ipc_memory_detach, POINTER_GET_DATUM(mem_addr));

  // Record key and ID in lockfile for data directory.
  record_shared_memory_in_lock_file(mem_key, shm_id);

  return mem_addr;
}

// Removes a shared memory segment from process' address space.
// (called as an on_shmem_exit callback, hence funny argument list)
static void ipc_memory_detach(int status, Datum shm_addr) {
  if (shmdt(DATUM_GET_POINTER(shm_addr)) < 0) {
    fprintf(stderr, "%s: shmdt(%p) failed: %s\n", __func__, DATUM_GET_POINTER(shm_addr), strerror(errno));
  }

  // We used to report a failure via elog(NOTICE), but that's pretty
  // pointless considering any client has long since disconnected ...
}

static void ipc_memory_delete(int status, Datum shm_id) {
  if (shmctl(DATUM_GET_INT32(shm_id), IPC_RMID, (struct shmid_ds*)NULL) < 0) {
    fprintf(stderr, "%s: shmctl(%d, %d, 0) failed: %s\n", __func__, DATUM_GET_INT32(shm_id), IPC_RMID, strerror(errno));
  }

  // We used to report a failure via elog(NOTICE), but that's pretty
  // pointless considering any client has long since disconnected ...
}

// Rather than allocating shmem segments with IPC_PRIVATE key, we
// just malloc() the requested amount of space. This code emulates
// the needed shmem functions.
static void* private_memory_create(uint32 size) {
  void* mem_addr;

  mem_addr = malloc(size);

  if (!mem_addr) {
    fprintf(stderr, "%s: malloc(%u) failed\n", __func__, size);
    proc_exit(1);
  }

  MEMSET(mem_addr, 0, size);

  on_shmem_exit(private_memory_delete, POINTER_GET_DATUM(mem_addr));

  return mem_addr;
}

static void private_memory_delete(int status, Datum mem_addr) { free(DATUM_GET_POINTER(mem_addr)); }