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
extern char* DataDir;  // 数据目录

extern int MyProcPid;  // 当前进程ID
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

// Date/Time Configuration.
//
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
#define MAX_TZ_LEN 7

#define USE_POSTGRES_DATES 0
#define USE_ISO_DATES      1
#define USE_SQL_DATES      2
#define USE_GERMAN_DATES   3

extern int DateStyle;
extern bool EuroDates;
extern bool HasCTZSet;
extern bool CDayLight;
extern int CTimeZone;
extern char CTZName[];

extern char FloatFormat[];
extern char DateFormat[];

#define DISABLE_FSYNC PgOptions[OPT_NOFSYNC]
extern bool AllowSystemTableMods;
extern int SortMem;

extern Oid LastOidProcessed;  // For query rewrite.

// pdir.h --
//  POSTGRES directory path definitions.
extern char* DatabaseName;
extern char* DatabasePath;

// in utils/misc/database.c
void get_raw_database_info(const char* name, Oid* db_id, char* path);
char* expand_database_path(const char* path);

// now in utils/init/miscinit.c.
void set_database_name(const char* name);
void set_database_path(const char* path);

// Even if MULTIBYTE is not enabled, this function is neccesary
// since pg_proc.h does have.
const char* get_database_encoding();
const char* pg_encoding_to_char(int);
int pg_char_to_encoding(const char*);

char* get_pg_username();
void set_pg_username();
int get_user_id();
void set_user_id();
int validate_binary(char* path);
int find_exec(char* backend, char* argv0, char* binary_name);
int check_path_access(char* path, char* name, int open_mode);

// Lower case version for case-insensitive SQL referenced in pg_proc.h.
#define GET_PG_USER_NAME() get_pg_username()

// pmod.h --
//  POSTGRES processing mode definitions.
// Description:
//  There are three processing modes in POSTGRES.  They are
// "BootstrapProcessing or "bootstrap," InitProcessing or
// "initialization," and NormalProcessing or "normal."
//
// The first two processing modes are used during special times. When the
// system state indicates bootstrap processing, transactions are all given
// transaction id "one" and are consequently guarenteed to commit. This mode
// is used during the initial generation of template databases.
//
// Initialization mode until all normal initialization is complete.
// Some code behaves differently when executed in this mode to enable
// system bootstrapping.
//
// If a POSTGRES binary is in normal mode, then all code may be executed
// normally.
typedef enum ProcessingMode {
  BootstrapProcessing,  // Bootstrap creation of template database.
  InitProcessing,       // Initializing system.
  NormalProcessing      // Normal processing.
} ProcessingMode;

// pinit.h --
//  POSTGRES initialization and cleanup definitions.
//
// Note:
//  XXX AddExitHandler not defined yet.
typedef int16 ExitStatus;

#define NORMAL_EXIT_STATUS 0
#define FATAL_EXIT_STATUS  127

// in utils/init/postinit.c
extern bool PostgresIsInitialized;
void init_postgres(const char* dbname);

// TODO(gc): proc_exec or proc_exit
#define EXIT_POSTGRES(status) proc_exit(status)

// Processing mode support stuff.
extern ProcessingMode Mode;

#define IS_BOOTSTRAP_PROCESSING_MODE() (Mode == BootstrapProcessing)
#define IS_INIT_PROCESSING_MODE()      (Mode == InitProcessing)
#define IS_NORMAL_PROCESSING_MODE()    (Mode == NormalProcessing)

#define SET_PROCESSING_MODE(mode)                                                              \
  do {                                                                                         \
    assert(mode == BootstrapProcessing || mode == InitProcessing || mode == NormalProcessing); \
    Mode = mode;                                                                               \
  } while (0)

#define GET_PROCESSING_MODE() Mode

void ignore_system_indexes(bool mode);
bool is_ignoring_system_indexes();
bool is_cache_initialized();
void set_waiting_for_lock(bool);

// "postmaster.pid" is a file containing postmaster's pid, being
// created uder $PGDATA upon postmaster's starting up. When postmaster
// shuts down, it will be unlinked.
#define PID_FNAME "postmaster.pid"

void set_pid_fname(char* datadir);
char* get_pid_fname();
void unlink_pid_file();
int set_pid_file(pid_t pid);

#endif  // RDBMS_MISC_ADMIN_H_
