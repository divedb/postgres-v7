//===----------------------------------------------------------------------===//
//
// postgres.c
//  POSTGRES C Backend Interface
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
// $Header:
//  home/projects/pgsql/cvsroot/pgsql/src/backend/tcop/postgres.c
//  v 1.218 2001/04/14 19:11:45 momjian Exp $
//
// NOTES
//  this is the "main" module of the postgres backend and
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

#include "rdbms/lib/stringinfo.h"
#include "rdbms/miscadmin.h"
#include "rdbms/nodes/nodes.h"
#include "rdbms/nodes/pg_list.h"
#include "rdbms/tcop/dest.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/trace.h"

extern int optind;
extern char* optarg;

char* DebugQueryString;  // Used by pgmonitor

// For ps display.
bool HostnameLookup;
bool ShowPortNumber;
bool LogConnections = false;
CommandDest WhereToSendOutput = CMD_DEST_DEBUG;

static bool DontExecute = false;

// Note: these declarations had better match tcopprot.h
sigjmp_buf WarnRestart;

bool WarnRestartReady = false;
bool InError = false;

static bool EchoQuery = false;  // Default don't echo
char PgPathname[MAX_PG_PATH];
FILE* StatFp = NULL;

// Decls for routines only used in this file.
static int interactive_backend(StringInfo in_buf);

// People who want to use EOF should #define DONTUSENEWLINE in
// tcop/tcopdebug.h

#ifndef TCOP_DONTUSENEWLINE
int UseNewLine = 1;  // Use newlines query delimiters (the default)
#else
int UseNewLine = 0;  // Use EOF as query delimiters
#endif  // TCOP_DONTUSENEWLINE

// Flags for expensive function optimization -- JMH 3/9/92
int XfuncMode = 0;

static int interactive_backend(StringInfo in_buf);
static int socket_backend(StringInfo in_buf);
static int read_command(StringInfo in_buf);
static List* pg_parse_query(char* query_string, Oid* typev, int nargs);
static List* pg_analyze_and_rewrite(Node* parse_tree);
static void start_xact_command();
static void finish_xact_command();
static void sig_hup_handler(int);
static void float_exception_handler(int);

// Flag to mark SIGHUP. Whenever the main loop comes around it
// will reread the configuration file. (Better than doing the
// reading in the signal handler, ey?)
static volatile bool GotSigHup = false;

void process_interrupts() {
  if (InterruptHoldoffCount != 0 || CritSectionCount != 0) {
    return;
  }

  InterruptPending = false;

  if (ProcDiePending) {
    ProcDiePending = false;
    QueryCancelPending = false;    // ProcDie trumps QueryCancel
    ImmediateInterruptOK = false;  // Not idle anymore
    elog(FATAL, "This connection has been terminated by the administrator.");
  }

  if (QueryCancelPending) {
    QueryCancelPending = false;
    ImmediateInterruptOK = false;  // Not idle anymore
    elog(ERROR, "Query was cancelled.");
  }
}