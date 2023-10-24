// =========================================================================
//
// s_lock.c
//  buffer manager interface routines
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/buffer/s_lock.c,v 1.24 2000/01/26 05:56:52 momjian Exp $
//
// =========================================================================

#include "rdbms/storage/s_lock.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

// Each time we busy spin we select the next element of this array as the
// number of microseconds to wait. This accomplishes pseudo random back-off.
// Values are not critical but 10 milliseconds is a common platform
// granularity.
// note: total time to cycle through all 16 entries might be about .07 sec.
#define S_NSPINCYCLE 20
#define S_MAX_BUSY   1000 * S_NSPINCYCLE

int SpinCycle[S_NSPINCYCLE] = {0, 0, 0, 0, 10000, 0, 0, 0, 10000, 0, 0, 10000, 0, 0, 10000, 0, 10000, 0, 10000, 10000};

static void s_lock_stuck(volatile slock_t* lock, const char* file, const int line) {
  fprintf(stderr, "\nFATAL: s_lock(%p) at %s:%d, stuck spinlock. Aborting.\n", lock, file, line);
  fprintf(stdout, "\nFATAL: s_lock(%p) at %s:%d, stuck spinlock. Aborting.\n", lock, file, line);
  abort();
}

void s_lock_sleep(unsigned spin) {
  struct timeval delay;

  delay.tv_sec = 0;
  delay.tv_usec = SpinCycle[spin % S_NSPINCYCLE];
  (void)select(0, NULL, NULL, NULL, &delay);
}

void s_lock(volatile slock_t* lock, const char* file, const int line) {
  unsigned spins = 0;

  while (TAS(lock)) {
    s_lock_sleep(spins);

    if (++spins > S_MAX_BUSY) {
      s_lock_stuck(lock, file, line);
    }
  }
}
