// =========================================================================
//
// miscadmin.h
//  This file contains general postgres administration and initialization
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

// From utils/init/globals.c
extern bool Noversion;
extern bool Quiet;
extern bool QueryCancel;
extern char* DataDir;

extern int MyProcPid;
extern struct Port* MyProcPort;
extern long MyCancelKey;

extern char OutputFileName[];

extern char* UserName;

// From storage/backendid.h
extern bool MyDatabaseIdIsInitialized;
extern Oid MyDatabaseId;
extern bool TransactionInitWasProcessed;

extern bool IsUnderPostmaster;

extern int DebugLvl;

// Constants to pass info from runtime environment:
//  USE_POSTGRES_DATES specifies traditional postgres format for output.
//  USE_ISO_DATES specifies ISO-compliant format for output.
//  USE_SQL_DATES specified Oracle/Ingres-compliant format for output.
//  USE_GERMAN_DATES specifies German-style dd.mm/yyyy date format.
//
// DateStyle specifies preference for date formatting for output.
// EuroDates if client prefers dates interpreted and written w/European conventions.
//
// HasCTZSet if client timezone is specified by client.
// CDayLight is the apparent daylight savings time status.
// CTimeZone is the timezone offset in seconds.
// CTZName is the timezone label.
extern int DateStyle;
extern bool EuroDates;
extern bool HasCTZSet;
extern bool CDayLight;
extern int CTimeZone;
extern char CTZName[];

extern char FloatFormat[];
extern char DateFormat[];

#define DISABLE_FSYNC pg_options[OPT_NOFSYNC]
extern bool AllowSystemTableMods;
extern int SortMem;

extern Oid LastOidProcessed;  // For query rewrite.

// pdir.h --
//  POSTGRES directory path definitions.
extern char* DatabaseName;
extern char* DatabasePath;

#endif  // RDBMS_MISC_ADMIN_H_
