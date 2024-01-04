//===----------------------------------------------------------------------===//
//
// miscadmin.h
//  This file contains general postgres administration and initialization
//  stuff that used to be spread out between the following files:
//      globals.h   global variables
//      pdir.h      directory path crud
//      pinit.h     postgres initialization
//      pmod.h      processing modes
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: miscadmin.h,v 1.83 2001/03/22 04:00:25 momjian Exp $
//
// NOTES
//  Some of the information in this file should be moved to
//  other files.
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_MISC_ADMIN_H_
#define RDBMS_MISC_ADMIN_H_

#include <sys/types.h>  // For pid_t

#include "rdbms/storage/ipc.h"

// System interrupt and critical section handling
//
// There are two types of interrupts that a running backend needs to accept
// without messing up its state: QueryCancel (SIGINT) and ProcDie (SIGTERM).
// In both cases, we need to be able to clean up the current transaction
// gracefully, so we can't respond to the interrupt instantaneously ---
// there's no guarantee that internal data structures would be self-consistent
// if the code is interrupted at an arbitrary instant. Instead, the signal
// handlers set flags that are checked periodically during execution.
//
// The CHECK_FOR_INTERRUPTS() macro is called at strategically located spots
// where it is normally safe to accept a cancel or die interrupt.  In some
// cases, we invoke CHECK_FOR_INTERRUPTS() inside low-level subroutines that
// might sometimes be called in contexts that do *not* want to allow a cancel
// or die interrupt.  The HOLD_INTERRUPTS() and RESUME_INTERRUPTS() macros
// allow code to ensure that no cancel or die interrupt will be accepted,
// even if CHECK_FOR_INTERRUPTS() gets called in a subroutine. The interrupt
// will be held off until the last matching RESUME_INTERRUPTS() occurs.
//
// Special mechanisms are used to let an interrupt be accepted when we are
// waiting for a lock or spinlock, and when we are waiting for command input
// (but, of course, only if the interrupt holdoff counter is zero).  See the
// related code for details.
//
// A related, but conceptually distinct, mechanism is the "critical section"
// mechanism.  A critical section not only holds off cancel/die interrupts,
// but causes any elog(ERROR) or elog(FATAL) to become elog(STOP) --- that is,
// a system-wide reset is forced.  Needless to say, only really *critical*
// code should be marked as a critical section!  Currently, this mechanism
// is only used for XLOG-related code.

// globals.c
// These are marked volatile because they are set by signal handlers:
extern volatile bool InterruptPending;
extern volatile bool QueryCancelPending;
extern volatile bool ProcDiePending;

// These are marked volatile because they are examined by signal handlers:
extern volatile bool ImmediateInterruptOK;
extern volatile uint32 InterruptHoldoffCount;
extern volatile uint32 CritSectionCount;

// postgres.c
void process_interrupts();

#define CHECK_FOR_INTERRUPTS()                  \
  do {                                          \
    if (InterruptPending) process_interrupts(); \
  } while (0)

#define HOLD_INTERRUPTS() (InterruptHoldoffCount++)

#define RESUME_INTERRUPTS()                    \
  do {                                         \
    Assert(InterruptHoldoffCount > 0);         \
    InterruptHoldoffCount--;                   \
    if (InterruptPending) ProcessInterrupts(); \
  } while (0)

#define START_CRIT_SECTION() (CritSectionCount++)
#define END_CRIT_SECTION()                      \
  do {                                          \
    ASSERT(CritSectionCount > 0);               \
    CritSectionCount--;                         \
    if (InterruptPending) process_interrupts(); \
  } while (0)

// postmaster/postmaster.c
int postmaster_main(int argc, char* argv[]);

// utils/init/globals.c
extern bool Noversion;
extern bool Quiet;
extern char* DataDir;  // 数据目录

extern int MyProcPid;  // 当前进程ID
extern struct Port* MyProcPort;
extern long MyCancelKey;

extern char OutputFileName[];

extern Oid MyDatabaseId;
extern bool IsUnderPostmaster;
extern int DebugLvl;

// Date/Time Configuration
//
// Constants to pass info from runtime environment:
//  USE_POSTGRES_DATES specifies traditional postgres format for output.
//  USE_ISO_DATES specifies ISO-compliant format for output.
//  USE_SQL_DATES specified Oracle/Ingres-compliant format for output.
//  USE_GERMAN_DATES specifies German-style dd.mm/yyyy date format.
//
// DateStyle specifies preference for date formatting for output.
// EuroDates if client prefers dates interpreted and written w/European
// conventions.
//
// HasCTZSet if client timezone is specified by client.
// CDayLight is the apparent daylight savings time status.
// CTimeZone is the timezone offset in seconds.
// CTZName is the timezone label.
#define MAX_TZ_LEN 10  // Max TZ name len, not counting tr. null

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

extern bool EnableFsync;
extern bool AllowSystemTableMods;
extern int SortMem;

// A few postmaster startup options are exported here so tht configuration file
// processor has access to them.
extern bool NetServer;
extern bool EnableSSL;
extern bool SilentMode;
extern int MaxBackends;
extern int NBuffers;
extern int PostPortNumber;
extern int UnixSocketPermissions;
extern char* UnixSocketGroup;
extern char* UnixSocketDir;
extern char* VirtualHost;

// POSTGRES directory path definitions.
extern char* DatabaseName;
extern char* DatabasePath;

// utils/misc/database.c
void get_raw_database_info(const char* name, Oid* db_id, char* path);
char* expand_database_path(const char* path);

// utils/init/miscinit.c
void set_database_name(const char* name);
void set_database_path(const char* path);

Oid get_user_id();
void set_user_id(Oid user_id);
Oid get_session_user_id();
void set_session_user_id(Oid user_id);
void set_session_user_id_from_username(const char* username);
void set_data_dir(const char* dir);
int find_exec(char* fullpath, const char* argv0, const char* binary_name);
int check_path_access(char* path, char* name, int open_mode);

#ifdef CYR_RECODE

extern void get_charset_by_host(char* table_name, int host,
                                const char* data_dir);
extern void set_charset(void);
extern char* convert_str(unsigned char* buff, int len, int dest);

#endif

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

// utils/init/postinit.c
extern int LockingOff;
void init_postgres(const char* dbname, const char* username);
void base_init();

// Processing mode support stuff.
extern ProcessingMode Mode;

#define IS_BOOTSTRAP_PROCESSING_MODE() (Mode == BootstrapProcessing)
#define IS_INIT_PROCESSING_MODE()      (Mode == InitProcessing)
#define IS_NORMAL_PROCESSING_MODE()    (Mode == NormalProcessing)

#define SET_PROCESSING_MODE(mode)                                           \
  do {                                                                      \
    ASSERT_ARG((mode) == BootstrapProcessing || (mode) == InitProcessing || \
               (mode) == NormalProcessing);                                 \
    Mode = (mode);                                                          \
  } while (0)

#define GET_PROCESSING_MODE() Mode

bool create_data_dir_lock_file(const char* data_dir, bool am_postmaster);
bool create_socket_lock_file(const char* socket_file, bool am_postmaster);
void touch_socket_lock_file();
void record_shared_memory_in_lock_file(IpcMemoryKey shm_key,
                                       IpcMemoryId shm_id);
void validate_pg_version(const char* path);

// These externs do not belong here...
void ignore_system_indexes(bool mode);
bool is_ignoring_system_indexes();
bool is_cache_initialized();

#endif  // RDBMS_MISC_ADMIN_H_
