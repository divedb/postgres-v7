// =========================================================================
//
// smgr.h
//  storage manager switch public interface declarations.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: smgr.h,v 1.20 2000/04/12 17:16:52 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_SMGR_H_
#define RDBMS_STORAGE_SMGR_H_

#include "rdbms/storage/block.h"
#include "rdbms/utils/rel.h"

#define SM_FAIL      0
#define SM_SUCCESS   1
#define DEFAULT_SMGR 0

int smgr_init();

// In md.c
int md_init();
int md_create(Relation relation);
int md_unlink(Relation relation);
int md_extend(Relation relation, char* buffer);
int md_open(Relation relation);
int md_close(Relation relation);
int md_read(Relation relation, BlockNumber block_num, char* buffer);
int md_write(Relation relation, BlockNumber block_num, char* buffer);
int md_flush(Relation relation, BlockNumber block_num, char* buffer);
int md_blind_wrt(char* db_name, char* rel_name, Oid db_id, Oid rel_id, BlockNumber block_num, char* buffer,
                 bool do_fsync);
int md_mark_dirty(Relation relation, BlockNumber block_num);
int md_blind_mark_dirty(char* db_name, char* rel_name, Oid db_id, Oid rel_id, BlockNumber block_num);
int md_nblocks(Relation relation);
int md_truncate(Relation relation, int bnlocks);
int md_commit();
int md_abort();

#endif  // RDBMS_STORAGE_SMGR_H_
