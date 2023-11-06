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

// In md.c.
int md_init();
int md_create(Relation relation);

#endif  // RDBMS_STORAGE_SMGR_H_
