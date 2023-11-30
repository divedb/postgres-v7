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

#include "rdbms/postgres.h"
#include "rdbms/storage/ipc.h"
#include "rdbms/utils/elog.h"

static void smgr_shutdown(int dummy);

typedef struct f_smgr {
  int (*smgr_init)();      // May be NULL.
  int (*smgr_shutdown)();  // May be NULL.
  int (*smgr_create)(Relation relation);
  int (*smgr_unlink)(Relation relation);
  int (*smgr_extend)(Relation relation, char* buffer);
  int (*smgr_open)(Relation relation);
  int (*smgr_close)(Relation relation);
  int (*smgr_read)(Relation relation, BlockNumber block_num, char* buffer);
  int (*smgr_flush)(Relation relation, BlockNumber block_num, char* buffer);
  int (*smgr_blind_wrt)(char* db_name, char* rel_name, Oid db_id, Oid rel_id, BlockNumber blk_no, char* buffer,
                        bool do_fsync);
  int (*smgr_mark_dirty)(Relation relation, BlockNumber block_num);
  int (*smgr_blind_mark_dirty)(char* db_name, char* rel_name, Oid db_id, Oid rel_id, BlockNumber blk_no);
  int (*smgr_blocks)(Relation relation);
  int (*smgr_truncate)(Relation relation, int nblocks);
  int (*smgr_commit)();  // May be NULL.
  int (*smgr_abort)();   // May be NULL.
} f_smgr;

// The weird placement of commas in this init block is to keep the compiler
// happy, regardless of what storage managers we have (or don't have).
static f_smgr SmgrSW[] = {
    // Magnetic disk.
    {md_init, NULL, md_create, md_unlink, md_extend, md_open, md_close, md_read, md_write, md_flush, md_blind_wrt,
     md_mark_dirty, md_blind_mark_dirty, md_nblocks, md_truncate, md_commit, md_abort},

#ifdef STABLE_MEMORY_STORAGE

    // Main memory.
    {mminit, mmshutdown, mmcreate, mmunlink, mmextend, mmopen, mmclose, mmread, mmwrite, mmflush, mmblindwrt,
     mmmarkdirty, mmblindmarkdirty, mmnblocks, NULL, mmcommit, mmabort},

#endif
};

static int NSmgr = LENGTH_OF(SmgrSW);

// Initialize or shut down all storage managers.
int smgr_init() {
  int i;

  for (i = 0; i < NSmgr; i++) {
    if (SmgrSW[i].smgr_init) {
      if ((*(SmgrSW[i].smgr_init))() == SM_FAIL) {
        elog(FATAL, "%s: initialization failed on %s", __func__, smgrout(i));
      }
    }
  }

  // Register the shutdown proc.
  on_proc_exit(smgr_shutdown, NULL);

  return SM_SUCCESS;
}

// Create a new relation.
//
// This routine takes a reldesc, creates the relation on the appropriate
// device, and returns a file descriptor for it.
int smgr_create(int16 which, Relation relation) {
  int fd;

  if ((fd = (*(SmgrSW[which].smgr_create))(relation)) < 0) {
    elog(ERROR, "%s: cannot create %s", __func__, RELATION_GET_RELATION_NAME(relation));
  }

  return fd;
}

// Unlink a relation.
//
// The relation is removed from the store.
int smgr_unlink(int16 which, Relation relation) {
  int status;

  if ((status = (*(SmgrSW[which].smgr_unlink))(relation)) == SM_FAIL) {
    elog(ERROR, "%s: cannot unlink %s", __func__, RELATION_GET_RELATION_NAME(relation));
  }

  return status;
}

int smgr_extend(int16 which, Relation relation, char* buffer) {
    
}