//===----------------------------------------------------------------------===//
//
// postgres.c
//  POSTGRES C Backend Interface
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/tcop/postgres.c,v 1.154 2000/04/30 21:29:23 tgl Exp $
//
// NOTES
//  This is the "main" module of the postgres backend and
//  hence the main module of the "traffic cop".
//
//===----------------------------------------------------------------------===//
#include "rdbms/postgres.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "rdbms/lib/"
#include "rdbms/tcop/dest.h"
#include "rdbms/utils/trace.h"

#define VERBOSE                           PgOptions[TRACE_VERBOSE]
#define DEBUG_PRINT_QUERY                 PgOptions[TRACE_QUERY]
#define DEBUG_PRINT_PLAN                  PgOptions[TRACE_PLAN]
#define DEBUG_PRINT_PARSE                 PgOptions[TRACE_PARSE]
#define DEBUG_PRINT_REWRITTEN_PARSE_TREE  PgOptions[TRACE_REWRITTEN]
#define DEBUG_PPRINT_PLAN                 PgOptions[TRACE_PRETTY_PLAN]
#define DEBUG_PPRINT_PARSE                PgOptions[TRACE_PRETTY_PARSE]
#define DEBUG_PPRINT_REWRITTEN_PARSE_TREE PgOptions[TRACE_PRETTY_REWRITTEN]
#define SHOW_PARSER_STATS                 PgOptions[TRACE_PARSERSTATS]
#define SHOW_PLANNER_STATS                PgOptions[TRACE_PLANNERSTATS]
#define SHOW_EXECUTOR_STATS               PgOptions[TRACE_EXECUTORSTATS]

#ifdef LOCK_MGR_DEBUG
#define LOCK_DEBUG PgOptions[TRACE_LOCKS]
#endif

#define DEAD_LOCK_CHECK_TIMER PgOptions[OPT_DEADLOCKTIMEOUT]
#define HOST_NAME_LOOK_UP     PgOptions[OPT_HOSTLOOKUP]
#define SHOW_PORT_NUMBER      PgOptions[OPT_SHOWPORTNUMBER]

CommandDest WhereToSendOutput = ENUM_DEBUG;

extern void base_init();
extern void startup_xlog();
extern void shutdown_xlog();
extern void handle_deadlock(int postgres_signal_arg);

extern char XLogDir[];
extern char ControlFilePath[];

extern int LockingOff;
extern int NBuffers;

int DontExecute = 0;
static int ShowStats;
static bool IsEmptyQuery = false;

sigjmp_buf WarnRestart;
bool WarnRestartReady = false;
bool InError = false;
bool ExitAfterAbort = false;

static bool EchoQuery = false;  // Default don't echo.
char PgPathname[MAX_PG_PATH];
FILE* StatFp = NULL;

// People who want to use EOF should #define DONTUSENEWLINE in tcop/tcopdebug.h.
#ifndef TCOP_DONT_USE_NEWLINE
int UseNewLine = 1;  // Use newlines query delimiters (the default).
#else
int UseNewLine = 0;
#endif

// Flags for expensive function optimization -- JMH 3/9/92
int XfuncMode = 0;

// Decls for routines only used in this file.
static int interactive_backend(StringInfo in_buf);