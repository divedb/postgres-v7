//===----------------------------------------------------------------------===//
//
// miscinit.c
//  miscellaneous initialization support stuff
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
// $Header:
//  home/projects/pgsql/cvsroot/pgsql/src/backend/utils/init/miscinit.c
//  v 1.65.2.1 2001/08/08 22:25:45 tgl Exp $
//
//===----------------------------------------------------------------------===//
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "rdbms/miscadmin.h"
#include "rdbms/postgres.h"

#ifdef CYR_RECODE
unsigned char RecodeForwTable[128];
unsigned char RecodeBackTable[128];
#endif

ProcessingMode Mode = InitProcessing;

// Note: we rely on these to initialize as zeroes.
static char DirectoryLockFile[MAX_PG_PATH];
static char SocketLockFile[MAX_PG_PATH];

static bool IsIgnoringSystemIndexes = false;

bool is_ignoring_system_indexes() { return IsIgnoringSystemIndexes; }