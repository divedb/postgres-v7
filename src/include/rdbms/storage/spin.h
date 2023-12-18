// =========================================================================
//
// spin.h
//  synchronization routines
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: spin.h,v 1.11 2000/01/26 05:58:33 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_SPIN_H_
#define RDBMS_STORAGE_SPIN_H_

#include "rdbms/storage/ipc.h"

// Two implementations of spin locks
//
// sequent, sparc, sun3: real spin locks. uses a TAS instruction; see
// src/storage/ipc/s_lock.c for details.
//
// default: fake spin locks using semaphores.  see spin.c

typedef int SpinLock;

#ifdef STABLE_MEMORY_STORAGE
extern SpinLock MMCacheLock;
#endif

void create_spin_locks(IPCKey key);
void init_spin_locks();
void spin_acquire(SpinLock lock);
void spin_release(SpinLock lock);

#endif  // RDBMS_STORAGE_SPIN_H_
