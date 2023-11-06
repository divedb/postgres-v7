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
static f_smgr smgrsw[] = {
    // magnetic disk.
    {mdinit, NULL, mdcreate, mdunlink, mdextend, mdopen, mdclose, mdread, mdwrite, mdflush, mdblindwrt, mdmarkdirty,
     mdblindmarkdirty, mdnblocks, mdtruncate, mdcommit, mdabort},

#ifdef STABLE_MEMORY_STORAGE
    // main memory.
    {mminit, mmshutdown, mmcreate, mmunlink, mmextend, mmopen, mmclose, mmread, mmwrite, mmflush, mmblindwrt,
     mmmarkdirty, mmblindmarkdirty, mmnblocks, NULL, mmcommit, mmabort},

#endif
};