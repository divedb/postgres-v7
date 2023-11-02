// =========================================================================
//
// miscadmin.h
//  this file contains general postgres administration and initialization
//  stuff that used to be spread out between the following files:
//      globals.h                           global variables
//      pdir.h                              directory path crud
//      pinit.h                             postgres initialization
//      pmod.h                              processing modes
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: miscadmin.h,v 1.55 2000/04/12 17:16:24 momjian Exp $
//
// NOTES
//  some of the information in this file will be moved to
//  other files.
//
// =========================================================================

#ifndef RDBMS_MISC_ADMIN_H_
#define RDBMS_MISC_ADMIN_H_

#include <sys/types.h>  // For pid_t

#include "rdbms/postgres.h"

extern bool Noversion;
extern bool Quiet;
extern bool QueryCancel;
extern char* DataDir;

extern int MyProcPid;
extern struct Port* MyProcPort;
extern long MyCancelKey;

extern char OutputFileName[];

extern char* UserName;

#endif  // RDBMS_MISC_ADMIN_H_
