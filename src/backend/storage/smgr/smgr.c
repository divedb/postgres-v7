// =========================================================================
//
// smgr.c
//  public interface routines to storage manager switch.
//
//  All file system operations in POSTGRES dispatch through these
//  routines.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/smgr/smgr.c,v 1.35 2000/04/12 17:15:42 momjian Exp $
//
// =========================================================================

#include "rdbms/storage/smgr.h"

static void smgr_shutdown(int dummy);

typedef struct f_smgr {
  int (*smgr_init)(void);      // May be NULL.
  int (*smgr_shutdown)(void);  // May be NULL.
  int (*smgr_create)(Relation rel);

} f_smgr;