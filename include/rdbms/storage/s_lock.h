// =========================================================================
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
// =========================================================================

// Description
//  The public macros that must be provided are:
//
//  void S_INIT_LOCK(slock_t *lock)
//  void S_LOCK(slock_t *lock)
//  void S_UNLOCK(slock_t *lock)
//  void S_LOCK_FREE(slock_t *lock)

#ifndef RDBMS_STORAGE_S_LOCK_H_
#define RDBMS_STORAGE_S_LOCK_H_

typedef unsigned char slock_t;

static inline int tas(volatile slock_t* lock) {
  slock_t _res = 1;

  __asm__("lock; xchgb %0, %1" : "=q"(_res), "=m"(*lock) : "0"(_res));

  return _res;
}

#define TAS(lock) tas((volatile slock_t*)lock)

void s_lock(volatile slock_t* lock, const char* file, const int line);

#define S_INIT_LOCK(lock) S_UNLOCK(lock)

#define S_LOCK(lock)                                                                       \
  do {                                                                                     \
    if (TAS((volatile slock_t*)lock)) s_lock((volatile slock_t*)lock, __FILE__, __LINE__); \
  } while (0)

#define S_UNLOCK(lock)    (*(lock) = 0)
#define S_LOCK_FREE(lock) (*(lock) == 0)

#endif  // RDBMS_STORAGE_S_LOCK_H_
