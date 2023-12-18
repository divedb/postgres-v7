//===----------------------------------------------------------------------===//
//
// s_lock.h
//  This file contains the implementation (if any) for spinlocks.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/include/storage/s_lock.h,v 1.70 2000/04/12 17:16:51 momjian Exp $
//
//===----------------------------------------------------------------------===//

// Description
//  The public macros that must be provided are:
//
//  void S_INIT_LOCK(slock_t *lock)
//  void S_LOCK(slock_t *lock)
//  void S_UNLOCK(slock_t *lock)
//  void S_LOCK_FREE(slock_t *lock)

#ifndef RDBMS_STORAGE_S_LOCK_H_
#define RDBMS_STORAGE_S_LOCK_H_

typedef unsigned char TasLock;

void s_lock(volatile TasLock* lock, const char* file, const int line);

static inline int tas(volatile TasLock* lock) {
  TasLock res = 1;

  __asm__("lock; xchgb %0, %1" : "=q"(res), "=m"(*lock) : "0"(res));

  return res;
}

#define TAS(lock) tas((volatile TasLock*)lock)

#define INIT_LOCK(lock) LOCK_RELEASE(lock)
#define LOCK_ACQUIRE(lock)                                                                 \
  do {                                                                                     \
    if (TAS((volatile TasLock*)lock)) s_lock((volatile TasLock*)lock, __FILE__, __LINE__); \
  } while (0)
#define LOCK_RELEASE(lock) (*(lock) = 0)
#define LOCK_IS_FREE(lock) (*(lock) == 0)

#endif  // RDBMS_STORAGE_S_LOCK_H_
