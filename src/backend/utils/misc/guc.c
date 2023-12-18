//===----------------------------------------------------------------------===//
// guc.c
//
// Support for grand unified configuration scheme, including SET
// command, configuration file, and command line options.
//
// $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/utils/misc/guc.c,v 1.35 2001/03/22 17:41:47 tgl Exp $
//
// Copyright 2000 by PostgreSQL Global Development Group
// Written by Peter Eisentraut <peter_e@gmx.net>.
//===----------------------------------------------------------------------===//

#include "rdbms/utils/guc.h"

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <unistd.h>

#include "rdbms/postgres.h"

// XXX these should be in other modules' header files.
extern bool LogConnections;
extern int CheckPointTimeout;
extern int CommitDelay;
extern int CommitSiblings;
extern bool FixBTree;

#ifdef ENABLE_SYSLOG
extern char* SyslogFacility;
extern char* SyslogIdent;
static bool check_facility(const char* facility);
#endif

// Debugging options
#ifdef USE_ASSERT_CHECKING
bool AssertEnabled = true;
#endif

bool DebugPrintQuery = false;
bool DebugPrintPlan = false;
bool DebugPrintParse = false;
bool DebugPrintRewritten = false;
bool DebugPrettyPrint = false;

bool ShowParserStats = false;
bool ShowPlannerStats = false;
bool ShowExecutorStats = false;
bool ShowQueryStats = false;
bool ShowBtreeBuildStats = false;

bool SQLInheritance = false;

#ifndef PG_KRB_SRVTAB
#define PG_KRB_SRVTAB ""
#endif

enum ConfigType { PGC_NONE = 0, PGC_BOOL, PGC_INT, PGC_REAL, PGC_STRING };

struct ConfigGeneric {
  const char* name;
  GucContext context;
  void* variable;
};

struct ConfigBool {
  const char* name;
  GucContext context;
  bool* variable;
  bool default_val;
};

struct ConfigInt {
  const char* name;
  GucContext context;
  int* variable;
  int default_val;
  int min;
  int max;
};

struct ConfigReal {
  const char* name;
  GucContext context;
  double* variable;
  double default_val;
  double min;
  double max;
};

// String value options are allocated with strdup, not with the
// pstrdup/palloc mechanisms. That is because configuration settings
// are already in place before the memory subsystem is up. It would
// perhaps be an idea to change that sometime.
struct ConfigString {
  const char* name;
  GucContext context;
  char** variable;
  const char* default_val;
  bool (*parse_hook)(const char* proposed);
  void (*assign_hook)(const char* newval);
};

// TO ADD AN OPTION:
//
// 1. Declare a global variable of type bool, int, double, or char*
// and make use of it.
//
// 2. Decide at what times it's safe to set the option. See guc.h for
// details.
//
// 3. Decide on a name, a default value, upper and lower bounds (if
// applicable), etc.
//
// 4. Add a record below.
//
// 5. Add it to postgresql.conf.sample
//
// 6. Don't forget to document that option.
//
// WHEN MAKING MODIFICATIONS, remember to update postgresql.conf.sample
static struct ConfigBool ConfigureNamesBool[] = {{"enable_seqscan", PGC_USERSET, &enable_seqscan, true},
                                                 {"enable_indexscan", PGC_USERSET, &enable_indexscan, true},
                                                 {"enable_tidscan", PGC_USERSET, &enable_tidscan, true},
                                                 {"enable_sort", PGC_USERSET, &enable_sort, true},
                                                 {"enable_nestloop", PGC_USERSET, &enable_nestloop, true},
                                                 {"enable_mergejoin", PGC_USERSET, &enable_mergejoin, true},
                                                 {"enable_hashjoin", PGC_USERSET, &enable_hashjoin, true},

                                                 {"ksqo", PGC_USERSET, &_use_keyset_query_optimizer, false},
                                                 {"geqo", PGC_USERSET, &enable_geqo, true},

                                                 {"tcpip_socket", PGC_POSTMASTER, &NetServer, false},
                                                 {"ssl", PGC_POSTMASTER, &EnableSSL, false},
                                                 {"fsync", PGC_SIGHUP, &enableFsync, true},
                                                 {"silent_mode", PGC_POSTMASTER, &SilentMode, false},

                                                 {"log_connections", PGC_SIGHUP, &Log_connections, false},
                                                 {"log_timestamp", PGC_SIGHUP, &Log_timestamp, false},
                                                 {"log_pid", PGC_SIGHUP, &Log_pid, false},

#ifdef USE_ASSERT_CHECKING
                                                 {"debug_assertions", PGC_USERSET, &assert_enabled, true},
#endif

                                                 {"debug_print_query", PGC_USERSET, &Debug_print_query, false},
                                                 {"debug_print_parse", PGC_USERSET, &Debug_print_parse, false},
                                                 {"debug_print_rewritten", PGC_USERSET, &Debug_print_rewritten, false},
                                                 {"debug_print_plan", PGC_USERSET, &Debug_print_plan, false},
                                                 {"debug_pretty_print", PGC_USERSET, &Debug_pretty_print, false},

                                                 {"show_parser_stats", PGC_USERSET, &Show_parser_stats, false},
                                                 {"show_planner_stats", PGC_USERSET, &Show_planner_stats, false},
                                                 {"show_executor_stats", PGC_USERSET, &Show_executor_stats, false},
                                                 {"show_query_stats", PGC_USERSET, &Show_query_stats, false},
#ifdef BTREE_BUILD_STATS
                                                 {"show_btree_build_stats", PGC_SUSET, &Show_btree_build_stats, false},
#endif

                                                 {"trace_notify", PGC_USERSET, &Trace_notify, false},

#ifdef LOCK_DEBUG
                                                 {"trace_locks", PGC_SUSET, &Trace_locks, false},
                                                 {"trace_userlocks", PGC_SUSET, &Trace_userlocks, false},
                                                 {"trace_spinlocks", PGC_SUSET, &Trace_spinlocks, false},
                                                 {"debug_deadlocks", PGC_SUSET, &Debug_deadlocks, false},
#endif

                                                 {"hostname_lookup", PGC_SIGHUP, &HostnameLookup, false},
                                                 {"show_source_port", PGC_SIGHUP, &ShowPortNumber, false},

                                                 {"sql_inheritance", PGC_USERSET, &SQL_inheritance, true},

                                                 {"fixbtree", PGC_POSTMASTER, &FixBTree, true},

                                                 {NULL, 0, NULL, false}};

static struct ConfigInt ConfigureNamesInt[] = {
    {"geqo_threshold", PGC_USERSET, &geqo_rels, DEFAULT_GEQO_RELS, 2, INT_MAX},
    {"geqo_pool_size", PGC_USERSET, &Geqo_pool_size, DEFAULT_GEQO_POOL_SIZE, 0, MAX_GEQO_POOL_SIZE},
    {"geqo_effort", PGC_USERSET, &Geqo_effort, 1, 1, INT_MAX},
    {"geqo_generations", PGC_USERSET, &Geqo_generations, 0, 0, INT_MAX},
    {"geqo_random_seed", PGC_USERSET, &Geqo_random_seed, -1, INT_MIN, INT_MAX},

    {"deadlock_timeout", PGC_POSTMASTER, &DeadlockTimeout, 1000, 0, INT_MAX},

#ifdef ENABLE_SYSLOG
    {"syslog", PGC_SIGHUP, &Use_syslog, 0, 0, 2},
#endif

    /*
     * Note: There is some postprocessing done in PostmasterMain() to make
     * sure the buffers are at least twice the number of backends, so the
     * constraints here are partially unused.
     */
    {"max_connections", PGC_POSTMASTER, &MaxBackends, DEF_MAXBACKENDS, 1, MAXBACKENDS},
    {"shared_buffers", PGC_POSTMASTER, &NBuffers, DEF_NBUFFERS, 16, INT_MAX},
    {"port", PGC_POSTMASTER, &PostPortNumber, DEF_PGPORT, 1, 65535},

    {"sort_mem", PGC_USERSET, &SortMem, 512, 1, INT_MAX},

    {"debug_level", PGC_USERSET, &DebugLvl, 0, 0, 16},

#ifdef LOCK_DEBUG
    {"trace_lock_oidmin", PGC_SUSET, &Trace_lock_oidmin, BootstrapObjectIdData, 1, INT_MAX},
    {"trace_lock_table", PGC_SUSET, &Trace_lock_table, 0, 0, INT_MAX},
#endif
    {"max_expr_depth", PGC_USERSET, &max_expr_depth, DEFAULT_MAX_EXPR_DEPTH, 10, INT_MAX},

    {"unix_socket_permissions", PGC_POSTMASTER, &Unix_socket_permissions, 0777, 0000, 0777},

    {"checkpoint_segments", PGC_SIGHUP, &CheckPointSegments, 3, 1, INT_MAX},

    {"checkpoint_timeout", PGC_SIGHUP, &CheckPointTimeout, 300, 30, 3600},

    {"wal_buffers", PGC_POSTMASTER, &XLOGbuffers, 8, 4, INT_MAX},

    {"wal_files", PGC_SIGHUP, &XLOGfiles, 0, 0, 64},

    {"wal_debug", PGC_SUSET, &XLOG_DEBUG, 0, 0, 16},

    {"commit_delay", PGC_USERSET, &CommitDelay, 0, 0, 100000},

    {"commit_siblings", PGC_USERSET, &CommitSiblings, 5, 1, 1000},

    {NULL, 0, NULL, 0, 0, 0}};

static struct ConfigReal ConfigureNamesReal[] = {
    {"effective_cache_size", PGC_USERSET, &effective_cache_size, DEFAULT_EFFECTIVE_CACHE_SIZE, 0, DBL_MAX},
    {"random_page_cost", PGC_USERSET, &random_page_cost, DEFAULT_RANDOM_PAGE_COST, 0, DBL_MAX},
    {"cpu_tuple_cost", PGC_USERSET, &cpu_tuple_cost, DEFAULT_CPU_TUPLE_COST, 0, DBL_MAX},
    {"cpu_index_tuple_cost", PGC_USERSET, &cpu_index_tuple_cost, DEFAULT_CPU_INDEX_TUPLE_COST, 0, DBL_MAX},
    {"cpu_operator_cost", PGC_USERSET, &cpu_operator_cost, DEFAULT_CPU_OPERATOR_COST, 0, DBL_MAX},

    {"geqo_selection_bias", PGC_USERSET, &Geqo_selection_bias, DEFAULT_GEQO_SELECTION_BIAS, MIN_GEQO_SELECTION_BIAS,
     MAX_GEQO_SELECTION_BIAS},

    {NULL, 0, NULL, 0.0, 0.0, 0.0}};

static struct ConfigString ConfigureNamesString[] = {
    {"krb_server_keyfile", PGC_POSTMASTER, &pg_krb_server_keyfile, PG_KRB_SRVTAB, NULL, NULL},

#ifdef ENABLE_SYSLOG
    {"syslog_facility", PGC_POSTMASTER, &Syslog_facility, "LOCAL0", check_facility, NULL},
    {"syslog_ident", PGC_POSTMASTER, &Syslog_ident, "postgres", NULL, NULL},
#endif

    {"unix_socket_group", PGC_POSTMASTER, &Unix_socket_group, "", NULL, NULL},

    {"unix_socket_directory", PGC_POSTMASTER, &UnixSocketDir, "", NULL, NULL},

    {"virtual_host", PGC_POSTMASTER, &VirtualHost, "", NULL, NULL},

    {"wal_sync_method", PGC_SIGHUP, &XLOG_sync_method, XLOG_sync_method_default, check_xlog_sync_method,
     assign_xlog_sync_method},

    {NULL, 0, NULL, NULL, NULL, NULL}};
