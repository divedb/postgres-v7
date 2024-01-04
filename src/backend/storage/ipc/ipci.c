/*-------------------------------------------------------------------------
 *
 * ipci.c
 *	  POSTGRES inter-process communication initialization code.
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/storage/ipc/ipci.c,v 1.40 2001/03/22 03:59:45 momjian
 *Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "rdbms/storage/ipc.h"

// Creates and initializes shared memory and semaphores.
//
// This is called by the postmaster or by a standalone backend.
// It is NEVER called by a backend forked from the postmaster;
// for such a backend, the shared memory is already ready-to-go.
//
// If "make_private" is true then we only need private memory, not shared
// memory. This is true for a standalone backend, false for a postmaster.
void create_shared_memory_and_semaphores(bool make_private, int max_backends) {
  int size;
  PGShmemHeader* seg_hdr;

  // Size of the Postgres shared-memory block is estimated via
  // moderately-accurate estimates for the big hogs, plus 100K for the
  // stuff that's too small to bother with estimating.
}